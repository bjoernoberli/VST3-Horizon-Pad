/*
    HorizonPadSoundTool
    ====================

    A small, deterministic-as-possible offline render + analysis CLI for the
    Horizon Pad synth's DSP. It links directly against the same
    HorizonPad_SharedCode static library the real VST3/Standalone plugin
    targets use (see the root CMakeLists.txt), instantiates the real
    HorizonPadAudioProcessor, feeds it synthetic MIDI, renders audio by
    calling processBlock() repeatedly (no audio device, no host, no editor),
    and prints a single JSON object of objective metrics to stdout.

    This exists so a sound-design iteration loop (change DSP source -> build
    -> render -> analyze -> compare -> repeat) can run unattended, without a
    human listening to every candidate render. See .claude/agents/sound-designer.md
    for the agent that drives this tool.

    Usage examples
    --------------
      # Solo the Root (warmFoundation) layer on a single note, default timing.
      HorizonPadSoundTool --solo=root --notes=60

      # Solo Expanse (airy choir) on a chord, write a WAV for manual listening.
      HorizonPadSoundTool --solo=expanse --notes=48,52,55 --out=/tmp/expanse.wav

      # All four layers at their default blend (no solo), with a custom macro.
      HorizonPadSoundTool --notes=60 --param=filterMacro=0.8

      # Discover the factory presets without rendering anything.
      HorizonPadSoundTool --list-presets

    Run with --help for the full flag list.

    A note on determinism: each DSP layer seeds a juce::Random per instance
    from system entropy (LayerBase::rng), used only to pick each voice's
    starting oscillator/LFO phase (an intentional anti-"digital-static"
    design choice - see WarmFoundationLayer.h etc.) so runs are NOT
    bit-exact identical. Aggregate metrics (peak, RMS, envelope shape,
    spectral energy ratios) are stable run-to-run to a small fraction of a
    percent; that stability, not byte-identical audio, is what before/after
    comparisons should rely on.
*/

#include <JuceHeader.h>

#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <optional>
#include <vector>

using namespace horizon;

//==============================================================================
namespace
{
    //--------------------------------------------------------------------
    // CLI argument parsing: a flat --key=value (or bare --flag) map.
    //--------------------------------------------------------------------
    struct Args
    {
        std::map<juce::String, juce::String> values;

        bool has (const juce::String& key) const { return values.find (key) != values.end(); }

        juce::String getString (const juce::String& key, const juce::String& fallback) const
        {
            auto it = values.find (key);
            return it == values.end() ? fallback : it->second;
        }

        float getFloat (const juce::String& key, float fallback) const
        {
            auto it = values.find (key);
            return it == values.end() || it->second.isEmpty() ? fallback : it->second.getFloatValue();
        }

        int getInt (const juce::String& key, int fallback) const
        {
            auto it = values.find (key);
            return it == values.end() || it->second.isEmpty() ? fallback : it->second.getIntValue();
        }

        std::vector<juce::String> getList (const juce::String& key, const juce::String& fallback) const
        {
            auto raw = getString (key, fallback);
            std::vector<juce::String> out;

            for (auto token : juce::StringArray::fromTokens (raw, ",", ""))
            {
                token = token.trim();
                if (token.isNotEmpty())
                    out.push_back (token);
            }

            return out;
        }
    };

    Args parseArgs (int argc, char* argv[])
    {
        Args args;

        for (int i = 1; i < argc; ++i)
        {
            juce::String arg (argv[i]);

            if (! arg.startsWith ("--"))
                continue;

            arg = arg.substring (2);
            const auto eq = arg.indexOfChar ('=');

            if (eq >= 0)
                args.values[arg.substring (0, eq)] = arg.substring (eq + 1);
            else
                args.values[arg] = "1"; // bare flag
        }

        return args;
    }

    void printUsage()
    {
        std::cout <<
            "HorizonPadSoundTool - render + analyze Horizon Pad's DSP offline\n\n"
            "Usage: HorizonPadSoundTool [options]\n\n"
            "Rendering:\n"
            "  --notes=60[,64,67]     MIDI note numbers to play as a chord (default 60)\n"
            "  --velocity=0.85        Note-on velocity, 0..1 (default 0.85)\n"
            "  --hold=4.0             Seconds from note-on to note-off (default 4.0)\n"
            "  --tail=6.0             Seconds to keep rendering after note-off,\n"
            "                         to capture release + reverb tail (default 6.0)\n"
            "  --sample-rate=48000    Render sample rate in Hz (default 48000)\n"
            "  --block=512            Block size passed to processBlock (default 512)\n\n"
            "Patch selection (applied in this order - each stage can override the last):\n"
            "  --preset=<name|index>  Apply a factory preset first (see --list-presets)\n"
            "  --solo=root,clearing   Zero every OTHER layer's volume, set these to 1.0\n"
            "                         (aliases: warmFoundation/analogEnsemble/airyChoir/\n"
            "                         motionPad, or root/clearing/expanse/bloom)\n"
            "  --param=id=value       Raw APVTS override, repeatable, e.g.\n"
            "                         --param=filterMacro=0.8 --param=reverbMacro=0\n"
            "                         Accepts either the raw ParamID or a short alias:\n"
            "                         root/clearing/expanse/bloom (volume),\n"
            "                         root-width/clearing-width/expanse-width/bloom-width,\n"
            "                         attack/release/filter/reverb (macros)\n\n"
            "Analysis:\n"
            "  --window-ms=50         Envelope RMS window size in ms (default 50)\n"
            "  --fft-size=8192        Spectral analysis window length, rounded to the\n"
            "                         nearest power of two (default 8192)\n"
            "  --click-threshold=0.2  Per-sample delta above which a discontinuity is\n"
            "                         flagged as a possible click (default 0.2)\n\n"
            "Output:\n"
            "  --out=<path.wav>       Also write the render to a 32-bit float WAV file\n"
            "  --list-presets         Print the factory preset table as JSON and exit\n"
            "  --help                 Print this message and exit\n";
    }

    //--------------------------------------------------------------------
    // Parameter ID resolution (raw ParamID string or short alias).
    //--------------------------------------------------------------------
    juce::String resolveParamId (const juce::String& nameIn)
    {
        auto name = nameIn.trim();

        static const std::map<juce::String, juce::String> aliases {
            { "root",     ParamID::rootVolume },
            { "clearing", ParamID::clearingVolume },
            { "expanse",  ParamID::expanseVolume },
            { "bloom",    ParamID::bloomVolume },
            { "attack",   ParamID::attackMacro },
            { "release",  ParamID::releaseMacro },
            { "filter",   ParamID::filterMacro },
            { "reverb",   ParamID::reverbMacro },

            // Per-layer width knobs (width moved off the shared macros and
            // onto each pad's own card - see LayerBase::setWidth()).
            { "root-width",     ParamID::rootWidth },
            { "clearing-width", ParamID::clearingWidth },
            { "expanse-width",  ParamID::expanseWidth },
            { "bloom-width",    ParamID::bloomWidth },

            // LayerIndex-style names, also accepted for --solo.
            { "warmfoundation", ParamID::rootVolume },
            { "analogensemble", ParamID::clearingVolume },
            { "airychoir",      ParamID::expanseVolume },
            { "motionpad",      ParamID::bloomVolume },
        };

        auto it = aliases.find (name.toLowerCase());
        if (it != aliases.end())
            return it->second;

        return name; // assume it's already a raw ParamID
    }

    const std::array<const char*, (size_t) kNumLayers>& allVolumeIds()
    {
        static const std::array<const char*, (size_t) kNumLayers> ids {
            ParamID::rootVolume, ParamID::clearingVolume, ParamID::expanseVolume, ParamID::bloomVolume
        };
        return ids;
    }

    //--------------------------------------------------------------------
    // Preset lookup by index or case-insensitive substring of the name.
    //--------------------------------------------------------------------
    const Preset* findPreset (const juce::String& key)
    {
        const auto& presets = getFactoryPresets();

        if (key.containsOnly ("0123456789"))
        {
            const auto idx = key.getIntValue();
            if (juce::isPositiveAndBelow (idx, (int) presets.size()))
                return &presets[(size_t) idx];
            return nullptr;
        }

        for (auto& p : presets)
            if (p.name.containsIgnoreCase (key))
                return &p;

        return nullptr;
    }

    juce::var presetToVar (const Preset& p, int index)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("index", index);
        obj->setProperty ("name", p.name);
        obj->setProperty ("description", p.description);

        auto* vols = new juce::DynamicObject();
        vols->setProperty ("root", p.volumes[(size_t) LayerIndex::warmFoundation]);
        vols->setProperty ("clearing", p.volumes[(size_t) LayerIndex::analogEnsemble]);
        vols->setProperty ("expanse", p.volumes[(size_t) LayerIndex::airyChoir]);
        vols->setProperty ("bloom", p.volumes[(size_t) LayerIndex::motionPad]);
        obj->setProperty ("volumes", juce::var (vols));

        auto* widths = new juce::DynamicObject();
        widths->setProperty ("root", p.widths[(size_t) LayerIndex::warmFoundation]);
        widths->setProperty ("clearing", p.widths[(size_t) LayerIndex::analogEnsemble]);
        widths->setProperty ("expanse", p.widths[(size_t) LayerIndex::airyChoir]);
        widths->setProperty ("bloom", p.widths[(size_t) LayerIndex::motionPad]);
        obj->setProperty ("widths", juce::var (widths));

        auto* macros = new juce::DynamicObject();
        macros->setProperty ("attack", p.macros[0]);
        macros->setProperty ("release", p.macros[1]);
        macros->setProperty ("filter", p.macros[2]);
        macros->setProperty ("reverb", p.macros[3]);
        obj->setProperty ("macros", juce::var (macros));

        return juce::var (obj);
    }

    //--------------------------------------------------------------------
    // Metrics
    //--------------------------------------------------------------------
    struct SpectralResult
    {
        bool valid = false;
        double fundamentalHz = 0.0;
        double nonHarmonicRatio = 0.0;   // 0 = perfectly harmonic, 1 = all energy non-harmonic
        double highFreqEnergyRatio = 0.0; // energy above 12kHz / total energy
        double spectralCentroidHz = 0.0;
        int windowsAveraged = 0;
        int fftSizeUsed = 0;

        struct Peak { double freqHz; double magnitude; };
        std::vector<Peak> topNonHarmonicPeaks;
    };

    int nextPowerOfTwo (int n)
    {
        int p = 1;
        while (p < n) p <<= 1;
        return juce::jmax (256, p);
    }

    SpectralResult analyzeSpectrum (const juce::AudioBuffer<float>& buffer,
                                     double sampleRate,
                                     int regionStartSample,
                                     int regionEndSample,
                                     double fundamentalHz,
                                     int requestedFftSize)
    {
        SpectralResult result;

        const auto fftSize = nextPowerOfTwo (requestedFftSize);
        const auto order = (int) std::round (std::log2 ((double) fftSize));

        const auto regionLen = regionEndSample - regionStartSample;
        if (regionLen < fftSize || fundamentalHz <= 0.0)
            return result; // not enough material - leave invalid

        // Up to 4 overlapping windows spread across the region, averaged.
        const int maxWindows = 4;
        const int numWindows = juce::jmin (maxWindows, juce::jmax (1, regionLen / (fftSize / 2)));

        juce::dsp::FFT fft (order);
        juce::dsp::WindowingFunction<float> window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann);

        std::vector<float> accum ((size_t) fftSize, 0.0f);
        std::vector<float> scratch ((size_t) (fftSize * 2), 0.0f);

        const auto* readL = buffer.getReadPointer (0);
        const auto* readR = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : readL;

        int windowsDone = 0;

        for (int w = 0; w < numWindows; ++w)
        {
            const auto step = numWindows > 1 ? (regionLen - fftSize) / (numWindows - 1) : 0;
            const auto start = regionStartSample + w * step;

            if (start < 0 || start + fftSize > buffer.getNumSamples())
                continue;

            std::fill (scratch.begin(), scratch.end(), 0.0f);

            for (int n = 0; n < fftSize; ++n)
                scratch[(size_t) n] = 0.5f * (readL[start + n] + readR[start + n]);

            window.multiplyWithWindowingTable (scratch.data(), (size_t) fftSize);
            fft.performFrequencyOnlyForwardTransform (scratch.data());

            for (int n = 0; n < fftSize; ++n)
                accum[(size_t) n] += scratch[(size_t) n];

            ++windowsDone;
        }

        if (windowsDone == 0)
            return result;

        for (auto& v : accum)
            v /= (float) windowsDone;

        const int numBins = fftSize / 2;
        const double binHz = sampleRate / (double) fftSize;

        // Mark every bin that falls within tolerance of a harmonic of the
        // fundamental (accounting for the layers' own detune/drift, a few
        // cents to a few Hz, and chorus/shimmer spread on top of that).
        std::vector<bool> isHarmonic ((size_t) numBins, false);
        const auto nyquist = sampleRate * 0.5;

        for (int k = 1; (double) k * fundamentalHz < nyquist; ++k)
        {
            const auto harmonicHz = (double) k * fundamentalHz;
            const auto toleranceHz = juce::jmax (binHz * 2.0, harmonicHz * 0.025);
            const int loBin = juce::jmax (1, (int) std::floor ((harmonicHz - toleranceHz) / binHz));
            const int hiBin = juce::jmin (numBins - 1, (int) std::ceil ((harmonicHz + toleranceHz) / binHz));

            for (int b = loBin; b <= hiBin; ++b)
                isHarmonic[(size_t) b] = true;
        }

        double totalEnergy = 0.0, harmonicEnergy = 0.0, highFreqEnergy = 0.0;
        double centroidNum = 0.0, centroidDen = 0.0;

        struct BinPeak { double freqHz; double mag; };
        std::vector<BinPeak> nonHarmonicPeaks;

        for (int b = 1; b < numBins; ++b)
        {
            const double mag = accum[(size_t) b];
            const double energy = mag * mag;
            const double freqHz = b * binHz;

            totalEnergy += energy;
            centroidNum += freqHz * mag;
            centroidDen += mag;

            if (isHarmonic[(size_t) b])
                harmonicEnergy += energy;
            else
                nonHarmonicPeaks.push_back ({ freqHz, mag });

            if (freqHz > 12000.0)
                highFreqEnergy += energy;
        }

        std::sort (nonHarmonicPeaks.begin(), nonHarmonicPeaks.end(),
                   [] (const BinPeak& a, const BinPeak& b) { return a.mag > b.mag; });

        result.valid = true;
        result.fundamentalHz = fundamentalHz;
        result.nonHarmonicRatio = totalEnergy > 1.0e-12 ? juce::jlimit (0.0, 1.0, (totalEnergy - harmonicEnergy) / totalEnergy) : 0.0;
        result.highFreqEnergyRatio = totalEnergy > 1.0e-12 ? juce::jlimit (0.0, 1.0, highFreqEnergy / totalEnergy) : 0.0;
        result.spectralCentroidHz = centroidDen > 1.0e-9 ? centroidNum / centroidDen : 0.0;
        result.windowsAveraged = windowsDone;
        result.fftSizeUsed = fftSize;

        for (size_t i = 0; i < nonHarmonicPeaks.size() && i < 5; ++i)
            result.topNonHarmonicPeaks.push_back ({ nonHarmonicPeaks[i].freqHz, nonHarmonicPeaks[i].mag });

        return result;
    }

    struct ClickReport
    {
        int count = 0;
        std::vector<double> firstTimestampsSec; // up to 10
    };

    ClickReport detectClicks (const juce::AudioBuffer<float>& buffer, double sampleRate, float threshold)
    {
        ClickReport report;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer (ch);

            for (int n = 1; n < buffer.getNumSamples(); ++n)
            {
                const auto delta = std::abs (data[n] - data[n - 1]);

                if (delta > threshold)
                {
                    ++report.count;

                    if (report.firstTimestampsSec.size() < 10)
                        report.firstTimestampsSec.push_back ((double) n / sampleRate);
                }
            }
        }

        return report;
    }
}

//==============================================================================
// The actual entry point, wrapped so main() can guarantee MessageManager's
// singleton is torn down (via deleteInstance()) on every exit path - JUCE's
// leak detector otherwise flags it (and asserts) at static-teardown time.
static int runTool (int argc, char* argv[])
{
    const auto args = parseArgs (argc, argv);

    if (args.has ("help"))
    {
        printUsage();
        return 0;
    }

    if (args.has ("list-presets"))
    {
        const auto& presets = getFactoryPresets();
        juce::Array<juce::var> arr;

        for (int i = 0; i < (int) presets.size(); ++i)
            arr.add (presetToVar (presets[(size_t) i], i));

        std::cout << juce::JSON::toString (juce::var (arr), true) << std::endl;
        return 0;
    }

    // ---- Config -------------------------------------------------------
    const double sampleRate   = (double) args.getFloat ("sample-rate", 48000.0f);
    const int    blockSize    = args.getInt ("block", 512);
    const double holdSeconds  = (double) args.getFloat ("hold", 4.0f);
    const double tailSeconds  = (double) args.getFloat ("tail", 6.0f);
    const float  velocity     = juce::jlimit (0.0f, 1.0f, args.getFloat ("velocity", 0.85f));
    const double windowMs     = (double) args.getFloat ("window-ms", 50.0f);
    const int    fftSizeReq   = args.getInt ("fft-size", 8192);
    const float  clickThresh  = args.getFloat ("click-threshold", 0.2f);
    const auto   outPath      = args.getString ("out", "");

    std::vector<int> notes;
    for (auto& tok : args.getList ("notes", "60"))
        notes.push_back (juce::jlimit (0, 127, tok.getIntValue()));

    if (notes.empty())
        notes.push_back (60);

    std::sort (notes.begin(), notes.end());

    // ---- Build the processor -------------------------------------------
    HorizonPadAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);

    // 1) Optional factory preset.
    if (args.has ("preset"))
    {
        const auto key = args.getString ("preset", "");
        const auto* preset = findPreset (key);

        if (preset == nullptr)
        {
            std::cerr << "error: no factory preset matches --preset=" << key.toStdString()
                       << " (see --list-presets)" << std::endl;
            return 1;
        }

        for (int i = 0; i < kNumLayers; ++i)
            if (auto* p = processor.getAPVTS().getParameter (allVolumeIds()[(size_t) i]))
                p->setValueNotifyingHost (p->convertTo0to1 (preset->volumes[(size_t) i]));

        static const std::array<const char*, kNumLayers> widthIds {
            ParamID::rootWidth, ParamID::clearingWidth, ParamID::expanseWidth, ParamID::bloomWidth
        };

        for (int i = 0; i < kNumLayers; ++i)
            if (auto* p = processor.getAPVTS().getParameter (widthIds[(size_t) i]))
                p->setValueNotifyingHost (p->convertTo0to1 (preset->widths[(size_t) i]));

        static const std::array<const char*, kNumGlobalParams> macroIds {
            ParamID::attackMacro, ParamID::releaseMacro, ParamID::filterMacro, ParamID::reverbMacro
        };

        for (int i = 0; i < kNumGlobalParams; ++i)
            if (auto* p = processor.getAPVTS().getParameter (macroIds[(size_t) i]))
                p->setValueNotifyingHost (p->convertTo0to1 (preset->macros[(size_t) i]));
    }

    // 2) Optional solo: zero every layer not named, set the named ones to 1.0.
    if (args.has ("solo") || args.has ("layer"))
    {
        const auto soloList = args.has ("solo") ? args.getList ("solo", "") : args.getList ("layer", "");
        std::vector<juce::String> soloIds;

        for (auto& name : soloList)
            soloIds.push_back (resolveParamId (name));

        for (auto* id : allVolumeIds())
        {
            const bool solo = std::find (soloIds.begin(), soloIds.end(), juce::String (id)) != soloIds.end();

            if (auto* p = processor.getAPVTS().getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (solo ? 1.0f : 0.0f));
        }
    }

    // 3) Raw parameter overrides, repeatable: --param=id=value. Because a
    //    single flag can only appear once in our simple parser's map, allow
    //    a semicolon-separated batch too: --param=root=1;filter=0.8
    if (args.has ("param"))
    {
        const auto raw = args.getString ("param", "");

        for (auto assignment : juce::StringArray::fromTokens (raw, ";", ""))
        {
            assignment = assignment.trim();
            if (assignment.isEmpty())
                continue;

            const auto eq = assignment.indexOfChar ('=');
            if (eq < 0)
            {
                std::cerr << "error: --param entries must be id=value, got: " << assignment.toStdString() << std::endl;
                return 1;
            }

            const auto id = resolveParamId (assignment.substring (0, eq));
            const auto value = juce::jlimit (0.0f, 1.0f, assignment.substring (eq + 1).getFloatValue());

            auto* p = processor.getAPVTS().getParameter (id);
            if (p == nullptr)
            {
                std::cerr << "error: unknown parameter id: " << id.toStdString() << std::endl;
                return 1;
            }

            p->setValueNotifyingHost (p->convertTo0to1 (value));
        }
    }

    // ---- Render ---------------------------------------------------------
    const int noteOffSample = (int) std::round (holdSeconds * sampleRate);
    const int totalSamples  = (int) std::round ((holdSeconds + tailSeconds) * sampleRate);

    juce::AudioBuffer<float> render (2, juce::jmax (1, totalSamples));
    render.clear();

    bool noteOnSent = false;
    bool noteOffSent = false;

    int position = 0;

    while (position < totalSamples)
    {
        const int blockLen = juce::jmin (blockSize, totalSamples - position);

        juce::MidiBuffer midi;

        if (! noteOnSent && position == 0)
        {
            for (auto note : notes)
                midi.addEvent (juce::MidiMessage::noteOn (1, note, velocity), 0);
            noteOnSent = true;
        }

        if (! noteOffSent && noteOffSample >= position && noteOffSample < position + blockLen)
        {
            const auto offset = noteOffSample - position;
            for (auto note : notes)
                midi.addEvent (juce::MidiMessage::noteOff (1, note), offset);
            noteOffSent = true;
        }

        juce::AudioBuffer<float> block (render.getArrayOfWritePointers(), 2, position, blockLen);
        processor.processBlock (block, midi);

        position += blockLen;
    }

    // ---- Metrics ----------------------------------------------------------
    const auto* readL = render.getReadPointer (0);
    const auto* readR = render.getNumChannels() > 1 ? render.getReadPointer (1) : readL;

    double peak = 0.0, sumSquares = 0.0, sumL = 0.0, sumR = 0.0;
    long long nanCount = 0, infCount = 0, hotSampleCount = 0; // hot = |x| >= 0.999
    bool trueClip = false;

    for (int n = 0; n < totalSamples; ++n)
    {
        for (auto s : { readL[n], readR[n] })
        {
            if (std::isnan (s)) { ++nanCount; continue; }
            if (std::isinf (s)) { ++infCount; continue; }

            const auto a = std::abs (s);
            peak = juce::jmax (peak, (double) a);
            sumSquares += (double) s * (double) s;

            if (a >= 0.999f) ++hotSampleCount;
            if (a > 1.0f) trueClip = true;
        }

        sumL += readL[n];
        sumR += readR[n];
    }

    const auto totalSampleFrames = (double) totalSamples * 2.0;
    const auto rms = totalSampleFrames > 0 ? std::sqrt (sumSquares / totalSampleFrames) : 0.0;
    const auto dcL = totalSamples > 0 ? sumL / totalSamples : 0.0;
    const auto dcR = totalSamples > 0 ? sumR / totalSamples : 0.0;

    // Envelope shape: RMS per window across the whole render.
    const int windowSamples = juce::jmax (1, (int) std::round (windowMs * 0.001 * sampleRate));
    juce::Array<double> envelope;

    for (int start = 0; start < totalSamples; start += windowSamples)
    {
        const int len = juce::jmin (windowSamples, totalSamples - start);
        double ss = 0.0;

        for (int n = start; n < start + len; ++n)
            ss += 0.5 * ((double) readL[n] * readL[n] + (double) readR[n] * readR[n]);

        envelope.add (std::sqrt (ss / (double) len));
    }

    // Spectral / aliasing analysis over the back half of the "hold" region
    // (settled sustain, away from the attack transient and the note-off).
    const int marginSamples = (int) std::round (0.05 * sampleRate);
    const int regionStart = juce::jlimit (0, noteOffSample, (int) std::round (holdSeconds * 0.5 * sampleRate));
    const int regionEnd   = juce::jmax (regionStart, noteOffSample - marginSamples);
    const double fundamentalHz = juce::MidiMessage::getMidiNoteInHertz (notes.front());

    const auto spectral = analyzeSpectrum (render, sampleRate, regionStart, regionEnd, fundamentalHz, fftSizeReq);
    const auto clicks = detectClicks (render, sampleRate, clickThresh);

    // ---- Optional WAV export ----------------------------------------------
    bool wavWritten = false;
    juce::String wavError;

    if (outPath.isNotEmpty())
    {
        juce::File outFile (outPath);
        outFile.getParentDirectory().createDirectory();
        outFile.deleteFile();

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> stream (outFile.createOutputStream());

        if (stream != nullptr)
        {
            std::unique_ptr<juce::AudioFormatWriter> writer (
                wavFormat.createWriterFor (stream.get(), sampleRate, 2, 32, {}, 0));

            if (writer != nullptr)
            {
                stream.release(); // writer now owns the stream
                writer->writeFromAudioSampleBuffer (render, 0, totalSamples);
                writer.reset(); // flush
                wavWritten = true;
            }
            else
            {
                wavError = "could not create WAV writer";
            }
        }
        else
        {
            wavError = "could not open output stream for " + outPath;
        }
    }

    // ---- Assemble JSON result ----------------------------------------------
    auto* root = new juce::DynamicObject();

    root->setProperty ("tool", "HorizonPadSoundTool");
    root->setProperty ("ok", true);

    {
        auto* config = new juce::DynamicObject();
        config->setProperty ("sampleRate", sampleRate);
        config->setProperty ("blockSize", blockSize);
        config->setProperty ("holdSeconds", holdSeconds);
        config->setProperty ("tailSeconds", tailSeconds);
        config->setProperty ("totalSeconds", (double) totalSamples / sampleRate);
        config->setProperty ("velocity", velocity);

        juce::Array<juce::var> noteArr;
        for (auto n : notes) noteArr.add (n);
        config->setProperty ("notes", noteArr);

        if (args.has ("preset")) config->setProperty ("preset", args.getString ("preset", ""));
        if (args.has ("solo"))   config->setProperty ("solo", args.getString ("solo", ""));
        if (args.has ("layer"))  config->setProperty ("solo", args.getString ("layer", ""));
        if (args.has ("param"))  config->setProperty ("paramOverrides", args.getString ("param", ""));

        root->setProperty ("config", juce::var (config));
    }

    {
        auto* params = new juce::DynamicObject();
        static const std::array<std::pair<const char*, const char*>, 12> shown {{
            { "root", ParamID::rootVolume }, { "clearing", ParamID::clearingVolume },
            { "expanse", ParamID::expanseVolume }, { "bloom", ParamID::bloomVolume },
            { "root-width", ParamID::rootWidth }, { "clearing-width", ParamID::clearingWidth },
            { "expanse-width", ParamID::expanseWidth }, { "bloom-width", ParamID::bloomWidth },
            { "attack", ParamID::attackMacro }, { "release", ParamID::releaseMacro },
            { "filter", ParamID::filterMacro }, { "reverb", ParamID::reverbMacro }
        }};

        for (auto& [alias, id] : shown)
            if (auto* p = processor.getAPVTS().getParameter (id))
                params->setProperty (alias, p->getValue());

        root->setProperty ("activeParams", juce::var (params));
    }

    {
        auto* levels = new juce::DynamicObject();
        levels->setProperty ("peak", peak);
        levels->setProperty ("peakDb", peak > 0.0 ? 20.0 * std::log10 (peak) : -300.0);
        levels->setProperty ("rms", rms);
        levels->setProperty ("rmsDb", rms > 0.0 ? 20.0 * std::log10 (rms) : -300.0);
        levels->setProperty ("dcOffsetL", dcL);
        levels->setProperty ("dcOffsetR", dcR);
        root->setProperty ("levels", juce::var (levels));
    }

    {
        auto* safety = new juce::DynamicObject();
        safety->setProperty ("nanCount", (double) nanCount);
        safety->setProperty ("infCount", (double) infCount);
        safety->setProperty ("hasNanOrInf", nanCount > 0 || infCount > 0);
        safety->setProperty ("trueClipping", trueClip); // |sample| > 1.0 (shouldn't happen - output stage is tanh-limited)
        safety->setProperty ("hotSampleCount", (double) hotSampleCount); // |sample| >= 0.999, i.e. tanh near-saturation
        safety->setProperty ("hotSampleRatio", totalSamples > 0 ? (double) hotSampleCount / (2.0 * totalSamples) : 0.0);
        safety->setProperty ("clickThreshold", clickThresh);
        safety->setProperty ("clickCount", clicks.count);

        juce::Array<juce::var> clickTimes;
        for (auto t : clicks.firstTimestampsSec) clickTimes.add (t);
        safety->setProperty ("firstClickTimestampsSec", clickTimes);

        root->setProperty ("safety", juce::var (safety));
    }

    {
        auto* env = new juce::DynamicObject();
        env->setProperty ("windowMs", windowMs);
        env->setProperty ("noteOffAtSec", (double) noteOffSample / sampleRate);

        juce::Array<juce::var> rmsArr;
        for (int i = 0; i < envelope.size(); ++i) rmsArr.add (envelope[i]);
        env->setProperty ("rmsPerWindow", rmsArr);

        root->setProperty ("envelope", juce::var (env));
    }

    {
        auto* spec = new juce::DynamicObject();
        spec->setProperty ("valid", spectral.valid);

        if (spectral.valid)
        {
            spec->setProperty ("fundamentalHz", spectral.fundamentalHz);
            spec->setProperty ("nonHarmonicEnergyRatio", spectral.nonHarmonicRatio);
            spec->setProperty ("highFreqEnergyRatioAbove12kHz", spectral.highFreqEnergyRatio);
            spec->setProperty ("spectralCentroidHz", spectral.spectralCentroidHz);
            spec->setProperty ("fftSizeUsed", spectral.fftSizeUsed);
            spec->setProperty ("windowsAveraged", spectral.windowsAveraged);

            juce::Array<juce::var> peaks;
            for (auto& pk : spectral.topNonHarmonicPeaks)
            {
                auto* po = new juce::DynamicObject();
                po->setProperty ("freqHz", pk.freqHz);
                po->setProperty ("magnitude", pk.magnitude);
                peaks.add (juce::var (po));
            }
            spec->setProperty ("topNonHarmonicPeaks", peaks);
        }
        else
        {
            spec->setProperty ("reason", "render region too short for the requested FFT size, or silent");
        }

        root->setProperty ("spectrum", juce::var (spec));
    }

    if (outPath.isNotEmpty())
    {
        auto* wav = new juce::DynamicObject();
        wav->setProperty ("path", outPath);
        wav->setProperty ("written", wavWritten);
        if (wavError.isNotEmpty()) wav->setProperty ("error", wavError);
        root->setProperty ("wav", juce::var (wav));
    }

    std::cout << juce::JSON::toString (juce::var (root), true) << std::endl;

    return 0;
}

int main (int argc, char* argv[])
{
    // MessageManager must exist (and be bound to this thread) before we touch
    // any JUCE class that uses AsyncUpdater/ChangeBroadcaster/Timer - the
    // processor is a ChangeBroadcaster and calls sendChangeMessage() from
    // setCurrentProgram()/etc. We never pump the message loop (nothing here
    // needs the callback delivered), this just avoids the "no message
    // thread" assertion. It must be explicitly torn down before exit, or
    // JUCE's leak detector fires an assertion at static-teardown time.
    juce::MessageManager::getInstance();
    const auto result = runTool (argc, argv);

    // Standard JUCE shutdown order (mirrors what JUCEApplicationBase does):
    // DeletedAtShutdown singletons first (juce_events' ShutdownDetector and
    // friends register themselves this way), then the MessageManager itself.
    juce::DeletedAtShutdown::deleteAll();
    juce::MessageManager::deleteInstance();

    return result;
}

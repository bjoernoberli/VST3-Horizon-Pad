/*
    Faust architecture file: offline, headless WAV renderer.

    Exists so the G2/G5 gates can drive `sound design/four_pads.dsp` - the
    prototype the whole C++ engine was ported from - without an audio device,
    JACK, CoreAudio or libsndfile. `faust` compiles the .dsp against this file
    and the result is a command-line program that renders a note and writes a
    32-bit float WAV.

    Deliberately not wired into the plugin's CMake build: this compiles the
    PROTOTYPE, not the product, and invariant #15 says no throwaway targets in
    the plugin build. Build it with tools/faust_render/build.sh instead.

    Usage (after building):
        four_pads_render --dur=8 --gate-off=4 --freq=261.63 --out=x.wav \
                         --set="v:Pads/[1]Warm Foundation=1" ...
        four_pads_render --list-params
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "faust/dsp/dsp.h"
#include "faust/gui/MapUI.h"
#include "faust/gui/meta.h"

<<includeIntrinsic>>
<<includeclass>>

namespace
{

// 32-bit float WAV (WAVE_FORMAT_IEEE_FLOAT). Interleaved.
bool writeWav (const std::string& path, const std::vector<float>& interleaved,
               int numChannels, int sampleRate)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (f == nullptr) return false;

    const uint32_t dataBytes = (uint32_t) (interleaved.size() * sizeof (float));
    const uint32_t byteRate  = (uint32_t) (sampleRate * numChannels * 4);
    const uint16_t blockAlign = (uint16_t) (numChannels * 4);

    auto u32 = [f] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [f] (uint16_t v) { std::fwrite (&v, 2, 1, f); };

    std::fwrite ("RIFF", 1, 4, f);  u32 (36 + dataBytes);
    std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f);  u32 (16);
    u16 (3);                                  // IEEE float
    u16 ((uint16_t) numChannels);
    u32 ((uint32_t) sampleRate);
    u32 (byteRate);
    u16 (blockAlign);
    u16 (32);                                 // bits per sample
    std::fwrite ("data", 1, 4, f);  u32 (dataBytes);
    std::fwrite (interleaved.data(), 1, dataBytes, f);
    std::fclose (f);
    return true;
}

bool argValue (const char* arg, const char* name, std::string& out)
{
    const auto n = std::strlen (name);
    if (std::strncmp (arg, name, n) == 0 && arg[n] == '=')
    {
        out = arg + n + 1;
        return true;
    }
    return false;
}

} // namespace

int main (int argc, char* argv[])
{
    int   sampleRate = 48000;
    int   blockSize  = 512;
    double duration  = 8.0;   // total render, seconds
    double gateOn    = 0.0;   // note-on time
    double gateOff   = 4.0;   // note-off time
    double freq      = 261.6255653;  // C4
    double gain      = 0.7;
    std::string outPath;
    std::vector<std::pair<std::string, double>> sets;
    bool listParams = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string v;
        if      (argValue (argv[i], "--sample-rate", v)) sampleRate = std::atoi (v.c_str());
        else if (argValue (argv[i], "--block", v))       blockSize  = std::atoi (v.c_str());
        else if (argValue (argv[i], "--dur", v))         duration   = std::atof (v.c_str());
        else if (argValue (argv[i], "--gate-on", v))     gateOn     = std::atof (v.c_str());
        else if (argValue (argv[i], "--gate-off", v))    gateOff    = std::atof (v.c_str());
        else if (argValue (argv[i], "--freq", v))        freq       = std::atof (v.c_str());
        else if (argValue (argv[i], "--gain", v))        gain       = std::atof (v.c_str());
        else if (argValue (argv[i], "--out", v))         outPath    = v;
        else if (argValue (argv[i], "--set", v))
        {
            const auto eq = v.rfind ('=');
            if (eq == std::string::npos)
            {
                std::fprintf (stderr, "--set needs path=value, got '%s'\n", v.c_str());
                return 2;
            }
            sets.emplace_back (v.substr (0, eq), std::atof (v.c_str() + eq + 1));
        }
        else if (std::strcmp (argv[i], "--list-params") == 0) listParams = true;
        else if (std::strcmp (argv[i], "--help") == 0)
        {
            std::printf ("four_pads_render - offline renderer for the Faust prototype\n\n"
                         "  --sample-rate=48000  --block=512   --dur=8      (seconds)\n"
                         "  --gate-on=0          --gate-off=4  --freq=261.63 --gain=0.7\n"
                         "  --set=\"<param path>=<value>\"  (repeatable)\n"
                         "  --out=<path.wav>     --list-params\n");
            return 0;
        }
        else
        {
            std::fprintf (stderr, "unknown argument: %s\n", argv[i]);
            return 2;
        }
    }

    mydsp DSP;
    MapUI ui;
    DSP.init (sampleRate);
    DSP.buildUserInterface (&ui);

    if (listParams)
    {
        for (const auto& kv : ui.getFullpathMap())
            std::printf ("%s\n", kv.first.c_str());
        return 0;
    }

    // Set by LABEL, not by full path: the path is prefixed with the .dsp
    // file's name, so hardcoding it would tie this architecture to one file.
    // MapUI resolves a bare label. `gain` only exists in four_pads.dsp's
    // mixer, so g5_layers.dsp legitimately does not have it.
    auto setIfPresent = [&ui] (const char* label, double value)
    {
        if (ui.getLabelMap().count (label) > 0)
            ui.setParamValue (label, (FAUSTFLOAT) value);
    };

    setIfPresent ("freq", freq);
    setIfPresent ("gain", gain);

    if (ui.getLabelMap().count ("gate") == 0)
    {
        std::fprintf (stderr, "this dsp has no 'gate' control - nothing to trigger\n");
        return 3;
    }

    for (const auto& s : sets)
    {
        // Accept either a full path or a bare label; MapUI resolves both.
        ui.setParamValue (s.first, (FAUSTFLOAT) s.second);
    }

    const auto numOut = DSP.getNumOutputs();
    const auto totalFrames = (long) (duration * sampleRate);

    std::vector<std::vector<FAUSTFLOAT>> chanBuf ((size_t) numOut,
                                                  std::vector<FAUSTFLOAT> ((size_t) blockSize, 0.0f));
    std::vector<FAUSTFLOAT*> chanPtr ((size_t) numOut);
    for (int c = 0; c < numOut; ++c) chanPtr[(size_t) c] = chanBuf[(size_t) c].data();

    std::vector<float> interleaved;
    interleaved.reserve ((size_t) (totalFrames * numOut));

    const auto gateOnFrame  = (long) (gateOn  * sampleRate);
    const auto gateOffFrame = (long) (gateOff * sampleRate);

    double peak = 0.0, sumSq = 0.0;
    long nanCount = 0;

    for (long pos = 0; pos < totalFrames; pos += blockSize)
    {
        const auto n = (int) std::min ((long) blockSize, totalFrames - pos);

        // Gate is updated on block boundaries, which is how a host delivers
        // note events anyway at this resolution.
        const auto held = (pos >= gateOnFrame && pos < gateOffFrame) ? 1.0f : 0.0f;
        ui.setParamValue ("gate", (FAUSTFLOAT) held);

        DSP.compute (n, nullptr, chanPtr.data());

        for (int i = 0; i < n; ++i)
            for (int c = 0; c < numOut; ++c)
            {
                const auto s = (float) chanBuf[(size_t) c][(size_t) i];
                if (std::isnan (s) || std::isinf (s)) ++nanCount;
                peak = std::max (peak, (double) std::fabs (s));
                sumSq += (double) s * (double) s;
                interleaved.push_back (s);
            }
    }

    const auto rms = std::sqrt (sumSq / std::max<size_t> (1, interleaved.size()));
    auto db = [] (double x) { return x > 0.0 ? 20.0 * std::log10 (x) : -300.0; };

    std::printf ("{\"tool\":\"four_pads_render\",\"sampleRate\":%d,\"frames\":%ld,"
                 "\"channels\":%d,\"peak\":%.9f,\"peakDb\":%.3f,\"rms\":%.9f,"
                 "\"rmsDb\":%.3f,\"nonFinite\":%ld}\n",
                 sampleRate, totalFrames, numOut, peak, db (peak), rms, db (rms), nanCount);

    if (! outPath.empty() && ! writeWav (outPath, interleaved, numOut, sampleRate))
    {
        std::fprintf (stderr, "could not write %s\n", outPath.c_str());
        return 1;
    }
    return 0;
}

#pragma once

#include "ToneState.h"

namespace horizon
{

/**
    Common machinery shared by the four sound layers.

    Voice allocation happens once, centrally, in the processor: every layer is
    told about the same note in the same voice slot, so a chord plays coherently
    across all four layers. Each layer keeps its own oscillator / grain state per
    voice and its own amplitude envelope (so a layer with an 8 second attack can
    still be swelling while a snappier layer has already finished).

    Rendering is done into a per-layer scratch buffer so that each layer can be
    filtered independently before being summed into the mix.

    Everything here is allocation-free once prepare() has run. prepare() is the
    only method that may allocate and is only ever called from prepareToPlay().
*/
class LayerBase
{
public:
    virtual ~LayerBase() = default;

    void prepare (double newSampleRate, int maximumBlockSize)
    {
        sampleRate = newSampleRate;
        maxBlockSize = juce::jmax (1, maximumBlockSize);

        scratch.setSize (2, maxBlockSize, false, true, false);
        scratch.clear();

        juce::dsp::ProcessSpec spec { newSampleRate, (juce::uint32) maxBlockSize, 2 };
        toneFilter.prepare (spec);
        toneFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        toneFilter.setResonance (0.55f);
        toneFilter.reset();

        for (auto& v : voices)
        {
            v.env.setSampleRate (newSampleRate);
            v.env.reset();
            v.active = false;
        }

        prepareLayer (spec);
        applyTone (tone);
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.env.reset();
            v.active = false;
        }

        scratch.clear();
        toneFilter.reset();
        resetLayer();
    }

    /** Called from the audio thread, once per block, when the tone block changed. */
    void setTone (const LayerTone& newTone)
    {
        tone = newTone;
        applyTone (newTone);
    }

    const LayerTone& getTone() const noexcept { return tone; }

    void noteOn (int voiceIndex, int midiNote, float velocity)
    {
        auto& v = voices[(size_t) voiceIndex];
        v.midiNote = midiNote;
        v.frequency = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
        v.velocity = velocity;
        v.active = true;
        v.env.noteOn();

        startVoice (voiceIndex, v.frequency, velocity);
    }

    void noteOff (int voiceIndex)
    {
        voices[(size_t) voiceIndex].env.noteOff();
    }

    /** Immediately silences a voice (used on allNotesOff / reset). */
    void killVoice (int voiceIndex)
    {
        auto& v = voices[(size_t) voiceIndex];
        v.env.reset();
        v.active = false;
    }

    /**
        Renders this layer and sums it into `target` (stereo).
        Audio thread only: no allocation, no locks, no file I/O.
    */
    void render (juce::AudioBuffer<float>& target, int numSamples)
    {
        jassert (numSamples <= scratch.getNumSamples());
        numSamples = juce::jmin (numSamples, scratch.getNumSamples());

        scratch.clear (0, numSamples);
        beginBlock (numSamples);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[(size_t) i];

            if (! v.active)
                continue;

            renderVoice (i, scratch, numSamples);

            if (! v.env.isActive())
                v.active = false;
        }

        renderLayerTail (scratch, numSamples);

        {
            juce::dsp::AudioBlock<float> block (scratch.getArrayOfWritePointers(), 2, 0, (size_t) numSamples);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            toneFilter.process (ctx);
        }

        for (int ch = 0; ch < juce::jmin (2, target.getNumChannels()); ++ch)
            target.addFrom (ch, 0, scratch, ch, 0, numSamples);
    }

    bool isVoiceActive (int voiceIndex) const noexcept { return voices[(size_t) voiceIndex].active; }

protected:
    struct Voice
    {
        bool active = false;
        int midiNote = -1;
        float frequency = 440.0f;
        float velocity = 0.0f;
        juce::ADSR env;
    };

    /** Subclass hooks. */
    virtual void prepareLayer (const juce::dsp::ProcessSpec&) {}
    virtual void resetLayer() {}
    virtual void startVoice (int /*voiceIndex*/, float /*frequency*/, float /*velocity*/) {}

    /** Called once per block before any voice is rendered. */
    virtual void beginBlock (int /*numSamples*/) {}

    virtual void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) = 0;

    /** Optional post-voice processing that must run even with no active voices. */
    virtual void renderLayerTail (juce::AudioBuffer<float>& /*target*/, int /*numSamples*/) {}

    /** Called whenever the tone block changes (and from prepare()). */
    virtual void applyTone (const LayerTone& newTone)
    {
        juce::ADSR::Parameters p;
        p.attack  = attackSeconds (newTone.attack);
        p.decay   = 0.75f;
        p.sustain = 0.85f;
        p.release = releaseSeconds (newTone.release);

        for (auto& v : voices)
            v.env.setParameters (p);

        toneFilter.setCutoffFrequency (juce::jlimit (40.0f,
                                                     (float) (sampleRate * 0.45),
                                                     toneCutoffHz (newTone.tone)));
    }

    /** Cheap band-limited-ish saw: a polyBLEP-corrected ramp. */
    static float polyBlepSaw (float phase, float phaseIncrement) noexcept
    {
        float value = 2.0f * phase - 1.0f;

        if (phase < phaseIncrement)
        {
            const auto t = phase / phaseIncrement;
            value -= (t + t - t * t - 1.0f);
        }
        else if (phase > 1.0f - phaseIncrement)
        {
            const auto t = (phase - 1.0f) / phaseIncrement;
            value -= (t * t + t + t + 1.0f);
        }

        return value;
    }

    static float wrapPhase (float phase) noexcept
    {
        while (phase >= 1.0f) phase -= 1.0f;
        while (phase < 0.0f)  phase += 1.0f;
        return phase;
    }

    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    LayerTone tone;
    std::array<Voice, (size_t) kMaxVoices> voices;

    juce::AudioBuffer<float> scratch;
    juce::dsp::StateVariableTPTFilter<float> toneFilter;

    /** Cheap noise source. Audio thread only. */
    juce::Random rng { juce::Random::getSystemRandom().nextInt64() };
};

} // namespace horizon

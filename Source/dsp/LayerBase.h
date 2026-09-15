#pragma once

#include "HorizonTypes.h"

namespace horizon
{

/**
    Common machinery shared by the four Four Pads sound layers.

    Voice allocation happens once, centrally, in the processor: every layer is
    told about the same note in the same voice slot, so a chord plays coherently
    across all four layers. Each layer keeps its own oscillator state per voice
    and its own amplitude envelope with layer-specific, fixed (non-user-editable)
    ADSR timings taken straight from the validated Faust sound design
    (four_pads.dsp) - unlike an earlier iteration of this plugin, the current
    product spec exposes no per-layer tone knobs, only the four pad volumes and
    four global macros, so each layer's character is baked in rather than
    user-editable.

    Two global macros reach every layer identically, set once per block by the
    processor:
      * attackTimeScale     - ATTACK macro. 1.0 = the layer's own designed
                               attack time; <1 snappier, >1 slower. Multiplies
                               attack only, so each layer's relative timing
                               offset (part of what keeps the four pads from
                               all arriving at once) is preserved.
      * brightnessMultiplier - FILTER macro. 1.0 = the layer's own designed
                               cutoff curve; <1 darker, >1 brighter. Multiplies
                               whatever cutoff-over-time curve the layer already
                               computes from its envelope, rather than replacing
                               it, so the "opens with the envelope" shape from
                               the sound design survives at every macro setting.

    Performance state (pitch bend, mod wheel) also reaches every layer
    identically, set once per block:
      * pitchBendSemitones  - added to every voice's frequency this block.
      * modAmount            - extra analog-style drift depth on top of each
                               layer's own baseline drift, 0 = baseline only.

    Rendering is done into a per-layer scratch buffer so that each layer can
    apply its own filtering/effects independently before being summed into the
    mix by the processor.

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

        for (auto& v : voices)
        {
            v.env.setSampleRate (newSampleRate);
            v.env.reset();
            v.active = false;
        }

        prepareLayer ({ newSampleRate, (juce::uint32) maxBlockSize, 2 });
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.env.reset();
            v.active = false;
        }

        scratch.clear();
        resetLayer();
    }

    /** Called from the audio thread, once per block. */
    void setMacros (float newAttackTimeScale, float newBrightnessMultiplier) noexcept
    {
        attackTimeScale = newAttackTimeScale;
        brightnessMultiplier = newBrightnessMultiplier;
    }

    /** Called from the audio thread, once per block. */
    void setPerformance (float newPitchBendSemitones, float newModAmount) noexcept
    {
        pitchBendSemitones = newPitchBendSemitones;
        modAmount = newModAmount;
    }

    void noteOn (int voiceIndex, int midiNote, float velocity)
    {
        auto& v = voices[(size_t) voiceIndex];
        v.midiNote = midiNote;
        v.frequency = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
        v.velocity = velocity;
        v.active = true;

        juce::ADSR::Parameters p;
        p.attack  = juce::jmax (0.001f, attackSeconds() * attackTimeScale);
        p.decay   = decaySeconds();
        p.sustain = sustainLevel();
        p.release = releaseSeconds();
        v.env.setParameters (p);
        v.env.reset();
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

    /** This layer's fixed (Faust-validated) ADSR timings, in seconds/0..1. Subclasses override. */
    virtual float attackSeconds() const noexcept = 0;
    virtual float decaySeconds() const noexcept = 0;
    virtual float sustainLevel() const noexcept = 0;
    virtual float releaseSeconds() const noexcept = 0;

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

    /** Naive triangle from a 0..1 phase - triangle's weak high-frequency content
        makes it low-alias-risk enough to skip polyBLEP correction (same call
        made elsewhere in this codebase). */
    static float triangleWave (float phase) noexcept
    {
        return 4.0f * std::abs (phase - 0.5f) - 1.0f;
    }

    static float wrapPhase (float phase) noexcept
    {
        while (phase >= 1.0f) phase -= 1.0f;
        while (phase < 0.0f)  phase += 1.0f;
        return phase;
    }

    /** Applies pitchBendSemitones to a base frequency. Call from renderVoice. */
    float bentFrequency (float baseFrequency) const noexcept
    {
        return baseFrequency * std::pow (2.0f, pitchBendSemitones / 12.0f);
    }

    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    std::array<Voice, (size_t) kMaxVoices> voices;

    juce::AudioBuffer<float> scratch;

    /** Set once per block by the processor from the ATTACK/FILTER macros. */
    float attackTimeScale = 1.0f;
    float brightnessMultiplier = 1.0f;

    /** Set once per block by the processor from MIDI/UI performance state. */
    float pitchBendSemitones = 0.0f;
    float modAmount = 0.0f;

    /** Cheap noise source, kept for any layer that still wants a bit of texture. */
    juce::Random rng { juce::Random::getSystemRandom().nextInt64() };
};

} // namespace horizon

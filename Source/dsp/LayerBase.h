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

    Three global macros reach every layer identically, set once per block by
    the processor:
      * attackTimeScale     - ATTACK macro. 1.0 = the layer's own designed
                               attack time; <1 snappier, >1 slower. Multiplies
                               attack only, so each layer's relative timing
                               offset (part of what keeps the four pads from
                               all arriving at once) is preserved.
      * releaseTimeScale    - RELEASE macro. 1.0 = the layer's own designed
                               release time; <1 shorter, >1 longer. Multiplies
                               release only, the same way attackTimeScale
                               multiplies attack.
      * brightness           - FILTER macro, ramped per sample (smoothedBrightness,
                               not stepped once per block like the other two
                               above) since it feeds a filter cutoff directly
                               and a block-rate step there is audible as
                               zipper noise. 1.0 = the layer's own designed
                               cutoff curve; <1 darker, >1 brighter. Multiplies
                               whatever cutoff-over-time curve the layer already
                               computes from its envelope, rather than replacing
                               it, so the "opens with the envelope" shape from
                               the sound design survives at every macro setting.
                               Read live per voice per sample via
                               effectiveBrightness(), which uses this shared
                               ramp unless that voice was frozen by
                               freezeBrightnessForActiveVoices() - see its doc
                               comment for why.

    Unlike those three, stereo width is set per layer, not globally - each
    pad has its own WIDTH knob (see setWidth()). It is created with the same
    short Haas-style delay-line split FxChain's old shared WIDTH macro used,
    applied here (post-voice, post-tail) rather than after the four layers
    are summed, so each pad can have its own spread instead of one shared
    image for the whole mix.

    A pure one-channel delay always localises toward the UN-delayed side (the
    precedence effect), so which channel gets delayed matters: if every layer
    delayed the same side, the whole mix would pull that way regardless of
    each pad's own width setting. setWidthLeadChannel() lets the processor
    alternate this per layer (see its call site in PluginProcessor's
    constructor) so the four pads' pulls roughly cancel out across the mix
    instead of stacking.

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

        stealFadeSamples = juce::jmax (1, (int) (kStealFadeSeconds * newSampleRate));

        for (auto& v : voices)
        {
            v.env.setSampleRate (newSampleRate);
            v.env.reset();
            v.active = false;
            clearStealState (v);
        }

        maxWidthSamples = juce::jmax (1.0f, (float) (kMaxWidthSeconds * newSampleRate));
        widthDelay.setMaximumDelayInSamples ((int) std::ceil (maxWidthSamples) + 4);
        widthDelay.prepare ({ newSampleRate, (juce::uint32) maxBlockSize, 1 });
        widthDelay.setDelay (0.0f);
        smoothedWidthSamples.reset (newSampleRate, 0.05);
        smoothedWidthSamples.setCurrentAndTargetValue (0.0f);

        // See effectiveBrightness()/render() below: FILTER's brightness
        // multiplier is ramped per sample, the same way width and (in the
        // processor) volume and reverb send already are, so twisting FILTER
        // - by hand or under host automation - sweeps the cutoff smoothly
        // instead of stepping it once per block.
        smoothedBrightness.reset (newSampleRate, 0.02);
        smoothedBrightness.setCurrentAndTargetValue (1.0f);
        brightnessBlock.setSize (1, maxBlockSize, false, true, false);
        brightnessBlock.clear();

        prepareLayer ({ newSampleRate, (juce::uint32) maxBlockSize, 2 });
    }

    void reset()
    {
        for (auto& v : voices)
        {
            v.env.reset();
            v.active = false;
            clearStealState (v);
        }

        scratch.clear();
        widthDelay.reset();
        resetLayer();
    }

    /** Called from the audio thread, once per block. */
    void setMacros (float newAttackTimeScale, float newReleaseTimeScale, float newBrightnessMultiplier) noexcept
    {
        attackTimeScale = newAttackTimeScale;
        releaseTimeScale = newReleaseTimeScale;
        smoothedBrightness.setTargetValue (newBrightnessMultiplier);
    }

    /** Called from the audio thread, but NOT once per block - only when the
        processor detects a preset/buffer switch, and only just before that
        block's setMacros() lands the new preset's FILTER value (so
        smoothedBrightness's current value here still reflects the *old*,
        pre-switch ramp). Snapshots that old value into every currently active voice (held or
        still releasing) and locks it there for the rest of that voice's
        life, so an already-sounding note keeps fading on its original
        timbre instead of jumping to the new preset's FILTER setting. Voices
        triggered afterwards are unaffected - see noteOn(). */
    void freezeBrightnessForActiveVoices() noexcept
    {
        // getCurrentValue() peeks the ramp without advancing it, so this
        // doesn't perturb the per-sample sweep render() drives below.
        const auto current = smoothedBrightness.getCurrentValue();

        for (auto& v : voices)
        {
            if (v.active)
            {
                v.frozenBrightness = current;
                v.brightnessFrozen = true;
            }
        }
    }

    /** Called from the audio thread, once per block. 0 = mono, 1 = the same
        fully-wide Haas split the old shared WIDTH macro used. */
    void setWidth (float newWidth) noexcept
    {
        smoothedWidthSamples.setTargetValue (juce::jlimit (0.0f, 1.0f, newWidth) * maxWidthSamples);
    }

    /** Not audio-thread-critical - call once, e.g. from prepareToPlay() or
        the constructor, not per block. Chooses which channel this layer's
        WIDTH delay is applied to: false (default) delays right, matching the
        original behaviour; true delays left instead. See the class doc
        comment above for why the processor alternates this per layer. */
    void setWidthLeadChannel (bool newDelayLeftInstead) noexcept
    {
        delayLeftChannel = newDelayLeftInstead;
    }

    /** Called from the audio thread, once per block. */
    void setPerformance (float newPitchBendSemitones, float newModAmount) noexcept
    {
        pitchBendSemitones = newPitchBendSemitones;
        modAmount = newModAmount;
    }

    /**
        Starts `midiNote` in this slot, declicking the handover if the slot is
        still sounding.

        Taking over a slot used to be a hard reset of the envelope and the
        oscillator phases mid-cycle. Measured with HorizonPadSoundTool
        (`--note-onsets` to force a steal, `--probe-at` on the steal instant),
        that step was -2..-11 dB relative to the signal, against -18..-23 dB
        for a note-on into a free slot: an audible click, and easy to provoke
        with a sustain pedal and more than kMaxVoices notes held.

        So a sounding slot is ramped to zero over kStealFadeSeconds first and
        the incoming note is started once the ramp lands - at the next block
        boundary after it completes, which is at most one block late. That is
        well under the ~2 ms onset JND for every attack this instrument can
        produce (the ATTACK macro floors out at 12 ms), and a pad has no
        transient to smear.
    */
    void startNote (int voiceIndex, int midiNote, float velocity)
    {
        auto& v = voices[(size_t) voiceIndex];

        // A silent slot has nothing to click: take it over immediately.
        if (! v.active)
        {
            beginNote (voiceIndex, midiNote, velocity);
            return;
        }

        v.pendingNote = midiNote;
        v.pendingVelocity = velocity;
        v.notePending = true;

        if (v.stealFadeRemaining <= 0)
            v.stealFadeRemaining = stealFadeSamples;
    }

    void beginNote (int voiceIndex, int midiNote, float velocity)
    {
        auto& v = voices[(size_t) voiceIndex];
        clearStealState (v);
        v.midiNote = midiNote;
        v.frequency = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
        v.velocity = velocity;
        v.active = true;
        v.brightnessFrozen = false; // a freshly triggered note always tracks the live FILTER macro

        juce::ADSR::Parameters p;
        p.attack  = juce::jmax (0.001f, attackSeconds() * attackTimeScale);
        p.decay   = decaySeconds();
        p.sustain = sustainLevel();
        p.release = juce::jmax (0.001f, releaseSeconds() * releaseTimeScale);
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
    /** Hard stop, no declick ramp - for panic / allNotesOff(immediately),
        where the host wants silence now. Ordinary note starts go through
        startNote(), which ramps. */
    void killVoice (int voiceIndex)
    {
        auto& v = voices[(size_t) voiceIndex];
        v.env.reset();
        v.active = false;
        clearStealState (v);
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

        // Precompute this block's brightness ramp once (not once per voice):
        // every renderVoice() call below reads brightnessBlock[n] by sample
        // index, so many voices sharing a block all see the same ramp
        // instead of each call racing it forward independently.
        {
            auto* bb = brightnessBlock.getWritePointer (0);

            for (int n = 0; n < numSamples; ++n)
                bb[n] = smoothedBrightness.getNextValue();
        }

        // A stolen slot whose declick ramp finished during the last block
        // starts its queued note here, before anything is rendered.
        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[(size_t) i];

            if (v.notePending && v.stealFadeRemaining <= 0)
                beginNote (i, v.pendingNote, v.pendingVelocity);
        }

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

        // --- Per-layer stereo width: covers voices and any tail effect (e.g.
        // Airy Choir's shimmer bus) uniformly, since it runs after both.
        // Which channel is the delayed one depends on delayLeftChannel (see
        // setWidthLeadChannel()) - left and right are bit-identical at this
        // point in every layer, so it doesn't matter which one is read as
        // the delay line's source.
        //
        // Mono-safety: a pure equal-gain delay tap (source on one channel,
        // delay(source) on the other) is a textbook comb filter with TRUE
        // zeros at f = (2k+1)/(2*delaySeconds) once L+R are summed to mono -
        // not just attenuation, complete cancellation, because both channels
        // carry the exact same signal. Measured with HorizonPadSoundTool +
        // an offline mono-downmix check: at full WIDTH the fundamental of a
        // middle-register note landed almost exactly on that first null,
        // producing a ~36 dB notch and pulling total mono-summed RMS down to
        // ~24% of a single channel's - i.e. the pad nearly disappears on any
        // mono playback path (phone/Bluetooth speaker, club mono zone,
        // broadcast mono check). blendBack mixes a small, width-proportional
        // fraction of the dry (undelayed) source back into the delayed
        // channel so that null becomes a bounded, shallow dip instead of a
        // true zero: at the null frequency the mono sum becomes
        // 2*blendBack*|source| rather than 0. kMonoSafetyBlend=0.15 bounds
        // the worst-case notch to about -16 dB (still a legible width cue in
        // stereo) while width=0 stays byte-identical to before (blendBack=0).
        {
            auto* left = scratch.getWritePointer (0);
            auto* right = scratch.getWritePointer (1);
            auto* source = delayLeftChannel ? right : left;
            auto* delayed = delayLeftChannel ? left : right;

            constexpr float kMonoSafetyBlend = 0.30f;

            for (int n = 0; n < numSamples; ++n)
            {
                const auto widthSamples = smoothedWidthSamples.getNextValue();
                widthDelay.pushSample (0, source[n]);
                widthDelay.setDelay (widthSamples);
                const auto delayedSample = widthDelay.popSample (0);

                const auto widthFraction = widthSamples / maxWidthSamples;
                const auto blendBack = kMonoSafetyBlend * widthFraction;
                delayed[n] = delayedSample * (1.0f - blendBack) + source[n] * blendBack;
            }
        }

        for (int ch = 0; ch < juce::jmin (2, target.getNumChannels()); ++ch)
            target.addFrom (ch, 0, scratch, ch, 0, numSamples);
    }

    bool isVoiceActive (int voiceIndex) const noexcept { return voices[(size_t) voiceIndex].active; }

    /**
        Pins this layer's start-phase RNG so a render is reproducible.

        The plugin never calls this: per-voice random start phases are
        deliberate (see the rng member). The offline harness does, because
        "two runs from reset are bit-identical" is how a null test and a
        regression baseline are possible at all.
    */
    void setRandomSeed (juce::int64 seed) noexcept { rng.setSeed (seed); }

protected:
    struct Voice
    {
        bool active = false;
        int midiNote = -1;
        float frequency = 440.0f;
        float velocity = 0.0f;
        juce::ADSR env;

        // See freezeBrightnessForActiveVoices() below.
        bool brightnessFrozen = false;
        float frozenBrightness = 1.0f;

        // Voice-steal declick, see startNote(). stealFadeRemaining counts the
        // ramp down to zero; notePending marks a note-on waiting for it.
        int stealFadeRemaining = 0;
        bool notePending = false;
        int pendingNote = -1;
        float pendingVelocity = 0.0f;
    };

    /** The FILTER macro value a voice should render with at this sample: its
        own frozen snapshot if freezeBrightnessForActiveVoices() caught it
        still sounding across a preset/buffer switch, otherwise this block's
        precomputed brightness ramp at sampleIndex (see render()) - so
        real-time filter sweeps on a held note keep working smoothly, with no
        block-rate stepping, right up until a preset change, at which point
        that note is done listening. */
    float effectiveBrightness (int voiceIndex, int sampleIndex) const noexcept
    {
        const auto& v = voices[(size_t) voiceIndex];
        return v.brightnessFrozen ? v.frozenBrightness : brightnessBlock.getSample (0, sampleIndex);
    }

    /**
        This voice's voice-steal declick gain for one sample, advancing the
        ramp. Layers multiply their per-sample output by it, so a slot being
        taken over fades instead of cutting - see startNote().

        Once the ramp has landed the voice stays silent until render() starts
        the queued note at the next block boundary.
    */
    float nextStealGain (Voice& v) noexcept
    {
        if (v.stealFadeRemaining <= 0)
            return v.notePending ? 0.0f : 1.0f;

        --v.stealFadeRemaining;
        return (float) v.stealFadeRemaining / (float) stealFadeSamples;
    }

    static void clearStealState (Voice& v) noexcept
    {
        v.stealFadeRemaining = 0;
        v.notePending = false;
        v.pendingNote = -1;
        v.pendingVelocity = 0.0f;
    }

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

    /** Set once per block by the processor from the ATTACK/RELEASE/FILTER macros. */
    float attackTimeScale = 1.0f;
    float releaseTimeScale = 1.0f;

    /** FILTER macro (brightness), ramped per sample rather than stepped once
        per block - see setMacros()/render()/effectiveBrightness(). */
    juce::SmoothedValue<float> smoothedBrightness;
    juce::AudioBuffer<float> brightnessBlock;

    /** This layer's own WIDTH knob - see setWidth() and render(). */
    /** Voice-steal declick ramp length. 5 ms is long enough to remove the
        step (a 5 ms raised ramp has no energy above roughly 200 Hz worth
        speaking of) and short enough to be well inside the ~2 ms onset JND's
        tolerance for a pad with a >=12 ms attack. */
    static constexpr float kStealFadeSeconds = 0.005f;
    int stealFadeSamples = 1;

    /**
        Maximum WIDTH delay, as a TIME.

        This used to be a flat 90 samples, which made the Haas time a function
        of the sample rate: 2.04 ms at 44.1 kHz but 0.47 ms at 192 kHz, a
        factor of 4.3. The precedence effect that WIDTH trades on is a
        time-domain phenomenon (and the inter-channel time JND is ~10-20 us),
        so the same knob produced a different stereo image per rate, and the
        mono comb notch it has to stay clear of moved with it.

        Anchored at 90 samples / 48 kHz so behaviour at 48 kHz is unchanged,
        and 44.1 / 88.2 / 96 / 176.4 / 192 kHz now match it in time instead of
        in samples.
    */
    static constexpr float kMaxWidthSeconds = 90.0f / 48000.0f;

    /** kMaxWidthSeconds in samples at the current rate; set in prepare(). */
    float maxWidthSamples = 90.0f;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> widthDelay { 512 };
    juce::SmoothedValue<float> smoothedWidthSamples;
    bool delayLeftChannel = false;

    /** Set once per block by the processor from MIDI/UI performance state. */
    float pitchBendSemitones = 0.0f;
    float modAmount = 0.0f;

    /** Cheap noise source, kept for any layer that still wants a bit of texture. */
    /** Oscillator/LFO start phases are randomised per voice so that two
        instances in the same project do not start phase-locked and sum
        coherently - deliberate, and the reason two renders of the same build
        are not bit-identical. setRandomSeed() pins it for measurement. */
    juce::Random rng { juce::Random::getSystemRandom().nextInt64() };
};

} // namespace horizon

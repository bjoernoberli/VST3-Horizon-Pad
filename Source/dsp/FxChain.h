#pragma once

#include "HorizonTypes.h"

namespace horizon
{

/**
    Global post-mix stage: the shared REVERB send, ported from the tail end of
    four_pads.dsp.

    The Faust source sums all four pads into one MONO `dry` signal and adds
    one shared room send:

        wetMono = dry : re.mono_freeverb(0.7, 0.35, 0.25, 0) * reverbSend
        outL    = dry * 0.85 + wetMono
        outR    = dry * 0.85 + wetMono

    Stereo width used to be created right here too (a shared WIDTH macro
    driving a Haas-style delay split between channels), but each of the four
    layers now has its own WIDTH knob instead (see LayerBase::setWidth()) -
    so by the time a buffer reaches this stage it may already carry real,
    distinct left/right content, and this stage's job is only to preserve
    that image while adding the shared reverb on top, not to rebuild it from
    a mono sum the way the old single WIDTH macro did.

    REVERB (0..1) is the wet send level; 0.28 was the fixed value the sound
    design was validated against, kept here as the parameter's default.
    Dry and wet are combined with an equal-power (cos/sin) crossfade so
    total output RMS stays flat as REVERB sweeps 0->1, instead of the wet
    tail just adding on top of a still-full-level dry signal - see the
    comment in process() and kWetCalibrationGain below for why a plain
    linear crossfade isn't enough on its own.
*/
class FxChain
{
public:
    FxChain() = default;

    void prepare (double sampleRate, int maximumBlockSize);
    void reset();

    /** Message-thread-free: call from processBlock with the current REVERB macro value (0..1). */
    void setParameters (float reverbSend) noexcept;

    void process (juce::AudioBuffer<float>& buffer, int numSamples);

private:
    // juce::dsp::Reverb driven fully wet (see prepare()) puts out roughly
    // 2x the RMS energy of the mono signal fed into it, for the roomSize/
    // damping this synth uses on typical sustained pad material - measured
    // with HorizonPadSoundTool (fixed note, dry vs. fully-wet RMS at
    // REVERB=1) rather than derived from the freeverb coefficients. This
    // scales the wet path back down so it's level-matched to dry *before*
    // the equal-power crossfade in process() runs, so that crossfade's
    // constant-total-power guarantee actually holds instead of being
    // swamped by the wet path's own extra gain.
    //
    // Re-verified 2026-09-20 (DSP invariant audit, #8): a solo'd single-layer
    // sweep of REVERB 0->1 (5 renders per point) holds RMS flat within noise
    // (0.0263 / 0.0244 / 0.0256 at 0 / 0.5 / 1.0, stdev ~5-8% per point) at
    // this value. G=0.2 or G=0.35 both under-shoot noticeably (0.0263->0.0104
    // and 0.0263->0.0182) - do not "fix" this constant on the basis of a
    // full-4-layer-mix sweep without averaging several renders per point
    // first: that configuration's many near-unison, independently-phased
    // oscillators (LayerBase::rng-seeded per voice) beat against each other
    // on a multi-second cycle, and a single 10 s render can land anywhere in
    // that cycle - single-sample sweeps on the full mix swung this same
    // measurement by 2-3x run to run, which is what first (wrongly) flagged
    // this constant as miscalibrated during that audit.
    static constexpr float kWetCalibrationGain = 0.48f;

    double currentSampleRate = 44100.0;

    juce::dsp::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    juce::AudioBuffer<float> dryMono;

    // Genuinely 2 channels, fed the same mono sum on both sides: JUCE's
    // Reverb only decorrelates left/right (its stereo-spread comb/allpass
    // tunings) when it's driven through processStereo(), which only happens
    // when given a 2-channel block - a 1-channel block silently falls back
    // to processMono() and the identical mono tail then gets copied onto
    // both output channels by hand below, which is what made the old tail
    // sound flat/mono instead of spacious.
    juce::AudioBuffer<float> wetStereo;

    juce::SmoothedValue<float> smoothedReverbSend;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxChain)
};

} // namespace horizon

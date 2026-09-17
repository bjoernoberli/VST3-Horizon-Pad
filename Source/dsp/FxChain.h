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
    design was validated against, kept here as the parameter's default. The
    dry level is trimmed down slightly as REVERB rises (0.85 -> ~0.60) so
    raising the send reads as a wet/dry blend rather than a pure loudness
    increase - see the comment in process().
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

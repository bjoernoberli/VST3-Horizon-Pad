#pragma once

#include "HorizonTypes.h"

namespace horizon
{

/**
    Global post-mix stage: WIDTH -> REVERB, ported from the tail end of
    four_pads.dsp.

    The Faust source sums all four pads into one MONO `dry` signal, then
    builds stereo purely from a short Haas-style delay difference between
    channels (a fixed 0/90-sample split), and adds one shared room send:

        wetMono = dry : re.mono_freeverb(0.7, 0.35, 0.25, 0) * reverbSend
        outL    = dry               * 0.85 + wetMono
        outR    = delay(dry, width) * 0.85 + wetMono

    WIDTH (0..1) scales that delay split from mono (0) up to the fully-wide
    90-sample split the sound design was tuned and validated against (1) -
    the DSP layers upstream already write identical left/right (see
    LayerBase - each pad is authored as a mono voice, exactly like the Faust
    source), so this is the only place stereo width is created at all.

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

    /** Message-thread-free: call from processBlock with the current macro values (0..1). */
    void setParameters (float width, float reverbSend) noexcept;

    void process (juce::AudioBuffer<float>& buffer, int numSamples);

private:
    static constexpr int kMaxWidthSamples = 90;

    double currentSampleRate = 44100.0;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> widthDelay { kMaxWidthSamples + 4 };
    juce::dsp::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    juce::AudioBuffer<float> dryMono;
    juce::AudioBuffer<float> wetStereo;

    juce::SmoothedValue<float> smoothedWidthSamples;
    juce::SmoothedValue<float> smoothedReverbSend;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxChain)
};

} // namespace horizon

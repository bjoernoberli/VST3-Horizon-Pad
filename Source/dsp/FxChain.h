#pragma once

#include "ToneState.h"

namespace horizon
{

/**
    Global post-mix effect chain: FILTER -> DELAY -> REVERB.

    Driven by the four global host parameters (all 0..1 here; the UI shows them
    as 0..100%).

    ---------------------------------------------------------------------------
    FX AMOUNT mapping (the "how much of all this do I want" macro)
    ---------------------------------------------------------------------------
    FX Amount is a single depth macro over the whole wet side of the chain. The
    rule is: at fxAmount = 0 the chain is audibly transparent regardless of the
    other three knobs; at fxAmount = 1 the other three knobs act at full value.

      * Filter  - the Filter parameter is a brightness macro where 1 = wide
                  open. Rather than scaling the cutoff (which would make
                  fxAmount = 0 sound *dark*), fxAmount scales how far the filter
                  is allowed to close:

                      effectiveFilter = 1 - fxAmount * (1 - filter)

                  so fxAmount = 0 pins the cutoff wide open (no effect) and
                  fxAmount = 1 gives the user exactly the cutoff they dialled.
                  effectiveFilter then maps exponentially onto 200 Hz..18 kHz.

      * Delay   - wet mix = delay * fxAmount. Feedback is also gently scaled
                  (0.18 .. 0.62 by the delay knob) so a high Delay setting means
                  both "more repeats" and "louder repeats".

      * Reverb  - wet = reverb * fxAmount, and the reverb room size tracks the
                  Reverb knob (0.35 .. 0.95) so turning it up makes the space
                  bigger as well as wetter. Dry is kept at 1 - 0.65 * wet so the
                  source never disappears completely.

    Every one of these targets is smoothed (juce::SmoothedValue, 50 ms ramp) and
    updated once per block, so parameter moves and preset switches never click.
    ---------------------------------------------------------------------------
*/
class FxChain
{
public:
    FxChain() = default;

    void prepare (double sampleRate, int maximumBlockSize);
    void reset();

    /** Message-thread-free: call from processBlock with the current parameter values. */
    void setParameters (float reverb, float delay, float filter, float fxAmount) noexcept;

    void process (juce::AudioBuffer<float>& buffer, int numSamples);

private:
    static constexpr float kMaxDelaySeconds = 1.5f;
    static constexpr float kDelayTimeSeconds = 0.44f;   // musical-ish default, no host sync

    double currentSampleRate = 44100.0;

    juce::dsp::StateVariableTPTFilter<float> filter;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine
        { 1 }; // real size set in prepare()
    juce::dsp::StateVariableTPTFilter<float> delayDamping;

    juce::dsp::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    juce::AudioBuffer<float> wetScratch;

    juce::SmoothedValue<float> smoothedCutoff;
    juce::SmoothedValue<float> smoothedDelayMix;
    juce::SmoothedValue<float> smoothedDelayFeedback;
    juce::SmoothedValue<float> smoothedReverbWet;

    float lastReverbSize = -1.0f;
    bool reverbIdle = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxChain)
};

} // namespace horizon

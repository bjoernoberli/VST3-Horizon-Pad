#include "FxChain.h"

namespace horizon
{

void FxChain::prepare (double sampleRate, int maximumBlockSize)
{
    currentSampleRate = sampleRate;

    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, maximumBlockSize), 2 };

    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setResonance (0.7f);

    delayLine.setMaximumDelayInSamples ((int) (kMaxDelaySeconds * sampleRate) + 4);
    delayLine.prepare (spec);
    delayLine.setDelay ((float) (kDelayTimeSeconds * sampleRate));

    delayDamping.prepare (spec);
    delayDamping.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    delayDamping.setResonance (0.5f);
    delayDamping.setCutoffFrequency (4200.0f); // tape-ish repeats, each one darker

    reverb.prepare (spec);

    wetScratch.setSize (2, juce::jmax (1, maximumBlockSize), false, true, false);

    const auto ramp = 0.05; // 50 ms
    smoothedCutoff.reset (sampleRate, ramp);
    smoothedDelayMix.reset (sampleRate, ramp);
    smoothedDelayFeedback.reset (sampleRate, ramp);
    smoothedReverbWet.reset (sampleRate, ramp);

    smoothedCutoff.setCurrentAndTargetValue (18000.0f);
    smoothedDelayMix.setCurrentAndTargetValue (0.0f);
    smoothedDelayFeedback.setCurrentAndTargetValue (0.3f);
    smoothedReverbWet.setCurrentAndTargetValue (0.0f);

    reset();
}

void FxChain::reset()
{
    filter.reset();
    delayLine.reset();
    delayDamping.reset();
    reverb.reset();
    reverbIdle = true;
    wetScratch.clear();
}

void FxChain::setParameters (float reverbAmount, float delayAmount, float filterAmount, float fxAmount) noexcept
{
    reverbAmount = juce::jlimit (0.0f, 1.0f, reverbAmount);
    delayAmount  = juce::jlimit (0.0f, 1.0f, delayAmount);
    filterAmount = juce::jlimit (0.0f, 1.0f, filterAmount);
    fxAmount     = juce::jlimit (0.0f, 1.0f, fxAmount);

    // See the header comment for the reasoning behind this mapping.
    const auto effectiveFilter = 1.0f - fxAmount * (1.0f - filterAmount);
    const auto cutoff = 200.0f * std::pow (90.0f, effectiveFilter); // 200 Hz .. 18 kHz
    smoothedCutoff.setTargetValue (juce::jlimit (60.0f, (float) (currentSampleRate * 0.45), cutoff));

    smoothedDelayMix.setTargetValue (delayAmount * fxAmount);
    smoothedDelayFeedback.setTargetValue (0.18f + 0.44f * delayAmount);
    smoothedReverbWet.setTargetValue (reverbAmount * fxAmount);

    const auto size = 0.35f + 0.60f * reverbAmount;

    if (std::abs (size - lastReverbSize) > 1.0e-4f)
    {
        lastReverbSize = size;
        reverbParams.roomSize = size;
        reverbParams.damping = 0.42f;
        reverbParams.width = 1.0f;
        reverbParams.freezeMode = 0.0f;
        // The reverb module runs fully wet here; the dry/wet blend is done by
        // hand below so that FX Amount controls it.
        reverbParams.wetLevel = 1.0f;
        reverbParams.dryLevel = 0.0f;
        reverb.setParameters (reverbParams);
    }
}

void FxChain::process (juce::AudioBuffer<float>& buffer, int numSamples)
{
    const auto numChannels = juce::jmin (2, buffer.getNumChannels());

    if (numChannels == 0 || numSamples <= 0)
        return;

    numSamples = juce::jmin (numSamples, wetScratch.getNumSamples());

    auto* left  = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : left;

    // ---------------------------------------------------------------- FILTER
    {
        // Per-block cutoff update; the SmoothedValue supplies the ramp so a
        // fast knob move does not zipper.
        filter.setCutoffFrequency (smoothedCutoff.skip (numSamples));

        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(),
                                            (size_t) numChannels, 0, (size_t) numSamples);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        filter.process (ctx);
    }

    // ----------------------------------------------------------------- DELAY
    for (int n = 0; n < numSamples; ++n)
    {
        const auto mix = smoothedDelayMix.getNextValue();
        const auto fb  = smoothedDelayFeedback.getNextValue();

        const auto inL = left[n];
        const auto inR = right[n];

        const auto dL = delayLine.popSample (0);
        const auto dR = delayLine.popSample (1);

        // Cross-feed the repeats for a wide, drifting ping-pong-ish tail.
        const auto fbL = delayDamping.processSample (0, inL + dR * fb);
        const auto fbR = delayDamping.processSample (1, inR + dL * fb);

        delayLine.pushSample (0, fbL);
        delayLine.pushSample (1, fbR);

        left[n]  = inL + dL * mix;
        right[n] = inR + dR * mix;
    }

    // ---------------------------------------------------------------- REVERB
    {
        const auto wet = smoothedReverbWet.skip (numSamples);

        if (wet > 1.0e-4f)
        {
            reverbIdle = false;

            for (int ch = 0; ch < numChannels; ++ch)
                wetScratch.copyFrom (ch, 0, buffer, ch, 0, numSamples);

            if (numChannels == 1)
                wetScratch.copyFrom (1, 0, buffer, 0, 0, numSamples);

            juce::dsp::AudioBlock<float> wetBlock (wetScratch.getArrayOfWritePointers(),
                                                    2, 0, (size_t) numSamples);
            juce::dsp::ProcessContextReplacing<float> ctx (wetBlock);
            reverb.process (ctx);

            // Dry is only partially ducked so the pad keeps its front edge.
            const auto dryGain = 1.0f - 0.65f * wet;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* dst = buffer.getWritePointer (ch);
                const auto* src = wetScratch.getReadPointer (ch);

                for (int n = 0; n < numSamples; ++n)
                    dst[n] = dst[n] * dryGain + src[n] * wet;
            }
        }
        else
        {
            // Fully dry: flush the tail exactly once so that turning the reverb
            // up later does not reveal stale audio, then leave it alone.
            if (! reverbIdle)
            {
                reverbIdle = true;
                reverb.reset();
            }
        }
    }
}

} // namespace horizon

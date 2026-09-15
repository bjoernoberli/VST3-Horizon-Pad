#include "FxChain.h"

namespace horizon
{

void FxChain::prepare (double sampleRate, int maximumBlockSize)
{
    currentSampleRate = sampleRate;

    const juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) juce::jmax (1, maximumBlockSize), 1 };

    widthDelay.prepare (monoSpec);
    widthDelay.setDelay (0.0f);

    reverb.prepare (monoSpec);
    reverbParams.roomSize = 0.7f;
    reverbParams.damping = 0.35f;
    reverbParams.width = 1.0f;
    reverbParams.wetLevel = 1.0f;   // fully wet; the send-level blend happens by hand below
    reverbParams.dryLevel = 0.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);

    dryMono.setSize (1, juce::jmax (1, maximumBlockSize), false, true, false);
    wetStereo.setSize (1, juce::jmax (1, maximumBlockSize), false, true, false);

    const auto ramp = 0.05; // 50 ms
    smoothedWidthSamples.reset (sampleRate, ramp);
    smoothedReverbSend.reset (sampleRate, ramp);
    smoothedWidthSamples.setCurrentAndTargetValue ((float) kMaxWidthSamples);
    smoothedReverbSend.setCurrentAndTargetValue (0.28f);

    reset();
}

void FxChain::reset()
{
    widthDelay.reset();
    reverb.reset();
    dryMono.clear();
    wetStereo.clear();
}

void FxChain::setParameters (float width, float reverbSend) noexcept
{
    width = juce::jlimit (0.0f, 1.0f, width);
    reverbSend = juce::jlimit (0.0f, 1.0f, reverbSend);

    smoothedWidthSamples.setTargetValue (width * (float) kMaxWidthSamples);
    smoothedReverbSend.setTargetValue (reverbSend);
}

void FxChain::process (juce::AudioBuffer<float>& buffer, int numSamples)
{
    const auto numChannels = juce::jmin (2, buffer.getNumChannels());

    if (numChannels == 0 || numSamples <= 0)
        return;

    numSamples = juce::jmin (numSamples, dryMono.getNumSamples());

    // The layers upstream write identical L/R (each pad is a mono voice, as in
    // the Faust source), so channel 0 is the authoritative dry mono sum.
    auto* dry = dryMono.getWritePointer (0);
    juce::FloatVectorOperations::copy (dry, buffer.getReadPointer (0), numSamples);

    auto* wet = wetStereo.getWritePointer (0);
    juce::FloatVectorOperations::copy (wet, dry, numSamples);

    {
        juce::dsp::AudioBlock<float> block (wetStereo.getArrayOfWritePointers(), 1, 0, (size_t) numSamples);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        reverb.process (ctx);
    }

    auto* left  = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : left;

    for (int n = 0; n < numSamples; ++n)
    {
        const auto widthSamples = smoothedWidthSamples.getNextValue();
        const auto reverbSend = smoothedReverbSend.getNextValue();
        const auto wetSample = wet[n] * reverbSend;

        widthDelay.pushSample (0, dry[n]);
        widthDelay.setDelay (widthSamples);
        const auto delayed = widthDelay.popSample (0);

        left[n]  = dry[n]  * 0.85f + wetSample;
        right[n] = delayed * 0.85f + wetSample;
    }
}

} // namespace horizon

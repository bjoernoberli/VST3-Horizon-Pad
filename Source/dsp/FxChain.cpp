#include "FxChain.h"

namespace horizon
{

void FxChain::prepare (double sampleRate, int maximumBlockSize)
{
    currentSampleRate = sampleRate;

    const juce::dsp::ProcessSpec stereoSpec { sampleRate, (juce::uint32) juce::jmax (1, maximumBlockSize), 2 };

    reverb.prepare (stereoSpec);
    reverbParams.roomSize = 0.7f;
    reverbParams.damping = 0.35f;
    reverbParams.width = 1.0f;
    reverbParams.wetLevel = 1.0f;   // fully wet; the send-level blend happens by hand below
    reverbParams.dryLevel = 0.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);

    dryMono.setSize (1, juce::jmax (1, maximumBlockSize), false, true, false);
    wetStereo.setSize (2, juce::jmax (1, maximumBlockSize), false, true, false);

    const auto ramp = 0.05; // 50 ms
    smoothedReverbSend.reset (sampleRate, ramp);
    smoothedReverbSend.setCurrentAndTargetValue (0.28f);

    reset();
}

void FxChain::reset()
{
    reverb.reset();
    dryMono.clear();
    wetStereo.clear();
}

void FxChain::setParameters (float reverbSend) noexcept
{
    reverbSend = juce::jlimit (0.0f, 1.0f, reverbSend);
    smoothedReverbSend.setTargetValue (reverbSend);
}

void FxChain::process (juce::AudioBuffer<float>& buffer, int numSamples)
{
    const auto numChannels = juce::jmin (2, buffer.getNumChannels());

    if (numChannels == 0 || numSamples <= 0)
        return;

    numSamples = juce::jmin (numSamples, dryMono.getNumSamples());

    auto* left  = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : left;

    // Each layer now has its own width, so left/right may genuinely differ by
    // this point - the reverb send is still built from a mono sum (a stereo
    // room send from two already-different channels would just smear the
    // image), but the dry path below preserves left/right exactly as they
    // arrived rather than rebuilding one from the other.
    auto* dry = dryMono.getWritePointer (0);
    juce::FloatVectorOperations::copy (dry, left, numSamples);

    if (right != left)
        for (int n = 0; n < numSamples; ++n)
            dry[n] = 0.5f * (dry[n] + right[n]);

    // Both wet channels start from the same mono sum - the stereo image in
    // the tail comes entirely from Reverb's own internal stereo-spread comb
    // filters once it's driven through processStereo() (see the member
    // comment in FxChain.h), not from feeding it different input per side.
    auto* wetL = wetStereo.getWritePointer (0);
    auto* wetR = wetStereo.getWritePointer (1);
    juce::FloatVectorOperations::copy (wetL, dry, numSamples);
    juce::FloatVectorOperations::copy (wetR, dry, numSamples);

    {
        juce::dsp::AudioBlock<float> block (wetStereo.getArrayOfWritePointers(), 2, 0, (size_t) numSamples);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        reverb.process (ctx);
    }

    for (int n = 0; n < numSamples; ++n)
    {
        const auto reverbSend = smoothedReverbSend.getNextValue();
        const auto wetSampleL = wetL[n] * reverbSend;
        const auto wetSampleR = wetR[n] * reverbSend;

        // Turning REVERB up adds a decorrelated tail on top of the dry
        // signal, which raises total output energy if the dry path stays at
        // a fixed level - that's the "reverb makes it louder" complaint. Trim
        // dry down a little as the send rises (0.85 at 0%, ~0.60 at 100%) so
        // the knob reads as a wet/dry blend rather than a pure add; at
        // REVERB=0 this is exactly the original fixed 0.85 trim, so the
        // un-reverbed sound is unchanged.
        const auto dryLevel = 0.85f * (1.0f - 0.3f * reverbSend);

        left[n]  = left[n]  * dryLevel + wetSampleL;
        right[n] = right[n] * dryLevel + wetSampleR;
    }
}

} // namespace horizon

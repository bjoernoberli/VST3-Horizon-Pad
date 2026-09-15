#include "AiryChoirLayer.h"

namespace horizon
{

void AiryChoirLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    const juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) maxBlockSize, 1 };

    for (auto& vs : voiceState)
    {
        vs.bandpass.prepare (monoSpec);
        vs.bandpass.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        vs.bandpass.setResonance (1.6f);

        vs.safetyLowpass.prepare (monoSpec);
        vs.safetyLowpass.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        vs.safetyLowpass.setResonance (0.5f);
        vs.safetyLowpass.setCutoffFrequency (1800.0f);
    }

    shimmerBus.setSize (1, maxBlockSize, false, true, false);

    shimmerHighpass.prepare (monoSpec);
    shimmerHighpass.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    shimmerHighpass.setResonance (0.6f);
    shimmerHighpass.setCutoffFrequency (1200.0f);

    shimmerPitch.prepare (sampleRate);

    shimmerReverb.prepare (monoSpec);
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.45f;
    rp.damping = 0.6f;   // darkened per later taming passes on this pad's tail
    rp.width = 1.0f;
    rp.wetLevel = 1.0f;  // fully wet here; the 0.16 blend happens in renderLayerTail
    rp.dryLevel = 0.0f;
    rp.freezeMode = 0.0f;
    shimmerReverb.setParameters (rp);

    resetLayer();
}

void AiryChoirLayer::resetLayer()
{
    for (auto& vs : voiceState)
    {
        vs.phase.fill (0.0f);
        vs.driftPhase.fill (0.0f);
        vs.sweepPhase = 0.0f;
        vs.bandpass.reset();
        vs.safetyLowpass.reset();
    }

    shimmerBus.clear();
    shimmerHighpass.reset();
    shimmerPitch.reset();
    shimmerReverb.reset();
}

void AiryChoirLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];

    for (int i = 0; i < kNumOscs; ++i)
    {
        vs.phase[(size_t) i] = rng.nextFloat();
        vs.driftPhase[(size_t) i] = rng.nextFloat();
    }

    vs.sweepPhase = rng.nextFloat();
    vs.bandpass.reset();
    vs.safetyLowpass.reset();
}

void AiryChoirLayer::beginBlock (int numSamples)
{
    shimmerBus.clear (0, juce::jmin (numSamples, shimmerBus.getNumSamples()));
}

void AiryChoirLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);
    auto* shimmerIn = shimmerBus.getWritePointer (0);
    const auto shimmerLen = juce::jmin (numSamples, shimmerBus.getNumSamples());

    const auto invSr = 1.0f / (float) sampleRate;
    const auto driftDepth = 0.004f + modAmount * 0.010f;
    const auto level = 0.22f * v.velocity;
    const auto baseFreq = bentFrequency (v.frequency) * 2.0f; // an octave up, per the Faust source

    for (int n = 0; n < numSamples; ++n)
    {
        const auto envGain = v.env.getNextSample();

        float stack = 0.0f;

        for (int i = 0; i < kNumOscs; ++i)
        {
            vs.driftPhase[(size_t) i] = wrapPhase (vs.driftPhase[(size_t) i] + kOscDriftRateHz[i] * invSr);
            const auto drift = std::sin (vs.driftPhase[(size_t) i] * juce::MathConstants<float>::twoPi) * driftDepth;

            const auto freqHz = baseFreq * (1.0f + kOscDetuneFraction[i] + drift);
            const auto inc = juce::jlimit (0.0f, 0.49f, freqHz * invSr);
            vs.phase[(size_t) i] = wrapPhase (vs.phase[(size_t) i] + inc);

            stack += triangleWave (vs.phase[(size_t) i]);
        }

        stack *= 0.3f;

        vs.sweepPhase = wrapPhase (vs.sweepPhase + 0.09f * invSr);
        const auto sweepHz = 450.0f + (std::sin (vs.sweepPhase * juce::MathConstants<float>::twoPi) * 0.5f + 0.5f) * 900.0f;
        vs.bandpass.setCutoffFrequency (juce::jlimit (40.0f, (float) (sampleRate * 0.45), sweepHz * brightnessMultiplier));

        const auto bandpassed = vs.bandpass.processSample (0, stack);
        const auto swept = vs.safetyLowpass.processSample (0, bandpassed);

        const auto sweptOut = swept * envGain * level * 0.55f;
        left[n]  += sweptOut;
        right[n] += sweptOut;

        if (n < shimmerLen)
            shimmerIn[n] += swept * envGain * level;
    }
}

void AiryChoirLayer::renderLayerTail (juce::AudioBuffer<float>& target, int numSamples)
{
    const auto n = juce::jmin (numSamples, shimmerBus.getNumSamples());
    auto* bus = shimmerBus.getWritePointer (0);
    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const auto highpassed = shimmerHighpass.processSample (0, bus[i]);
        bus[i] = shimmerPitch.processSample (highpassed);
    }

    {
        juce::dsp::AudioBlock<float> block (shimmerBus.getArrayOfWritePointers(), 1, 0, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        shimmerReverb.process (ctx);
    }

    for (int i = 0; i < n; ++i)
    {
        const auto s = bus[i] * 0.16f;
        left[i]  += s;
        right[i] += s;
    }
}

} // namespace horizon

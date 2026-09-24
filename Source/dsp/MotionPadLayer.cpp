#include "MotionPadLayer.h"

namespace horizon
{

void MotionPadLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    const juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) maxBlockSize, 1 };

    for (auto& vs : voiceState)
    {
        vs.filter.prepare (monoSpec);
        vs.filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        vs.filter.setResonance (0.5f);
    }

    resetLayer();
}

void MotionPadLayer::resetLayer()
{
    for (auto& vs : voiceState)
    {
        vs.phase.fill (0.0f);
        vs.driftPhase.fill (0.0f);
        vs.tremPhase = 0.0f;
        vs.filterLfoPhase = 0.0f;
        vs.filter.reset();
    }
}

void MotionPadLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];

    for (int i = 0; i < kNumOscs; ++i)
    {
        vs.phase[(size_t) i] = rng.nextFloat();
        vs.driftPhase[(size_t) i] = rng.nextFloat();
    }

    vs.tremPhase = rng.nextFloat();
    vs.filterLfoPhase = rng.nextFloat();
    vs.filter.reset();
}

void MotionPadLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    const auto invSr = 1.0f / (float) sampleRate;
    const auto driftDepth = 0.004f + modAmount * 0.010f;
    const auto level = 0.24f * v.velocity;
    const auto baseFreq = bentFrequency (v.frequency);

    for (int n = 0; n < numSamples; ++n)
    {
        const auto envGain = v.env.getNextSample();
        // Voice-steal declick ramp; 1.0 unless this slot is being taken over.
        const auto stealGain = nextStealGain (v);
        const auto brightness = effectiveBrightness (voiceIndex, n);

        float stack = 0.0f;

        for (int i = 0; i < kNumOscs; ++i)
        {
            vs.driftPhase[(size_t) i] = wrapPhase (vs.driftPhase[(size_t) i] + kOscDriftRateHz[i] * invSr);
            const auto drift = std::sin (vs.driftPhase[(size_t) i] * juce::MathConstants<float>::twoPi) * driftDepth;

            const auto freqHz = baseFreq * (1.0f + kOscDetuneFraction[i] + drift);
            const auto inc = juce::jlimit (0.0f, 0.49f, freqHz * invSr);
            vs.phase[(size_t) i] = wrapPhase (vs.phase[(size_t) i] + inc);

            stack += polyBlepSaw (vs.phase[(size_t) i], inc);
        }

        stack *= 0.4f;

        vs.filterLfoPhase = wrapPhase (vs.filterLfoPhase + 0.6f * invSr);
        const auto filtLfo = std::sin (vs.filterLfoPhase * juce::MathConstants<float>::twoPi) * 600.0f + 1400.0f;
        vs.filter.setCutoffFrequency (juce::jlimit (40.0f, (float) (sampleRate * 0.45), filtLfo * brightness));

        const auto filtered = vs.filter.processSample (0, stack);

        vs.tremPhase = wrapPhase (vs.tremPhase + 3.2f * invSr);
        const auto trem = std::sin (vs.tremPhase * juce::MathConstants<float>::twoPi) * 0.35f + 0.65f;

        const auto s = filtered * envGain * level * trem * stealGain;

        left[n]  += s;
        right[n] += s;
    }
}

} // namespace horizon

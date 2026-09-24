#include "AnalogEnsembleLayer.h"

namespace horizon
{

void AnalogEnsembleLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    const juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) maxBlockSize, 1 };

    for (auto& vs : voiceState)
    {
        vs.delayBuffer.assign ((size_t) kFlangerMaxDelaySamples + 4, 0.0f);
        vs.filter.prepare (monoSpec);
        vs.filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        vs.filter.setResonance (0.5f);
    }

    resetLayer();
}

void AnalogEnsembleLayer::resetLayer()
{
    for (auto& vs : voiceState)
    {
        vs.phase.fill (0.0f);
        vs.driftPhase.fill (0.0f);
        vs.sweepPhase = 0.0f;
        vs.writePos = 0;
        std::fill (vs.delayBuffer.begin(), vs.delayBuffer.end(), 0.0f);
        vs.filter.reset();
    }
}

void AnalogEnsembleLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];

    for (int i = 0; i < kNumOscs; ++i)
    {
        vs.phase[(size_t) i] = rng.nextFloat();
        vs.driftPhase[(size_t) i] = rng.nextFloat();
    }

    vs.sweepPhase = rng.nextFloat();
    vs.writePos = 0;
    std::fill (vs.delayBuffer.begin(), vs.delayBuffer.end(), 0.0f);
    vs.filter.reset();
}

void AnalogEnsembleLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    const auto invSr = 1.0f / (float) sampleRate;
    const auto driftDepth = 0.004f + modAmount * 0.010f;
    const auto level = 0.16f * v.velocity;
    const auto baseFreq = bentFrequency (v.frequency);
    const auto delaySize = (int) vs.delayBuffer.size();

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

        stack *= 0.3f;

        // --- LFO-swept flanger: write the dry stack into the delay ring, read
        // back at a continuously-moving delay time (2..11 ms at 0.11 Hz) and
        // sum with the dry signal. It is the SWEEP that makes this a chorus
        // rather than a fixed comb filter.
        vs.delayBuffer[(size_t) vs.writePos] = stack;

        vs.sweepPhase = wrapPhase (vs.sweepPhase + 0.11f * invSr);
        const auto sweepMs = (std::sin (vs.sweepPhase * juce::MathConstants<float>::twoPi) * 0.5f + 0.5f) * 9.0f + 2.0f;
        const auto delaySamples = juce::jlimit (0.0f, (float) (kFlangerMaxDelaySamples - 2),
                                                sweepMs * 0.001f * (float) sampleRate);

        const auto readPosF = (float) vs.writePos - delaySamples;
        auto readPos0 = (int) std::floor (readPosF);
        const auto frac = readPosF - (float) readPos0;

        readPos0 = ((readPos0 % delaySize) + delaySize) % delaySize;
        const auto readPos1 = (readPos0 + 1) % delaySize;

        const auto delayed = vs.delayBuffer[(size_t) readPos0] * (1.0f - frac)
                            + vs.delayBuffer[(size_t) readPos1] * frac;

        vs.writePos = (vs.writePos + 1) % delaySize;

        const auto ensemble = stack + delayed * 0.55f;

        const auto cutoff = juce::jlimit (40.0f, (float) (sampleRate * 0.45),
                                          (700.0f + envGain * 2200.0f) * brightness);
        vs.filter.setCutoffFrequency (cutoff);

        const auto filtered = vs.filter.processSample (0, ensemble);
        const auto s = filtered * envGain * level * stealGain;

        left[n]  += s;
        right[n] += s;
    }
}

} // namespace horizon

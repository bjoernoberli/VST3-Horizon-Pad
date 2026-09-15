#include "WarmFoundationLayer.h"

namespace horizon
{

void WarmFoundationLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    const juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) maxBlockSize, 1 };

    for (auto& vs : voiceState)
    {
        vs.filter.prepare (monoSpec);
        vs.filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        vs.filter.setResonance (0.55f);
    }

    resetLayer();
}

void WarmFoundationLayer::resetLayer()
{
    for (auto& vs : voiceState)
    {
        vs.phase.fill (0.0f);
        vs.driftPhase.fill (0.0f);
        vs.breathePhase = 0.0f;
        vs.filter.reset();
    }
}

void WarmFoundationLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];

    for (int i = 0; i < kNumOscs; ++i)
    {
        vs.phase[(size_t) i] = rng.nextFloat();
        vs.driftPhase[(size_t) i] = rng.nextFloat();
    }

    vs.breathePhase = rng.nextFloat();
    vs.filter.reset();
}

void WarmFoundationLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    const auto invSr = 1.0f / (float) sampleRate;
    const auto driftDepth = 0.004f + modAmount * 0.010f; // cycles of pitch ratio, matches drift() in the Faust source
    const auto level = 0.20f * v.velocity;
    const auto baseFreq = bentFrequency (v.frequency);

    for (int n = 0; n < numSamples; ++n)
    {
        const auto envGain = v.env.getNextSample();

        auto advance = [&] (int idx, float freqHz) noexcept
        {
            vs.driftPhase[(size_t) idx] = wrapPhase (vs.driftPhase[(size_t) idx] + kOscDriftRateHz[idx] * invSr);
            const auto drift = std::sin (vs.driftPhase[(size_t) idx] * juce::MathConstants<float>::twoPi) * driftDepth;

            const auto inc = juce::jlimit (0.0f, 0.49f, freqHz * (1.0f + drift) * invSr);
            vs.phase[(size_t) idx] = wrapPhase (vs.phase[(size_t) idx] + inc);
            return inc;
        };

        // Three detuned triangle voices + saw edge, at the base pitch.
        float stack = 0.0f;

        for (int i = 0; i < 3; ++i)
        {
            const auto freqHz = baseFreq * std::pow (2.0f, kOscDetuneCents[i] / 1200.0f);
            advance (i, freqHz);
            stack += triangleWave (vs.phase[(size_t) i]);
        }

        stack *= 0.3f;

        const auto sawInc = advance (3, baseFreq);
        const auto sawEdge = polyBlepSaw (vs.phase[3], sawInc) * 0.15f;

        // Sub-oscillator, one octave down.
        advance (4, baseFreq * 0.5f);
        const auto sub = triangleWave (vs.phase[4]) * 0.25f;

        const auto body = stack + sawEdge + sub;

        // Slow filter "breathe" so the timbre keeps moving once the envelope settles.
        vs.breathePhase = wrapPhase (vs.breathePhase + 0.13f * invSr);
        const auto breathe = std::sin (vs.breathePhase * juce::MathConstants<float>::twoPi) * 0.22f + 0.78f;

        const auto cutoff = juce::jlimit (40.0f, (float) (sampleRate * 0.45),
                                          (350.0f + envGain * 900.0f) * breathe * brightnessMultiplier);
        vs.filter.setCutoffFrequency (cutoff);

        const auto filtered = vs.filter.processSample (0, body);
        const auto s = filtered * envGain * level;

        left[n]  += s;
        right[n] += s;
    }
}

} // namespace horizon

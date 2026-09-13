#include "WarmPadLayer.h"

namespace horizon
{

void WarmPadLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    resetLayer();
}

void WarmPadLayer::resetLayer()
{
    for (auto& vs : voiceState)
        for (auto& o : vs.oscs)
        {
            o.phase = 0.0f;
            o.driftPhase = 0.0f;
        }
}

void WarmPadLayer::applyTone (const LayerTone& newTone)
{
    LayerBase::applyTone (newTone);

    // 0 .. 22 cents of static spread, plus up to 9 cents of slow drift on top.
    detuneCents = 22.0f * juce::jlimit (0.0f, 1.0f, newTone.detune);
    driftCents  = 2.0f + 7.0f * juce::jlimit (0.0f, 1.0f, newTone.detune);
}

void WarmPadLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];

    for (int i = 0; i < kOscsPerVoice; ++i)
    {
        auto& o = vs.oscs[(size_t) i];

        // Randomised start phase keeps repeated notes from sounding identical.
        o.phase = rng.nextFloat();
        o.driftPhase = rng.nextFloat();

        // Slightly different drift rate per oscillator -> never-repeating beating.
        o.driftRate = 0.035f + 0.055f * (float) i + 0.02f * rng.nextFloat();

        // -1, 0, +1 detune units, hard-wired stereo spread.
        o.centsOffset = (float) (i - 1);
        o.pan = 0.7f * (float) (i - 1);
    }
}

void WarmPadLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    const auto blend = juce::jlimit (0.0f, 1.0f, tone.waveBlend);
    const auto invSr = 1.0f / (float) sampleRate;
    const auto level = 0.16f * v.velocity;

    for (int n = 0; n < numSamples; ++n)
    {
        const auto envGain = v.env.getNextSample();

        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (auto& o : vs.oscs)
        {
            o.driftPhase = wrapPhase (o.driftPhase + o.driftRate * invSr);
            const auto drift = std::sin (o.driftPhase * juce::MathConstants<float>::twoPi);

            const auto cents = o.centsOffset * detuneCents + drift * driftCents;
            const auto freq = v.frequency * std::pow (2.0f, cents / 1200.0f);
            const auto inc = juce::jlimit (0.0f, 0.49f, freq * invSr);

            o.phase = wrapPhase (o.phase + inc);

            const auto sine = std::sin (o.phase * juce::MathConstants<float>::twoPi);
            const auto saw  = polyBlepSaw (o.phase, inc);
            const auto osc  = (1.0f - blend) * sine + blend * 0.7f * saw;

            const auto gainL = std::sqrt (0.5f * (1.0f - o.pan));
            const auto gainR = std::sqrt (0.5f * (1.0f + o.pan));

            sampleL += osc * gainL;
            sampleR += osc * gainR;
        }

        const auto g = envGain * level;
        left[n]  += sampleL * g;
        right[n] += sampleR * g;
    }
}

} // namespace horizon

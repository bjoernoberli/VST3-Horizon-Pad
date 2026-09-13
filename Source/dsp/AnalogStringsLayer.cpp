#include "AnalogStringsLayer.h"

namespace horizon
{

void AnalogStringsLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    // Symmetric spread: -3 -2 -1 0 +1 +2 +3 detune units, panned to match.
    for (int i = 0; i < kUnison; ++i)
    {
        const auto unit = (float) i - (float) (kUnison - 1) * 0.5f;    // -3 .. +3
        spreadUnits[(size_t) i] = unit / ((float) (kUnison - 1) * 0.5f); // -1 .. +1
        panPositions[(size_t) i] = 0.85f * spreadUnits[(size_t) i];
    }

    resetLayer();
}

void AnalogStringsLayer::resetLayer()
{
    for (auto& vs : voiceState)
    {
        vs.phase.fill (0.0f);
        vs.vibratoPhase = 0.0f;
    }
}

void AnalogStringsLayer::applyTone (const LayerTone& newTone)
{
    LayerBase::applyTone (newTone);

    // A string machine lives on a fairly narrow spread; 4..34 cents covers
    // "tight ensemble" through to "wide, seasick chorus".
    detuneCents = 4.0f + 30.0f * juce::jlimit (0.0f, 1.0f, newTone.detune);
}

void AnalogStringsLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];

    for (auto& p : vs.phase)
        p = rng.nextFloat();

    vs.vibratoPhase = rng.nextFloat();
    vs.vibratoRate = 4.0f + 1.6f * rng.nextFloat();
}

void AnalogStringsLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    const auto blend = juce::jlimit (0.0f, 1.0f, tone.waveBlend);
    const auto invSr = 1.0f / (float) sampleRate;
    const auto level = 0.085f * v.velocity;

    for (int n = 0; n < numSamples; ++n)
    {
        const auto envGain = v.env.getNextSample();

        vs.vibratoPhase = wrapPhase (vs.vibratoPhase + vs.vibratoRate * invSr);
        const auto vibrato = std::sin (vs.vibratoPhase * juce::MathConstants<float>::twoPi) * 3.0f; // cents

        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (int i = 0; i < kUnison; ++i)
        {
            const auto cents = spreadUnits[(size_t) i] * detuneCents + vibrato;
            const auto freq = v.frequency * std::pow (2.0f, cents / 1200.0f);
            const auto inc = juce::jlimit (0.0f, 0.49f, freq * invSr);

            auto& phase = vs.phase[(size_t) i];
            phase = wrapPhase (phase + inc);

            const auto saw = polyBlepSaw (phase, inc);

            // Triangle: cheap, naturally band-limited enough for a soft blend partner.
            const auto tri = 4.0f * std::abs (phase - 0.5f) - 1.0f;

            const auto osc = blend * saw + (1.0f - blend) * tri;

            const auto pan = panPositions[(size_t) i];
            sampleL += osc * std::sqrt (0.5f * (1.0f - pan));
            sampleR += osc * std::sqrt (0.5f * (1.0f + pan));
        }

        const auto g = envGain * level;
        left[n]  += sampleL * g;
        right[n] += sampleR * g;
    }
}

} // namespace horizon

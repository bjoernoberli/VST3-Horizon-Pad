#include "SubPadLayer.h"

namespace horizon
{

void SubPadLayer::prepareLayer (const juce::dsp::ProcessSpec&)
{
    resetLayer();
}

void SubPadLayer::resetLayer()
{
    for (auto& vs : voiceState)
    {
        vs.phase = 0.0f;
        vs.driftPhase = 0.0f;
    }
}

void SubPadLayer::applyTone (const LayerTone& newTone)
{
    LayerBase::applyTone (newTone);

    const auto d = juce::jlimit (0.0f, 1.0f, newTone.detune);
    octaveShift = -1.0f - d;           // one octave down .. two octaves down
    driftCents = 1.0f + 4.0f * d;
}

void SubPadLayer::startVoice (int voiceIndex, float, float)
{
    auto& vs = voiceState[(size_t) voiceIndex];
    vs.phase = 0.0f;                   // start at zero-crossing: no sub-bass click
    vs.driftPhase = rng.nextFloat();
    vs.driftRate = 0.03f + 0.05f * rng.nextFloat();
}

void SubPadLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];
    auto& vs = voiceState[(size_t) voiceIndex];

    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);

    const auto blend = juce::jlimit (0.0f, 1.0f, tone.waveBlend);
    const auto invSr = 1.0f / (float) sampleRate;
    const auto level = 0.30f * v.velocity;

    // Below 20 Hz there is nothing useful left, so clamp the shifted pitch.
    const auto baseFreq = juce::jmax (18.0f, v.frequency * std::pow (2.0f, octaveShift));

    for (int n = 0; n < numSamples; ++n)
    {
        const auto envGain = v.env.getNextSample();

        vs.driftPhase = wrapPhase (vs.driftPhase + vs.driftRate * invSr);
        const auto drift = std::sin (vs.driftPhase * juce::MathConstants<float>::twoPi) * driftCents;

        const auto freq = baseFreq * std::pow (2.0f, drift / 1200.0f);
        const auto inc = juce::jlimit (0.0f, 0.49f, freq * invSr);

        vs.phase = wrapPhase (vs.phase + inc);

        const auto sine = std::sin (vs.phase * juce::MathConstants<float>::twoPi);
        const auto tri  = 4.0f * std::abs (vs.phase - 0.5f) - 1.0f;
        const auto osc  = (1.0f - blend) * sine + blend * tri;

        const auto s = osc * envGain * level;
        left[n]  += s;
        right[n] += s;
    }
}

} // namespace horizon

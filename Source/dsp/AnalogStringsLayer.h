#pragma once

#include "LayerBase.h"

namespace horizon
{

/**
    Layer 2 - ANALOG STRINGS.

    The classic string-machine recipe: a unison stack of seven sawtooth
    oscillators per voice, detuned symmetrically around the note and panned
    across the stereo field, run into the layer lowpass (LayerBase::toneFilter)
    which supplies the "analog" roll-off.

    `waveBlend` crossfades the stack between saw (full string ensemble) and a
    softer triangle-ish shape for a more muted, felt-covered character.
    `detune` controls the unison spread, which is what makes the ensemble
    effect wide or tight.
*/
class AnalogStringsLayer final : public LayerBase
{
public:
    AnalogStringsLayer() = default;

protected:
    void prepareLayer (const juce::dsp::ProcessSpec& spec) override;
    void resetLayer() override;
    void startVoice (int voiceIndex, float frequency, float velocity) override;
    void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) override;
    void applyTone (const LayerTone& newTone) override;

private:
    static constexpr int kUnison = 7;

    struct VoiceState
    {
        std::array<float, (size_t) kUnison> phase {};
        float vibratoPhase = 0.0f;
        float vibratoRate = 4.6f;
    };

    std::array<VoiceState, (size_t) kMaxVoices> voiceState;

    std::array<float, (size_t) kUnison> spreadUnits {};
    std::array<float, (size_t) kUnison> panPositions {};

    float detuneCents = 12.0f;
};

} // namespace horizon

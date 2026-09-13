#pragma once

#include "LayerBase.h"

namespace horizon
{

/**
    Layer 4 - SUB PAD.

    Deliberately the simplest layer: one oscillator per voice, an octave or two
    below the played note, blending sine (pure, felt more than heard) into
    triangle (a little more edge so it survives on small speakers).

    `waveBlend` is that sine/triangle blend. `detune` chooses how far down the
    octave shift goes (0 = one octave, 1 = two octaves) plus a tiny amount of
    slow drift so two held sub notes do not phase-lock into a static tone.
    Voices are summed to mono-centre, because stereo content down here just
    makes a mix muddy.
*/
class SubPadLayer final : public LayerBase
{
public:
    SubPadLayer() = default;

protected:
    void prepareLayer (const juce::dsp::ProcessSpec& spec) override;
    void resetLayer() override;
    void startVoice (int voiceIndex, float frequency, float velocity) override;
    void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) override;
    void applyTone (const LayerTone& newTone) override;

private:
    struct VoiceState
    {
        float phase = 0.0f;
        float driftPhase = 0.0f;
        float driftRate = 0.05f;
    };

    std::array<VoiceState, (size_t) kMaxVoices> voiceState;

    float octaveShift = -1.0f;   // in octaves
    float driftCents = 2.0f;
};

} // namespace horizon

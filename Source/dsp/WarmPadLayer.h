#pragma once

#include "LayerBase.h"

namespace horizon
{

/**
    Layer 1 - WARM PAD.

    A small oscillator bank (three oscillators per voice) where each oscillator
    crossfades between a sine and a band-limited saw. `waveBlend` moves the whole
    bank from pure sine (soft, glassy) to saw (rich, reedy). `detune` spreads the
    three oscillators apart in cents, and each oscillator's pitch is additionally
    modulated by its own very slow random-ish LFO, which gives the gentle
    chorus-like drift that makes the pad breathe instead of sitting still.
*/
class WarmPadLayer final : public LayerBase
{
public:
    WarmPadLayer() = default;

protected:
    void prepareLayer (const juce::dsp::ProcessSpec& spec) override;
    void resetLayer() override;
    void startVoice (int voiceIndex, float frequency, float velocity) override;
    void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) override;
    void applyTone (const LayerTone& newTone) override;

private:
    static constexpr int kOscsPerVoice = 3;

    struct OscState
    {
        float phase = 0.0f;
        float driftPhase = 0.0f;
        float driftRate = 0.07f;   // Hz
        float centsOffset = 0.0f;
        float pan = 0.0f;          // -1 .. +1
    };

    struct VoiceState
    {
        std::array<OscState, (size_t) kOscsPerVoice> oscs;
    };

    std::array<VoiceState, (size_t) kMaxVoices> voiceState;

    float detuneCents = 8.0f;
    float driftCents = 4.0f;
};

} // namespace horizon

#pragma once

#include "LayerBase.h"

namespace horizon
{

/**
    PAD 1 / ROOT - Warm Foundation: dull, slow attack, low-mid register.

    Direct C++ port of the "warmFoundation" instrument in four_pads.dsp: a
    three-oscillator detuned triangle stack plus a touch of sawtooth edge, a
    sub-oscillator an octave down, all through a 2-pole lowpass whose cutoff
    opens from dark to bright following the amplitude envelope, further
    shaped by a slow "breathe" LFO so the timbre never sits frozen even once
    the envelope has settled. Every oscillator drifts slightly in pitch (a
    plain sine LFO, not filtered noise - see four_pads.dsp for why) to read
    as analog rather than digital-static.
*/
class WarmFoundationLayer final : public LayerBase
{
public:
    WarmFoundationLayer() = default;

protected:
    void prepareLayer (const juce::dsp::ProcessSpec& spec) override;
    void resetLayer() override;
    void startVoice (int voiceIndex, float frequency, float velocity) override;
    void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) override;

    float attackSeconds() const noexcept override  { return 2.2f; }
    float decaySeconds() const noexcept override   { return 0.8f; }
    float sustainLevel() const noexcept override   { return 0.75f; }
    float releaseSeconds() const noexcept override { return 2.4f; }

private:
    // Oscillator layout: 0..2 = detuned triangle stack, 3 = saw edge, 4 = sub.
    static constexpr int kNumOscs = 5;
    static constexpr float kOscDetuneCents[3] { 0.0f, 7.0f, -6.0f };
    static constexpr float kOscDriftRateHz[5] { 0.11f, 0.14f, 0.09f, 0.15f, 0.08f };

    struct VoiceState
    {
        std::array<float, kNumOscs> phase {};
        std::array<float, kNumOscs> driftPhase {};
        float breathePhase = 0.0f;
        juce::dsp::StateVariableTPTFilter<float> filter;
    };

    std::array<VoiceState, (size_t) kMaxVoices> voiceState;
};

} // namespace horizon

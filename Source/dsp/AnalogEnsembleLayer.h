#pragma once

#include "LayerBase.h"

namespace horizon
{

/**
    PAD 2 / CLEARING - Analog Ensemble: bright, medium attack, chorused mid
    register.

    Direct C++ port of "analogEnsemble" in four_pads.dsp: a three-oscillator
    detuned sawtooth stack through a genuinely LFO-swept chorus/flanger (the
    delay time itself moves - a *fixed* delay is just a static comb filter and
    reads as reedy/brassy, a mistake caught and fixed during the Faust sound
    design). The lowpass cutoff starts closed and opens with the envelope, like
    every other pad, so this layer doesn't sound "ahead" of the others at
    note-on regardless of the ATTACK macro setting.
*/
class AnalogEnsembleLayer final : public LayerBase
{
public:
    AnalogEnsembleLayer() = default;

protected:
    void prepareLayer (const juce::dsp::ProcessSpec& spec) override;
    void resetLayer() override;
    void startVoice (int voiceIndex, float frequency, float velocity) override;
    void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) override;

    float attackSeconds() const noexcept override  { return 2.6f; }
    float decaySeconds() const noexcept override   { return 0.7f; }
    float sustainLevel() const noexcept override   { return 0.8f; }
    float releaseSeconds() const noexcept override { return 2.2f; }

private:
    static constexpr int kNumOscs = 3;
    static constexpr float kOscDetuneFraction[3] { 0.0003f, -0.0004f, 0.0009f }; // det * 0.01 from the Faust source
    static constexpr float kOscDriftRateHz[3] { 0.19f, 0.23f, 0.27f };

    // Flanger: a modulated delay line, max 2048 samples, swept 2..11 ms at 0.11 Hz.
    static constexpr int kFlangerMaxDelaySamples = 2048;

    struct VoiceState
    {
        std::array<float, kNumOscs> phase {};
        std::array<float, kNumOscs> driftPhase {};
        float sweepPhase = 0.0f;

        std::vector<float> delayBuffer;
        int writePos = 0;

        juce::dsp::StateVariableTPTFilter<float> filter;
    };

    std::array<VoiceState, (size_t) kMaxVoices> voiceState;
};

} // namespace horizon

#pragma once

#include "LayerBase.h"

namespace horizon
{

/**
    PAD 4 / BLOOM - Motion Pad: medium brightness, medium attack, built-in
    movement.

    Direct C++ port of "motionPad" in four_pads.dsp: a two-oscillator detuned
    sawtooth stack through a slow-sweeping lowpass, with audible tremolo on
    top. The odd one out on purpose - where the other three pads sustain and
    swell, this one keeps moving throughout the note, so it reads as
    "motion/interest" rather than another sustain layer.
*/
class MotionPadLayer final : public LayerBase
{
public:
    MotionPadLayer() = default;

protected:
    void prepareLayer (const juce::dsp::ProcessSpec& spec) override;
    void resetLayer() override;
    void startVoice (int voiceIndex, float frequency, float velocity) override;
    void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) override;

    float attackSeconds() const noexcept override  { return 1.2f; }
    float decaySeconds() const noexcept override   { return 0.5f; }
    float sustainLevel() const noexcept override   { return 0.7f; }
    float releaseSeconds() const noexcept override { return 1.6f; }

private:
    static constexpr int kNumOscs = 2;
    static constexpr float kOscDetuneFraction[2] { 0.0004f, -0.0005f }; // det * 0.01 from the Faust source
    static constexpr float kOscDriftRateHz[2] { 0.33f, 0.37f };

    struct VoiceState
    {
        std::array<float, kNumOscs> phase {};
        std::array<float, kNumOscs> driftPhase {};
        float tremPhase = 0.0f;
        float filterLfoPhase = 0.0f;

        juce::dsp::StateVariableTPTFilter<float> filter;
    };

    std::array<VoiceState, (size_t) kMaxVoices> voiceState;
};

} // namespace horizon

#pragma once

#include "LayerBase.h"
#include "OctaveShimmer.h"

namespace horizon
{

/**
    PAD 3 / EXPANSE - Airy Choir: very bright, very slow attack, high/air
    register with a long tail.

    Direct C++ port of "airyChoir" in four_pads.dsp: a three-oscillator detuned
    triangle stack an octave up, through a resonant bandpass that sweeps slowly
    into the top end, plus an octave-up "shimmer" send (highpass -> pitch shift
    -> reverb) so this pad occupies clear air above the other three. All
    oscillator-based, no noise source, per the sound design's own history (an
    earlier noise-based texture layer was replaced for reading as unwanted hiss).

    The shimmer's pitch shift and reverb are shared per LAYER rather than
    computed per voice (each voice still gets its own bandpass sweep and its
    own envelope): the Faust source's envelope is applied to the *combined*
    swept+shimmer signal, not before, so per-voice contributions can be summed
    into one shimmer bus and processed once - both truer to the original
    signal-flow order and far cheaper than a pitch-shifter and reverb per voice.
*/
class AiryChoirLayer final : public LayerBase
{
public:
    AiryChoirLayer() = default;

protected:
    void prepareLayer (const juce::dsp::ProcessSpec& spec) override;
    void resetLayer() override;
    void startVoice (int voiceIndex, float frequency, float velocity) override;
    void beginBlock (int numSamples) override;
    void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) override;
    void renderLayerTail (juce::AudioBuffer<float>& target, int numSamples) override;

    float attackSeconds() const noexcept override  { return 2.6f; }
    float decaySeconds() const noexcept override   { return 1.0f; }
    float sustainLevel() const noexcept override   { return 0.4f; }
    float releaseSeconds() const noexcept override { return 1.8f; }

private:
    static constexpr int kNumOscs = 3;
    static constexpr float kOscDetuneFraction[3] { 0.0002f, -0.0003f, 0.0005f }; // det * 0.01 from the Faust source
    static constexpr float kOscDriftRateHz[3] { 0.29f, 0.31f, 0.24f };

    struct VoiceState
    {
        std::array<float, kNumOscs> phase {};
        std::array<float, kNumOscs> driftPhase {};
        float sweepPhase = 0.0f;

        juce::dsp::StateVariableTPTFilter<float> bandpass;
        juce::dsp::StateVariableTPTFilter<float> safetyLowpass;
    };

    std::array<VoiceState, (size_t) kMaxVoices> voiceState;

    // Layer-wide shimmer bus: sum of every voice's (highpassed, enveloped) raw
    // swept signal, pitch-shifted and reverbed once per block.
    juce::AudioBuffer<float> shimmerBus;
    juce::dsp::StateVariableTPTFilter<float> shimmerHighpass;
    OctaveShimmer shimmerPitch;
    juce::dsp::Reverb shimmerReverb;
};

} // namespace horizon

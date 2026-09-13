#pragma once

#include "LayerBase.h"

namespace horizon
{

/**
    Layer 3 - GRANULAR TEXTURE.

    A real (if deliberately compact) grain engine rather than "noise through a
    filter":

      * Two procedurally generated source tables are built once in prepare():
        a NOISY table (white noise smoothed by a two-pole lowpass, i.e. coloured
        noise) and a TONAL table (a harmonic partial stack tuned to a 110 Hz
        reference). `waveBlend` crossfades what the grains read from, so the
        layer can move from breathy air to pitched shimmer.

      * A scheduler counts down samples to the next grain onset. `density` maps
        that interval from ~260 ms (sparse, individual drops) to ~9 ms (a dense
        continuous cloud).

      * Each grain gets a Hann window, a random read offset into the source, a
        random pan, and a playback rate derived from one of the notes currently
        held (so the texture follows the chord), with `detune` adding per-grain
        pitch jitter in cents.

      * Grains live in a fixed-size pool allocated in prepare(); firing a grain
        never allocates. If the pool is full the onset is simply dropped.

    The per-voice amplitude envelopes are summed into a per-sample "gate" which
    scales the whole grain cloud, so the layer swells and decays with the notes.
*/
class GranularTextureLayer final : public LayerBase
{
public:
    GranularTextureLayer() = default;

protected:
    void prepareLayer (const juce::dsp::ProcessSpec& spec) override;
    void resetLayer() override;
    void beginBlock (int numSamples) override;
    void renderVoice (int voiceIndex, juce::AudioBuffer<float>& target, int numSamples) override;
    void renderLayerTail (juce::AudioBuffer<float>& target, int numSamples) override;
    void applyTone (const LayerTone& newTone) override;

private:
    static constexpr int kMaxGrains = 48;
    static constexpr float kSourceSeconds = 2.0f;
    static constexpr float kSourceReferenceHz = 110.0f;

    struct Grain
    {
        bool active = false;
        double readPos = 0.0;     // fractional index into the source tables
        double readInc = 1.0;     // playback rate
        int age = 0;              // samples since onset
        int length = 1;           // total grain length in samples
        float gainL = 0.7f;
        float gainR = 0.7f;
        float amplitude = 1.0f;
    };

    void triggerGrain();
    float readSource (double position) const noexcept;

    void buildSourceTables();

    juce::AudioBuffer<float> sourceNoisy;  // mono
    juce::AudioBuffer<float> sourceTonal;  // mono
    int sourceLength = 0;

    std::array<Grain, (size_t) kMaxGrains> grains;
    juce::AudioBuffer<float> gate;         // mono per-sample envelope sum

    std::array<float, (size_t) kMaxVoices> liveFrequencies {};
    int numLiveFrequencies = 0;

    int samplesToNextGrain = 0;
    float grainIntervalSamples = 2000.0f;
    float grainLengthSamples = 8000.0f;
    float detuneCents = 30.0f;
    float blend = 0.5f;
};

} // namespace horizon

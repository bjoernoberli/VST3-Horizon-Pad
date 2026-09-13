#include "GranularTextureLayer.h"

namespace horizon
{

void GranularTextureLayer::prepareLayer (const juce::dsp::ProcessSpec& spec)
{
    sourceLength = juce::jmax (1024, (int) (kSourceSeconds * (float) spec.sampleRate));

    sourceNoisy.setSize (1, sourceLength, false, true, false);
    sourceTonal.setSize (1, sourceLength, false, true, false);
    gate.setSize (1, juce::jmax (1, (int) spec.maximumBlockSize), false, true, false);
    gate.clear();

    buildSourceTables();
    resetLayer();
}

void GranularTextureLayer::buildSourceTables()
{
    // --- Noisy table: white noise through a two-pole lowpass = soft coloured air.
    {
        auto* dst = sourceNoisy.getWritePointer (0);
        float z1 = 0.0f, z2 = 0.0f;
        const float a = 0.08f;

        for (int i = 0; i < sourceLength; ++i)
        {
            const auto white = rng.nextFloat() * 2.0f - 1.0f;
            z1 += a * (white - z1);
            z2 += a * (z1 - z2);
            dst[i] = z2;
        }

        const auto peak = juce::jmax (1.0e-6f, sourceNoisy.getMagnitude (0, sourceLength));
        sourceNoisy.applyGain (1.0f / peak);
    }

    // --- Tonal table: a partial stack at the 110 Hz reference, with slightly
    //     inharmonic upper partials so granulating it shimmers rather than buzzes.
    {
        auto* dst = sourceTonal.getWritePointer (0);
        juce::FloatVectorOperations::clear (dst, sourceLength);

        struct Partial { float ratio; float gain; };
        const Partial partials[] {
            { 1.000f, 1.00f }, { 2.001f, 0.50f }, { 3.004f, 0.30f },
            { 4.010f, 0.18f }, { 5.980f, 0.12f }, { 8.030f, 0.08f }
        };

        for (const auto& p : partials)
        {
            const auto inc = juce::MathConstants<float>::twoPi * kSourceReferenceHz * p.ratio / (float) sampleRate;
            auto phase = rng.nextFloat() * juce::MathConstants<float>::twoPi;

            for (int i = 0; i < sourceLength; ++i)
            {
                dst[i] += std::sin (phase) * p.gain;
                phase += inc;
            }
        }

        const auto peak = juce::jmax (1.0e-6f, sourceTonal.getMagnitude (0, sourceLength));
        sourceTonal.applyGain (1.0f / peak);
    }
}

void GranularTextureLayer::resetLayer()
{
    for (auto& g : grains)
        g.active = false;

    samplesToNextGrain = 0;
    numLiveFrequencies = 0;
    gate.clear();
}

void GranularTextureLayer::applyTone (const LayerTone& newTone)
{
    LayerBase::applyTone (newTone);

    blend = juce::jlimit (0.0f, 1.0f, newTone.waveBlend);
    detuneCents = 120.0f * juce::jlimit (0.0f, 1.0f, newTone.detune);

    const auto density = juce::jlimit (0.0f, 1.0f, newTone.density);

    // Sparse (260 ms between onsets) .. dense cloud (9 ms), exponential so the
    // knob feels even across its range.
    const auto intervalSeconds = 0.26f * std::pow (0.035f, density);
    grainIntervalSamples = juce::jmax (8.0f, intervalSeconds * (float) sampleRate);

    // Denser clouds use shorter grains, otherwise the overlap count explodes.
    const auto lengthSeconds = 0.26f - 0.17f * density;
    grainLengthSamples = juce::jmax (64.0f, lengthSeconds * (float) sampleRate);
}

void GranularTextureLayer::beginBlock (int numSamples)
{
    gate.clear (0, juce::jmin (numSamples, gate.getNumSamples()));
    numLiveFrequencies = 0;
}

void GranularTextureLayer::renderVoice (int voiceIndex, juce::AudioBuffer<float>& /*target*/, int numSamples)
{
    auto& v = voices[(size_t) voiceIndex];

    if (numLiveFrequencies < kMaxVoices)
        liveFrequencies[(size_t) numLiveFrequencies++] = v.frequency;

    auto* g = gate.getWritePointer (0);
    const auto n = juce::jmin (numSamples, gate.getNumSamples());

    for (int i = 0; i < n; ++i)
        g[i] += v.env.getNextSample() * v.velocity;
}

float GranularTextureLayer::readSource (double position) const noexcept
{
    const auto i0 = (int) position;
    const auto frac = (float) (position - (double) i0);
    const auto i1 = (i0 + 1) % sourceLength;

    const auto* noisy = sourceNoisy.getReadPointer (0);
    const auto* tonal = sourceTonal.getReadPointer (0);

    const auto n = noisy[i0] + frac * (noisy[i1] - noisy[i0]);
    const auto t = tonal[i0] + frac * (tonal[i1] - tonal[i0]);

    return (1.0f - blend) * n + blend * t;
}

void GranularTextureLayer::triggerGrain()
{
    if (numLiveFrequencies <= 0)
        return;

    for (auto& g : grains)
    {
        if (g.active)
            continue;

        const auto baseFreq = liveFrequencies[(size_t) rng.nextInt (numLiveFrequencies)];
        const auto cents = (rng.nextFloat() * 2.0f - 1.0f) * detuneCents;

        // Occasionally drop a grain an octave down for body / an octave up for sparkle.
        const auto octave = rng.nextFloat() < 0.18f ? (rng.nextBool() ? 1200.0f : -1200.0f) : 0.0f;

        const auto ratio = (baseFreq / kSourceReferenceHz) * std::pow (2.0f, (cents + octave) / 1200.0f);

        g.active = true;
        g.readPos = (double) rng.nextInt (sourceLength);
        g.readInc = (double) juce::jlimit (0.05f, 16.0f, ratio);
        g.age = 0;
        g.length = juce::jmax (32, (int) (grainLengthSamples * (0.7f + 0.6f * rng.nextFloat())));
        g.amplitude = 0.55f + 0.45f * rng.nextFloat();

        const auto pan = rng.nextFloat() * 1.8f - 0.9f;
        g.gainL = std::sqrt (0.5f * (1.0f - pan));
        g.gainR = std::sqrt (0.5f * (1.0f + pan));

        return;
    }

    // Pool exhausted: drop this onset rather than allocate on the audio thread.
}

void GranularTextureLayer::renderLayerTail (juce::AudioBuffer<float>& target, int numSamples)
{
    auto* left  = target.getWritePointer (0);
    auto* right = target.getWritePointer (1);
    const auto* gatePtr = gate.getReadPointer (0);
    const auto gateLen = gate.getNumSamples();

    // Normalise the grain cloud against its expected overlap count so density
    // changes the texture, not the loudness.
    const auto overlap = juce::jmax (1.0f, grainLengthSamples / grainIntervalSamples);
    const auto level = 0.42f / std::sqrt (overlap);

    for (int n = 0; n < numSamples; ++n)
    {
        if (--samplesToNextGrain <= 0)
        {
            triggerGrain();
            samplesToNextGrain = juce::jmax (1, (int) (grainIntervalSamples * (0.6f + 0.8f * rng.nextFloat())));
        }

        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (auto& g : grains)
        {
            if (! g.active)
                continue;

            // Hann window over the grain's lifetime.
            const auto t = (float) g.age / (float) g.length;
            const auto window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * t);

            const auto s = readSource (g.readPos) * window * g.amplitude;
            sampleL += s * g.gainL;
            sampleR += s * g.gainR;

            g.readPos += g.readInc;
            while (g.readPos >= (double) sourceLength)
                g.readPos -= (double) sourceLength;

            if (++g.age >= g.length)
                g.active = false;
        }

        const auto gateValue = juce::jlimit (0.0f, 2.0f, n < gateLen ? gatePtr[n] : 0.0f);
        const auto g = gateValue * level;

        left[n]  += sampleL * g;
        right[n] += sampleR * g;
    }
}

} // namespace horizon

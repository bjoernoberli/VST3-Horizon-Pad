#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cmath>

namespace horizon
{

/** Number of sound layers. Fixed by design (mockup shows exactly four). */
static constexpr int kNumLayers = 4;

/** Simple polyphony cap. Ambient pads do not need a huge voice count. */
static constexpr int kMaxVoices = 8;

/** Index of each layer, used everywhere (DSP, GUI, presets). */
enum LayerIndex
{
    warmPad = 0,
    analogStrings,
    granularTexture,
    subPad
};

/**
    Plain-old-data tone-shaping values for one layer.

    These are deliberately NOT host-automatable parameters: per the product
    spec only the four layer volumes and the four global FX macros are exposed
    to the DAW. Everything below is "sound design" state that is baked into
    factory presets, editable from the plugin UI, and serialised with the
    plugin state.

    All members are normalised 0..1 and mapped to real units by the layers.
*/
struct LayerTone
{
    float waveBlend = 0.5f;   ///< 0 = pure/soft waveform, 1 = bright/harmonic-rich waveform
    float detune    = 0.3f;   ///< unison / drift spread, 0 = none, 1 = wide
    float tone      = 0.5f;   ///< per-layer lowpass brightness macro
    float attack    = 0.4f;   ///< 0 = 10 ms, 1 = 8 s (exponential)
    float release   = 0.5f;   ///< 0 = 50 ms, 1 = 12 s (exponential)
    float density   = 0.5f;   ///< granular grain density (only used by the granular layer)
};

/** The complete non-automated tone block: one LayerTone per layer. */
using ToneSet = std::array<LayerTone, (size_t) kNumLayers>;

/**
    Lock-free bridge for the tone block between the message thread (UI, preset
    changes) and the audio thread.

    Writers (UI / preset load) store individual floats and then bump a
    generation counter with release semantics. The audio thread reads the
    generation counter with acquire semantics once per block and only re-reads
    the values when it changed, so the steady-state cost is a single relaxed
    load per block. No locks, no allocation, nothing that can block the audio
    thread.
*/
class AtomicToneState
{
public:
    AtomicToneState() { store (ToneSet{}); }

    void store (const ToneSet& src) noexcept
    {
        for (size_t layer = 0; layer < (size_t) kNumLayers; ++layer)
        {
            auto& dst = values[layer];
            dst[0].store (src[layer].waveBlend, std::memory_order_relaxed);
            dst[1].store (src[layer].detune,    std::memory_order_relaxed);
            dst[2].store (src[layer].tone,      std::memory_order_relaxed);
            dst[3].store (src[layer].attack,    std::memory_order_relaxed);
            dst[4].store (src[layer].release,   std::memory_order_relaxed);
            dst[5].store (src[layer].density,   std::memory_order_relaxed);
        }

        generation.fetch_add (1, std::memory_order_release);
    }

    void storeValue (int layer, int field, float value) noexcept
    {
        jassert (juce::isPositiveAndBelow (layer, kNumLayers));
        jassert (juce::isPositiveAndBelow (field, kNumToneFields));

        values[(size_t) layer][(size_t) field].store (value, std::memory_order_relaxed);
        generation.fetch_add (1, std::memory_order_release);
    }

    ToneSet load() const noexcept
    {
        ToneSet out;

        for (size_t layer = 0; layer < (size_t) kNumLayers; ++layer)
        {
            const auto& src = values[layer];
            out[layer].waveBlend = src[0].load (std::memory_order_relaxed);
            out[layer].detune    = src[1].load (std::memory_order_relaxed);
            out[layer].tone      = src[2].load (std::memory_order_relaxed);
            out[layer].attack    = src[3].load (std::memory_order_relaxed);
            out[layer].release   = src[4].load (std::memory_order_relaxed);
            out[layer].density   = src[5].load (std::memory_order_relaxed);
        }

        return out;
    }

    float loadValue (int layer, int field) const noexcept
    {
        return values[(size_t) layer][(size_t) field].load (std::memory_order_relaxed);
    }

    /** Audio thread: cheap "has anything changed since I last looked?" check. */
    bool pollGeneration (juce::uint32& lastSeen) const noexcept
    {
        const auto current = generation.load (std::memory_order_acquire);

        if (current == lastSeen)
            return false;

        lastSeen = current;
        return true;
    }

    static constexpr int kNumToneFields = 6;

    /** Field names, used for ValueTree serialisation and by the UI knobs. */
    static const char* fieldName (int field) noexcept
    {
        static const char* names[] { "waveBlend", "detune", "tone", "attack", "release", "density" };
        return names[(size_t) juce::jlimit (0, kNumToneFields - 1, field)];
    }

    static float getField (const LayerTone& t, int field) noexcept
    {
        switch (field)
        {
            case 0:  return t.waveBlend;
            case 1:  return t.detune;
            case 2:  return t.tone;
            case 3:  return t.attack;
            case 4:  return t.release;
            default: return t.density;
        }
    }

    static void setField (LayerTone& t, int field, float v) noexcept
    {
        switch (field)
        {
            case 0:  t.waveBlend = v; break;
            case 1:  t.detune    = v; break;
            case 2:  t.tone      = v; break;
            case 3:  t.attack    = v; break;
            case 4:  t.release   = v; break;
            default: t.density   = v; break;
        }
    }

private:
    std::array<std::array<std::atomic<float>, (size_t) kNumToneFields>, (size_t) kNumLayers> values;
    std::atomic<juce::uint32> generation { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtomicToneState)
};

/** Maps the normalised attack value to seconds. */
inline float attackSeconds (float normalised) noexcept
{
    return 0.01f * std::pow (800.0f, juce::jlimit (0.0f, 1.0f, normalised)); // 10 ms .. 8 s
}

/** Maps the normalised release value to seconds. */
inline float releaseSeconds (float normalised) noexcept
{
    return 0.05f * std::pow (240.0f, juce::jlimit (0.0f, 1.0f, normalised)); // 50 ms .. 12 s
}

/** Maps a normalised "tone" (brightness) value to a lowpass cutoff in Hz. */
inline float toneCutoffHz (float normalised) noexcept
{
    return 180.0f * std::pow (100.0f, juce::jlimit (0.0f, 1.0f, normalised)); // 180 Hz .. 18 kHz
}

} // namespace horizon

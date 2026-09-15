#pragma once

#include <JuceHeader.h>
#include <atomic>

namespace horizon
{

/**
    Lock-free bridge for the two "wheel" performance controls (PITCH and MOD),
    mirroring the mockup's pitch-bend and mod wheels.

    Unlike the four pad volumes and four macros, these are not APVTS host
    parameters: they behave like a real keyboard's wheels, driven by either
    incoming MIDI (pitch bend / CC1) or the on-screen wheel being dragged, and
    they spring back into the plugin's normal behaviour rather than being
    something you'd typically automate from a host lane. Both are plain
    independent atomics (not a generation-counted block like a correlated
    struct would need) since there are only two of them and they are read
    independently by the audio thread every block.
*/
class PerformanceState
{
public:
    /** Pitch bend in semitones, range [-pitchBendRangeSemitones, +pitchBendRangeSemitones]. */
    void setPitchBendSemitones (float semitones) noexcept { pitchBend.store (semitones, std::memory_order_relaxed); }
    float getPitchBendSemitones() const noexcept { return pitchBend.load (std::memory_order_relaxed); }

    /** Mod wheel amount, 0..1. Adds extra analog-style movement on top of the baseline drift. */
    void setModAmount (float amount) noexcept { mod.store (juce::jlimit (0.0f, 1.0f, amount), std::memory_order_relaxed); }
    float getModAmount() const noexcept { return mod.load (std::memory_order_relaxed); }

    static constexpr float kPitchBendRangeSemitones = 2.0f;

private:
    std::atomic<float> pitchBend { 0.0f };
    std::atomic<float> mod { 0.0f };
};

} // namespace horizon

#pragma once

#include "../dsp/HorizonTypes.h"

#include <vector>

namespace horizon
{

/**
    A user-saved preset (the "+Save preset" library). Unlike the five factory
    Preset entries, these are created by the player and must survive across
    projects/DAW sessions/plugin instances - so, like a real commercial
    plugin, they live in one shared file on disk rather than in the host's
    per-project state.
*/
struct UserPreset
{
    juce::String name;

    /** Root, Clearing, Expanse, Bloom. 0..1. */
    std::array<float, (size_t) kNumLayers> vols {};

    /** Root, Clearing, Expanse, Bloom. 0..1. */
    std::array<float, (size_t) kNumLayers> widths { 0.5f, 0.5f, 0.5f, 0.5f };

    /** Attack, Release, Filter, Reverb. 0..1. */
    std::array<float, (size_t) kNumGlobalParams> macros {};
};

/**
    Reads/writes the shared user-preset library, mirroring the reference
    design's localStorage-based persistence with a small XML file in the
    per-user application-data directory (shared by every instance of the
    plugin, in every DAW, exactly like the design's browser storage was
    shared across every tab).
*/
class UserPresetStore
{
public:
    static juce::File getPresetFile();

    static std::vector<UserPreset> load();
    static void save (const std::vector<UserPreset>& presets);
};

} // namespace horizon

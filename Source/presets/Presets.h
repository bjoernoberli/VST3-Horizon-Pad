#pragma once

#include "../dsp/HorizonTypes.h"

#include <vector>

namespace horizon
{

/** Canonical parameter IDs for the eight host-automatable parameters. */
namespace ParamID
{
    // Pad volumes (ROOT/CLEARING/EXPANSE/BLOOM in the GUI), LayerIndex order.
    static constexpr const char* rootVolume     = "rootVolume";
    static constexpr const char* clearingVolume = "clearingVolume";
    static constexpr const char* expanseVolume  = "expanseVolume";
    static constexpr const char* bloomVolume    = "bloomVolume";

    // Global macros.
    static constexpr const char* attackMacro = "attackMacro";
    static constexpr const char* filterMacro = "filterMacro";
    static constexpr const char* widthMacro  = "widthMacro";
    static constexpr const char* reverbMacro = "reverbMacro";
}

/**
    A complete factory program: the four pad volumes plus the four macros, all
    0..1 (shown as 0..100% in the GUI). There is no per-layer tone block - each
    pad's character is fixed by its own validated Faust-derived DSP, so a
    preset is just a point in this 8-dimensional blend/macro space (matching
    the product's "eight knobs" GUI exactly, one per automatable parameter).
*/
struct Preset
{
    // juce::String, not const char*: a couple of preset names (Alpenglühen)
    // contain non-ASCII characters, and juce::String's const-char* / char[]
    // constructor assumes plain ASCII (it asserts - and, in this project's
    // build, crashes - on bytes above 127; see Presets.cpp for where that
    // bites and how it's worked around).
    juce::String name;
    juce::String description;

    /** Root, Clearing, Expanse, Bloom. 0..1. */
    std::array<float, (size_t) kNumLayers> volumes;

    /** Attack, Filter, Width, Reverb. 0..1. */
    std::array<float, (size_t) kNumGlobalParams> macros;
};

/** The five factory programs, in host program order. */
const std::vector<Preset>& getFactoryPresets();

int getNumFactoryPresets();

} // namespace horizon

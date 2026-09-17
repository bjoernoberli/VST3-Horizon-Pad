#pragma once

#include "../dsp/HorizonTypes.h"

#include <vector>

namespace horizon
{

/** Canonical parameter IDs for the twelve host-automatable parameters. */
namespace ParamID
{
    // Pad volumes (ROOT/CLEARING/EXPANSE/BLOOM in the GUI), LayerIndex order.
    static constexpr const char* rootVolume     = "rootVolume";
    static constexpr const char* clearingVolume = "clearingVolume";
    static constexpr const char* expanseVolume  = "expanseVolume";
    static constexpr const char* bloomVolume    = "bloomVolume";

    // Per-layer stereo width, LayerIndex order - each pad's own WIDTH knob
    // (a per-layer Haas-delay spread; see LayerBase), replacing what used to
    // be one shared WIDTH macro across the whole mix.
    static constexpr const char* rootWidth     = "rootWidth";
    static constexpr const char* clearingWidth = "clearingWidth";
    static constexpr const char* expanseWidth  = "expanseWidth";
    static constexpr const char* bloomWidth    = "bloomWidth";

    // Global macros.
    static constexpr const char* attackMacro  = "attackMacro";
    static constexpr const char* releaseMacro = "releaseMacro";
    static constexpr const char* filterMacro  = "filterMacro";
    static constexpr const char* reverbMacro  = "reverbMacro";
}

/**
    A complete factory program: the four pad volumes, the four per-layer
    widths, and the four macros, all 0..1 (shown as 0..100% in the GUI).
    There is no per-layer tone block beyond width - each pad's character is
    otherwise fixed by its own validated Faust-derived DSP, so a preset is
    just a point in this 12-dimensional blend/width/macro space (matching the
    product's "twelve knobs" GUI exactly, one per automatable parameter).
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

    /** Root, Clearing, Expanse, Bloom. 0..1. */
    std::array<float, (size_t) kNumLayers> widths;

    /** Attack, Release, Filter, Reverb. 0..1. */
    std::array<float, (size_t) kNumGlobalParams> macros;
};

/** The factory programs, in host program order. */
const std::vector<Preset>& getFactoryPresets();

int getNumFactoryPresets();

} // namespace horizon

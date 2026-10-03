#pragma once

#include "../dsp/HorizonTypes.h"

#include <vector>

namespace horizon
{

/** Canonical parameter IDs for the ten host-automatable parameters. */
namespace ParamID
{
    // Pad volumes (ROOT/CLEARING/EXPANSE/BLOOM in the GUI), LayerIndex order.
    static constexpr const char* rootVolume     = "rootVolume";
    static constexpr const char* clearingVolume = "clearingVolume";
    static constexpr const char* expanseVolume  = "expanseVolume";
    static constexpr const char* bloomVolume    = "bloomVolume";

    // Global macros, MacroIndex order.
    static constexpr const char* attackMacro  = "attackMacro";
    static constexpr const char* releaseMacro = "releaseMacro";
    static constexpr const char* filterMacro  = "filterMacro";
    static constexpr const char* reverbMacro  = "reverbMacro";

    // WIDTH acts on every pad through that pad's own fixed width profile
    // (LayerBase::widthProfile()), DETUNE scales every pad's designed
    // unison detune (LayerBase::setDetune()).
    static constexpr const char* widthMacro   = "widthMacro";
    static constexpr const char* detuneMacro  = "detuneMacro";
}

/**
    A complete factory program: the four pad volumes and the six macros, all
    0..1 (shown as 0..100% in the GUI). There is no per-layer tone block -
    each pad's character is fixed by its own validated Faust-derived DSP, and
    every macro acts on all four pads at once - so a preset is just a point
    in this 10-dimensional blend/macro space (matching the GUI exactly, one
    knob per automatable parameter).
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

    /** Attack, Release, Filter, Reverb, Width, Detune (MacroIndex). 0..1. */
    std::array<float, (size_t) kNumGlobalParams> macros;
};

/** The factory programs, in host program order. */
const std::vector<Preset>& getFactoryPresets();

int getNumFactoryPresets();

} // namespace horizon

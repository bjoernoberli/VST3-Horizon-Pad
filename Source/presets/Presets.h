#pragma once

#include "../dsp/ToneState.h"

#include <vector>

namespace horizon
{

/** Canonical parameter IDs for the eight host-automatable parameters. */
namespace ParamID
{
    static constexpr const char* warmPadVolume        = "warmPadVolume";
    static constexpr const char* analogStringsVolume  = "analogStringsVolume";
    static constexpr const char* granularVolume       = "granularVolume";
    static constexpr const char* subPadVolume         = "subPadVolume";
    static constexpr const char* reverb               = "reverb";
    static constexpr const char* delay                = "delay";
    static constexpr const char* filter               = "filter";
    static constexpr const char* fxAmount             = "fxAmount";
}

static constexpr int kNumGlobalParams = 4;

/** A complete factory program: eight automatable values plus the tone block. */
struct Preset
{
    const char* name;
    const char* description;

    /** Layer volumes, in the LayerIndex order. 0..1 (shown as 0..100%). */
    std::array<float, (size_t) kNumLayers> volumes;

    /** Reverb, Delay, Filter, FX Amount. 0..1. */
    std::array<float, (size_t) kNumGlobalParams> globals;

    /** Non-automated per-layer tone shaping. */
    ToneSet tone;
};

/** The six factory programs, in host program order. */
const std::vector<Preset>& getFactoryPresets();

int getNumFactoryPresets();

} // namespace horizon

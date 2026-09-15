#pragma once

#include <JuceHeader.h>

namespace horizon
{

/** Number of sound layers. Fixed by design: four independently-blendable pads. */
static constexpr int kNumLayers = 4;

/** Simple polyphony cap. Ambient pads do not need a huge voice count. */
static constexpr int kMaxVoices = 8;

/** Number of host-automatable global macro parameters (Attack/Filter/Width/Reverb). */
static constexpr int kNumGlobalParams = 4;

/**
    Index of each layer, used everywhere (DSP, GUI, presets). Order matches the
    GUI's knob row: ROOT, CLEARING, EXPANSE, BLOOM.
*/
enum LayerIndex
{
    warmFoundation = 0,   ///< ROOT - dull, slow attack, low-mid body
    analogEnsemble,       ///< CLEARING - bright, medium attack, chorused mid
    airyChoir,            ///< EXPANSE - very bright, very slow, high/air, long tail
    motionPad             ///< BLOOM - medium brightness, built-in movement
};

} // namespace horizon

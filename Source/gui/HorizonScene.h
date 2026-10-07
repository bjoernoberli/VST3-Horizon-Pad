#pragma once

#include "HorizonLookAndFeel.h"
#include "Motion.h"

namespace horizon::ui
{

/**
    The one shared panel that is the whole plugin window - sunset sky
    gradient, a low glow, a blurred mountain ridge along the bottom - drawn
    as a slow picture of the patch:

      FILTER -> time of day. 50% (the macro's neutral, the designed
                brightness) is the palette exactly as designed; darker
                settings move toward a dusk-blue sky, brighter ones toward
                golden hour.
      REVERB -> haze. More reverb softens the ridge and lifts it toward the
                colour of the air behind it (aerial perspective). The
                default (20%) is the blur as designed.
      WIDTH  -> the low glow spreads wider along the horizon. The default
                (40%) is the glow as designed.

    Each follows its parameter on a spring: about 1 s after a preset recall
    (beginRecallGlide), quicker under a hand on the knob, so a preset change
    is a change of light rather than a cut.

    The text drawn straight on the panel has to stay readable across the
    whole range. Measured with tools/gui_snapshot (2026-10-07), as designed
    (FILTER 50%) -> golden hour (100%): preset pills 5.3 -> 4.7:1, wordmark
    3.6 -> 3.35:1 (36 px: large text), footer 2.19 -> 2.12:1 (the design's
    own weakest spot, which is why golden hour keeps the lower sky and glow
    at the designed brightness). Every setting below 50% is darker, so
    higher: at 0% the footer reaches 4.2:1. Re-measure after a palette change.

    Rendered once into an image at 1x - it is all gradients and blur,
    nothing that needs device resolution - and re-rendered only while
    something moves, so an idle editor paints the panel as a blit.
    Message thread only.
*/
class HorizonScene
{
public:
    HorizonScene() = default;

    /** FILTER, REVERB and WIDTH, 0..1. */
    void setTargets (float filter, float reverb, float width) noexcept;
    void snapToTargets() noexcept;

    /** Glide to the next targets slowly (a preset recall or A/B switch). */
    void beginRecallGlide() noexcept { recallGlide = true; }

    /** E1, sunrise on open: start from night with the glow out; advance()
        then rises to the targets. */
    void prepareSunrise() noexcept;

    enum class Change { none, lower, all };

    /** Advances the springs. Returns what needs repainting. */
    Change advance (float dt) noexcept;

    /** The band Change::lower affects: the glow and the ridge. */
    static juce::Rectangle<int> lowerRegion (juce::Rectangle<int> bounds) noexcept;

    /** Paints the panel (clipped to its rounded outline) and its border. */
    void paint (juce::Graphics&, juce::Rectangle<float> bounds);

    /** The sky gradient's colour `proportionDown` of the way down the window
        (0 = top), without the glow - for components that fade into the sky
        (the preset row's scroll cue). */
    juce::Colour skyColourAt (float proportionDown) const;

private:
    juce::ColourGradient skyGradient (float top, float bottom) const;
    void renderCache (juce::Rectangle<int> bounds);
    void buildRidgeMasks (juce::Rectangle<int> bounds);
    void blendRidgeMask (float sigma);

    motion::Spring timeOfDay, haze, spread, glowLevel;
    bool recallGlide = false;
    bool rising = false;

    // Background cache (1x) and the ridge's blurred masks, see renderCache().
    juce::Image cache;
    bool cacheDirty = true;

    struct RidgeMask
    {
        float sigma = 0.0f;
        std::vector<float> coverage;   // half resolution, 0..1
    };

    std::vector<RidgeMask> ridgeMasks;
    juce::Image ridgeMask;              // the blend for the current haze, half resolution
    float ridgeMaskSigma = -1.0f;
    juce::Rectangle<int> ridgeBounds;   // where the mask lands, in panel coordinates (full resolution)
    int maskWidth = 0, maskHeight = 0;
};

} // namespace horizon::ui

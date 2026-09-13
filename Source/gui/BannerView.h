#pragma once

#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

/**
    The sunset-over-mountains banner from the mockup, drawn entirely in vector:
    a vertical gradient from a warm gold horizon glow up into a dusky
    blue/purple sky, a soft radial sun sitting on the horizon line, and two
    layered mountain silhouette ranges along the bottom of the band.

    No bitmap assets anywhere in this plugin - the ridge lines are generated
    once from a fixed seed so the artwork is stable across redraws and across
    machines.
*/
class BannerView final : public juce::Component
{
public:
    BannerView();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void rebuildRidges();

    juce::Path farRidge, nearRidge;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BannerView)
};

} // namespace horizon::ui

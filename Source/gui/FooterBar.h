#pragma once

#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

/** The bottom strip: a thin mountain-skyline silhouette behind
    "HORIZON PAD - VST3" (left) and "BUFFER A - 4 LAYERS - 8 MACROS" (right),
    matching the mockup's footer. All vector, fixed seed, no bitmap assets. */
class FooterBar final : public juce::Component
{
public:
    FooterBar();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void rebuildRidge();

    juce::Path ridge;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FooterBar)
};

} // namespace horizon::ui

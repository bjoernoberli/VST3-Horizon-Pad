#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/** The bottom strip: a thin mountain-skyline silhouette behind
    "HORIZON PAD - VST3" (left) and "BUFFER A/B - 4 LAYERS - 8 MACROS" (right,
    the letter tracking whichever A/B slot is actually active), matching the
    mockup's footer. All vector, fixed seed, no bitmap assets. */
class FooterBar final : public juce::Component
{
public:
    explicit FooterBar (HorizonPadAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Re-reads the active A/B buffer from the processor. */
    void refreshFromProcessor();

private:
    void rebuildRidge();

    HorizonPadAudioProcessor& processor;
    juce::Path ridge;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FooterBar)
};

} // namespace horizon::ui

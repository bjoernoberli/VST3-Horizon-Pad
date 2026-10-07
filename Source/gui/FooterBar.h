#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/** The bottom strip: a thin top divider above "HORIZON PAD - VST3" (left),
    the "Wäg zom Läbe - a life of joy" tagline (centred - moved here from
    TitleBanner, which now just has the logo/wordmark), and
    "BUFFER A/B - 4 LAYERS - 6 MACROS" (right, the letter tracking whichever
    A/B slot is actually active). The mountain-skyline silhouette lives in
    the shared panel background (HorizonScene), not here, since the
    design's skyline is one element behind the whole panel rather than a
    per-strip decoration. */
class FooterBar final : public juce::Component
{
public:
    explicit FooterBar (HorizonPadAudioProcessor&);

    void paint (juce::Graphics&) override;

    /** Re-reads the active A/B buffer from the processor; repaints only if it changed. */
    void refreshFromProcessor();

private:
    HorizonPadAudioProcessor& processor;
    int shownBuffer = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FooterBar)
};

} // namespace horizon::ui

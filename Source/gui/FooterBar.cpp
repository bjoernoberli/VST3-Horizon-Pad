#include "FooterBar.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

FooterBar::FooterBar (HorizonPadAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    setInterceptsMouseClicks (false, false);
}

void FooterBar::refreshFromProcessor()
{
    repaint();
}

void FooterBar::paint (juce::Graphics& g)
{
    // No background/skyline of its own - the shared panel (drawHorizonPanel)
    // already paints the gradient and mountain silhouette behind the whole
    // window; this strip only draws the top divider and the three labels.
    auto bounds = getLocalBounds().toFloat();

    g.setColour (Palette::dividerColor);
    g.fillRect (bounds.removeFromTop (1.0f));

    auto r = bounds.reduced (0.0f, 4.0f).toNearestInt();

    g.setColour (Palette::textFooter);
    g.setFont (labelFont (12.5f, true));
    g.drawText ("HORIZON PAD " + juce::String::fromUTF8 ("\xc2\xb7") + " VST3",
               r, juce::Justification::centredLeft);

    const auto bufferLetter = processor.getActiveBufferIndex() == 0 ? "A" : "B";

    g.drawText (juce::String ("BUFFER ") + bufferLetter + " " + juce::String::fromUTF8 ("\xc2\xb7") + " 4 LAYERS "
               + juce::String::fromUTF8 ("\xc2\xb7") + " 12 MACROS",
               r, juce::Justification::centredRight);

    // --- Tagline, centred between the two labels above (moved here from
    // TitleBanner, which now just has the logo and wordmark) - same font as
    // its neighbours, not the italic treatment it had in the banner.
    g.setFont (labelFont (12.5f, true));
    // Split after each \xNN escape (as separate literals) so the compiler's
    // greedy hex-escape parsing can't swallow the following letters (e.g.
    // "\xa4be" would otherwise be read as one 4-digit escape, not \xa4 + "be").
    g.drawText (juce::String::fromUTF8 ("W\xc3\xa4" "g zom L\xc3\xa4" "be \xe2\x80\x94 a life of joy"),
                r, juce::Justification::centred);
}

} // namespace horizon::ui

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
    // window; this strip only draws the top divider and the two labels.
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
}

} // namespace horizon::ui

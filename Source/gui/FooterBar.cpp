#include "FooterBar.h"

namespace horizon::ui
{

FooterBar::FooterBar()
{
    setInterceptsMouseClicks (false, false);
}

void FooterBar::resized()
{
    rebuildRidge();
}

void FooterBar::rebuildRidge()
{
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();

    if (w <= 1.0f || h <= 1.0f)
        return;

    juce::Random rng (0x480b17); // fixed seed: part of the product's look, must not reshuffle

    ridge.clear();
    ridge.startNewSubPath (0.0f, h);
    ridge.lineTo (0.0f, h * 0.55f);

    constexpr int peakCount = 9;
    const auto step = w / (float) peakCount;
    float x = 0.0f;

    for (int i = 0; i < peakCount; ++i)
    {
        const auto peakX = x + step * (0.3f + 0.4f * rng.nextFloat());
        const auto peakY = h * (0.15f + 0.35f * rng.nextFloat());
        const auto nextX = x + step;
        const auto valleyY = h * (0.5f + 0.08f * rng.nextFloat());

        ridge.lineTo (peakX, peakY);
        ridge.lineTo (nextX, valleyY);

        x = nextX;
    }

    ridge.lineTo (w, h * 0.55f);
    ridge.lineTo (w, h);
    ridge.closeSubPath();
}

void FooterBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (Palette::panelBorder);
    g.fillRect (bounds.removeFromTop (1.0f));

    g.setColour (Palette::panel.withAlpha (0.6f));
    g.fillPath (ridge);

    auto r = getLocalBounds().reduced (16, 0);

    g.setColour (Palette::textFaint);
    g.setFont (labelFont (10.0f, true));
    g.drawText ("HORIZON PAD " + juce::String::fromUTF8 ("\xc2\xb7") + " VST3",
               r, juce::Justification::centredLeft);

    g.drawText ("BUFFER A " + juce::String::fromUTF8 ("\xc2\xb7") + " 4 LAYERS "
               + juce::String::fromUTF8 ("\xc2\xb7") + " 8 MACROS",
               r, juce::Justification::centredRight);
}

} // namespace horizon::ui

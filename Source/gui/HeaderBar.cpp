#include "HeaderBar.h"

namespace horizon::ui
{

HeaderBar::HeaderBar()
{
    for (auto* b : { &prevButton, &nextButton })
    {
        b->setColour (juce::TextButton::buttonColourId, Palette::panel);
        b->setColour (juce::TextButton::textColourOffId, Palette::textDim);
        addAndMakeVisible (*b);
    }

    prevButton.onClick = [this] { if (onPreviousPreset) onPreviousPreset(); };
    nextButton.onClick = [this] { if (onNextPreset)     onNextPreset(); };
}

HeaderBar::~HeaderBar() = default;

void HeaderBar::setPresetName (const juce::String& name)
{
    if (presetName == name)
        return;

    presetName = name;
    repaint (presetFieldBounds.getSmallestIntegerContainer());
}

void HeaderBar::resized()
{
    auto r = getLocalBounds().reduced (16, 12);

    const auto arrowWidth = 24;
    auto right = r.removeFromRight (34);                 // room for the menu glyph
    juce::ignoreUnused (right);

    auto field = r.removeFromRight (250);
    prevButton.setBounds (field.removeFromLeft (arrowWidth).reduced (1));
    nextButton.setBounds (field.removeFromRight (arrowWidth).reduced (1));
    presetFieldBounds = field.toFloat();
}

void HeaderBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Header sits slightly lighter than the window ground, with a hairline base.
    g.setColour (Palette::panel);
    g.fillRect (bounds);
    g.setColour (Palette::panelBorder);
    g.fillRect (bounds.removeFromBottom (1.0f));

    auto r = getLocalBounds().reduced (16, 0);

    // ---------------------------------------------------------------- logo
    {
        auto logoArea = r.removeFromLeft (30).toFloat().withSizeKeepingCentre (26.0f, 20.0f);

        juce::Path peaks;
        peaks.startNewSubPath (logoArea.getX(), logoArea.getBottom());
        peaks.lineTo (logoArea.getX() + logoArea.getWidth() * 0.34f, logoArea.getY() + 2.0f);
        peaks.lineTo (logoArea.getX() + logoArea.getWidth() * 0.56f, logoArea.getBottom() * 1.0f
                                                                     - logoArea.getHeight() * 0.32f);
        peaks.lineTo (logoArea.getX() + logoArea.getWidth() * 0.72f, logoArea.getY() + logoArea.getHeight() * 0.30f);
        peaks.lineTo (logoArea.getRight(), logoArea.getBottom());
        peaks.closeSubPath();

        juce::ColourGradient grad (Palette::sunsetGlow, logoArea.getCentreX(), logoArea.getY(),
                                   Palette::sunsetMid,  logoArea.getCentreX(), logoArea.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (peaks);
    }

    r.removeFromLeft (10);

    // ------------------------------------------------------------ wordmark
    {
        auto wordmark = r.removeFromLeft (150);
        g.setColour (Palette::text);
        g.setFont (labelFont (17.0f, true));
        g.drawText ("HORIZON PAD", wordmark, juce::Justification::centredLeft);
    }

    r.removeFromLeft (18);

    // ----------------------------------------------------------------- nav
    {
        auto nav = r.removeFromLeft (260);
        g.setColour (Palette::textFaint);
        g.setFont (labelFont (11.0f));
        g.drawText ("DREAMS   /   ATMOSPHERES   /   TEXTURES", nav, juce::Justification::centredLeft);
    }

    // ------------------------------------------------- menu glyph (far right)
    {
        auto menu = getLocalBounds().removeFromRight (34).withSizeKeepingCentre (16, 12).toFloat();
        g.setColour (Palette::textDim);

        for (int i = 0; i < 3; ++i)
            g.fillRoundedRectangle (menu.getX(), menu.getY() + (float) i * 5.0f,
                                    menu.getWidth(), 1.6f, 0.8f);
    }

    // ------------------------------------------------------- preset name field
    if (! presetFieldBounds.isEmpty())
    {
        drawPanel (g, presetFieldBounds, Palette::panelRaised, 6.0f);
        g.setColour (Palette::text);
        g.setFont (labelFont (13.0f, true));
        g.drawText (presetName, presetFieldBounds, juce::Justification::centred);
    }

    // --------------------------------------------- version tag + VST3 badge
    {
        const auto rightEdge = (float) prevButton.getX() - 16.0f;
        auto area = juce::Rectangle<float> (rightEdge - 120.0f, 0.0f, 120.0f, (float) getHeight())
                        .reduced (0.0f, 16.0f);

        auto vst3 = area.removeFromRight (46.0f);
        g.setColour (Palette::layerAccents[1].withAlpha (0.18f));
        g.fillRoundedRectangle (vst3, 4.0f);
        g.setColour (Palette::layerAccents[1]);
        g.setFont (labelFont (9.5f, true));
        g.drawText ("VST3", vst3, juce::Justification::centred);

        area.removeFromRight (8.0f);

        auto version = area.removeFromRight (52.0f);
        g.setColour (Palette::panelRaised);
        g.fillRoundedRectangle (version, 4.0f);
        g.setColour (Palette::textDim);
        g.setFont (labelFont (9.5f));
        g.drawText ("v1.0.0", version, juce::Justification::centred);
    }
}

} // namespace horizon::ui

#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

HorizonLookAndFeel::HorizonLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Palette::background);
    setColour (juce::Label::textColourId,                 Palette::text);
    setColour (juce::Slider::rotarySliderFillColourId,    Palette::layerAccents[0]);
    setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);
    setColour (juce::Slider::textBoxTextColourId,         Palette::textDim);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId,          Palette::panelRaised);
    setColour (juce::TextButton::buttonOnColourId,        Palette::layerAccents[0]);
    setColour (juce::TextButton::textColourOffId,         Palette::textDim);
    setColour (juce::TextButton::textColourOnId,          Palette::background);
    setColour (juce::TooltipWindow::backgroundColourId,   Palette::panelRaised);
}

juce::Font HorizonLookAndFeel::getLabelFont (juce::Label& label)
{
    return labelFont ((float) juce::jmax (10, label.getHeight() - 2));
}

juce::Font HorizonLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return labelFont (juce::jlimit (9.0f, 15.0f, (float) buttonHeight * 0.55f), true);
}

void HorizonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto corner = juce::jmin (6.0f, bounds.getHeight() * 0.3f);

    auto fill = backgroundColour;

    if (shouldDrawButtonAsDown)
        fill = fill.brighter (0.25f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.12f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (Palette::panelBorder);
    g.drawRoundedRectangle (bounds, corner, 1.0f);
}

void HorizonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre (size, size);

    const auto centre = bounds.getCentre();
    const auto arcThickness = juce::jmax (2.5f, size * 0.085f);
    const auto arcRadius = size * 0.5f - arcThickness * 0.5f;
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // --- Track
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
        g.strokePath (track, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // --- Value arc
    if (sliderPos > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, angle, true);
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // --- Knob cap
    const auto capRadius = arcRadius - arcThickness * 1.15f;

    if (capRadius > 2.0f)
    {
        const auto capBounds = juce::Rectangle<float> (capRadius * 2.0f, capRadius * 2.0f)
                                   .withCentre (centre);

        juce::ColourGradient cap (Palette::panelRaised.brighter (0.16f), capBounds.getCentreX(), capBounds.getY(),
                                  Palette::panel.darker (0.35f),          capBounds.getCentreX(), capBounds.getBottom(),
                                  false);
        g.setGradientFill (cap);
        g.fillEllipse (capBounds);

        g.setColour (Palette::panelBorder);
        g.drawEllipse (capBounds.reduced (0.5f), 1.0f);

        // --- Pointer
        juce::Path pointer;
        const auto pointerLength = capRadius * 0.72f;
        const auto pointerWidth = juce::jmax (1.6f, size * 0.035f);
        pointer.addRoundedRectangle (-pointerWidth * 0.5f, -capRadius * 0.92f,
                                     pointerWidth, pointerLength, pointerWidth * 0.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

        g.setColour (accent.brighter (0.3f));
        g.fillPath (pointer);
    }
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour fill, float corner)
{
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (Palette::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
}

} // namespace horizon::ui

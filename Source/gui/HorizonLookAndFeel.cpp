#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

HorizonLookAndFeel::HorizonLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Palette::background);
    setColour (juce::Label::textColourId,                 Palette::text);
    setColour (juce::Slider::rotarySliderFillColourId,    Palette::layerAccents[0]);
    setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);
    setColour (juce::Slider::trackColourId,               Palette::knobTrack);
    setColour (juce::Slider::thumbColourId,               Palette::layerAccents[0]);
    setColour (juce::Slider::textBoxTextColourId,         Palette::textDim);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId,          Palette::panelRaised);
    setColour (juce::TextButton::buttonOnColourId,        Palette::layerAccents[1]);
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
    return labelFont (juce::jlimit (9.0f, 15.0f, (float) buttonHeight * 0.5f), true);
}

void HorizonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto corner = bounds.getHeight() * 0.5f; // full pill, matching the mockup's preset buttons

    auto fill = backgroundColour;

    if (shouldDrawButtonAsDown)
        fill = fill.brighter (0.25f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.12f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (button.getToggleState() ? fill : Palette::panelBorder);
    g.drawRoundedRectangle (bounds, corner, 1.0f);
}

void HorizonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider& slider)
{
    // Ring + dot style, matching the design handoff's ringKnob() helper: a
    // thin dark track ring, an accent-coloured ring lit up to the current
    // value, a dark cap, and a small glowing dot marking the exact value
    // position - not a thick glowing filled arc (an earlier iteration of
    // this look and feel).
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre (size, size);

    const auto centre = bounds.getCentre();
    const auto arcThickness = juce::jmax (2.0f, size * 0.07f);
    const auto arcRadius = size * 0.5f - arcThickness * 0.5f;
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // --- Track (the full 270 degree sweep, dark).
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
        g.strokePath (track, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // --- Value ring: lit from the start up to the current value, same
    // thickness as the track (a ring reading as "how full", not a fader).
    if (sliderPos > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, angle, true);

        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // --- Dark cap (mostly empty centre - the design's knobs read as rings,
    // not solid dials).
    const auto capRadius = arcRadius - arcThickness * 1.1f;

    if (capRadius > 2.0f)
    {
        const auto capBounds = juce::Rectangle<float> (capRadius * 2.0f, capRadius * 2.0f)
                                   .withCentre (centre);

        g.setColour (Palette::panelRaised);
        g.fillEllipse (capBounds);
    }

    // --- Glowing dot at the current value's position along the ring.
    {
        const auto dotCentre = centre.getPointOnCircumference (arcRadius, angle);
        const auto dotRadius = arcThickness * 0.62f;

        g.setColour (accent.withAlpha (0.30f));
        g.fillEllipse (juce::Rectangle<float> (dotRadius * 3.2f, dotRadius * 3.2f).withCentre (dotCentre));

        g.setColour (accent);
        g.fillEllipse (juce::Rectangle<float> (dotRadius * 2.0f, dotRadius * 2.0f).withCentre (dotCentre));
    }
}

void HorizonLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float minSliderPos, float maxSliderPos,
                                           const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos, style);

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto trackWidth = juce::jmin (bounds.getWidth(), 10.0f);
    auto track = bounds.withSizeKeepingCentre (trackWidth, bounds.getHeight()).reduced (0.0f, 2.0f);

    const auto accent = slider.findColour (juce::Slider::thumbColourId);

    g.setColour (Palette::knobTrack);
    g.fillRoundedRectangle (track, trackWidth * 0.5f);

    // Fill from the bottom (rest position) up to the thumb - a "wheel" reads
    // naturally as "how far pushed from rest", whether the range is unipolar
    // (MOD) or bipolar (PITCH, resting in the middle).
    const auto restY = slider.isVertical() && slider.getMinimum() < 0.0
                          ? track.getCentreY()
                          : track.getBottom();

    auto fillBounds = track;
    fillBounds.setTop (juce::jmin (sliderPos, restY));
    fillBounds.setBottom (juce::jmax (sliderPos, restY));

    if (fillBounds.getHeight() > 0.5f)
    {
        g.setColour (accent.withAlpha (0.85f));
        g.fillRoundedRectangle (fillBounds, trackWidth * 0.5f);
    }

    // Thumb: a rounded pill, like a hardware fader cap.
    const auto thumbWidth = bounds.getWidth() * 0.82f;
    const auto thumbHeight = 20.0f;
    auto thumb = juce::Rectangle<float> (thumbWidth, thumbHeight)
                    .withCentre ({ bounds.getCentreX(), sliderPos });

    g.setColour (accent);
    g.fillRoundedRectangle (thumb, thumbHeight * 0.4f);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour fill, float corner)
{
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (Palette::panelBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
}

} // namespace horizon::ui

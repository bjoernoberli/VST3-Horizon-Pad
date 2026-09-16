#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

HorizonLookAndFeel::HorizonLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Palette::background);
    setColour (juce::Label::textColourId,                 Palette::text);
    setColour (juce::Slider::rotarySliderFillColourId,    Palette::layerAccents[0]);
    setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);
    setColour (juce::Slider::trackColourId,               Palette::cardBg);
    setColour (juce::Slider::thumbColourId,               Palette::gold);
    setColour (juce::Slider::textBoxTextColourId,         Palette::textDim);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId,          juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonOnColourId,        Palette::pillActiveBg);
    setColour (juce::TextButton::textColourOffId,         Palette::text);
    setColour (juce::TextButton::textColourOnId,          Palette::gold);
    setColour (juce::TooltipWindow::backgroundColourId,   Palette::cardBg);
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
    const auto corner = bounds.getHeight() * 0.5f; // full pill, matching the design's preset/tab buttons

    auto fill = backgroundColour;

    if (shouldDrawButtonAsDown)
        fill = fill.brighter (0.25f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.12f);

    if (fill.getFloatAlpha() > 0.001f)
    {
        g.setColour (fill);
        g.fillRoundedRectangle (bounds, corner);
    }

    const auto& props = button.getProperties();

    if ((bool) props.getWithDefault ("noBorder", false))
        return;

    juce::Colour borderColour = button.getToggleState() ? Palette::gold : Palette::pillBorder;

    if (! button.getToggleState() && props.contains ("borderColour"))
        borderColour = juce::Colour ((juce::uint32) (int) props["borderColour"]);

    if ((bool) props.getWithDefault ("dashedBorder", false) && ! button.getToggleState())
    {
        juce::Path pillPath;
        pillPath.addRoundedRectangle (bounds, corner);

        juce::Path dashed;
        const float dashLengths[] { 4.0f, 3.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, pillPath, dashLengths, 2);
        g.setColour (borderColour);
        g.fillPath (dashed);
        return;
    }

    g.setColour (borderColour);
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
    const auto arcThickness = juce::jmax (2.0f, size * 0.1f);
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
    const auto capRadius = arcRadius - arcThickness * 1.0f;

    if (capRadius > 2.0f)
    {
        const auto capBounds = juce::Rectangle<float> (capRadius * 2.0f, capRadius * 2.0f)
                                   .withCentre (centre);

        g.setColour (Palette::knobInner);
        g.fillEllipse (capBounds);
    }

    // --- Glowing dot at the current value's position along the ring.
    {
        const auto dotCentre = centre.getPointOnCircumference (arcRadius, angle);
        const auto dotRadius = juce::jmax (2.5f, size * 0.057f);

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
    // A flat track with a solid pill thumb - matching the design's
    // pitchTrackStyle/pitchThumbStyle exactly (no fill-from-rest bar).
    juce::ignoreUnused (minSliderPos, maxSliderPos, style);

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto trackWidth = juce::jmin (bounds.getWidth(), 24.0f);
    auto track = bounds.withSizeKeepingCentre (trackWidth, bounds.getHeight());

    g.setColour (Palette::cardBg);
    g.fillRoundedRectangle (track, trackWidth * 0.5f);
    g.setColour (Palette::pillBorder);
    g.drawRoundedRectangle (track.reduced (0.5f), trackWidth * 0.5f, 1.0f);

    // Thumb: a rounded pill, sized and positioned exactly as the design's
    // 20x24 thumb inside a 24-wide track (2px margin each side).
    const auto thumbWidth = trackWidth * 0.83f;
    const auto thumbHeight = 24.0f;
    auto thumb = juce::Rectangle<float> (thumbWidth, thumbHeight)
                    .withCentre ({ bounds.getCentreX(), sliderPos });

    const auto accent = slider.findColour (juce::Slider::thumbColourId);

    g.setColour (accent.withAlpha (0.5f));
    g.fillRoundedRectangle (thumb.expanded (3.0f), (thumbHeight + 6.0f) * 0.5f);

    g.setColour (accent);
    g.fillRoundedRectangle (thumb, thumbHeight * 0.28f);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour fill, float corner)
{
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (Palette::cardBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
}

void paintPanelSurface (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // --- Gradient fill: near-black at the bottom rising to deep amber
    // at the top, matching panelStyle's four gradient stops exactly.
    juce::ColourGradient grad (Palette::panelGradientBottom, bounds.getX(), bounds.getBottom(),
                               Palette::panelGradientTop, bounds.getX(), bounds.getY(), false);
    grad.addColour (0.45, Palette::panelGradientLower);
    grad.addColour (0.78, Palette::panelGradientUpper);
    g.setGradientFill (grad);
    g.fillRect (bounds);

    // --- Mountain-skyline silhouette along the bottom (fixed jagged
    // polygon, matching skylineStyle's clip-path points exactly).
    {
        const auto skylineHeight = juce::jmin (120.0f, bounds.getHeight() * 0.4f);
        auto skylineArea = bounds.removeFromBottom (skylineHeight);
        const auto w = skylineArea.getWidth();
        const auto top = skylineArea.getY();
        const auto h = skylineArea.getHeight();

        const float pointsPct[][2] {
            { 0.0f, 100.0f }, { 0.0f, 78.0f }, { 9.0f, 60.0f }, { 18.0f, 82.0f },
            { 29.0f, 55.0f }, { 40.0f, 84.0f }, { 52.0f, 58.0f }, { 64.0f, 86.0f },
            { 76.0f, 56.0f }, { 88.0f, 80.0f }, { 100.0f, 62.0f }, { 100.0f, 100.0f },
        };

        juce::Path skyline;
        skyline.startNewSubPath (skylineArea.getX() + pointsPct[0][0] * 0.01f * w,
                                 top + pointsPct[0][1] * 0.01f * h);

        for (auto& p : pointsPct)
            skyline.lineTo (skylineArea.getX() + p[0] * 0.01f * w, top + p[1] * 0.01f * h);

        skyline.closeSubPath();

        g.setColour (Palette::skyline);
        g.fillPath (skyline);
    }

    // --- Soft gold glow, top-right corner (radial-gradient(circle, gold/0.16, transparent 70%)).
    {
        const auto glowDiameter = 420.0f;
        const auto glowCentre = juce::Point<float> (bounds.getRight() - 100.0f + glowDiameter * 0.5f,
                                                    bounds.getY() - 140.0f + glowDiameter * 0.5f);

        juce::ColourGradient radial (Palette::glow, glowCentre.x, glowCentre.y,
                                    Palette::glow.withAlpha (0.0f), glowCentre.x, glowCentre.y - glowDiameter * 0.35f, true);
        g.setGradientFill (radial);
        g.fillEllipse (juce::Rectangle<float> (glowDiameter, glowDiameter).withCentre (glowCentre));
    }
}

void drawHorizonPanel (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    constexpr float corner = 24.0f;

    juce::Path panelPath;
    panelPath.addRoundedRectangle (bounds, corner);

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (panelPath);
        paintPanelSurface (g, bounds);
    }

    g.setColour (Palette::panelBorder);
    g.strokePath (panelPath, juce::PathStrokeType (1.0f));
}

ColumnSlots computeColumnSlots (juce::Rectangle<int> cardBounds)
{
    auto r = cardBounds.reduced (12, 18);

    ColumnSlots slots;
    slots.icon = r.removeFromTop (28);
    r.removeFromTop (7);
    slots.label = r.removeFromTop (16);
    r.removeFromTop (7);
    slots.caption = r.removeFromTop (30);
    r.removeFromTop (7);
    slots.value = r.removeFromBottom (18);
    r.removeFromBottom (7);
    slots.control = r;

    return slots;
}

void drawColumnCard (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour (Palette::cardBg);
    g.fillRoundedRectangle (bounds, 14.0f);

    g.setColour (Palette::cardBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 14.0f, 1.0f);
}

} // namespace horizon::ui

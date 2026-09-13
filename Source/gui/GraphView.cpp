#include "GraphView.h"

namespace horizon::ui
{

GraphView::GraphView (juce::String titleToUse, juce::String minLabelToUse, juce::String maxLabelToUse,
                      juce::Colour accentToUse, Style styleToUse)
    : title (std::move (titleToUse)),
      minLabel (std::move (minLabelToUse)),
      maxLabel (std::move (maxLabelToUse)),
      accent (accentToUse),
      style (styleToUse)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (24);
}

GraphView::~GraphView()
{
    stopTimer();
}

void GraphView::timerCallback()
{
    phase += 0.012f;

    if (phase > juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;

    // Ballistic smoothing so the curve swells and decays instead of flickering.
    const auto target = std::sqrt (activity) * 3.0f;
    smoothedActivity += (juce::jlimit (0.0f, 1.0f, target) - smoothedActivity) * 0.18f;

    repaint();
}

void GraphView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawPanel (g, bounds.reduced (0.5f));

    auto r = getLocalBounds().reduced (14, 12);

    // ---------------------------------------------------------------- header
    {
        auto header = r.removeFromTop (16);

        g.setColour (Palette::text);
        g.setFont (labelFont (11.0f, true));
        g.drawText (title, header.removeFromLeft (80), juce::Justification::centredLeft);

        g.setColour (Palette::textFaint);
        g.setFont (labelFont (9.5f));
        g.drawText (minLabel + "  ..  " + maxLabel, header, juce::Justification::centredRight);
    }

    r.removeFromTop (8);

    auto plot = r.toFloat();

    if (plot.getWidth() < 8.0f || plot.getHeight() < 8.0f)
        return;

    // ------------------------------------------------------------------ grid
    {
        g.setColour (Palette::panelBorder.withAlpha (0.55f));

        for (int i = 0; i <= 4; ++i)
        {
            const auto y = plot.getY() + plot.getHeight() * (float) i * 0.25f;
            g.fillRect (plot.getX(), y, plot.getWidth(), 1.0f);
        }

        for (int i = 1; i < 6; ++i)
        {
            const auto x = plot.getX() + plot.getWidth() * (float) i / 6.0f;
            g.fillRect (x, plot.getY(), 1.0f, plot.getHeight());
        }
    }

    // ------------------------------------------------------------------ curve
    const auto centreY = style == Style::bipolar ? plot.getCentreY() : plot.getBottom();
    const auto span = style == Style::bipolar ? plot.getHeight() * 0.44f : plot.getHeight() * 0.86f;

    const auto amplitude = 0.22f + 0.78f * smoothedActivity;

    juce::Path curve;
    juce::Path fill;

    const int steps = juce::jmax (16, (int) plot.getWidth() / 2);

    for (int i = 0; i <= steps; ++i)
    {
        const auto t = (float) i / (float) steps;
        const auto x = plot.getX() + t * plot.getWidth();

        // Three incommensurate partials: never repeats inside the visible window.
        auto value = 0.62f * std::sin (t * 6.1f + phase * 1.7f)
                   + 0.26f * std::sin (t * 11.7f - phase * 2.6f)
                   + 0.12f * std::sin (t * 19.3f + phase * 4.1f);

        if (style == Style::unipolar)
            value = value * 0.5f + 0.5f;

        const auto y = centreY - value * span * amplitude;

        if (i == 0)
        {
            curve.startNewSubPath (x, y);
            fill.startNewSubPath (x, plot.getBottom());
            fill.lineTo (x, y);
        }
        else
        {
            curve.lineTo (x, y);
            fill.lineTo (x, y);
        }
    }

    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.closeSubPath();

    juce::ColourGradient under (accent.withAlpha (0.28f), plot.getCentreX(), plot.getY(),
                                accent.withAlpha (0.0f),  plot.getCentreX(), plot.getBottom(), false);
    g.setGradientFill (under);
    g.fillPath (fill);

    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Zero / baseline reference.
    g.setColour (Palette::textFaint.withAlpha (0.5f));
    g.fillRect (plot.getX(), centreY, plot.getWidth(), 1.0f);
}

} // namespace horizon::ui

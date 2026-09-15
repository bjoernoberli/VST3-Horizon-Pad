#include "TitleBanner.h"

namespace horizon::ui
{

TitleBanner::TitleBanner()
{
    setInterceptsMouseClicks (false, false);
}

void TitleBanner::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll (Palette::background);

    // --- Warm radial glow behind the title.
    {
        const auto glowCentre = bounds.getCentre().withY (bounds.getY() + bounds.getHeight() * 0.35f);
        const auto glowRadius = bounds.getWidth() * 0.38f;

        juce::ColourGradient glow (Palette::glowMid.withAlpha (0.55f), glowCentre.x, glowCentre.y,
                                   Palette::background.withAlpha (0.0f), glowCentre.x, glowCentre.y - glowRadius, true);
        glow.isRadial = true;
        glow.addColour (0.35, Palette::glowMid.withAlpha (0.28f));
        g.setGradientFill (glow);
        g.fillRect (bounds);
    }

    // --- Mountain-sunrise emblem.
    {
        const auto emblemSize = 34.0f;
        auto emblem = juce::Rectangle<float> (emblemSize, emblemSize)
                         .withCentre ({ bounds.getCentreX(), bounds.getY() + 34.0f });

        juce::Path peak;
        peak.startNewSubPath (emblem.getX(), emblem.getBottom());
        peak.lineTo (emblem.getCentreX(), emblem.getY());
        peak.lineTo (emblem.getRight(), emblem.getBottom());
        peak.closeSubPath();

        juce::ColourGradient peakGrad (Palette::glowCore, emblem.getCentreX(), emblem.getY(),
                                       Palette::macroAccents[0].darker (0.2f), emblem.getCentreX(), emblem.getBottom(), false);
        g.setGradientFill (peakGrad);
        g.fillPath (peak);

        // A small sun/glow disc sitting on the peak.
        const auto sunR = emblemSize * 0.16f;
        g.setColour (Palette::glowCore.withAlpha (0.9f));
        g.fillEllipse (emblem.getCentreX() - sunR, emblem.getY() - sunR * 0.4f, sunR * 2.0f, sunR * 2.0f);
    }

    // --- Wordmark.
    {
        auto titleArea = bounds.withY (bounds.getY() + 58.0f).withHeight (40.0f);
        g.setColour (Palette::text);
        g.setFont (titleFont (30.0f));
        g.drawText ("Horizon Pad", titleArea, juce::Justification::centred);
    }

    // --- Tagline.
    {
        auto tagArea = bounds.withY (bounds.getY() + 100.0f).withHeight (20.0f);
        g.setColour (Palette::textDim);
        g.setFont (labelFont (13.0f));
        // Split after each \xNN escape (as separate literals) so the compiler's
        // greedy hex-escape parsing can't swallow the following letters (e.g.
        // "\xa4be" would otherwise be read as one 4-digit escape, not \xa4 + "be").
        g.drawText (juce::String::fromUTF8 ("W\xc3\xa4" "g zom L\xc3\xa4" "be - a life of joy"),
                    tagArea, juce::Justification::centred);
    }
}

} // namespace horizon::ui

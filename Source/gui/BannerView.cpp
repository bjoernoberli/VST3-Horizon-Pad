#include "BannerView.h"

namespace horizon::ui
{

namespace
{
    /** Builds one ridge line as a closed path filling the bottom of the band. */
    juce::Path makeRidge (float width, float height, float baseY, float amplitude,
                          int peakCount, juce::Random& rng)
    {
        juce::Path p;
        p.startNewSubPath (0.0f, height);
        p.lineTo (0.0f, baseY);

        const auto step = width / (float) peakCount;
        float x = 0.0f;

        for (int i = 0; i < peakCount; ++i)
        {
            const auto peakX = x + step * (0.3f + 0.4f * rng.nextFloat());
            const auto peakY = baseY - amplitude * (0.45f + 0.55f * rng.nextFloat());
            const auto nextX = x + step;
            const auto valleyY = baseY - amplitude * 0.12f * rng.nextFloat();

            p.lineTo (peakX, peakY);
            p.lineTo (nextX, valleyY);

            x = nextX;
        }

        p.lineTo (width, baseY);
        p.lineTo (width, height);
        p.closeSubPath();

        return p;
    }
}

BannerView::BannerView()
{
    setInterceptsMouseClicks (false, false);
}

void BannerView::resized()
{
    rebuildRidges();
}

void BannerView::rebuildRidges()
{
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();

    if (w <= 1.0f || h <= 1.0f)
        return;

    // Fixed seed: the mountain range is part of the product's look, so it must
    // not reshuffle every time the editor is resized or reopened.
    juce::Random rng (0x480b17);

    farRidge  = makeRidge (w, h, h * 0.74f, h * 0.30f, 7, rng);
    nearRidge = makeRidge (w, h, h * 0.88f, h * 0.36f, 5, rng);
}

void BannerView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto w = bounds.getWidth();
    const auto h = bounds.getHeight();
    const auto horizonY = h * 0.74f;

    // ------------------------------------------------------------------ sky
    juce::ColourGradient sky (Palette::duskTop, bounds.getCentreX(), bounds.getY(),
                              Palette::sunsetGlow, bounds.getCentreX(), horizonY, false);
    sky.addColour (0.34, Palette::duskUpper);
    sky.addColour (0.66, Palette::sunsetMid);
    sky.addColour (0.87, Palette::sunsetCore);

    g.setGradientFill (sky);
    g.fillRect (bounds);

    // ------------------------------------------------------------------ sun
    {
        const auto sunX = w * 0.62f;
        const auto sunR = h * 0.42f;

        juce::ColourGradient glow (Palette::sunsetGlow.withAlpha (0.95f), sunX, horizonY,
                                   Palette::sunsetGlow.withAlpha (0.0f),  sunX, horizonY - sunR, true);
        glow.isRadial = true;
        g.setGradientFill (glow);
        g.fillEllipse (sunX - sunR, horizonY - sunR, sunR * 2.0f, sunR * 2.0f);

        g.setColour (juce::Colours::white.withAlpha (0.55f));
        const auto coreR = h * 0.105f;
        g.fillEllipse (sunX - coreR, horizonY - coreR * 1.15f, coreR * 2.0f, coreR * 2.0f);
    }

    // --------------------------------------------------- mountain silhouettes
    g.setColour (Palette::mountainFar);
    g.fillPath (farRidge);

    g.setColour (Palette::mountainNear);
    g.fillPath (nearRidge);

    // ------------------------------------------- vignette + top/bottom fades
    {
        juce::ColourGradient topFade (Palette::background.withAlpha (0.85f), bounds.getCentreX(), bounds.getY(),
                                      Palette::background.withAlpha (0.0f),  bounds.getCentreX(), bounds.getY() + h * 0.22f,
                                      false);
        g.setGradientFill (topFade);
        g.fillRect (bounds.withHeight (h * 0.22f));

        juce::ColourGradient bottomFade (Palette::background.withAlpha (0.0f),  bounds.getCentreX(), h * 0.78f,
                                         Palette::background.withAlpha (0.92f), bounds.getCentreX(), h,
                                         false);
        g.setGradientFill (bottomFade);
        g.fillRect (bounds.withTop (h * 0.78f));
    }
}

} // namespace horizon::ui

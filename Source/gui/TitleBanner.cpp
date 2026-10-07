#include "TitleBanner.h"

namespace horizon::ui
{

namespace
{
    // Preset title: fade in, stay, fade out; a quick fade-out when another
    // recall replaces it before it is gone.
    constexpr double kTitleHoldSeconds = 4.0;
    constexpr float kTitleFadeIn = 0.18f;
    constexpr float kTitleFadeOut = 0.45f;
    constexpr float kTitleSwapOut = 0.06f;
    constexpr float kTitleDrift = 4.0f;       // px it rises while fading in

    // Sunrise: ~1.3 s from behind the peaks into place (motion::Spring).
    constexpr float kSunriseOmega = 3.6f;
    constexpr float kSunsetDepth = 17.0f;     // logo units below its place: hidden by the peaks

    constexpr float kLogoSize = 96.0f;
    constexpr float kLogoHaloReach = 18.0f;
}

TitleBanner::TitleBanner()
{
    setInterceptsMouseClicks (false, false);
    sunHeight.snapTo (1.0f);
}

void TitleBanner::showPresetTitle (const juce::String& name, const juce::String& description)
{
    if (titleAlpha.value > 0.0f)
    {
        pendingName = name;
        pendingDescription = description;
        hasPending = true;
    }
    else
    {
        titleName = name;
        titleDescription = description;
        hasPending = false;
    }

    startHold = true;
}

void TitleBanner::prepareSunrise() noexcept
{
    sunHeight.value = 0.0f;
    sunHeight.velocity = 0.0f;
    sunHeight.target = 1.0f;
}

juce::Rectangle<int> TitleBanner::logoArea() const
{
    return getLocalBounds().removeFromTop ((int) kLogoSize).withSizeKeepingCentre ((int) kLogoSize, (int) kLogoSize);
}

juce::Rectangle<int> TitleBanner::titleArea() const
{
    return getLocalBounds().withTrimmedTop (kHeaderHeight);
}

void TitleBanner::advance (double now, float dt)
{
    if (startHold)
    {
        titleHoldUntil = now + kTitleHoldSeconds;
        startHold = false;
    }

    const auto showing = ! hasPending && titleName.isNotEmpty() && now < titleHoldUntil;

    if (titleAlpha.advance (showing ? 1.0f : 0.0f, dt, kTitleFadeIn,
                            hasPending ? kTitleSwapOut : kTitleFadeOut, 2.0e-3f))
        repaint (titleArea());

    if (hasPending && titleAlpha.value <= 0.0f)
    {
        titleName = pendingName;
        titleDescription = pendingDescription;
        hasPending = false;
    }

    if (sunHeight.advance (dt, kSunriseOmega, 1.0e-3f))
        repaint (logoArea().expanded ((int) kLogoHaloReach + 2));
}

void TitleBanner::paint (juce::Graphics& g)
{
    // No background here - the shared panel (HorizonScene, painted once
    // behind the whole window by the editor) already provides the gradient,
    // skyline and glow. This component only draws the header content itself:
    // the mountain-sunrise logo (large), the wordmark below it, and the
    // preset title strip. The tagline that used to live here moved to
    // FooterBar, centred in the footer strip.
    auto r = getLocalBounds().toFloat();
    auto header = r.removeFromTop ((float) kHeaderHeight);

    // --- Logo: a sun over a mountain range, drawn in the SVG's own 46x46
    // unit space via a graphics transform (viewBox="0 0 46 46").
    const auto rise = juce::jlimit (0.0f, 1.0f, sunHeight.value);
    auto logo = header.removeFromTop (kLogoSize).withSizeKeepingCentre (kLogoSize, kLogoSize);
    {
        // A soft halo behind the emblem (the design's drop-shadow(0 0 10px ...)),
        // coming up with the sun.
        juce::ColourGradient halo (Palette::logoGold.withAlpha (0.35f * rise), logo.getCentreX(), logo.getCentreY(),
                                   Palette::logoGold.withAlpha (0.0f), logo.getCentreX(), logo.getY() - 6.0f, true);
        g.setGradientFill (halo);
        g.fillEllipse (logo.expanded (kLogoHaloReach));

        juce::Graphics::ScopedSaveState save (g);
        const auto scale = logo.getWidth() / 46.0f;
        g.addTransform (juce::AffineTransform::scale (scale).translated (logo.getX(), logo.getY()));

        // The sun - deeper orange while it is still low - kept above the
        // horizon line, so while rising it shows only between the peaks.
        {
            juce::Graphics::ScopedSaveState sunClip (g);
            g.reduceClipRegion (juce::Rectangle<int> (0, -20, 46, 53));
            g.setColour (juce::Colour (0xffe0703a).interpolatedWith (Palette::logoGold, rise));
            g.fillEllipse (23.0f - 10.0f, 20.0f - 10.0f + (1.0f - rise) * kSunsetDepth, 20.0f, 20.0f);
        }

        juce::Path mountain;
        mountain.startNewSubPath (0.0f, 33.0f);
        mountain.lineTo (9.0f, 21.0f);
        mountain.lineTo (16.0f, 29.0f);
        mountain.lineTo (23.0f, 15.0f);
        mountain.lineTo (30.0f, 29.0f);
        mountain.lineTo (37.0f, 21.0f);
        mountain.lineTo (46.0f, 33.0f);
        mountain.closeSubPath();
        g.setColour (Palette::logoMountain);
        g.fillPath (mountain);

        g.setColour (Palette::logoGold);
        g.drawLine (1.0f, 33.5f, 45.0f, 33.5f, 1.5f);
    }

    header.removeFromTop (8.0f);

    // --- Wordmark: solid warm gold, not a gradient.
    //
    // A gradient fill on drawText() was found (via a pixel-level inspection
    // of an offscreen createComponentSnapshot() render, independent of any
    // host/window compositing) to be the actual cause of a much bigger,
    // long-standing bug: the wordmark's white-to-gold ColourGradient was
    // being used to fill this *entire component's* bounds - not clipped to
    // the "Horizon Pad" glyphs at all - while the glyphs themselves never
    // rendered. That's a JUCE/CoreGraphics gradient-text rendering fault,
    // not anything about paint order, clipping calls, or host compositing
    // (every one of those theories was tried and ruled out first). A solid
    // colour fill for drawText() sidesteps it entirely.
    auto wordmarkArea = header.removeFromTop (46.0f);
    g.setColour (Palette::gold);
    g.setFont (titleFont (36.0f));
    g.drawText ("Horizon Pad", wordmarkArea, juce::Justification::centred);

    // --- Preset title: name in the wordmark's serif, then the preset's
    // one-line description, in the strip above the divider.
    const auto alpha = titleAlpha.value;

    if (alpha > 0.002f && titleName.isNotEmpty())
    {
        juce::AttributedString line;
        line.setJustification (juce::Justification::centred);
        line.append (titleName, titleFont (18.0f), Palette::presetTitle.withMultipliedAlpha (alpha));

        if (titleDescription.isNotEmpty())
        {
            line.append (juce::String::fromUTF8 ("   \xc2\xb7   "), labelFont (13.0f, true),
                         Palette::textFooter.withMultipliedAlpha (alpha));
            line.append (titleDescription, labelFont (13.5f).italicised(), Palette::text.withMultipliedAlpha (alpha));
        }

        juce::TextLayout layout;
        layout.createLayout (line, r.getWidth());

        const auto drift = (1.0f - alpha) * kTitleDrift;
        layout.draw (g, r.withSizeKeepingCentre (r.getWidth(), layout.getHeight()).translated (0.0f, drift));
    }
}

} // namespace horizon::ui

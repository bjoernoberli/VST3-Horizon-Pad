#include "TitleBanner.h"

namespace horizon::ui
{

TitleBanner::TitleBanner()
{
    setInterceptsMouseClicks (false, false);
}

void TitleBanner::paint (juce::Graphics& g)
{
    // No background here - the shared panel (drawHorizonPanel, painted once
    // behind the whole window by the editor) already provides the gradient,
    // skyline and glow. This component only draws the header content itself:
    // the mountain-sunrise logo (large), then the wordmark below it. The
    // tagline that used to live here moved to FooterBar, centred in the
    // footer strip.
    auto r = getLocalBounds().toFloat();

    // --- Logo: a sun over a mountain range, drawn in the SVG's own 46x46
    // unit space via a graphics transform (viewBox="0 0 46 46").
    auto logoArea = r.removeFromTop (96.0f).withSizeKeepingCentre (96.0f, 96.0f);
    {
        // A soft halo behind the emblem (the design's drop-shadow(0 0 10px ...)).
        juce::ColourGradient halo (Palette::logoGold.withAlpha (0.35f), logoArea.getCentreX(), logoArea.getCentreY(),
                                   Palette::logoGold.withAlpha (0.0f), logoArea.getCentreX(), logoArea.getY() - 6.0f, true);
        g.setGradientFill (halo);
        g.fillEllipse (logoArea.expanded (18.0f));

        juce::Graphics::ScopedSaveState save (g);
        const auto scale = logoArea.getWidth() / 46.0f;
        g.addTransform (juce::AffineTransform::scale (scale).translated (logoArea.getX(), logoArea.getY()));

        g.setColour (Palette::logoGold);
        g.fillEllipse (23.0f - 10.0f, 20.0f - 10.0f, 20.0f, 20.0f);

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

    r.removeFromTop (8.0f);

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
    auto wordmarkArea = r.removeFromTop (46.0f);
    g.setColour (Palette::gold);
    g.setFont (titleFont (36.0f));
    g.drawText ("Horizon Pad", wordmarkArea, juce::Justification::centred);
}

} // namespace horizon::ui

#pragma once

#include <JuceHeader.h>

namespace horizon::ui
{

/** The whole plugin's colour vocabulary in one place - values are exact
    conversions (OKLCH -> linear sRGB -> gamma-encoded sRGB, Bjorn Ottosson's
    OKLab formulas) of the oklch() colours in the authoritative Claude Design
    handoff ("Horizon Pad.dc.html"), so this is a pixel-accurate palette
    match, not an approximation. */
namespace Palette
{
    const juce::Colour background    { 0xff020306 };

    // The shared warm gradient panel behind the whole window (see
    // TitleBanner/FooterBar), bottom-to-top: near-black -> deep amber.
    const juce::Colour panelGradientBottom { 0xff05070d };
    const juce::Colour panelGradientLower  { 0xff060c13 };
    const juce::Colour panelGradientUpper  { 0xff211300 };
    const juce::Colour panelGradientTop    { 0xff311e00 };

    const juce::Colour panel         { 0xff10141b }; // card/pill background
    const juce::Colour panelRaised   { 0xff12161d }; // knob inner cap / raised chrome
    const juce::Colour panelBorder   { 0x99232933 };
    const juce::Colour panelShadow   { 0x80010000 };

    const juce::Colour text          { 0xffebeff5 };
    const juce::Colour textDim       { 0xff7f8793 };
    const juce::Colour textFaint     { 0xff6b727e };

    const juce::Colour knobTrack     { 0xff2a2e36 };
    const juce::Colour dotUnlit      { 0xff2f333b };

    // Warm amber/dusk glow behind the title banner, and the logo's gold.
    const juce::Colour glowCore      { 0xfffdc436 };
    const juce::Colour glowMid       { 0xfff5ae39 };
    const juce::Colour glowFar       { 0x29f5ae39 };

    // Per-pad accents, in LayerIndex order: Root, Clearing, Expanse, Bloom.
    const juce::Colour layerAccents[] {
        juce::Colour (0xff5bbd74),   // 1. Root (Warm Foundation)     - green,  "life grows"
        juce::Colour (0xfff0bb3b),   // 2. Clearing (Analog Ensemble) - amber,  "light breaks in"
        juce::Colour (0xff2fb5d8),   // 3. Expanse (Airy Choir)       - blue,   "life opens up"
        juce::Colour (0xffed76b3)    // 4. Bloom (Motion Pad)         - pink,   "life blooms"
    };

    // Macro accents: Attack, Filter, Width, Reverb - taking the design's four
    // macro-slot colours in order (its fourth slot is labelled DRIVE there;
    // WIDTH occupies it here instead, a considered, already-approved swap -
    // see FxChain's doc comment for why WIDTH replaced a distortion stage).
    const juce::Colour macroAccents[] {
        juce::Colour (0xfff5ae39),   // Attack - gold
        juce::Colour (0xff2fb5d8),   // Filter - blue
        juce::Colour (0xffed76b3),   // Width  - pink
        juce::Colour (0xff5bbd74)    // Reverb - green
    };

    // Pitch / mod wheels share one neutral performance-control accent.
    const juce::Colour wheelAccent   { 0xfff5ae39 };

    // Output meter.
    const juce::Colour meterBg       { 0xff040609 };
    const juce::Colour meterFillLow  { 0xffb37903 };
    const juce::Colour meterFillHigh { 0xfffac547 };
}

/** Shared typography helper; keeps every label on the same handful of type sizes. */
inline juce::Font labelFont (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions()
                           .withHeight (height)
                           .withStyle (bold ? "Bold" : "Regular"));
}

/** The serif italic "Horizon Pad" wordmark font. */
inline juce::Font titleFont (float height)
{
    return juce::Font (juce::FontOptions()
                           .withName ("Georgia")
                           .withHeight (height)
                           .withStyle ("Italic"));
}

/**
    Custom LookAndFeel: a thin, flat rotary that matches the mockup - a dark
    track arc, an accent-coloured value arc, a subtly shaded knob cap and a
    single pointer line - plus a pill-track vertical slider for the PITCH/MOD
    wheels. The accent colour is taken from the slider's rotarySliderFillColourId
    so each knob/wheel can tint itself independently.
*/
class HorizonLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    HorizonLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider&) override;

    juce::Font getLabelFont (juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HorizonLookAndFeel)
};

/** Rounded panel with a 1px border, used by preset pills and the header field. */
void drawPanel (juce::Graphics& g, juce::Rectangle<float> bounds,
                juce::Colour fill = Palette::panel, float corner = 10.0f);

} // namespace horizon::ui

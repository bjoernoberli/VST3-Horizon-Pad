#pragma once

#include <JuceHeader.h>

namespace horizon::ui
{

/** The whole plugin's colour vocabulary in one place - values are exact
    conversions (OKLCH -> linear sRGB -> gamma-encoded sRGB, Bjorn Ottosson's
    OKLab formulas) of the oklch() colours in the authoritative Claude Design
    handoff ("Horizon Pad.dc.html"), so this is a pixel-accurate palette
    match, not an approximation. Names follow the handoff's own style names
    (panelStyle, cardBg, pillBorder, ...) so the two stay easy to cross-check. */
namespace Palette
{
    const juce::Colour background       { 0xff020306 };

    // The one shared panel that is the whole plugin window: a warm gradient
    // (near-black at the bottom to deep amber at the top), a thin border, a
    // clipped mountain-skyline silhouette along the bottom, and a soft gold
    // glow in the top-right corner. See drawHorizonPanel().
    const juce::Colour panelGradientBottom { 0xff05070d };
    const juce::Colour panelGradientLower  { 0xff060c13 };
    const juce::Colour panelGradientUpper  { 0xff211300 };
    const juce::Colour panelGradientTop    { 0xff311e00 };
    const juce::Colour panelBorder         { 0x99232933 };
    const juce::Colour skyline             { 0x8010141b };
    const juce::Colour glow                { 0x29f5ae39 };

    // Grid-card chrome (PITCH/MOD/ROOT/CLEARING/EXPANSE/BLOOM/MACROS/OUTPUT).
    const juce::Colour cardBg           { 0xff10141b };
    const juce::Colour cardBorder       { 0x80282e38 };
    const juce::Colour iconGlyph        { 0xfff0bb3b };

    // Pills (preset/user-preset/tab/save buttons).
    const juce::Colour pillBorder       { 0xb22d333d };
    const juce::Colour pillActiveBg     { 0x29f5ae39 };
    const juce::Colour pillInactiveUserBg { 0x08ffffff };
    const juce::Colour saveInputBg      { 0x0affffff };
    const juce::Colour saveConfirmBorder{ 0xff5bbd74 };
    const juce::Colour saveConfirmBg    { 0x2e5bbd74 };

    // Text.
    const juce::Colour text             { 0xffebeff5 }; // "hi" - wordmark, active pill text
    const juce::Colour textKnobLabel    { 0xffe4e8ef }; // card labels (PITCH, ROOT, ...)
    const juce::Colour textValue        { 0xffcaced4 }; // card value readouts
    const juce::Colour textDim          { 0xff7f8793 }; // captions, tagline, macro values
    const juce::Colour textFaint        { 0xff79818d }; // delete/copy/cancel/save-open text
    const juce::Colour textFooter       { 0xff5d646f }; // footer strip text
    const juce::Colour macroLabel       { 0xffb4b8be };

    const juce::Colour dividerColor     { 0x80282e38 };

    const juce::Colour knobTrack        { 0xff2a2e36 }; // unlit portion of a ring knob
    const juce::Colour knobInner        { 0xff12161d }; // ring knob's dark cap
    const juce::Colour dotUnlit         { 0xff2f333b }; // layer status dot when silent

    const juce::Colour logoMountain     { 0xff080b12 };
    const juce::Colour logoGold         { 0xfff0bb3b };
    const juce::Colour wordmarkGoldEnd  { 0xffeabb79 };

    const juce::Colour gold             { 0xfff5ae39 }; // the one warm accent: active borders/text, wheel thumbs
    const juce::Colour thumbGlow        { 0x80f5ae39 };

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

/** The serif italic "Horizon Pad" wordmark font (the design specifies
    Newsreader italic; JUCE's generic serif placeholder - resolved to
    whatever serif the host OS actually has - is used rather than naming a
    specific family like "Georgia": asking for a named font that happens not
    to be installed doesn't just look wrong, it has been observed to crash
    JUCE's text shaper outright on at least one Linux setup, so the generic,
    always-resolvable placeholder is the safe choice here). */
inline juce::Font titleFont (float height)
{
    return juce::Font (juce::FontOptions()
                           .withName (juce::Font::getDefaultSerifFontName())
                           .withHeight (height)
                           .withStyle ("Italic"));
}

/**
    Custom LookAndFeel: a thin ring-and-dot rotary (matching the design's
    conic-gradient ringKnob() helper), a flat track-and-pill-thumb vertical
    slider for the PITCH/MOD wheels, and pill buttons whose border follows a
    small per-Component::Properties convention so one generic
    drawButtonBackground can produce every pill style the design uses:

      - by default: pillBorder when off, gold when toggled on;
      - "noBorder"     (bool)  - draw no border at all (a pill's inner
                                  sub-buttons, whose *parent* paints the
                                  shared border - see PresetBar's UserPresetPill);
      - "dashedBorder" (bool)  - dash the (untoggled) border - the "+ Save
                                  preset" control;
      - "borderColour" (int, packed ARGB) - a fixed border colour overriding
                                  the default/gold choice (e.g. the green
                                  "Save" confirm button).
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
                juce::Colour fill = Palette::cardBg, float corner = 10.0f);

/**
    Paints the one shared panel that is the whole plugin window - gradient
    fill, border, clipped mountain-skyline silhouette, and the top-right glow
    - exactly matching the design handoff's panelStyle/skylineStyle/glowStyle.
    Call once from the editor's paint(), behind every (transparent) child.
*/
void drawHorizonPanel (juce::Graphics& g, juce::Rectangle<float> bounds);

/**
    The fixed vertical slot layout every grid card (PITCH, MOD, ROOT,
    CLEARING, EXPANSE, BLOOM, MACROS, OUTPUT) shares, matching the design's
    colCardStyle exactly: 18px/12px padding, then icon row (28px), label
    (16px), caption (30px), a flexible control area, and a value readout
    (18px), each separated by a 7px gap.
*/
struct ColumnSlots
{
    juce::Rectangle<int> icon, label, caption, control, value;
};

ColumnSlots computeColumnSlots (juce::Rectangle<int> cardBounds);

/** Fills+strokes one grid card's rounded-rect chrome (cardBg/cardBorder, 14px corner). */
void drawColumnCard (juce::Graphics& g, juce::Rectangle<float> bounds);

} // namespace horizon::ui

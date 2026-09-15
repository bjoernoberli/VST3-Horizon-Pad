#pragma once

#include <JuceHeader.h>

namespace horizon::ui
{

/** The whole plugin's colour vocabulary in one place, matched to the Claude
    Design GUI draft (dusk/amber ambient theme). */
namespace Palette
{
    const juce::Colour background    { 0xff0c0a08 };
    const juce::Colour panel         { 0xff15120e };
    const juce::Colour panelRaised   { 0xff1c1712 };
    const juce::Colour panelBorder   { 0xff332a20 };
    const juce::Colour text          { 0xfff2ede4 };
    const juce::Colour textDim       { 0xffa39a8c };
    const juce::Colour textFaint     { 0xff6b6258 };
    const juce::Colour knobTrack     { 0xff2a241c };

    // Warm amber/dusk glow behind the title banner.
    const juce::Colour glowCore      { 0xffffb15e };
    const juce::Colour glowMid       { 0xffb5622f };
    const juce::Colour glowFar       { 0xff2a1810 };

    // Per-pad accents, in LayerIndex order: Root, Clearing, Expanse, Bloom.
    const juce::Colour layerAccents[] {
        juce::Colour (0xff4ade80),   // 1. Root (Warm Foundation)     - green,  "life grows"
        juce::Colour (0xfff5a623),   // 2. Clearing (Analog Ensemble) - amber,  "light breaks in"
        juce::Colour (0xff38bdf8),   // 3. Expanse (Airy Choir)       - blue,   "life opens up"
        juce::Colour (0xffec4899)    // 4. Bloom (Motion Pad)         - pink,   "life blooms"
    };

    // Macro accents: Attack, Filter, Width, Reverb.
    const juce::Colour macroAccents[] {
        juce::Colour (0xfff5a623),   // Attack - orange
        juce::Colour (0xff2dd4bf),   // Filter - teal
        juce::Colour (0xffa78bfa),   // Width  - purple
        juce::Colour (0xff86efac)    // Reverb - soft green
    };

    // Pitch / mod wheels share one neutral performance-control accent.
    const juce::Colour wheelAccent   { 0xfff5a623 };
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

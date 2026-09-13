#pragma once

#include <JuceHeader.h>

namespace horizon::ui
{

/** The whole plugin's colour vocabulary in one place. */
namespace Palette
{
    const juce::Colour background    { 0xff0e1320 };
    const juce::Colour panel         { 0xff161d2e };
    const juce::Colour panelRaised   { 0xff1c2437 };
    const juce::Colour panelBorder   { 0xff28324a };
    const juce::Colour text          { 0xffe8edf7 };
    const juce::Colour textDim       { 0xff8a96ae };
    const juce::Colour textFaint     { 0xff5d6880 };
    const juce::Colour knobTrack     { 0xff2b3450 };

    // Sunset banner ramp, bottom (horizon) to top (dusk sky).
    const juce::Colour sunsetGlow    { 0xffffd08a };
    const juce::Colour sunsetCore    { 0xfff59a45 };
    const juce::Colour sunsetMid     { 0xffc55a6b };
    const juce::Colour duskUpper     { 0xff4a4a7d };
    const juce::Colour duskTop       { 0xff232a4a };
    const juce::Colour mountainNear  { 0xff141a2b };
    const juce::Colour mountainFar   { 0xff2e3354 };

    // Per-layer accents, in LayerIndex order.
    const juce::Colour layerAccents[] {
        juce::Colour (0xfff2913d),   // 1. Warm Pad       - orange
        juce::Colour (0xff5b9bd5),   // 2. Analog Strings - blue
        juce::Colour (0xff57be8e),   // 3. Granular       - green
        juce::Colour (0xffa483e0)    // 4. Sub Pad        - purple
    };
}

/** Shared typography helper; keeps every label on the same two type sizes. */
inline juce::Font labelFont (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions()
                           .withHeight (height)
                           .withStyle (bold ? "Bold" : "Regular"));
}

/**
    Custom LookAndFeel: a thin, flat rotary that matches the mockup - a dark
    track arc, an accent-coloured value arc, a subtly shaded knob cap and a
    single pointer line. The accent colour is taken from the slider's
    rotarySliderFillColourId so each layer panel can tint its own knobs.
*/
class HorizonLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    HorizonLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    juce::Font getLabelFont (juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HorizonLookAndFeel)
};

/** Rounded panel with a 1px border, used by every panel in the layout. */
void drawPanel (juce::Graphics& g, juce::Rectangle<float> bounds,
                juce::Colour fill = Palette::panel, float corner = 10.0f);

} // namespace horizon::ui

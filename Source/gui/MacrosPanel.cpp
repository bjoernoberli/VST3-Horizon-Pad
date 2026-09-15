#include "MacrosPanel.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

void MacrosPanel::setUpKnob (Knob& knob, const juce::String& caption, const char* paramId,
                             juce::Colour accent, HorizonPadAudioProcessor& processor)
{
    knob.caption = caption;
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f,
                                     juce::MathConstants<float>::pi * 2.78f,
                                     true);
    addAndMakeVisible (knob.slider);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.getAPVTS(), paramId, knob.slider);

    knob.slider.onValueChange = [this] { repaint(); };
}

MacrosPanel::MacrosPanel (HorizonPadAudioProcessor& processor)
{
    setUpKnob (knobs[0], "ATTACK", ParamID::attackMacro, Palette::macroAccents[0], processor);
    setUpKnob (knobs[1], "FILTER", ParamID::filterMacro, Palette::macroAccents[1], processor);
    setUpKnob (knobs[2], "WIDTH",  ParamID::widthMacro,  Palette::macroAccents[2], processor);
    setUpKnob (knobs[3], "REVERB", ParamID::reverbMacro, Palette::macroAccents[3], processor);
}

MacrosPanel::~MacrosPanel() = default;

void MacrosPanel::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (20); // "MACROS" caption + subtitle, painted

    const auto rowHeight = r.getHeight() / 2;
    const auto colWidth = r.getWidth() / 2;

    for (int i = 0; i < 4; ++i)
    {
        auto cell = juce::Rectangle<int> (r.getX() + (i % 2) * colWidth, r.getY() + (i / 2) * rowHeight,
                                          colWidth, rowHeight);
        cell.removeFromBottom (26); // caption + percentage, painted
        const auto size = juce::jmin (cell.getWidth(), cell.getHeight()) - 4;
        knobs[(size_t) i].slider.setBounds (cell.withSizeKeepingCentre (size, size));
    }
}

void MacrosPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();

    auto header = r.removeFromTop (20);
    g.setColour (Palette::textDim);
    g.setFont (labelFont (11.0f, true));
    g.drawText ("MACROS", header, juce::Justification::centredTop);

    const auto rowHeight = r.getHeight() / 2;
    const auto colWidth = r.getWidth() / 2;

    for (int i = 0; i < 4; ++i)
    {
        auto cell = juce::Rectangle<int> (r.getX() + (i % 2) * colWidth, r.getY() + (i / 2) * rowHeight,
                                          colWidth, rowHeight);
        auto footer = cell.removeFromBottom (26);

        auto caption = footer.removeFromTop (14);
        g.setColour (Palette::textFaint);
        g.setFont (labelFont (9.0f, true));
        g.drawText (knobs[(size_t) i].caption, caption, juce::Justification::centred);

        g.setColour (Palette::text);
        g.setFont (labelFont (11.0f));
        g.drawText (juce::String (juce::roundToInt (knobs[(size_t) i].slider.getValue() * 100.0)) + "%",
                    footer, juce::Justification::centred);
    }
}

} // namespace horizon::ui

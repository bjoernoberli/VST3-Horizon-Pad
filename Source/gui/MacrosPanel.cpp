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
    // -135deg..+135deg (a 270deg sweep with a 90deg gap centred at the
    // bottom), matching the design handoff's ringKnob() geometry exactly.
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f,
                                     true);
    addAndMakeVisible (knob.slider);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.getAPVTS(), paramId, knob.slider);

    knob.slider.onValueChange = [this] { repaint(); };
}

MacrosPanel::MacrosPanel (HorizonPadAudioProcessor& processor)
{
    setUpKnob (knobs[0], "ATTACK",  ParamID::attackMacro,  Palette::macroAccents[0], processor);
    setUpKnob (knobs[1], "RELEASE", ParamID::releaseMacro, Palette::macroAccents[1], processor);
    setUpKnob (knobs[2], "FILTER",  ParamID::filterMacro,  Palette::macroAccents[2], processor);
    setUpKnob (knobs[3], "REVERB",  ParamID::reverbMacro,  Palette::macroAccents[3], processor);
}

MacrosPanel::~MacrosPanel() = default;

void MacrosPanel::resized()
{
    const auto slots = computeColumnSlots (getLocalBounds());
    auto r = slots.control;

    const auto rowHeight = r.getHeight() / 2;
    const auto colWidth = r.getWidth() / 2;

    for (int i = 0; i < 4; ++i)
    {
        auto cell = juce::Rectangle<int> (r.getX() + (i % 2) * colWidth, r.getY() + (i / 2) * rowHeight,
                                          colWidth, rowHeight);
        cell.removeFromTop (16);   // macro label, painted
        cell.removeFromBottom (16); // percentage, painted
        const auto size = juce::jmin (cell.getWidth(), cell.getHeight(), 48);
        knobs[(size_t) i].slider.setBounds (cell.withSizeKeepingCentre (size, size));
    }
}

void MacrosPanel::paint (juce::Graphics& g)
{
    drawColumnCard (g, getLocalBounds().toFloat());

    const auto slots = computeColumnSlots (getLocalBounds());

    g.setColour (Palette::iconGlyph);
    g.setFont (juce::Font (juce::FontOptions().withHeight (TypeScale::icon)));
    g.drawText (juce::String::fromUTF8 ("\xe2\x97\x8d"), slots.icon, juce::Justification::centred); // "◍"

    g.setColour (Palette::textKnobLabel);
    g.setFont (labelFont (TypeScale::label, true));
    g.drawText ("MACROS", slots.label, juce::Justification::centred);

    g.setColour (Palette::textDim);
    g.setFont (labelFont (TypeScale::caption).italicised());
    g.drawFittedText ("shape the air", slots.caption, juce::Justification::centred, 2);

    auto r = slots.control;
    const auto rowHeight = r.getHeight() / 2;
    const auto colWidth = r.getWidth() / 2;

    for (int i = 0; i < 4; ++i)
    {
        auto cell = juce::Rectangle<int> (r.getX() + (i % 2) * colWidth, r.getY() + (i / 2) * rowHeight,
                                          colWidth, rowHeight);
        auto labelArea = cell.removeFromTop (16);
        auto valueArea = cell.removeFromBottom (16);

        g.setColour (Palette::macroLabel);
        g.setFont (labelFont (11.0f, true));
        g.drawText (knobs[(size_t) i].caption, labelArea, juce::Justification::centred);

        g.setColour (Palette::textDim);
        g.setFont (labelFont (12.0f));
        g.drawText (juce::String (juce::roundToInt (knobs[(size_t) i].slider.getValue() * 100.0)) + "%",
                    valueArea, juce::Justification::centred);
    }
}

} // namespace horizon::ui

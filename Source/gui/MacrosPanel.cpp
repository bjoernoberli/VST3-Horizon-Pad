#include "MacrosPanel.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

void MacrosPanel::setUpKnob (Knob& knob, const juce::String& caption, const juce::String& tooltip,
                             const char* paramId, juce::Colour accent, HorizonPadAudioProcessor& processor)
{
    knob.caption = caption;
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);
    // -135deg..+135deg (a 270deg sweep with a 90deg gap centred at the
    // bottom), matching the design handoff's ringKnob() geometry exactly.
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f,
                                     true);
    knob.slider.setTooltip (tooltip);
    addAndMakeVisible (knob.slider);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.getAPVTS(), paramId, knob.slider);

    knob.slider.onValueChange = [this] { repaint(); };
}

MacrosPanel::MacrosPanel (HorizonPadAudioProcessor& processor)
{
    setUpKnob (knobs[0], "ATTACK",
              "How quickly notes fade in - left is fast and percussive, right is slow and gradual "
              "(scales each layer's own designed attack time).",
              ParamID::attackMacro, Palette::macroAccents[0], processor);
    setUpKnob (knobs[1], "RELEASE",
              "How long notes take to fade out after you release them - left is short, right is long "
              "(scales each layer's own designed release time).",
              ParamID::releaseMacro, Palette::macroAccents[1], processor);
    setUpKnob (knobs[2], "FILTER",
              "Overall brightness - darker to the left, brighter to the right "
              "(scales each layer's own filter cutoff curve).",
              ParamID::filterMacro, Palette::macroAccents[2], processor);
    setUpKnob (knobs[3], "REVERB",
              "How much reverb is mixed in - dry at 0%, a full wet tail at 100%.",
              ParamID::reverbMacro, Palette::macroAccents[3], processor);
    setUpKnob (knobs[4], "WIDTH",
              "Stereo width of the whole pad - 0% is mono. The airy pads open first (Expanse, then "
              "Clearing and Bloom), the foundation (Root) last and least. Mono-safe; the bass stays centred.",
              ParamID::widthMacro, Palette::macroAccents[4], processor);
    setUpKnob (knobs[5], "DETUNE",
              "How far apart each pad's stacked oscillators are tuned - 50% is the designed sound, "
              "left is tighter and cleaner, right is wider and lusher. Eases off in the bass so low notes stay steady.",
              ParamID::detuneMacro, Palette::macroAccents[5], processor);
}

MacrosPanel::~MacrosPanel() = default;

void MacrosPanel::resized()
{
    const auto slots = computeColumnSlots (getLocalBounds());
    const auto m = computeMacroGridLayout (slots.control.getUnion (slots.value));

    for (size_t row = 0; row < m.rows.size(); ++row)
    {
        auto control = m.rows[row].control;
        const auto colWidth = control.getWidth() / 2;
        knobs[row * 2].slider.setBounds (control.removeFromLeft (colWidth).withSizeKeepingCentre (m.knobSize, m.knobSize));
        knobs[row * 2 + 1].slider.setBounds (control.withSizeKeepingCentre (m.knobSize, m.knobSize));
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

    const auto m = computeMacroGridLayout (slots.control.getUnion (slots.value));

    g.setColour (Palette::dividerColor);
    for (auto y : m.dividerYs)
        g.fillRect (juce::Rectangle<int> (m.rows[0].label.getX(), y, m.rows[0].label.getWidth(), 1));

    const auto drawPair = [&] (juce::Rectangle<int> labelArea, juce::Rectangle<int> valueArea,
                               float labelSize, float valueSize, int i0, int i1)
    {
        const auto labelColWidth = labelArea.getWidth() / 2;
        const auto valueColWidth = valueArea.getWidth() / 2;

        const std::array<juce::Rectangle<int>, 2> labelCols {
            labelArea.removeFromLeft (labelColWidth), labelArea
        };
        const std::array<juce::Rectangle<int>, 2> valueCols {
            valueArea.removeFromLeft (valueColWidth), valueArea
        };
        const std::array<int, 2> indices { i0, i1 };

        for (int col = 0; col < 2; ++col)
        {
            const auto& knob = knobs[(size_t) indices[(size_t) col]];

            g.setColour (Palette::macroLabel);
            g.setFont (labelFont (labelSize, true));
            g.drawText (knob.caption, labelCols[(size_t) col], juce::Justification::centred);

            g.setColour (Palette::textDim);
            g.setFont (labelFont (valueSize));
            g.drawText (juce::String (juce::roundToInt (knob.slider.getValue() * 100.0)) + "%",
                       valueCols[(size_t) col], juce::Justification::centred);
        }
    };

    for (size_t row = 0; row < m.rows.size(); ++row)
        drawPair (m.rows[row].label, m.rows[row].value, 10.0f, 11.0f, (int) row * 2, (int) row * 2 + 1);
}

} // namespace horizon::ui

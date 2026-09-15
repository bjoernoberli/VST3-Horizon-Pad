#include "PadKnob.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

PadKnob::PadKnob (HorizonPadAudioProcessor& processorToUse, int layerIndex,
                  juce::String captionToUse, juce::String subtitleToUse, const char* paramId)
    : layer (layerIndex),
      accent (Palette::layerAccents[juce::jlimit (0, kNumLayers - 1, layerIndex)]),
      caption (std::move (captionToUse)),
      subtitle (std::move (subtitleToUse))
{
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);

    // -135deg..+135deg (a 270deg sweep with a 90deg gap centred at the
    // bottom), matching the design handoff's ringKnob() geometry exactly.
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f,
                                true);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorToUse.getAPVTS(), paramId, slider);

    slider.onValueChange = [this] { repaint(); };
}

PadKnob::~PadKnob() = default;

void PadKnob::resized()
{
    const auto slots = computeColumnSlots (getLocalBounds());
    const auto size = juce::jmin (slots.control.getWidth(), slots.control.getHeight(), 70);
    slider.setBounds (slots.control.withSizeKeepingCentre (size, size));
}

void PadKnob::paint (juce::Graphics& g)
{
    drawColumnCard (g, getLocalBounds().toFloat());

    const auto slots = computeColumnSlots (getLocalBounds());

    // --- Status dot: lit (with a glow) once the pad is audible, matching the
    // design's dotStyle threshold (vol > 0.04).
    {
        const auto lit = slider.getValue() > 0.04;
        const auto dotSize = 14.0f;
        auto dot = juce::Rectangle<float> (dotSize, dotSize).withCentre (slots.icon.toFloat().getCentre());

        if (lit)
        {
            g.setColour (accent.withAlpha (0.35f));
            g.fillEllipse (dot.expanded (5.0f));
        }

        g.setColour (lit ? accent : Palette::dotUnlit);
        g.fillEllipse (dot);
    }

    g.setColour (Palette::textKnobLabel);
    g.setFont (labelFont (11.5f, true));
    g.drawText (caption, slots.label, juce::Justification::centred);

    g.setColour (Palette::textDim);
    g.setFont (labelFont (10.5f).italicised());
    g.drawFittedText (subtitle, slots.caption, juce::Justification::centred, 2);

    g.setColour (Palette::textValue);
    g.setFont (labelFont (12.0f, true));
    g.drawText (juce::String (juce::roundToInt (slider.getValue() * 100.0)) + "%",
               slots.value, juce::Justification::centred);
}

} // namespace horizon::ui

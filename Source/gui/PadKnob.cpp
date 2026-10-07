#include "PadKnob.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

namespace
{
    void setUpRingKnob (juce::Slider& slider, juce::Colour accent)
    {
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, Palette::knobTrack);

        // -135deg..+135deg (a 270deg sweep with a 90deg gap centred at the
        // bottom), matching the design handoff's ringKnob() geometry exactly.
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f,
                                    true);
    }
}

PadKnob::PadKnob (HorizonPadAudioProcessor& processorToUse, int layerIndex,
                  juce::String captionToUse, juce::String subtitleToUse,
                  const char* volumeParamId)
    : accent (Palette::layerAccents[juce::jlimit (0, kNumLayers - 1, layerIndex)]),
      caption (std::move (captionToUse)),
      subtitle (std::move (subtitleToUse))
{
    setUpRingKnob (volumeSlider, accent);
    volumeSlider.setTooltip (caption + " volume - how loud this layer sits in the mix.");
    addAndMakeVisible (volumeSlider);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorToUse.getAPVTS(), volumeParamId, volumeSlider);
    volumeSlider.onValueChange = [this] { repaint(); };
}

PadKnob::~PadKnob() = default;

void PadKnob::resized()
{
    const auto slots = computeColumnSlots (getLocalBounds());

    // VOL's value sits right under its own knob instead of in the shared
    // row-aligned value slot, so the group is centred in both slots together.
    const auto c = computePadControlLayout (slots.control.getUnion (slots.value));
    volumeSlider.setBounds (c.control.withSizeKeepingCentre (78, 78));
}

void PadKnob::paint (juce::Graphics& g)
{
    drawColumnCard (g, getLocalBounds().toFloat());

    const auto slots = computeColumnSlots (getLocalBounds());
    const auto c = computePadControlLayout (slots.control.getUnion (slots.value));

    // --- Status dot: lit (with a glow) once the pad is audible, matching the
    // design's dotStyle threshold (vol > 0.04).
    {
        const auto lit = volumeSlider.getValue() > 0.04;
        const auto dotSize = 17.0f;
        auto dot = juce::Rectangle<float> (dotSize, dotSize).withCentre (slots.icon.toFloat().getCentre());

        if (lit)
        {
            g.setColour (accent.withAlpha (0.35f));
            g.fillEllipse (dot.expanded (6.0f));
        }

        g.setColour (lit ? accent : Palette::dotUnlit);
        g.fillEllipse (dot);
    }

    g.setColour (Palette::textKnobLabel);
    g.setFont (labelFont (TypeScale::label, true));
    g.drawText (caption, slots.label, juce::Justification::centred);

    g.setColour (Palette::textDim);
    g.setFont (labelFont (TypeScale::caption).italicised());
    g.drawFittedText (subtitle, slots.caption, juce::Justification::centred, 2);

    // --- VOL: label, then (drawn via the slider itself) its knob, then its
    // own value directly beneath it.
    g.setColour (Palette::macroLabel);
    g.setFont (labelFont (11.0f, true));
    g.drawText ("VOL", c.label, juce::Justification::centred);

    g.setColour (Palette::textValue);
    g.setFont (labelFont (17.0f, true));
    g.drawText (juce::String (juce::roundToInt (volumeSlider.getValue() * 100.0)) + "%",
               c.value, juce::Justification::centred);
}

} // namespace horizon::ui

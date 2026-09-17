#include "PadKnob.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

namespace
{
    struct SubKnobLayout
    {
        juce::Rectangle<int> volLabel, volKnob, widthLabel, widthKnob, widthValue;
    };

    /** Splits a PadKnob's control area into the two stacked knobs (VOL on
        top, WIDTH below) and their small labels/readout. */
    SubKnobLayout computeSubKnobLayout (juce::Rectangle<int> control)
    {
        SubKnobLayout s;

        control.removeFromTop (8);
        s.volLabel = control.removeFromTop (11);
        control.removeFromTop (4);
        s.volKnob = control.removeFromTop (56).withSizeKeepingCentre (56, 56);

        control.removeFromTop (8);
        s.widthLabel = control.removeFromTop (11);
        control.removeFromTop (3);
        s.widthKnob = control.removeFromTop (36).withSizeKeepingCentre (36, 36);

        control.removeFromTop (4);
        s.widthValue = control.removeFromTop (12);

        return s;
    }

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
                  const char* volumeParamId, const char* widthParamId)
    : layer (layerIndex),
      accent (Palette::layerAccents[juce::jlimit (0, kNumLayers - 1, layerIndex)]),
      caption (std::move (captionToUse)),
      subtitle (std::move (subtitleToUse))
{
    setUpRingKnob (volumeSlider, accent);
    addAndMakeVisible (volumeSlider);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorToUse.getAPVTS(), volumeParamId, volumeSlider);
    volumeSlider.onValueChange = [this] { repaint(); };

    setUpRingKnob (widthSlider, Palette::widthAccent);
    addAndMakeVisible (widthSlider);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorToUse.getAPVTS(), widthParamId, widthSlider);
    widthSlider.onValueChange = [this] { repaint(); };
}

PadKnob::~PadKnob() = default;

void PadKnob::resized()
{
    const auto slots = computeColumnSlots (getLocalBounds());
    const auto sub = computeSubKnobLayout (slots.control);

    volumeSlider.setBounds (sub.volKnob);
    widthSlider.setBounds (sub.widthKnob);
}

void PadKnob::paint (juce::Graphics& g)
{
    drawColumnCard (g, getLocalBounds().toFloat());

    const auto slots = computeColumnSlots (getLocalBounds());
    const auto sub = computeSubKnobLayout (slots.control);

    // --- Status dot: lit (with a glow) once the pad is audible, matching the
    // design's dotStyle threshold (vol > 0.04).
    {
        const auto lit = volumeSlider.getValue() > 0.04;
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

    // --- Tiny VOL/WIDTH labels, disambiguating the two stacked knobs.
    g.setColour (Palette::macroLabel);
    g.setFont (labelFont (8.5f, true));
    g.drawText ("VOL", sub.volLabel, juce::Justification::centred);
    g.drawText ("WIDTH", sub.widthLabel, juce::Justification::centred);

    // --- WIDTH's own small value readout, next to its knob.
    g.setColour (Palette::textDim);
    g.setFont (labelFont (9.5f));
    g.drawText (juce::String (juce::roundToInt (widthSlider.getValue() * 100.0)) + "%",
               sub.widthValue, juce::Justification::centred);

    // --- VOL's value readout, at the card's fixed bottom slot (unchanged
    // position, so the card's overall rhythm matches every other column).
    g.setColour (Palette::textValue);
    g.setFont (labelFont (12.0f, true));
    g.drawText (juce::String (juce::roundToInt (volumeSlider.getValue() * 100.0)) + "%",
               slots.value, juce::Justification::centred);
}

} // namespace horizon::ui

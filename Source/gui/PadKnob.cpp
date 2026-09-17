#include "PadKnob.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

namespace
{
    struct SubKnobLayout
    {
        juce::Rectangle<int> volLabel, volKnob, volValue, widthLabel, widthKnob, widthValue;
        int dividerY = 0;
    };

    /** Splits a PadKnob's full control area (control slot + value slot
        combined - see PadKnob::resized()/paint() - since neither knob uses
        the shared row-aligned value slot other cards share) into VOLUME
        (label, knob, then its own value directly beneath - the primary,
        emphasised control) and, below a divider, WIDTH (label, a smaller
        knob, then its own smaller value - secondary). */
    SubKnobLayout computeSubKnobLayout (juce::Rectangle<int> area)
    {
        SubKnobLayout s;

        area.removeFromTop (4);
        s.volLabel = area.removeFromTop (14);
        area.removeFromTop (4);
        s.volKnob = area.removeFromTop (78).withSizeKeepingCentre (78, 78);
        area.removeFromTop (5);
        s.volValue = area.removeFromTop (22);

        area.removeFromTop (5);
        s.dividerY = area.getY();
        area.removeFromTop (5);

        s.widthLabel = area.removeFromTop (12);
        area.removeFromTop (4);
        s.widthKnob = area.removeFromTop (42).withSizeKeepingCentre (42, 42);
        area.removeFromTop (4);
        s.widthValue = area.removeFromTop (16);

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

    // Neither knob here uses the shared row-aligned value slot other cards
    // put their one readout in (VOL's value sits right under its own knob
    // instead, WIDTH's under its own) - reclaim that space for the two
    // knobs' extra room rather than leaving it blank.
    const auto sub = computeSubKnobLayout (slots.control.getUnion (slots.value));

    volumeSlider.setBounds (sub.volKnob);
    widthSlider.setBounds (sub.widthKnob);
}

void PadKnob::paint (juce::Graphics& g)
{
    drawColumnCard (g, getLocalBounds().toFloat());

    const auto slots = computeColumnSlots (getLocalBounds());
    const auto fullControl = slots.control.getUnion (slots.value);
    const auto sub = computeSubKnobLayout (fullControl);

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
    // own value directly beneath it - the primary, emphasised readout.
    g.setColour (Palette::macroLabel);
    g.setFont (labelFont (11.0f, true));
    g.drawText ("VOL", sub.volLabel, juce::Justification::centred);

    g.setColour (Palette::textValue);
    g.setFont (labelFont (17.0f, true));
    g.drawText (juce::String (juce::roundToInt (volumeSlider.getValue() * 100.0)) + "%",
               sub.volValue, juce::Justification::centred);

    // --- A thin divider separating the primary VOL control from the
    // secondary WIDTH control below it.
    g.setColour (Palette::dividerColor);
    g.fillRect (sub.widthLabel.withY (sub.dividerY).withHeight (1));

    // --- WIDTH: the same label/knob/value grouping, smaller throughout.
    g.setColour (Palette::macroLabel);
    g.setFont (labelFont (10.0f, true));
    g.drawText ("WIDTH", sub.widthLabel, juce::Justification::centred);

    g.setColour (Palette::textDim);
    g.setFont (labelFont (11.0f));
    g.drawText (juce::String (juce::roundToInt (widthSlider.getValue() * 100.0)) + "%",
               sub.widthValue, juce::Justification::centred);
}

} // namespace horizon::ui

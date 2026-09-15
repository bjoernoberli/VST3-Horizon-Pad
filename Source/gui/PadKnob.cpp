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
    auto r = getLocalBounds();
    r.removeFromTop (34); // status dot + caption, painted
    r.removeFromBottom (36); // subtitle + percentage, painted

    const auto size = juce::jmin (r.getWidth(), r.getHeight()) - 8;
    slider.setBounds (r.withSizeKeepingCentre (size, size));
}

void PadKnob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();

    // --- status dot + caption
    {
        auto header = r.removeFromTop (34);
        const auto dotSize = 8.0f;
        auto dot = juce::Rectangle<float> (dotSize, dotSize).withCentre ({ (float) header.getCentreX(), 10.0f });
        g.setColour (accent);
        g.fillEllipse (dot);

        auto captionArea = header.withY (16).withHeight (18);
        g.setColour (Palette::text);
        g.setFont (labelFont (13.0f, true));
        g.drawText (caption, captionArea, juce::Justification::centred);
    }

    // --- subtitle + percentage
    {
        auto footer = r.removeFromBottom (36);

        auto subtitleArea = footer.removeFromTop (16);
        g.setColour (Palette::textFaint);
        g.setFont (labelFont (10.5f));
        g.drawText (subtitle, subtitleArea, juce::Justification::centred);

        auto pctArea = footer.removeFromTop (18);
        g.setColour (Palette::text);
        g.setFont (labelFont (13.0f, true));
        g.drawText (juce::String (juce::roundToInt (slider.getValue() * 100.0)) + "%",
                    pctArea, juce::Justification::centred);
    }
}

} // namespace horizon::ui

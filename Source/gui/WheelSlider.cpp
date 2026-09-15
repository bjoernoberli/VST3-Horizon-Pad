#include "WheelSlider.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

namespace
{
    juce::String captionFor (WheelSlider::Kind k)  { return k == WheelSlider::Kind::pitch ? "PITCH" : "MOD"; }
    juce::String subtitleFor (WheelSlider::Kind k) { return k == WheelSlider::Kind::pitch ? "bend the sky" : "a soft wind"; }
}

WheelSlider::WheelSlider (HorizonPadAudioProcessor& processorToUse, Kind kindToUse)
    : processor (processorToUse), kind (kindToUse),
      caption (captionFor (kindToUse)), subtitle (subtitleFor (kindToUse))
{
    slider.setColour (juce::Slider::thumbColourId, Palette::wheelAccent);
    slider.setColour (juce::Slider::trackColourId, Palette::knobTrack);

    if (kind == Kind::pitch)
        slider.setRange (-horizon::PerformanceState::kPitchBendRangeSemitones,
                         horizon::PerformanceState::kPitchBendRangeSemitones, 0.01);
    else
        slider.setRange (0.0, 1.0, 0.001);

    slider.setDoubleClickReturnValue (true, 0.0);

    slider.onValueChange = [this]
    {
        if (updating)
            return;

        if (kind == Kind::pitch)
            processor.getPerformanceState().setPitchBendSemitones ((float) slider.getValue());
        else
            processor.getPerformanceState().setModAmount ((float) slider.getValue());

        repaint();
    };

    // PITCH is a spring-loaded wheel, like a real keyboard's: it snaps back to
    // centre (0 semitones) the moment you let go, matching the reference
    // design's onPointerUp behaviour. MOD stays wherever it's left.
    if (kind == Kind::pitch)
        slider.onDragEnd = [this] { slider.setValue (0.0, juce::sendNotification); };

    addAndMakeVisible (slider);
    refreshFromProcessor();
}

WheelSlider::~WheelSlider() = default;

void WheelSlider::refreshFromProcessor()
{
    const juce::ScopedValueSetter<bool> guard (updating, true);

    const auto value = kind == Kind::pitch ? processor.getPerformanceState().getPitchBendSemitones()
                                           : processor.getPerformanceState().getModAmount();

    if (std::abs (slider.getValue() - value) > 1.0e-4)
    {
        slider.setValue (value, juce::dontSendNotification);
        repaint();
    }
}

void WheelSlider::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (34);
    r.removeFromBottom (36);
    slider.setBounds (r.reduced (getWidth() / 2 - 12, 4));
}

void WheelSlider::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();

    {
        auto header = r.removeFromTop (34);
        auto captionArea = header.withY (16).withHeight (18);
        g.setColour (Palette::text);
        g.setFont (labelFont (13.0f, true));
        g.drawText (caption, captionArea, juce::Justification::centred);
    }

    {
        auto footer = r.removeFromBottom (36);

        auto subtitleArea = footer.removeFromTop (16);
        g.setColour (Palette::textFaint);
        g.setFont (labelFont (10.5f));
        g.drawText (subtitle, subtitleArea, juce::Justification::centred);

        juce::String valueText;

        if (kind == Kind::pitch)
            valueText = (slider.getValue() >= 0.0 ? "+" : "") + juce::String (slider.getValue(), 1) + " st";
        else
            valueText = juce::String (juce::roundToInt (slider.getValue() * 100.0)) + "%";

        g.setColour (Palette::text);
        g.setFont (labelFont (13.0f, true));
        g.drawText (valueText, footer.removeFromTop (18), juce::Justification::centred);
    }
}

} // namespace horizon::ui

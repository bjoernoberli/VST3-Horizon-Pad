#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    One pad's controls - ROOT / CLEARING / EXPANSE / BLOOM - with its small
    coloured status dot, caption, one-line poetic subtitle and live percentage
    readout, matching the mockup exactly. Two rotary knobs live here: the
    pad's own VOLUME (large, the pad's own layer accent colour) and its own
    WIDTH (smaller, the shared neutral width accent - see
    LayerBase::setWidth()), stacked in the card's control area. Beyond that,
    there is no per-layer tone block: each pad's character is otherwise fixed
    by its own DSP.
*/
class PadKnob final : public juce::Component
{
public:
    PadKnob (HorizonPadAudioProcessor& processorToUse, int layerIndex,
            juce::String caption, juce::String subtitle,
            const char* volumeParamId, const char* widthParamId);
    ~PadKnob() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const int layer;
    const juce::Colour accent;
    const juce::String caption;
    const juce::String subtitle;

    juce::Slider volumeSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    juce::Slider widthSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadKnob)
};

} // namespace horizon::ui

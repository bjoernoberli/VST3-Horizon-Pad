#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    One pad's control - ROOT / CLEARING / EXPANSE / BLOOM - with its small
    coloured status dot, caption, one-line poetic subtitle and its VOLUME
    knob (in the pad's own layer accent colour, its value directly beneath
    it). Volume is the only per-pad control: there is no per-layer tone
    block, each pad's character is fixed by its own DSP, and WIDTH - once a
    second knob on every card - is a macro acting on all four (MacrosPanel).
*/
class PadKnob final : public juce::Component
{
public:
    PadKnob (HorizonPadAudioProcessor& processorToUse, int layerIndex,
            juce::String caption, juce::String subtitle,
            const char* volumeParamId);
    ~PadKnob() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const juce::Colour accent;
    const juce::String caption;
    const juce::String subtitle;

    juce::Slider volumeSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadKnob)
};

} // namespace horizon::ui

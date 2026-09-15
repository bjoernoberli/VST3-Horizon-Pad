#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    One big rotary knob for one pad's volume - ROOT / CLEARING / EXPANSE /
    BLOOM - with its small coloured status dot, caption, one-line poetic
    subtitle and live percentage readout, matching the mockup exactly. Unlike
    an earlier iteration of this plugin's GUI, there are no per-layer tone
    knobs here: each pad's character is fixed by its own DSP, so volume is the
    only control the pad needs.
*/
class PadKnob final : public juce::Component
{
public:
    PadKnob (HorizonPadAudioProcessor& processorToUse, int layerIndex,
            juce::String caption, juce::String subtitle, const char* paramId);
    ~PadKnob() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    const int layer;
    const juce::Colour accent;
    const juce::String caption;
    const juce::String subtitle;

    juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadKnob)
};

} // namespace horizon::ui

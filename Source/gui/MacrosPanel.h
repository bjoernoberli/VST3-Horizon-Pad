#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    The MACROS block: four small knobs in a 2x2 grid - ATTACK/RELEASE on top,
    FILTER/REVERB below - all genuine host-automatable parameters, all acting
    globally across the four pads (see LayerBase and FxChain for what each one
    actually does to the DSP). Stereo width lives on each pad's own PadKnob
    card instead, not here - see PadKnob.h.
*/
class MacrosPanel final : public juce::Component
{
public:
    explicit MacrosPanel (HorizonPadAudioProcessor&);
    ~MacrosPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String caption;
    };

    void setUpKnob (Knob& knob, const juce::String& caption, const char* paramId,
                    juce::Colour accent, HorizonPadAudioProcessor& processor);

    std::array<Knob, 4> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacrosPanel)
};

} // namespace horizon::ui

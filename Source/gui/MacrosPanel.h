#pragma once

#include "HorizonLookAndFeel.h"
#include "../dsp/HorizonTypes.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    The MACROS block: six small knobs in three rows of two - ATTACK/RELEASE,
    FILTER/REVERB, WIDTH/DETUNE - all genuine host-automatable parameters,
    all acting globally across the four pads (see LayerBase and FxChain for
    what each one actually does to the DSP).
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

    void setUpKnob (Knob& knob, const juce::String& caption, const juce::String& tooltip, const char* paramId,
                    juce::Colour accent, HorizonPadAudioProcessor& processor);

    std::array<Knob, (size_t) kNumGlobalParams> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacrosPanel)
};

} // namespace horizon::ui

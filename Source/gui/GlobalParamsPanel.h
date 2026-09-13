#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    The GLOBAL PARAMETERS panel: REVERB, DELAY, FILTER and FX AMOUNT.

    All four are genuine host-automatable parameters (5-8 of the eight in the
    spec) and are attached to the APVTS. Each knob carries a small vector icon
    drawn above its caption, matching the mockup.
*/
class GlobalParamsPanel final : public juce::Component
{
public:
    explicit GlobalParamsPanel (HorizonPadAudioProcessor&);
    ~GlobalParamsPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class Icon { reverb, delay, filter, fx };

    struct Entry
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String caption;
        juce::Colour accent;
        Icon icon {};
    };

    static void drawIcon (juce::Graphics&, Icon, juce::Rectangle<float>, juce::Colour);

    HorizonPadAudioProcessor& processor;
    std::array<Entry, 4> entries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlobalParamsPanel)
};

} // namespace horizon::ui

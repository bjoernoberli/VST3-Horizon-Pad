#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    A vertical "wheel" control for PITCH or MOD - not a host-automatable
    parameter (see horizon::PerformanceState): dragging it writes straight into
    the processor's lock-free performance state, and it is also kept in sync
    with incoming MIDI (an external pitch-bend/mod-wheel controller moves this
    on screen too), via periodic refreshFromProcessor() calls from the editor's
    timer.
*/
class WheelSlider final : public juce::Component
{
public:
    enum class Kind { pitch, mod };

    WheelSlider (HorizonPadAudioProcessor& processorToUse, Kind kind);
    ~WheelSlider() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void refreshFromProcessor();

private:
    HorizonPadAudioProcessor& processor;
    const Kind kind;
    const juce::String caption, subtitle;

    juce::Slider slider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };

    bool updating = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WheelSlider)
};

} // namespace horizon::ui

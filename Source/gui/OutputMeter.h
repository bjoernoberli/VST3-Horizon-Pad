#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/** The OUTPUT column: a live level meter (read-only - output level is a
    consequence of everything else, not a control) matching the mockup's
    vertical meter/fader look. */
class OutputMeter final : public juce::Component
{
public:
    explicit OutputMeter (HorizonPadAudioProcessor&);

    void paint (juce::Graphics&) override;
    void refreshFromProcessor();

private:
    HorizonPadAudioProcessor& processor;
    float displayedLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputMeter)
};

} // namespace horizon::ui

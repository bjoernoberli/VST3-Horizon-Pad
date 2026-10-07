#pragma once

#include "HorizonLookAndFeel.h"
#include "Motion.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/** The OUTPUT column: a live level meter (read-only - output level is a
    consequence of everything else, not a control) matching the mockup's
    vertical meter/fader look, with a peak line that holds for a moment and
    then falls. Inherits SettableTooltipClient (plain Component doesn't have
    it) purely so hovering it can explain what it shows, even though it
    isn't interactive. */
class OutputMeter final : public juce::Component,
                          public juce::SettableTooltipClient
{
public:
    explicit OutputMeter (HorizonPadAudioProcessor&);

    /** One editor frame: reads the processor's level and moves the meter. */
    void advance (double now, float dt);

    void paint (juce::Graphics&) override;

private:
    juce::Rectangle<float> trackArea() const;

    HorizonPadAudioProcessor& processor;

    motion::Follower displayedLevel;   // 0..1 of the meter's dB scale
    float peakLevel = 0.0f;
    double peakHoldUntil = 0.0;
    int shownPercent = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputMeter)
};

} // namespace horizon::ui

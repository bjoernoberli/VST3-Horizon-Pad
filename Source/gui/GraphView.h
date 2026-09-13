#pragma once

#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

/**
    The PITCH and MOD line graphs from the mockup.

    These are visualisers, not editors: they draw a slowly evolving multi-sine
    curve whose amplitude follows the plugin's current output level, so the
    panel is alive while notes are sounding and settles when they are not. They
    are deliberately not bound 1:1 to automatable parameters - there is no
    pitch or mod host parameter in the eight-parameter spec.
*/
class GraphView final : public juce::Component,
                        private juce::Timer
{
public:
    enum class Style { bipolar, unipolar };

    GraphView (juce::String title, juce::String minLabel, juce::String maxLabel,
               juce::Colour accent, Style style);
    ~GraphView() override;

    void paint (juce::Graphics&) override;

    /** Called by the editor with the processor's current RMS (0..1). */
    void setActivityLevel (float level) noexcept { activity = juce::jlimit (0.0f, 1.0f, level); }

private:
    void timerCallback() override;

    const juce::String title, minLabel, maxLabel;
    const juce::Colour accent;
    const Style style;

    float phase = 0.0f;
    float activity = 0.0f;
    float smoothedActivity = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphView)
};

} // namespace horizon::ui

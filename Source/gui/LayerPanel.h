#pragma once

#include "HorizonLookAndFeel.h"
#include "../dsp/ToneState.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    One of the four layer panels.

    VOLUME is a real host-automatable parameter and is attached to the APVTS.
    TONE / ATTACK / RELEASE write into the non-automated tone block via
    HorizonPadAudioProcessor::setToneValue(), which is lock-free; they are not
    exposed to the host as automation lanes, but they are saved with the plugin
    state and are set by every factory preset.
*/
class LayerPanel final : public juce::Component
{
public:
    LayerPanel (HorizonPadAudioProcessor& processorToUse,
                int layerIndex,
                juce::String displayName);
    ~LayerPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Pulls the tone knobs back in line with the processor (after a preset change). */
    void refreshFromProcessor();

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::String caption;
    };

    void styleKnob (Knob& knob, const juce::String& caption);

    HorizonPadAudioProcessor& processor;
    const int layer;
    const juce::String name;
    const juce::Colour accent;

    Knob volumeKnob, toneKnob, attackKnob, releaseKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    juce::TextButton soloButton { "S" };

    // Guards against the refresh loop: refreshFromProcessor() -> setValue ->
    // onValueChange -> setToneValue -> (no-op, but pointless traffic).
    bool updating = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayerPanel)
};

} // namespace horizon::ui

#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    The preset row: one pill button per factory preset, a disabled-look
    "+ Save preset" placeholder (saving user presets is a deliberate follow-up,
    not wired up yet), and an A/B compare pair with a swap button.

    A/B is presentational for now: both slots show the same live parameter
    state. Making them independently store/restore a snapshot is a follow-up
    once the plugin has a place to persist more than the host's own state.
*/
class PresetBar final : public juce::Component
{
public:
    explicit PresetBar (HorizonPadAudioProcessor&);
    ~PresetBar() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Re-reads the current program from the processor. */
    void refreshFromProcessor();

private:
    HorizonPadAudioProcessor& processor;

    juce::OwnedArray<juce::TextButton> presetButtons;
    juce::TextButton saveButton { "+ Save preset" };
    juce::TextButton slotAButton { "A" };
    juce::TextButton slotBButton { "B" };
    juce::TextButton swapButton { juce::String::fromUTF8 ("\xe2\x87\x84") }; // unicode left-right arrows

    int abSlot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};

} // namespace horizon::ui

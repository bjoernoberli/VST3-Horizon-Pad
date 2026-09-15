#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    The preset row: one pill button per factory preset, a dynamic row of the
    player's own saved presets (each with a small delete button), a
    "+ Save preset" control that expands into an inline name field, and a
    real A/B compare pair - switching slots recalls that slot's own
    independently-stored snapshot of all eight parameters, and the swap (⇄)
    button copies the active slot onto the other one. All backed by the
    processor's real, persistent state - see HorizonPadAudioProcessor's
    class-level thread-safety note for how the A/B buffers and the on-disk
    user-preset library work.
*/
class PresetBar final : public juce::Component
{
public:
    explicit PresetBar (HorizonPadAudioProcessor&);
    ~PresetBar() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Re-reads the current program, user presets, and A/B state from the processor. */
    void refreshFromProcessor();

private:
    void rebuildUserPresetButtons();
    void beginSavingNewPreset();
    void commitSavingNewPreset();
    void cancelSavingNewPreset();

    HorizonPadAudioProcessor& processor;

    juce::OwnedArray<juce::TextButton> presetButtons;

    juce::OwnedArray<juce::TextButton> userPresetButtons;
    juce::OwnedArray<juce::TextButton> userPresetDeleteButtons;

    juce::TextButton saveButton { "+ Save preset" };
    juce::TextEditor saveNameEditor;

    juce::TextButton slotAButton { "A" };
    juce::TextButton slotBButton { "B" };
    juce::TextButton swapButton { juce::String::fromUTF8 ("\xe2\x87\x84") }; // unicode left-right arrows

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};

} // namespace horizon::ui

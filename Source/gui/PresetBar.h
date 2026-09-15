#pragma once

#include "HorizonLookAndFeel.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    The preset row: one pill button per factory preset, a dynamic row of the
    player's own saved presets (each a merged name+delete pill), a
    "+ Save preset" control that expands into an inline name field with
    Save/Cancel buttons, and a real A/B compare pair in its own rounded
    group - switching slots recalls that slot's own independently-stored
    snapshot of all eight parameters, and the swap (⇄) button copies the
    active slot onto the other one. All backed by the processor's real,
    persistent state - see HorizonPadAudioProcessor's class-level
    thread-safety note for how the A/B buffers and the on-disk user-preset
    library work. Visuals match the design handoff's presetsRowStyle exactly.
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
    /** One user-saved preset: a single rounded pill containing a clickable
        name (applies the preset) and a small "x" (deletes it) - matching the
        design's up.wrapStyle/up.nameStyle/up.delStyle exactly. */
    class UserPresetPill final : public juce::Component
    {
    public:
        UserPresetPill (juce::String presetName, std::function<void()> onApply, std::function<void()> onDelete);

        void paint (juce::Graphics&) override;
        void resized() override;

        void setActive (bool shouldBeActive);
        int preferredWidth() const;

        const juce::String name;

    private:
        bool active = false;
        juce::TextButton nameButton, deleteButton;
    };

    void rebuildUserPresetButtons();
    void beginSavingNewPreset();
    void commitSavingNewPreset();
    void cancelSavingNewPreset();

    HorizonPadAudioProcessor& processor;

    juce::OwnedArray<juce::TextButton> presetButtons;
    juce::OwnedArray<UserPresetPill> userPresetPills;

    juce::TextButton saveButton { "+ Save preset" };
    juce::TextEditor saveNameEditor;
    juce::TextButton saveConfirmButton { "Save" };
    juce::TextButton saveCancelButton { "Cancel" };

    juce::TextButton slotAButton { "A" };
    juce::TextButton slotBButton { "B" };
    juce::TextButton swapButton { juce::String::fromUTF8 ("\xe2\x87\x84") }; // unicode left-right arrows

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};

} // namespace horizon::ui

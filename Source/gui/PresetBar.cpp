#include "PresetBar.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

//==============================================================================
PresetBar::UserPresetPill::UserPresetPill (juce::String presetName, std::function<void()> onApply,
                                           std::function<void()> onDelete)
    : name (std::move (presetName))
{
    nameButton.setButtonText (name);
    nameButton.setClickingTogglesState (false);
    nameButton.getProperties().set ("noBorder", true);
    nameButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    nameButton.setColour (juce::TextButton::textColourOffId, Palette::text);
    nameButton.onClick = std::move (onApply);
    addAndMakeVisible (nameButton);

    deleteButton.setButtonText (juce::String::fromUTF8 ("\xc3\x97")); // "x"
    deleteButton.setClickingTogglesState (false);
    deleteButton.getProperties().set ("noBorder", true);
    deleteButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    deleteButton.setColour (juce::TextButton::textColourOffId, Palette::textFaint);
    deleteButton.setTooltip ("Delete \"" + name + "\"");
    deleteButton.onClick = std::move (onDelete);
    addAndMakeVisible (deleteButton);
}

void PresetBar::UserPresetPill::setActive (bool shouldBeActive)
{
    if (active == shouldBeActive)
        return;

    active = shouldBeActive;
    nameButton.setColour (juce::TextButton::textColourOffId, active ? Palette::gold : Palette::text);
    repaint();
}

int PresetBar::UserPresetPill::preferredWidth() const
{
    return juce::jmax (70, name.length() * 9 + 28) + 22;
}

void PresetBar::UserPresetPill::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    const auto corner = bounds.getHeight() * 0.5f;

    g.setColour (active ? Palette::pillActiveBg : Palette::pillInactiveUserBg);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (active ? Palette::gold : Palette::pillBorder);
    g.drawRoundedRectangle (bounds, corner, 1.0f);
}

void PresetBar::UserPresetPill::resized()
{
    auto r = getLocalBounds();
    deleteButton.setBounds (r.removeFromRight (22));
    nameButton.setBounds (r);
}

//==============================================================================
PresetBar::PresetBar (HorizonPadAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    const auto& presets = processor.getPresets();

    for (int i = 0; i < (int) presets.size(); ++i)
    {
        auto* b = presetButtons.add (new juce::TextButton (presets[(size_t) i].name));
        b->setClickingTogglesState (false);
        b->onClick = [this, i] { processor.setCurrentProgram (i); refreshFromProcessor(); };
        addAndMakeVisible (b);
    }

    saveButton.setClickingTogglesState (false);
    saveButton.getProperties().set ("dashedBorder", true);
    saveButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    saveButton.setColour (juce::TextButton::textColourOffId, Palette::textFaint);
    saveButton.onClick = [this] { beginSavingNewPreset(); };
    addAndMakeVisible (saveButton);

    saveNameEditor.setSelectAllWhenFocused (true);
    saveNameEditor.setColour (juce::TextEditor::backgroundColourId, Palette::saveInputBg);
    saveNameEditor.setColour (juce::TextEditor::textColourId, Palette::text);
    saveNameEditor.setColour (juce::TextEditor::outlineColourId, Palette::gold);
    saveNameEditor.setColour (juce::TextEditor::focusedOutlineColourId, Palette::gold);
    saveNameEditor.setJustification (juce::Justification::centredLeft);
    saveNameEditor.setTextToShowWhenEmpty ("Preset name", Palette::textFaint);
    saveNameEditor.onReturnKey = [this] { commitSavingNewPreset(); };
    saveNameEditor.onEscapeKey = [this] { cancelSavingNewPreset(); };
    saveNameEditor.setVisible (false);
    addChildComponent (saveNameEditor);

    saveConfirmButton.setClickingTogglesState (false);
    saveConfirmButton.getProperties().set ("borderColour", (int) Palette::saveConfirmBorder.getARGB());
    saveConfirmButton.setColour (juce::TextButton::buttonColourId, Palette::saveConfirmBg);
    saveConfirmButton.setColour (juce::TextButton::textColourOffId, Palette::text);
    saveConfirmButton.onClick = [this] { commitSavingNewPreset(); };
    saveConfirmButton.setVisible (false);
    addChildComponent (saveConfirmButton);

    saveCancelButton.setClickingTogglesState (false);
    saveCancelButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    saveCancelButton.setColour (juce::TextButton::textColourOffId, Palette::textFaint);
    saveCancelButton.onClick = [this] { cancelSavingNewPreset(); };
    saveCancelButton.setVisible (false);
    addChildComponent (saveCancelButton);

    for (auto* b : { &slotAButton, &slotBButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (0xa5);
        b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        b->setColour (juce::TextButton::textColourOffId, Palette::textFaint);
        addAndMakeVisible (b);
    }

    slotAButton.setToggleState (true, juce::dontSendNotification);
    slotAButton.onClick = [this] { processor.switchBuffer (0); refreshFromProcessor(); };
    slotBButton.onClick = [this] { processor.switchBuffer (1); refreshFromProcessor(); };

    swapButton.setClickingTogglesState (false);
    swapButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    swapButton.setColour (juce::TextButton::textColourOffId, Palette::textFaint);
    swapButton.setTooltip ("Copy the active slot onto the other one");
    swapButton.onClick = [this] { processor.copyActiveBufferToOtherBuffer(); };
    addAndMakeVisible (swapButton);

    refreshFromProcessor();
}

PresetBar::~PresetBar() = default;

void PresetBar::rebuildUserPresetButtons()
{
    userPresetPills.clear();

    const auto& userPresets = processor.getUserPresets();

    for (int i = 0; i < (int) userPresets.size(); ++i)
    {
        auto* pill = userPresetPills.add (new UserPresetPill (
            userPresets[(size_t) i].name,
            [this, i] { processor.applyUserPreset (i); refreshFromProcessor(); },
            [this, i] { processor.deleteUserPreset (i); refreshFromProcessor(); }));
        addAndMakeVisible (pill);
    }

    resized();
}

void PresetBar::beginSavingNewPreset()
{
    saveNameEditor.setText ({}, juce::dontSendNotification);
    saveButton.setVisible (false);
    saveNameEditor.setVisible (true);
    saveConfirmButton.setVisible (true);
    saveCancelButton.setVisible (true);
    resized();
    saveNameEditor.grabKeyboardFocus();
}

void PresetBar::commitSavingNewPreset()
{
    const auto name = saveNameEditor.getText().trim();

    saveNameEditor.setVisible (false);
    saveConfirmButton.setVisible (false);
    saveCancelButton.setVisible (false);
    saveButton.setVisible (true);

    if (name.isNotEmpty())
    {
        processor.saveCurrentAsUserPreset (name);
        refreshFromProcessor();
    }

    resized();
}

void PresetBar::cancelSavingNewPreset()
{
    saveNameEditor.setVisible (false);
    saveConfirmButton.setVisible (false);
    saveCancelButton.setVisible (false);
    saveButton.setVisible (true);
    resized();
}

void PresetBar::refreshFromProcessor()
{
    if ((int) userPresetPills.size() != (int) processor.getUserPresets().size())
        rebuildUserPresetButtons();

    const auto activeKind = processor.getActivePresetKind();
    const auto activeIndex = processor.getActivePresetIndex();

    for (int i = 0; i < presetButtons.size(); ++i)
        presetButtons[i]->setToggleState (activeKind == HorizonPadAudioProcessor::PresetKind::factory && i == activeIndex,
                                          juce::dontSendNotification);

    for (int i = 0; i < userPresetPills.size(); ++i)
        userPresetPills[i]->setActive (activeKind == HorizonPadAudioProcessor::PresetKind::user && i == activeIndex);

    const auto activeBuffer = processor.getActiveBufferIndex();
    slotAButton.setToggleState (activeBuffer == 0, juce::dontSendNotification);
    slotBButton.setToggleState (activeBuffer == 1, juce::dontSendNotification);

    repaint();
}

void PresetBar::resized()
{
    auto r = getLocalBounds();

    // --- A/B group, pinned to the right end of the row (margin-left:auto).
    auto abArea = r.removeFromRight (118).reduced (2);
    swapButton.setBounds (abArea.removeFromRight (32).reduced (1));
    abArea.removeFromRight (6);
    slotBButton.setBounds (abArea.removeFromRight (32).reduced (1));
    abArea.removeFromRight (6);
    slotAButton.setBounds (abArea.removeFromRight (32).reduced (1));

    r.removeFromRight (14);

    if (saveNameEditor.isVisible())
    {
        saveNameEditor.setBounds (r.removeFromLeft (150).reduced (2));
        r.removeFromLeft (8);
        saveConfirmButton.setBounds (r.removeFromLeft (70).reduced (2));
        r.removeFromLeft (8);
        saveCancelButton.setBounds (r.removeFromLeft (80).reduced (2));
    }
    else
    {
        saveButton.setBounds (r.removeFromLeft (130).reduced (2));
    }

    r.removeFromLeft (10);

    for (auto* b : presetButtons)
    {
        const auto w = juce::jmax (70, b->getButtonText().length() * 9 + 28);
        b->setBounds (r.removeFromLeft (w).reduced (2));
        r.removeFromLeft (10);
    }

    for (auto* pill : userPresetPills)
    {
        pill->setBounds (r.removeFromLeft (pill->preferredWidth()).reduced (2));
        r.removeFromLeft (10);
    }
}

void PresetBar::paint (juce::Graphics& g)
{
    // A/B group background: one rounded card behind the A/B/swap buttons,
    // matching the design's abGroupStyle exactly.
    auto abArea = getLocalBounds().removeFromRight (118).toFloat();
    g.setColour (Palette::cardBg);
    g.fillRoundedRectangle (abArea.reduced (2.0f), 10.0f);
}

} // namespace horizon::ui

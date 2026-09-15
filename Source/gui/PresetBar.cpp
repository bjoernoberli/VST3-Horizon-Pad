#include "PresetBar.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

PresetBar::PresetBar (HorizonPadAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    const auto& presets = processor.getPresets();

    for (int i = 0; i < (int) presets.size(); ++i)
    {
        auto* b = presetButtons.add (new juce::TextButton (presets[(size_t) i].name));
        b->setClickingTogglesState (false);
        b->setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
        b->setColour (juce::TextButton::buttonOnColourId, Palette::layerAccents[(size_t) (i % kNumLayers)]);
        b->onClick = [this, i] { processor.setCurrentProgram (i); refreshFromProcessor(); };
        addAndMakeVisible (b);
    }

    saveButton.setColour (juce::TextButton::buttonColourId, Palette::panel);
    saveButton.setColour (juce::TextButton::textColourOffId, Palette::textDim);
    saveButton.onClick = [this] { beginSavingNewPreset(); };
    addAndMakeVisible (saveButton);

    saveNameEditor.setSelectAllWhenFocused (true);
    saveNameEditor.setColour (juce::TextEditor::backgroundColourId, Palette::panelRaised);
    saveNameEditor.setColour (juce::TextEditor::textColourId, Palette::text);
    saveNameEditor.setColour (juce::TextEditor::outlineColourId, Palette::panelBorder);
    saveNameEditor.setColour (juce::TextEditor::focusedOutlineColourId, Palette::layerAccents[0]);
    saveNameEditor.setJustification (juce::Justification::centredLeft);
    saveNameEditor.setTextToShowWhenEmpty ("preset name...", Palette::textFaint);
    saveNameEditor.onReturnKey = [this] { commitSavingNewPreset(); };
    saveNameEditor.onEscapeKey = [this] { cancelSavingNewPreset(); };
    saveNameEditor.onFocusLost = [this] { cancelSavingNewPreset(); };
    saveNameEditor.setVisible (false);
    addChildComponent (saveNameEditor);

    for (auto* b : { &slotAButton, &slotBButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (0xa5);
        b->setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
        b->setColour (juce::TextButton::buttonOnColourId, Palette::text);
        b->setColour (juce::TextButton::textColourOnId, Palette::background);
        addAndMakeVisible (b);
    }

    slotAButton.setToggleState (true, juce::dontSendNotification);
    slotAButton.onClick = [this] { processor.switchBuffer (0); refreshFromProcessor(); };
    slotBButton.onClick = [this] { processor.switchBuffer (1); refreshFromProcessor(); };

    swapButton.setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
    swapButton.setTooltip ("Copy the active slot onto the other one");
    swapButton.onClick = [this] { processor.copyActiveBufferToOtherBuffer(); };
    addAndMakeVisible (swapButton);

    refreshFromProcessor();
}

PresetBar::~PresetBar() = default;

void PresetBar::rebuildUserPresetButtons()
{
    userPresetButtons.clear();
    userPresetDeleteButtons.clear();

    const auto& userPresets = processor.getUserPresets();

    for (int i = 0; i < (int) userPresets.size(); ++i)
    {
        auto* b = userPresetButtons.add (new juce::TextButton (userPresets[(size_t) i].name));
        b->setClickingTogglesState (false);
        b->setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
        b->setColour (juce::TextButton::buttonOnColourId, Palette::wheelAccent);
        b->onClick = [this, i] { processor.applyUserPreset (i); refreshFromProcessor(); };
        addAndMakeVisible (b);

        auto* del = userPresetDeleteButtons.add (new juce::TextButton (juce::String::fromUTF8 ("\xc3\x97"))); // "×"
        del->setColour (juce::TextButton::buttonColourId, Palette::panel);
        del->setColour (juce::TextButton::textColourOffId, Palette::textFaint);
        del->setTooltip ("Delete \"" + userPresets[(size_t) i].name + "\"");
        del->onClick = [this, i] { processor.deleteUserPreset (i); refreshFromProcessor(); };
        addAndMakeVisible (del);
    }

    resized();
}

void PresetBar::beginSavingNewPreset()
{
    saveNameEditor.setText ({}, juce::dontSendNotification);
    saveButton.setVisible (false);
    saveNameEditor.setVisible (true);
    resized();
    saveNameEditor.grabKeyboardFocus();
}

void PresetBar::commitSavingNewPreset()
{
    const auto name = saveNameEditor.getText().trim();

    saveNameEditor.setVisible (false);
    saveButton.setVisible (true);

    if (name.isNotEmpty())
    {
        processor.saveCurrentAsUserPreset (name);
        refreshFromProcessor();
    }
}

void PresetBar::cancelSavingNewPreset()
{
    if (! saveNameEditor.isVisible())
        return;

    saveNameEditor.setVisible (false);
    saveButton.setVisible (true);
    resized();
}

void PresetBar::refreshFromProcessor()
{
    if ((int) userPresetButtons.size() != (int) processor.getUserPresets().size())
        rebuildUserPresetButtons();

    const auto activeKind = processor.getActivePresetKind();
    const auto activeIndex = processor.getActivePresetIndex();

    for (int i = 0; i < presetButtons.size(); ++i)
        presetButtons[i]->setToggleState (activeKind == HorizonPadAudioProcessor::PresetKind::factory && i == activeIndex,
                                          juce::dontSendNotification);

    for (int i = 0; i < userPresetButtons.size(); ++i)
        userPresetButtons[i]->setToggleState (activeKind == HorizonPadAudioProcessor::PresetKind::user && i == activeIndex,
                                              juce::dontSendNotification);

    const auto activeBuffer = processor.getActiveBufferIndex();
    slotAButton.setToggleState (activeBuffer == 0, juce::dontSendNotification);
    slotBButton.setToggleState (activeBuffer == 1, juce::dontSendNotification);

    repaint();
}

void PresetBar::resized()
{
    auto r = getLocalBounds().reduced (4);

    auto abArea = r.removeFromRight (110);
    swapButton.setBounds (abArea.removeFromRight (26).withSizeKeepingCentre (22, 22));
    abArea.removeFromRight (4);
    slotBButton.setBounds (abArea.removeFromRight (30).reduced (2));
    slotAButton.setBounds (abArea.removeFromRight (30).reduced (2));

    r.removeFromRight (12);

    if (saveNameEditor.isVisible())
        saveNameEditor.setBounds (r.removeFromLeft (160).reduced (2));
    else
        saveButton.setBounds (r.removeFromLeft (130).reduced (2));

    r.removeFromLeft (8);

    for (auto* b : presetButtons)
    {
        const auto w = juce::jmax (70, b->getName().length() * 8 + 28);
        b->setBounds (r.removeFromLeft (w).reduced (2));
        r.removeFromLeft (8);
    }

    if (! userPresetButtons.isEmpty())
    {
        r.removeFromLeft (4);

        for (int i = 0; i < userPresetButtons.size(); ++i)
        {
            auto* b = userPresetButtons[i];
            const auto w = juce::jmax (70, b->getName().length() * 8 + 28);
            b->setBounds (r.removeFromLeft (w).reduced (2));
            userPresetDeleteButtons[i]->setBounds (r.removeFromLeft (22).reduced (2, 6));
            r.removeFromLeft (6);
        }
    }
}

void PresetBar::paint (juce::Graphics&) {}

} // namespace horizon::ui

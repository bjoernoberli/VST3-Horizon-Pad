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

    saveButton.setEnabled (false);
    saveButton.setColour (juce::TextButton::buttonColourId, Palette::panel);
    saveButton.setColour (juce::TextButton::textColourOffId, Palette::textFaint);
    addAndMakeVisible (saveButton);

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
    slotAButton.onClick = [this] { abSlot = 0; };
    slotBButton.onClick = [this] { abSlot = 1; };

    swapButton.setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
    swapButton.onClick = [this]
    {
        (abSlot == 0 ? slotBButton : slotAButton).setToggleState (true, juce::sendNotification);
    };
    addAndMakeVisible (swapButton);

    refreshFromProcessor();
}

PresetBar::~PresetBar() = default;

void PresetBar::refreshFromProcessor()
{
    const auto current = processor.getCurrentProgram();

    for (int i = 0; i < presetButtons.size(); ++i)
        presetButtons[i]->setToggleState (i == current, juce::dontSendNotification);

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

    for (auto* b : presetButtons)
    {
        const auto w = juce::jmax (70, b->getName().length() * 8 + 28);
        b->setBounds (r.removeFromLeft (w).reduced (2));
        r.removeFromLeft (8);
    }

    saveButton.setBounds (r.removeFromLeft (130).reduced (2));
}

void PresetBar::paint (juce::Graphics&) {}

} // namespace horizon::ui

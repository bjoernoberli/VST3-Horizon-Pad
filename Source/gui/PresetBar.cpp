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
    presetViewport.setViewedComponent (&presetScrollContent, false);
    presetViewport.setScrollBarsShown (false, false);
    presetViewport.setScrollOnDragEnabled (true);
    addAndMakeVisible (presetViewport);

    const auto& presets = processor.getPresets();

    for (int i = 0; i < (int) presets.size(); ++i)
    {
        auto* b = presetButtons.add (new juce::TextButton (presets[(size_t) i].name));
        b->setClickingTogglesState (false);
        b->onClick = [this, i] { processor.setCurrentProgram (i); refreshFromProcessor(); };
        presetScrollContent.addAndMakeVisible (b);
    }

    saveButton.setClickingTogglesState (false);
    saveButton.getProperties().set ("dashedBorder", true);
    saveButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    saveButton.setColour (juce::TextButton::textColourOffId, Palette::textKnobLabel);
    saveButton.onClick = [this] { beginSavingNewPreset(); };
    addAndMakeVisible (saveButton);

    saveNameEditor.setSelectAllWhenFocused (true);
    // Background/outline drawn by PresetBar::paint() instead (a rounded
    // pill, matching every other geometry here) - JUCE's own TextEditor
    // outline is a plain square-cornered rectangle.
    saveNameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    saveNameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    saveNameEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    saveNameEditor.setColour (juce::TextEditor::textColourId, Palette::text);
    saveNameEditor.setFont (juce::Font (juce::FontOptions().withHeight (14.0f)));
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
    swapButton.onClick = [this]
    {
        processor.copyActiveBufferToOtherBuffer();

        // Brief "copied" flash - the same green used for the save-confirm
        // button - since this action has no other visible effect to look at
        // (it silently overwrites the other, currently-hidden A/B slot).
        swapButton.getProperties().set ("borderColour", (int) Palette::saveConfirmBorder.getARGB());
        swapButton.setColour (juce::TextButton::buttonColourId, Palette::saveConfirmBg);
        swapButton.setColour (juce::TextButton::textColourOffId, Palette::text);
        swapButton.repaint();

        juce::Component::SafePointer<PresetBar> safeThis (this);
        juce::Timer::callAfterDelay (450, [safeThis]
        {
            if (safeThis == nullptr)
                return;

            auto& b = safeThis->swapButton;
            b.getProperties().remove ("borderColour");
            b.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            b.setColour (juce::TextButton::textColourOffId, Palette::textFaint);
            b.repaint();
        });
    };
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
        presetScrollContent.addAndMakeVisible (pill);
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

    // --- "+ Save preset" (or its inline name-entry form): a fixed slot
    // right before the A/B group, outside the scrollable presets area below
    // - so it's always reachable no matter how many presets there are,
    // instead of potentially being scrolled out of the visible row.
    if (saveNameEditor.isVisible())
    {
        saveNameEditor.setBounds (r.removeFromRight (150).reduced (2));
        r.removeFromRight (8);
        saveConfirmButton.setBounds (r.removeFromRight (70).reduced (2));
        r.removeFromRight (8);
        saveCancelButton.setBounds (r.removeFromRight (80).reduced (2));
    }
    else
    {
        saveButton.setBounds (r.removeFromRight (130).reduced (2));
    }

    r.removeFromRight (14); // gap before the fade zone (see paint())

    // --- The scrollable presets row fills whatever's left: factory presets
    // then saved-preset pills, laid out left to right inside
    // presetScrollContent at whatever total width they need - that width
    // can exceed the viewport's visible width, which is the point.
    presetViewport.setBounds (r);
    layOutScrollContent();
}

void PresetBar::layOutScrollContent()
{
    const auto rowHeight = presetViewport.getHeight();
    int x = 0;

    for (auto* b : presetButtons)
    {
        const auto w = juce::jmax (70, b->getButtonText().length() * 9 + 28);
        b->setBounds (juce::Rectangle<int> (x, 0, w, rowHeight).reduced (2));
        x += w + 10;
    }

    for (auto* pill : userPresetPills)
    {
        const auto w = pill->preferredWidth();
        pill->setBounds (juce::Rectangle<int> (x, 0, w, rowHeight).reduced (2));
        x += w + 10;
    }

    presetScrollContent.setSize (juce::jmax (presetViewport.getWidth(), x), rowHeight);
}

void PresetBar::paint (juce::Graphics& g)
{
    // A/B group background: one rounded card behind the A/B/swap buttons,
    // matching the design's abGroupStyle exactly.
    auto abArea = getLocalBounds().removeFromRight (118).toFloat();
    g.setColour (Palette::cardBg);
    g.fillRoundedRectangle (abArea.reduced (2.0f), 10.0f);

    // The name-entry field's own background/border - a full pill/capsule,
    // the same shape (corner = height/2) every button on this row uses via
    // drawButtonBackground(), not the tighter fixed-radius rounding an
    // ordinary rounded-rectangle panel gets. saveNameEditor's own colours
    // are transparent (see its setup in the constructor) so this is the
    // only thing drawing it.
    if (saveNameEditor.isVisible())
    {
        auto bounds = saveNameEditor.getBounds().toFloat();
        const auto corner = bounds.getHeight() * 0.5f;
        g.setColour (Palette::saveInputBg);
        g.fillRoundedRectangle (bounds, corner);
        g.setColour (Palette::gold);
        g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 1.0f);
    }
}

void PresetBar::paintOverChildren (juce::Graphics& g)
{
    // --- A soft fade over whichever edge of the scrollable presets has more
    // content just out of view - a visual cue that it scrolls, not a
    // functional mask (the viewport itself already clips anything scrolled
    // out of sight). Only shown once there's actually something to scroll
    // to, and only on the edge that currently has it. Drawn over the
    // children (paint() runs *before* them) so it actually shows on top of
    // whatever preset pill happens to sit at that edge, not underneath it.
    if (presetScrollContent.getWidth() <= presetViewport.getWidth())
        return;

    constexpr int fadeWidth = 36;
    const auto viewportBounds = presetViewport.getBounds();
    const auto scrollX = presetViewport.getViewPositionX();

    if (scrollX + presetViewport.getWidth() < presetScrollContent.getWidth())
    {
        auto fadeArea = viewportBounds.withTrimmedLeft (viewportBounds.getWidth() - fadeWidth).toFloat();
        juce::ColourGradient fade (Palette::background.withAlpha (0.0f), fadeArea.getX(), 0.0f,
                                   Palette::background, fadeArea.getRight(), 0.0f, false);
        g.setGradientFill (fade);
        g.fillRect (fadeArea);
    }

    if (scrollX > 0)
    {
        auto fadeArea = viewportBounds.withTrimmedRight (viewportBounds.getWidth() - fadeWidth).toFloat();
        juce::ColourGradient fade (Palette::background, fadeArea.getX(), 0.0f,
                                   Palette::background.withAlpha (0.0f), fadeArea.getRight(), 0.0f, false);
        g.setGradientFill (fade);
        g.fillRect (fadeArea);
    }
}

} // namespace horizon::ui

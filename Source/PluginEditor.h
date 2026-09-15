#pragma once

#include "PluginProcessor.h"

#include "gui/HorizonLookAndFeel.h"
#include "gui/TitleBanner.h"
#include "gui/PresetBar.h"
#include "gui/PadKnob.h"
#include "gui/WheelSlider.h"
#include "gui/MacrosPanel.h"
#include "gui/OutputMeter.h"
#include "gui/FooterBar.h"

/**
    The editor: title banner, preset row, a single knob row (PITCH, MOD, the
    four pad knobs, the macros grid, OUTPUT), and the footer - matching the
    Claude Design GUI draft this plugin was built from. There is no on-screen
    keyboard: play from a MIDI keyboard/controller, matching a real Launchkey-
    style workflow (the eight knobs mirror a Launchkey 25's eight knobs, and
    the wheels mirror its pitch/mod wheels).

    A fixed-size window (see kWindowWidth/kWindowHeight): every child paints
    its own precise layout rather than scaling a "design surface", so there is
    no benefit to resizing and real risk of blurring the fine knob artwork.
*/
class HorizonPadAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit HorizonPadAudioProcessorEditor (HorizonPadAudioProcessor&);
    ~HorizonPadAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    static constexpr int kWindowWidth = 1120;
    static constexpr int kWindowHeight = 780;

    HorizonPadAudioProcessor& processor;
    horizon::ui::HorizonLookAndFeel lookAndFeel;

    horizon::ui::TitleBanner titleBanner;
    horizon::ui::PresetBar presetBar;

    horizon::ui::WheelSlider pitchWheel, modWheel;
    horizon::ui::PadKnob rootKnob, clearingKnob, expanseKnob, bloomKnob;
    horizon::ui::MacrosPanel macrosPanel;
    horizon::ui::OutputMeter outputMeter;

    horizon::ui::FooterBar footerBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HorizonPadAudioProcessorEditor)
};

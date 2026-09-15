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
    The editor: one shared gradient panel (drawHorizonPanel - background,
    border, mountain skyline, corner glow) behind a title banner, a preset
    row, an 8-column knob grid (PITCH, MOD, the four pad knobs, the macros
    grid, OUTPUT), and a footer strip - a pixel-accurate recreation of the
    Claude Design GUI handoff ("Horizon Pad.dc.html"), read directly rather
    than approximated. There is no on-screen keyboard: play from a MIDI
    keyboard/controller, matching a real Launchkey-style workflow (the eight
    knobs mirror a Launchkey 25's eight knobs, and the wheels mirror its
    pitch/mod wheels).

    The window size and every inset below (kTopBottomPadding etc.) are taken
    straight from the handoff's own CSS (panelStyle's padding, presetsRowStyle's
    margin-top, mainGridStyle's gap, ...), scaled 1:1 - not eyeballed.

    A fixed-size window: every child paints its own precise layout rather
    than scaling a "design surface", so there is no benefit to resizing and
    real risk of blurring the fine knob artwork.
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

    // The design's panel is 1080px wide with 36px top/bottom and 40px
    // left/right padding; the window height follows from stacking its own
    // header/divider/presets/grid/footer measurements exactly.
    static constexpr int kWindowWidth = 1080;
    static constexpr int kWindowHeight = 748;
    static constexpr int kSidePadding = 40;
    static constexpr int kTopBottomPadding = 36;

    juce::Rectangle<int> dividerBounds;

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

#pragma once

#include "PluginProcessor.h"

#include "gui/HorizonLookAndFeel.h"
#include "gui/HeaderBar.h"
#include "gui/BannerView.h"
#include "gui/LayerPanel.h"
#include "gui/GraphView.h"
#include "gui/GlobalParamsPanel.h"
#include "gui/PresetBrowser.h"

/**
    The editor lays everything out inside a fixed 1120 x 780 "content" component
    and scales that component with an AffineTransform, so the window can be
    resized without every child needing its own responsive layout maths.
*/
class HorizonPadAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::ChangeListener,
                                             private juce::Timer
{
public:
    explicit HorizonPadAudioProcessorEditor (HorizonPadAudioProcessor&);
    ~HorizonPadAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refreshFromProcessor();

    static constexpr int kDesignWidth = 1120;
    static constexpr int kDesignHeight = 780;

    HorizonPadAudioProcessor& processor;
    horizon::ui::HorizonLookAndFeel lookAndFeel;

    /** Fixed-size container holding the real layout; scaled to fit the window. */
    juce::Component content;

    horizon::ui::HeaderBar headerBar;
    horizon::ui::BannerView banner;
    juce::OwnedArray<horizon::ui::LayerPanel> layerPanels;
    horizon::ui::GraphView pitchGraph, modGraph;
    horizon::ui::GlobalParamsPanel globalPanel;
    horizon::ui::PresetBrowser presetBrowser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HorizonPadAudioProcessorEditor)
};

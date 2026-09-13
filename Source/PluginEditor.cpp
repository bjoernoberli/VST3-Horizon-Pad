#include "PluginEditor.h"

using namespace horizon;
using namespace horizon::ui;

HorizonPadAudioProcessorEditor::HorizonPadAudioProcessorEditor (HorizonPadAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      pitchGraph ("PITCH", "-24", "+24", Palette::layerAccents[0], GraphView::Style::bipolar),
      modGraph   ("MOD",   "0.0", "1.0", Palette::layerAccents[2], GraphView::Style::unipolar),
      globalPanel (p),
      presetBrowser (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (content);
    content.addAndMakeVisible (headerBar);
    content.addAndMakeVisible (banner);

    const char* const layerNames[] { "WARM PAD", "ANALOG STRINGS", "GRANULAR TEXTURE", "SUB PAD" };

    for (int i = 0; i < kNumLayers; ++i)
    {
        auto* panel = layerPanels.add (new LayerPanel (processor, i, layerNames[i]));
        content.addAndMakeVisible (panel);
    }

    content.addAndMakeVisible (pitchGraph);
    content.addAndMakeVisible (modGraph);
    content.addAndMakeVisible (globalPanel);
    content.addAndMakeVisible (presetBrowser);

    headerBar.onPreviousPreset = [this] { presetBrowser.selectRelative (-1); };
    headerBar.onNextPreset     = [this] { presetBrowser.selectRelative (1); };

    processor.addChangeListener (this);
    refreshFromProcessor();

    setResizable (true, true);
    setResizeLimits (kDesignWidth * 3 / 4, kDesignHeight * 3 / 4,
                     kDesignWidth * 3 / 2, kDesignHeight * 3 / 2);

    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio ((double) kDesignWidth / (double) kDesignHeight);

    setSize (kDesignWidth, kDesignHeight);

    startTimerHz (15);
}

HorizonPadAudioProcessorEditor::~HorizonPadAudioProcessorEditor()
{
    stopTimer();
    processor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void HorizonPadAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshFromProcessor();
}

void HorizonPadAudioProcessorEditor::refreshFromProcessor()
{
    presetBrowser.refreshFromProcessor();
    headerBar.setPresetName (processor.getProgramName (processor.getCurrentProgram()));

    for (auto* panel : layerPanels)
        panel->refreshFromProcessor();
}

void HorizonPadAudioProcessorEditor::timerCallback()
{
    const auto level = processor.getOutputLevel();
    pitchGraph.setActivityLevel (level);
    modGraph.setActivityLevel (level);
}

void HorizonPadAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);
}

void HorizonPadAudioProcessorEditor::resized()
{
    // Scale the fixed design surface to whatever size the host gave us.
    content.setBounds (0, 0, kDesignWidth, kDesignHeight);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kDesignWidth,
                                                        (float) getHeight() / (float) kDesignHeight));

    auto r = juce::Rectangle<int> (0, 0, kDesignWidth, kDesignHeight);

    headerBar.setBounds (r.removeFromTop (58));

    r.reduce (18, 0);
    r.removeFromTop (14);

    banner.setBounds (r.removeFromTop (150));

    r.removeFromTop (16);

    // --- four equal-width layer panels
    {
        auto row = r.removeFromTop (248);
        const auto gap = 14;
        const auto panelWidth = (row.getWidth() - gap * (kNumLayers - 1)) / kNumLayers;

        for (int i = 0; i < kNumLayers; ++i)
        {
            auto cell = row.removeFromLeft (i == kNumLayers - 1 ? row.getWidth() : panelWidth);
            layerPanels[i]->setBounds (cell);

            if (i < kNumLayers - 1)
                row.removeFromLeft (gap);
        }
    }

    r.removeFromTop (16);

    // --- pitch / mod graphs, side by side
    {
        auto row = r.removeFromTop (128);
        const auto half = (row.getWidth() - 14) / 2;
        pitchGraph.setBounds (row.removeFromLeft (half));
        row.removeFromLeft (14);
        modGraph.setBounds (row);
    }

    r.removeFromTop (16);

    // --- global parameters (left, wider) + preset browser (right)
    {
        auto row = r.removeFromTop (juce::jmax (0, r.getHeight() - 18));
        const auto globalWidth = juce::roundToInt ((float) (row.getWidth() - 14) * 0.6f);

        globalPanel.setBounds (row.removeFromLeft (globalWidth));
        row.removeFromLeft (14);
        presetBrowser.setBounds (row);
    }
}

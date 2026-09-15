#include "PluginEditor.h"

using namespace horizon::ui;

HorizonPadAudioProcessorEditor::HorizonPadAudioProcessorEditor (HorizonPadAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse),
      processor (processorToUse),
      presetBar (processorToUse),
      pitchWheel (processorToUse, WheelSlider::Kind::pitch),
      modWheel (processorToUse, WheelSlider::Kind::mod),
      rootKnob (processorToUse, (int) horizon::LayerIndex::warmFoundation,
               "ROOT", "life grows", horizon::ParamID::rootVolume),
      clearingKnob (processorToUse, (int) horizon::LayerIndex::analogEnsemble,
                   "CLEARING", "light breaks in", horizon::ParamID::clearingVolume),
      expanseKnob (processorToUse, (int) horizon::LayerIndex::airyChoir,
                  "EXPANSE", "life opens up", horizon::ParamID::expanseVolume),
      bloomKnob (processorToUse, (int) horizon::LayerIndex::motionPad,
                "BLOOM", "life blooms", horizon::ParamID::bloomVolume),
      macrosPanel (processorToUse),
      outputMeter (processorToUse),
      keyboard (processorToUse)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (titleBanner);
    addAndMakeVisible (presetBar);
    addAndMakeVisible (pitchWheel);
    addAndMakeVisible (modWheel);
    addAndMakeVisible (rootKnob);
    addAndMakeVisible (clearingKnob);
    addAndMakeVisible (expanseKnob);
    addAndMakeVisible (bloomKnob);
    addAndMakeVisible (macrosPanel);
    addAndMakeVisible (outputMeter);
    addAndMakeVisible (keyboard);
    addAndMakeVisible (footerBar);

    setResizable (false, false);
    setSize (kWindowWidth, kWindowHeight);

    startTimerHz (15);
}

HorizonPadAudioProcessorEditor::~HorizonPadAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void HorizonPadAudioProcessorEditor::timerCallback()
{
    presetBar.refreshFromProcessor();
    pitchWheel.refreshFromProcessor();
    modWheel.refreshFromProcessor();
    outputMeter.refreshFromProcessor();
    keyboard.pollComputerKeyboard();
}

void HorizonPadAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);
}

void HorizonPadAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();

    footerBar.setBounds (r.removeFromBottom (32));
    keyboard.setBounds (r.removeFromBottom (140).reduced (24, 8));

    titleBanner.setBounds (r.removeFromTop (140));
    presetBar.setBounds (r.removeFromTop (48).reduced (20, 4));

    r.reduce (20, 8);

    // Knob row: PITCH, MOD, ROOT, CLEARING, EXPANSE, BLOOM, MACROS (wider), OUTPUT.
    struct Column { juce::Component* component; float weight; };
    const Column columns[] {
        { &pitchWheel,    1.0f },
        { &modWheel,      1.0f },
        { &rootKnob,      1.0f },
        { &clearingKnob,  1.0f },
        { &expanseKnob,   1.0f },
        { &bloomKnob,     1.0f },
        { &macrosPanel,   1.8f },
        { &outputMeter,   1.0f },
    };

    float totalWeight = 0.0f;
    for (auto& c : columns)
        totalWeight += c.weight;

    const auto unitWidth = (float) r.getWidth() / totalWeight;
    auto x = (float) r.getX();

    for (auto& c : columns)
    {
        const auto w = juce::roundToInt (c.weight * unitWidth);
        c.component->setBounds (juce::Rectangle<int> (juce::roundToInt (x), r.getY(), w, r.getHeight()).reduced (6, 0));
        x += (float) w;
    }
}

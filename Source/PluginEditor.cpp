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
      footerBar (processorToUse)
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
    footerBar.refreshFromProcessor();
}

void HorizonPadAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);
}

void HorizonPadAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();

    footerBar.setBounds (r.removeFromBottom (32));

    titleBanner.setBounds (r.removeFromTop (140));
    presetBar.setBounds (r.removeFromTop (48).reduced (20, 4));

    r.reduce (20, 8);

    // Knob row: PITCH and MOD ride the wheels; eight equal-width columns mirror
    // a Launchkey 25's knob row - four blend the pads (ROOT/CLEARING/EXPANSE/
    // BLOOM), four shape the tone (the MACROS panel: ATTACK/FILTER/WIDTH/
    // REVERB) - plus the OUTPUT meter, all the same width, per the design.
    juce::Component* columns[] {
        &pitchWheel, &modWheel, &rootKnob, &clearingKnob,
        &expanseKnob, &bloomKnob, &macrosPanel, &outputMeter,
    };

    const auto unitWidth = (float) r.getWidth() / (float) juce::numElementsInArray (columns);
    auto x = (float) r.getX();

    for (auto* c : columns)
    {
        const auto w = juce::roundToInt (unitWidth);
        c->setBounds (juce::Rectangle<int> (juce::roundToInt (x), r.getY(), w, r.getHeight()).reduced (6, 0));
        x += unitWidth;
    }
}

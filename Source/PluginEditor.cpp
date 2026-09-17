#include "PluginEditor.h"

using namespace horizon::ui;

HorizonPadAudioProcessorEditor::HorizonPadAudioProcessorEditor (HorizonPadAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse),
      processor (processorToUse),
      presetBar (processorToUse),
      pitchWheel (processorToUse, WheelSlider::Kind::pitch),
      modWheel (processorToUse, WheelSlider::Kind::mod),
      rootKnob (processorToUse, (int) horizon::LayerIndex::warmFoundation,
               "ROOT", "life grows", horizon::ParamID::rootVolume, horizon::ParamID::rootWidth),
      clearingKnob (processorToUse, (int) horizon::LayerIndex::analogEnsemble,
                   "CLEARING", "light breaks in", horizon::ParamID::clearingVolume, horizon::ParamID::clearingWidth),
      expanseKnob (processorToUse, (int) horizon::LayerIndex::airyChoir,
                  "EXPANSE", "life opens up", horizon::ParamID::expanseVolume, horizon::ParamID::expanseWidth),
      bloomKnob (processorToUse, (int) horizon::LayerIndex::motionPad,
                "BLOOM", "life blooms", horizon::ParamID::bloomVolume, horizon::ParamID::bloomWidth),
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
    drawHorizonPanel (g, getLocalBounds().toFloat());

    g.setColour (Palette::dividerColor);
    g.fillRect (dividerBounds);
}

void HorizonPadAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (kSidePadding, kTopBottomPadding);

    titleBanner.setBounds (r.removeFromTop (150));

    r.removeFromTop (26);
    dividerBounds = r.removeFromTop (1);

    r.removeFromTop (22);
    presetBar.setBounds (r.removeFromTop (36));

    r.removeFromTop (36);

    // The grid row's height is set by its tallest cards (PITCH/MOD/OUTPUT,
    // whose 190px vertical tracks/meters dominate), matching the design's
    // CSS grid (implicit row height = max content) rather than a fixed guess.
    constexpr int kGridRowHeight = 346;
    constexpr int kGridGap = 14;
    auto gridArea = r.removeFromTop (kGridRowHeight);

    // Eight equal-width columns: PITCH and MOD ride the wheels; four blend
    // the pads (ROOT/CLEARING/EXPANSE/BLOOM); four shape the tone (the
    // MACROS panel: ATTACK/FILTER/WIDTH/REVERB); OUTPUT is the meter -
    // mirroring a Launchkey 25's eight knobs, per the design's mainGridStyle.
    juce::Component* columns[] {
        &pitchWheel, &modWheel, &rootKnob, &clearingKnob,
        &expanseKnob, &bloomKnob, &macrosPanel, &outputMeter,
    };

    const auto numColumns = (float) juce::numElementsInArray (columns);
    const auto totalGap = kGridGap * (numColumns - 1.0f);
    const auto colWidth = ((float) gridArea.getWidth() - totalGap) / numColumns;

    auto x = (float) gridArea.getX();

    for (auto* c : columns)
    {
        c->setBounds (juce::Rectangle<int> (juce::roundToInt (x), gridArea.getY(),
                                            juce::roundToInt (colWidth), gridArea.getHeight()));
        x += colWidth + (float) kGridGap;
    }

    r.removeFromTop (28);
    footerBar.setBounds (r.removeFromTop (24));
}

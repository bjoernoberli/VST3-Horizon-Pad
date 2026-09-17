#include "OutputMeter.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

OutputMeter::OutputMeter (HorizonPadAudioProcessor& processorToUse)
    : processor (processorToUse)
{
}

void OutputMeter::refreshFromProcessor()
{
    // Standard peak/VU-meter convention: map the level in dB, not raw linear
    // amplitude, onto the fill. Ear and eye both track level logarithmically,
    // so a linear mapping crowds this DSP's whole normal RMS range into the
    // meter's bottom sliver (previously patched over with an arbitrary *3.0
    // linear boost that only "worked" for typical playing levels and still
    // pinned to 100% well before actual clipping).
    constexpr float kFloorDb = -48.0f;
    const auto levelDb = juce::Decibels::gainToDecibels (processor.getOutputLevel(), kFloorDb);
    const auto target = juce::jlimit (0.0f, 1.0f, (levelDb - kFloorDb) / -kFloorDb);
    displayedLevel += (target - displayedLevel) * 0.3f;
    repaint();
}

void OutputMeter::paint (juce::Graphics& g)
{
    drawColumnCard (g, getLocalBounds().toFloat());

    const auto slots = computeColumnSlots (getLocalBounds());

    g.setColour (Palette::iconGlyph);
    g.setFont (juce::Font (juce::FontOptions().withHeight (TypeScale::icon)));
    g.drawText (juce::String::fromUTF8 ("\xe2\x86\x92"), slots.icon, juce::Justification::centred); // "→"

    g.setColour (Palette::textKnobLabel);
    g.setFont (labelFont (TypeScale::label, true));
    g.drawText ("OUTPUT", slots.label, juce::Justification::centred);

    g.setColour (Palette::textDim);
    g.setFont (labelFont (TypeScale::caption).italicised());
    g.drawFittedText ("the valley", slots.caption, juce::Justification::centred, 2);

    // --- meter track: a slim rounded column, matching meterOuterStyle exactly.
    auto track = slots.control.withSizeKeepingCentre (26, slots.control.getHeight()).toFloat();
    g.setColour (Palette::meterBg);
    g.fillRoundedRectangle (track, 11.0f);

    auto fill = track.reduced (1.0f);
    fill.setTop (fill.getBottom() - fill.getHeight() * displayedLevel);

    if (fill.getHeight() > 0.5f)
    {
        juce::ColourGradient grad (Palette::meterFillLow, fill.getX(), fill.getBottom(),
                                   Palette::meterFillHigh, fill.getX(), fill.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 10.0f);
    }

    g.setColour (Palette::textValue);
    g.setFont (labelFont (TypeScale::value, true));
    g.drawText (juce::String (juce::roundToInt (displayedLevel * 100.0f)) + "%",
               slots.value, juce::Justification::centred);
}

} // namespace horizon::ui

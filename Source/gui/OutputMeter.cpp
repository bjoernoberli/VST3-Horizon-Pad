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
    const auto target = juce::jlimit (0.0f, 1.0f, processor.getOutputLevel() * 3.0f); // RMS reads low; scale for a lively meter
    displayedLevel += (target - displayedLevel) * 0.3f;
    repaint();
}

void OutputMeter::paint (juce::Graphics& g)
{
    drawColumnCard (g, getLocalBounds().toFloat());

    const auto slots = computeColumnSlots (getLocalBounds());

    g.setColour (Palette::iconGlyph);
    g.setFont (juce::Font (juce::FontOptions().withHeight (20.0f)));
    g.drawText (juce::String::fromUTF8 ("\xe2\x86\x92"), slots.icon, juce::Justification::centred); // "→"

    g.setColour (Palette::textKnobLabel);
    g.setFont (labelFont (11.5f, true));
    g.drawText ("OUTPUT", slots.label, juce::Justification::centred);

    g.setColour (Palette::textDim);
    g.setFont (labelFont (10.5f).italicised());
    g.drawFittedText ("the valley", slots.caption, juce::Justification::centred, 2);

    // --- meter track: a slim rounded column, matching meterOuterStyle exactly.
    auto track = slots.control.withSizeKeepingCentre (22, slots.control.getHeight()).toFloat();
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
    g.setFont (labelFont (12.0f, true));
    g.drawText (juce::String (juce::roundToInt (displayedLevel * 100.0f)) + "%",
               slots.value, juce::Justification::centred);
}

} // namespace horizon::ui

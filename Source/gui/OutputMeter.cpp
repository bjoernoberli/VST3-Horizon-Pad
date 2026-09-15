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
    auto r = getLocalBounds();

    {
        auto header = r.removeFromTop (34);
        auto captionArea = header.withY (16).withHeight (18);
        g.setColour (Palette::text);
        g.setFont (labelFont (13.0f, true));
        g.drawText ("OUTPUT", captionArea, juce::Justification::centred);
    }

    auto footer = r.removeFromBottom (36);

    // --- meter track
    auto track = r.reduced (getWidth() / 2 - 8, 4).toFloat();
    g.setColour (Palette::knobTrack);
    g.fillRoundedRectangle (track, 8.0f);

    auto fill = track;
    fill.setTop (track.getBottom() - track.getHeight() * displayedLevel);

    if (fill.getHeight() > 0.5f)
    {
        juce::ColourGradient grad (Palette::layerAccents[0], fill.getX(), fill.getBottom(),
                                   Palette::layerAccents[1], fill.getX(), fill.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 8.0f);
    }

    {
        auto subtitleArea = footer.removeFromTop (16);
        g.setColour (Palette::textFaint);
        g.setFont (labelFont (10.5f));
        g.drawText ("the valley", subtitleArea, juce::Justification::centred);

        g.setColour (Palette::text);
        g.setFont (labelFont (13.0f, true));
        g.drawText (juce::String (juce::roundToInt (displayedLevel * 100.0f)) + "%",
                    footer.removeFromTop (18), juce::Justification::centred);
    }
}

} // namespace horizon::ui

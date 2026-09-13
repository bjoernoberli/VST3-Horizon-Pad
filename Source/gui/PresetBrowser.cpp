#include "PresetBrowser.h"
#include "../PluginProcessor.h"

namespace horizon::ui
{

PresetBrowser::PresetBrowser (HorizonPadAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    for (auto* b : { &prevButton, &nextButton })
    {
        b->setColour (juce::TextButton::buttonColourId, Palette::panelRaised);
        b->setColour (juce::TextButton::textColourOffId, Palette::textDim);
        addAndMakeVisible (*b);
    }

    prevButton.onClick = [this] { selectRelative (-1); };
    nextButton.onClick = [this] { selectRelative (1); };

    refreshFromProcessor();
}

PresetBrowser::~PresetBrowser() = default;

void PresetBrowser::refreshFromProcessor()
{
    const auto program = processor.getCurrentProgram();

    if (program != selected)
    {
        selected = program;
        repaint();
    }
}

void PresetBrowser::selectRelative (int delta)
{
    const auto count = juce::jmax (1, processor.getNumPrograms());
    const auto next = ((selected + delta) % count + count) % count;

    processor.setCurrentProgram (next);
    selected = next;
    repaint();
}

void PresetBrowser::resized()
{
    auto r = getLocalBounds().reduced (16, 14);

    auto header = r.removeFromTop (20);
    nextButton.setBounds (header.removeFromRight (22).withSizeKeepingCentre (20, 18));
    header.removeFromRight (4);
    prevButton.setBounds (header.removeFromRight (22).withSizeKeepingCentre (20, 18));

    r.removeFromTop (8);

    footerBounds = r.removeFromBottom (40);
    r.removeFromBottom (8);

    listBounds = r;

    const auto count = juce::jmax (1, (int) processor.getPresets().size());
    rowHeight = juce::jlimit (18, 30, listBounds.getHeight() / count);
}

juce::Rectangle<int> PresetBrowser::getRowBounds (int index) const
{
    return listBounds.withY (listBounds.getY() + index * rowHeight).withHeight (rowHeight);
}

int PresetBrowser::rowAt (juce::Point<int> position) const
{
    if (! listBounds.contains (position))
        return -1;

    const auto index = (position.y - listBounds.getY()) / juce::jmax (1, rowHeight);

    return juce::isPositiveAndBelow (index, (int) processor.getPresets().size()) ? index : -1;
}

void PresetBrowser::mouseDown (const juce::MouseEvent& e)
{
    const auto row = rowAt (e.getPosition());

    if (row < 0 || row == selected)
        return;

    processor.setCurrentProgram (row);
    selected = row;
    repaint();
}

void PresetBrowser::mouseMove (const juce::MouseEvent& e)
{
    const auto row = rowAt (e.getPosition());

    if (row != hovered)
    {
        hovered = row;
        repaint (listBounds);
    }
}

void PresetBrowser::mouseExit (const juce::MouseEvent&)
{
    if (hovered != -1)
    {
        hovered = -1;
        repaint (listBounds);
    }
}

void PresetBrowser::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat().reduced (0.5f));

    g.setColour (Palette::textDim);
    g.setFont (labelFont (11.0f, true));
    g.drawText ("PRESETS", getLocalBounds().reduced (16, 14).removeFromTop (20),
                juce::Justification::centredLeft);

    const auto& presets = processor.getPresets();

    // ------------------------------------------------------------------ list
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        auto row = getRowBounds (i);

        if (! listBounds.intersects (row))
            continue;

        const auto accent = Palette::layerAccents[(size_t) (i % kNumLayers)];
        const auto isSelected = (i == selected);

        if (isSelected)
        {
            g.setColour (accent.withAlpha (0.16f));
            g.fillRoundedRectangle (row.toFloat().reduced (1.0f, 1.0f), 5.0f);
        }
        else if (i == hovered)
        {
            g.setColour (Palette::panelRaised.withAlpha (0.7f));
            g.fillRoundedRectangle (row.toFloat().reduced (1.0f, 1.0f), 5.0f);
        }

        auto content = row.reduced (8, 0);

        // Small preview swatch in the preset's accent colour.
        auto swatch = content.removeFromLeft (14).toFloat().withSizeKeepingCentre (10.0f, 10.0f);
        juce::ColourGradient grad (accent.brighter (0.25f), swatch.getX(), swatch.getY(),
                                   accent.darker (0.45f),   swatch.getRight(), swatch.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (swatch, 2.5f);

        content.removeFromLeft (8);

        g.setColour (isSelected ? Palette::text : Palette::textDim);
        g.setFont (labelFont (11.5f, isSelected));
        g.drawText (presets[(size_t) i].name, content, juce::Justification::centredLeft);
    }

    // ------------------------------------------- selected preset description
    if (juce::isPositiveAndBelow (selected, (int) presets.size()) && ! footerBounds.isEmpty())
    {
        auto footer = footerBounds;

        g.setColour (Palette::panelBorder);
        g.fillRect (footer.removeFromTop (1));
        footer.removeFromTop (8);

        g.setColour (Palette::textFaint);
        g.setFont (labelFont (10.5f));
        g.drawFittedText (presets[(size_t) selected].description, footer,
                          juce::Justification::topLeft, 2);
    }
}

} // namespace horizon::ui

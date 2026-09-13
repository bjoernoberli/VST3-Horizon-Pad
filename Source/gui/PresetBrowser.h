#pragma once

#include "HorizonLookAndFeel.h"
#include "../presets/Presets.h"

class HorizonPadAudioProcessor;

namespace horizon::ui
{

/**
    The PRESETS panel: the six factory programs as a clickable list, a preview
    swatch and one-line description for the selection, and browse arrows.

    Selecting here calls AudioProcessor::setCurrentProgram(), so the plugin's own
    browser and the host's program menu always agree.
*/
class PresetBrowser final : public juce::Component
{
public:
    explicit PresetBrowser (HorizonPadAudioProcessor&);
    ~PresetBrowser() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    /** Re-reads the current program from the processor. */
    void refreshFromProcessor();

    void selectRelative (int delta);

private:
    juce::Rectangle<int> getRowBounds (int index) const;
    int rowAt (juce::Point<int> position) const;

    HorizonPadAudioProcessor& processor;
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };

    juce::Rectangle<int> listBounds;
    juce::Rectangle<int> footerBounds;

    int selected = 0;
    int hovered = -1;
    int rowHeight = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};

} // namespace horizon::ui

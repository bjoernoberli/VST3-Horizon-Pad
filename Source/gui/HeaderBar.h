#pragma once

#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

/**
    Top strip: mountain logo + wordmark, section nav, version tag, VST3 badge,
    the preset name field with browse arrows, and the menu glyph.

    The nav words and the menu glyph are drawn, not interactive - they are part
    of the product's visual identity in the mockup and there is nothing behind
    them yet. The browse arrows are real.
*/
class HeaderBar final : public juce::Component
{
public:
    HeaderBar();
    ~HeaderBar() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setPresetName (const juce::String& name);

    std::function<void()> onPreviousPreset;
    std::function<void()> onNextPreset;

private:
    juce::String presetName { "Golden Horizon" };
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::Rectangle<float> presetFieldBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
};

} // namespace horizon::ui

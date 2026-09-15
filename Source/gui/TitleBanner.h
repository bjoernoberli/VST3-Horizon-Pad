#pragma once

#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

/**
    The top of the plugin: a warm radial glow behind a small mountain-sunrise
    emblem, the serif italic "Horizon Pad" wordmark, and the tagline. Drawn
    entirely in vector, matching the Claude Design GUI draft this plugin is
    built from.
*/
class TitleBanner final : public juce::Component
{
public:
    TitleBanner();

    void paint (juce::Graphics&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TitleBanner)
};

} // namespace horizon::ui

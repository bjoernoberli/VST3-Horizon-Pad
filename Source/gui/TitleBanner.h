#pragma once

#include "HorizonLookAndFeel.h"

namespace horizon::ui
{

/**
    The top of the plugin: a warm radial glow behind a large mountain-sunrise
    emblem, with the serif italic "Horizon Pad" wordmark below it. Drawn
    entirely in vector. The tagline that originally lived here now sits
    centred in the footer strip instead (FooterBar).
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

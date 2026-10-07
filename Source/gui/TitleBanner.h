#pragma once

#include "HorizonLookAndFeel.h"
#include "Motion.h"

namespace horizon::ui
{

/**
    The top of the plugin: a warm radial glow behind a large mountain-sunrise
    emblem, with the serif italic "Horizon Pad" wordmark below it and, in the
    strip between the wordmark and the divider, the title of a preset just
    recalled. Drawn entirely in vector. The tagline that originally lived
    here sits centred in the footer strip instead (FooterBar).

    Two animations live here:
      - showPresetTitle(): the recalled preset's name and one-line
        description fade in and drift up a few pixels, stay for a few
        seconds and fade out (a quick crossfade if another recall comes
        first).
      - The sunrise on open (prepareSunrise/startSunrise): the emblem's sun
        rises from behind its peaks into place while its halo comes up.
*/
class TitleBanner final : public juce::Component
{
public:
    TitleBanner();

    /** Height of the logo + wordmark block; the strip below it holds the preset title. */
    static constexpr int kHeaderHeight = 150;

    void showPresetTitle (const juce::String& name, const juce::String& description);

    /** Hides the sun behind the peaks; call before the editor is first shown. */
    void prepareSunrise() noexcept;

    /** One editor frame. */
    void advance (double now, float dt);

    void paint (juce::Graphics&) override;

private:
    juce::Rectangle<int> logoArea() const;
    juce::Rectangle<int> titleArea() const;

    // --- Preset title.
    juce::String titleName, titleDescription;      // what is drawn
    juce::String pendingName, pendingDescription;  // waiting for the old title to fade out
    bool hasPending = false;
    bool startHold = false;                        // set by showPresetTitle(), timed by the next frame
    double titleHoldUntil = 0.0;
    motion::Follower titleAlpha;

    // --- Sunrise: 0 = sun hidden behind the peaks, 1 = in place.
    motion::Spring sunHeight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TitleBanner)
};

} // namespace horizon::ui

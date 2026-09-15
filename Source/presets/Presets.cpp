#include "Presets.h"

namespace horizon
{

/*
    Factory programs.

    Reading the numbers:
      volumes = { Root (Warm Foundation), Clearing (Analog Ensemble),
                  Expanse (Airy Choir), Bloom (Motion Pad) }
      macros  = { Attack, Filter, Width, Reverb }

    Values are the starting points worked out for the VST3 handoff, matching
    each preset's name against the four pads' existing character - a first
    pass for ear-tuning, not a final mix.
*/

static const std::vector<Preset>& buildPresets()
{
    static const std::vector<Preset> presets
    {
        // ------------------------------------------------------------------
        // 1. LAGERFEUER (campfire) - warm, close, grounded. Root carries it,
        // narrow width for an intimate, near-the-fire feel, small reverb.
        // ------------------------------------------------------------------
        {
            "Lagerfeuer",
            "Warm, close and grounded - the campfire pad.",
            { 0.75f, 0.30f, 0.15f, 0.25f },
            { 0.40f, 0.30f, 0.40f, 0.20f }
        },

        // ------------------------------------------------------------------
        // 2. ALPENGLUHEN (alpenglow) - warm light spreading across the peaks.
        // Clearing steps forward, wide image, medium reverb.
        //
        // juce::String's const-char* constructor assumes UTF-8, so the raw
        // \xc3\xbc escape for u-umlaut renders correctly (it terminates
        // cleanly at the following 'h', unlike TitleBanner's tagline fix).
        // ------------------------------------------------------------------
        {
            "Alpengl\xc3\xbchen",
            "Warm light spreading wide across the peaks.",
            { 0.45f, 0.65f, 0.55f, 0.35f },
            { 0.60f, 0.65f, 0.80f, 0.45f }
        },

        // ------------------------------------------------------------------
        // 3. MORGENTAU (morning dew) - fresh, delicate, Expanse and Bloom
        // carry it. Open but soft.
        // ------------------------------------------------------------------
        {
            "Morgentau",
            "Fresh and delicate, open but soft.",
            { 0.30f, 0.40f, 0.70f, 0.55f },
            { 0.50f, 0.55f, 0.70f, 0.55f }
        },

        // ------------------------------------------------------------------
        // 4. STERNENZELT (starry sky) - vast, celestial, night. Expanse
        // dominates, full width, long airy reverb.
        // ------------------------------------------------------------------
        {
            "Sternenzelt",
            "Vast and celestial - Expanse fills the whole sky.",
            { 0.15f, 0.15f, 0.90f, 0.25f },
            { 0.75f, 0.80f, 1.00f, 0.75f }
        },

        // ------------------------------------------------------------------
        // 5. TALWIND (valley wind) - movement and breeze. Bloom carries it,
        // wide image, snappier attack.
        // ------------------------------------------------------------------
        {
            "Talwind",
            "Movement and breeze - Bloom leads the way.",
            { 0.40f, 0.40f, 0.40f, 0.75f },
            { 0.30f, 0.50f, 0.85f, 0.35f }
        }
    };

    return presets;
}

const std::vector<Preset>& getFactoryPresets()
{
    return buildPresets();
}

int getNumFactoryPresets()
{
    return (int) getFactoryPresets().size();
}

} // namespace horizon

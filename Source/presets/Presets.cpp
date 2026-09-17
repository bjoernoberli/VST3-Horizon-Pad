#include "Presets.h"

namespace horizon
{

/*
    Factory programs.

    Reading the numbers:
      volumes = { Root (Warm Foundation), Clearing (Analog Ensemble),
                  Expanse (Airy Choir), Bloom (Motion Pad) }
      widths  = { Root, Clearing, Expanse, Bloom }
      macros  = { Attack, Release, Filter, Reverb }

    Values are the starting points worked out for the VST3 handoff, matching
    each preset's name against the four pads' existing character - a first
    pass for ear-tuning, not a final mix. widths are seeded from what used to
    be each preset's one shared WIDTH macro (applied uniformly to all four
    layers here, since that's exactly what the old shared macro did to the
    mix), and release mirrors attack - both are starting points for a real
    sound-design pass, not deliberately tuned per layer yet.
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
            { 0.40f, 0.40f, 0.40f, 0.40f },
            { 0.40f, 0.40f, 0.30f, 0.20f }
        },

        // ------------------------------------------------------------------
        // 2. ALPENGLUHEN (alpenglow) - warm light spreading across the peaks.
        // Clearing steps forward, wide image, medium reverb.
        //
        // juce::String's const-char* constructor assumes plain ASCII (see
        // Presets.h) and asserts - crashing outright in this project's build
        // - on the high bytes in the raw \xc3\xbc (u-umlaut) UTF-8 escape, so
        // this one name has to be built explicitly as UTF-8 instead of via
        // the struct's normal aggregate-init string literal.
        // ------------------------------------------------------------------
        {
            juce::String (juce::CharPointer_UTF8 ("Alpengl\xc3\xbchen")),
            "Warm light spreading wide across the peaks.",
            { 0.45f, 0.65f, 0.55f, 0.35f },
            { 0.80f, 0.80f, 0.80f, 0.80f },
            { 0.60f, 0.60f, 0.65f, 0.45f }
        },

        // ------------------------------------------------------------------
        // 3. MORGENTAU (morning dew) - fresh, delicate, Expanse and Bloom
        // carry it. Open but soft.
        // ------------------------------------------------------------------
        {
            "Morgentau",
            "Fresh and delicate, open but soft.",
            { 0.30f, 0.40f, 0.70f, 0.55f },
            { 0.70f, 0.70f, 0.70f, 0.70f },
            { 0.50f, 0.50f, 0.55f, 0.55f }
        },

        // ------------------------------------------------------------------
        // 4. STERNENZELT (starry sky) - vast, celestial, night. Expanse
        // dominates, full width, long airy reverb.
        // ------------------------------------------------------------------
        {
            "Sternenzelt",
            "Vast and celestial - Expanse fills the whole sky.",
            { 0.15f, 0.15f, 0.90f, 0.25f },
            { 1.00f, 1.00f, 1.00f, 1.00f },
            { 0.75f, 0.75f, 0.80f, 0.75f }
        },

        // ------------------------------------------------------------------
        // 5. TALWIND (valley wind) - movement and breeze. Bloom carries it,
        // wide image, snappier attack.
        // ------------------------------------------------------------------
        {
            "Talwind",
            "Movement and breeze - Bloom leads the way.",
            { 0.40f, 0.40f, 0.40f, 0.75f },
            { 0.85f, 0.85f, 0.85f, 0.85f },
            { 0.30f, 0.30f, 0.50f, 0.35f }
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

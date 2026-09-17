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
        },

        // ==================================================================
        // The following 25 extend the original 5-preset handoff set into a
        // full library spanning lush/ambient, dark, bright, movement/evolving,
        // minimal/sparse and big/cinematic character - each ear-checked
        // through HorizonPadSoundTool for clipping/NaN/DC before landing here,
        // same as the original 5.
        // ==================================================================

        // ------------------------------------------------------------------
        // 6. NEBELMEER (sea of fog) - hazy, layered fog bank. Lush/ambient.
        // ------------------------------------------------------------------
        {
            "Nebelmeer",
            "A hazy, layered fog bank stretching to the horizon.",
            { 0.40f, 0.55f, 0.60f, 0.35f },
            { 0.75f, 0.75f, 0.85f, 0.65f },
            { 0.55f, 0.60f, 0.45f, 0.55f }
        },

        // ------------------------------------------------------------------
        // 7. SCHATTENTAL (shadow valley) - dark, brooding, narrow. Dark.
        // ------------------------------------------------------------------
        {
            "Schattental",
            "Dark and brooding, low light in a narrow valley.",
            { 0.70f, 0.45f, 0.20f, 0.20f },
            { 0.35f, 0.35f, 0.35f, 0.35f },
            { 0.55f, 0.65f, 0.15f, 0.30f }
        },

        // ------------------------------------------------------------------
        // 8. MITTERNACHTSBLAU (midnight blue) - deep, near-black drone. Dark.
        // ------------------------------------------------------------------
        {
            "Mitternachtsblau",
            "A deep midnight drone, barely lit.",
            { 0.80f, 0.35f, 0.30f, 0.15f },
            { 0.45f, 0.45f, 0.50f, 0.40f },
            { 0.80f, 0.85f, 0.12f, 0.40f }
        },

        // ------------------------------------------------------------------
        // 9. SONNENAUFGANG (sunrise) - bright, uplifting, slow bloom. Bright.
        // ------------------------------------------------------------------
        {
            "Sonnenaufgang",
            "A bright, uplifting sunrise, slowly blooming open.",
            { 0.45f, 0.60f, 0.65f, 0.50f },
            { 0.75f, 0.75f, 0.80f, 0.70f },
            { 0.65f, 0.55f, 0.70f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 10. BERGECHO (mountain echo) - vast cinematic space. Big/cinematic.
        // ------------------------------------------------------------------
        {
            "Bergecho",
            "A vast mountain echo - huge, cinematic space.",
            { 0.55f, 0.50f, 0.75f, 0.45f },
            { 1.00f, 1.00f, 1.00f, 0.90f },
            { 0.70f, 0.80f, 0.55f, 0.80f }
        },

        // ------------------------------------------------------------------
        // 11. KRISTALLBACH (crystal stream) - bright, shimmering. Bright.
        // ------------------------------------------------------------------
        {
            "Kristallbach",
            "A bright, shimmering stream of crystal tones.",
            { 0.25f, 0.35f, 0.80f, 0.40f },
            { 0.80f, 0.80f, 0.90f, 0.70f },
            { 0.35f, 0.45f, 0.75f, 0.60f }
        },

        // ------------------------------------------------------------------
        // 12. FEUERGLUT (ember glow) - warm, smouldering, slow. Dark/warm.
        // ------------------------------------------------------------------
        {
            "Feuerglut",
            "A warm, smouldering ember - slow and deep.",
            { 0.80f, 0.40f, 0.15f, 0.20f },
            { 0.35f, 0.40f, 0.40f, 0.35f },
            { 0.60f, 0.70f, 0.25f, 0.25f }
        },

        // ------------------------------------------------------------------
        // 13. WINDHARFE (wind harp) - airborne, restless. Movement.
        // ------------------------------------------------------------------
        {
            "Windharfe",
            "An airborne harp caught in the wind.",
            { 0.30f, 0.35f, 0.55f, 0.70f },
            { 0.80f, 0.80f, 0.85f, 0.90f },
            { 0.35f, 0.40f, 0.55f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 14. STEINERNE RUHE (stillness in stone) - minimal, sparse, slow.
        // ------------------------------------------------------------------
        {
            "Steinerne Ruhe",
            "Stillness carved in stone - minimal, slow and sparse.",
            { 0.35f, 0.20f, 0.20f, 0.15f },
            { 0.40f, 0.40f, 0.40f, 0.35f },
            { 0.85f, 0.85f, 0.30f, 0.35f }
        },

        // ------------------------------------------------------------------
        // 15. GOLDSTAUB (gold dust) - bright, airy, shimmering. Bright.
        // ------------------------------------------------------------------
        {
            "Goldstaub",
            "Golden dust catching the light - bright and airy.",
            { 0.25f, 0.30f, 0.85f, 0.45f },
            { 0.85f, 0.85f, 0.95f, 0.75f },
            { 0.45f, 0.50f, 0.80f, 0.65f }
        },

        // ------------------------------------------------------------------
        // 16. TIEFENSOG (deep pull) - dark, sub-heavy, narrow. Dark.
        // ------------------------------------------------------------------
        {
            "Tiefensog",
            "A deep pull from below - sub-heavy and dark.",
            { 0.85f, 0.35f, 0.15f, 0.20f },
            { 0.30f, 0.30f, 0.30f, 0.30f },
            { 0.50f, 0.60f, 0.15f, 0.20f }
        },

        // ------------------------------------------------------------------
        // 17. LICHTNEBEL (light fog) - soft, bright, balanced. Lush/ambient.
        // ------------------------------------------------------------------
        {
            "Lichtnebel",
            "Soft, bright fog - gentle and balanced.",
            { 0.45f, 0.45f, 0.50f, 0.40f },
            { 0.65f, 0.65f, 0.65f, 0.60f },
            { 0.45f, 0.50f, 0.55f, 0.45f }
        },

        // ------------------------------------------------------------------
        // 18. STURMFRONT (storm front) - dramatic, wide, rolling. Big/cinematic.
        // ------------------------------------------------------------------
        {
            "Sturmfront",
            "A dramatic storm front rolling in - big and wide.",
            { 0.60f, 0.60f, 0.55f, 0.65f },
            { 0.95f, 0.95f, 0.90f, 1.00f },
            { 0.30f, 0.55f, 0.60f, 0.55f }
        },

        // ------------------------------------------------------------------
        // 19. BLUTENWIND (blossom wind) - bright, airy movement. Movement.
        //
        // Non-ASCII name (u-umlaut) - see Alpenglühen above for why this has
        // to be built explicitly as UTF-8 rather than via the struct's normal
        // aggregate-init string literal.
        // ------------------------------------------------------------------
        {
            juce::String (juce::CharPointer_UTF8 ("Bl\xc3\xbctenwind")),
            "Blossoms carried on a bright, airy breeze.",
            { 0.30f, 0.35f, 0.60f, 0.65f },
            { 0.80f, 0.80f, 0.85f, 0.85f },
            { 0.30f, 0.35f, 0.65f, 0.45f }
        },

        // ------------------------------------------------------------------
        // 20. DAMMERLICHT (dusk light) - warm, settling. Lush/ambient.
        //
        // Non-ASCII name (a-umlaut) - same UTF-8 workaround as above.
        // ------------------------------------------------------------------
        {
            juce::String (juce::CharPointer_UTF8 ("D\xc3\xa4mmerlicht")),
            "Warm dusk light, gently settling.",
            { 0.60f, 0.55f, 0.35f, 0.30f },
            { 0.55f, 0.55f, 0.55f, 0.50f },
            { 0.55f, 0.60f, 0.40f, 0.40f }
        },

        // ------------------------------------------------------------------
        // 21. FROSTKLANG (frost sound) - cold, sharp, bright. Bright.
        // ------------------------------------------------------------------
        {
            "Frostklang",
            "Cold, bright and sharp - a frozen ring.",
            { 0.30f, 0.30f, 0.70f, 0.35f },
            { 0.55f, 0.55f, 0.60f, 0.50f },
            { 0.25f, 0.35f, 0.85f, 0.35f }
        },

        // ------------------------------------------------------------------
        // 22. OZEANWEITE (ocean vastness) - huge, endless reverb. Big/cinematic.
        // ------------------------------------------------------------------
        {
            "Ozeanweite",
            "The vast width of an open ocean - endless reverb.",
            { 0.35f, 0.40f, 0.70f, 0.55f },
            { 1.00f, 1.00f, 1.00f, 0.95f },
            { 0.75f, 0.85f, 0.50f, 0.85f }
        },

        // ------------------------------------------------------------------
        // 23. KUPFERGLANZ (copper shine) - warm, mid-bright, present. Bright/warm.
        // ------------------------------------------------------------------
        {
            "Kupferglanz",
            "Warm copper shine - mid-bright and present.",
            { 0.45f, 0.70f, 0.40f, 0.30f },
            { 0.60f, 0.60f, 0.60f, 0.55f },
            { 0.40f, 0.45f, 0.55f, 0.35f }
        },

        // ------------------------------------------------------------------
        // 24. NACHTREISE (night journey) - dark, slow, evolving. Movement/dark.
        // ------------------------------------------------------------------
        {
            "Nachtreise",
            "A slow journey through the night - dark and evolving.",
            { 0.55f, 0.40f, 0.30f, 0.60f },
            { 0.60f, 0.60f, 0.60f, 0.70f },
            { 0.65f, 0.75f, 0.25f, 0.45f }
        },

        // ------------------------------------------------------------------
        // 25. FEDERLEICHT (feather-light) - delicate, quiet. Minimal/sparse.
        // ------------------------------------------------------------------
        {
            "Federleicht",
            "Feather-light and delicate, barely there.",
            { 0.25f, 0.25f, 0.30f, 0.20f },
            { 0.55f, 0.55f, 0.55f, 0.50f },
            { 0.40f, 0.45f, 0.45f, 0.30f }
        },

        // ------------------------------------------------------------------
        // 26. GLETSCHERKLANG (glacier sound) - icy, bright, wide. Bright.
        // ------------------------------------------------------------------
        {
            "Gletscherklang",
            "Icy glacier tones - bright and wide.",
            { 0.30f, 0.35f, 0.75f, 0.35f },
            { 0.90f, 0.90f, 0.95f, 0.80f },
            { 0.50f, 0.55f, 0.75f, 0.60f }
        },

        // ------------------------------------------------------------------
        // 27. WALDLICHT (forest light) - dappled, organic, warm. Lush/ambient.
        // ------------------------------------------------------------------
        {
            "Waldlicht",
            "Dappled forest light - organic and warm.",
            { 0.60f, 0.45f, 0.40f, 0.35f },
            { 0.55f, 0.55f, 0.60f, 0.55f },
            { 0.45f, 0.50f, 0.45f, 0.40f }
        },

        // ------------------------------------------------------------------
        // 28. STERNENSTAUB (stardust) - shimmering, restless, bright. Movement/bright.
        // ------------------------------------------------------------------
        {
            "Sternenstaub",
            "Shimmering stardust - restless and bright.",
            { 0.30f, 0.30f, 0.70f, 0.60f },
            { 0.85f, 0.85f, 0.90f, 0.90f },
            { 0.35f, 0.40f, 0.70f, 0.60f }
        },

        // ------------------------------------------------------------------
        // 29. RUHEPULS (resting pulse) - slow, subtle motion underneath. Movement.
        // ------------------------------------------------------------------
        {
            "Ruhepuls",
            "A slow resting pulse, with subtle motion underneath.",
            { 0.50f, 0.30f, 0.25f, 0.55f },
            { 0.50f, 0.50f, 0.50f, 0.60f },
            { 0.60f, 0.65f, 0.30f, 0.35f }
        },

        // ------------------------------------------------------------------
        // 30. KLARHEIT (clarity) - clear, present, minimal reverb. Minimal/mix-friendly.
        // ------------------------------------------------------------------
        {
            "Klarheit",
            "Clear, present and simple - a mix-friendly starting point.",
            { 0.55f, 0.45f, 0.35f, 0.30f },
            { 0.50f, 0.50f, 0.50f, 0.50f },
            { 0.35f, 0.40f, 0.55f, 0.25f }
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

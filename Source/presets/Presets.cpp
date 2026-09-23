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
            { 0.508f, 0.203f, 0.102f, 0.169f },
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
            { 0.371f, 0.536f, 0.454f, 0.289f },
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
            { 0.293f, 0.390f, 0.683f, 0.537f },
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
            { 0.167f, 0.167f, 1.000f, 0.278f },
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
            { 0.308f, 0.308f, 0.308f, 0.578f },
            { 0.85f, 0.85f, 0.85f, 0.85f },
            { 0.30f, 0.30f, 0.50f, 0.35f }
        },

        // ==================================================================
        // The rest of the bank, spanning lush/ambient, dark, bright,
        // movement/evolving, minimal/sparse and big/cinematic character.
        //
        // Curated from 30 to 18 on 2026-09-23. Playbook 5.3 asks for 8-20
        // presets that span the instrument's range rather than 40 variations
        // of one patch; measured on layer balance, width, envelope, filter,
        // reverb and spectral centroid, the 30 contained twelve near-
        // duplicates. Cutting them took the closest pair in the bank from
        // 0.79 to 1.53 in that space and the mean nearest-neighbour distance
        // from 1.39 to 2.05, i.e. every remaining preset is now audibly its
        // own thing. docs/preset-curation-2026-09-23.md records which preset
        // each cut one duplicated.
        // ==================================================================

        // ------------------------------------------------------------------
        // 6. MITTERNACHTSBLAU (midnight blue) - deep, near-black drone. Dark.
        // ------------------------------------------------------------------
        {
            "Mitternachtsblau",
            "A deep midnight drone, barely lit.",
            { 0.599f, 0.262f, 0.225f, 0.112f },
            { 0.45f, 0.45f, 0.50f, 0.40f },
            { 0.80f, 0.85f, 0.12f, 0.40f }
        },

        // ------------------------------------------------------------------
        // 7. BERGECHO (mountain echo) - vast cinematic space. Big/cinematic.
        // ------------------------------------------------------------------
        {
            "Bergecho",
            "A vast mountain echo - huge, cinematic space.",
            { 0.503f, 0.457f, 0.686f, 0.411f },
            { 1.00f, 1.00f, 1.00f, 0.90f },
            { 0.70f, 0.80f, 0.55f, 0.80f }
        },

        // ------------------------------------------------------------------
        // 8. STEINERNE RUHE (stillness in stone) - minimal, sparse, slow.
        // ------------------------------------------------------------------
        {
            "Steinerne Ruhe",
            "Stillness carved in stone - minimal, slow and sparse.",
            { 0.554f, 0.317f, 0.317f, 0.237f },
            { 0.40f, 0.40f, 0.40f, 0.35f },
            { 0.85f, 0.85f, 0.30f, 0.35f }
        },

        // ------------------------------------------------------------------
        // 9. GOLDSTAUB (gold dust) - bright, airy, shimmering. Bright.
        // ------------------------------------------------------------------
        {
            "Goldstaub",
            "Golden dust catching the light - bright and airy.",
            { 0.294f, 0.353f, 1.000f, 0.529f },
            { 0.85f, 0.85f, 0.95f, 0.75f },
            { 0.45f, 0.50f, 0.80f, 0.65f }
        },

        // ------------------------------------------------------------------
        // 10. TIEFENSOG (deep pull) - dark, sub-heavy, narrow. Dark.
        // ------------------------------------------------------------------
        {
            "Tiefensog",
            "A deep pull from below - sub-heavy and dark.",
            { 0.546f, 0.225f, 0.096f, 0.129f },
            { 0.30f, 0.30f, 0.30f, 0.30f },
            { 0.50f, 0.60f, 0.15f, 0.20f }
        },

        // ------------------------------------------------------------------
        // 11. LICHTNEBEL (light fog) - soft, bright, balanced. Lush/ambient.
        // ------------------------------------------------------------------
        {
            "Lichtnebel",
            "Soft, bright fog - gentle and balanced.",
            { 0.410f, 0.410f, 0.456f, 0.365f },
            { 0.65f, 0.65f, 0.65f, 0.60f },
            { 0.45f, 0.50f, 0.55f, 0.45f }
        },

        // ------------------------------------------------------------------
        // 12. STURMFRONT (storm front) - dramatic, wide, rolling. Big/cinematic.
        // ------------------------------------------------------------------
        {
            "Sturmfront",
            "A dramatic storm front rolling in - big and wide.",
            { 0.420f, 0.420f, 0.385f, 0.455f },
            { 0.95f, 0.95f, 0.90f, 1.00f },
            { 0.30f, 0.55f, 0.60f, 0.55f }
        },

        // ------------------------------------------------------------------
        // 13. DAMMERLICHT (dusk light) - warm, settling. Lush/ambient.
        //
        // Non-ASCII name (a-umlaut) - same UTF-8 workaround as above.
        // ------------------------------------------------------------------
        {
            juce::String (juce::CharPointer_UTF8 ("D\xc3\xa4mmerlicht")),
            "Warm dusk light, gently settling.",
            { 0.449f, 0.411f, 0.262f, 0.224f },
            { 0.55f, 0.55f, 0.55f, 0.50f },
            { 0.55f, 0.60f, 0.40f, 0.40f }
        },

        // ------------------------------------------------------------------
        // 14. FROSTKLANG (frost sound) - cold, sharp, bright. Bright.
        // ------------------------------------------------------------------
        {
            "Frostklang",
            "Cold, bright and sharp - a frozen ring.",
            { 0.328f, 0.328f, 0.765f, 0.382f },
            { 0.55f, 0.55f, 0.60f, 0.50f },
            { 0.25f, 0.35f, 0.85f, 0.35f }
        },

        // ------------------------------------------------------------------
        // 15. KUPFERGLANZ (copper shine) - warm, mid-bright, present. Bright/warm.
        // ------------------------------------------------------------------
        {
            "Kupferglanz",
            "Warm copper shine - mid-bright and present.",
            { 0.334f, 0.519f, 0.297f, 0.222f },
            { 0.60f, 0.60f, 0.60f, 0.55f },
            { 0.40f, 0.45f, 0.55f, 0.35f }
        },

        // ------------------------------------------------------------------
        // 16. STERNENSTAUB (stardust) - shimmering, restless, bright. Movement/bright.
        // ------------------------------------------------------------------
        {
            "Sternenstaub",
            "Shimmering stardust - restless and bright.",
            { 0.306f, 0.306f, 0.713f, 0.611f },
            { 0.85f, 0.85f, 0.90f, 0.90f },
            { 0.35f, 0.40f, 0.70f, 0.60f }
        },

        // ------------------------------------------------------------------
        // 17. RUHEPULS (resting pulse) - slow, subtle motion underneath. Movement.
        // ------------------------------------------------------------------
        {
            "Ruhepuls",
            "A slow resting pulse, with subtle motion underneath.",
            { 0.439f, 0.263f, 0.220f, 0.483f },
            { 0.50f, 0.50f, 0.50f, 0.60f },
            { 0.60f, 0.65f, 0.30f, 0.35f }
        },

        // ------------------------------------------------------------------
        // 18. KLARHEIT (clarity) - clear, present, minimal reverb. Minimal/mix-friendly.
        // ------------------------------------------------------------------
        {
            "Klarheit",
            "Clear, present and simple - a mix-friendly starting point.",
            { 0.421f, 0.344f, 0.268f, 0.229f },
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

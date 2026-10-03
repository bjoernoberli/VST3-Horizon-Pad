#include "Presets.h"

namespace horizon
{

/*
    Factory programs.

    Reading the numbers:
      volumes = { Root (Warm Foundation), Clearing (Analog Ensemble),
                  Expanse (Airy Choir), Bloom (Motion Pad) }
      macros  = { Attack, Release, Filter, Reverb, Width, Detune }

    Values are the starting points worked out for the VST3 handoff, matching
    each preset's name against the four pads' existing character - a first
    pass for ear-tuning, not a final mix. Release mirrors attack - a starting
    point for a real sound-design pass, not deliberately tuned yet.

    WIDTH (2026-10-02) is each preset's former Expanse width. Expanse is the
    pad whose width profile is the identity, so the widest pad sits exactly
    where it did and the profiles narrow the other three (see
    LayerBase::widthProfile()). Until then every preset gave all four pads the
    same width, so the shape across the pads never changed between presets;
    now it changes with the knob. DETUNE is 0.5 everywhere, the designed
    detune, so the bank sounds as voiced until someone tunes it by ear.
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
            { 0.477f, 0.191f, 0.096f, 0.159f },
            { 0.40f, 0.40f, 0.30f, 0.20f, 0.40f, 0.50f }
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
            { 0.307f, 0.443f, 0.375f, 0.239f },
            { 0.60f, 0.60f, 0.65f, 0.45f, 0.80f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 3. MORGENTAU (morning dew) - fresh, delicate, Expanse and Bloom
        // carry it. Open but soft.
        // ------------------------------------------------------------------
        {
            "Morgentau",
            "Fresh and delicate, open but soft.",
            { 0.227f, 0.302f, 0.529f, 0.415f },
            { 0.50f, 0.50f, 0.55f, 0.55f, 0.70f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 4. STERNENZELT (starry sky) - vast, celestial, night. Expanse
        // dominates, full width, long airy reverb.
        // ------------------------------------------------------------------
        {
            "Sternenzelt",
            "Vast and celestial - Expanse fills the whole sky.",
            { 0.167f, 0.167f, 1.000f, 0.278f },
            { 0.75f, 0.75f, 0.80f, 0.75f, 1.00f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 5. TALWIND (valley wind) - movement and breeze. Bloom carries it,
        // wide image, snappier attack.
        // ------------------------------------------------------------------
        {
            "Talwind",
            "Movement and breeze - Bloom leads the way.",
            { 0.252f, 0.252f, 0.252f, 0.472f },
            { 0.30f, 0.30f, 0.50f, 0.35f, 0.85f, 0.50f }
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
            { 0.538f, 0.236f, 0.203f, 0.101f },
            { 0.80f, 0.85f, 0.12f, 0.40f, 0.50f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 7. BERGECHO (mountain echo) - vast cinematic space. Big/cinematic.
        // ------------------------------------------------------------------
        {
            "Bergecho",
            "A vast mountain echo - huge, cinematic space.",
            { 0.349f, 0.317f, 0.476f, 0.285f },
            { 0.70f, 0.80f, 0.55f, 0.80f, 1.00f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 8. STEINERNE RUHE (stillness in stone) - minimal, sparse, slow.
        // ------------------------------------------------------------------
        {
            "Steinerne Ruhe",
            "Stillness carved in stone - minimal, slow and sparse.",
            { 0.504f, 0.289f, 0.289f, 0.215f },
            { 0.85f, 0.85f, 0.30f, 0.35f, 0.40f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 9. GOLDSTAUB (gold dust) - bright, airy, shimmering. Bright.
        // ------------------------------------------------------------------
        {
            "Goldstaub",
            "Golden dust catching the light - bright and airy.",
            { 0.225f, 0.270f, 0.765f, 0.404f },
            { 0.45f, 0.50f, 0.80f, 0.65f, 0.95f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 10. TIEFENSOG (deep pull) - dark, sub-heavy, narrow. Dark.
        // ------------------------------------------------------------------
        {
            "Tiefensog",
            "A deep pull from below - sub-heavy and dark.",
            { 0.528f, 0.217f, 0.093f, 0.125f },
            { 0.50f, 0.60f, 0.15f, 0.20f, 0.30f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 11. LICHTNEBEL (light fog) - soft, bright, balanced. Lush/ambient.
        // ------------------------------------------------------------------
        {
            "Lichtnebel",
            "Soft, bright fog - gentle and balanced.",
            { 0.339f, 0.339f, 0.378f, 0.302f },
            { 0.45f, 0.50f, 0.55f, 0.45f, 0.65f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 12. STURMFRONT (storm front) - dramatic, wide, rolling. Big/cinematic.
        // ------------------------------------------------------------------
        {
            "Sturmfront",
            "A dramatic storm front rolling in - big and wide.",
            { 0.318f, 0.318f, 0.291f, 0.344f },
            { 0.30f, 0.55f, 0.60f, 0.55f, 0.90f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 13. DAMMERLICHT (dusk light) - warm, settling. Lush/ambient.
        //
        // Non-ASCII name (a-umlaut) - same UTF-8 workaround as above.
        // ------------------------------------------------------------------
        {
            juce::String (juce::CharPointer_UTF8 ("D\xc3\xa4mmerlicht")),
            "Warm dusk light, gently settling.",
            { 0.394f, 0.361f, 0.229f, 0.197f },
            { 0.55f, 0.60f, 0.40f, 0.40f, 0.55f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 14. FROSTKLANG (frost sound) - cold, sharp, bright. Bright.
        // ------------------------------------------------------------------
        {
            "Frostklang",
            "Cold, bright and sharp - a frozen ring.",
            { 0.287f, 0.287f, 0.670f, 0.334f },
            { 0.25f, 0.35f, 0.85f, 0.35f, 0.60f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 15. KUPFERGLANZ (copper shine) - warm, mid-bright, present. Bright/warm.
        // ------------------------------------------------------------------
        {
            "Kupferglanz",
            "Warm copper shine - mid-bright and present.",
            { 0.298f, 0.462f, 0.265f, 0.198f },
            { 0.40f, 0.45f, 0.55f, 0.35f, 0.60f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 16. STERNENSTAUB (stardust) - shimmering, restless, bright. Movement/bright.
        // ------------------------------------------------------------------
        {
            "Sternenstaub",
            "Shimmering stardust - restless and bright.",
            { 0.230f, 0.230f, 0.537f, 0.460f },
            { 0.35f, 0.40f, 0.70f, 0.60f, 0.90f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 17. RUHEPULS (resting pulse) - slow, subtle motion underneath. Movement.
        // ------------------------------------------------------------------
        {
            "Ruhepuls",
            "A slow resting pulse, with subtle motion underneath.",
            { 0.387f, 0.231f, 0.194f, 0.426f },
            { 0.60f, 0.65f, 0.30f, 0.35f, 0.50f, 0.50f }
        },

        // ------------------------------------------------------------------
        // 18. KLARHEIT (clarity) - clear, present, minimal reverb. Minimal/mix-friendly.
        // ------------------------------------------------------------------
        {
            "Klarheit",
            "Clear, present and simple - a mix-friendly starting point.",
            { 0.383f, 0.313f, 0.244f, 0.209f },
            { 0.35f, 0.40f, 0.55f, 0.25f, 0.50f, 0.50f }
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

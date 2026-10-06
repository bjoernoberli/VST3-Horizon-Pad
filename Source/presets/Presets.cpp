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
    now it changes with the knob. DETUNE is 0.5, the designed detune, on
    the pads; leads and in-betweens set their own (Funkenflug 65% for its
    supersaw edge, Glasperle 60% so Expanse's triangles beat).

    Three families since 2026-10-07 (see the bank-order block below):
    PADS (ATTACK/RELEASE 40-85%, the original design), IN-BETWEEN (8-12% /
    13-16%: onset ~0.2-0.45 s) and LEADS (0-7% / 5-10%: onset 15-165 ms).
    Every preset is loudness-matched to the pad bank's median, -16.9 LUFS
    (tools/measure/descriptors.py --presets).
*/

static const std::vector<Preset>& buildPresets()
{
    static const std::vector<Preset> presets
    {
        // ==================================================================
        // Bank order (2026-10-07): 17 presets in three families - PADS, IN-
        // BETWEEN (pads with lead-like speed) and LEADS. The first five are
        // the most different sounds in the bank, one or more from each
        // family, chosen by measured distance in character space (layer
        // balance, width, envelope, filter, reverb, spectral centroid); every
        // later preset is a variant of the first-five sound it is nearest to.
        // docs/preset-curation-2026-09-23.md records the method and numbers.
        //   1-5   Lagerfeuer, Funkenflug, Alpengluehen, Frostklang, Talwind
        //   6-17  their variants, grouped in that order
        // ==================================================================

        // ------------------------------------------------------------------
        // [PAD]
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
        // [LEAD]
        // 2. FUNKENFLUG (flying sparks) - bright saw lead. Clearing's unison
        // saws lead, DETUNE above the design for a supersaw edge, Root for
        // body. Medium-fast: 74 ms onset at C5 (Clearing's stack starts
        // slower than the other pads), 22 ms at C6; release ~0.2 s.
        // ------------------------------------------------------------------
        {
            "Funkenflug",
            "Bright saw lead - sparks flying, cuts through the band.",
            { 0.281f, 0.514f, 0.187f, 0.047f },
            { 0.00f, 0.05f, 0.65f, 0.20f, 0.25f, 0.65f }
        },

        // ------------------------------------------------------------------
        // [PAD]
        // 3. ALPENGLUHEN (alpenglow) - warm light spreading across the peaks.
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
        // [IN-BETWEEN]
        // 4. FROSTKLANG (frost sound) - cold, sharp, bright. Bright.
        // 2026-10-07: converted from pad to IN-BETWEEN - same layer balance,
        // ATTACK/RELEASE moved into the fast zone (onset ~0.2-0.45 s,
        // release ~0.8-1.4 s with the room), so it bridges the leads and pads.
        // ------------------------------------------------------------------
        {
            "Frostklang",
            "Cold, bright and sharp - a frozen ring that speaks at once.",
            { 0.298f, 0.298f, 0.695f, 0.346f },
            { 0.09f, 0.13f, 0.75f, 0.35f, 0.55f, 0.55f }
        },

        // ------------------------------------------------------------------
        // [IN-BETWEEN]
        // 5. TALWIND (valley wind) - movement and breeze. Bloom carries it,
        // wide image, snappier attack.
        // 2026-10-07: converted from pad to IN-BETWEEN - same layer balance,
        // ATTACK/RELEASE moved into the fast zone (onset ~0.2-0.45 s,
        // release ~0.8-1.4 s with the room), so it bridges the leads and pads.
        // ------------------------------------------------------------------
        {
            "Talwind",
            "Movement and breeze - a moving synth that answers in a breath.",
            { 0.269f, 0.269f, 0.269f, 0.503f },
            { 0.11f, 0.15f, 0.50f, 0.35f, 0.70f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [PAD]
        // 6. STEINERNE RUHE (stillness in stone) - minimal, sparse, slow.
        // ------------------------------------------------------------------
        {
            "Steinerne Ruhe",
            "Stillness carved in stone - minimal, slow and sparse.",
            { 0.504f, 0.289f, 0.289f, 0.215f },
            { 0.85f, 0.85f, 0.30f, 0.35f, 0.40f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [PAD]
        // 7. MITTERNACHTSBLAU (midnight blue) - deep, near-black drone. Dark.
        // ------------------------------------------------------------------
        {
            "Mitternachtsblau",
            "A deep midnight drone, barely lit.",
            { 0.538f, 0.236f, 0.203f, 0.101f },
            { 0.80f, 0.85f, 0.12f, 0.40f, 0.50f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [IN-BETWEEN]
        // 8. KUPFERGLANZ (copper shine) - warm, mid-bright, present. Bright/warm.
        // 2026-10-07: converted from pad to IN-BETWEEN - same layer balance,
        // ATTACK/RELEASE moved into the fast zone (onset ~0.2-0.45 s,
        // release ~0.8-1.4 s with the room), so it bridges the leads and pads.
        // ------------------------------------------------------------------
        {
            "Kupferglanz",
            "Warm copper shine - a present poly synth that speaks quickly.",
            { 0.298f, 0.462f, 0.265f, 0.198f },
            { 0.10f, 0.14f, 0.55f, 0.30f, 0.50f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [PAD]
        // 9. DAMMERLICHT (dusk light) - warm, settling. Lush/ambient.
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
        // [PAD]
        // 10. BERGECHO (mountain echo) - vast cinematic space. Big/cinematic.
        // ------------------------------------------------------------------
        {
            "Bergecho",
            "A vast mountain echo - huge, cinematic space.",
            { 0.349f, 0.317f, 0.476f, 0.285f },
            { 0.70f, 0.80f, 0.55f, 0.80f, 1.00f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [PAD]
        // 11. STERNENZELT (starry sky) - vast, celestial, night. Expanse
        // dominates, full width, long airy reverb.
        // ------------------------------------------------------------------
        {
            "Sternenzelt",
            "Vast and celestial - Expanse fills the whole sky.",
            { 0.167f, 0.167f, 1.000f, 0.278f },
            { 0.75f, 0.75f, 0.80f, 0.75f, 1.00f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [LEAD]
        // 12. GLASPERLE (glass bead) - glassy bell lead. Expanse's octave-up
        // triangles lead and decay to their 40% sustain, so every note
        // plucks bright and then sings softer. Fast: 18 ms onset at C5;
        // release ~0.6 s with the shimmer's tail. Root and Clearing under
        // it are what lets it reach the bank's loudness (Expanse alone tops
        // out 2 dB short at full volume - Sternenzelt's EX-002 cause).
        // 2026-10-07: less piercing. Expanse's octave-up tone dominated held
        // high notes (-5 dB of the whole sound at G5); Expanse down, Bloom and
        // Clearing up around it, DETUNE 60% so its triangles beat instead of
        // fusing - strongest 1.5-6 kHz partial -12.4 -> -16.6 dB, FILTER 60%.
        // ------------------------------------------------------------------
        {
            "Glasperle",
            "Glassy bell lead - plucks bright, then sings softer.",
            { 0.331f, 0.275f, 0.496f, 0.441f },
            { 0.00f, 0.10f, 0.60f, 0.30f, 0.35f, 0.60f }
        },

        // ------------------------------------------------------------------
        // [IN-BETWEEN]
        // 13. KLARHEIT (clarity) - clear, present, minimal reverb. Minimal/mix-friendly.
        // 2026-10-07: converted from pad to IN-BETWEEN - same layer balance,
        // ATTACK/RELEASE moved into the fast zone (onset ~0.2-0.45 s,
        // release ~0.8-1.4 s with the room), so it bridges the leads and pads.
        // ------------------------------------------------------------------
        {
            "Klarheit",
            "Clear, present and simple - quick enough for rhythm parts.",
            { 0.383f, 0.313f, 0.244f, 0.209f },
            { 0.08f, 0.13f, 0.55f, 0.25f, 0.45f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [LEAD]
        // 14. SILBERPFAD (silver path) - soft melodic lead to play over the pads.
        // All four layers, Bloom adding gentle motion. Medium-fast: 72-164 ms
        // onset, release ~0.4 s.
        // ------------------------------------------------------------------
        {
            "Silberpfad",
            "Soft, singing lead to play over the pads.",
            { 0.377f, 0.330f, 0.235f, 0.235f },
            { 0.07f, 0.10f, 0.55f, 0.30f, 0.30f, 0.40f }
        },

        // ------------------------------------------------------------------
        // [PAD]
        // 15. GOLDSTAUB (gold dust) - bright, airy, shimmering. Bright.
        // ------------------------------------------------------------------
        {
            "Goldstaub",
            "Golden dust catching the light - bright and airy.",
            { 0.225f, 0.270f, 0.765f, 0.404f },
            { 0.45f, 0.50f, 0.80f, 0.65f, 0.95f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [IN-BETWEEN]
        // 16. STURMFRONT (storm front) - dramatic, wide, rolling. Big/cinematic.
        // 2026-10-07: converted from pad to IN-BETWEEN - same layer balance,
        // ATTACK/RELEASE moved into the fast zone (onset ~0.2-0.45 s,
        // release ~0.8-1.4 s with the room), so it bridges the leads and pads.
        // ------------------------------------------------------------------
        {
            "Sturmfront",
            "A dramatic storm front - big, wide chords that hit sooner.",
            { 0.318f, 0.318f, 0.291f, 0.344f },
            { 0.12f, 0.16f, 0.60f, 0.50f, 0.85f, 0.50f }
        },

        // ------------------------------------------------------------------
        // [PAD]
        // 17. RUHEPULS (resting pulse) - slow, subtle motion underneath. Movement.
        // ------------------------------------------------------------------
        {
            "Ruhepuls",
            "A slow resting pulse, with subtle motion underneath.",
            { 0.387f, 0.231f, 0.194f, 0.426f },
            { 0.60f, 0.65f, 0.30f, 0.35f, 0.50f, 0.50f }
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

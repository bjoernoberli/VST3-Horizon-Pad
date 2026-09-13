#include "Presets.h"

namespace horizon
{

/*
    Factory programs.

    Reading the numbers:
      volumes  = { warm pad, analog strings, granular texture, sub pad }
      globals  = { reverb, delay, filter, fx amount }
      tone     = one LayerTone per layer, in the same layer order:
                 { waveBlend, detune, tone/brightness, attack, release, density }

    Useful reference points for the exponential mappings in ToneState.h:
      attack   0.30 = 75 ms,  0.55 = 0.44 s, 0.70 = 1.1 s, 0.88 = 4.0 s
      release  0.40 = 0.45 s, 0.60 = 1.3 s,  0.80 = 4.2 s, 0.94 = 9.3 s
      tone     0.30 = 720 Hz, 0.50 = 1.8 kHz, 0.70 = 4.5 kHz, 0.90 = 11 kHz
*/

static const std::vector<Preset>& buildPresets()
{
    static const std::vector<Preset> presets
    {
        // ------------------------------------------------------------------
        // 1. GOLDEN HORIZON  (default program)
        //
        // The reference sound for the plugin: warm pad and string ensemble
        // carry the chord with a bright-but-not-glassy top end, so it reads as
        // "sun sitting on the horizon" rather than either muddy or brittle.
        // Movement comes from moderate detune drift and a half-second attack,
        // not from heavy FX, which stay at a mix-usable middle setting.
        // ------------------------------------------------------------------
        {
            "Golden Horizon",
            "Warm, evolving pad with bright horizon and soft motion.",
            { 0.82f, 0.66f, 0.28f, 0.45f },
            { 0.45f, 0.22f, 0.68f, 0.60f },
            {{
                /* warm pad   */ { 0.42f, 0.35f, 0.66f, 0.55f, 0.66f, 0.50f },
                /* strings    */ { 0.70f, 0.38f, 0.58f, 0.62f, 0.60f, 0.50f },
                /* granular   */ { 0.55f, 0.30f, 0.62f, 0.68f, 0.70f, 0.45f },
                /* sub pad    */ { 0.20f, 0.15f, 0.30f, 0.58f, 0.68f, 0.50f }
            }}
        },

        // ------------------------------------------------------------------
        // 2. MOUNTAIN BREEZE
        //
        // Air and motion instead of weight: the sub is pulled almost out, the
        // granular layer is pushed forward with a noise-dominant source and a
        // fairly dense cloud, and every layer's brightness is opened up. Wide
        // detune gives the breeze its wobble; reverb stays light so the texture
        // reads as moving air rather than a big room.
        // ------------------------------------------------------------------
        {
            "Mountain Breeze",
            "Airy high texture with light air and open brightness.",
            { 0.58f, 0.40f, 0.72f, 0.26f },
            { 0.30f, 0.28f, 0.86f, 0.55f },
            {{
                /* warm pad   */ { 0.22f, 0.48f, 0.84f, 0.62f, 0.62f, 0.50f },
                /* strings    */ { 0.35f, 0.55f, 0.78f, 0.70f, 0.58f, 0.50f },
                /* granular   */ { 0.30f, 0.55f, 0.88f, 0.50f, 0.62f, 0.68f },
                /* sub pad    */ { 0.10f, 0.05f, 0.26f, 0.55f, 0.60f, 0.50f }
            }}
        },

        // ------------------------------------------------------------------
        // 3. DEEP FOREST
        //
        // Everything moved downwards: cutoffs sit in the low-mids (roughly
        // 700-900 Hz), attacks stretch to ~2 s and releases to ~5 s so notes
        // grow rather than start, and the sub pad is the loudest layer. Longer
        // delay and generous reverb supply depth without brightness, which is
        // what makes it feel enclosed instead of just dull.
        // ------------------------------------------------------------------
        {
            "Deep Forest",
            "Dark, slow-blooming pad with deep low end and long tails.",
            { 0.70f, 0.52f, 0.44f, 0.80f },
            { 0.62f, 0.46f, 0.34f, 0.72f },
            {{
                /* warm pad   */ { 0.55f, 0.30f, 0.40f, 0.78f, 0.82f, 0.50f },
                /* strings    */ { 0.62f, 0.30f, 0.34f, 0.80f, 0.80f, 0.50f },
                /* granular   */ { 0.62f, 0.35f, 0.42f, 0.72f, 0.80f, 0.36f },
                /* sub pad    */ { 0.30f, 0.45f, 0.22f, 0.72f, 0.84f, 0.50f }
            }}
        },

        // ------------------------------------------------------------------
        // 4. OCEAN MIST
        //
        // A wash rather than a chord: strings are turned down and softened onto
        // the triangle side of the blend, detune is pushed wide on every layer
        // so the whole thing undulates, and delay plus reverb are both high with
        // a high FX Amount to let them fully through. Long releases keep the
        // swell overlapping with itself.
        // ------------------------------------------------------------------
        {
            "Ocean Mist",
            "Lush undulating wash with long delay and deep reverb.",
            { 0.64f, 0.34f, 0.56f, 0.38f },
            { 0.78f, 0.64f, 0.60f, 0.80f },
            {{
                /* warm pad   */ { 0.30f, 0.68f, 0.62f, 0.74f, 0.84f, 0.50f },
                /* strings    */ { 0.25f, 0.72f, 0.52f, 0.84f, 0.86f, 0.50f },
                /* granular   */ { 0.40f, 0.62f, 0.66f, 0.66f, 0.84f, 0.52f },
                /* sub pad    */ { 0.15f, 0.30f, 0.28f, 0.70f, 0.86f, 0.50f }
            }}
        },

        // ------------------------------------------------------------------
        // 5. NIGHT AMBIENT
        //
        // The sparse one. Strings are nearly muted, the granular cloud is thinned
        // right down (density 0.18 = individual drops, several hundred ms apart)
        // and the sub pad two octaves down carries the piece. Very long attacks
        // and ~9 s releases mean notes overlap into each other. Reverb is high
        // for space but delay is kept low so nothing rhythmic intrudes.
        // ------------------------------------------------------------------
        {
            "Night Ambient",
            "Sparse, dark and very slow, carried by a deep sub.",
            { 0.46f, 0.12f, 0.34f, 0.86f },
            { 0.72f, 0.18f, 0.28f, 0.58f },
            {{
                /* warm pad   */ { 0.15f, 0.22f, 0.34f, 0.88f, 0.94f, 0.50f },
                /* strings    */ { 0.30f, 0.20f, 0.26f, 0.90f, 0.92f, 0.50f },
                /* granular   */ { 0.25f, 0.40f, 0.38f, 0.80f, 0.92f, 0.18f },
                /* sub pad    */ { 0.08f, 0.60f, 0.18f, 0.76f, 0.95f, 0.50f }
            }}
        },

        // ------------------------------------------------------------------
        // 6. DREAMSCAPE
        //
        // The most processed program. The granular layer is the lead voice with
        // a tonal, very dense, very bright and heavily jittered cloud (the
        // "shimmer"), the string stack goes full saw and wide, and reverb, delay
        // and FX Amount all sit near the top so the wet side dominates. The sub
        // is reined in, otherwise the whole thing turns to porridge.
        // ------------------------------------------------------------------
        {
            "Dreamscape",
            "Shimmering, heavily processed cloud with a huge wet tail.",
            { 0.54f, 0.48f, 0.88f, 0.30f },
            { 0.92f, 0.78f, 0.90f, 0.95f },
            {{
                /* warm pad   */ { 0.48f, 0.60f, 0.90f, 0.70f, 0.88f, 0.50f },
                /* strings    */ { 0.80f, 0.66f, 0.86f, 0.72f, 0.84f, 0.50f },
                /* granular   */ { 0.72f, 0.75f, 0.94f, 0.58f, 0.88f, 0.82f },
                /* sub pad    */ { 0.35f, 0.10f, 0.32f, 0.64f, 0.82f, 0.50f }
            }}
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

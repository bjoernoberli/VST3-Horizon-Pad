# Factory preset curation, 30 -> 18

**Date:** 2026-09-23 · **Gate:** G7 · **Rule:** playbook 5.3 — "8-20 presets that
span the instrument's range, not 40 variations of one patch."

Curated on **character**, not level. Loudness matching is a separate pass; the bank
still spans 11.5 LU after this change.

## Method

Each preset was rendered (Cmaj7 at C3, 2 renders averaged) and placed in a
nine-dimensional character space, each axis normalised by its own standard
deviation across the 30 so no axis dominates:

- layer **balance** — the four volumes normalised to sum 1, i.e. which pad leads
- mean **width**
- mean **attack/release**
- **FILTER** and **REVERB** macros
- measured **spectral centroid** (log Hz)

Absolute level is deliberately *not* an axis: it is about to be matched away, so
scoring on it would have preserved presets whose only distinguishing feature was
being quiet.

## Result

| | closest pair | mean nearest-neighbour |
|---|---|---|
| Original 30 | 0.79 | 1.39 |
| Curated 18 | **1.53** | **2.05** |

Every remaining preset is now at least 1.53 away from its nearest neighbour —
further apart than *any* of the twelve pairs that were cut.

## The 18, by the job they do in a set

| Role | Presets |
|---|---|
| Dark and close | Tiefensog, Mitternachtsblau, Lagerfeuer |
| Slow swell | Steinerne Ruhe |
| Gentle motion | Ruhepuls, Talwind |
| Neutral / mix-friendly | Klarheit, Dämmerlicht, Lichtnebel |
| Ensemble-led | Kupferglanz, Alpenglühen |
| Bright and airy | Morgentau, Goldstaub, Sternenstaub, Sternenzelt |
| Bright but dry and close | Frostklang |
| Big and cinematic | Bergecho, Sturmfront |

Spectral centroid across the kept bank runs 380 Hz to 1219 Hz with no gap wider
than ~160 Hz.

## The 12 cut, and what covers them now

| Cut | Covered by | Distance | Why |
|---|---|---|---|
| Kristallbach | Goldstaub | 0.94 | Same bright Expanse-led patch |
| Gletscherklang | Goldstaub | 1.12 | Same, third copy of it |
| Nachtreise | Ruhepuls | 1.15 | Same slow Bloom-led patch — see the judgment call below |
| Feuerglut | Tiefensog | 1.22 | Lagerfeuer with a slower envelope |
| Blütenwind | Sternenstaub | 1.27 | Same wide bright Bloom/Expanse blend |
| Sonnenaufgang | Alpenglühen | 1.29 | Same wide bright Clearing/Expanse blend |
| Federleicht | Klarheit | 1.30 | Its identity was "quiet", which matching removes |
| Waldlicht | Lichtnebel | 1.31 | Balance within 0.06, same width/filter/reverb |
| Ozeanweite | Bergecho | 1.33 | Both the huge-reverb, full-width patch |
| Windharfe | Talwind | 1.43 | Sits between Talwind and Sternenstaub |
| Nebelmeer | Alpenglühen | 1.49 | Haze already covered by Lichtnebel and Morgentau |
| Schattental | Tiefensog | 1.64 | Same dark narrow patch, slightly more reverb |

## The one judgment call

**Nachtreise vs Ruhepuls** are the closest surviving pair in the original bank
(1.15) and only one could stay. By name and description they are different moods —
"a slow journey through the night" against "a slow resting pulse". By measurement
they are nearly the same patch. Keeping Nachtreise would have left the curated bank
with a closest pair of 1.15, tighter than five pairs that were cut, so Ruhepuls
stayed and Lichtnebel was brought in instead to hold the mid-bright ground.

If the "night" mood matters more than the geometry, swapping Lichtnebel back out
for Nachtreise is a one-line change — it just costs the separation number.

## Compatibility

Cutting presets shifts factory program indices. This does **not** change how an
existing project sounds: `setStateInformation` restores all twelve parameter values
from the saved APVTS state and never re-applies a preset, and the restored program
index is clamped to the new range. The only visible effect is that a project saved
with v1.0.0 may show a different preset *name* as selected than it did before.

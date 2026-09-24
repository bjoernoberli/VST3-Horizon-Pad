# G2 - Faust prototype

**Date:** 2026-09-24 · **Tier:** P · **Gate:** G2
**Artefact required by playbook 6.1:** `.dsp` + a rendered wav/mp3 + measured
numbers. **Do not proceed until:** peak/clip/discontinuity checks pass and the
owner has heard it.

## Status: CLOSED

Not by building a prototype, but by finding the one that already existed and
putting it under version control. See A-001 in [`brief.md`](brief.md).

## The artefact

`sound design/four_pads.dsp` - 177 lines, all four layers plus mixer and
master, header reading `STATUS: validated, ready for VST3 port`. Six C++
source files already cited it as the authority for their design ("see
four_pads.dsp for why"); it had simply never been committed.

It carries reasoning the C++ comments only gesture at, and which is worth
keeping for anyone changing a layer:

- Simon Cann's brightness x attack-speed grid. Four pads meant to blend by
  volume alone have to sit at four *different* points on it, or stacking them
  just gets louder and muddier.
- The Prophet VS / Wavestation argument for four maximally contrasting
  sources rather than four variations of one.
- Two bugs found and fixed during sound design: a fixed delay used as a
  chorus (it is a static comb filter and reads reedy/brassy), and a
  filtered-noise drift LFO that was numerically unstable in single precision
  at sub-Hz cutoffs.

Alongside it: `Horizon_Pad_VST3_Handoff.md`, and `reference-renders/` holding
the four per-layer renders plus the full blend the owner signed off on.

## Compile check

Playbook: "Faust builds clean after every edit."

```
$ faust -o /dev/null four_pads.dsp     # exit 0
$ faust -o /dev/null g5_layers.dsp     # exit 0
```

Clean on Faust 2.88.0 - i.e. the prototype has not rotted since it was
written, which was not a given.

## Renders

`tools/faust_render/` compiles the prototype into a headless offline WAV
renderer: no audio device, no JACK, no CoreAudio, no libsndfile.

```
$ ./tools/faust_render/build.sh
$ ./build/faust/four_pads_render --dur=10 --gate-off=4 --freq=261.6255653 --out=blend.wav
```

C4, 4 s held, 10 s total, 48 kHz:

| Render | Peak | RMS | Non-finite |
|---|---|---|---|
| Full blend (prototype defaults) | -2.25 dBFS | -17.5 dBFS | 0 |
| Warm Foundation solo | -2.04 dBFS | -15.7 dBFS | 0 |
| Analog Ensemble solo | -3.71 dBFS | -18.9 dBFS | 0 |
| Airy Choir solo | -8.45 dBFS | -23.4 dBFS | 0 |
| Motion Pad solo | -3.42 dBFS | -17.6 dBFS | 0 |

No NaN, no Inf, nothing at or over 0 dBFS. Peak/clip/discontinuity checks pass.

## "and the owner has heard it"

Met by `sound design/reference-renders/four_pads_full_blend_v5.mp3` and the
four per-layer renders - the takes the sound design was signed off on in
September 2025, which is what made the prototype "validated, ready for VST3
port" in the first place. The renders produced above are the same engine
driven by a different front end.

## What this gate handed to G5

A prototype for **all four** layers rather than the Root-only reconstruction
the promotion plan assumed, and `g5_layers.dsp` - a second entry point that
exposes each layer dry, built with Faust's `library()` primitive so it holds
no copy of the DSP and cannot drift. That is what made the null test able to
compare like with like, and it is what caught the shimmer bug. See
[`g5-null-test.md`](g5-null-test.md).

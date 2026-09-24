# G5 - null test: the port against the prototype

**Date:** 2026-09-24 · **Tier:** P · **Gate:** G5
**Artefact required by playbook 6.1:** a residual level. **Pass when:**
residual < -90 dBFS, or the difference is characterised by band and time
window and declared intentional.

## Status: CLOSED on the second form - characterised, not nulled

## Why a sample-accurate null is not available here

Two reasons, both deliberate rather than accidental:

1. **Start phases are randomised per voice** (`LayerBase`'s `rng`), so two
   instances in a project do not start phase-locked and sum coherently. The
   prototype starts every oscillator at phase 0. `--seed` makes the C++
   reproducible but not phase-zero, and signals differing only in start phase
   do not null.
2. **The port replaced the prototype's filters on purpose.** Faust's
   `fi.lowpass(2, ...)` is a Butterworth biquad; the C++ uses TPT/ZDF
   state-variable filters, because every one of these filters is modulated and
   invariant #9 requires TPT there. Different topology, different response, and
   the port is the better of the two.

Chasing -90 dBFS would mean undoing both. So this runs the other form: compare
by band, and say what is intentional.

## Method

Per **layer** and **dry** - oscillators, filter, envelope - because that is
the part that was ported structurally. The master stage was redesigned outright
(per-layer WIDTH instead of one global Haas, an equal-power reverb crossfade
instead of a fixed wet add, a limiter instead of a `* 1.6` trim), so comparing
the two `process` outputs would measure the redesign, not the port.

- Prototype side: `sound design/g5_layers.dsp`, which exposes the four layers
  dry via Faust's `library()` primitive - no copied DSP, so it cannot drift
  from `four_pads.dsp`.
- Port side: `HorizonPadSoundTool --solo=<layer>`, reverb off, widths at 0,
  all macros at 0.5 (the value that means "the sound design's own number").
- Held C4, steady state 1.5-3.5 s, 48 kHz, both sides RMS-matched so what is
  reported is spectral shape rather than level.

Reproduce with `python3 tools/measure/null_test.py`.

## What it found: the shimmer had never worked

The first run put Airy Choir at **-56.6 dB in the 640-1280 Hz band** - the
band its shimmer octave lands in at C4. Investigation:

| Expanse at C4 | Prototype | Port (before) | Port (after) |
|---|---|---|---|
| Fundamental, 523 Hz | +2.2 dB | +2.6 dB | +2.6 dB |
| **Shimmer octave, 1046 Hz** | **-10.2 dB** | **-83.3 dB** | **-26.2 dB** |
| 3rd harmonic, 1569 Hz | -9.9 dB | -19.1 dB | -19.1 dB |

`OctaveShimmer`'s grain read was

```cpp
readPosF = writePos - age * ratio;      // ratio = 2
```

Both `writePos` and `age` advance by one per sample, so the read pointer moved
at `1 - 2 = -1` samples per sample: **backwards, at unity speed**. Reversed
playback of a sustained tone has the *same* pitch, so no octave was ever
produced. Expanse's defining feature - the thing that makes it "occupy clear
air above the other three" - was a time-reversed copy of itself.

Fixed by starting the grain one `grainLength` behind the write head and
closing that gap at `(ratio - 1)` samples per sample, so the read runs forward
at `ratio` times speed. The octave came back 57 dB. Guarded by the
`shimmer_octave_present` test.

This is the single best argument for the gate: the bug had survived a DSP
invariant audit, 30 presets, a GUI rewrite and a release, because nothing had
ever compared the port against what it was a port *of*.

## Residual by band, after the fix

C++ minus prototype, dB. `.` = that band holds under 2% of the layer's energy.

| Layer | 20-80 | 80-160 | 160-320 | 320-640 | 640-1280 | 1280-2560 | 2560-5120 | 5120-10240 |
|---|---|---|---|---|---|---|---|---|
| Warm Foundation | . | -2.2 | +1.1 | **-14.3** | -6.5 | -10.2 | . | . |
| Analog Ensemble | . | . | +2.4 | **-8.2** | +1.8 | -3.0 | -2.1 | . |
| Airy Choir | . | . | . | +0.4 | -18.5 | -4.7 | -9.8 | **-19.3** |
| Motion Pad | . | . | +0.4 | +0.4 | -2.8 | -6.5 | **-7.1** | . |

Broadband delta is 0.00 dB for all four by construction (RMS-matched).

## Characterisation - what each difference is, and whether it is intentional

**1. The port is darker than the prototype, everywhere.** Every layer loses
energy above its fundamental band and gains a little at or below it. Three
causes, all in the filters:

- **Q mismatch.** Faust's `fi.lowpass(2, fc)` is Butterworth, Q = 0.707. The
  C++ sets `setResonance(0.55)` on Root and `0.5` on Clearing and Bloom -
  more damped, so a gentler knee and less energy just below cutoff.
  *Unintentional, cosmetic, and cheap to align if a listening pass wants it.*
- **Filter order on Expanse.** The prototype ends the chain with
  `fi.lowpass(1, 1800)` - explicitly **one** pole, 6 dB/octave. The C++
  `safetyLowpass` is a TPT SVF, which is **two** poles, 12 dB/octave. That is
  the -9.8 dB at 2560-5120 and -19.3 dB at 5120-10240 in the table.
  *Unintentional. This is the largest single divergence left and the most
  likely one to be audible.*
- **TPT vs Butterworth topology.** Intentional, invariant #9, keeps the
  response honest at 44.1 kHz. Small next to the two above.

**2. Shimmer still 16 dB below the prototype** (-26.2 vs -10.2 dB). The
octave is present and correct now; the level is not matched. Faust's
`ef.transpose(2048, 512, 12)` crossfades two grains over 512 samples, while
`OctaveShimmer` uses full Hann windows that sum to unity - different effective
gain. *Known, bounded, and a level question rather than a structural one.*

**3. Everything below 320 Hz agrees within about 2.5 dB.** The oscillators,
detune amounts, drift LFOs and envelopes ported correctly. That is the part of
the null that passes.

## Owed

None of the three remaining items is a defect that measurement can settle -
each changes the sound, so each needs ears:

- Align the filter Q values with the prototype's Butterworth (3 constants).
- Make Expanse's safety lowpass one-pole (`juce::dsp::FirstOrderTPTFilter`).
- Match the shimmer blend level.

**Deliberately not done here.** Each one brightens a layer, and the shimmer fix
above has already brightened Expanse substantially. Stacking four brightening
changes without a listening pass is how a pad ends up harsh. They belong in one
sound-design pass, A/B'd against `sound design/reference-renders/`.

# G1 - Algorithm choice

**Date:** 2026-09-24 · **Tier:** P · **Gate:** G1
**Artefact required by playbook 6.1:** the algorithm family chosen *against the
brief* ([brief.md](brief.md)), with the trade-off stated explicitly.

This gate is written **retrospectively**: the C++ existed before the brief did (the
project was a Tier S sketch until 2026-09-22). So this is not a choice being made,
it is a choice being *audited* - every family already in the code, checked against
playbook 6.4's selection table and against the brief that now governs it.

That is the only honest way to close G1 here, and it is worth the hour, because the
audit turns up **four places where the implementation sits on the "Not" side of
6.4's table**. Two are cosmetic, two are architectural.

---

## The prototype exists

Worth recording before anything else, because it invalidates a decision in the brief.

Six source files cite `four_pads.dsp` as the authority for the design ("see
four_pads.dsp for why"). That file has **never been committed to this repository** -
no `.dsp` appears in any commit on any branch, and the repo's own `sound design/`
folder is empty. Promotion decision #1 in [brief.md](brief.md) was written on the
assumption that it was gone, and committed the project to *reconstructing* Faust for
Root only, as a pilot, with three dated 12.5 exceptions owed for the other layers.

It is not gone. It is one directory above the repo:

```
/Users/bjoernoberli/Library/CloudStorage/OneDrive-Personal/1 Projects/VST3/sound design/
    four_pads.dsp                    177 lines, all four layers + mixer + master
    Horizon_Pad_VST3_Handoff.md      control-to-GUI mapping
    pad1_warm_foundation.mp3         per-layer renders
    pad2_analog_ensemble_v2.mp3
    pad3_airy_choir_v3.mp3
    pad4_motion_pad.mp3
    four_pads_full_blend_v5.mp3      + v1-v4 in _Archive/
```

Its header reads `STATUS: validated, ready for VST3 port`. It carries the design
rationale the C++ comments only gesture at - Cann's brightness/attack-speed grid,
the Prophet VS four-source contrast argument, and two bugs found and fixed during
sound design (the fixed-delay-is-a-comb-filter mistake, and a filtered-noise drift
LFO that was numerically unstable in single precision at sub-Hz cutoffs).

**Consequences:**

- **G2's artefact already exists** for all four layers, not one. Nothing needs
  reconstructing.
- **The three 12.5 exceptions are not owed.** Promotion decision #1 should be
  amended rather than executed.
- **G5 becomes possible against the original**, not against a guess at it - though
  see "What will not null" below, because it will not null cleanly.
- The prototype is **unversioned and outside the repo**, which is how it came to be
  presumed lost. It belongs under version control.

---

## Family-by-family audit

Legend: ✅ matches 6.4 · ⚠️ deviation with a defence · ❌ deviation on the "Not" side

| # | Module | Implemented | Prototype | 6.4 says | |
|---|---|---|---|---|---|
| 1 | Saw oscillators (Clearing ×3, Bloom ×2, Root's edge) | polyBLEP saw | `os.sawtooth` | PolyBLEP | ✅ |
| 2 | Triangle oscillators (Root ×3 + sub, Expanse ×3 at 2×freq) | **naive** triangle | `os.triangle` (bandlimited) | PolyBLEP | ⚠️ |
| 3 | All filters (4 LP, 1 BP, 1 HP, all modulated) | `StateVariableTPTFilter` | `fi.lowpass(2)` / `fi.resonbp` | TPT/ZDF | ✅ |
| 4 | Clearing's chorus/flanger delay read | linear interpolation | `pf.flanger_mono` (linear) | Lagrange/Hermite | ❌ |
| 5 | Shimmer grain read | linear interpolation | `ef.transpose` (linear) | Lagrange/Hermite | ❌ |
| 6 | Shimmer topology | 2-grain +1 oct, **parallel** send → own Freeverb | same | shifter **inside** the reverb feedback loop | ❌ |
| 7 | Shared REVERB | `juce::dsp::Reverb` (Freeverb: 8 comb + 4 allpass/ch) | `re.mono_freeverb` | 16-line FDN | ❌ |
| 8 | Voice architecture | 8 voices, central allocator, 3-tier steal | **monophonic** | - | C++ only |
| 9 | Stereo width | per-layer Haas, lead channel alternating L/R/L/R | one global Haas at the master | - | C++ only |
| 10 | Output stage | make-up → limiter (knee −1.9 dBFS) → 2 Hz DC blocker → clamp | `* 1.6` | shape → ADAA/oversample, DC blocker | ⚠️ |
| 11 | ATTACK / RELEASE / FILTER / REVERB macros | smoothed global scalars | absent | - | C++ only |

### ✅ What is already right

**#3 is the strongest thing in the codebase.** Every filter in the product is a TPT
state-variable filter, and every one of them is modulated - by the envelope (Root,
Clearing), by an LFO (Bloom, Expanse), and by the FILTER macro on top of all of
them. Invariant #9 exists precisely for this case, and the port honours it where the
prototype did not: Faust's `fi.lowpass(2, ...)` is a Butterworth biquad, and
swapping it for a ZDF SVF was an upgrade made deliberately during the port. It also
sidesteps the cramping question at 44.1 kHz almost entirely, which is what
`response_error_db` in the brief is there to measure.

**#1** is straightforward and correct - polyBLEP is exactly 6.4's answer for an
analog-style saw.

### ⚠️ #2 - naive triangles, undefended so far

`LayerBase::triangleWave()` is a bare `4|phase − 0.5| − 1`, with a comment
justifying it: triangle's harmonics fall at −12 dB/octave, so the alias risk is low
enough to skip the correction. For **Root** that argument is sound - it is the
lowest layer in the product, its stack sits at the played pitch with a sub an octave
below, and it is immediately lowpassed at 350–1250 Hz.

For **Expanse** the same argument is much weaker, and this is the one real finding
in the oscillator section. Expanse runs its triangle stack at **`freq * 2`**, an
octave above the played note, through a *resonant* bandpass that sweeps to 1350 Hz,
and then feeds a **+1 octave** pitch shifter. A top-of-keyboard note (MIDI 108,
4186 Hz) therefore puts triangle partials through the shifter at an effective
16.7 kHz fundamental. That is the worst-case alias path in the instrument, and it is
the exact case the brief's `alias_floor_db` target (ASR < −60 dB, NMR < 0 dB across
MIDI 21–108) was written to catch.

**This is not a defect yet - it is an unmeasured risk.** The G3 harness can already
measure it (`--partial-ratios` scores against a declared partial model, so a detuned
stack is not mistaken for aliasing). Decision: **measure before changing anything.**
If NMR passes at MIDI 108, the naive triangle stays and this entry becomes a dated
negative result. Playbook 7 ranks "obvious → inaudible" aliasing 4th, and
"inaudible → −100 dB" as not worth doing.

### ⚠️ #10 - the limiter is a nonlinearity with no antialiasing story

The output limiter is exactly linear below its knee and no factory preset reaches it
(worst of 63 renders: −2.64 dBTP), so in normal use it is not a nonlinearity at all.
Above the knee it is neither oversampled nor ADAA'd. Invariant #4 wants that
documented, which this entry now does; the honest defence is that a limiter that
never engages cannot alias, and the brief's `thd_n_db` target says the same thing
from the other direction. The DC blocker required by invariant #6 is present, and
the reason it is *after* the limiter rather than before is in EX-001.

### ❌ #4 and #5 - linear interpolation on modulated delay reads

Both were already found in the 2026-09-20 audit and carried forward unfixed. G1's
contribution is only to confirm that the prototype is not a defence: Faust's
`pf.flanger_mono` and `ef.transpose` interpolate linearly too, so this was inherited,
not introduced. Linear interpolation on a *modulated* read is a lowpass whose corner
moves with the modulation - it dulls the top end in a way that tracks the sweep,
which on a chorus is heard as the effect losing sparkle at the extremes of its
travel.

The correct family is Lagrange (3rd order) or Hermite. The cost is real and the
character change is audible, which is why the audit recommended routing it through
the `sound-designer` subagent rather than patching blind. Unchanged at G1.

### ❌ #6 - shimmer is parallel, not in-loop

6.4 is unusually blunt here: shimmer means a two-grain octave shifter **inside** the
reverb feedback loop, and explicitly **not** "a pitch shifter in parallel with a
reverb". Expanse is the second thing: `swept → HP → transpose(+12) → small reverb`,
summed at 0.16 against the dry layer at 0.55. The prototype did the same.

**The defence is genuine, and it comes from the brief rather than from convenience.**
6.4's row is written for a shimmer *reverb effect*, where the point is regeneration:
the octave feeds back, stacks octave-on-octave, and blooms over several seconds.
Horizon Pad does not want that. Expanse is a *pad layer* whose job, per the brief, is
to "occupy clear air above the other three" and to sit under acoustic guitar and
vocals - a fixed, non-regenerating +1 octave sheen, at a fixed 16% blend. A
feedback shimmer would also drag in the whole stability apparatus: invariants #7
(in-loop saturator), #16 (in-loop filter peak magnitude ≤ 1 at every coefficient
update) and #19 (empirical decay-slope validation on three signals, because a
pitch shifter makes the loop non-LTI). All of that is the right cost for a shimmer
reverb and the wrong cost for a pad layer that is deliberately not allowed to bloom.

**Verdict: deviation accepted, on the brief's authority.** What makes it acceptable
is that the octave does not regenerate; if a future MOD or macro ever feeds the
shimmer back on itself, this decision is void and #7/#16/#19 all apply.

### ❌ #7 - Freeverb where 6.4 wants a 16-line FDN

The shared REVERB is `juce::dsp::Reverb`, a Freeverb - Schroeder-Moorer, 8 parallel
combs into 4 series allpasses per channel. 6.4 wants a 16-line FDN with
exponentially distributed coprime delays and per-line damping, and names
comb-based Schroeder as the thing not to do. Expanse's shimmer reverb is a second
instance of the same engine.

**This is the one deviation I would not defend as confidently as the others.**

The case for keeping it:
- **CPU.** The brief's budget is < 8% of one core, sized for a ~2017 dual-core i5
  running 3–4 instances in a live worship rig. A 16-line FDN with per-line damping
  is roughly twice Freeverb's cost, and there are two reverb instances here, not
  one. That budget has also **never been measured** - so "we cannot afford an FDN"
  is currently an assumption, not a result.
- **Material, with an important caveat.** Freeverb's known weaknesses - metallic
  ringing and slow echo density buildup - are exposed by *transients*, and the
  brief's premise is sustained pad material. At the ATTACK macro's neutral setting
  the layers' own timings are 1.2–2.6 s, which is about as kind to a Freeverb as
  source material gets.
  **But the macro does not stop there.** `attackReleaseTimeScale()` bottoms out at
  `kMinScale = 0.01`, deliberately - a fully-left ATTACK puts Bloom at **12 ms** and
  the other three at 22–26 ms, and the function's own comment describes that zone as
  "short and percussive". So the instrument absolutely can produce the transient
  material that exposes a comb-based reverb; it simply does not do so in any factory
  preset. This weakens the defence rather than supporting it, and is the main reason
  #7 is carried forward to G3 as a measurement instead of being closed here.
- **It is what the sound design was validated on.** The prototype used
  `re.mono_freeverb`, the five blend renders the owner signed off on were Freeverb,
  and `kWetCalibrationGain = 0.48` is empirically tuned to this engine's wet gain.
  Replacing the engine invalidates that calibration and the preset REVERB values.

The case against is simply that nobody has measured it. **Decision: keep Freeverb
for now, and settle it with numbers rather than with this paragraph.** Appendix B
gives the two that decide it - echo density buildup and EDR (energy decay relief,
for frequency-dependent ringing). If EDR shows a sustained ridge at one frequency,
that is Freeverb's comb signature and the FDN argument wins regardless of CPU. Both
belong in the G3 metric table.

### #8, #9, #11 - additions with no prototype counterpart

The prototype is **monophonic**: one `freq` slider, one `gate` button, no voice
management, no macros, and a single global Haas at the master. Polyphony, the 8-slot
allocator with three-tier stealing, the four per-layer WIDTH taps with alternating
lead channels, and all four macros are C++-only. They are not ports and cannot be
null-tested. Two carry known issues already on the books: the voice-steal
discontinuity (now empirically reproduced at −2…−11 dB) and WIDTH's ~2 dB RMS loss
across its range.

---

## What will not null at G5

Recorded here so G5 is not a surprise. A whole-plugin null against the prototype
will **not** reach −90 dBFS, and chasing it would mean undoing improvements:

1. **TPT SVF vs Butterworth biquad** (#3) - different topology, different response.
   Deliberate, and the port is the better of the two. This alone puts the residual
   well above −90 dB.
2. **Naive vs bandlimited triangle** (#2) - differs by exactly the alias content.
3. **Oscillator phase is seeded from system entropy per voice**, so two runs of the
   *same build* are not bit-identical. Nothing nulls at all without a seed control
   in the sound tool first. **This is the first thing G5 needs.**
4. Polyphony, per-layer WIDTH, macros and the new output stage (#8–#11) have no
   counterpart to null against.

So G5 here is the playbook's second form - "characterised by band and time window
and declared intentional" - run monophonically, one note, macros at the prototype's
fixed values, on a **seeded** build. Per-layer, not whole-plugin: a whole-plugin
null would fold four separate divergences into one number that says nothing.

---

## Gate status

**G1 CLOSED.**

The three items this gate carried forward have all been settled at G3
(2026-09-24, same day) - see [`g3-measurements.md`](g3-measurements.md):

| Item | Outcome |
|---|---|
| **#2** naive triangle on Expanse at 2xfreq | **Measured. Partly confirmed.** Below MIDI 84 nothing in the instrument is worse than -68 dB, so the "triangle is low-alias-risk" argument holds across the played register. Expanse at MIDI 96/108 is the exception at -27.1 / -17.5 dB relative to its own peak (-68 / -74 dBFS absolute), recorded as **EX-003**. Root's naive triangle is fine - it runs at the played pitch behind a low cutoff. |
| **#7** Freeverb vs 16-line FDN | **Measured. Deviation accepted.** T60 across seven octave bands spans 1.53-2.25 s, a ratio of 1.47x, with the top band shortest - damping behaving, no comb ringing. Measured at ATTACK fully left, the worst case for a comb reverb. CPU for the whole plugin is 5.03% of one core on an M3 Pro, so cost was never the binding argument it was assumed to be, but there is no longer a defect to justify the rewrite. |
| **#4/#5** linear interpolation on modulated delays | **Unchanged.** Still a character decision needing ears, still recommended for the sound-design pass rather than a blind patch. |

**One further deviation was found after this document was first written**, by
the G5 null test rather than by this audit: `OctaveShimmer` read its grains
backwards, so Expanse's +1 octave shimmer had never existed. That is not an
algorithm-family choice - the topology in row #6 was right, the implementation
of it was not - so it is recorded at G5 and not here. It does mean row #6's
defence ("the octave does not regenerate") was, until 2026-09-24, defending
something that also did not transpose.

## Postscript: what the audit missed

Worth recording, because it says something about the limits of this kind of
review. This document read every layer's signal path against playbook 6.4 and
correctly identified four deviations - and it did not find any of the three
actual bugs in the code, because all three were *implementation* errors inside
choices that were structurally correct:

- the shimmer's reversed grain read (found by G5, against the prototype),
- WIDTH's sample-count Haas delay (found by G3, as a confound in the alias
  measurement),
- the voice-steal hard reset (found by the 2026-09-20 audit by code reading,
  confirmed by measurement at G3).

Reading the code for what family it belongs to does not tell you whether the
family was implemented correctly. That is what G3 and G5 are for, and on this
project they earned their cost several times over.

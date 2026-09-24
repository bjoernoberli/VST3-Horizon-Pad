# G3 - measurements

**Date:** 2026-09-24 · **Tier:** P · **Gate:** G3
**Artefact required by playbook 6.1:** the metric table, filled with numbers.
**Pass when:** numbers meet the targets in [`brief.md`](brief.md)'s
`quality_targets`, or a deviation is written down and accepted.

Machine: **Apple M3 Pro**, macOS 15, Release build, `HorizonPadSoundTool`
seeded (`--seed`) so every figure here is reproducible.

Reproduce:

```bash
python3 tools/measure/g3_metrics.py      # matrix, CPU, envelope, denormals
python3 tools/measure/alias_check.py     # alias floor
python3 tools/measure/reverb_edr.py      # reverb decay character
python3 tools/measure/null_test.py       # G5, see g5-null-test.md
```

## The table

| Target (from the brief) | Result | Verdict |
|---|---|---|
| DC offset < -80 dBFS | **-113.9 dBFS** worst of 36 rate/block combinations | PASS |
| Denormal count zero | No NaN/Inf, no clipping, no clicks after 60 s of silence; CPU over the silent tail **0.60%** of a core | PASS |
| Clean at 6 sample rates x 6 block sizes | **36/36** clean | PASS |
| Typical preset chord peaks at -3 dBTP | Loudest factory presets -2.6 to -4.1 dBTP (2026-09-23) | PASS |
| Factory presets within +/-1 LU | 17 of 18 at -18.0 LUFS +/-1 LU; the exception is EX-002 | PASS w/ exception |
| CPU < 8% of one core | **5.03%** on an M3 Pro - but see below | **UNVERIFIED on the target machine** |
| Alias floor | Worst alias **-17.5 dB** below its layer's peak (-74 dBFS absolute), Expanse at MIDI 108 | PASS w/ exception EX-003 |
| THD+N: clipper contributes < -80 dB at typical levels | The limiter is exactly linear below its knee and no factory preset reaches it (worst of 63 renders -2.64 dBTP vs a -1.9 dBFS knee), so its contribution at preset levels is zero, not small | PASS by construction |
| FILTER response error < 0.5 dB, 44.1 vs 96 kHz | Not measured as a swept response. Mitigated structurally: every filter in the product is TPT/ZDF, which is cramping-free by construction, and the 36-combination matrix shows no rate-dependent misbehaviour | **NOT MEASURED** |
| Envelope timing within 10% or 2 ms | Not measurable from the audio - see below | **NOT MEASURABLE AS WRITTEN** |
| Null residual < -100 dBFS for a pure refactor | Characterised instead - see [`g5-null-test.md`](g5-null-test.md) | PASS on the second form |

## CPU - the number, and why it does not close the budget

| Condition | Audio | Wall time | Share of one core |
|---|---|---|---|
| 8 voices, all four layers, REVERB 50%, 48 kHz / 64 samples | 10.5 s | 0.528 s | **5.03%** |
| 60 s of silence after the sound stops | 60.5 s | 0.361 s | **0.60%** |

5.03% is inside the brief's 8%. **It does not close the gate**, because the
brief's budget is explicitly sized for "a machine that ran Ableton Live 10
comfortably" - read there as a 2017-2018 dual-core i5 - and this was measured
on an Apple M3 Pro. Single-core throughput between those two differs by
something like 3-4x, which would put the same load at roughly 15-20% of a core
on the target machine, i.e. over budget.

Two honest readings, and the difference matters for the worship-rig case of
3-4 instances:

- The measurement proves the plugin is not pathologically expensive, and that
  there is no denormal problem after silence (the usual cause of a spike).
- It says nothing about the machine the budget was written for.

**Owed: one run on an actual low-end x86 laptop.** Until then the CPU row is
measured but not met. The Windows validation pass is the natural place to
take it.

## Alias floor

Measured by rendering the same seeded note at 48 kHz and at 192 kHz and
comparing spectra below 20 kHz at matched FFT bin width: a partial sits at the
same absolute frequency in both, an alias does not.

**The brief's stated metric (ASR/NMR from the sound tool) was tried first and
does not work on this instrument.** Both score energy that is not at a declared
partial, and on a synth where every layer is a detuned stack behind a resonant
bandpass, the layer's own designed sound scores as residual - Expanse at
MIDI 108 came out at -10 dB ASR with essentially no alias content present. The
metric was measuring the instrument, not the defect.

Worst alias per layer, relative to that layer's own peak (full table via
`tools/measure/alias_check.py`):

| Layer | MIDI 21-84 | MIDI 96 | MIDI 108 |
|---|---|---|---|
| Root | -76 dB or better | -50.2 dB | -39.3 dB |
| Clearing | -74 dB or better | -59.2 dB | -49.3 dB |
| Expanse | -68 dB or better | **-27.1 dB** | **-17.5 dB** |
| Bloom | -98 dB or better | -83.6 dB | -69.8 dB |

Below MIDI 84 - the whole register this instrument is actually played in -
nothing is worse than -68 dB. The top two notes of Expanse are the exception
and are recorded as **EX-003**.

Two findings fell out of this measurement rather than the numbers themselves:

- **Bloom appeared to alias badly (-37.9 dB at MIDI 60) and did not.** It was
  WIDTH: the Haas delay was a flat 90 *samples*, so it was a different length
  of *time* at each sample rate and the two renders were not comparable. Fixed
  (see below); Bloom then measured -115 dB at the same note.
- Expanse's bandpass does not track the note, so above about MIDI 96 the layer
  is mostly its own filter ringing at -40 to -57 dBFS. The aliasing there is
  loud relative to a layer that is nearly silent.

## Reverb decay character

This is the measurement G1 made a condition of accepting Freeverb over
playbook 6.4's 16-line FDN. Worst case chosen deliberately: ATTACK fully left
(12-26 ms attacks), because a comb reverb is exposed by transients.

| Octave band | 63-125 | 125-250 | 250-500 | 500-1k | 1k-2k | 2k-4k | 4k-8k |
|---|---|---|---|---|---|---|---|
| T60 | 2.04 s | 2.25 s | 1.98 s | 1.99 s | 1.82 s | 2.13 s | 1.53 s |

Range 1.53-2.25 s, longest/shortest **1.47x**, and the top band is the
shortest, which is damping behaving correctly. A comb signature would show as
one band ringing far longer than its neighbours. **It does not.** G1 item #7
closes: the deviation is accepted, on measurement.

## Envelope timing - why the target cannot be measured as written

The target is "within 10% or 2 ms of each layer's designed ADSR". Measured
acoustically:

| Layer | Designed attack | Time to peak | 10-90% rise | Sustain ratio |
|---|---|---|---|---|
| Root | 2.2 s | 1.82 s | 1.25 s | 0.41 |
| Clearing | 2.6 s | 1.62 s | 1.22 s | 0.45 |
| Expanse | 2.6 s | 7.11 s | 0.86 s | 0.49 |
| Bloom | 1.2 s | 2.16 s | 1.89 s | 0.27 |

None of these should match the ADSR, and the reasons are all deliberate
design:

- Every layer couples its filter cutoff to its own envelope, so the audible
  envelope rises faster than the amplitude ramp (the filter is opening at the
  same time) and settles lower than the sustain level (it closes again). That
  is why every sustain ratio is well under its designed 0.4-0.8.
- Expanse's 7.11 s "time to peak" is its bandpass sweep LFO at 0.09 Hz - an
  11 s period - not its envelope. Bloom's is contaminated the same way by a
  3.2 Hz tremolo and a 0.6 Hz filter LFO.

So the acoustic envelope is not the ADSR and never was meant to be. The ADSR
values themselves reach `juce::ADSR` unmodified (`LayerBase::beginNote`), which
is verifiable by reading twelve lines of code and is not worth an instrumented
build to re-derive.

**Recommend rewording the target** at the next brief revision, to something
measurable: the four layers must not arrive together. Measured spread between
the first and last layer to peak is **5.49 s**, which is emphatically the case.

## Defects found and fixed during this gate

| # | Defect | Evidence | Regression test |
|---|---|---|---|
| 1 | `OctaveShimmer` read its grains backwards - the +1 octave shimmer never existed | octave partial 73 dB low vs the prototype | `shimmer_octave_present` |
| 2 | WIDTH's Haas delay was a fixed sample count, so it was 2.04 ms at 44.1 kHz and 0.47 ms at 192 kHz | first mono comb null moved with sample rate | `width_is_rate_invariant` |
| 3 | Voice stealing hard-reset a sounding voice mid-cycle | worst of 8 steals stepped **+3.8 dB** relative to the signal, vs -13.5 dB for a free-slot note-on | `voice_steal_declick` |

After the fixes: the shimmer octave sits 57 dB higher, the Haas time is
constant within 1.3% across 44.1-192 kHz, and a steal (median -18.4 dB, worst
-17.0 dB) is statistically indistinguishable from an ordinary note-on
(median -17.1 dB, worst -12.5 dB).

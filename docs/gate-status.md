# Tier P gate status

One page: where Horizon Pad stands against playbook 6.1's gates. Last updated
**2026-09-24**.

Tier **P** (Product), promoted from S on 2026-09-22, which means re-entering at
G0 and doing every gate the project skipped as a sketch.

| Gate | Status | Artefact |
|---|---|---|
| **G0** Brief | **CLOSED** 2026-09-23, owner-confirmed | [`brief.md`](brief.md) + amendments A-001..A-004 |
| **G1** Algorithm | **CLOSED** 2026-09-24 | [`g1-algorithm.md`](g1-algorithm.md) |
| **G2** Prototype | **CLOSED** 2026-09-24 | [`g2-prototype.md`](g2-prototype.md), `sound design/four_pads.dsp` |
| **G3** Measure | **CLOSED** 2026-09-24, two rows short of the brief | [`g3-measurements.md`](g3-measurements.md) |
| **G4** Port | **CLOSED** | Clean Release build; constants documented beside the code |
| **G5** Null test | **CLOSED** on "characterised and declared intentional" | [`g5-null-test.md`](g5-null-test.md) |
| **G6** Validate | **3 of 4** - hosts outstanding | below |
| **G7** Product | **CLOSED**, compatibility matrix pending hosts | README + below |

## G6 in detail

| Requirement | Status |
|---|---|
| Steinberg validator exit 0 | **PASS** - 47 tests passed, 0 failed, 2026-09-24, against the current build |
| pluginval strictness 10 | **PASS** - exit 0, zero failures, 2026-09-24 |
| CTest suite green | **PASS** - 8/8 in 28 s (`ctest --test-dir build-release`) |
| Three hosts, two platforms | **1 of 3.** Ableton Live on macOS confirmed by the owner. Windows Ableton and Waveform outstanding. |

The tests are in `tools/tests/dsp_tests.py`, registered with CTest by
`CMakeLists.txt`. Five assert definition-of-done properties (latency, seeded
determinism, the 36-combination rate/block matrix, behaviour after 60 s of
silence, all 18 presets safe); three are regression guards, one per bug that
measurement caught on 2026-09-24.

## What is outstanding

Nothing blocks a release except host validation. In rough order of how much
each would change:

1. **Three hosts, two platforms** (G6). Windows Ableton and Waveform. The only
   hard gate still open.
2. **CPU on the target machine** (G3). 5.03% of a core measured on an M3 Pro;
   the brief's 8% budget is written for a 2017-era dual-core i5, where the
   same load would plausibly be 15-20%. Take the measurement during the
   Windows pass.
3. **A listening pass.** Nothing in this instrument has been heard since the
   2026-09-23 re-stage, and four measured-but-unheard changes have landed
   since. See below.
4. **FILTER response error at 44.1 vs 96 kHz** (G3). Not measured as a swept
   response; mitigated structurally by every filter being TPT/ZDF.

## The listening pass, in one place

These are the items that measurement has taken as far as it can. Each is a
character decision.

| From | Item |
|---|---|
| EX-001 | The 2 Hz DC blocker's effect on the lowest octave (-0.09 dB at 13.75 Hz, calculated not heard) |
| EX-002 | Whether to rebalance Sternenzelt into the loudness match, at the cost of some of its purity |
| EX-003 | Whether Expanse's alias at MIDI 96-108 is audible at all |
| G5 | The shimmer now actually transposes - Expanse is substantially brighter than the shipped sound |
| G5 | Filter Q values sit below the prototype's Butterworth; Expanse's safety lowpass is 2-pole where the prototype's is 1-pole |
| G5 | Shimmer blend sits 16 dB below the prototype |
| G1 #4/#5 | Linear interpolation on the flanger and shimmer grain reads, where the rule wants Lagrange/Hermite |

Reference material for the A/B is committed: `sound design/reference-renders/`
holds the four per-layer renders and the full blend the sound design was
signed off on.

## Exceptions on the books

| ID | Subject | Status |
|---|---|---|
| EX-001 | DC offset when the output limiter engages | CLOSED 2026-09-23 - fixed, marginal pass |
| EX-002 | Sternenzelt ~4.8 LU below the matched bank | OPEN - accepted |
| EX-003 | Expanse aliases at MIDI 96-108 | OPEN - accepted |

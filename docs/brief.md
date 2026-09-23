# Horizon Pad - sonic brief (G0)

Template: `dsp-sound-design-playbook.md` 6.2. This is the G0 artefact. The gate does
not close until the four `TODO (owner)` fields below are filled and the whole document
is confirmed.

**Status: G0 CLOSED, confirmed by the owner 2026-09-23.** Written on commit
`a0a0aec`. Every field is confirmed except the two items under `open_questions`,
neither of which blocks design work.

```yaml
name:            Horizon Pad (Quelle Music)
type:            synth
tier:            P          # promoted from S on 2026-09-22 - see "Tier history"
one_sentence:    A four-layer ambient pad instrument whose four pads are each usable
                 on their own for a different section of a song, and which blend into
                 one another by volume alone.
references:
  - Vital - a synth you program. Wavetables, a full modulation matrix, an editing
    surface that rewards study. Horizon Pad is not that: it is an INSTRUMENT you
    play. Some depth, nowhere near Vital's, and what depth there is sits on
    performance controls rather than on a design surface.
  - Spitfire Audio (LABS / Originals) - the same "load it and it already sounds
    like the record" immediacy, and the same instrument framing, but sample-based,
    so a library download and disk streaming. Horizon Pad is synthesised in code:
    no samples, instant load, and it never runs out of a round-robin.
  # The frame: an instrument, not a synth. Spitfire's immediacy at Vital's zero
  # install weight. Editing depth is deliberately shallow - see out_of_scope.
must_do:
  - Four contrasting pads, not four variations of one idea; any single pad must hold
    up alone in a mix.
  - Blending is by volume alone. No per-layer tone block beyond WIDTH.
  - Everything synthesised in code: no samples, no audio files, no bitmap artwork.
  - Playable live from a keyboard: zero latency, spring-loaded PITCH, held MOD.
out_of_scope:
  - Per-layer tone knobs (cutoff, detune, osc mix). Confirmed out, 2026-09-23.
    This is the line that keeps the product an instrument rather than a synth -
    each pad's character stays baked in, and a preset stays a point in the
    12-dimensional blend/width/macro space.
  - MPE and per-note expression
  - A resizable GUI (fixed 1080x748, every child paints its own layout)
  - Double-precision processing (supportsDoublePrecisionProcessing left at false)
  # Deferred, NOT permanently excluded:
  #   Sampled content - "follows on a later date" (owner, 2026-09-22). That is a
  #   promotion trigger, not a v1 feature: it changes the install story, the
  #   licensing and the CPU profile, and re-enters at G0 when it happens.
core_controls:
  - VOL x4 (Root/Clearing/Expanse/Bloom) - the blend; the product's central idea
  - FILTER - global brightness macro, ramped per-sample via smoothedBrightness
  - ATTACK / RELEASE - global scalars on each layer's own fixed ADSR timings,
    preserving the relative timing offsets that stop all four pads arriving at once
  - REVERB - shared stereo send, equal-power wet/dry
  - WIDTH x4 - per-pad Haas spread, lead channel alternated per layer
  # Performance state, deliberately NOT host parameters: PITCH (spring-loaded),
  # MOD (holds). Two atomics, read once per block.
signal_flow: |
  MIDI -> central voice allocator (8 slots, 3-tier steal: free -> oldest released
          -> oldest held). Every layer gets the same note in the same slot.
    |
    +-- Root      tri stack + polyBLEP saw + sub(-1 oct) -> LPF (breathing cutoff)
    +-- Clearing  unison stack -> LFO-swept delay 2-11 ms @ 0.11 Hz -> LPF
    +-- Expanse   detuned stack -> swept BPF
    |               \-- HPF -> OctaveShimmer (2-grain, +1 oct) -> small reverb
    +-- Bloom      detuned stack -> LPF (LFO cutoff) -> tremolo
    |
    each layer: -> ADSR (fixed per-layer timings) -> WIDTH (Haas tap, <=90 samples,
                   lead channel alternates L/R/L/R across the four layers)
    |
    sum with per-layer smoothed gain
    -> FxChain: shared stereo juce::dsp::Reverb send, equal-power crossfade,
       wet pre-scaled by kWetCalibrationGain = 0.48
    -> output trim 0.75 -> tanh soft clip -> out
cpu_budget:      CONFIRMED 2026-09-23: < 8% of one core per instance at
                 48 kHz / 64 samples, with 8 voices sounding, all four layers at
                 non-zero volume and REVERB at 50%. Measured in Release, and again
                 AFTER the sound stops (where denormal problems appear).
                 Minimum target machine: "a machine that ran Ableton Live 10
                 comfortably" (owner) - read as a ~2017-2018 dual-core i5 laptop.
                 8% leaves room for 3-4 instances plus a band's worth of other
                 tracks, which is the real worship-rig case.
latency_budget:  0 samples. Non-negotiable: this is played live. No setLatencySamples
                 call exists; to be asserted by impulse measurement in a test (G6).
sample_rates:    [44100, 48000, 88200, 96000, 176400, 192000]
                 # verified clean 2026-09-22: no NaN/Inf/clip/click at any of them
channels:        mono-to-stereo
target_user:     A musician with technical interests - reads a spec, but is
                 playing a set on Sunday, not designing a patch.
target_genre:    Worship and CCM. Reference artists: Rend Collective, Eden Music,
                 Upstream - specifically their acoustic versions. That sets the
                 job: sit UNDER acoustic guitar, piano and group vocals without
                 fighting them, and stay playable live from a controller.
quality_targets:
  alias_floor_db:    ASR < -60 dB measured on a single high note (worst case, sparse
                     spectrum, not a dense chord); NMR < 0 dB across MIDI 21-108.
                     NMR is the decisive metric - ASR can pass while one unmasked
                     alias tone is clearly audible.
  thd_n_db:          The instrument is not a clean path; the only nonlinearity is the
                     output tanh. Target: contribution of the safety clipper < -80 dB
                     at typical preset levels, i.e. it must not engage in normal use.
                     Conditions to be stated with the number.
  response_error_db: < 0.5 dB (analog-style) for the FILTER macro against its design
                     target, with the 44.1 vs 96 kHz responses overlaid - this doubles
                     as the cramping test.
  other:
    - DC offset < -80 dBFS (measured 2026-09-22: ~1e-6 worst case, i.e. ~-120 dB)
    - Denormal count zero
    - Null residual for a pure refactor < -100 dBFS
    - Envelope timing within 10% or 2 ms of each layer's designed ADSR
    - Typical preset chord peaks at -3 dBTP (decided 2026-09-22). DONE
      2026-09-23: output stage re-staged +8.9 dB. Loudest factory presets now
      -2.6 to -4.1 dBTP, median -7.3 dBTP, pathological maximum -0.1 dBTP with
      no clipping. The remaining spread is preset work, not gain-stage work.
    - Factory presets loudness-matched to within +/-1 LU. DONE 2026-09-23:
      bank curated 30 -> 18 on character (docs/preset-curation-2026-09-23.md),
      then matched to -18.0 LUFS. 17 of 18 land within +/-1 LU (measured spread
      1.04 LU across three voicings); Sternenzelt cannot reach it and is
      recorded as EX-002
open_questions:
  - Windows x64 host access: CI builds it, but 5.5 needs a human to tick the boxes.
  - Logic Pro would need an AU target; CMakeLists currently builds VST3 + Standalone.
  - Three of four layers will ship without a Faust prototype (see Tier history).
```

## Tier history

| Date | Tier | Note |
|---|---|---|
| (origin) | S, undeclared | Built as a sketch; G0 was never written |
| 2026-09-22 | **P** | Promoted by the owner. Re-enters at G0 and does every skipped gate, per 0.1. No gate is grandfathered on the grounds that the plugin already works. |

## Decisions taken at promotion (2026-09-22)

1. **Prototype / null-test basis.** Faust is reconstructed for **Root only**, as a
   pilot, to find out whether the C++ can null against a rebuilt prototype at all.
   The other three layers proceed without one, which is a deviation from 6.1's G5 and
   needs a dated 12.5 exception each. 0.1's Tier P definition of null testing
   ("whole-plugin, vs previous release") governs in the meantime.
2. **Output level.** Re-stage to a typical preset chord peak of **-3 dBFS**.
3. **Presets.** Curate 30 down to **~18**, loudness-matched. Measured spread today is
   9.1 dB RMS.
4. **Hosts.** Ableton Live confirmed, plus Waveform Free and Reaper covering
   Windows x64 (5.5's matrix still needs a human to tick the boxes).
5. **Positioning.** An instrument, not a synth. Confirmed 2026-09-23 after the
   first draft read as "no editing surface at all" - there is some depth, it is
   just far shallower than Vital's and lives on performance controls.
6. **Per-layer tone knobs.** Confirmed OUT of scope, 2026-09-23.

## Where the exceptions go

12.5 requires a written, dated, named exception whenever a measurement fails and the
decision is to ship anyway: what failed, by how much, why it is acceptable, and what
would make it unacceptable. Those live in [`exceptions.md`](exceptions.md), which
currently holds one open entry (EX-001, limiter-induced DC offset).

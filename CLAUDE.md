# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Horizon Pad (Quelle Music) — a four-layer ambient pad synthesiser VST3/Standalone
plugin, built with JUCE 8.0.4 (C++17). Everything is synthesised in DSP code:
no samples, audio files, or bitmap artwork anywhere in the repo — oscillators,
the shimmer/pitch-shift engine, and the entire GUI (including the sunset
banner and mountain range) are drawn/generated in code.

## Build commands

JUCE is pulled in automatically via CMake `FetchContent` — no submodule init needed.

```bash
# Configure + build the VST3 (macOS, Ninja)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target HorizonPad_VST3

# Windows
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --target HorizonPad_VST3 --config Release
```

Bundle output: `build/HorizonPad_artefacts/Release/VST3/Horizon Pad.vst3`
(Debug config → `.../Debug/VST3/...`).

For local dev iteration on macOS, `tools/install_plugin.sh` builds Debug and
copies straight to `~/Library/Audio/Plug-Ins/VST3/` (per-user folder, no
admin password prompt, safe to run unattended).

Faster iteration against a local JUCE checkout instead of re-fetching:
`cmake -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE`.

### Offline DSP sanity/analysis tool

```bash
cmake --build build --target HorizonPadSoundTool -j 8
./build/HorizonPadSoundTool --help
```

`HorizonPadSoundTool` (`tools/sound_tool/Main.cpp`) links directly against the
real plugin code and renders offline — no audio device, host, or editor
needed. It prints one JSON object to stdout with peak/RMS levels, NaN/Inf/
clipping/click detection, DC offset, envelope trajectory, and spectral
analysis. This is the fastest way to verify a DSP change didn't introduce
clipping, NaN/Inf, or zipper artifacts, without opening a DAW. Key flags:
`--solo=root|clearing|expanse|bloom`, `--notes=60`, `--preset=<name>`,
`--param=<id>=<value>`, `--out=<wav path>`, `--list-presets`. Applied in
order preset → solo → param, each overriding the last.

### The Faust prototype

`sound design/four_pads.dsp` is the sound-design source of truth every layer
class cites by name, and `sound design/g5_layers.dsp` exposes the same layers
dry for null testing (via Faust's `library()`, so it holds no copied DSP).
`./tools/faust_render/build.sh` compiles either into a headless offline WAV
renderer — no audio device needed. `sound design/reference-renders/` holds the
signed-off renders to A/B against. These are design references, not product
assets: nothing under `sound design/` is compiled, linked or loaded at runtime.

### Validating a structural change

```bash
git clone --depth 1 https://github.com/steinbergmedia/vst3sdk.git
cd vst3sdk && git submodule update --init --depth 1 -- base cmake pluginterfaces public.sdk && cd ..
cmake -B vst3sdk-build -S vst3sdk -DCMAKE_BUILD_TYPE=Release \
  -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF \
  -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=OFF
cmake --build vst3sdk-build --target validator
./vst3sdk-build/bin/Release/validator "build/HorizonPad_artefacts/Release/VST3/Horizon Pad.vst3"
```

`pluginval --strictness-level 10 --validate "path/to/Horizon Pad.vst3"` is also
recommended if installed. Known gap: not re-validated against either tool
since the four-layer/12-parameter rewrite (see README "Known gaps").

### Tests

```bash
cmake --build build-release --target HorizonPadSoundTool -j 8
ctest --test-dir build-release --output-on-failure
```

Nineteen tests (~40 s), `tools/tests/dsp_tests.py`, registered by CMake. They
drive the real DSP through `HorizonPadSoundTool` rather than unit-testing
classes: latency, seeded-reset determinism, the 36 sample-rate x block-size
combinations, 60 s of silence, all 18 presets; regression guards, one per bug
measurement has caught (`shimmer_octave_present`, `width_is_rate_invariant`,
`voice_steal_declick`, `reverb_level_flat`); a presence test per defining
feature (`root_sub_present`, `clearing_ensemble_present`,
`bloom_tremolo_survives_chords`, `width_profile_staggered`,
`detune_spreads_every_stack`); and instrument-level contracts
(`keyboard_level_span`, `width_mono_safe_and_level_flat`, `sustain_pedal_holds`,
`low_register_stays_musical`, `bass_is_mono`). **Add a regression test whenever a
DSP defect is fixed, and show it failing on the build that has the defect**
(a `git worktree` of the previous commit). Needs python3 + numpy + scipy; CMake
skips registration if they are missing, so a plain plugin build never fails
without them.

`tools/measure/` holds the measurement scripts: G3/G5 (alias floor, metric
battery, reverb decay, port-vs-prototype null test), `descriptors.py` (is it the
sound we designed: layer grid, keyboard, mix pocket, stereo, movement, presets -
four seeds per figure, `--save`/`--compare` against `docs/baselines/`) and
`register.py` (roughness, pitch salience, harmonic content and low side energy
per note, C1-C5 and low voicings). `tools/listening/make_session.py` renders a
blind, loudness-matched A/B/X page (baseline build vs candidate) for the
listening passes the measurements cannot settle.

The tool's `--param` flag is repeatable (it silently kept only the last one
before 2026-09-26 - G5 was re-run, see `docs/g5-null-test.md`); the applied
state is echoed under `activeParams`, and scripts should assert on it.
`--pedal=down,up` sends the sustain pedal.

### CI

`.github/workflows/build.yml`: builds VST3 on macOS (universal binary) +
Windows (x64), packages an unsigned (ad-hoc signed) macOS `.pkg` and a Windows
installer folder, zips both into one cross-platform release artifact, runs the
DSP test suite, and runs the Steinberg validator as a **hard gate** (promoted from
non-blocking on 2026-09-24). Not yet in CI (playbook D.4): pluginval, a sanitizer
(RTSan) job, a pinned VST3 SDK commit for the validator build.

## Architecture

### Signal path

`PluginProcessor` owns four independent `LayerBase`-derived synth layers plus
one shared `FxChain`. Each `processBlock` call: MIDI is turned into centralized
voice allocation (see below), each layer renders into its own scratch buffer
(`layerBuffer`) with its own oscillators/filter/envelope, the four layer
outputs are summed with per-layer smoothed gain, then the mix passes through
`FxChain` (a single shared stereo `juce::dsp::Reverb` send) and a final `tanh`
soft-clip safety net.

### The four layers (`Source/dsp/`)

| GUI name | Class | `LayerIndex` | Character |
|---|---|---|---|
| Root | `WarmFoundationLayer` | `warmFoundation` | Detuned triangle stack + polyBLEP saw edge + sub-osc (−1 oct), lowpass with "breathing" cutoff |
| Clearing | `AnalogEnsembleLayer` | `analogEnsemble` | Unison saw stack into a lowpass, then a wet-only stereo 3-tap ensemble on the layer bus (0.6 + 5.5 Hz, bass below 200 Hz bypasses it) |
| Expanse | `AiryChoirLayer` | `airyChoir` | Detuned triangle stack through swept bandpass + parallel shimmer send (highpass → `OctaveShimmer` 2-grain +1-octave pitch shift → its own stereo reverb); below C4 pinned near C5 (register pinning) |
| Bloom | `MotionPadLayer` | `motionPad` | Detuned stack through lowpass with LFO-modulated cutoff + tremolo — deliberately never static |

Each layer's class doc comment is the authoritative description of its
design intent (why triangle vs saw, why a filter opens with the envelope,
detune amounts, etc.) — read it before changing behavior; don't "fix"
something the comment explains is deliberate. The `sound design/` folder at
the repo root may hold additional design notes/references.

`LayerBase.h` is the shared machinery all four layers inherit: fixed
(non-user-editable) per-layer ADSR timings from the original Faust sound
design, the global macros applied to every layer each block
(`attackTimeScale`, `releaseTimeScale`, `brightness` — ramped per-sample via
`smoothedBrightness` to avoid filter zipper noise — plus WIDTH and DETUNE),
stereo WIDTH (one macro, turned into each layer's own width by its fixed
`widthProfile()` - Expanse opens from 0%, Clearing 10%, Bloom 20%, Root 30% and
only to 60%; the width spreads each voice's detuned oscillators across the
field, constant-power, mirrored on odd voices; mono-safe and level-flat - it
replaced a Haas tap on 2026-09-26 and four per-pad WIDTH knobs on 2026-10-02),
DETUNE (`makeDetuneRamp()`/`driftDepth()`: scales each stack's static detune
and baseline drift, 0.5x-2x with 50% bit-exact, eased out below C3; Root's saw
edge and sub and the MOD wheel's drift are not scaled), and the register helpers every layer uses:
`keyTrack` (asymmetric key tracking anchored at C4, the voicing note),
`unisonFor` (partners and drift tighten below C3), smoothed-random `Drift`,
`polyBlampTriangle`. `FxChain.h/.cpp` holds the shared reverb send: the side
channel is high-passed at 140 Hz first (mono bass), then a one-pole split at
160 Hz keeps the bass dry and only the band above is sent (20 ms pre-delay) and
crossfaded equal-power.

### Voice allocation

Centralized in `PluginProcessor`, not per-layer: up to `kMaxVoices` (8) voice
slots, four-tier stealing (free slot → oldest released-but-ringing slot →
oldest note held only by the sustain pedal → oldest held key). CC64 holds
released keys until pedal-up; striking a key that is already sounding releases
its old voice first. Every layer is told about the same note in the same voice
slot each block, so a chord stays coherent across all four layers.

### Parameters and thread-safety model

Ten host-automatable APVTS parameters (all 0–100%) in fixed order: 4
volumes, 6 macros (Attack/Release/Filter/Reverb/Width/Detune, `MacroIndex`) — see
`Source/presets/Presets.h` for the parameter IDs and README's Parameters
table for the full semantics of each macro. All are smoothed
(`juce::SmoothedValue`) somewhere on their path to audio.

- Host parameters live in an `AudioProcessorValueTreeState`; audio thread
  reads them via cached `std::atomic<float>*` pointers (no string lookups,
  no locks).
- PITCH/MOD are performance controls (`Source/dsp/PerformanceState.h`), not
  host parameters — two atomics written by MIDI pitch-bend/CC1 or the
  on-screen wheel, read once per block. PITCH is spring-loaded; MOD holds.
- A/B buffers (two full 10-parameter snapshots) stay in sync via an APVTS
  `Listener::parameterChanged()` callback, which can fire from any thread.
- User presets (`UserPresetStore`, `UserPresets.xml`) are message-thread-only
  — created/read/deleted exclusively from GUI actions, so no extra sync is
  needed.

See the class doc comment at the top of `Source/PluginProcessor.h` for the
full thread-safety model before touching processor state.

### GUI (`Source/gui/`)

Fixed 1080×748 window (`setResizable(false, false)`), pixel-accurate to a
design handoff — every child paints its own precise layout rather than
scaling a shared "design surface". `HorizonLookAndFeel` centralizes
palette/typography/panel painters; other classes are one widget each
(`PadKnob` = one layer's VOL knob, `MacrosPanel` = the six macros in three
rows, `WheelSlider` =
PITCH/MOD, `PresetBar`, `OutputMeter`, `FooterBar`, `TitleBanner`).

## Tier P process

The project runs at playbook **Tier P**. `docs/gate-status.md` is the one-page
view of where each gate stands and what is outstanding; the per-gate artefacts
sit beside it (`brief.md`, `g1-algorithm.md`, `g2-prototype.md`,
`g3-measurements.md`, `g5-null-test.md`), and `docs/exceptions.md` is the
playbook 12.5 log. **A failed measurement is either fixed or gets a dated,
named exception** — it is not negotiable by whoever wrote the code. `brief.md`
is owner-confirmed: do not edit the YAML, add a dated amendment beneath it.

## Working on DSP sound quality

For "make layer X sound like Y" / artifact-hunting / tone-shaping tasks on
the four layers or `FxChain`, prefer the `sound-designer` subagent
(`.claude/agents/sound-designer.md`) — it drives the render→measure→edit→
rebuild→re-render loop against `HorizonPadSoundTool` with defined tolerances
for run-to-run variance (each layer seeds its RNG from system entropy per
voice, so renders are not bit-identical run to run; `levels.rms` is the
stable metric, `levels.peak` and spectral ratios can swing widely and need
2–3 renders averaged). It treats `Source/PluginProcessor.h/.cpp`,
`Source/dsp/LayerBase.h`, `Source/dsp/FxChain.h/.cpp`,
`Source/presets/Presets.h`, and `Source/gui/` as off-limits without explicit
permission, staying inside individual layer `.h`/`.cpp` files.

## DSP work

Invariants load automatically for `Source/**` and `*.dsp`. For design, porting, measurement, testing, review or release, use `/dsp-playbook`; for a specific defect, `/dsp-diagnose`. The playbook is user-level, not in this repo, in `~/.claude/docs/`: the core card `dsp-playbook-core.md` (read whole; section 0 maps tasks to recipes), and the reference split into `dsp-selection.md` (S, algorithm choice), `dsp-foundations.md` (F), `dsp-instruments.md` (I), `dsp-effects.md` (E), `dsp-control.md` (C), `dsp-analysis.md` (A) and `dsp-development.md` (D: real-time safety, performance, testing, CI, host contract, licensing, agent workflow, task recipes) - grep them, never read them whole. `dsp-sound-design-playbook.md` is the index. `~/.claude/docs/dsp-kit/` holds the tested measurement library (`dspkit.py`), the harness contract new products follow, a test scaffold and the document templates.

The playbook is at v3.2 (2026-10-02): 37 rules tagged by track. Horizon Pad is an instrument (track I) that also owns control (C: MIDI, pedal, wheels), processing (E: the shimmer, ensemble, reverb), mix and output (F.5-F.6). Its v2 sound-design pass is the playbook's instrument worked example (I.10); see `docs/dsp-review-2026-09-26.md` (findings) and `docs/sound-design-v2.md` (changes, measurements, listening list). Not yet audited against this repo: event timing (rule 29 - JUCE applies automation per block), control mappings and note hygiene (30), nonlinear models (33 - the output limiter and tanh), the translation battery (34 - not yet run), real-time safety verified by RTSan or pluginval `--rtcheck` (35), frozen IDs with a versioned state (36 - **not yet applicable**: Horizon Pad is unreleased and testing-only, so parameters, state and presets change freely with no migration or version flags until the owner announces a release; at that point the state gains a version and golden state files, D.5.3), and worst-case block time (37 - the 5.0% CPU figure in the G7 notes is an average). Rule 32 (bypass) does not apply to an instrument.

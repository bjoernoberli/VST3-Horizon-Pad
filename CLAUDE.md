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

There are no unit tests in this repo — correctness is verified via
`HorizonPadSoundTool` (DSP) and the VST3 validator/pluginval (plugin format
conformance).

### CI

`.github/workflows/build.yml`: builds VST3 on macOS (universal binary) +
Windows (x64), packages an unsigned macOS `.pkg` and a Windows installer
folder, zips both into one cross-platform release artifact, and runs the
Steinberg validator non-blocking (`continue-on-error: true`).

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
| Clearing | `AnalogEnsembleLayer` | `analogEnsemble` | Unison stack + LFO-swept delay line (2–11ms, 0.11Hz) chorus/flanger, into a lowpass |
| Expanse | `AiryChoirLayer` | `airyChoir` | Detuned stack through swept bandpass + parallel shimmer send (highpass → `OctaveShimmer` 2-grain +1-octave pitch shift → its own small reverb) |
| Bloom | `MotionPadLayer` | `motionPad` | Detuned stack through lowpass with LFO-modulated cutoff + tremolo — deliberately never static |

Each layer's class doc comment is the authoritative description of its
design intent (why triangle vs saw, why a filter opens with the envelope,
detune amounts, etc.) — read it before changing behavior; don't "fix"
something the comment explains is deliberate. The `sound design/` folder at
the repo root may hold additional design notes/references.

`LayerBase.h` is the shared machinery all four layers inherit: fixed
(non-user-editable) per-layer ADSR timings from the original Faust sound
design, three global macros applied identically to every layer each block
(`attackTimeScale`, `releaseTimeScale`, `brightness` — the last ramped
per-sample via `smoothedBrightness` to avoid filter zipper noise), and each
layer's own per-layer stereo WIDTH (a short ≤90-sample Haas delay tap,
alternated left/right per layer via `setWidthLeadChannel()` so the four
layers' precedence-effect pulls roughly cancel in the mix — see the width
block comment in `LayerBase.h` for why a naive equal-gain delay tap would
null in mono). `FxChain.h/.cpp` holds only the shared reverb send now (WIDTH
used to live there as one global macro; it moved to a per-pad knob).

### Voice allocation

Centralized in `PluginProcessor`, not per-layer: up to `kMaxVoices` (8) voice
slots, three-tier stealing (free slot → oldest released-but-ringing slot →
oldest held note). Every layer is told about the same note in the same voice
slot each block, so a chord stays coherent across all four layers.

### Parameters and thread-safety model

Twelve host-automatable APVTS parameters (all 0–100%) in fixed order: 4
volumes, 4 macros (Attack/Release/Filter/Reverb), 4 widths — see
`Source/presets/Presets.h` for the parameter IDs and README's Parameters
table for the full semantics of each macro. All are smoothed
(`juce::SmoothedValue`) somewhere on their path to audio.

- Host parameters live in an `AudioProcessorValueTreeState`; audio thread
  reads them via cached `std::atomic<float>*` pointers (no string lookups,
  no locks).
- PITCH/MOD are performance controls (`Source/dsp/PerformanceState.h`), not
  host parameters — two atomics written by MIDI pitch-bend/CC1 or the
  on-screen wheel, read once per block. PITCH is spring-loaded; MOD holds.
- A/B buffers (two full 12-parameter snapshots) stay in sync via an APVTS
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
(`PadKnob` = one layer's VOL/WIDTH pair, `MacrosPanel`, `WheelSlider` =
PITCH/MOD, `PresetBar`, `OutputMeter`, `FooterBar`, `TitleBanner`).

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

Invariants load automatically for `Source/**` and `*.dsp`. For design, porting, measurement or review, use `/dsp-playbook`. For a specific defect, `/dsp-diagnose`. Reference: `docs/dsp-sound-design-playbook.md` - grep it, never read it whole.


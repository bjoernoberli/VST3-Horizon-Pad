# Horizon Pad

**Quelle Music — Horizon Pad v1.0.0**

A four-layer ambient pad synthesiser, built with JUCE and shipped as a VST3
instrument.

Everything Horizon Pad makes is synthesised from scratch in DSP code. There is
no sample content, no audio files and no bitmap artwork anywhere in this
repository — the oscillators, the grain engine and the entire user interface
(including the sunset banner and the mountain range) are generated in code.

---

## The four layers

| # | Layer | Synthesis |
|---|-------|-----------|
| 1 | **Warm Pad** | Three-oscillator bank per voice, each crossfading sine → band-limited saw, with slow independent pitch drift for a gentle chorus-like breathing |
| 2 | **Analog Strings** | Seven detuned oscillators in unison per voice (classic string-machine stack), stereo-spread and run into a lowpass |
| 3 | **Granular Texture** | A real grain engine: a fixed grain pool, a density-driven scheduler, Hann-windowed grains reading from two procedurally generated source tables (coloured noise and an inharmonic partial stack), pitched to the notes being held |
| 4 | **Sub Pad** | One oscillator per voice, sine → triangle blend, one or two octaves down, summed to centre |

All four layers respond to MIDI note on/off with shared voice allocation (up to
8 voices), so a held chord sounds coherently across every layer. Attack and
release times are long by design — this is an ambient pad, not a lead.

## Global FX chain

Post layer-mix: **filter → delay → reverb**.

- **Filter** — state-variable lowpass, cutoff 200 Hz … 18 kHz
- **Delay** — 440 ms cross-fed stereo delay with damped (darkening) repeats
- **Reverb** — `juce::dsp::Reverb`; room size tracks the Reverb parameter
- **FX Amount** — a single depth macro over the whole wet side. At `0` the chain
  is audibly transparent no matter where the other three knobs sit; at `1` they
  act at full value. The exact mapping is documented in
  [`Source/dsp/FxChain.h`](Source/dsp/FxChain.h).

A `tanh` soft clip on the output keeps extreme preset/FX combinations sane.

---

## Parameters

Exactly **eight** parameters are exposed to the host as automation lanes. All
are 0–100 %.

| # | Parameter | ID |
|---|-----------|-----|
| 1 | Warm Pad Volume | `warmPadVolume` |
| 2 | Analog Strings Volume | `analogStringsVolume` |
| 3 | Granular Texture Volume | `granularVolume` |
| 4 | Sub Pad Volume | `subPadVolume` |
| 5 | Reverb | `reverb` |
| 6 | Delay | `delay` |
| 7 | Filter | `filter` |
| 8 | FX Amount | `fxAmount` |

### The tone block (not automatable, but saved)

The per-layer **TONE / ATTACK / RELEASE** knobs in the UI — plus the waveform
blend, detune amount and grain density that presets set behind the scenes — are
*not* host parameters. They are a separate, non-automated tone block:

- set by every factory preset,
- editable from the plugin's own UI,
- serialised into the plugin state alongside the APVTS,
- carried between the UI and the audio thread by a lock-free array of
  `std::atomic<float>` guarded by a generation counter
  (`Source/dsp/ToneState.h`) — no locks and no shared mutable structs on the
  audio path.

This is deliberate: it keeps the host's automation view to the eight parameters
that matter for a mix, while leaving the sound design to presets.

---

## Factory presets

Six factory programs, wired to both the plugin's preset panel and the host's
program list (`getNumPrograms` / `setCurrentProgram` / `getProgramName`).

| # | Preset | Character |
|---|--------|-----------|
| 1 | **Golden Horizon** | Warm, evolving pad with bright horizon and soft motion (default) |
| 2 | **Mountain Breeze** | Airy high texture with light air and open brightness |
| 3 | **Deep Forest** | Dark, slow-blooming pad with deep low end and long tails |
| 4 | **Ocean Mist** | Lush undulating wash with long delay and deep reverb |
| 5 | **Night Ambient** | Sparse, dark and very slow, carried by a deep sub |
| 6 | **Dreamscape** | Shimmering, heavily processed cloud with a huge wet tail |

Each preset's full value set, and a comment explaining the tone-design intent
behind it, lives in [`Source/presets/Presets.cpp`](Source/presets/Presets.cpp).

---

## Building

Requirements: **CMake ≥ 3.22** and a C++17 compiler. JUCE 8.0.4 is pulled in
automatically by CMake `FetchContent` — there is no submodule to initialise and
nothing to install first.

```bash
git clone https://github.com/bjoernoberli/VST3-Horizon-Pad.git
cd VST3-Horizon-Pad

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target HorizonPad_VST3 --config Release
```

The bundle lands at:

```
build/HorizonPad_artefacts/Release/VST3/Horizon Pad.vst3
```

### macOS

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target HorizonPad_VST3
cp -R "build/HorizonPad_artefacts/Release/VST3/Horizon Pad.vst3" ~/Library/Audio/Plug-Ins/VST3/
```

For a universal binary add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

### Windows

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --target HorizonPad_VST3 --config Release
```

Then copy the `.vst3` folder to `C:\Program Files\Common Files\VST3\`.

### Faster iteration with a local JUCE checkout

```bash
cmake -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE
```

---

## Validating

Per the project's VST3 playbook, run Steinberg's `validator` after any
structural change:

```bash
git clone --depth 1 https://github.com/steinbergmedia/vst3sdk.git
cd vst3sdk
git submodule update --init --depth 1 -- base cmake pluginterfaces public.sdk
cd ..

cmake -B vst3sdk-build -S vst3sdk -DCMAKE_BUILD_TYPE=Release \
  -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF \
  -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF \
  -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=OFF
cmake --build vst3sdk-build --target validator

./vst3sdk-build/bin/Release/validator \
  "build/HorizonPad_artefacts/Release/VST3/Horizon Pad.vst3"
```

**Current status:** the plugin builds warning-clean and passes the validator with
**47 tests passed, 0 failed** (verified on Linux/x86-64, GCC 13, Release). The
validator emits one benign warning — the module ships both a `moduleinfo.json`
and an exported `IPluginCompatibility` class, and prefers the former; this is
JUCE 8 default behaviour and not a defect in this plugin.

### Known gaps

- The validator result above was produced on **Linux**. CI runs it on macOS,
  where it has not yet been confirmed green — which is why the validator job in
  CI is marked `continue-on-error: true` for now. Promote it to a hard gate once
  it has been observed passing on the macOS runner.
- The per-layer **solo ("S")** button is presentational. Solo is not a host
  parameter and there is no per-layer mute in the DSP yet; toggling it only
  marks the panel. Wiring it up is a planned follow-up.
- The **PITCH** and **MOD** graphs are visualisers, not editors. They react to
  the plugin's output level but are not bound to automatable parameters — there
  are no pitch/mod parameters in the eight-parameter spec.
- 64-bit (double) audio processing is not supported; the validator reports this
  as information, not a failure.

---

## Continuous integration

[`.github/workflows/build.yml`](.github/workflows/build.yml) builds the VST3 on
macOS and Windows runners and uploads each as an artefact, then runs the
Steinberg validator against the macOS build in a separate, non-blocking job.

---

## Repository layout

```
CMakeLists.txt                     JUCE CMake API, FetchContent, juce_add_plugin
Source/
  PluginProcessor.{h,cpp}          APVTS, voice allocation, state, programs
  PluginEditor.{h,cpp}             Layout; scales a fixed 1120x780 design surface
  dsp/
    ToneState.h                    Lock-free tone block + unit mappings
    LayerBase.h, LayerBase.cpp     Shared voice/envelope/filter machinery
    WarmPadLayer.{h,cpp}
    AnalogStringsLayer.{h,cpp}
    GranularTextureLayer.{h,cpp}
    SubPadLayer.{h,cpp}
    FxChain.{h,cpp}                Filter -> delay -> reverb + FX Amount macro
  presets/
    Presets.{h,cpp}                Six factory programs + parameter IDs
  gui/
    HorizonLookAndFeel.{h,cpp}     Palette, typography, custom rotary
    HeaderBar.{h,cpp}              Logo, wordmark, nav, badges, preset field
    BannerView.{h,cpp}             Coded sunset gradient + mountain silhouettes
    LayerPanel.{h,cpp}             Per-layer accent, swatches, four knobs
    GraphView.{h,cpp}              PITCH / MOD line graphs
    GlobalParamsPanel.{h,cpp}      Reverb / Delay / Filter / FX Amount + icons
    PresetBrowser.{h,cpp}          Preset list, swatch, description, arrows
.github/workflows/build.yml        Build matrix + validator job
```

---

© Quelle Music. All rights reserved.

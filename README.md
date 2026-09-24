# Horizon Pad

**Quelle Music — Horizon Pad v1.0.0**

A four-layer ambient pad synthesiser, built with JUCE and shipped as a VST3
instrument (a Standalone build is also produced for quick testing outside a
host).

Everything Horizon Pad makes is synthesised from scratch in DSP code. There is
no sample content, no audio files and no bitmap artwork anywhere in this
repository — the oscillators, the shimmer/pitch-shift engine and the entire
user interface (including the sunset banner and the mountain range) are
generated in code.

---

## The four layers

| # | GUI name | Class | Synthesis |
|---|----------|-------|-----------|
| 1 | **Root** | `WarmFoundationLayer` | Three detuned triangle oscillators + a band-limited (polyBLEP) saw edge + a sub-oscillator one octave down, run through a lowpass filter with a slow "breathing" cutoff modulation |
| 2 | **Clearing** | `AnalogEnsembleLayer` | A multi-oscillator unison stack summed with its own output through an LFO-swept delay line (2–11 ms, 0.11 Hz), i.e. a chorus/flanger built from the sweep itself rather than a fixed comb, into a lowpass |
| 3 | **Expanse** | `AiryChoirLayer` | A detuned oscillator stack through a swept bandpass, with a parallel shimmer send: highpass → a two-grain +1-octave pitch shifter (`OctaveShimmer`, the C++ equivalent of a granular `transpose`) → its own small reverb, blended back in |
| 4 | **Bloom** | `MotionPadLayer` | A detuned oscillator stack through a lowpass with an LFO-modulated cutoff and a tremolo, for a pad that visibly "breathes"/moves over time |

All four layers respond to MIDI note on/off with shared voice allocation (up
to 8 voices, three-tier stealing: free slot → oldest released-but-ringing
slot → oldest held note), so a chord sounds coherent across every layer.
Attack/release are long by design — this is an ambient pad, not a lead — but
the ATTACK/RELEASE macros can pull them down into a fast, click-free,
percussive-pad range too (see below).

Each layer also has its own **stereo width**: a short (≤90-sample)
Haas-style delay tap on one channel, alternated left/right per layer so the
four layers' precedence-effect pulls roughly cancel across the mix. A
width-proportional dry-signal blend is mixed back into the delayed channel
so a full-width setting never produces a true null when the mix is summed to
mono (a real hazard with an unmitigated equal-gain delay tap) — see the
comment above the width block in `Source/dsp/LayerBase.h` for the numbers.

## Global FX chain

Post layer-mix: a single shared **reverb send** (`Source/dsp/FxChain.{h,cpp}`,
`juce::dsp::Reverb`, driven genuinely in stereo). REVERB (0–100%) is the wet
send level; raising it also trims the dry level slightly (0.85× down to
~0.60×) so it reads as a wet/dry blend rather than a pure loudness increase.
A final `tanh` soft clip on the output is a safety net for pathological
automation, not part of the normal sound (headroom is trimmed well ahead of
it — factory presets peak in the −9 to −21 dB range on a 4-note chord).

There is no separate delay or global filter stage — FILTER is a macro that
scales each layer's own already-envelope-modulated cutoff curve instead (see
Parameters below), and each layer's own oscillator/filter chain provides its
own movement.

---

## Parameters

Twelve host-automatable parameters (all 0–100%), in this fixed order — four
volumes, then four macros, then four widths — so that a controller mapping
its first 8 knobs to a plugin's first 8 host parameters (e.g. a Launchkey in
Live's generic Device-knob mode) lands knobs 1–4 on the pad volumes and
knobs 5–8 on the macros:

| # | Parameter | ID |
|---|-----------|-----|
| 1 | Root volume | `rootVolume` |
| 2 | Clearing volume | `clearingVolume` |
| 3 | Expanse volume | `expanseVolume` |
| 4 | Bloom volume | `bloomVolume` |
| 5 | Attack | `attackMacro` |
| 6 | Release | `releaseMacro` |
| 7 | Filter | `filterMacro` |
| 8 | Reverb | `reverbMacro` |
| 9 | Root width | `rootWidth` |
| 10 | Clearing width | `clearingWidth` |
| 11 | Expanse width | `expanseWidth` |
| 12 | Bloom width | `bloomWidth` |

- **Attack / Release** scale each layer's own designed attack/release time.
  50% is unity; below ~15% the mapping tapers exponentially down to a fast,
  click-free ~10–25 ms floor instead of just halving the (multi-second)
  baseline, so the far left of the knob is a genuinely fast/percussive
  setting, not just "a bit faster."
- **Filter** scales each layer's own cutoff-over-time curve (darker below
  50%, brighter above), ramped per sample rather than stepped once per
  block, so sweeping it under host automation doesn't zipper.
- **Reverb** is the shared reverb send level (see FX chain above).

All twelve parameters are smoothed (a `SmoothedValue` ramp, not a raw value
applied instantaneously) somewhere on their path to audio, to avoid zipper
noise under host automation.

### Performance controls (not host parameters)

**PITCH** and **MOD**, matching a real keyboard's wheels: on-screen
draggable, and also driven by incoming MIDI pitch-bend / mod wheel (CC1).
PITCH is spring-loaded (snaps back to centre on release); MOD stays wherever
it's left. These are intentionally *not* automation lanes — see
`Source/dsp/PerformanceState.h`.

### A/B buffers and user presets

Two independent snapshots (**A** / **B**) of all twelve parameters, switchable
and copyable from the preset row — handy for comparing a tweak against where
you started. Beyond the factory presets below, **"+ Save preset"** writes to
a small on-disk library (`UserPresets.xml` next to the plugin's app-data
folder) shared across every instance of the plugin, independent of the
host's own per-track program list.

---

## Factory presets

**18** factory programs (`Source/presets/Presets.cpp`), spanning lush/ambient,
dark/brooding, bright/shimmering, movement/evolving, minimal/sparse and
big/cinematic character.

The bank was curated down from an earlier 30 on character rather than on
level — it held twelve near-duplicates — and then loudness-matched to
−18.0 LUFS. 17 of the 18 sit within ±1 LU of that; the exception is recorded
as EX-002 in [`docs/exceptions.md`](docs/exceptions.md). Method and the
survivor covering each cut are in
[`docs/preset-curation-2026-09-23.md`](docs/preset-curation-2026-09-23.md).

Every preset is checked by the test suite (`all_presets_safe`) for clipping,
NaN/Inf and peaks above 0 dBFS on an eight-note chord.

| # | Preset | Character |
|---|--------|-----------|
| 1 | Lagerfeuer | Warm, close and grounded - the campfire pad |
| 2 | Alpenglühen | Warm light spreading wide across the peaks |
| 3 | Morgentau | Fresh and delicate, open but soft |
| 4 | Sternenzelt | Vast and celestial - Expanse fills the whole sky |
| 5 | Talwind | Movement and breeze - Bloom leads the way |
| 6 | Mitternachtsblau | A deep midnight drone, barely lit |
| 7 | Bergecho | A vast mountain echo - huge, cinematic space |
| 8 | Steinerne Ruhe | Stillness carved in stone - minimal, slow and sparse |
| 9 | Goldstaub | Golden dust catching the light - bright and airy |
| 10 | Tiefensog | A deep pull from below - sub-heavy and dark |
| 11 | Lichtnebel | Soft, bright fog - gentle and balanced |
| 12 | Sturmfront | A dramatic storm front rolling in - big and wide |
| 13 | Dämmerlicht | Warm dusk light, gently settling |
| 14 | Frostklang | Cold, bright and sharp - a frozen ring |
| 15 | Kupferglanz | Warm copper shine - mid-bright and present |
| 16 | Sternenstaub | Shimmering stardust - restless and bright |
| 17 | Ruhepuls | A slow resting pulse, with subtle motion underneath |
| 18 | Klarheit | Clear, present and simple - a mix-friendly starting point |

Each preset's full twelve-value parameter set lives in
[`Source/presets/Presets.cpp`](Source/presets/Presets.cpp).

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

Or, for local dev iteration, `tools/install_plugin.sh` builds a Debug config
and copies it straight to `~/Library/Audio/Plug-Ins/VST3/` (the per-user
folder every major DAW, including Ableton Live, scans — no admin privileges
or password prompt needed).

The `.vst3` is a **universal binary** (arm64 + x86_64 in one bundle) by
default — CMake sets `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"` automatically
unless you override it. For a faster single-arch dev build, pass your own
architecture explicitly, e.g. `-DCMAKE_OSX_ARCHITECTURES="arm64"`.

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

### Offline DSP sanity checks

`HorizonPadSoundTool` (`cmake --build build --target HorizonPadSoundTool`)
renders the real DSP offline — no audio device, no host, no editor — and
prints peak/RMS/dB, NaN/Inf/clipping detection, DC offset, envelope and
spectral analysis as JSON. Run `./build/HorizonPadSoundTool --help` for the
full flag list (choosing a preset, soloing a layer, overriding any parameter,
rendering at any sample rate/block size, writing a WAV for manual listening).
This is the fastest way to verify a DSP change didn't introduce clipping,
NaN/Inf, or a stepping/zipper artifact, without opening a DAW.

---

## Testing and validating

The project runs at playbook **Tier P**; [`docs/gate-status.md`](docs/gate-status.md)
is the one-page view of where each gate stands, and the artefacts sit beside it
in [`docs/`](docs/).

### Test suite

```bash
cmake --build build-release --target HorizonPadSoundTool -j 8
ctest --test-dir build-release --output-on-failure
```

Eight tests, about 30 seconds, driving the real plugin DSP through
`HorizonPadSoundTool` ([`tools/tests/dsp_tests.py`](tools/tests/dsp_tests.py)).
Five assert end-to-end properties — zero reported latency, bit-identical
renders from a seeded reset, all 36 sample-rate × block-size combinations
clean, no NaN or denormal storm after 60 s of silence, all 18 factory presets
safe. Three are regression guards, one per bug that measurement caught:

| Test | Guards against |
|---|---|
| `shimmer_octave_present` | Expanse's +1 octave shimmer silently not transposing |
| `width_is_rate_invariant` | WIDTH's Haas delay drifting back to a sample count |
| `voice_steal_declick` | Voice stealing cutting a sounding voice mid-cycle |

The suite needs `python3` with `numpy` and `scipy`. Without them CMake skips
test registration, so a plain plugin build never fails for want of them.

### Measurement tools

[`tools/measure/`](tools/measure/) holds the G3/G5 measurement scripts — alias
floor by sample-rate comparison, the metric battery, reverb decay per octave
band, and the port-vs-prototype null test. Each prints a markdown table and is
runnable on its own.

### Plugin-format validators

Both pass against the current build (2026-09-24):

```bash
# pluginval — strictness 10, exit 0
pluginval --strictness-level 10 --validate "build-release/HorizonPad_artefacts/Release/VST3/Horizon Pad.vst3"

# Steinberg validator — 47 tests passed, 0 failed
git clone --depth 1 https://github.com/steinbergmedia/vst3sdk.git
cd vst3sdk && git submodule update --init --depth 1 -- base cmake pluginterfaces public.sdk && cd ..
cmake -B vst3sdk-build -S vst3sdk -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF \
  -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=OFF
cmake --build vst3sdk-build --target validator
./vst3sdk-build/bin/Release/validator "build-release/HorizonPad_artefacts/Release/VST3/Horizon Pad.vst3"
```

On a Mac with only the Command Line Tools installed, the SDK's configure step
fails its Xcode version check. Add `-DXCODE_VERSION=16.0` and set
`XCODE_VERSION=16.0` in the environment — it needs both, the cache variable
for the comparison and the environment variable to skip the `xcodebuild`
probe.

### The Faust prototype

`sound design/four_pads.dsp` is the sound-design source of truth the C++ was
ported from, and the layer classes cite it by name. It builds into an offline
renderer that needs no audio device:

```bash
./tools/faust_render/build.sh
./build/faust/four_pads_render --dur=10 --gate-off=4 --freq=261.63 --out=blend.wav
```

`sound design/g5_layers.dsp` exposes the same four layers dry, one per output
channel, for the null test. It imports them with Faust's `library()` primitive
rather than copying them, so it cannot drift from the prototype.

### Known gaps

- **Three hosts, two platforms is 1 of 3.** Ableton Live on macOS is
  confirmed. Windows Ableton and Waveform are outstanding — the last open
  item in G6.
- **The CPU budget is measured but not met.** 5.03% of one core on an Apple
  M3 Pro, against a budget written for a 2017-era dual-core i5 where the same
  load would plausibly be 15–20%. See A-004 in
  [`docs/brief.md`](docs/brief.md).
- **Nothing has been listened to since 2026-09-23**, and several
  measured-but-unheard changes have landed since — above all Expanse's
  shimmer, which now transposes and did not before. The full list is in
  [`docs/gate-status.md`](docs/gate-status.md).
- **Two open exceptions**, both accepted with written triggers: EX-002
  (Sternenzelt sits below the loudness-matched bank) and EX-003 (Expanse
  aliases at MIDI 96–108). See [`docs/exceptions.md`](docs/exceptions.md).
- **Code-signing / notarization**: the macOS `.pkg` is unsigned (no Apple
  Developer ID on this project) — Gatekeeper will warn on first launch (see
  Installing below for the workaround). This needs the project owner's own
  Apple Developer credentials to resolve; out of scope for a code change.
- **Fixed-size window only** (`setResizable(false, false)`); every child
  paints its own precise layout rather than scaling a "design surface", so
  there's no benefit to resizing without a real layout-scaling pass first.
- 64-bit (double-precision) audio processing is not implemented —
  `supportsDoublePrecisionProcessing()` is not overridden, so it defaults to
  `false`; hosts running a double-precision graph will process this plugin
  in single precision.

---

## Installing

Prebuilt installers are produced by CI on every push to `main` (see
[Continuous integration](#continuous-integration) below) - download them from
the latest successful **Build** workflow run's Artifacts.

### macOS - `Horizon Pad.pkg`

Double-click and follow the prompts; it installs to
`/Library/Audio/Plug-Ins/VST3/Horizon Pad.vst3` (system-wide, so it asks for
your password). The package is **unsigned and not notarized** - there's no
Apple Developer ID on this project yet - so Gatekeeper will warn on first
launch.

What that warning looks like depends on the macOS version:

- **If a dialog offers "Open" or "Open Anyway" directly** (older macOS): click
  it and continue.
- **If the dialog only offers "Move to Trash" and "Cancel"** (current macOS -
  Sonoma/Sequoia and later removed the direct bypass button): click
  **Cancel** - do *not* move it to the Trash - then go to
  **System Settings > Privacy & Security**, scroll down to the "Security"
  section, and you'll see *"Horizon Pad.pkg" was blocked to protect your Mac*
  with an **Open Anyway** button next to it. Click it, confirm once more in
  the popup that appears, then double-click the `.pkg` again to install.

The CI artifact ships `INSTALL-INSTRUCTIONS.txt` right next to the `.pkg`
with these same steps spelled out - handy since it's a plain text file with
no Gatekeeper warning of its own, so it's always readable even before you've
gotten past the warning on the `.pkg` itself.

To uninstall, delete
`/Library/Audio/Plug-Ins/VST3/Horizon Pad.vst3` yourself - `pkgbuild`
packages don't register an uninstaller.

The `.pkg` carries a `postinstall` script (`packaging/macos/scripts/postinstall`)
that runs on your Mac, with the same admin privileges as the install itself,
right after Installer.app copies the plugin into place. It re-signs the
installed bundle ad-hoc and verifies it (the true last step of the whole
distribution chain, after every possible copy/zip/extract along the way),
and - importantly - exits with a failure if anything is missing or invalid.
Installer.app only shows "The installation was successful" when that script
actually confirms the plugin landed correctly; a broken install now shows a
real failure dialog instead of a false "success".

To build the `.pkg` yourself from a local build:

```bash
packaging/macos/build-pkg.sh "build/HorizonPad_artefacts/Release/VST3/Horizon Pad.vst3" dist
```

### Windows - `install.bat`

Unzip the Windows installer package (it contains `Horizon Pad.vst3`,
`install.bat` and `uninstall.bat` together - keep them side by side) and
double-click `install.bat`. It self-elevates (one UAC prompt) and copies the
plugin to `C:\Program Files\Common Files\VST3\`. Run `uninstall.bat` the same
way to remove it. There's no signed installer here either - Windows
SmartScreen may still show a warning the first time; click **More info > Run
anyway**.

### Both platforms at once

The `HorizonPad-CrossPlatform-Release` CI artifact bundles the macOS `.pkg`
and the Windows installer folder together in one zip, for handing a single
file to someone who might be on either OS.

---

## Continuous integration

[`.github/workflows/build.yml`](.github/workflows/build.yml):

1. **`build`** - builds the VST3 on macOS (universal binary) and Windows
   (x64) runners, uploads each as a raw artefact.
2. **`package-macos`** - turns the macOS build into an unsigned `.pkg`
   (`packaging/macos/build-pkg.sh`).
3. **`package-windows`** - bundles the Windows build with
   `packaging/windows/install.bat` / `uninstall.bat` into an installer
   folder.
4. **`package-release`** - zips both installers together into one
   `HorizonPad-CrossPlatform-Release` artifact.
5. **`validate`** - runs the Steinberg validator against the macOS build, in
   parallel, non-blocking (`continue-on-error: true`).

---

## Repository layout

```
CMakeLists.txt                     JUCE CMake API, FetchContent, juce_add_plugin
Source/
  PluginProcessor.{h,cpp}          APVTS, voice allocation/stealing, state,
                                    A/B buffers, programs
  PluginEditor.{h,cpp}             Fixed 1080x748 layout, pixel-accurate to
                                    the design handoff
  dsp/
    HorizonTypes.h                 kNumLayers/kMaxVoices/kNumGlobalParams etc.
    PerformanceState.h             Lock-free PITCH/MOD (not host parameters)
    LayerBase.{h,cpp}              Shared voice/envelope/filter/width machinery
    WarmFoundationLayer.{h,cpp}    Root
    AnalogEnsembleLayer.{h,cpp}    Clearing
    AiryChoirLayer.{h,cpp}         Expanse
    MotionPadLayer.{h,cpp}         Bloom
    OctaveShimmer.h                Two-grain +1-octave pitch shifter (Expanse's shimmer send)
    FxChain.{h,cpp}                Shared stereo reverb send
  presets/
    Presets.{h,cpp}                18 factory programs + parameter IDs
    UserPresetStore.{h,cpp}        On-disk user preset library (message-thread only)
  gui/
    HorizonLookAndFeel.{h,cpp}     Palette, typography, shared panel/card painters
    TitleBanner.{h,cpp}            Logo, wordmark
    PresetBar.{h,cpp}              Factory + user presets, save flow, A/B buffers
    PadKnob.{h,cpp}                One layer's VOL/WIDTH knob pair
    MacrosPanel.{h,cpp}            ATTACK/RELEASE/FILTER/REVERB
    WheelSlider.{h,cpp}            PITCH/MOD wheels
    OutputMeter.{h,cpp}            Live output level (read-only)
    FooterBar.{h,cpp}              Bottom strip: wordmark, tagline, buffer/layer/macro counts
tools/
  install_plugin.sh                Local dev build+install to ~/Library/Audio/Plug-Ins/VST3
  sound_tool/Main.cpp              HorizonPadSoundTool - offline DSP render+analysis CLI
  tests/dsp_tests.py               The CTest suite (G6)
  measure/                         G3/G5 measurement scripts
    alias_check.py                 Alias floor, by 48 vs 192 kHz comparison
    g3_metrics.py                  Rate/block matrix, CPU, envelope, denormals
    reverb_edr.py                  Reverb decay per octave band
    null_test.py                   Port vs prototype, per layer
  faust_render/                    Offline renderer for the Faust prototype
    render_arch.cpp                Faust architecture file: headless WAV out
    build.sh                       faust + c++ -> build/faust/*_render
sound design/
  four_pads.dsp                    The Faust prototype - sound-design source of truth
  g5_layers.dsp                    The same four layers, dry, for the null test
  Horizon_Pad_VST3_Handoff.md      Control-to-GUI mapping from the original handoff
  reference-renders/               Signed-off renders: per layer + the full blend
docs/
  gate-status.md                   One page: where every Tier P gate stands
  brief.md                         G0 sonic brief + dated amendments
  g1-algorithm.md                  G1 algorithm audit against the playbook
  g2-prototype.md                  G2 prototype artefact
  g3-measurements.md               G3 metric table
  g5-null-test.md                  G5 port-vs-prototype characterisation
  exceptions.md                    Playbook 12.5 exception log
  preset-curation-2026-09-23.md    How the bank went from 30 to 18
  dsp-audit-2026-09-20.md          DSP invariant audit
packaging/
  macos/build-pkg.sh               Builds the unsigned .pkg installer
  macos/scripts/postinstall        Runs on-Mac after install: re-signs + verifies
  macos/INSTALL-INSTRUCTIONS.txt   Gatekeeper walkthrough, shipped beside the .pkg
  windows/install.bat              Self-elevating installer (Common Files\VST3)
  windows/uninstall.bat            Self-elevating uninstaller
.github/workflows/build.yml        Build matrix, installer packaging, validator job
```

---

© Quelle Music. All rights reserved.

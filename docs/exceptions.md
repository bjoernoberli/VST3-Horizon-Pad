# Exception log

Playbook 12.5: a failed measurement is not negotiable by the person who wrote the
code. Either fix it, or record a written, dated, named exception stating **what
failed, by how much, why shipping anyway is acceptable, and what would make it
unacceptable.**

Entries are append-only. Closing one means fixing the underlying failure, not
deleting the entry.

---

## EX-001 - DC offset above the -80 dBFS target when the output limiter engages

- **Date:** 2026-09-23
- **Raised by:** Claude, during the G4 output-stage re-stage
- **Rule / target:** Invariant #6 (DC blocker after asymmetric nonlinearities) and
  playbook 4.4's DC offset target of < -80 dBFS
- **Status:** CLOSED 2026-09-23 - fixed, see "Resolution" below

### What failed, and by how much

The re-staged output limiter ([PluginProcessor.cpp:735](../Source/PluginProcessor.cpp))
is odd-symmetric, but the summed four-layer signal is not, so when the limiter
engages it shaves asymmetric peaks asymmetrically and leaves a DC residue.

Measured with `HorizonPadSoundTool` (Release), 8 notes at velocity 1.0, all four
volumes, all four widths, FILTER and REVERB at 100%, 96 kHz, block 13:

| Volumes | Peak | DC offset | Limiter |
|---|---|---|---|
| 1.0 | -0.04 dBFS | **-92.4 dBFS** | engaged |
| 0.5 | -2.24 dBFS | -182.8 dBFS | at the knee |
| 0.25 | -6.93 dBFS | -128.2 dBFS | not engaged |

Across render lengths at volumes 1.0 the figure wanders between **-70.2 and
-92.3 dBFS** - it does not average away with a longer window, so it is a real
residue and not a measurement artefact. The worst observed, -70.2 dBFS, is
**9.8 dB above** the -80 dBFS target.

### Why shipping this way is acceptable for now

- It requires all twelve parameters at or near maximum with eight voices sounding.
  No factory preset engages the limiter at all: across 63 renders of the seven
  loudest presets in three voicings, the worst true peak was -2.64 dBTP, 0.7 dB
  below the knee.
- At -70 to -92 dBFS the offset is inaudible and consumes no meaningful headroom.
- The obvious fix - a DC blocker on the output - is not free: this instrument has a
  sub-oscillator an octave below the played note, so at MIDI 21 there is real
  musical content at 13.75 Hz. A highpass placed to remove DC sits close enough to
  that to deserve a listening pass rather than a blind patch.

### What would make it unacceptable

- Any factory preset reaching the limiter knee in normal playing (re-check after
  the preset loudness-matching pass - that pass raises quiet presets).
- The offset exceeding -60 dBFS under any setting.
- A user report of a click on host mute/unmute or on bounce boundaries, which is how
  output DC actually becomes audible.
- Stacking instances: DC sums coherently across instances, so a worship rig with
  four of them turns -70 dBFS into -58 dBFS.

### Why it was fixed rather than carried

The output gain was raised a further 2.5 dB when the factory bank was
loudness-matched later the same day, which drove the limiter harder and took the
offset to **-58.6 dBFS at 96 kHz** - past this entry's own stated trigger of
"exceeding -60 dBFS under any setting". The exception had invalidated itself, so
12.5 left only one option: fix it.

### Resolution

A one-pole DC blocker at 2 Hz, sample-rate aware, placed after the limiter because
the limiter is what creates the offset. State lives per channel and is cleared in
both `prepareToPlay` and `releaseResources` (invariant #12).

**The first attempt made things worse and is worth recording.** With the blocker
alone, DC fell to -147 dBFS but peaks rose to **+0.26 to +0.52 dBFS** and the
renders reported true clipping. The blocker subtracts a slowly-varying *local* mean,
so on asymmetric material it pushes a sample the limiter had just placed at the
ceiling back over it - the fix had broken the one guarantee the output stage exists
to make. A hard clamp at kCeiling was added as the final step.

Measured after the full chain (limiter -> DC blocker -> clamp), same pathological
extreme as the table above. **Six runs per configuration**: this metric swings
20-40 dB run to run, because each voice seeds its oscillator phase from system
entropy and the waveform's asymmetry - the thing the limiter acts on - changes with
it. A single render of it is not a result.

| Sample rate / block | Peak | DC worst of 6 | DC median | Clipping |
|---|---|---|---|---|
| 48 kHz / 512 | -0.04 dBFS | -87.1 dBFS | -92.3 dBFS | none |
| 96 kHz / 13 | -0.04 dBFS | **-80.3 dBFS** | -89.6 dBFS | none |

**This is a pass, but a marginal one - state it that way.** The worst observed run
sits at -80.3 dBFS against a -80 dBFS target, i.e. on the line, not inside it. What
the fix bought is a 22 dB improvement on the worst case (-58.6 -> -80.3 dBFS) and
the removal of the trigger condition that invalidated the original exception.

The clamp is why it is not better than that: it reintroduces a little DC of its own,
because asymmetric clipping does. Moving the blocker's corner up would buy a few
more dB (roughly 6 dB at 4 Hz) at a cost of -0.35 dB on the deepest sub instead of
-0.09 dB. That trade was not taken: the residue only exists at the single most
pathological setting in the product, -80 dBFS is inaudible, and playbook 7 is
explicit that the last 20-40 dB of a measurement is where effort stops paying.

On any factory preset or the default patch the figure is -140 dBFS or lower, because
none of them reach the limiter at all.

**Still owed:** a listening pass on the lowest octave. The blocker is -0.09 dB at
13.75 Hz and -0.04 dB at 20 Hz, so it should be inaudible, but it is a filter in the
output path of an instrument with a sub-oscillator and that claim is calculated, not
heard.

---

## EX-002 - Sternenzelt sits ~4.8 LU below the loudness-matched bank

- **Date:** 2026-09-23
- **Raised by:** Claude, during the G7 preset loudness-matching pass
- **Rule / target:** Playbook 5.3 - "every preset checked at the same output
  loudness"; brief target of +/-1 LU
- **Status:** OPEN - accepted for now, one-line fix available if the owner prefers it

### What failed, and by how much

17 of the 18 factory presets match to **-18.0 LUFS within +/-1 LU** (measured
spread 1.04 LU across three voicings spanning the worship-keys register).
**Sternenzelt lands at about -22.8 LUFS, roughly 4.8 LU below the rest.**

That figure needs a qualifier: Sternenzelt is also the least repeatable preset in
the bank. Over 4 renders per voicing its per-render standard deviation reaches
1.4 LU, against 0.2-0.8 LU for a stable preset like Klarheit or Morgentau, and its
loudness varies 3.9 LU across the three voicings against ~1.1 LU for those two. A
single measurement of it can land anywhere in a ~3 LU window, so any number quoted
for this preset is a mean, not a value.

It cannot be raised. Preset loudness is set by the four pad volumes, which are
capped at 1.0, and Sternenzelt is quiet precisely because one layer does nearly all
the work: its balance is 0.10 / 0.10 / 0.62 / 0.17, with Expanse already at 0.90 of
full. That left **+0.92 dB** of available gain before clamping, against the ~4.8 dB
it needed. The volumes are now scaled to that maximum - Expanse sits at 1.000 - so
there is nothing left to give.

### Why shipping this way is acceptable

The alternative was measured, not assumed. Forcing Sternenzelt into the match means
making it the target, which drags every other preset down by a further **8.3 dB**,
with volume scale factors reaching x0.38. That trades one quiet preset for a quiet
instrument.

Sternenzelt is also the patch where this matters least: "Vast and celestial -
Expanse fills the whole sky" is an atmospheric wash, not a preset anyone plays a
verse on. The instability above is itself part of the reason: so little of it is
fundamental that "how loud is Sternenzelt" does not have one answer to match to.

### What would make it unacceptable

- A second preset needing the same exemption - two is a pattern, and the fix then
  belongs in the gain structure rather than in an exception.
- User reports of reaching for the volume knob every time they load it.
- Any future preset whose character depends on a single layer near 1.0, which would
  hit the same ceiling.

### The fix, if it is preferred

Rebalance Sternenzelt: raise Root, Clearing and Bloom enough to make up the 5.3 dB
while keeping Expanse at 1.0. That reaches the target, and costs some of the purity
the preset is named for - Expanse would fall from 62% of the mix to roughly half. It
would also steady the measurement, since more of the mix would be fundamental. It is a one-line change to the preset table plus a listening pass, and it is
the owner's call because it is a character decision, not a level one.

---

## EX-003 - Expanse aliases at the top two octaves

- **Date:** 2026-09-24
- **Raised by:** Claude, during the G3 alias measurement
- **Rule / target:** Brief `quality_targets.alias_floor_db`
- **Status:** OPEN - accepted, with a named fix if a listening pass disagrees

### What failed, and by how much

Measured by rendering the same seeded note at 48 kHz and at 192 kHz and
comparing spectra below 20 kHz at matched FFT bin width (see
[`g3-measurements.md`](g3-measurements.md) for why the brief's own ASR/NMR
metric does not work on this instrument).

Expanse, relative to its own peak:

| MIDI | Layer peak | Loudest alias | Absolute |
|---|---|---|---|
| 84 and below | - | -68 dB or better | below -93 dBFS |
| 96 | -41.0 dBFS | **-27.1 dB** | -68.1 dBFS |
| 108 | -56.6 dBFS | **-17.5 dB** | -74.1 dBFS |

The cause is known and is in the code: Expanse runs its oscillator stack at
`freq * 2`, an octave above the played note, and `LayerBase::triangleWave()`
is a naive triangle with no polyBLEP correction. At MIDI 108 that puts the
stack at 8372 Hz, whose upper odd harmonics fold back into the passband. No
other layer is close: Root uses the same naive triangle but at the played
pitch behind a 350-1250 Hz lowpass, and Clearing and Bloom use polyBLEP saws.

### Why shipping this way is acceptable

- **Absolute level.** -68 and -74 dBFS. In a four-layer mix where the other
  three sit near -10 dBFS in the same region, the alias is 55-65 dB down and
  inside any reasonable masking estimate.
- **The register is not played.** The target genre is worship and CCM keys.
  MIDI 96-108 is C7-C8, above the top of essentially any part this instrument
  exists to play. Everything from MIDI 21 to 84 measures -68 dB or better.
- **The layer is nearly silent there anyway.** Expanse's bandpass is fixed in
  absolute Hz and does not track the note, so at MIDI 96+ the layer produces
  -41 to -57 dBFS. The alias is loud *relative to* a layer that has almost
  stopped contributing.
- **Playbook 7 ranks this.** "Aliasing in the top two octaves: obvious to
  inaudible matters enormously; inaudible to -100 dB matters not at all."
  Nothing here is in the obvious category.

### What would make it unacceptable

- Any alias above **-60 dBFS absolute**, at any note.
- A preset or future layer that makes Expanse loud above MIDI 96 - the
  exception is written on the fact that it is not.
- Expanse's bandpass ever being made to track the note, which would move its
  passband up with the alias content.
- A listening pass hearing it. It has not been listened to; the numbers above
  are calculated, not heard.

### The fix, if it is preferred

Give `triangleWave()` a polyBLAMP correction at its two corners - the
integrated-BLEP counterpart of the existing `polyBlepSaw()`, about ten lines,
and it leaves the waveform untouched except within one sample of each corner.
It is not done here because it changes the harmonic content of both Root and
Expanse, and because Expanse has already been changed once today (the shimmer
fix in [`g5-null-test.md`](g5-null-test.md) brought its octave back 57 dB).
Two brightness changes to one layer without ears is how a pad ends up harsh.
It belongs in the same sound-design pass as the G5 items.

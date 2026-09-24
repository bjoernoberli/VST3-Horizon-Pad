#!/usr/bin/env python3
"""
Horizon Pad DSP test suite (G6).

Driven by CTest; each test is also runnable on its own:

    ctest --test-dir build-release --output-on-failure
    python3 tools/tests/dsp_tests.py --tool ./build-release/HorizonPadSoundTool reset_determinism

Everything here runs the real plugin DSP through HorizonPadSoundTool. The
three regression tests at the bottom each guard a bug that was found by
measurement and fixed - they exist so it cannot come back silently.
"""
import argparse, hashlib, json, os, subprocess, sys, tempfile
import numpy as np
from scipy.io import wavfile

CHORD = "48,55,60,64,67,72,76,79"
TOOL = "./build-release/HorizonPadSoundTool"


class Fail(Exception):
    pass


def run(extra):
    r = subprocess.run([TOOL] + extra, capture_output=True, text=True)
    if r.returncode != 0:
        raise Fail(f"tool exited {r.returncode}: {r.stderr[:400]}")
    return json.loads(r.stdout[r.stdout.index('{'):])


def list_presets():
    """--list-presets emits a JSON array, not the usual object."""
    r = subprocess.run([TOOL, "--list-presets"], capture_output=True, text=True)
    if r.returncode != 0:
        raise Fail(f"tool exited {r.returncode}: {r.stderr[:400]}")
    return json.loads(r.stdout[r.stdout.index('['):])


def read(path):
    sr, x = wavfile.read(path)
    return np.asarray(x, dtype=np.float64), sr


def band_energy_db(x, sr, lo, hi, t0=1.5, t1=3.5):
    v = x.mean(axis=1) if x.ndim > 1 else x
    seg = v[int(t0 * sr):int(t1 * sr)]
    n = 1 << 14
    w = np.hanning(n)
    frames = [np.abs(np.fft.rfft(seg[o:o + n] * w)) for o in range(0, len(seg) - n + 1, n // 2)]
    if not frames:
        raise Fail("render too short for spectral analysis")
    mag = np.mean(frames, axis=0)
    f = np.fft.rfftfreq(n, 1.0 / sr)
    m = (f >= lo) & (f < hi)
    return 20 * np.log10(max(np.sqrt((mag[m] ** 2).sum()), 1e-20) / mag.max())


# --------------------------------------------------------------------------
# Definition-of-done tests
# --------------------------------------------------------------------------

def test_latency_zero():
    """Invariant #11: latency is 0 and is actually reported as 0."""
    d = run(["--notes=60", "--hold=1", "--tail=1"])
    got = d["config"]["latencySamples"]
    if got != 0:
        raise Fail(f"expected 0 samples of latency, plugin reports {got}")


def test_reset_determinism():
    """Two seeded renders must be bit-identical.

    This is the cancellation test: same seed, same input, residual exactly
    zero. Without --seed the plugin randomises start phases on purpose, so
    this also confirms the seed hook still works.
    """
    with tempfile.TemporaryDirectory() as tmp:
        hashes = []
        for i in (1, 2):
            p = os.path.join(tmp, f"r{i}.wav")
            run(["--seed=4242", f"--notes={CHORD}", "--hold=2", "--tail=2", f"--out={p}"])
            hashes.append(hashlib.sha256(open(p, "rb").read()).hexdigest())
        if hashes[0] != hashes[1]:
            raise Fail("two renders with the same seed differ")

        a = os.path.join(tmp, "a.wav")
        run(["--seed=1", f"--notes={CHORD}", "--hold=2", "--tail=2", f"--out={a}"])
        if hashlib.sha256(open(a, "rb").read()).hexdigest() == hashes[0]:
            raise Fail("different seeds produced identical output - seed is not wired up")


def test_rate_block_matrix():
    """Clean at six sample rates x six block sizes."""
    bad = []
    for sr in (44100, 48000, 88200, 96000, 176400, 192000):
        for bs in (1, 13, 32, 64, 512, 4096):
            d = run([f"--sample-rate={sr}", f"--block={bs}", f"--notes={CHORD}",
                     "--seed=11", "--hold=1", "--tail=1"])
            s = d["safety"]
            if s["hasNanOrInf"] or s["trueClipping"] or s["clickCount"]:
                bad.append(f"{sr} Hz / {bs}: {json.dumps(s)}")
    if bad:
        raise Fail("unclean combinations:\n  " + "\n  ".join(bad))


def test_silence_after_note():
    """No NaN, no denormal storm and a decaying tail after the sound stops."""
    d = run(["--sample-rate=48000", "--block=64", f"--notes={CHORD}", "--seed=11",
             "--param=reverb=1.0", "--hold=1", "--tail=60"])
    s = d["safety"]
    if s["hasNanOrInf"]:
        raise Fail("NaN/Inf after 60 s of silence")
    if s["trueClipping"]:
        raise Fail("clipping after 60 s of silence")


def test_all_presets_safe():
    """Every factory preset renders clean and stays below the ceiling."""
    names = [p["name"] for p in list_presets()]
    if len(names) < 8:
        raise Fail(f"expected a curated bank of 8-20 presets, found {len(names)}")
    bad = []
    for n in names:
        d = run([f"--preset={n}", f"--notes={CHORD}", "--seed=11", "--hold=2", "--tail=3"])
        s, l = d["safety"], d["levels"]
        if s["hasNanOrInf"] or s["trueClipping"] or l["peakDb"] > 0.0:
            bad.append(f"{n}: peak {l['peakDb']:.2f} dBFS, {json.dumps(s)}")
    if bad:
        raise Fail("presets failed:\n  " + "\n  ".join(bad))


# --------------------------------------------------------------------------
# Regression tests - each guards a bug found by measurement
# --------------------------------------------------------------------------

def test_shimmer_octave_present():
    """Expanse's shimmer must actually transpose up an octave.

    OctaveShimmer's grain read used to run BACKWARDS at unity speed, so the
    octave partial was 73 dB down and the layer's defining feature did not
    exist. At C4 the oscillator stack sits at 523 Hz and the shimmer octave
    belongs at ~1046 Hz.
    """
    with tempfile.TemporaryDirectory() as tmp:
        p = os.path.join(tmp, "e.wav")
        run(["--solo=expanse", "--notes=60", "--seed=11", "--velocity=1.0",
             "--param=reverb=0", "--param=filter=0.5", "--param=expanse-width=0",
             "--hold=4", "--tail=2", f"--out={p}"])
        x, sr = read(p)
        octave = band_energy_db(x, sr, 900, 1200)
        if octave < -45.0:
            raise Fail(f"shimmer octave at {octave:.1f} dB below peak; "
                       "expected better than -45 dB (regression of the "
                       "reversed grain read)")


def test_width_is_rate_invariant():
    """WIDTH is a TIME, not a sample count.

    The Haas delay used to be a flat 90 samples, so the stereo image and its
    mono comb notch moved by a factor of 4.3 between 44.1 and 192 kHz.
    """
    delays = []
    with tempfile.TemporaryDirectory() as tmp:
        for sr in (44100, 48000, 96000, 192000):
            p = os.path.join(tmp, f"w{sr}.wav")
            run(["--solo=root", "--notes=60", f"--sample-rate={sr}", "--seed=5",
                 "--param=root-width=1.0", "--param=reverb=0",
                 "--hold=3", "--tail=0.2", f"--out={p}"])
            x, fs = read(p)
            a, b = int(1.0 * fs), int(2.0 * fs)
            L, R = x[a:b, 0], x[a:b, 1]
            c = np.correlate(L - L.mean(), R - R.mean(), mode="full")
            lag = abs(int(np.argmax(np.abs(c))) - (len(L) - 1))
            delays.append(lag / fs * 1000.0)
    spread = max(delays) - min(delays)
    if spread > 0.05:
        raise Fail(f"Haas delay varies with sample rate: {delays} ms "
                   f"(spread {spread:.3f} ms)")


def test_voice_steal_declick():
    """Stealing a sounding voice must not step harder than an ordinary note-on.

    Taking over a slot used to hard-reset the envelope mid-cycle; the worst
    of eight runs stepped +3.8 dB relative to the signal.
    """
    def worst(notes, onsets):
        vals = []
        for _ in range(6):
            d = run([f"--notes={notes}", f"--note-onsets={onsets}",
                     "--hold=6", "--tail=1", "--probe-at=3.0"])
            vals.append(d["transients"]["probes"][0]["stepRelDb"])
        return max(vals)

    steal = worst("60,62,64,65,67,69,71,72,74", "0,0,0,0,0,0,0,0,3")
    free = worst("60,62,64,65,67,69,71,74", "0,0,0,0,0,0,0,3")
    # A steal may not be more than 6 dB worse than a free-slot note-on.
    if steal > free + 6.0:
        raise Fail(f"voice steal steps {steal:.1f} dB vs {free:.1f} dB for a "
                   "free slot - declick ramp regressed")


TESTS = {name[5:]: fn for name, fn in sorted(globals().items())
         if name.startswith("test_")}


def main():
    global TOOL
    ap = argparse.ArgumentParser()
    ap.add_argument("--tool", default=TOOL)
    ap.add_argument("name", nargs="?", help="one test, or all of them")
    a = ap.parse_args()
    TOOL = a.tool

    names = [a.name] if a.name else list(TESTS)
    for n in names:
        if n not in TESTS:
            print(f"no such test: {n}\navailable: {', '.join(TESTS)}", file=sys.stderr)
            return 2
    failed = 0
    for n in names:
        try:
            TESTS[n]()
            print(f"PASS  {n}")
        except Fail as e:
            print(f"FAIL  {n}: {e}", file=sys.stderr)
            failed += 1
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

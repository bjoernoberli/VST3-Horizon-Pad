#!/usr/bin/env python3
"""
Sound-design descriptors for Horizon Pad: is it the sound we designed?

The rest of tools/measure and tools/tests answer "is anything broken?". This
answers the design questions the brief and the prototype header actually ask:

  grid      Where each layer sits on the brightness x attack grid the four pads
            were designed on (spectral centroid, time to -3 dB of peak), plus
            its band balance. Four pads are supposed to be four different points.
  keyboard  Loudness and centroid of each layer, solo, from C2 to C7. An
            instrument is played across its range, not at C4.
  pocket    Where the default blend's energy sits against the brief's job:
            "sit UNDER acoustic guitar, piano and group vocals".
  stereo    Inter-channel correlation and mono-sum loss per layer at WIDTH 0
            and 1, and loudness change from WIDTH 0 to 1 (rule 8). Since
            2026-10-02 WIDTH is one macro and 1 is each pad's profile maximum
            (Root 0.6, Bloom 0.9), so Root and Bloom at "w1" are not comparable
            with baselines from before that date.
  movement  Envelope modulation spectrum per layer, single note and chord:
            dominant rate and depth. Movement that cancels in chords is found here.
  presets   Loudness, centroid and band balance of every factory preset.

Every render is seeded, 48 kHz, reverb and width as the patch sets them unless a
section says otherwise, and every figure is the mean over --seeds seeds (default
4). The layers have LFOs down to 0.09 Hz whose start phase is random per voice,
so a single seed is one sample of an 11-second cycle, not a measurement
(playbook 4.10) - band levels at one seed swing by several dB. Output is one JSON object. Use --save to write a
baseline and --compare to diff against one:

    python3 tools/measure/descriptors.py --all --save docs/baselines/descriptors-main.json
    python3 tools/measure/descriptors.py --all --compare docs/baselines/descriptors-main.json

The tool path defaults to build-release/HorizonPadSoundTool (--tool to override).
Needs numpy and scipy.
"""
import argparse
import json
import os
import subprocess
import sys
import tempfile

import numpy as np
from scipy import signal
from scipy.io import wavfile

SR = 48000
SEEDS = [7]          # --seeds N uses 7, 8, ... ; slow LFOs make one seed a sample, not a measurement
LAYERS = ["root", "clearing", "expanse", "bloom"]
CHORD = "48,55,60,64"            # C3 G3 C4 E4: an open voicing a keys player uses
KEYBOARD_NOTES = [36, 48, 60, 72, 84, 96]
BANDS = [("sub", 20, 80), ("low", 80, 200), ("mud", 200, 500), ("lowmid", 500, 1000),
         ("presence", 1000, 4000), ("high", 4000, 8000), ("air", 8000, 20000)]

TOOL = None


def render(args, hold=6.0, tail=2.0, seed=7):
    fd, path = tempfile.mkstemp(suffix=".wav")
    os.close(fd)
    try:
        cmd = [TOOL, f"--hold={hold}", f"--tail={tail}", f"--seed={seed}",
               f"--sample-rate={SR}", f"--out={path}"] + list(args)
        subprocess.run(cmd, check=True, capture_output=True)
        _, x = wavfile.read(path)
    finally:
        os.remove(path)
    return x.astype(np.float64)


# --- ITU-R BS.1770-4 loudness (48 kHz coefficients) -------------------------
_K1 = ([1.53512485958697, -2.69169618940638, 1.19839281085285],
       [1.0, -1.69065929318241, 0.73248077421585])
_K2 = ([1.0, -2.0, 1.0], [1.0, -1.99004745483398, 0.99007225036621])


def lufs(x):
    y = signal.lfilter(*_K2, signal.lfilter(*_K1, x, axis=0), axis=0)
    return float(-0.691 + 10 * np.log10(np.mean(y ** 2, axis=0).sum() + 1e-20))


def window(x, t0, t1):
    return x[int(t0 * SR):int(t1 * SR)]


def spectrum(x):
    """Power spectrum averaged over the two channels, (L^2 + R^2) / 2 - NOT the
    spectrum of the mono sum. A mono sum inherits whatever comb a stereo trick
    puts in it (v1's Haas WIDTH), which made v1 look less muddy than it was.
    Mono compatibility is measured separately (section stereo)."""
    f, pl = signal.welch(x[:, 0], SR, nperseg=8192)
    _, pr = signal.welch(x[:, 1], SR, nperseg=8192)
    return f, 0.5 * (pl + pr)


def centroid(x):
    f, p = spectrum(x)
    return float((f * p).sum() / (p.sum() + 1e-30))


def bands(x):
    """Share of the layer's energy per band, dB relative to its total."""
    f, p = spectrum(x)
    total = p[(f >= 20) & (f <= 20000)].sum() + 1e-30
    return {name: round(float(10 * np.log10(p[(f >= lo) & (f < hi)].sum() / total + 1e-12)), 1)
            for name, lo, hi in BANDS}


def bands_abs(x):
    """Absolute energy per band, dBFS (mono sum). Use this, not the shares,
    to say whether a band got louder: a share rises when another band falls."""
    f, p = spectrum(x)
    df = f[1] - f[0]
    return {name: round(float(10 * np.log10(p[(f >= lo) & (f < hi)].sum() * df + 1e-20)), 1)
            for name, lo, hi in BANDS}


def averaged(fn):
    """Run fn(seed) for every seed and average the numeric leaves (dB values
    are averaged in dB). Strings and nested dicts are handled recursively."""
    results = [fn(seed) for seed in SEEDS]

    def merge(items):
        first = items[0]
        if isinstance(first, dict):
            return {k: merge([it[k] for it in items]) for k in first}
        if isinstance(first, (int, float)):
            return round(float(np.mean(items)), 2)
        return first

    return merge(results)


def envelope_db(x, win_s=0.05):
    m = x.mean(axis=1)
    w = max(1, int(win_s * SR))
    env = np.sqrt(np.convolve(m ** 2, np.ones(w) / w, mode="same") + 1e-20)
    return 20 * np.log10(env)


def attack_time(x, hold):
    """Seconds from note-on to within 3 dB of the loudest point of the hold."""
    h = envelope_db(x)[: int(hold * SR)]
    return float(np.argmax(h >= h.max() - 3.0) / SR)


def stereo(x):
    L, R = x[:, 0], x[:, 1]
    corr = float(np.corrcoef(L, R)[0, 1]) if np.std(L) > 0 and np.std(R) > 0 else 1.0
    mono = 0.5 * (L + R)
    loss = 10 * np.log10((mono ** 2).mean() / (0.5 * ((L ** 2).mean() + (R ** 2).mean())) + 1e-20)
    return round(corr, 3), round(float(loss), 2)


def modulation(x, fmin=0.4, fmax=12.0):
    """Dominant envelope-modulation rate (Hz) and its depth (% of mean level)."""
    m = x.mean(axis=1)
    w = int(0.01 * SR)
    env = np.sqrt(np.convolve(m ** 2, np.ones(w) / w, mode="same"))
    env = env / (env.mean() + 1e-20) - 1.0
    f, p = signal.welch(env, SR, nperseg=len(env))
    sel = (f > fmin) & (f < fmax)
    i = int(np.argmax(p[sel]))
    df = f[1] - f[0]
    return round(float(f[sel][i]), 2), round(float(np.sqrt(2 * p[sel][i] * df) * 100), 1)


# --- sections ---------------------------------------------------------------
def section_grid():
    out = {}
    for layer in LAYERS:
        def one(seed, layer=layer):
            hold = 8.0
            x = render([f"--solo={layer}", f"--notes={CHORD}"], hold=hold, seed=seed)
            ss = window(x, hold - 3, hold)
            return {"centroid_hz": centroid(ss), "attack_s": attack_time(x, hold),
                    "lufs": lufs(ss), "bands_db": bands(ss), "bands_dbfs": bands_abs(ss)}
        out[layer] = averaged(one)
    return out


def section_keyboard():
    out = {}
    for layer in LAYERS:
        row = {}
        for note in KEYBOARD_NOTES:
            def one(seed, layer=layer, note=note):
                x = render([f"--solo={layer}", f"--notes={note}"], hold=6.0, seed=seed)
                ss = window(x, 3, 6)
                return {"lufs": lufs(ss), "centroid_hz": centroid(ss)}
            row[str(note)] = averaged(one)
        played = [row[str(n)]["lufs"] for n in KEYBOARD_NOTES if 36 <= n <= 84]
        row["span_36_84_db"] = round(max(played) - min(played), 1)
        out[layer] = row
    return out


def section_pocket():
    def one(seed):
        hold = 8.0
        x = render([f"--notes={CHORD}"], hold=hold, seed=seed)      # the default patch
        ss = window(x, hold - 3, hold)
        return {"centroid_hz": centroid(ss), "lufs": lufs(ss), "bands_db": bands(ss), "bands_dbfs": bands_abs(ss)}
    return {"default_patch": averaged(one)}


def section_stereo():
    out = {}
    for layer in LAYERS:
        def one(seed, layer=layer):
            row, levels = {}, {}
            for width in (0.0, 1.0):
                hold = 6.0
                x = render([f"--solo={layer}", f"--notes={CHORD}", "--param=reverb=0",
                            f"--param=width={width}"], hold=hold, seed=seed)
                ss = window(x, 3, hold)
                corr, loss = stereo(ss)
                levels[width] = lufs(ss)
                row[f"w{int(width)}"] = {"corr": corr, "mono_loss_db": loss}
            row["width_level_change_db"] = levels[1.0] - levels[0.0]
            return row
        out[layer] = averaged(one)
    return out


def section_movement():
    out = {}
    for layer in LAYERS:
        def one(seed, layer=layer):
            row = {}
            for label, notes in (("note", "60"), ("chord", CHORD)):
                x = render([f"--solo={layer}", f"--notes={notes}", "--param=reverb=0",
                            "--param=width=0"], hold=10.0, seed=seed)
                rate, depth = modulation(window(x, 4, 10))
                row[label] = {"rate_hz": rate, "depth_pct": depth}
            return row
        out[layer] = averaged(one)
    return out


def section_presets():
    listing = json.loads(subprocess.run([TOOL, "--list-presets"], capture_output=True,
                                        text=True, check=True).stdout)
    presets = listing["presets"] if isinstance(listing, dict) else listing
    out = {}
    for preset in presets:
        name = preset["name"] if isinstance(preset, dict) else str(preset)
        def one(seed, name=name):
            hold = 8.0
            x = render([f"--preset={name}", f"--notes={CHORD}"], hold=hold, seed=seed)
            ss = window(x, hold - 3, hold)
            return {"lufs": lufs(ss), "centroid_hz": centroid(ss), "bands_db": bands(ss)}
        out[name] = averaged(one)
    values = [v["lufs"] for v in out.values()]
    out["_spread_lu"] = round(max(values) - min(values), 1)
    return out


SECTIONS = {"grid": section_grid, "keyboard": section_keyboard, "pocket": section_pocket,
            "stereo": section_stereo, "movement": section_movement, "presets": section_presets}


def flatten(d, prefix=""):
    for k, v in d.items():
        key = f"{prefix}.{k}" if prefix else k
        if isinstance(v, dict):
            yield from flatten(v, key)
        else:
            yield key, v


def compare(current, baseline):
    base = dict(flatten(baseline))
    lines = []
    for key, value in flatten(current):
        if key in base and isinstance(value, (int, float)) and isinstance(base[key], (int, float)):
            delta = value - base[key]
            if abs(delta) > 1e-9:
                lines.append(f"{key:60s} {base[key]:>10} -> {value:<10} ({delta:+.2f})")
    return "\n".join(lines) if lines else "no numeric differences"


def main():
    global TOOL
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    for name in SECTIONS:
        ap.add_argument(f"--{name}", action="store_true")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--tool", default="build-release/HorizonPadSoundTool")
    ap.add_argument("--seeds", type=int, default=4,
                    help="average every figure over this many seeds (default 4; see playbook 4.10)")
    ap.add_argument("--save", help="write the result JSON here")
    ap.add_argument("--compare", help="print differences against this baseline JSON")
    args = ap.parse_args()
    TOOL = args.tool
    SEEDS[:] = list(range(7, 7 + max(1, args.seeds)))

    chosen = [n for n in SECTIONS if args.all or getattr(args, n)]
    if not chosen:
        ap.error("pick at least one section, or --all")

    result = {}
    for name in chosen:
        print(f"[descriptors] {name}...", file=sys.stderr, flush=True)
        result[name] = SECTIONS[name]()

    text = json.dumps(result, indent=1)
    if args.save:
        os.makedirs(os.path.dirname(args.save) or ".", exist_ok=True)
        with open(args.save, "w") as fh:
            fh.write(text + "\n")
    if args.compare:
        with open(args.compare) as fh:
            print(compare(result, json.load(fh)))
    else:
        print(text)


if __name__ == "__main__":
    main()

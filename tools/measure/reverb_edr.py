#!/usr/bin/env python3
"""
Reverb decay character - the measurement that decides G1 item #7.

Playbook 6.4 wants a 16-line FDN for a big reverb and names comb-based
Schroeder (which juce::dsp::Reverb / Freeverb is) as the thing not to do. The
deviation was accepted at G1 on CPU and material grounds, on condition that it
be settled with numbers rather than argument. These are the numbers.

What is measured: per-octave-band energy decay (EDR) of the tail after
note-off, and from it a T60 per band. Freeverb's failure mode is a comb
signature - one or two bands ringing markedly longer than their neighbours,
heard as a metallic tail. An even spread of T60 across bands means the engine
is behaving, whatever its topology.

Worst case is chosen deliberately: ATTACK fully left, which the macro maps to
1% of each layer's designed time (12-26 ms), because a comb reverb is exposed
by transients and nothing else in this instrument produces one.
"""
import subprocess, tempfile, os, sys
import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfiltfilt

TOOL = "./build-release/HorizonPadSoundTool"
BANDS = [(63, 125), (125, 250), (250, 500), (500, 1000),
         (1000, 2000), (2000, 4000), (4000, 8000)]


def t60_from_decay(env_db, sr):
    """Schroeder backward integration, fitted over -5 .. -25 dB."""
    top = np.argmax(env_db)
    e = env_db[top:]
    try:
        i5 = np.where(e <= e[0] - 5)[0][0]
        i25 = np.where(e <= e[0] - 25)[0][0]
    except IndexError:
        return None
    if i25 <= i5 + 10:
        return None
    n = np.arange(i5, i25)
    slope = np.polyfit(n / sr, e[i5:i25], 1)[0]     # dB per second
    return -60.0 / slope if slope < 0 else None


def main():
    with tempfile.TemporaryDirectory() as tmp:
        wav = os.path.join(tmp, "tail.wav")
        subprocess.run([TOOL, "--notes=48,55,60,64", "--seed=11", "--velocity=1.0",
                        "--param=reverb=1.0", "--param=attack=0.0", "--param=release=0.0",
                        "--hold=1.0", "--tail=12", "--sample-rate=48000",
                        f"--out={wav}"], check=True, capture_output=True)
        sr, x = wavfile.read(wav)
        x = np.asarray(x, dtype=np.float64)
        mono = x.mean(axis=1) if x.ndim > 1 else x

    # The tail proper: everything after note-off at 1.0 s.
    tail = mono[int(1.05 * sr):]

    print("# Reverb decay character (REVERB=1, ATTACK fully left, 4-note chord)\n")
    print("| Octave band | T60 |")
    print("|---|---|")
    t60s = []
    for lo, hi in BANDS:
        sos = butter(4, [lo / (sr / 2), min(hi / (sr / 2), 0.999)], btype="band", output="sos")
        b = sosfiltfilt(sos, tail)
        # Schroeder backward integration of the squared envelope.
        e = np.cumsum(b[::-1] ** 2)[::-1]
        e = e / max(e[0], 1e-30)
        edb = 10 * np.log10(np.maximum(e, 1e-30))
        t = t60_from_decay(edb, sr)
        t60s.append(t)
        print(f"| {lo}-{hi} Hz | " + (f"{t:.2f} s |" if t else "not resolvable |"))

    good = [t for t in t60s if t]
    if len(good) >= 3:
        spread = max(good) / min(good)
        print(f"\n- T60 range across bands: **{min(good):.2f} - {max(good):.2f} s**")
        print(f"- Ratio longest/shortest: **{spread:.2f}x**")
        print("\nA comb-filter signature shows up as one band ringing far longer "
              "than its neighbours. A ratio under about 2x across the musical "
              "bands, with a smooth downward trend towards the top (damping), "
              "is ordinary well-behaved reverb decay.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

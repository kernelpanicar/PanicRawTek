#!/usr/bin/env python3
"""Demo hard groove (~30 s) de Panic Raw Tek a 148 BPM.
Un loop de percusión que se va armando con presets GROOVE PERC, y encima
un preset HARD GROOVE / OLD SCHOOL distinto cada 2 compases. Todo sale del plugin.
Uso: python3 tools/demo_hardgroove.py <dir_salida>"""
import os, sys, subprocess
import numpy as np
import scipy.io.wavfile as wavfile
from scipy import signal
from scipy.ndimage import maximum_filter1d

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
RENDER = os.path.join(ROOT, "build/PanicRenderMidi_artefacts/Release/PanicRenderMidi")
OUT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "demo_hg")
os.makedirs(OUT, exist_ok=True)

SR, BPM = 48000, 148.0
STEP = 60.0 / BPM / 4
BAR = STEP * 16
SEG = 2                      # compases por preset
LEADS = [
    ("Hard Groove/Broom Stab", 0.0, lambda b: [(s, [57, 64], 0.9, 0.6) for s in (2, 7, 10, 15)]),
    ("Hard Groove/Hyperdrive Riff", -1.0, lambda b: [(s, [n], 0.9 if s % 4 == 2 else 0.7, 0.8) for s, n in
                                                       zip((0, 2, 3, 6, 8, 10, 11, 14), (45, 45, 57, 45, 48, 45, 57, 43 if b else 46))]),
    ("Hard Groove/Federation Stab", 0.0, lambda b: [(s, [57], 0.9, 0.7) for s in (3, 6, 11, 14)]),
    ("Hard Groove/Organ Hit 93", -1.0, lambda b: [(s, [57, 60, 64], 0.9, 0.7) for s in (0, 3, 6, 10, 13)]),
    ("Old School/Surgeon Loop", 0.0, lambda b: [(s, [57], 0.95 if s in (0, 8) else 0.7, 0.5) for s in range(0, 16, 2)]),
    ("Hard Groove/Mutual Dub", 0.0, lambda b: [(s, [57, 60], 0.9, 1.0) for s in ((3, 10) if b == 0 else (3, 8, 14))]),
    ("Hard Groove/Hoover Hit", -1.5, lambda b: [(0, [45], 1.0, 2), (6, [48], 0.9, 2), (12, [43 if b else 46], 0.9, 2)]),
    ("Old School/Mentasm Stab", -2.0, lambda b: [(0, [45], 1.0, 6), (8, [44 if b else 46], 1.0, 6)]),
    ("Old School/Formant Stab", 0.0, lambda b: [(s, [57], 0.9, 0.6) for s in (2, 5, 7, 10, 13, 15)]),
]
N_SEG = len(LEADS)
BARS = N_SEG * SEG
LENGTH = BARS * BAR + 2.0
N = int(LENGTH * SR)

def t(bar, step): return bar * BAR + step * STEP

def render(preset, events, name):
    ev = os.path.join(OUT, name + ".txt")
    with open(ev, "w") as f:
        for s0, n, v, d in sorted(events):
            f.write(f"{s0:.6f} {n} {v:.3f} {d:.6f}\n")
    wav = os.path.join(OUT, name + ".wav")
    subprocess.run([RENDER, f"Presets/{preset}.rtkpreset", ev, f"{LENGTH:.3f}", str(BPM), wav], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL)
    x = wavfile.read(wav)[1].astype(np.float64).T
    out = np.zeros((2, N)); m = min(N, x.shape[1]); out[:, :m] = x[:, :m]
    return out

def butter(x, kind, fc, order=2):
    return signal.sosfilt(signal.butter(order, fc, kind, fs=SR, output="sos"), x, axis=-1)
db = lambda g: 10 ** (g / 20)
def pan(x, p):  # p -1..1
    a = (p + 1) * np.pi / 4
    m = x.mean(0)
    return np.stack([m * np.cos(a), m * np.sin(a)]) * np.sqrt(2)

# ---------------------------------------------------------------- percusión (entra de a poco)
perc = {}  # nombre -> (preset, gain, pan, eventos, desde compás)
def add(name, preset, gain, p, from_bar, fn):
    evs = []
    for bar in range(from_bar, BARS):
        for st, note, vel, dur in fn(bar):
            evs.append((t(bar, st), note, vel, dur * STEP))
    perc[name] = (preset, gain, p, evs)

add("kick", "Perc/Raw Kick", -1.0, 0.0, 0, lambda b: [(s, 33, 1.0, 1.5) for s in (0, 4, 8, 12)])
add("conga", "Groove Perc/HG Conga Low", -6.0, -0.35, 0,
    lambda b: [(3, 50, 0.9, 1), (6, 55, 0.7, 1), (11, 50, 0.9, 1), (14, 57, 0.6, 1)])
add("cowbell", "Groove Perc/Cowbell 808", -12.0, 0.3, 2, lambda b: [(s, 69, 0.8, 1) for s in (3, 7, 10, 15)])
add("hat", "Groove Perc/Closed Hat Tight", -10.0, 0.2, 4,
    lambda b: [(s, 60, 1.0 if s % 4 == 2 else 0.45, 0.5) for s in range(16)])
add("bass", "Bass/Hard Groove Bass", -4.0, 0.0, 4, lambda b: [(s, 33 if b % 4 != 3 else 36, 0.9, 0.7) for s in (2, 6, 10, 14)])
add("ride", "Groove Perc/Ride Metal", -14.0, 0.4, 6, lambda b: [(s, 64, 0.8, 1) for s in (2, 6, 10, 14)])
add("clap", "Groove Perc/Noise Clap", -7.0, 0.0, 8, lambda b: [(s, 60, 1.0, 1) for s in (4, 12)])
add("shaker", "Groove Perc/Shaker Groove", -13.0, -0.25, 10, lambda b: [(s, 60, 0.9 if s % 2 else 0.5, 0.5) for s in range(16)])
add("tom", "Groove Perc/Tribal Tom", -6.0, -0.15, 12,
    lambda b: [(13, 45, 0.8, 1), (14, 43, 0.9, 1), (15, 41, 1.0, 1)] if b % 2 == 1 else [(7, 45, 0.6, 1)])

print("Renderizando percusión...")
kick_times = [s for s, *_ in perc["kick"][3]]
mix = np.zeros((2, N))
tracks = {}
for name, (preset, gain, p, evs) in perc.items():
    x = render(preset, evs, name)
    tracks[name] = x
duck = np.zeros(N)
cl = int(0.18 * SR); curve = np.exp(-np.arange(cl) / SR / 0.045)
for s in kick_times:
    i = int(s * SR); j = min(N, i + cl)
    duck[i:j] = np.maximum(duck[i:j], curve[: j - i])

for name, (preset, gain, p, evs) in perc.items():
    x = tracks[name]
    if name == "kick":
        x = butter(x, "highpass", 32)
    elif name == "bass":
        x = butter(x, "highpass", 40) * (1 - 0.9 * duck)
    else:
        x = butter(x, "highpass", 150)
        x = pan(x, p) if abs(p) > 0 else x
    mix += x * db(gain)

# rumble del kick (reverb oscura + saturación + sidechain)
rng = np.random.default_rng(5)
ir_len = int(1.4 * SR)
ir = rng.standard_normal(ir_len) * np.exp(-np.arange(ir_len) / SR / 0.35)
ir = butter(ir, "lowpass", 1800); ir[: int(0.01 * SR)] = 0
rum = signal.fftconvolve(tracks["kick"].mean(0), ir)[:N]
rum = butter(butter(rum, "lowpass", 120, 4), "highpass", 40)
rum = np.tanh(rum / (np.abs(rum).max() + 1e-9) * 3.0) * (1 - 0.95 * duck)
mix += np.stack([rum, rum]) * db(-21.0)

# ---------------------------------------------------------------- leads: uno nuevo cada 2 compases
print("Renderizando leads...")
timeline = []
for idx, (preset, gain, seq) in enumerate(LEADS):
    b0 = idx * SEG
    evs = [(t(b0 + b, st), n, v, d * STEP) for b in range(SEG) for st, notes, v, d in seq(b) for n in notes]
    x = render(preset, evs, f"lead{idx:02d}")
    x = butter(x, "highpass", 140) * (1 - 0.35 * duck)
    mix += x * db(gain - 3.0)
    timeline.append((b0 * BAR, preset.split("/")[1]))

# ---------------------------------------------------------------- master
mix = butter(mix, "highpass", 25)
mix = np.tanh(mix * 1.25) / np.tanh(1.25)
ceiling = db(-1.0); pre = db(4.0); la = int(0.004 * SR)
pk = maximum_filter1d(np.abs(mix).max(axis=0), size=2 * la + 1)
pk = np.concatenate([pk[la:], np.full(la, pk[-1])])
need = np.minimum(1.0, ceiling / (pk * pre + 1e-12))
rel = np.exp(-1 / (0.08 * SR)); g = np.empty_like(need); cur = 1.0
for i in range(len(need)):
    cur = need[i] if need[i] < cur else need[i] + (cur - need[i]) * rel
    g[i] = cur
mix = np.clip(mix * pre * g, -ceiling, ceiling)
fi, fo = int(0.01 * SR), int(1.8 * SR)
mix[:, :fi] *= np.linspace(0, 1, fi); mix[:, -fo:] *= np.linspace(1, 0, fo)

out = os.path.join(OUT, "PanicRawTek_HardGroove_Demo.wav")
wavfile.write(out, SR, (mix.T * 32767).astype(np.int16))
print(f"{LENGTH:.1f} s  peak {20*np.log10(np.abs(mix).max()):.1f} dBFS  rms {20*np.log10(np.sqrt((mix**2).mean())):.1f} dBFS")
for t0, name in timeline:
    print(f"  {int(t0 // 60)}:{t0 % 60:05.2f}  {name}")

#!/usr/bin/env python3
"""Demo corto (~30 s) de Panic Raw Tek: 8 presets raw / groove / rave, 2 compases cada uno,
sobre kick + rumble + hats del propio synth a 145 BPM. Render estéreo con el plugin real.
Uso: python3 tools/demo_short.py <dir_salida>"""
import os, sys, subprocess
import numpy as np
import scipy.io.wavfile as wavfile
from scipy import signal
from scipy.ndimage import maximum_filter1d

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
RENDER = os.path.join(ROOT, "build/PanicRenderMidi_artefacts/Release/PanicRenderMidi")
OUT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "demo_short")
os.makedirs(OUT, exist_ok=True)

SR, BPM = 48000, 145.0
STEP = 60.0 / BPM / 4
BAR = STEP * 16
SEG_BARS = 2

def hits(seq, bars=2):
    out = []
    for b in range(bars):
        for s, notes, v, d in seq(b):
            out.append((b * 16 + s, notes, v, d))
    return out

# (preset, ganancia dB, sidechain, notas)
showcase = [
    ("Rave/Hard Groove Stab", -1.0, 0.45,
     hits(lambda b: [(s, [57, 60, 64], 0.95, 1.2) for s in ((2, 6, 10, 13) if b == 0 else (2, 6, 10, 14))])),
    ("Groove/Hypno Stab", 0.0, 0.3,
     hits(lambda b: [(s, [n], 0.9, 0.7) for s, n in zip((0, 3, 7, 10, 12), (57, 57, 60, 57, 55 if b else 62))])),
    ("Bass/Rolling Bass", -2.0, 0.85,
     hits(lambda b: [(beat * 4 + s, [33 if not (b and beat == 3) else 31], 0.95 if s == 1 else 0.7, 0.75)
                     for beat in range(4) for s in (1, 2, 3)])),
    ("Groove/Mental Bleep", 0.0, 0.0,
     hits(lambda b: [(s, [n], 0.9, 0.5) for s, n in zip((0, 3, 5, 8, 11, 14), (69, 72, 69, 76, 69, 74 if b else 79))])),
    ("Rave/Rave Hoover", -1.5, 0.4,
     [(0, [45], 1.0, 7.5), (8, [48], 1.0, 3.5), (12, [46], 1.0, 3.5), (16, [45], 1.0, 7.5), (24, [43], 1.0, 7.5)]),
    ("Groove/Dub Chord Deep", 0.0, 0.4,
     hits(lambda b: [(s, [57, 60], 0.9, 0.8) for s in ((3, 10) if b == 0 else (3, 8, 14))])),
    ("Metal/Anvil Hit", -1.0, 0.0,
     hits(lambda b: [(0, [52], 1.0, 1), (3, [52], 0.7, 1), (6, [55], 0.9, 1), (10, [52], 0.8, 1), (14, [58 if b else 61], 1.0, 1)])),
    ("Rave/Trance Gate", -2.0, 0.3,
     [(0, [57, 60, 64], 1.0, 15.5), (16, [55, 59, 62], 1.0, 15.5)]),
]

N_SEG = len(showcase)
LENGTH = N_SEG * SEG_BARS * BAR + 2.0
N = int(LENGTH * SR)

def render(preset_spec, events, name):
    ev = os.path.join(OUT, name + ".txt")
    with open(ev, "w") as f:
        for s0, n, v, d in sorted(events):
            f.write(f"{s0:.6f} {n} {v:.3f} {d:.6f}\n")
    wav = os.path.join(OUT, name + ".wav")
    subprocess.run([RENDER, preset_spec, ev, f"{LENGTH:.3f}", str(BPM), wav], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL)
    x = wavfile.read(wav)[1].astype(np.float64).T   # (2, n)
    out = np.zeros((2, N))
    m = min(N, x.shape[1])
    out[:, :m] = x[:, :m]
    return out

def butter(x, kind, fc, order=2):
    return signal.sosfilt(signal.butter(order, fc, kind, fs=SR, output="sos"), x, axis=-1)
db = lambda g: 10 ** (g / 20)

# ---- groove de fondo: kick 4/4, rumble, hats offbeat (todo del synth)
total_beats = N_SEG * SEG_BARS * 4
kick_times = [i * 4 * STEP for i in range(total_beats)]
kick = render("Presets/Perc/Raw Kick.rtkpreset", [(t, 33, 1.0, STEP * 1.5) for t in kick_times], "kick")
kick = butter(kick, "highpass", 32)
hat_ev = [(i * 4 * STEP + 2 * STEP, 62, 1.0, STEP * 0.5) for i in range(total_beats)]
hat_ev += [(i * 4 * STEP + 3 * STEP, 62, 0.4, STEP * 0.3) for i in range(total_beats) if i % 2]
hats = butter(render("Presets/Perc/Tick Noise.rtkpreset", hat_ev, "hats"), "highpass", 5000)

duck = np.zeros(N)
cl = int(0.2 * SR); curve = np.exp(-np.arange(cl) / SR / 0.05)
for t in kick_times:
    i = int(t * SR); j = min(N, i + cl)
    duck[i:j] = np.maximum(duck[i:j], curve[: j - i])

# rumble: kick -> reverb oscura -> lowpass -> saturación -> sidechain
rng = np.random.default_rng(3)
ir_len = int(1.6 * SR)
ir = rng.standard_normal(ir_len) * np.exp(-np.arange(ir_len) / SR / 0.4)
ir = butter(ir, "lowpass", 2000)
ir[: int(0.01 * SR)] = 0
rum = signal.fftconvolve(kick.mean(0), ir)[:N]
rum = butter(butter(rum, "lowpass", 130, 4), "highpass", 40)
rum = np.tanh(rum / (np.abs(rum).max() + 1e-9) * 3.0) * (1 - 0.95 * duck)

mix = kick * db(-2.0) + hats * db(-9.0) + np.stack([rum, rum]) * db(-19.0)

timeline = []
for idx, (preset, gain, sc, seq) in enumerate(showcase):
    t0 = idx * SEG_BARS * BAR
    events = [(t0 + st * STEP, n, vel, dur * STEP) for st, notes, vel, dur in seq for n in notes]
    x = render(f"Presets/{preset}.rtkpreset", events, f"{idx:02d}")
    x = butter(x, "highpass", 40 if "Bass" in preset else 130)
    x = x * (1 - sc * duck) * db(gain - 1.0)
    mix += x
    timeline.append((t0, preset.split("/")[1]))

# ---- master: glue + limitador
mix = butter(mix, "highpass", 25)
mix = np.tanh(mix * 1.3) / np.tanh(1.3)
ceiling = db(-1.0); pre = db(5.0); la = int(0.004 * SR)
pk = maximum_filter1d(np.abs(mix).max(axis=0), size=2 * la + 1)
pk = np.concatenate([pk[la:], np.full(la, pk[-1])])
need = np.minimum(1.0, ceiling / (pk * pre + 1e-12))
rel = np.exp(-1 / (0.08 * SR)); g = np.empty_like(need); cur = 1.0
for i in range(len(need)):
    cur = need[i] if need[i] < cur else need[i] + (cur - need[i]) * rel
    g[i] = cur
mix = np.clip(mix * pre * g, -ceiling, ceiling)
fi, fo = int(0.01 * SR), int(1.8 * SR)
mix[:, :fi] *= np.linspace(0, 1, fi)
mix[:, -fo:] *= np.linspace(1, 0, fo)

out = os.path.join(OUT, "PanicRawTek_Demo_30s.wav")
wavfile.write(out, SR, (mix.T * 32767).astype(np.int16))
seg = SEG_BARS * BAR
levels = [20 * np.log10(np.sqrt((mix[:, int(i * seg * SR):int((i + 1) * seg * SR)] ** 2).mean()) + 1e-12) for i in range(N_SEG)]
print(f"{LENGTH:.1f} s  peak {20*np.log10(np.abs(mix).max()):.1f} dBFS  rms {20*np.log10(np.sqrt((mix**2).mean())):.1f} dBFS")
for (t0, name), lv in zip(timeline, levels):
    print(f"  {int(t0 // 60)}:{t0 % 60:05.2f}  {name:20s} rms {lv:5.1f}")

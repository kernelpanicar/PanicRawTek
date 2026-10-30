#!/usr/bin/env python3
"""Demo track de RawTek: techno crudo / industrial a 135 BPM, todo hecho con presets del plugin.

1) genera la secuencia de cada pista
2) renderiza cada pista con RawTekRenderMidi (el plugin real)
3) mezcla: rumble del kick, sidechain, paneo, delay, master con limitador

Uso: python3 tools/demo_song.py <dir_salida>
"""
import os, sys, subprocess
import numpy as np
import scipy.io.wavfile as wavfile
from scipy import signal

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
RENDER = os.path.join(ROOT, "build/PanicRenderMidi_artefacts/Release/PanicRenderMidi")
OUT = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "demo")
STEMS = os.path.join(OUT, "stems")
os.makedirs(STEMS, exist_ok=True)

SR = 48000
BPM = 135.0
STEP = 60.0 / BPM / 4          # 16avo
BAR = STEP * 16
BARS = 48
LENGTH = BARS * BAR + 4.0       # + cola

def t(bar, step=0.0):
    return bar * BAR + step * STEP

# ---------------------------------------------------------------- estructura
INTRO, BUILD, GROW, BREAK, DROP, OUTRO = (0, 8), (8, 16), (16, 24), (24, 30), (30, 44), (44, 48)
def in_(sec, bar): return sec[0] <= bar < sec[1]

tracks = {}  # nombre -> (preset, [eventos])
def add(name, preset, start, note, vel, dur):
    tracks.setdefault(name, (preset, []))[1].append((start, note, vel, dur))

for bar in range(BARS):
    phrase_end = bar % 4 == 3
    kick_on = not in_(BREAK, bar)

    # KICK: 4/4, con huecos al final de las frases antes del break y del drop
    if kick_on:
        for beat in range(4):
            if bar in (23, 29) and beat == 3:
                continue
            add("kick", "Presets/Perc/Raw Kick.rtkpreset", t(bar, beat * 4), 33, 1.0, STEP * 1.5)

    # HATS: offbeat desde el compás 4, 16avos fantasma en GROW y DROP
    if bar >= 4 and not in_(BREAK, bar):
        for s in range(16):
            if s % 4 == 2:
                add("hat", "Presets/Perc/Steel Hat.rtkpreset", t(bar, s), 62, 1.0, STEP * 0.5)
            elif (in_(GROW, bar) or in_(DROP, bar)) and s % 2 == 1:
                add("hat", "Presets/Perc/Steel Hat.rtkpreset", t(bar, s), 62, 0.35 + 0.1 * (s % 4 == 3), STEP * 0.3)
    # redoble de hats al final del break
    if bar == 29:
        for s in range(16):
            add("hat", "Presets/Perc/Steel Hat.rtkpreset", t(bar, s), 62, 0.3 + 0.7 * s / 15, STEP * 0.3)

    # CLANK en 2 y 4 (lugar del clap)
    if (in_(BUILD, bar) or in_(GROW, bar) or in_(DROP, bar)):
        for s in (4, 12):
            add("clank", "Presets/Perc/Clank.rtkpreset", t(bar, s), 50, 0.9, STEP)

    # RIM industrial, contratiempos
    if in_(BUILD, bar) and bar >= 12 or in_(GROW, bar) or in_(DROP, bar):
        for s, v in ((3, 0.8), (7, 0.6), (11, 0.8), (13, 0.4), (15, 0.6)):
            add("rim", "Presets/Perc/Industrial Rim.rtkpreset", t(bar, s), 64, v, STEP * 0.5)

    # BAJO rodante entre kicks (sólo drop)
    if in_(DROP, bar):
        for beat in range(4):
            for s in (2, 3):
                add("bass", "Presets/Bass/Pressure Bass.rtkpreset", t(bar, beat * 4 + s), 33, 0.8 if s == 2 else 0.6, STEP * 0.7)

    # CHAIN DRAG: textura sostenida
    if bar in (12, 14) or (in_(DROP, bar) and bar % 4 == 0):
        add("chain", "Presets/Metal/Chain Drag.rtkpreset", t(bar), 45, 0.8, BAR * 2 - STEP)

    # SYNC SHRAPNEL: riff metálico
    if in_(GROW, bar) or in_(DROP, bar):
        riff = {0: 57, 3: 57, 6: 60, 8: 57, 10: 58, 13: 57, 15: 64}
        if bar % 2 == 1:
            riff = {0: 57, 3: 57, 6: 63, 9: 58, 11: 57, 14: 52}
        for s, n in riff.items():
            add("shrapnel", "Presets/Metal/Sync Shrapnel.rtkpreset", t(bar, s), n, 0.9 if s % 4 == 0 else 0.7, STEP * 0.6)

    # METAL PIPE en GROW
    if in_(GROW, bar):
        add("pipe", "Presets/Metal/Metal Pipe.rtkpreset", t(bar, 6), 52 if bar % 2 == 0 else 55, 0.8, STEP)
        if bar % 4 == 3:
            add("pipe", "Presets/Metal/Metal Pipe.rtkpreset", t(bar, 13), 58, 0.7, STEP)

    # METAL STAB sincopado en el drop
    if in_(DROP, bar):
        for s in ((2, 7, 10) if bar % 2 == 0 else (2, 7, 12)):
            for n in (57, 60, 63):
                add("stab", "Presets/Stab/Metal Stab.rtkpreset", t(bar, s), n, 0.85, STEP * 0.8)

    # ANVIL cada 2 compases en el drop
    if in_(DROP, bar) and bar % 2 == 1:
        add("anvil", "Presets/Metal/Anvil Hit.rtkpreset", t(bar, 14), 64, 1.0, STEP)

    # TIN CAN ZAP: fills al final de cada frase
    if (in_(GROW, bar) or in_(DROP, bar)) and phrase_end:
        for i, s in enumerate((12, 13, 14, 15)):
            add("zap", "Presets/Metal/Tin Can Zap.rtkpreset", t(bar, s), 60 + 3 * i, 0.7 + 0.1 * i, STEP * 0.6)

# BREAK: drone + campanas + riser
add("drone", "Presets/Drone/Furnace Drone.rtkpreset", t(24), 45, 0.9, BAR * 6)
add("drone", "Presets/Drone/Furnace Drone.rtkpreset", t(24), 52, 0.8, BAR * 6)
add("drone", "Presets/Drone/Furnace Drone.rtkpreset", t(44), 45, 0.8, BAR * 3.5)
for bar in range(24, 30):
    add("bell", "Presets/Metal/Rusted Bell.rtkpreset", t(bar), 69, 0.9, STEP * 2)
    add("bell", "Presets/Metal/Rusted Bell.rtkpreset", t(bar, 10), 70 if bar % 2 else 72, 0.7, STEP * 2)
add("riser", "Presets/FX/Riser Metal.rtkpreset", t(26), 57, 1.0, BAR * 4 - STEP)
# golpes de gong al entrar el drop y al final
add("gong", "Presets/Metal/Gong Strike.rtkpreset", t(30), 45, 1.0, STEP * 4)
add("gong", "Presets/Metal/Gong Strike.rtkpreset", t(47, 12), 45, 1.0, STEP * 4)

# ---------------------------------------------------------------- MIDI para Ableton
def write_midi(path):
    ppq = 480
    def vlq(n):
        out = [n & 0x7F]; n >>= 7
        while n: out.insert(0, (n & 0x7F) | 0x80); n >>= 7
        return bytes(out)
    def chunk(tag, data): return tag + len(data).to_bytes(4, "big") + data
    trks = []
    tempo = int(60_000_000 / BPM)
    meta = vlq(0) + b"\xff\x51\x03" + tempo.to_bytes(3, "big") + vlq(0) + b"\xff\x58\x04\x04\x02\x18\x08" + vlq(0) + b"\xff\x2f\x00"
    trks.append(chunk(b"MTrk", meta))
    sec2tick = lambda sec: int(round(sec * BPM / 60 * ppq))
    for name, (preset, events) in tracks.items():
        label = f"{name} - {os.path.splitext(os.path.basename(preset))[0]}".encode()
        evs = []
        for s0, n, v, d in events:
            evs.append((sec2tick(s0 + d), 0, bytes([0x80, n, 0])))
            evs.append((sec2tick(s0), 1, bytes([0x90, n, max(1, min(127, int(v * 127)))])))
        evs.sort(key=lambda e: (e[0], e[1]))
        data = vlq(0) + b"\xff\x03" + vlq(len(label)) + label
        last = 0
        for tick, _, msg in evs:
            data += vlq(tick - last) + msg; last = tick
        data += vlq(0) + b"\xff\x2f\x00"
        trks.append(chunk(b"MTrk", data))
    header = chunk(b"MThd", (1).to_bytes(2, "big") + len(trks).to_bytes(2, "big") + ppq.to_bytes(2, "big"))
    with open(path, "wb") as f:
        f.write(header + b"".join(trks))

write_midi(os.path.join(OUT, "RawTek_Demo_Track.mid"))

# ---------------------------------------------------------------- render
def render(name, preset, events):
    ev_file = os.path.join(STEMS, name + ".txt")
    with open(ev_file, "w") as f:
        for s, n, v, d in sorted(events):
            f.write(f"{s:.6f} {n} {v:.3f} {d:.6f}\n")
    wav = os.path.join(STEMS, name + ".wav")
    subprocess.run([RENDER, preset, ev_file, f"{LENGTH:.3f}", str(BPM), wav], cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL)
    sr, x = wavfile.read(wav)
    assert sr == SR
    return x[:, 0].astype(np.float64)

print("Renderizando pistas con RawTek...")
raw = {}
for name, (preset, events) in tracks.items():
    raw[name] = render(name, preset, events)
    print(f"  {name:9s} {len(events):4d} notas  ({os.path.basename(preset)})")

N = int(LENGTH * SR)
raw = {k: np.pad(v, (0, max(0, N - len(v))))[:N] for k, v in raw.items()}

# ---------------------------------------------------------------- mezcla
def butter(x, kind, fc, order=2):
    sos = signal.butter(order, fc, kind, fs=SR, output="sos")
    return signal.sosfilt(sos, x)

def db(g): return 10 ** (g / 20)

# Envolvente de sidechain: dip en cada kick
duck = np.zeros(N)
kick_times = [s for s, *_ in tracks["kick"][1]]
curve_len = int(0.22 * SR)
tt = np.arange(curve_len) / SR
curve = np.exp(-tt / 0.06)
for s in kick_times:
    i = int(s * SR)
    j = min(N, i + curve_len)
    duck[i:j] = np.maximum(duck[i:j], curve[: j - i])
def sidechain(x, depth): return x * (1 - depth * duck)

# RUMBLE: kick -> reverb larga -> lowpass -> saturación -> sidechain
rng = np.random.default_rng(7)
ir_len = int(2.2 * SR)
ir = rng.standard_normal(ir_len) * np.exp(-np.arange(ir_len) / SR / 0.55)
ir = butter(ir, "lowpass", 2500)
ir[: int(0.012 * SR)] = 0  # predelay
rumble = signal.fftconvolve(raw["kick"], ir)[:N]
rumble = butter(rumble, "lowpass", 140, 4)
rumble = butter(rumble, "highpass", 42, 2)
rumble = np.tanh(rumble / (np.abs(rumble).max() + 1e-9) * 3.0)
rumble = sidechain(rumble, 0.95)
# el rumble aparece desde el compás 2, se va en el break
rumble_gate = np.ones(N)
def fade_region(arr, t0, t1, v0, v1):
    i0, i1 = int(t0 * SR), int(t1 * SR)
    arr[i0:i1] = np.linspace(v0, v1, i1 - i0)
fade_region(rumble_gate, 0, t(2), 0, 1)
rumble_gate[int(t(24) * SR):int(t(30) * SR)] = 0
fade_region(rumble_gate, t(30) - 0.02, t(30) + 0.02, 0, 1)

kick = raw["kick"]
kick = kick + 0.25 * np.tanh(butter(kick, "highpass", 2500) * 6)  # click
kick = butter(kick, "highpass", 32)

# Delay 3/16 para stab y shrapnel (send)
def delay_send(x, time_steps, fb, lp=3500, hp=300):
    d = int(time_steps * STEP * SR)
    y = np.zeros(N)
    src = butter(butter(x, "highpass", hp), "lowpass", lp)
    buf = src.copy()
    g = 1.0
    for k in range(1, 6):
        g *= fb
        y[d * k:] += g * buf[: N - d * k]
    return y

# Canales: (señal, ganancia dB, pan -1..1)
stab = sidechain(raw["stab"], 0.6)
shrap = sidechain(raw["shrapnel"], 0.4)
ch = {
    "kick":     (kick, 0.0, 0.0),
    "rumble":   (rumble * rumble_gate, -17.0, 0.0),
    "bass":     (sidechain(butter(raw["bass"], "highpass", 45), 0.85), -9.0, 0.0),
    "hat":      (butter(raw["hat"], "highpass", 4500), -7.0, 0.25),
    "clank":    (butter(raw["clank"], "highpass", 150), -6.0, -0.15),
    "rim":      (raw["rim"], -9.0, -0.4),
    "chain":    (sidechain(butter(raw["chain"], "highpass", 200), 0.5), -16.0, 0.3),
    "shrapnel": (butter(shrap, "highpass", 180), -7.0, 0.35),
    "pipe":     (raw["pipe"], -11.0, -0.35),
    "stab":     (butter(stab, "highpass", 160), -8.5, -0.2),
    "anvil":    (raw["anvil"], -11.0, 0.45),
    "zap":      (raw["zap"], -11.0, -0.5),
    "drone":    (butter(raw["drone"], "highpass", 70), -8.0, 0.0),
    "bell":     (raw["bell"], -9.0, 0.2),
    "riser":    (butter(raw["riser"], "highpass", 150), -9.0, 0.0),
    "gong":     (raw["gong"], -6.0, 0.0),
    "delay":    (delay_send(stab * 0.6 + shrap * 0.5, 3, 0.5), -12.0, 0.0),
}

L = np.zeros(N); R = np.zeros(N)
for name, (x, g, pan) in ch.items():
    x = x[:N] * db(g)
    if name == "delay":   # ping-pong falso: canal derecho atrasado
        L += x
        R += np.concatenate([np.zeros(int(STEP * 1.5 * SR)), x])[:N]
        continue
    if name in ("drone", "stab", "bell"):  # ensanchar con haas
        d = int(0.011 * SR)
        L += x * np.sqrt(0.5)
        R += np.concatenate([np.zeros(d), x])[:N] * np.sqrt(0.5)
        continue
    a = (pan + 1) * np.pi / 4
    L += x * np.cos(a) * np.sqrt(2) * 0.7071
    R += x * np.sin(a) * np.sqrt(2) * 0.7071

# fade del outro
fade = np.ones(N)
fade_region(fade, t(46), LENGTH, 1, 0)
L *= fade; R *= fade

# ---------------------------------------------------------------- master
mix = np.stack([L, R])
mix = signal.sosfilt(signal.butter(2, 22, "highpass", fs=SR, output="sos"), mix, axis=1)
# glue: saturación suave
drive = 1.4
mix = np.tanh(mix * drive) / np.tanh(drive)
# limitador con lookahead
ceiling = db(-0.8)
target_peak = np.abs(mix).max(axis=0)
la = int(0.004 * SR)
from scipy.ndimage import maximum_filter1d
pk = maximum_filter1d(target_peak, size=2 * la + 1)
pk = np.concatenate([pk[la:], np.full(la, pk[-1])])
pre_gain = db(7.5)
need = np.minimum(1.0, ceiling / (pk * pre_gain + 1e-12))
# release suave
rel = np.exp(-1 / (0.08 * SR))
g = np.empty_like(need); cur = 1.0
for i in range(len(need)):
    cur = need[i] if need[i] < cur else need[i] + (cur - need[i]) * rel
    g[i] = cur
mix = mix * pre_gain * g
mix = np.clip(mix, -ceiling, ceiling)

wav_out = os.path.join(OUT, "RawTek_Demo_Track.wav")
wavfile.write(wav_out, SR, (mix.T * 32767).astype(np.int16))
peak_db = 20 * np.log10(np.abs(mix).max())
rms_db = 20 * np.log10(np.sqrt((mix ** 2).mean()))
print(f"Master: peak {peak_db:.1f} dBFS, RMS {rms_db:.1f} dBFS, {LENGTH:.1f} s")
print(wav_out)

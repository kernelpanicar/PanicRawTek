#!/usr/bin/env python3
"""Mide los renders de RenderTest y calcula el volumen de cada preset para nivelar la sonoridad.
Uso: python3 tools/level_presets.py <renderDir> <out.json>
Criterio: RMS de corto plazo máximo (ventanas de 100 ms) -> -11 dBFS, con techo de pico en -1.5 dBFS."""
import sys, os, json, glob
import numpy as np, scipy.io.wavfile as w

TARGET_SHORT_RMS = -11.0
PEAK_CEIL = -1.5
import re
PRESET_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Presets")

def current_volume(cat, name):
    """Volumen con el que se renderizó (el que tiene hoy el archivo del preset)."""
    try:
        txt = open(os.path.join(PRESET_ROOT, cat, name + ".rtkpreset")).read()
        m = re.search(r'id="volume" value="([-0-9.]+)"', txt)
        return float(m.group(1)) if m else -6.0
    except OSError:
        return -6.0

render_dir, out = sys.argv[1], sys.argv[2]
levels = {}
for f in sorted(glob.glob(os.path.join(render_dir, "*", "*.wav"))):
    cat = os.path.basename(os.path.dirname(f))
    if cat == "Factory":
        continue
    name = os.path.splitext(os.path.basename(f))[0].replace("_", " ")
    sr, x = w.read(f)
    x = x[:, 0].astype(np.float64) / 2**31
    win = int(sr * 0.1)
    n = len(x) // win
    short = np.sqrt((x[: n * win].reshape(n, win) ** 2).mean(axis=1))
    s_db = 20 * np.log10(short.max() + 1e-12)
    p_db = 20 * np.log10(np.abs(x).max() + 1e-12)
    adj = min(TARGET_SHORT_RMS - s_db, PEAK_CEIL - p_db)
    base = current_volume(cat, name)
    vol = float(np.clip(base + adj, -36, 12))
    levels[f"{cat}/{name}"] = round(vol, 1)
    flag = "  <-- CLAMP" if base + adj > 12 else ""
    print(f"{cat+'/'+name:28s} shortRMS {s_db:6.1f}  peak {p_db:6.1f}  -> vol {vol:5.1f}{flag}")
json.dump(levels, open(out, "w"), indent=1)

#!/usr/bin/env python3
"""Genera el pack de presets de RawTek (Presets/<Categoria>/<Nombre>.rtkpreset).

Valores en unidades reales (Hz, segundos, semitonos, 0..1). Los choices van por índice.
  Ondas:       saw=0 sq=1 pulse=2 tri=3
  Filtro:      LP24=0 LP12=1 BP=2 HP=3
  LFO wave:    sine=0 tri=1 square=2 S&H=3
  LFO div:     4/1=0 2/1=1 1/1=2 1/2=3 1/4=4 1/8=5 1/16=6 1/32=7 1/4T=8 1/8T=9 1/16T=10 1/8D=11 1/16D=12
  Dist:        soft=0 hard=1 fold=2 crush=3
  Voice mode:  mono=0 poly=1

Uso:  python3 tools/make_presets.py [levels.json]
  levels.json (opcional) = {"Categoria/Nombre": volumen_dB} generado por tools/level_presets.py
"""
import json, os, sys
from xml.sax.saxutils import quoteattr

SAW, SQ, PULSE, TRI = 0, 1, 2, 3
LP24, LP12, BP, HP = 0, 1, 2, 3
SINE, LTRI, LSQ, SH = 0, 1, 2, 3
SOFT, HARD, FOLD, CRUSH = 0, 1, 2, 3
MONO, POLY = 0, 1

# Base percusiva: sin sustain
PERC_AMP = dict(aA=0.001, aS=0.0)

P = {}
def preset(cat, name, **kw):
    P[(cat, name)] = kw

# ----------------------------------------------------------------------------- METAL
preset("Metal", "Steel Rod", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=7, osc2Detune=31, oscMix=0.0,
       fm=0.35, fmEnv=0.4, cutoff=3000, reso=0.3, envAmt=0.3, fD=0.3, keyTrack=0.5,
       aD=0.6, aR=0.4, **PERC_AMP, distType=SOFT, distAmt=0.2, mode=POLY)
preset("Metal", "Anvil Hit", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=19, osc2Detune=40, oscMix=0.2,
       fm=0.5, fmEnv=0.5, ring=0.4, pitchEnv=12, pitchDecay=0.03, filterMode=BP, cutoff=2500, reso=0.4,
       envAmt=0.3, fD=0.15, aD=0.35, aR=0.25, **PERC_AMP, distType=HARD, distAmt=0.4, mode=POLY)
preset("Metal", "Rusted Bell", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=17, osc2Detune=13, ring=0.7, fm=0.2,
       cutoff=5000, reso=0.15, envAmt=0.2, fD=0.6, aD=1.2, aR=1.0, **PERC_AMP,
       combMix=0.4, combTune=0, combFb=0.9, combDamp=0.2, distAmt=0.15, mode=POLY)
preset("Metal", "Factory Clang", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=11, osc2Detune=23, ring=0.8,
       filterMode=BP, cutoff=1800, reso=0.6, envAmt=0.4, fD=0.15, aD=0.25, aR=0.2, **PERC_AMP,
       distType=FOLD, distAmt=0.4, mode=POLY)
preset("Metal", "Metal Pipe", osc1Wave=SQ, oscMix=0.0, noise=0.7, cutoff=4000, reso=0.1, envAmt=0.3, fD=0.05,
       combMix=0.8, combTune=0, combFb=0.95, combDamp=0.1, keyTrack=0.4,
       aD=0.8, aR=0.5, **PERC_AMP, distAmt=0.2, mode=POLY)
preset("Metal", "Chain Drag", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=5, osc2Detune=47, noise=0.5, ring=0.6, fm=0.6,
       filterMode=BP, cutoff=1500, reso=0.5, envAmt=0.2,
       lfoWave=SH, lfoSync=1, lfoDiv=6, lfoDepth=0.5,
       aS=0.8, aR=0.2, distType=CRUSH, distAmt=0.35, mode=MONO)
preset("Metal", "Gong Strike", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=-5, osc2Detune=18, fm=0.3, fmEnv=0.5, ring=0.3,
       cutoff=2000, envAmt=0.4, fD=1.0, aD=2.5, aR=2.0, **PERC_AMP,
       combMix=0.3, combTune=7, combFb=0.8, combDamp=0.4, distAmt=0.2, mode=POLY)
preset("Metal", "Sheet Metal", osc1Wave=SQ, osc2Wave=PULSE, pw=0.2, osc2Semi=13, osc2Detune=9, ring=1.0,
       filterMode=HP, cutoff=600, reso=0.3, envAmt=0.1, aD=0.18, aR=0.12, **PERC_AMP,
       distType=HARD, distAmt=0.5, mode=POLY)
preset("Metal", "Tin Can Zap", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=12, osc2Detune=21, fm=0.6, fmEnv=0.4,
       pitchEnv=36, pitchDecay=0.02, cutoff=3500, reso=0.5, envAmt=0.2, aD=0.12, aR=0.08, **PERC_AMP,
       distType=FOLD, distAmt=0.3, mode=MONO)
preset("Metal", "Iron Resonator", osc1Wave=SAW, oscMix=0.0, cutoff=800, envAmt=0.5, fD=0.1,
       combMix=0.7, combTune=12, combFb=0.93, combDamp=0.5, aD=0.5, aS=0.2, aR=0.3,
       distAmt=0.4, mode=MONO)
preset("Metal", "Sync Shrapnel", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=19, osc2Detune=0, oscMix=1.0, sync=1, ring=0.4,
       cutoff=2800, reso=0.35, envAmt=0.35, fD=0.12, aD=0.22, aR=0.15, **PERC_AMP,
       distType=HARD, distAmt=0.35, mode=POLY)
preset("Metal", "Hammered Plate", osc1Wave=TRI, osc2Wave=SQ, osc2Semi=6, osc2Detune=37, fm=0.45, fmEnv=0.35, ring=0.5,
       noise=0.15, filterMode=BP, cutoff=3200, reso=0.45, envAmt=0.25, fD=0.08,
       combMix=0.35, combTune=5, combFb=0.88, combDamp=0.15,
       aD=0.7, aR=0.5, **PERC_AMP, distType=SOFT, distAmt=0.35, mode=POLY)

# ----------------------------------------------------------------------------- PLUCK
preset("Pluck", "Raw Pluck", osc1Wave=SAW, oscMix=0.3, cutoff=300, reso=0.35, envAmt=0.7, fD=0.12,
       aD=0.25, aR=0.1, **PERC_AMP, distAmt=0.3, mode=POLY)
preset("Pluck", "Karplus Wire", osc1Wave=SAW, oscMix=0.0, noise=0.6, cutoff=30, reso=0.0, envAmt=1.0, fA=0.0005, fD=0.015,
       combMix=1.0, combTune=0, combFb=0.97, combDamp=0.35, drive=0.3, keyTrack=0.0,
       aD=1.5, aR=0.6, **PERC_AMP, distAmt=0.15, mode=POLY)
preset("Pluck", "Steel String", osc1Wave=SAW, oscMix=0.0, noise=0.4, cutoff=40, reso=0.0, envAmt=1.0, fA=0.0005, fD=0.02,
       combMix=1.0, combTune=12, combFb=0.98, combDamp=0.08, keyTrack=0.0,
       aD=2.0, aR=0.8, **PERC_AMP, distType=SOFT, distAmt=0.25, mode=POLY)
preset("Pluck", "Bone Pluck", osc1Wave=TRI, oscMix=0.0, cutoff=200, reso=0.5, envAmt=0.8, fD=0.06,
       aD=0.18, aR=0.1, **PERC_AMP, distType=FOLD, distAmt=0.5, mode=POLY)
preset("Pluck", "Rubber Pluck", osc1Wave=SQ, oscMix=0.0, filterMode=LP12, cutoff=150, reso=0.7, envAmt=0.6, fD=0.09,
       pitchEnv=5, pitchDecay=0.02, aD=0.2, aR=0.1, **PERC_AMP, distAmt=0.3, mode=POLY)
preset("Pluck", "Metal Pluck", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=12, osc2Detune=0, oscMix=0.0, fm=0.25, fmEnv=0.5,
       cutoff=1500, envAmt=0.5, fD=0.1, combMix=0.3, combTune=19, combFb=0.8, combDamp=0.3,
       aD=0.3, aR=0.2, **PERC_AMP, distAmt=0.25, mode=POLY)
preset("Pluck", "Glass Shard", osc1Wave=TRI, osc2Wave=SAW, osc2Semi=24, osc2Detune=5, ring=0.6,
       filterMode=HP, cutoff=400, reso=0.2, envAmt=0.2, aD=0.4, aR=0.3, **PERC_AMP,
       combMix=0.4, combTune=24, combFb=0.85, combDamp=0.1, distAmt=0.2, mode=POLY)
preset("Pluck", "Dub Pluck", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=10, cutoff=250, reso=0.5, envAmt=0.55, fD=0.2,
       aD=0.35, aR=0.25, **PERC_AMP, distAmt=0.25, mode=POLY)
preset("Pluck", "Hollow Pluck", osc1Wave=PULSE, pw=0.12, oscMix=0.0, filterMode=BP, cutoff=600, reso=0.55, envAmt=0.4, fD=0.12,
       combMix=0.5, combTune=0, combFb=0.75, combDamp=0.6, aD=0.22, aR=0.15, **PERC_AMP, distAmt=0.3, mode=POLY)
preset("Pluck", "Wire Snap", osc1Wave=SQ, oscMix=0.0, noise=0.3, cutoff=60, envAmt=0.95, fD=0.01,
       combMix=0.9, combTune=7, combFb=0.94, combDamp=0.25, keyTrack=0.0, pitchEnv=12, pitchDecay=0.008,
       aD=0.6, aR=0.3, **PERC_AMP, distType=HARD, distAmt=0.3, mode=POLY)

# ----------------------------------------------------------------------------- BASS
preset("Bass", "Concrete Bass", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=-12, osc2Detune=0, sub=0.6,
       cutoff=220, reso=0.25, drive=0.7, envAmt=0.35, fD=0.15, aD=0.3, aS=0.6,
       distType=HARD, distAmt=0.45, mode=MONO)
preset("Bass", "Grinder Bass", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=18, fm=0.15, cutoff=400, reso=0.3, drive=0.8,
       envAmt=0.3, fD=0.2, aS=0.8, distType=FOLD, distAmt=0.45, mode=MONO)
preset("Bass", "Rumble Engine", osc1Wave=TRI, oscMix=0.0, sub=1.0, cutoff=120, reso=0.4, drive=0.6, envAmt=0.2,
       lfoWave=SINE, lfoSync=1, lfoDiv=6, lfoDepth=0.25, aS=1.0, aR=0.3,
       distType=SOFT, distAmt=0.6, mode=MONO)
preset("Bass", "Metal Bass", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=7, osc2Detune=3, ring=0.35, cutoff=350, reso=0.45,
       envAmt=0.5, fD=0.1, aS=0.7, distType=HARD, distAmt=0.4, mode=MONO)
preset("Bass", "Pressure Bass", osc1Wave=PULSE, pw=0.15, oscMix=0.0, sub=0.5, cutoff=180, reso=0.2, envAmt=0.6, fD=0.07,
       pitchEnv=12, pitchDecay=0.015, aD=0.2, aS=0.3, distType=SOFT, distAmt=0.5, mode=MONO)
preset("Bass", "Warehouse Low", osc1Wave=SAW, oscMix=0.0, filterMode=LP12, cutoff=160, reso=0.35, drive=0.5,
       envAmt=0.4, fD=0.12, aS=0.7, combMix=0.25, combTune=-12, combFb=0.6, combDamp=0.6, mode=MONO)
preset("Bass", "Crushed Bass", osc1Wave=SQ, oscMix=0.0, sub=0.5, cutoff=600, reso=0.2, envAmt=0.2, aS=0.8,
       distType=CRUSH, distAmt=0.5, distMix=0.7, mode=MONO)
preset("Bass", "Fold Bass", osc1Wave=TRI, oscMix=0.0, sub=0.4, cutoff=900, reso=0.0, envAmt=0.2, aS=0.8,
       distType=FOLD, distAmt=0.6, mode=MONO)
preset("Bass", "Iron Sub", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=12, osc2Detune=0, oscMix=0.0, fm=0.2, fmEnv=0.45,
       sub=0.7, cutoff=500, envAmt=0.25, fD=0.08, aD=0.4, aS=0.5, distType=SOFT, distAmt=0.5, mode=MONO)

# ----------------------------------------------------------------------------- STAB
preset("Stab", "Rave Stab Raw", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=12, osc2Detune=14, cutoff=1400, reso=0.3,
       envAmt=0.45, fD=0.18, aD=0.22, aR=0.15, **PERC_AMP, distType=HARD, distAmt=0.4, mode=POLY)
preset("Stab", "Metal Stab", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=5, osc2Detune=25, ring=0.5, filterMode=BP,
       cutoff=1800, reso=0.4, envAmt=0.4, fD=0.1, aD=0.2, aR=0.15, **PERC_AMP,
       distType=FOLD, distAmt=0.35, mode=POLY)
preset("Stab", "Dub Chord Raw", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=12, cutoff=700, reso=0.35, envAmt=0.4, fD=0.25,
       aD=0.4, aS=0.1, aR=0.4, combMix=0.35, combTune=12, combFb=0.7, combDamp=0.5, mode=POLY)
preset("Stab", "Hammer Stab", osc1Wave=SQ, osc2Wave=SAW, osc2Detune=8, pitchEnv=24, pitchDecay=0.012,
       cutoff=2000, envAmt=0.3, fD=0.06, aD=0.15, aR=0.1, **PERC_AMP, distType=HARD, distAmt=0.6, mode=POLY)
preset("Stab", "Shard Chord", osc1Wave=TRI, osc2Wave=SAW, osc2Semi=19, osc2Detune=8, fm=0.3, fmEnv=0.3,
       cutoff=2500, envAmt=0.25, fD=0.15, aD=0.3, aR=0.2, **PERC_AMP, distAmt=0.3, mode=POLY)
preset("Stab", "Berlin Stab", osc1Wave=PULSE, pw=0.3, osc2Wave=SAW, osc2Detune=9, cutoff=900, reso=0.5,
       envAmt=0.5, fD=0.14, aD=0.25, aR=0.15, **PERC_AMP, lfoWave=SH, lfoSync=1, lfoDiv=6, lfoDepth=0.15,
       distType=SOFT, distAmt=0.5, mode=POLY)
preset("Stab", "Rust Stab", osc1Wave=SAW, osc2Wave=TRI, osc2Semi=7, osc2Detune=17, ring=0.3, cutoff=1100, reso=0.45,
       envAmt=0.4, fD=0.12, combMix=0.45, combTune=0, combFb=0.86, combDamp=0.3,
       aD=0.35, aR=0.25, **PERC_AMP, distType=CRUSH, distAmt=0.3, distMix=0.6, mode=POLY)

# ----------------------------------------------------------------------------- PERC
preset("Perc", "Metal Tom", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=7, osc2Detune=33, oscMix=0.0, fm=0.3, fmEnv=0.6,
       pitchEnv=24, pitchDecay=0.06, cutoff=3000, envAmt=0.2, aD=0.3, aR=0.2, **PERC_AMP,
       distType=HARD, distAmt=0.3, mode=POLY)
preset("Perc", "Clank", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=23, osc2Detune=11, noise=0.5, ring=0.7,
       filterMode=BP, cutoff=2500, reso=0.6, envAmt=0.2, aD=0.08, aR=0.06, **PERC_AMP,
       distType=HARD, distAmt=0.5, mode=POLY)
preset("Perc", "Industrial Rim", osc1Wave=TRI, oscMix=0.0, noise=0.6, filterMode=HP, cutoff=1200, reso=0.5, envAmt=0.2,
       combMix=0.6, combTune=12, combFb=0.8, combDamp=0.3, aD=0.1, aR=0.08, **PERC_AMP, mode=POLY)
preset("Perc", "Zap Perc", osc1Wave=TRI, oscMix=0.0, pitchEnv=48, pitchDecay=0.025, cutoff=4000, envAmt=0.1,
       aD=0.12, aR=0.08, **PERC_AMP, distType=FOLD, distAmt=0.4, mode=POLY)
preset("Perc", "Shaker Crush", osc1Wave=TRI, oscMix=0.0, noise=1.0, filterMode=HP, cutoff=3000, reso=0.2, envAmt=0.0,
       aA=0.003, aD=0.07, aS=0.0, aR=0.05, distType=CRUSH, distAmt=0.4, mode=POLY)
preset("Perc", "Kick Body", osc1Wave=TRI, oscMix=0.0, sub=0.7, pitchEnv=36, pitchDecay=0.04, cutoff=500, envAmt=0.2,
       keyTrack=0.0, aD=0.35, aR=0.2, **PERC_AMP, distType=SOFT, distAmt=0.6, mode=MONO)
preset("Perc", "Raw Kick", osc1Wave=TRI, oscMix=0.0, sub=0.0, pitchEnv=34, pitchDecay=0.05, cutoff=6000, reso=0.0,
       envAmt=0.0, keyTrack=0.0, drive=0.35, velSens=0.2, aD=0.32, aR=0.06, **PERC_AMP,
       distType=SOFT, distAmt=0.55, mode=MONO)
preset("Perc", "Steel Hat", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=19, osc2Detune=43, ring=1.0, noise=0.3,
       filterMode=HP, cutoff=6000, reso=0.25, envAmt=0.0, aD=0.05, aR=0.04, **PERC_AMP,
       distType=HARD, distAmt=0.3, mode=POLY)

# ----------------------------------------------------------------------------- LEAD
preset("Lead", "Siren Raw", osc1Wave=SQ, osc2Wave=SAW, osc2Semi=7, osc2Detune=4, cutoff=2000, reso=0.4, envAmt=0.2,
       lfoWave=LTRI, lfoRate=0.5, lfoPitch=0.08, aS=0.9, distType=HARD, distAmt=0.4, glide=0.05, mode=MONO)
preset("Lead", "Sync Scream", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=17, osc2Detune=0, oscMix=1.0, sync=1,
       cutoff=3000, reso=0.3, envAmt=0.2, lfoWave=SINE, lfoRate=0.25, lfoDepth=0.3, aS=0.9,
       distType=SOFT, distAmt=0.5, glide=0.03, mode=MONO)
preset("Lead", "Ring Lead", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=12, osc2Detune=7, ring=0.6, cutoff=2500, reso=0.3,
       envAmt=0.25, aS=0.85, distType=FOLD, distAmt=0.3, mode=MONO)
preset("Lead", "Grind Lead", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=20, drive=0.8, cutoff=1800, reso=0.5, envAmt=0.2,
       lfoWave=LSQ, lfoSync=1, lfoDiv=5, lfoDepth=0.2, aS=0.9, distType=HARD, distAmt=0.5, mode=MONO)
preset("Lead", "Chrome Lead", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=7, osc2Detune=2, oscMix=0.0, fm=0.4, fmEnv=0.3,
       cutoff=3500, reso=0.2, envAmt=0.2, aS=0.85, lfoWave=SINE, lfoRate=5.5, lfoPitch=0.01,
       combMix=0.25, combTune=12, combFb=0.7, combDamp=0.3, distAmt=0.35, glide=0.04, mode=MONO)

# ----------------------------------------------------------------------------- DRONE
preset("Drone", "Furnace Drone", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=-12, osc2Detune=30, noise=0.2, cutoff=400,
       reso=0.6, envAmt=0.0, lfoWave=SINE, lfoRate=0.1, lfoDepth=0.4, aA=1.0, aS=1.0, aR=2.5,
       combMix=0.3, combTune=7, combFb=0.85, combDamp=0.6, distAmt=0.4, mode=POLY)
preset("Drone", "Rust Drone", osc1Wave=SAW, osc2Wave=TRI, osc2Semi=7, osc2Detune=13, ring=0.5, fm=0.15,
       filterMode=BP, cutoff=900, reso=0.5, envAmt=0.0, lfoWave=SH, lfoRate=0.5, lfoDepth=0.3,
       aA=0.8, aS=1.0, aR=2.0, distAmt=0.3, mode=POLY)
preset("Drone", "Machine Hum", osc1Wave=SQ, osc2Wave=SQ, osc2Detune=3, sub=0.5, cutoff=300, reso=0.3, envAmt=0.0,
       lfoWave=SINE, lfoSync=1, lfoDiv=2, lfoDepth=0.2, aA=0.5, aS=1.0, aR=1.5,
       distType=CRUSH, distAmt=0.25, mode=MONO)
preset("Drone", "Metal Wind", osc1Wave=TRI, oscMix=0.0, noise=0.9, filterMode=BP, cutoff=1500, reso=0.75, envAmt=0.0,
       lfoWave=LTRI, lfoRate=0.2, lfoDepth=0.5, combMix=0.5, combTune=0, combFb=0.9, combDamp=0.2,
       aA=1.5, aS=1.0, aR=3.0, distAmt=0.2, mode=POLY)

# ----------------------------------------------------------------------------- FX
preset("FX", "Riser Metal", osc1Wave=SAW, osc2Wave=TRI, osc2Semi=7, osc2Detune=20, fm=0.3, ring=0.3,
       pitchEnv=-24, pitchDecay=2.0, cutoff=500, envAmt=0.6, fA=2.0, fD=1.0, fS=1.0,
       lfoWave=SINE, lfoSync=1, lfoDiv=6, lfoDepth=0.2, aA=1.0, aS=1.0, aR=1.0, distAmt=0.4, mode=MONO)
preset("FX", "Laser Drop", osc1Wave=SQ, oscMix=0.0, pitchEnv=48, pitchDecay=0.3, cutoff=3000, reso=0.5, envAmt=0.0,
       aD=0.6, aS=0.0, aR=0.3, distType=FOLD, distAmt=0.3, mode=MONO)
preset("FX", "Alarm Metal", osc1Wave=SQ, osc2Wave=TRI, osc2Semi=6, osc2Detune=0, ring=0.6, cutoff=2500, reso=0.3,
       envAmt=0.0, lfoWave=LSQ, lfoSync=1, lfoDiv=5, lfoPitch=0.05, aS=1.0, aR=0.5,
       distType=HARD, distAmt=0.45, mode=MONO)


# =============================================================================
#  v3 — GROOVE / RAVE / rolling bass (unison, matriz, FX)
# =============================================================================
# Fuentes matriz: off=0 lfo1=1 lfo2=2 modenv=3 ampenv=4 vel=5 note=6 rnd=7 wheel=8 at=9
# Destinos: off=0 cut=1 reso=2 pitch=3 bpitch=4 pw=5 mix=6 fm=7 ring=8 det=9 noise=10 sub=11
#           drive=12 restune=13 resmix=14 amp=15 pan=16
L1, L2, MENV, AENV, VEL, NOTE, RND, WHEEL, AT = 1, 2, 3, 4, 5, 6, 7, 8, 9
D_CUT, D_RESO, D_PITCH, D_BPITCH, D_PW, D_MIX, D_FM, D_RING, D_DET, D_NOISE, D_SUB, D_DRIVE, D_RTUNE, D_RMIX, D_AMP, D_PAN = range(1, 17)
# Divisiones: 4/1=0 2/1=1 1/1=2 1/2=3 1/4=4 1/8=5 1/16=6 1/32=7 1/4T=8 1/8T=9 1/16T=10 1/8D=11 1/16D=12
SAWD, SAWU = 4, 5

def M(*slots):
    d = {}
    for i, (src, dst, amt) in enumerate(slots, start=1):
        d[f"m{i}Src"], d[f"m{i}Dst"], d[f"m{i}Amt"] = src, dst, amt
    return d

def DLY(mix, div=11, fb=0.5, tone=0.45, ping=1):
    return dict(dlyMix=mix, dlyDiv=div, dlyFb=fb, dlyTone=tone, dlyPing=ping)

def REV(mix, size=0.55, damp=0.45, pre=0.01, lowcut=150):
    return dict(revMix=mix, revSize=size, revDamp=damp, revPre=pre, revLowCut=lowcut)

def CH(mix, rate=0.4, depth=0.5):
    return dict(chMix=mix, chRate=rate, chDepth=depth)

# ----------------------------------------------------------------------------- GROOVE (hipnótico / mental)
preset("Groove", "Hypno Stab", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=9, uniA=3, detA=0.22, spread=0.7,
       cutoff=700, reso=0.45, envAmt=0.45, fD=0.12, aD=0.25, aR=0.2, **PERC_AMP,
       **DLY(0.35, 11, 0.55, 0.4), **REV(0.25, 0.6), **M((RND, D_CUT, 0.15)), distAmt=0.3, mode=POLY)
preset("Groove", "Dub Chord Deep", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=7, osc2Detune=6, uniA=3, detA=0.25,
       filterMode=LP12, cutoff=600, reso=0.3, envAmt=0.35, fD=0.3, aD=0.5, aS=0.1, aR=0.45,
       **DLY(0.45, 11, 0.65, 0.3), **REV(0.35, 0.75, 0.5, 0.02, 300), **CH(0.3), distAmt=0.15, mode=POLY)
preset("Groove", "Mental Bleep", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=19, oscMix=0.0, fm=0.35, fmEnv=0.5,
       filterMode=BP, cutoff=2200, reso=0.5, envAmt=0.2, aD=0.09, aR=0.08, **PERC_AMP,
       **DLY(0.3, 12, 0.5), **REV(0.2, 0.5), **M((RND, D_BPITCH, 0.1)), distAmt=0.25, mode=POLY)
preset("Groove", "Tunnel Chord", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=12, uniA=5, detA=0.35, uniB=3, detB=0.3, spread=0.8,
       oscMix=0.35, cutoff=500, reso=0.55, envAmt=0.2, fD=0.4, aA=0.01, aD=0.6, aS=0.3, aR=0.5,
       lfo2Wave=LTRI, lfo2Rate=0.12, **M((L2, D_CUT, 0.25)), **REV(0.45, 0.85), **DLY(0.25, 4, 0.45), distAmt=0.2, mode=POLY)
preset("Groove", "Fog Stab", osc1Wave=SQ, oscMix=0.0, noise=0.2, filterMode=BP, cutoff=900, reso=0.6, envAmt=0.3, fD=0.1,
       aD=0.18, aR=0.15, **PERC_AMP, **REV(0.5, 0.9, 0.6, 0.03), distAmt=0.3, mode=POLY)
preset("Groove", "Spiral Pluck", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=12, osc2Detune=0, oscMix=0.0, fm=0.2, fmEnv=0.6,
       cutoff=1800, envAmt=0.4, fD=0.08, aD=0.3, aR=0.2, **PERC_AMP,
       lfoWave=LTRI, lfoSync=1, lfoDiv=1, **M((L1, D_FM, 0.3)), **DLY(0.4, 11, 0.7), distAmt=0.2, mode=POLY)
preset("Groove", "Groove Tom", osc1Wave=TRI, oscMix=0.0, pitchEnv=14, pitchDecay=0.07, cutoff=1500, envAmt=0.0, keyTrack=0.0,
       aD=0.25, aR=0.15, **PERC_AMP, **REV(0.15, 0.3), **M((VEL, D_CUT, 0.3)), distType=SOFT, distAmt=0.5, mode=POLY)
preset("Groove", "Hypno Perc Loop", osc1Wave=TRI, oscMix=0.0, noise=0.4, filterMode=BP, cutoff=1800, reso=0.7, envAmt=0.1,
       combMix=0.4, combTune=7, combFb=0.8, combDamp=0.3, aD=0.07, aR=0.06, **PERC_AMP,
       **DLY(0.25, 12, 0.45), **M((RND, D_CUT, 0.3), (RND, D_RTUNE, 0.2)), distAmt=0.3, mode=POLY)
preset("Groove", "Warp Bleep", osc1Wave=SQ, oscMix=0.0, pitchEnv=-12, pitchDecay=0.05, cutoff=3000, envAmt=0.0,
       aD=0.1, aR=0.08, **PERC_AMP, **DLY(0.35, 11, 0.5), distType=HARD, distAmt=0.3, mode=POLY)
preset("Groove", "Ritual Bell", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=7, osc2Detune=0, ring=0.5, fm=0.15,
       cutoff=4000, envAmt=0.1, aD=1.0, aR=0.8, **PERC_AMP, combMix=0.25, combTune=12, combFb=0.8,
       **REV(0.35, 0.7), distAmt=0.15, mode=POLY)
preset("Groove", "Sonar Ping", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=24, oscMix=0.0, fm=0.1, fmEnv=0.3, cutoff=4000, envAmt=0.0,
       aD=0.5, aR=0.4, **PERC_AMP, **DLY(0.5, 11, 0.75, 0.3), **REV(0.4, 0.8), distAmt=0.1, mode=POLY)
preset("Groove", "Dark Organ", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=-12, osc2Detune=0, uniA=3, detA=0.1, filterMode=LP24,
       cutoff=900, reso=0.2, envAmt=0.1, aS=0.9, aR=0.3, lfo2Wave=SINE, lfo2Rate=0.2, **M((L2, D_CUT, 0.1)),
       **CH(0.5, 0.3), **REV(0.3, 0.6), distAmt=0.25, mode=POLY)
preset("Groove", "Concrete Haze", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=-12, osc2Detune=30, noise=0.2, filterMode=BP,
       cutoff=500, reso=0.7, envAmt=0.0, lfoWave=SH, lfoSync=1, lfoDiv=6, lfoDepth=0.25,
       aA=0.5, aS=1.0, aR=2.0, **REV(0.5, 0.9, 0.55), distAmt=0.35, mode=POLY)
preset("Groove", "Groove Stab Short", osc1Wave=SAW, osc2Wave=PULSE, pw=0.3, osc2Detune=12, cutoff=1400, reso=0.35,
       envAmt=0.35, fD=0.06, aD=0.12, aR=0.08, **PERC_AMP, **DLY(0.2, 12, 0.4), **M((VEL, D_CUT, 0.4)),
       distType=HARD, distAmt=0.35, mode=POLY)

# ----------------------------------------------------------------------------- RAVE (hard groove / rave revival)
preset("Rave", "Hard Groove Stab", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=12, uniA=7, detA=0.55, uniB=5, detB=0.5, spread=0.9,
       oscMix=0.35, cutoff=2500, reso=0.25, envAmt=0.3, fD=0.2, aD=0.35, aS=0.1, aR=0.25,
       **REV(0.3, 0.55), **DLY(0.15, 11, 0.4), distType=SOFT, distAmt=0.35, mode=POLY)
preset("Rave", "Rave Hoover", osc1Wave=PULSE, pw=0.25, osc2Wave=SAW, osc2Semi=-12, uniA=7, detA=0.7, uniB=5, detB=0.6,
       spread=0.9, cutoff=3000, reso=0.15, envAmt=0.2, pitchEnv=-5, pitchDecay=0.15, glide=0.1,
       aA=0.01, aS=0.9, aR=0.3, **CH(0.5, 0.5, 0.6), **M((MENV, D_PITCH, -0.05)), distAmt=0.45, mode=MONO)
preset("Rave", "Supersaw Rave", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=12, uniA=7, detA=0.6, uniB=7, detB=0.45, spread=1.0,
       oscMix=0.3, cutoff=5000, reso=0.15, envAmt=0.15, aA=0.005, aD=0.5, aS=0.6, aR=0.4,
       **REV(0.3, 0.6), **CH(0.2), distAmt=0.2, mode=POLY)
preset("Rave", "Trance Gate", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=7, uniA=7, detA=0.5, uniB=5, detB=0.4, spread=0.9,
       oscMix=0.3, cutoff=2800, reso=0.2, envAmt=0.0, aS=1.0, aR=0.3, lfoWave=LSQ, lfoSync=1, lfoDiv=6,
       **M((L1, D_AMP, 0.95)), **DLY(0.2, 11, 0.4), **REV(0.25, 0.6), distAmt=0.25, mode=POLY)
preset("Rave", "Rave Chord 90s", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=5, osc2Detune=4, uniA=3, detA=0.3, cutoff=1800, reso=0.3,
       envAmt=0.35, fD=0.25, aD=0.3, aR=0.2, **PERC_AMP, **REV(0.25, 0.5), distType=HARD, distAmt=0.3, mode=POLY)
preset("Rave", "Panic Alarm", osc1Wave=SQ, osc2Wave=SAW, osc2Semi=6, osc2Detune=0, ring=0.3, cutoff=2500, reso=0.3,
       envAmt=0.0, aS=1.0, aR=0.3, lfoWave=LSQ, lfoSync=1, lfoDiv=5, lfoPitch=0.04, **REV(0.2, 0.5),
       distType=HARD, distAmt=0.5, mode=MONO)
preset("Rave", "Euphoric Lead", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=7, uniA=5, detA=0.4, uniB=3, detB=0.3, oscMix=0.35,
       cutoff=3500, reso=0.25, envAmt=0.2, aS=0.9, aR=0.3, glide=0.05,
       lfo2Wave=SINE, lfo2Rate=5.5, **M((L2, D_PITCH, 0.006), (WHEEL, D_CUT, 0.4)),
       **DLY(0.3, 11, 0.5), **REV(0.35, 0.65), distAmt=0.3, mode=MONO)
preset("Rave", "Hard Groove Bleep", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=12, osc2Detune=0, fm=0.2, cutoff=2500, reso=0.3,
       envAmt=0.2, aD=0.08, aR=0.06, **PERC_AMP, **DLY(0.25, 12, 0.45), **M((RND, D_BPITCH, 0.2)),
       distType=HARD, distAmt=0.3, mode=POLY)
preset("Rave", "Rave Organ Stab", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=12, osc2Detune=0, uniA=3, detA=0.15, filterMode=LP12,
       cutoff=1500, reso=0.2, envAmt=0.25, fD=0.15, aD=0.25, aR=0.15, **PERC_AMP, **CH(0.25), **REV(0.2, 0.45),
       distAmt=0.4, mode=POLY)

# ----------------------------------------------------------------------------- BASS rodantes / groove
preset("Bass", "Rolling Bass", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=-12, osc2Detune=0, cutoff=260, reso=0.35, drive=0.5,
       envAmt=0.45, fD=0.09, aD=0.12, aS=0.2, aR=0.05, distType=SOFT, distAmt=0.5, mode=MONO)
preset("Bass", "Offbeat Sub", osc1Wave=TRI, oscMix=0.0, sub=0.4, cutoff=400, envAmt=0.1, pitchEnv=7, pitchDecay=0.01,
       aD=0.2, aS=0.3, aR=0.05, keyTrack=0.0, distAmt=0.35, mode=MONO)
preset("Bass", "Rumble Sub Bass", osc1Wave=TRI, oscMix=0.0, sub=0.8, noise=0.05, cutoff=140, reso=0.3, drive=0.7,
       envAmt=0.1, aS=1.0, aR=0.2, **REV(0.3, 0.5, 0.8, 0.0, 20), distType=SOFT, distAmt=0.55, mode=MONO)
preset("Bass", "Dist Rolling", osc1Wave=SAW, oscMix=0.0, fm=0.1, cutoff=380, reso=0.5, drive=0.8, envAmt=0.5, fD=0.07,
       aD=0.1, aS=0.1, aR=0.05, distType=FOLD, distAmt=0.4, mode=MONO)
preset("Bass", "Reese Groove", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=0, uniA=5, detA=0.5, uniB=5, detB=0.55, spread=0.5,
       cutoff=500, reso=0.3, drive=0.6, envAmt=0.1, lfo2Wave=SINE, lfo2Rate=0.15, **M((L2, D_CUT, 0.3)),
       aS=1.0, aR=0.2, distAmt=0.5, mode=MONO)
preset("Bass", "Groove Bass Pluck", osc1Wave=SQ, oscMix=0.0, filterMode=LP12, cutoff=180, reso=0.6, envAmt=0.6, fD=0.1,
       aD=0.18, aR=0.08, **PERC_AMP, **M((VEL, D_CUT, 0.3)), distAmt=0.35, mode=MONO)

# ----------------------------------------------------------------------------- PERC groove
preset("Perc", "Groove Tom Low", osc1Wave=TRI, oscMix=0.0, pitchEnv=18, pitchDecay=0.09, cutoff=1200, envAmt=0.0,
       keyTrack=0.0, aD=0.4, aR=0.2, **PERC_AMP, **REV(0.12, 0.3), distType=SOFT, distAmt=0.4, mode=POLY)
preset("Perc", "Metal Conga", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=7, osc2Detune=15, oscMix=0.0, fm=0.4, fmEnv=0.5,
       pitchEnv=10, pitchDecay=0.03, filterMode=BP, cutoff=1200, reso=0.5, envAmt=0.1, aD=0.18, aR=0.1, **PERC_AMP, mode=POLY)
preset("Perc", "Tick Noise", osc1Wave=TRI, oscMix=0.0, noise=1.0, filterMode=HP, cutoff=7000, reso=0.2, envAmt=0.0,
       aD=0.025, aR=0.02, **PERC_AMP, distAmt=0.2, mode=POLY)

# ----------------------------------------------------------------------------- FX
preset("FX", "Noise Sweep Up", osc1Wave=TRI, oscMix=0.0, noise=1.0, filterMode=BP, cutoff=300, reso=0.6, envAmt=1.0,
       fA=4.0, fS=1.0, aA=0.5, aS=1.0, aR=1.0, **REV(0.4, 0.75), distAmt=0.2, mode=MONO)
preset("FX", "Down Lifter", osc1Wave=SAW, oscMix=0.0, noise=0.7, cutoff=200, reso=0.4, envAmt=1.0, fA=0.001, fD=3.0, fS=0.0,
       aD=3.0, aR=1.0, **PERC_AMP, **REV(0.4, 0.8), distAmt=0.3, mode=MONO)
preset("FX", "Siren Rise", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=7, pitchEnv=-24, pitchDecay=3.0, cutoff=2500, envAmt=0.0,
       lfoWave=SINE, lfoRate=6.0, lfoPitch=0.01, aS=1.0, aR=0.6, **DLY(0.3, 11, 0.5), **REV(0.3, 0.6),
       distType=HARD, distAmt=0.35, mode=MONO)


# =============================================================================
#  v3.1 — HARD GROOVE / GROOVE PERC / OLD SCHOOL
#  Referencia: "modern old school techno" (SHDW & Obscure Shape / Mutual Rytm),
#  BCCO, Future.666, Überkikz: herramientas de loop, stabs cortos, percusión sintetizada.
# =============================================================================
HG, GP, OS = "Hard Groove", "Groove Perc", "Old School"

# ----------------------------------------------------------------------------- HARD GROOVE
preset(HG, "Broom Stab", osc1Wave=PULSE, pw=0.2, osc2Wave=SAW, osc2Semi=12, osc2Detune=6, cutoff=900, reso=0.55,
       envAmt=0.5, fD=0.09, aD=0.15, aR=0.1, **PERC_AMP, **DLY(0.15, 12, 0.4), distType=HARD, distAmt=0.3, mode=POLY)
preset(HG, "Hyperdrive Riff", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=10, uniA=3, detA=0.3, cutoff=1200, reso=0.6,
       envAmt=0.4, fD=0.07, aD=0.2, aS=0.4, aR=0.06, lfoWave=SH, lfoSync=1, lfoDiv=6, lfoDepth=0.2,
       distType=SOFT, distAmt=0.45, mode=MONO)
preset(HG, "Federation Stab", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=7, osc2Detune=0, ring=0.2, filterMode=BP, cutoff=1500,
       reso=0.5, envAmt=0.3, fD=0.08, aD=0.12, aR=0.1, **PERC_AMP, **REV(0.25, 0.4), **DLY(0.2, 11, 0.45),
       distAmt=0.35, mode=POLY)
preset(HG, "Loop Tool Bleep", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=12, osc2Detune=0, oscMix=0.0, fm=0.5, fmEnv=0.3,
       cutoff=3000, envAmt=0.1, aD=0.06, aR=0.05, **PERC_AMP, **M((RND, D_BPITCH, 0.15), (RND, D_FM, 0.2)),
       distType=HARD, distAmt=0.25, mode=POLY)
preset(HG, "Siren Hit", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=7, pitchEnv=-7, pitchDecay=0.3, cutoff=2500, reso=0.3,
       envAmt=0.1, aD=0.5, aR=0.3, **PERC_AMP, **REV(0.3, 0.55), distType=HARD, distAmt=0.35, mode=MONO)
preset(HG, "Organ Hit 93", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=19, osc2Detune=0, uniA=3, detA=0.1, oscMix=0.3,
       filterMode=LP12, cutoff=2000, reso=0.15, envAmt=0.2, fD=0.1, aD=0.2, aR=0.12, **PERC_AMP, **CH(0.3),
       distType=SOFT, distAmt=0.5, mode=POLY)
preset(HG, "Phaser Loop Stab", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=8, uniA=5, detA=0.3, cutoff=1600, reso=0.4,
       envAmt=0.3, fD=0.12, aD=0.2, aR=0.15, **PERC_AMP, **CH(0.8, 0.15, 1.0), distAmt=0.3, mode=POLY)
preset(HG, "Hoover Hit", osc1Wave=PULSE, pw=0.25, osc2Wave=SAW, osc2Semi=-12, uniA=7, detA=0.7, uniB=5, detB=0.6, spread=0.9,
       pitchEnv=-12, pitchDecay=0.08, cutoff=3000, envAmt=0.1, aD=0.4, aR=0.3, **PERC_AMP, **CH(0.4, 0.5, 0.6),
       distType=SOFT, distAmt=0.4, mode=POLY)
preset(HG, "Rave Bleep 155", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=12, osc2Detune=0, filterMode=HP, cutoff=600, reso=0.2,
       envAmt=0.0, aD=0.07, aR=0.05, **PERC_AMP, **DLY(0.3, 12, 0.5), distAmt=0.3, mode=POLY)
preset(HG, "Dirty Chord Loop", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=3, osc2Detune=5, uniA=3, detA=0.25, cutoff=800,
       reso=0.4, drive=0.6, envAmt=0.35, fD=0.1, aD=0.18, aR=0.12, **PERC_AMP, **DLY(0.25, 11, 0.45),
       distType=CRUSH, distAmt=0.25, distMix=0.6, mode=POLY)
preset(HG, "Mutual Dub", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=7, osc2Detune=6, uniA=3, detA=0.2, filterMode=LP12,
       cutoff=700, reso=0.35, envAmt=0.3, fD=0.2, aD=0.4, aS=0.05, aR=0.3,
       **DLY(0.55, 11, 0.7, 0.3), **REV(0.4, 0.8, 0.5, 0.02, 400), distAmt=0.2, mode=POLY)
preset(HG, "Shadow Pad", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=7, uniA=7, detA=0.35, uniB=5, detB=0.3, spread=0.9,
       oscMix=0.35, cutoff=600, reso=0.3, envAmt=0.1, aA=0.8, aS=1.0, aR=2.0,
       lfo2Wave=LTRI, lfo2Rate=0.08, **M((L2, D_CUT, 0.3)), **REV(0.5, 0.9), **CH(0.4), distAmt=0.15, mode=POLY)
preset(HG, "Obscure Arp", osc1Wave=TRI, osc2Wave=SAW, osc2Semi=12, osc2Detune=4, oscMix=0.3, fm=0.15, cutoff=2000,
       reso=0.3, envAmt=0.4, fD=0.1, aD=0.2, aR=0.15, **PERC_AMP, **DLY(0.35, 12, 0.55), **REV(0.25, 0.6),
       distAmt=0.2, mode=POLY)
preset(HG, "Stomp Stab", osc1Wave=SQ, osc2Wave=SAW, osc2Detune=12, pitchEnv=12, pitchDecay=0.015, cutoff=1800,
       envAmt=0.2, fD=0.05, aD=0.1, aR=0.08, **PERC_AMP, distType=HARD, distAmt=0.55, mode=POLY)
preset(HG, "Warehouse Chord Mem", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=5, osc2Detune=4, uniA=3, detA=0.25, cutoff=1300,
       reso=0.45, envAmt=0.35, fD=0.12, aD=0.2, aR=0.15, **PERC_AMP, **REV(0.2, 0.45), distType=HARD, distAmt=0.35, mode=POLY)
preset(HG, "Turbo Bleep", osc1Wave=PULSE, pw=0.1, oscMix=0.0, pitchEnv=24, pitchDecay=0.01, cutoff=4000, envAmt=0.0,
       aD=0.05, aR=0.04, **PERC_AMP, **DLY(0.25, 6, 0.4), distAmt=0.3, mode=POLY)

# ----------------------------------------------------------------------------- GROOVE PERC (percusión para loops)
preset(GP, "HG Conga Hi", osc1Wave=TRI, oscMix=0.0, pitchEnv=8, pitchDecay=0.02, cutoff=3000, envAmt=0.0, keyTrack=0.0,
       aD=0.12, aR=0.08, **PERC_AMP, distType=SOFT, distAmt=0.3, mode=POLY)
preset(GP, "HG Conga Low", osc1Wave=TRI, oscMix=0.0, pitchEnv=10, pitchDecay=0.03, cutoff=2500, envAmt=0.0, keyTrack=0.0,
       aD=0.22, aR=0.12, **PERC_AMP, distType=SOFT, distAmt=0.3, mode=POLY)
preset(GP, "Bongo Snap", osc1Wave=TRI, oscMix=0.0, noise=0.15, pitchEnv=6, pitchDecay=0.01, filterMode=LP12, cutoff=3500,
       reso=0.3, envAmt=0.1, keyTrack=0.0, aD=0.08, aR=0.05, **PERC_AMP, distAmt=0.25, mode=POLY)
preset(GP, "Cowbell 808", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=7, osc2Detune=0, filterMode=BP, cutoff=2600, reso=0.4,
       envAmt=0.0, keyTrack=0.0, aD=0.25, aR=0.15, **PERC_AMP, distType=SOFT, distAmt=0.2, mode=POLY)
preset(GP, "Ride Metal", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=13, osc2Detune=37, ring=1.0, noise=0.35, filterMode=HP,
       cutoff=5000, reso=0.2, envAmt=0.0, aD=0.6, aR=0.4, **PERC_AMP, distAmt=0.2, mode=POLY)
preset(GP, "Open Hat Groove", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=19, osc2Detune=41, ring=0.8, noise=1.0, filterMode=HP,
       cutoff=6500, reso=0.25, envAmt=0.0, aD=0.22, aR=0.12, **PERC_AMP, distAmt=0.2, mode=POLY)
preset(GP, "Closed Hat Tight", osc1Wave=TRI, oscMix=0.0, noise=1.0, filterMode=HP, cutoff=8000, reso=0.3, envAmt=0.0,
       aD=0.03, aR=0.025, **PERC_AMP, distType=HARD, distAmt=0.2, mode=POLY)
preset(GP, "Shaker Groove", osc1Wave=TRI, oscMix=0.0, noise=0.9, filterMode=BP, cutoff=6000, reso=0.2, envAmt=0.0,
       aA=0.008, aD=0.06, aS=0.0, aR=0.04, distAmt=0.15, mode=POLY)
preset(GP, "Rim Click", osc1Wave=TRI, oscMix=0.0, noise=0.3, pitchEnv=12, pitchDecay=0.005, filterMode=BP, cutoff=2000,
       reso=0.6, envAmt=0.1, keyTrack=0.3, aD=0.03, aR=0.02, **PERC_AMP, distType=HARD, distAmt=0.3, mode=POLY)
preset(GP, "Noise Clap", osc1Wave=TRI, oscMix=0.0, noise=1.0, filterMode=BP, cutoff=1300, reso=0.3, envAmt=0.0,
       keyTrack=0.0, aD=0.12, aR=0.1, **PERC_AMP, **REV(0.3, 0.35, 0.4, 0.005, 300), distType=SOFT, distAmt=0.3, mode=POLY)
preset(GP, "Tribal Tom", osc1Wave=TRI, oscMix=0.0, pitchEnv=20, pitchDecay=0.08, cutoff=2000, envAmt=0.0, keyTrack=0.0,
       aD=0.35, aR=0.2, **PERC_AMP, **REV(0.1, 0.3), distType=SOFT, distAmt=0.45, mode=POLY)
preset(GP, "Floor Tom Rumble", osc1Wave=TRI, oscMix=0.0, pitchEnv=12, pitchDecay=0.12, cutoff=1200, envAmt=0.0,
       keyTrack=0.0, aD=0.5, aR=0.3, **PERC_AMP, **REV(0.2, 0.4, 0.7, 0.0, 50), distType=SOFT, distAmt=0.5, mode=POLY)
preset(GP, "Tom Roll FM", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=7, osc2Detune=23, oscMix=0.0, fm=0.25, fmEnv=0.5,
       pitchEnv=16, pitchDecay=0.05, cutoff=3000, envAmt=0.0, aD=0.25, aR=0.15, **PERC_AMP, distAmt=0.3, mode=POLY)
preset(GP, "Tin Perc", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=17, osc2Detune=31, ring=0.8, filterMode=HP, cutoff=1200,
       reso=0.2, envAmt=0.0, keyTrack=0.5, aD=0.08, aR=0.06, **PERC_AMP, distType=HARD, distAmt=0.25, mode=POLY)
preset(GP, "Wood Block", osc1Wave=SQ, oscMix=0.0, noise=0.15, pitchEnv=12, pitchDecay=0.004, filterMode=BP,
       cutoff=900, reso=0.3, envAmt=0.0, keyTrack=1.0, combMix=0.6, combTune=12, combFb=0.7, combDamp=0.5, aD=0.05, aR=0.04, **PERC_AMP, distAmt=0.2, mode=POLY)
preset(GP, "Snare Raw", osc1Wave=TRI, oscMix=0.0, noise=0.7, pitchEnv=12, pitchDecay=0.02, cutoff=6000, envAmt=0.0,
       keyTrack=0.0, aD=0.15, aR=0.1, **PERC_AMP, **REV(0.15, 0.35), distType=HARD, distAmt=0.3, mode=POLY)

# ----------------------------------------------------------------------------- OLD SCHOOL (modern old school techno)
preset(OS, "Surgeon Loop", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=1, osc2Detune=0, ring=0.3, filterMode=BP, cutoff=1100,
       reso=0.6, envAmt=0.3, fD=0.06, aD=0.1, aR=0.08, **PERC_AMP, lfoWave=SH, lfoSync=1, lfoDiv=6, lfoDepth=0.2,
       distType=HARD, distAmt=0.5, mode=POLY)
preset(OS, "Birmingham Stab", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=-12, osc2Detune=30, cutoff=1000, reso=0.6,
       envAmt=0.5, fD=0.08, aD=0.15, aR=0.1, **PERC_AMP, distType=HARD, distAmt=0.4, mode=POLY)
preset(OS, "Detroit Strings 94", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=8, uniA=5, detA=0.3, cutoff=2500, reso=0.15,
       envAmt=0.1, aA=0.15, aS=0.8, aR=0.8, **CH(0.6, 0.35, 0.7), **REV(0.4, 0.7), distAmt=0.1, mode=POLY)
preset(OS, "Mentasm Stab", osc1Wave=PULSE, pw=0.3, osc2Wave=SAW, osc2Semi=-12, uniA=7, detA=0.75, uniB=3, detB=0.5,
       spread=0.9, pitchEnv=-12, pitchDecay=0.2, cutoff=3500, envAmt=0.1, aD=0.6, aS=0.3, aR=0.3,
       **CH(0.4, 0.5, 0.6), distType=SOFT, distAmt=0.55, mode=MONO)
preset(OS, "Bleep Hit 91", osc1Wave=TRI, osc2Wave=TRI, osc2Semi=12, osc2Detune=0, oscMix=0.0, fm=0.05,
       filterMode=HP, cutoff=70, reso=0.1, envAmt=0.0, keyTrack=0.0, aD=0.12, aR=0.1, **PERC_AMP, **DLY(0.3, 11, 0.45), distAmt=0.15, mode=POLY)
preset(OS, "Sequence 909", osc1Wave=SQ, oscMix=0.0, cutoff=900, reso=0.7, envAmt=0.6, fD=0.05, aD=0.1, aS=0.2, aR=0.05,
       glide=0.03, distType=SOFT, distAmt=0.4, mode=MONO)
preset(OS, "Formant Stab", osc1Wave=SAW, osc2Wave=SAW, osc2Semi=12, osc2Detune=6, filterMode=BP, cutoff=800, reso=0.85,
       envAmt=0.2, fD=0.1, aD=0.15, aR=0.1, **PERC_AMP, **M((RND, D_CUT, 0.2)), **REV(0.2, 0.45), distAmt=0.3, mode=POLY)
preset(OS, "Industrial Loop Hit", osc1Wave=SAW, osc2Wave=SQ, osc2Semi=5, ring=0.4, noise=0.3, cutoff=2000, envAmt=0.2,
       combMix=0.5, combTune=0, combFb=0.9, combDamp=0.2, aD=0.2, aR=0.15, **PERC_AMP,
       distType=CRUSH, distAmt=0.35, mode=POLY)
preset(OS, "Chord Hit Rave 92", osc1Wave=SQ, osc2Wave=SQ, osc2Semi=7, osc2Detune=0, uniA=3, detA=0.2, sub=0.3,
       cutoff=2000, reso=0.3, envAmt=0.3, fD=0.15, aD=0.25, aR=0.15, **PERC_AMP, **REV(0.25, 0.5),
       distType=HARD, distAmt=0.3, mode=POLY)
preset(OS, "Tunnel Siren", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=12, cutoff=3000, reso=0.25, envAmt=0.0, aS=1.0, aR=0.5,
       lfoWave=LTRI, lfoRate=0.25, lfoPitch=0.5, **DLY(0.3, 4, 0.5), **REV(0.4, 0.7), distType=HARD, distAmt=0.35, mode=MONO)

# ----------------------------------------------------------------------------- BASS hard groove
preset("Bass", "Hard Groove Bass", osc1Wave=SAW, oscMix=0.0, cutoff=300, reso=0.45, envAmt=0.5, fD=0.06,
       aD=0.1, aS=0.15, aR=0.04, distType=SOFT, distAmt=0.55, mode=MONO)
preset("Bass", "Kick Bass Groove", osc1Wave=TRI, oscMix=0.0, sub=0.5, pitchEnv=24, pitchDecay=0.03, cutoff=900,
       envAmt=0.0, keyTrack=0.0, aD=0.25, aR=0.08, **PERC_AMP, distType=SOFT, distAmt=0.6, mode=MONO)
preset("Bass", "Rumble Groove", osc1Wave=TRI, oscMix=0.0, sub=0.9, cutoff=110, reso=0.2, drive=0.8, envAmt=0.1,
       aS=1.0, aR=0.2, **REV(0.35, 0.45, 0.85, 0.0, 20), distType=SOFT, distAmt=0.6, mode=MONO)
preset("Bass", "Offbeat Pulse Bass", osc1Wave=PULSE, pw=0.3, oscMix=0.0, filterMode=LP12, cutoff=220, reso=0.5,
       envAmt=0.45, fD=0.05, aD=0.12, aR=0.05, **PERC_AMP, distAmt=0.4, mode=MONO)
preset("Bass", "Grind Roll", osc1Wave=SAW, osc2Wave=SAW, osc2Detune=12, uniA=3, detA=0.2, cutoff=350, reso=0.4, drive=0.8,
       envAmt=0.4, fD=0.05, aD=0.08, aS=0.2, aR=0.04, distType=FOLD, distAmt=0.35, mode=MONO)
preset("Bass", "Old School Sub Stab", osc1Wave=SQ, oscMix=0.0, sub=0.6, cutoff=500, envAmt=0.2, fD=0.08,
       aD=0.15, aR=0.08, **PERC_AMP, distAmt=0.35, mode=MONO)

# ----------------------------------------------------------------------------- FX groove
preset("FX", "Riser Groove Noise", osc1Wave=TRI, oscMix=0.0, noise=1.0, filterMode=BP, cutoff=250, reso=0.5, envAmt=1.0,
       fA=6.0, fS=1.0, lfoWave=LTRI, lfoSync=1, lfoDiv=6, lfoDepth=0.3, aA=2.0, aS=1.0, aR=1.0,
       **REV(0.4, 0.7), distAmt=0.2, mode=MONO)
preset("FX", "Impact Down", osc1Wave=TRI, oscMix=0.0, noise=0.3, pitchEnv=36, pitchDecay=0.6, cutoff=2000, envAmt=0.3,
       fD=0.8, keyTrack=0.0, aD=1.5, aR=0.8, **PERC_AMP, **REV(0.5, 0.8), distType=SOFT, distAmt=0.6, mode=MONO)


def write_all(root, levels):
    count = 0
    for (cat, name), params in P.items():
        params = dict(params)
        key = f"{cat}/{name}"
        params["volume"] = round(levels.get(key, params.get("volume", -6.0)), 1)
        d = os.path.join(root, cat)
        os.makedirs(d, exist_ok=True)
        lines = ['<?xml version="1.0" encoding="UTF-8"?>',
                 f'<RawTekPreset name={quoteattr(name)} category={quoteattr(cat)} author="Panic Audio" version="3">']
        for k, v in params.items():
            lines.append(f'  <PARAM id="{k}" value="{v}"/>')
        lines.append('</RawTekPreset>')
        with open(os.path.join(d, name + ".rtkpreset"), "w") as f:
            f.write("\n".join(lines) + "\n")
        count += 1
    return count


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.join(here, "..", "Presets")
    levels = {}
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            levels = json.load(f)
    n = write_all(root, levels)
    print(f"{n} presets escritos en {os.path.normpath(root)}")

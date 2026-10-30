#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <array>
#include <vector>

#include "Wavetables.h"

// ============================================================================
//  PANIC RAW TEK — motor de síntesis (sin dependencias de JUCE)
//  2 osciladores con unison estéreo, sub, ruido, FM/ring/sync, ladder,
//  resonador metálico, 2 LFOs, matriz de modulación de 8 slots.
// ============================================================================

namespace rawtek
{
constexpr float kPi    = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;
constexpr int   kMaxUnison = 7;
constexpr int   kModSlots  = 8;

inline float fastTanh (float x)
{
    if (x < -3.0f) return -1.0f;
    if (x >  3.0f) return  1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float midiToHz (float note) { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }

// Paneo de potencia constante con el centro en ganancia 1
inline void panGains (float pan, float& l, float& r)
{
    const float a = (std::clamp (pan, -1.0f, 1.0f) + 1.0f) * (kPi * 0.25f);
    l = std::cos (a) * 1.41421356f;
    r = std::sin (a) * 1.41421356f;
}

// ---------------------------------------------------------------------------
struct Rng
{
    uint32_t state = 0x12345678u;
    float next() // [-1, 1]
    {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return (float) (int32_t) state * (1.0f / 2147483648.0f);
    }
    float next01() { return next() * 0.5f + 0.5f; }
};

// ---------------------------------------------------------------------------
//  Oscilador PolyBLEP
// ---------------------------------------------------------------------------
enum class Wave { Saw = 0, Square, Pulse, Triangle, Table };

inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt;             return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

inline float renderWave (Wave w, float phase, float dt, float pw)
{
    switch (w)
    {
        case Wave::Saw:
            return 2.0f * phase - 1.0f - polyBlep (phase, dt);
        case Wave::Square:
            pw = 0.5f; [[fallthrough]];
        case Wave::Pulse:
        {
            float v = phase < pw ? 1.0f : -1.0f;
            v += polyBlep (phase, dt);
            float t2 = phase - pw; if (t2 < 0.0f) t2 += 1.0f;
            v -= polyBlep (t2, dt);
            return v;
        }
        case Wave::Triangle:
        default:
            return 1.0f - 4.0f * std::abs (phase - 0.5f);
    }
}

// ---------------------------------------------------------------------------
//  Envolvente ADSR
// ---------------------------------------------------------------------------
struct Envelope
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float value = 0.0f;
    float attackInc = 1.0f, decayCoef = 0.99f, sustain = 1.0f, releaseCoef = 0.99f;

    void set (double sr, float a, float d, float s, float r)
    {
        attackInc   = 1.0f / (float) (std::max (0.0005f, a) * sr);
        decayCoef   = std::exp (-5.0f / (float) (std::max (0.001f, d) * sr));
        releaseCoef = std::exp (-5.0f / (float) (std::max (0.001f, r) * sr));
        sustain     = s;
    }

    void gateOn()  { stage = Attack; }
    void gateOff() { if (stage != Idle) stage = Release; }
    void reset()   { stage = Idle; value = 0.0f; }
    bool isActive() const { return stage != Idle; }

    float next()
    {
        switch (stage)
        {
            case Attack:
                value += attackInc;
                if (value >= 1.0f) { value = 1.0f; stage = Decay; }
                break;
            case Decay:
                value = sustain + (value - sustain) * decayCoef;
                if (std::abs (value - sustain) < 1.0e-4f) { value = sustain; stage = Sustain; }
                break;
            case Sustain:
                value += (sustain - value) * 0.002f;
                if (sustain <= 0.0f && value < 1.0e-5f) { value = 0.0f; stage = Idle; }
                break;
            case Release:
                value *= releaseCoef;
                if (value < 1.0e-5f) { value = 0.0f; stage = Idle; }
                break;
            case Idle:
            default:
                break;
        }
        return value;
    }
};

// ---------------------------------------------------------------------------
//  Filtro ladder ZDF con saturación. Modos: LP24, LP12, BP, HP
// ---------------------------------------------------------------------------
struct LadderFilter
{
    float s[4] = { 0, 0, 0, 0 };
    void reset() { s[0] = s[1] = s[2] = s[3] = 0.0f; }

    float process (float x, float g, float k, int mode)
    {
        const float G  = g / (1.0f + g);
        const float iG = 1.0f - G;
        const float G2 = G * G, G3 = G2 * G, G4 = G3 * G;

        const float S = G3 * s[0] * iG + G2 * s[1] * iG + G * s[2] * iG + s[3] * iG;
        const float y4est = (G4 * x + S) / (1.0f + k * G4);

        const float a0 = fastTanh (x - k * y4est);
        float u = a0;
        float a[4];
        for (int i = 0; i < 4; ++i)
        {
            const float v = (u - s[i]) * G;
            const float y = v + s[i];
            s[i] = y + v;
            u = y;
            a[i] = y;
        }

        const float comp = 1.0f + 0.45f * k;
        switch (mode)
        {
            case 0:  return a[3] * comp;
            case 1:  return a[1] * comp;
            case 2:  return 2.0f * (a[0] - a[1]) * comp;
            default: return a0 - 4.0f * a[0] + 6.0f * a[1] - 4.0f * a[2] + a[3];
        }
    }
};

// ---------------------------------------------------------------------------
//  LFO global
// ---------------------------------------------------------------------------
struct Lfo
{
    float phase = 0.0f, sh = 0.0f, smoothed = 0.0f;
    void reset() { phase = 0.0f; sh = 0.0f; smoothed = 0.0f; }

    float tick (int wave, float inc, Rng& rng)
    {
        phase += inc;
        if (phase >= 1.0f) { phase -= std::floor (phase); sh = rng.next(); }
        float v;
        switch (wave)
        {
            case 0:  v = std::sin (kTwoPi * phase); break;
            case 1:  v = 1.0f - 4.0f * std::abs (phase - 0.5f); break;
            case 2:  v = phase < 0.5f ? 1.0f : -1.0f; break;
            case 3:  v = sh; break;
            case 4:  v = 1.0f - 2.0f * phase; break;   // saw down
            default: v = 2.0f * phase - 1.0f; break;   // saw up
        }
        smoothed += (v - smoothed) * 0.02f;
        return smoothed;
    }
};

// ---------------------------------------------------------------------------
//  Matriz de modulación
// ---------------------------------------------------------------------------
enum ModSrc { SrcOff = 0, SrcLfo1, SrcLfo2, SrcModEnv, SrcAmpEnv, SrcVelocity, SrcNote, SrcRandom, SrcModWheel, SrcAftertouch, NumModSrc };
enum ModDst { DstOff = 0, DstCutoff, DstReso, DstPitch, DstOscBPitch, DstPW, DstOscMix, DstFM, DstRing, DstDetune,
              DstNoise, DstSub, DstDrive, DstCombTune, DstCombMix, DstAmp, DstPan,
              DstWtPosA, DstWtPosB, NumModDst };

struct ModSlot { int src = 0, dst = 0; float amt = 0.0f; };

// ---------------------------------------------------------------------------
//  Parámetros del motor
// ---------------------------------------------------------------------------
struct EngineParams
{
    Wave  osc1Wave = Wave::Saw, osc2Wave = Wave::Saw;
    float pulseWidth = 0.25f;
    int   wtTableA = 0, wtTableB = 0;
    float wtPosA = 0.0f, wtPosB = 0.0f;
    float osc2Semi = 0.0f, osc2Detune = 0.0f;
    float oscMix = 0.5f, subLevel = 0.0f, noiseLevel = 0.0f;

    int   uniA = 1, uniB = 1;
    float detA = 0.2f, detB = 0.2f, spread = 0.5f;

    float fmAmount = 0.0f, fmEnv = 0.0f, ringMod = 0.0f;
    bool  hardSync = false;
    float pitchEnvAmt = 0.0f, pitchDecay = 0.08f;

    int   filterMode = 0;
    float cutoff = 2000.0f, resonance = 0.3f, drive = 0.2f;
    float filterEnvAmt = 0.5f, keyTrack = 0.0f;

    float combMix = 0.0f, combTune = 0.0f, combFb = 0.85f, combDamp = 0.3f;

    float fA = 0.001f, fD = 0.25f, fS = 0.0f, fR = 0.2f;
    float aA = 0.001f, aD = 0.3f,  aS = 0.8f, aR = 0.15f;

    int   lfoWave = 0;
    float lfoRateHz = 2.0f, lfoDepth = 0.0f, lfoPitch = 0.0f;
    int   lfo2Wave = 0;
    float lfo2RateHz = 1.0f;

    std::array<ModSlot, kModSlots> mod {};

    float glideTime = 0.0f;
    bool  mono = true;
    float velSens = 0.5f;
    float pitchBendRange = 2.0f;
};

// ---------------------------------------------------------------------------
//  Voz
// ---------------------------------------------------------------------------
struct Voice
{
    int   note = -1;
    float velocity = 1.0f;
    float currentNote = 60.0f, targetNote = 60.0f;
    bool  gate = false;
    uint64_t age = 0;
    float rnd = 0.0f;

    float phA[kMaxUnison] {}, phB[kMaxUnison] {};
    float phMod = 0.0f, phSub = 0.0f;
    float pitchEnv = 0.0f;
    Envelope ampEnv, fltEnv;
    LadderFilter filtL, filtR;

    std::vector<float> combL, combR;
    int   combWrite = 0;
    float combLpL = 0.0f, combLpR = 0.0f;

    bool isActive() const { return ampEnv.isActive(); }

    void clearComb()
    {
        if (! combL.empty()) std::memset (combL.data(), 0, combL.size() * sizeof (float));
        if (! combR.empty()) std::memset (combR.data(), 0, combR.size() * sizeof (float));
        combLpL = combLpR = 0.0f;
    }

    void startFresh (Rng& rng, int uniA, int uniB)
    {
        filtL.reset(); filtR.reset();
        clearComb();
        // con unison: fases aleatorias (supersaw); sin unison: fase 0 (ataque consistente)
        for (int k = 0; k < kMaxUnison; ++k)
        {
            phA[k] = uniA > 1 ? rng.next01() : 0.0f;
            phB[k] = uniB > 1 ? rng.next01() : 0.0f;
        }
        phMod = phSub = 0.0f;
    }

    void reset()
    {
        ampEnv.reset(); fltEnv.reset(); filtL.reset(); filtR.reset(); clearComb();
        note = -1; gate = false; pitchEnv = 0.0f;
    }
};

// ---------------------------------------------------------------------------
//  Engine
// ---------------------------------------------------------------------------
class Engine
{
public:
    static constexpr int kMaxVoices = 8;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        (void) wt::Bank::get(); // genera las tablas fuera del audio thread
        int size = 1;
        while (size < (int) (sr / 15.0) + 8) size <<= 1;
        combMask = size - 1;
        for (auto& v : voices)
        {
            v.combL.assign ((size_t) size, 0.0f);
            v.combR.assign ((size_t) size, 0.0f);
            v.combWrite = 0;
            v.reset();
        }
        heldNotes.clear();
        heldNotes.reserve (128);
        lfo1.reset(); lfo2.reset();
        smoothCutoff = 0.0f;
    }

    void setParams (const EngineParams& p)
    {
        params = p;
        params.uniA = std::clamp (p.uniA, 1, kMaxUnison);
        params.uniB = std::clamp (p.uniB, 1, kMaxUnison);
        for (auto& v : voices)
        {
            v.ampEnv.set (sr, p.aA, p.aD, p.aS, p.aR);
            v.fltEnv.set (sr, p.fA, p.fD, p.fS, p.fR);
        }
        glideCoef = p.glideTime <= 0.0005f ? 0.0f : std::exp (-1.0f / (float) (p.glideTime * 0.25f * sr));
        pitchDecayCoef = std::exp (-4.6f / (float) (std::max (0.001f, p.pitchDecay) * sr));
        if (smoothCutoff <= 0.0f) smoothCutoff = p.cutoff;

        // ¿qué destinos usa la matriz?
        modUsed.fill (false);
        anyMod = false;
        for (const auto& s : params.mod)
            if (s.src > 0 && s.dst > 0 && std::abs (s.amt) > 1.0e-4f)
            {
                modUsed[(size_t) s.dst] = true;
                anyMod = true;
            }

        // paneo de cada voz de unison
        auto computePans = [&] (int n, float* gl, float* gr, float* pos)
        {
            const float norm = 1.0f / std::sqrt ((float) n);
            for (int k = 0; k < n; ++k)
            {
                pos[k] = n == 1 ? 0.0f : -1.0f + 2.0f * (float) k / (float) (n - 1);
                panGains (pos[k] * params.spread, gl[k], gr[k]);
                gl[k] *= norm; gr[k] *= norm;
            }
        };
        computePans (params.uniA, uniGLA, uniGRA, uniPosA);
        computePans (params.uniB, uniGLB, uniGRB, uniPosB);
    }

    void setPitchBend (float normalised) { pitchBend = normalised; }
    void setModWheel (float v)   { modWheel = v; }
    void setAftertouch (float v) { aftertouch = v; }

    void noteOn (int note, float vel)
    {
        ++ageCounter;
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        heldNotes.push_back (note);

        if (params.mono)
        {
            auto& v = voices[0];
            const bool legato = v.gate && v.isActive();
            if (! v.isActive()) v.startFresh (rng, params.uniA, params.uniB);
            v.note = note;
            v.targetNote = (float) note;
            if (glideCoef <= 0.0f)   v.currentNote = (float) note;
            else if (! v.isActive()) v.currentNote = lastNote < 0 ? (float) note : (float) lastNote;
            if (! legato)
            {
                v.velocity = vel;
                v.pitchEnv = 1.0f;
                v.rnd = rng.next();
                v.ampEnv.gateOn();
                v.fltEnv.gateOn();
            }
            v.gate = true;
            v.age = ageCounter;
        }
        else
        {
            Voice* target = nullptr;
            for (auto& v : voices) if (v.note == note && v.isActive()) { target = &v; break; }
            if (! target) for (auto& v : voices) if (! v.isActive()) { target = &v; break; }
            if (! target)
            {
                for (auto& v : voices)
                    if (! v.gate && (! target || v.age < target->age)) target = &v;
                if (! target)
                    for (auto& v : voices)
                        if (! target || v.age < target->age) target = &v;
            }
            auto& v = *target;
            if (! v.isActive()) v.startFresh (rng, params.uniA, params.uniB);
            v.note = note;
            v.velocity = vel;
            v.targetNote = (float) note;
            v.currentNote = (glideCoef > 0.0f && lastNote >= 0) ? (float) lastNote : (float) note;
            v.gate = true;
            v.age = ageCounter;
            v.pitchEnv = 1.0f;
            v.rnd = rng.next();
            v.ampEnv.gateOn();
            v.fltEnv.gateOn();
        }
        lastNote = note;
    }

    void noteOff (int note)
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

        if (params.mono)
        {
            auto& v = voices[0];
            if (! heldNotes.empty())
            {
                const int back = heldNotes.back();
                if (v.note != back)
                {
                    v.note = back;
                    v.targetNote = (float) back;
                    if (glideCoef <= 0.0f) v.currentNote = (float) back;
                    lastNote = back;
                }
            }
            else
            {
                v.gate = false;
                v.ampEnv.gateOff();
                v.fltEnv.gateOff();
            }
        }
        else
        {
            for (auto& v : voices)
                if (v.note == note && v.gate)
                {
                    v.gate = false;
                    v.ampEnv.gateOff();
                    v.fltEnv.gateOff();
                }
        }
    }

    void allNotesOff()
    {
        heldNotes.clear();
        for (auto& v : voices) { v.gate = false; v.ampEnv.gateOff(); v.fltEnv.gateOff(); }
    }

    int getActiveVoiceCount() const
    {
        int n = 0;
        for (const auto& v : voices) if (v.isActive()) ++n;
        return n;
    }

    // Renderiza estéreo (suma de voces) en outL/outR [0..n)
    void render (float* outL, float* outR, int n)
    {
        const auto& p = params;
        const float srf = (float) sr;
        const float invSr = 1.0f / srf;
        const float nyqLimit = srf * 0.45f;
        const float lfoInc  = p.lfoRateHz * invSr;
        const float lfo2Inc = p.lfo2RateHz * invSr;
        const float bend = pitchBend * p.pitchBendRange;
        const float o2offset = p.osc2Semi + p.osc2Detune * 0.01f;
        const float voiceGain = p.mono ? 0.55f : 0.32f;
        const int   numVoices = p.mono ? 1 : kMaxVoices;
        const bool  useComb = p.combMix > 0.001f || modUsed[DstCombMix];
        const float combDampA = 1.0f - p.combDamp * 0.92f;
        const float combMaxDelay = (float) (combMask - 2);
        const int   nA = p.uniA, nB = p.uniB;
        const auto& bank = wt::Bank::get();
        const bool  tblA = p.osc1Wave == Wave::Table, tblB = p.osc2Wave == Wave::Table;
        const int   tA = std::clamp (p.wtTableA, 0, wt::kNumTables - 1);
        const int   tB = std::clamp (p.wtTableB, 0, wt::kNumTables - 1);

        for (int i = 0; i < n; ++i)
        {
            const float lfoV  = lfo1.tick (p.lfoWave, lfoInc, rng);
            const float lfo2V = lfo2.tick (p.lfo2Wave, lfo2Inc, rng);
            smoothCutoff += (p.cutoff - smoothCutoff) * 0.0015f;

            float sumL = 0.0f, sumR = 0.0f;
            for (int vi = 0; vi < numVoices; ++vi)
            {
                auto& v = voices[(size_t) vi];
                if (! v.isActive()) continue;

                if (glideCoef > 0.0f) v.currentNote = v.targetNote + (v.currentNote - v.targetNote) * glideCoef;
                else                  v.currentNote = v.targetNote;
                v.pitchEnv *= pitchDecayCoef;

                const float fenv = v.fltEnv.next();
                const float aenv = v.ampEnv.next();

                // ---- matriz
                float m[NumModDst] {};
                if (anyMod)
                {
                    float src[NumModSrc];
                    src[SrcOff] = 0.0f;
                    src[SrcLfo1] = lfoV;
                    src[SrcLfo2] = lfo2V;
                    src[SrcModEnv] = fenv;
                    src[SrcAmpEnv] = aenv;
                    src[SrcVelocity] = v.velocity;
                    src[SrcNote] = (v.currentNote - 60.0f) / 24.0f;
                    src[SrcRandom] = v.rnd;
                    src[SrcModWheel] = modWheel;
                    src[SrcAftertouch] = aftertouch;
                    for (const auto& s : p.mod)
                        if (s.src > 0 && s.dst > 0)
                            m[s.dst] += s.amt * src[s.src];
                }

                // ---- pitch
                const float pitch  = v.currentNote + bend + lfoV * p.lfoPitch * 12.0f
                                   + p.pitchEnvAmt * v.pitchEnv + m[DstPitch] * 24.0f;
                const float pitchB = pitch + o2offset + m[DstOscBPitch] * 24.0f;
                const float fA = std::min (midiToHz (pitch), nyqLimit);
                const float fB = std::min (midiToHz (pitchB), nyqLimit);
                const float dtA = fA * invSr, dtB = fB * invSr, dtS = 0.5f * dtA;

                const float pw = std::clamp (p.pulseWidth + m[DstPW] * 0.45f, 0.03f, 0.97f);
                const float detA = std::clamp (p.detA + m[DstDetune], 0.0f, 1.0f);
                const float detB = std::clamp (p.detB + m[DstDetune], 0.0f, 1.0f);
                const float centsA = 90.0f * detA * detA, centsB = 90.0f * detB * detB;

                // ---- FM (senoidal a la frecuencia de B modula la fase de A)
                float fmIdx = std::clamp (p.fmAmount + p.fmEnv * fenv + m[DstFM], 0.0f, 1.0f);
                fmIdx = fmIdx * fmIdx * 3.0f;
                const float fmSig = fmIdx > 1.0e-4f ? fmIdx * std::sin (kTwoPi * v.phMod) : 0.0f;

                const float posA = std::clamp (p.wtPosA + m[DstWtPosA], 0.0f, 1.0f);
                const float posB = std::clamp (p.wtPosB + m[DstWtPosB], 0.0f, 1.0f);
                const int   mipA = tblA ? wt::Bank::mipFor (dtA * (1.0f + centsA * 0.000578f)) : 0;
                const int   mipB = tblB ? wt::Bank::mipFor (dtB * (1.0f + centsB * 0.000578f)) : 0;

                // ---- osc A con unison
                float aL = 0.0f, aR = 0.0f;
                bool  wrapA[kMaxUnison] {};
                float fracA[kMaxUnison] {};
                for (int k = 0; k < nA; ++k)
                {
                    const float ratio = 1.0f + uniPosA[k] * centsA * 0.000577623f;
                    const float dt = std::min (dtA * ratio, 0.45f);
                    float ph = v.phA[k] + fmSig;
                    ph -= std::floor (ph);
                    const float s = tblA ? bank.read (tA, posA, ph, std::min (wt::kMips - 1, mipA + (fmIdx > 0.3f ? 1 : 0)))
                                         : renderWave (p.osc1Wave, ph, dt, pw);
                    aL += s * uniGLA[k];
                    aR += s * uniGRA[k];
                    v.phA[k] += dt;
                    if (v.phA[k] >= 1.0f) { v.phA[k] -= 1.0f; wrapA[k] = true; fracA[k] = v.phA[k] / dt; }
                }

                // ---- osc B con unison (+ hard sync a A)
                float bL = 0.0f, bR = 0.0f;
                for (int k = 0; k < nB; ++k)
                {
                    const float ratio = 1.0f + uniPosB[k] * centsB * 0.000577623f;
                    const float dt = std::min (dtB * ratio, 0.45f);
                    const float s = tblB ? bank.read (tB, posB, v.phB[k], mipB)
                                         : renderWave (p.osc2Wave, v.phB[k], dt, pw);
                    bL += s * uniGLB[k];
                    bR += s * uniGRB[k];
                    v.phB[k] += dt;
                    if (v.phB[k] >= 1.0f) v.phB[k] -= 1.0f;
                    const int master = k % nA;
                    if (p.hardSync && wrapA[master])
                    {
                        v.phB[k] = fracA[master] * dt;
                        v.phB[k] -= std::floor (v.phB[k]);
                    }
                }

                v.phMod += dtB; if (v.phMod >= 1.0f) v.phMod -= 1.0f;
                if (p.hardSync && wrapA[0]) v.phMod = fracA[0] * dtB;

                const float sub = renderWave (Wave::Square, v.phSub, dtS, 0.5f);
                v.phSub += dtS; if (v.phSub >= 1.0f) v.phSub -= 1.0f;

                const float mix  = std::clamp (p.oscMix + m[DstOscMix], 0.0f, 1.0f);
                const float ring = std::clamp (p.ringMod + m[DstRing], 0.0f, 1.0f);
                const float subL = std::clamp (p.subLevel + m[DstSub], 0.0f, 1.0f);
                const float noiL = std::clamp (p.noiseLevel + m[DstNoise], 0.0f, 1.0f);

                float cL = aL * (1.0f - mix) + bL * mix;
                float cR = aR * (1.0f - mix) + bR * mix;
                cL += (aL * bL * 1.2f - cL) * ring;
                cR += (aR * bR * 1.2f - cR) * ring;

                float oL = (cL + sub * subL + rng.next() * noiL) * 0.6f;
                float oR = (cR + sub * subL + rng.next() * noiL) * 0.6f;

                // ---- filtro estéreo
                const float drive = std::clamp (p.drive + m[DstDrive], 0.0f, 1.0f);
                const float driveGain = 1.0f + drive * drive * 14.0f;
                const float driveComp = 1.0f / (1.0f + drive * 1.6f);
                const float k = std::clamp (p.resonance + m[DstReso], 0.0f, 1.0f) * 4.2f;

                const float velMod = 1.0f - p.velSens + p.velSens * v.velocity;
                const float octaves = p.filterEnvAmt * 7.0f * fenv * velMod
                                    + lfoV * p.lfoDepth * 4.0f
                                    + p.keyTrack * (v.currentNote - 60.0f) / 12.0f
                                    + m[DstCutoff] * 6.0f;
                const float fc = std::clamp (smoothCutoff * std::exp2 (octaves), 15.0f, nyqLimit);
                const float g = std::tan (kPi * fc * invSr);

                float yL = v.filtL.process (oL * driveGain, g, k, p.filterMode) * driveComp;
                float yR = v.filtR.process (oR * driveGain, g, k, p.filterMode) * driveComp;

                // ---- resonador
                if (useComb)
                {
                    const float cmix = std::clamp (p.combMix + m[DstCombMix], 0.0f, 1.0f);
                    const float cf = std::clamp (midiToHz (pitch + p.combTune + m[DstCombTune] * 24.0f), 16.0f, nyqLimit);
                    const float d = std::clamp (srf / cf, 2.0f, combMaxDelay);
                    float rp = (float) v.combWrite - d;
                    if (rp < 0.0f) rp += (float) (combMask + 1);
                    const int   i0 = (int) rp;
                    const float fr = rp - (float) i0;
                    const size_t a0 = (size_t) (i0 & combMask), a1 = (size_t) ((i0 + 1) & combMask);

                    const float rdL = v.combL[a0] + (v.combL[a1] - v.combL[a0]) * fr;
                    const float rdR = v.combR[a0] + (v.combR[a1] - v.combR[a0]) * fr;
                    v.combLpL += (rdL - v.combLpL) * combDampA;
                    v.combLpR += (rdR - v.combLpR) * combDampA;
                    const float wL = fastTanh (yL + p.combFb * v.combLpL);
                    const float wR = fastTanh (yR + p.combFb * v.combLpR);
                    v.combL[(size_t) v.combWrite] = wL;
                    v.combR[(size_t) v.combWrite] = wR;
                    v.combWrite = (v.combWrite + 1) & combMask;
                    yL += (wL * 0.9f - yL) * cmix;
                    yR += (wR * 0.9f - yR) * cmix;
                }

                // ---- amp + pan
                const float velAmp = 1.0f - p.velSens * 0.7f + p.velSens * 0.7f * v.velocity;
                const float amp = aenv * velAmp * std::max (0.0f, 1.0f + m[DstAmp]);
                if (modUsed[DstPan])
                {
                    float pl, pr;
                    panGains (m[DstPan], pl, pr);
                    sumL += yL * amp * pl;
                    sumR += yR * amp * pr;
                }
                else
                {
                    sumL += yL * amp;
                    sumR += yR * amp;
                }

                if (! v.ampEnv.isActive()) { v.filtL.reset(); v.filtR.reset(); v.note = -1; v.gate = false; }
            }

            outL[i] = sumL * voiceGain;
            outR[i] = sumR * voiceGain;
        }
    }

private:
    double sr = 44100.0;
    EngineParams params;
    std::array<Voice, kMaxVoices> voices;
    std::vector<int> heldNotes;
    uint64_t ageCounter = 0;
    int lastNote = -1;
    int combMask = 4095;
    float glideCoef = 0.0f;
    float pitchDecayCoef = 0.999f;
    float pitchBend = 0.0f, modWheel = 0.0f, aftertouch = 0.0f;

    std::array<bool, NumModDst> modUsed {};
    bool anyMod = false;
    float uniGLA[kMaxUnison] {}, uniGRA[kMaxUnison] {}, uniPosA[kMaxUnison] {};
    float uniGLB[kMaxUnison] {}, uniGRB[kMaxUnison] {}, uniPosB[kMaxUnison] {};

    Lfo lfo1, lfo2;
    float smoothCutoff = 0.0f;
    Rng rng;
};

// ---------------------------------------------------------------------------
//  Distorsión (corre oversampleada, un estado por canal)
// ---------------------------------------------------------------------------
enum class DistType { Soft = 0, Hard, Fold, Crush };

struct Distortion
{
    float holdSample = 0.0f;
    float holdCounter = 0.0f;
    void reset() { holdSample = 0.0f; holdCounter = 0.0f; }

    float process (float x, DistType type, float amount, float rateFactor)
    {
        const float g = 1.0f + amount * amount * 30.0f;
        switch (type)
        {
            case DistType::Soft: return fastTanh (x * g);
            case DistType::Hard: return std::clamp (x * g, -0.9f, 0.9f);
            case DistType::Fold: return std::sin (x * (1.0f + amount * 9.0f) * kPi * 0.5f) * 0.9f;
            case DistType::Crush:
            default:
            {
                const float holdLen = (1.0f + amount * amount * 40.0f) * rateFactor;
                holdCounter += 1.0f;
                if (holdCounter >= holdLen) { holdCounter -= holdLen; holdSample = x; }
                const float bits = 16.0f - amount * 12.0f;
                const float steps = std::exp2 (bits - 1.0f);
                return std::round (std::clamp (holdSample * (1.0f + amount * 2.0f), -1.0f, 1.0f) * steps) / steps;
            }
        }
    }
};

} // namespace rawtek

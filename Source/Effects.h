#pragma once

#include "SynthEngine.h"

// ============================================================================
//  PANIC RAW TEK — efectos estéreo: chorus, delay dub, reverb FDN
// ============================================================================
namespace rawtek
{
inline int nextPow2 (int n) { int s = 1; while (s < n) s <<= 1; return s; }

// Lectura fraccional de buffer circular (máscara potencia de 2)
inline float readFrac (const std::vector<float>& buf, int mask, int writePos, float delay)
{
    float rp = (float) writePos - delay;
    while (rp < 0.0f) rp += (float) (mask + 1);
    const int   i0 = (int) rp;
    const float fr = rp - (float) i0;
    const float a = buf[(size_t) (i0 & mask)];
    const float b = buf[(size_t) ((i0 + 1) & mask)];
    return a + (b - a) * fr;
}

// ---------------------------------------------------------------------------
struct Chorus
{
    std::vector<float> bufL, bufR;
    int mask = 0, w = 0;
    float phase = 0.0f;
    double sr = 44100.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const int size = nextPow2 ((int) (sr * 0.06) + 4);
        bufL.assign ((size_t) size, 0.0f); bufR.assign ((size_t) size, 0.0f);
        mask = size - 1; w = 0; phase = 0.0f;
    }

    void process (float* L, float* R, int n, float rateHz, float depth, float mix)
    {
        const float inc = rateHz / (float) sr;
        const float base = (float) sr * 0.011f, range = (float) sr * 0.0065f * depth;
        for (int i = 0; i < n; ++i)
        {
            bufL[(size_t) w] = L[i];
            bufR[(size_t) w] = R[i];
            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
            const float mL = base + range * (0.5f + 0.5f * std::sin (kTwoPi * phase));
            const float mR = base + range * (0.5f + 0.5f * std::cos (kTwoPi * phase));
            const float wl = readFrac (bufL, mask, w, mL);
            const float wr = readFrac (bufR, mask, w, mR);
            w = (w + 1) & mask;
            L[i] = L[i] * (1.0f - 0.5f * mix) + wr * mix * 0.8f;   // cruzado: más ancho
            R[i] = R[i] * (1.0f - 0.5f * mix) + wl * mix * 0.8f;
        }
    }
};

// ---------------------------------------------------------------------------
//  Delay dub: feedback saturado, filtrado (HP + LP), ping-pong opcional
// ---------------------------------------------------------------------------
struct DubDelay
{
    std::vector<float> bufL, bufR;
    int mask = 0, w = 0;
    float cur = -1.0f;
    float lpL = 0, lpR = 0, hpL = 0, hpR = 0;
    double sr = 44100.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const int size = nextPow2 ((int) (sr * 2.6) + 4);
        bufL.assign ((size_t) size, 0.0f); bufR.assign ((size_t) size, 0.0f);
        mask = size - 1; w = 0; cur = -1.0f;
        lpL = lpR = hpL = hpR = 0.0f;
    }

    void process (float* L, float* R, int n, float delaySec, float fb, float tone, float mix, bool ping)
    {
        const float target = std::clamp (delaySec * (float) sr, 16.0f, (float) (mask - 4));
        if (cur < 0.0f) cur = target;
        const float lpA = std::clamp (0.05f + tone * 0.9f, 0.02f, 0.98f);   // tone: 0 oscuro, 1 brillante
        const float hpA = 0.012f;                                           // ~90 Hz a 48 k
        for (int i = 0; i < n; ++i)
        {
            cur += (target - cur) * 0.0004f;
            float rl = readFrac (bufL, mask, w, cur);
            float rr = readFrac (bufR, mask, w, cur);
            lpL += (rl - lpL) * lpA;  lpR += (rr - lpR) * lpA;
            hpL += (lpL - hpL) * hpA; hpR += (lpR - hpR) * hpA;
            const float wl = lpL - hpL, wr = lpR - hpR;

            float inL, inR;
            if (ping)
            {
                inL = fastTanh ((L[i] + R[i]) * 0.5f + fb * wr);
                inR = fastTanh (fb * wl);
            }
            else
            {
                inL = fastTanh (L[i] + fb * wl);
                inR = fastTanh (R[i] + fb * wr);
            }
            bufL[(size_t) w] = inL;
            bufR[(size_t) w] = inR;
            w = (w + 1) & mask;

            L[i] = L[i] * (1.0f - 0.3f * mix) + wl * mix;
            R[i] = R[i] * (1.0f - 0.3f * mix) + wr * mix;
        }
    }
};

// ---------------------------------------------------------------------------
//  Reverb FDN 8 líneas (Hadamard), predelay, low cut, damping, modulación
// ---------------------------------------------------------------------------
struct FdnReverb
{
    static constexpr int N = 8;
    std::vector<float> line[N];
    int mask[N] {}, w[N] {};
    float lp[N] {};
    std::vector<float> pre; int preMask = 0, preW = 0;
    float hpState = 0.0f, modPhase = 0.0f;
    double sr = 44100.0;

    static constexpr float baseMs[N] = { 31.7f, 37.9f, 41.3f, 47.1f, 53.9f, 59.3f, 67.7f, 73.1f };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int i = 0; i < N; ++i)
        {
            const int size = nextPow2 ((int) (sr * baseMs[i] * 0.001 * 2.2) + 64);
            line[i].assign ((size_t) size, 0.0f);
            mask[i] = size - 1; w[i] = 0; lp[i] = 0.0f;
        }
        const int ps = nextPow2 ((int) (sr * 0.26) + 4);
        pre.assign ((size_t) ps, 0.0f); preMask = ps - 1; preW = 0;
        hpState = 0.0f; modPhase = 0.0f;
    }

    // size 0..1, damp 0..1, preDelay s, lowCut Hz, mix 0..1
    void process (float* L, float* R, int n, float size, float damp, float preDelay, float lowCut, float mix)
    {
        const float scale = 0.55f + size * 1.05f;                 // tamaño de sala
        const float t60   = 0.35f + size * size * 11.0f;          // 0.35 s .. 11 s
        float len[N], gain[N];
        for (int i = 0; i < N; ++i)
        {
            len[i]  = std::min ((float) sr * baseMs[i] * 0.001f * scale, (float) (mask[i] - 8));
            gain[i] = std::pow (10.0f, -3.0f * len[i] / ((float) sr * t60));
        }
        const float dampA = 1.0f - damp * 0.88f;
        const float hpA = std::clamp (kTwoPi * lowCut / (float) sr, 0.0005f, 0.9f);
        const float preD = std::clamp (preDelay * (float) sr, 1.0f, (float) (preMask - 2));
        const float modInc = 0.37f / (float) sr;
        const float modDepth = 6.0f * (float) (sr / 48000.0);

        for (int s = 0; s < n; ++s)
        {
            // predelay + low cut
            pre[(size_t) preW] = (L[s] + R[s]) * 0.5f;
            float in = readFrac (pre, preMask, preW, preD);
            preW = (preW + 1) & preMask;
            hpState += (in - hpState) * hpA;
            in -= hpState;

            modPhase += modInc; if (modPhase >= 1.0f) modPhase -= 1.0f;
            const float mod = std::sin (kTwoPi * modPhase) * modDepth;

            float o[N];
            for (int i = 0; i < N; ++i)
            {
                const float d = len[i] + (i == 1 ? mod : (i == 6 ? -mod : 0.0f));
                const float r = readFrac (line[i], mask[i], w[i], d);
                lp[i] += (r - lp[i]) * dampA;
                o[i] = lp[i];
            }

            // Hadamard 8x8 (butterflies)
            float h[N];
            for (int i = 0; i < N; ++i) h[i] = o[i];
            for (int step = 1; step < N; step <<= 1)
                for (int i = 0; i < N; i += step << 1)
                    for (int j = i; j < i + step; ++j)
                    {
                        const float a = h[j], b = h[j + step];
                        h[j] = a + b; h[j + step] = a - b;
                    }
            const float hn = 0.35355339f; // 1/sqrt(8)

            for (int i = 0; i < N; ++i)
            {
                const float sign = (i & 1) ? -1.0f : 1.0f;
                line[i][(size_t) w[i]] = h[i] * hn * gain[i] + in * sign * 0.5f;
                w[i] = (w[i] + 1) & mask[i];
            }

            const float wetL = (o[0] - o[2] + o[4] - o[6]) * 0.45f;
            const float wetR = (o[1] - o[3] + o[5] - o[7]) * 0.45f;
            L[s] = L[s] * (1.0f - 0.3f * mix) + wetL * mix;
            R[s] = R[s] * (1.0f - 0.3f * mix) + wetR * mix;
        }
    }
};

} // namespace rawtek

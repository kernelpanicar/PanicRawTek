#pragma once

#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <cstdint>

// ============================================================================
//  PANIC RAW TEK — wavetables
//  Tablas generadas al cargar (sin archivos). Cada tabla tiene kFrames cuadros
//  que se morfean con WT POS; cada cuadro tiene kMips versiones band-limited
//  (una por octava) para que no haya aliasing en notas agudas.
// ============================================================================

namespace rawtek
{
namespace wt
{
constexpr int kLen    = 1024;          // muestras por ciclo
constexpr int kHalf   = kLen / 2;      // armónicos máx en el mip 0
constexpr int kFrames = 16;
constexpr int kMips   = 10;            // 512, 256 ... 1 armónicos
constexpr int kNumTables = 10;

inline const char* const* tableNames()
{
    static const char* names[kNumTables] = { "Saw>Sqr", "Sync", "Fold", "PWM", "Formant",
                                             "Metal", "Digital", "Grit", "Harmonic", "Vowel" };
    return names;
}

// ---------------------------------------------------------------------------
inline void fft (std::vector<std::complex<double>>& a, bool inverse)
{
    const int n = (int) a.size();
    for (int i = 1, j = 0; i < n; ++i)
    {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[(size_t) i], a[(size_t) j]);
    }
    for (int len = 2; len <= n; len <<= 1)
    {
        const double ang = 2.0 * 3.14159265358979323846 / len * (inverse ? 1.0 : -1.0);
        const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (int i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0);
            for (int j = 0; j < len / 2; ++j)
            {
                auto u = a[(size_t) (i + j)], v = a[(size_t) (i + j + len / 2)] * w;
                a[(size_t) (i + j)] = u + v;
                a[(size_t) (i + j + len / 2)] = u - v;
                w *= wl;
            }
        }
    }
    if (inverse) for (auto& x : a) x /= (double) n;
}

// ---------------------------------------------------------------------------
//  Definición de cada cuadro en el dominio del tiempo (ph en [0,1), t en [0,1])
// ---------------------------------------------------------------------------
inline double frameSample (int table, double ph, double t)
{
    constexpr double pi = 3.14159265358979323846, tp = 2.0 * pi;
    switch (table)
    {
        case 0: // saw -> square
        {
            const double saw = 2.0 * ph - 1.0;
            const double sq  = ph < 0.5 ? 1.0 : -1.0;
            return saw * (1.0 - t) + sq * t;
        }
        case 1: // hard sync: seno esclavo de 1x a 9x
        {
            const double ratio = 1.0 + t * 8.0;
            return std::sin (tp * std::fmod (ph * ratio, 1.0)) * (1.0 - ph * 0.35);
        }
        case 2: // wavefolder sobre seno
        {
            const double g = 1.0 + t * 7.0;
            return std::sin (pi * 0.5 * g * std::sin (tp * ph));
        }
        case 3: // PWM 50% -> 3%
        {
            const double pw = 0.5 - t * 0.47;
            return ph < pw ? 1.0 : -1.0;
        }
        case 4: // formante: ráfaga senoidal ventaneada, la formante sube
        {
            const double f = 1.0 + t * 15.0;
            const double win = 0.5 - 0.5 * std::cos (tp * ph);
            return std::sin (tp * ph * f) * win * 1.6;
        }
        case 5: // metal: parciales dispersos tipo campana (enteros, ricos)
        {
            static const int   h[]   = { 1, 3, 7, 11, 13, 17, 23, 29, 31, 37 };
            static const float amp[] = { 1.0f, .7f, .6f, .55f, .5f, .45f, .4f, .35f, .3f, .3f };
            double s = 0.0;
            const double reach = 1.0 + t * 9.0;          // cuántos parciales entran
            for (int i = 0; i < 10; ++i)
            {
                const double w = std::clamp (reach - i, 0.0, 1.0);
                if (w <= 0.0) break;
                s += std::sin (tp * ph * h[i] + i * 1.7) * amp[i] * w;
            }
            return s * 0.55;
        }
        case 6: // digital: saw escalonada (bit reduction) 32 -> 3 pasos
        {
            const double steps = std::round (32.0 * std::pow (3.0 / 32.0, t));
            return std::floor (ph * steps) / (steps - 1.0) * 2.0 - 1.0;
        }
        case 7: // grit: seno + armónicos saturados con drive creciente
        {
            const double g = 1.0 + t * t * 18.0;
            const double x = std::sin (tp * ph) + 0.3 * std::sin (2.0 * tp * ph);
            return std::tanh (x * g) / std::tanh (1.2 * g) ;
        }
        case 8: // harmonic: suma aditiva 1 -> 48 armónicos (de seno a saw brillante)
        {
            const double n = 1.0 + t * 47.0;
            double s = 0.0;
            for (int k = 1; k <= 48; ++k)
            {
                const double w = std::clamp (n - (k - 1), 0.0, 1.0);
                if (w <= 0.0) break;
                s += std::sin (tp * ph * k) / std::sqrt ((double) k) * w;
            }
            return s * 0.5;
        }
        case 9: // vowel: dos formantes que pasan A-E-I-O-U
        default:
        {
            static const double f1[] = { 8, 5, 3, 5, 3 }, f2[] = { 13, 19, 24, 9, 7 };
            const double pos = t * 4.0;
            const int i0 = std::min (3, (int) pos);
            const double fr = pos - i0;
            const double a = f1[i0] + (f1[i0 + 1] - f1[i0]) * fr;
            const double b = f2[i0] + (f2[i0 + 1] - f2[i0]) * fr;
            double s = 0.0;
            for (int k = 1; k <= 40; ++k)
            {
                const double g = std::exp (-0.5 * std::pow ((k - a) / 1.8, 2.0))
                               + 0.6 * std::exp (-0.5 * std::pow ((k - b) / 2.6, 2.0));
                s += std::sin (tp * ph * k) * g;
            }
            return s * 0.7;
        }
    }
}

// ---------------------------------------------------------------------------
struct Bank
{
    // data[((table * kFrames + frame) * kMips + mip) * (kLen + 1) + i]  (+1 = guard)
    std::vector<float> data;
    static constexpr int kStride = kLen + 1;

    Bank()
    {
        data.assign ((size_t) kNumTables * kFrames * kMips * kStride, 0.0f);
        std::vector<std::complex<double>> spec ((size_t) kLen), tmp ((size_t) kLen);

        for (int tb = 0; tb < kNumTables; ++tb)
            for (int fr = 0; fr < kFrames; ++fr)
            {
                const double t = (double) fr / (double) (kFrames - 1);
                // sobremuestreo 4x por punto para que las discontinuidades no se aliasen al muestrear
                for (int i = 0; i < kLen; ++i)
                {
                    double s = 0.0;
                    for (int o = 0; o < 4; ++o)
                    {
                        double ph = ((double) i + (o + 0.5) / 4.0 - 0.5) / (double) kLen;
                        if (ph < 0.0) ph += 1.0;
                        s += frameSample (tb, ph, t);
                    }
                    spec[(size_t) i] = s * 0.25;
                }
                fft (spec, false);
                spec[0] = std::complex<double> (0.0); // sin DC

                // normalización con la versión completa
                float peak = 0.0f;
                for (int mip = 0; mip < kMips; ++mip)
                {
                    const int maxH = std::max (1, std::min (kHalf - 1, kHalf >> mip));
                    for (int k = 0; k < kLen; ++k)
                    {
                        const int h = k <= kHalf ? k : kLen - k;
                        tmp[(size_t) k] = (h >= 1 && h <= maxH) ? spec[(size_t) k] : std::complex<double> (0.0);
                    }
                    fft (tmp, true);
                    float* dst = &data[(size_t) (((tb * kFrames + fr) * kMips + mip) * kStride)];
                    for (int i = 0; i < kLen; ++i) dst[i] = (float) tmp[(size_t) i].real();
                    dst[kLen] = dst[0];
                    if (mip == 0)
                        for (int i = 0; i < kLen; ++i) peak = std::max (peak, std::abs (dst[i]));
                }
                const float norm = peak > 1.0e-6f ? 0.95f / peak : 1.0f;
                for (int mip = 0; mip < kMips; ++mip)
                {
                    float* dst = &data[(size_t) (((tb * kFrames + fr) * kMips + mip) * kStride)];
                    for (int i = 0; i <= kLen; ++i) dst[i] *= norm;
                }
            }
    }

    static const Bank& get()
    {
        static const Bank bank;
        return bank;
    }

    // mip según el incremento de fase (dt = f / sr)
    static int mipFor (float dt)
    {
        // necesitamos maxH <= 0.5/dt  ->  kHalf >> mip <= 0.5/dt
        const float h = 0.5f / std::max (dt, 1.0e-7f);
        int mip = 0;
        while (mip < kMips - 1 && (float) std::min (kHalf - 1, kHalf >> mip) > h) ++mip;
        return mip;
    }

    float read (int table, float pos, float ph, int mip) const
    {
        const float fpos = std::clamp (pos, 0.0f, 1.0f) * (float) (kFrames - 1);
        const int   f0 = std::min ((int) fpos, kFrames - 2);
        const float ff = fpos - (float) f0;
        const float x  = ph * (float) kLen;
        const int   i0 = std::min ((int) x, kLen - 1);
        const float fx = x - (float) i0;
        const float* a = &data[(size_t) (((table * kFrames + f0) * kMips + mip) * kStride)];
        const float* b = a + (size_t) kMips * kStride;
        const float va = a[i0] + (a[i0 + 1] - a[i0]) * fx;
        const float vb = b[i0] + (b[i0 + 1] - b[i0]) * fx;
        return va + (vb - va) * ff;
    }
};

} // namespace wt
} // namespace rawtek

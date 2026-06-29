/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cmath>
#include <algorithm>

// Procedural, band-limited single-cycle wavetable bank. JUCE-free so the engine
// audit compiles it. Each of kNumWt waveforms is stored at kWtMips octave mip
// levels (additive synthesis with a per-mip harmonic cap), so the oscillator can
// pick a mip whose top harmonic is below Nyquist and never alias. Real AKWF .wav
// data can replace the procedural generator later without touching the osc.
namespace jove
{
constexpr int kWtLen  = 2048;  // samples per single cycle (power of two)
constexpr int kWtMips = 8;     // octave bands: harmonic cap 256,128,..,2
constexpr int kNumWt  = 16;    // waveforms in the bank

inline const char* const kWtNames[kNumWt] = {
    "SINE", "TRIANGLE", "SAW", "SQUARE", "PULSE 25", "PULSE 12", "BRIGHT", "ORGAN",
    "ODD", "FORMANT A", "FORMANT B", "SPARSE", "HOLLOW", "BRASS", "STRING", "VOCAL"};

// harmonic amplitude for waveform w, harmonic n (1-based). Classic Fourier
// series for the analog shapes, hand-tuned spectra for the character tables.
inline float wtHarmonic(int w, int n) noexcept
{
    const float fn = (float) n;
    switch(w)
    {
        case 0:  return n == 1 ? 1.0f : 0.0f;                                  // sine
        case 1:  return (n % 2 == 1) ? (1.0f / (fn * fn)) * ((n % 4 == 1) ? 1.0f : -1.0f) : 0.0f; // triangle
        case 2:  return 1.0f / fn;                                             // saw
        case 3:  return (n % 2 == 1) ? 1.0f / fn : 0.0f;                       // square
        case 4:  return std::fabs(std::sin(fn * 3.14159265f * 0.25f)) / fn * 2.0f;  // pulse 25%
        case 5:  return std::fabs(std::sin(fn * 3.14159265f * 0.125f)) / fn * 2.0f; // pulse 12.5%
        case 6:  return 1.0f / std::sqrt(fn);                                  // bright (slow rolloff)
        case 7:  { const float d[] = {1.0f, 0.7f, 0.5f, 0.4f, 0.0f, 0.3f, 0.0f, 0.2f}; return n <= 8 ? d[n - 1] : 0.0f; } // organ drawbars
        case 8:  return (n % 2 == 1) ? 1.0f / (fn * fn) : 0.0f;                // odd soft
        case 9:  { const float c = 3.0f; return std::exp(-((fn - c) * (fn - c)) / 4.0f) + 0.12f / fn; }  // formant ~3rd
        case 10: { const float c = 9.0f; return std::exp(-((fn - c) * (fn - c)) / 12.0f) + 0.06f / fn; } // formant ~9th
        case 11: return (n == 1 || n == 3 || n == 7 || n == 11 || n == 17) ? 1.0f / fn : 0.0f; // sparse
        case 12: return (n % 2 == 1) ? 1.2f / fn : 0.0f;                       // hollow (bright odd)
        case 13: { const float c = 5.0f; return (1.0f / fn) * (0.5f + std::exp(-((fn - c) * (fn - c)) / 20.0f)); } // brass
        case 14: return 1.0f / std::pow(fn, 1.2f);                             // string
        case 15: { float a = std::exp(-((fn - 4.0f) * (fn - 4.0f)) / 3.0f)
                        + 0.6f * std::exp(-((fn - 9.0f) * (fn - 9.0f)) / 6.0f)
                        + 0.3f * std::exp(-((fn - 13.0f) * (fn - 13.0f)) / 8.0f);
                   return a / fn + 0.05f / fn; }                               // vocal "ah"
        default: return 1.0f / fn;
    }
}

struct WtBank
{
    float t[kNumWt][kWtMips][kWtLen];

    WtBank() noexcept { generate(); }

    void generate() noexcept
    {
        for(int w = 0; w < kNumWt; ++w)
            for(int m = 0; m < kWtMips; ++m)
            {
                const int maxH = std::max(1, 256 >> m);
                float* tab = t[w][m];
                for(int i = 0; i < kWtLen; ++i) tab[i] = 0.0f;

                // additive synthesis via a rotating phasor (no sin() per sample)
                for(int n = 1; n <= maxH; ++n)
                {
                    const float a = wtHarmonic(w, n);
                    if(std::fabs(a) < 1e-5f) continue;
                    const float wn = 2.0f * 3.14159265f * (float) n / (float) kWtLen;
                    const float cw = std::cos(wn), sw = std::sin(wn);
                    float s = 0.0f, c = 1.0f;
                    for(int i = 0; i < kWtLen; ++i)
                    {
                        tab[i] += a * s;
                        const float ns = s * cw + c * sw;
                        c = c * cw - s * sw;
                        s = ns;
                    }
                }

                // normalise to +/-1
                float peak = 1e-9f;
                for(int i = 0; i < kWtLen; ++i) peak = std::max(peak, std::fabs(tab[i]));
                const float g = 1.0f / peak;
                for(int i = 0; i < kWtLen; ++i) tab[i] *= g;
            }
    }
};

// One shared bank, generated on first use (~0.1 s, one time).
inline const WtBank& wtBank() noexcept
{
    static const WtBank b;
    return b;
}
} // namespace jove

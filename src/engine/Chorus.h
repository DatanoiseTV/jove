/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cmath>
#include <cstdint>

namespace jove
{
// Classic BBD-style chorus / ensemble.
//
// A single mono delay line read by several modulated taps; the taps' pitch
// wobble is kept shallow (the perceived deviation of a swept delay is
// sweepMs*1e-3 * 2*pi*rate, ~+/-5 cents here) so it shimmers like a Juno/Solina
// instead of sounding detuned. Three voicings:
//
//   CHORUS I  — one slow ~0.5 Hz LFO, two quadrature taps (the Juno-1 sound).
//   CHORUS II — a faster, slightly deeper version.
//   ENSEMBLE  — THREE taps driven 120 degrees apart, panned left / centre /
//               right, for the lush wide string-machine ensemble.
//
// mix 0 = true bypass. Buffer (~21 ms) lives in normal RAM.
class Chorus
{
  public:
    enum Mode { ChorusI = 0, ChorusII, Ensemble, ModeCount };

    void prepare(float sampleRate) noexcept
    {
        sr_ = sampleRate;
        for(int i = 0; i < kSize; ++i)
            buf_[i] = 0.0f;
        wr_ = 0;
        ph_ = 0.0f;
    }

    void setMode(int m) noexcept { mode_ = (m < 0 || m >= ModeCount) ? ChorusI : m; }
    void setMix(float m) noexcept { mix_ = m < 0.0f ? 0.0f : (m > 1.0f ? 1.0f : m); }

    void process(float* l, float* r, int n) noexcept
    {
        if(mix_ < 0.001f)
            return;
        const float twoPi  = 6.2831853f;
        // per-mode rate (Hz) and sweep (ms)
        float rate, sweepMs;
        switch(mode_)
        {
            case ChorusII: rate = 0.83f; sweepMs = 1.1f; break;
            case Ensemble: rate = 0.6f;  sweepMs = 1.0f; break;
            default:       rate = 0.5f;  sweepMs = 0.9f; break; // Chorus I
        }
        const float baseSmp  = 7.0f * 0.001f * sr_;
        const float sweepSmp = sweepMs * 0.001f * sr_;
        const float inc      = rate / sr_;
        const bool  ens      = (mode_ == Ensemble);
        const float wet      = mix_;
        const float dry      = 1.0f - 0.5f * mix_;

        for(int i = 0; i < n; ++i)
        {
            buf_[wr_] = (l[i] + r[i]) * 0.5f; // mono feed

            if(ens)
            {
                // three taps 120 deg apart -> left / centre / right
                const float a = baseSmp + sweepSmp * std::sin(twoPi * ph_);
                const float b = baseSmp + sweepSmp * std::sin(twoPi * (ph_ + 0.3333f));
                const float c = baseSmp + sweepSmp * std::sin(twoPi * (ph_ + 0.6667f));
                const float ta = read(a), tb = read(b), tc = read(c);
                const float wl = ta + 0.6f * tb;
                const float wr = tc + 0.6f * tb;
                l[i] = l[i] * dry + wl * wet * 0.7f;
                r[i] = r[i] * dry + wr * wet * 0.7f;
            }
            else
            {
                const float dl = baseSmp + sweepSmp * std::sin(twoPi * ph_);
                const float dr = baseSmp + sweepSmp * std::sin(twoPi * (ph_ + 0.25f));
                l[i] = l[i] * dry + read(dl) * wet;
                r[i] = r[i] * dry + read(dr) * wet;
            }

            ph_ += inc;
            if(ph_ >= 1.0f) ph_ -= 1.0f;
            if(++wr_ >= kSize) wr_ = 0;
        }
    }

  private:
    static constexpr int kSize = 1024; // ~21 ms at 48 kHz

    inline float read(float delaySamples) const noexcept
    {
        float rp = (float)wr_ - delaySamples;
        while(rp < 0.0f) rp += (float)kSize;
        const int   i0 = (int)rp;
        const int   i1 = (i0 + 1) % kSize;
        const float fr = rp - (float)i0;
        return buf_[i0] + (buf_[i1] - buf_[i0]) * fr;
    }

    float sr_   = 48000.0f;
    float buf_[kSize];
    int   wr_   = 0;
    float ph_   = 0.0f;
    int   mode_ = ChorusI;
    float mix_  = 0.0f;
};

inline constexpr const char* kChorusModeNames[] = {"CHORUS I", "CHORUS II", "ENSEMBLE"};
} // namespace jove

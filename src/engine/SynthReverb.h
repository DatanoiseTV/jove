/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace jove
{
// Clean Schroeder-Moorer (Freeverb-style) stereo reverb: 8 damped comb filters
// in parallel into 4 allpass filters in series, per channel, with the right
// channel's delays offset for stereo width. There is NO modulation anywhere, so
// the tail is perfectly pitch-stable — the opposite of the tape engine's
// pitch-wobbling spring/plate. Buffers live in SDRAM. mix 0 = true bypass.
class SynthReverb
{
  public:
    void prepare(float sampleRate) noexcept
    {
        sr_         = sampleRate;
        const float k = sampleRate / 44100.0f; // Freeverb tunings are for 44.1 kHz
        static const int combTune[kNumComb] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        static const int apTune[kNumAp]     = {556, 441, 341, 225};
        // input-diffusion allpasses (prime, decorrelated) smear transients before
        // the tank so the tail is smooth instead of grainy/metallic.
        static const int diffTune[kNumDiff] = {142, 107, 379, 277};
        const int        spread             = (int)(23 * k);
        for(int c = 0; c < kNumComb; ++c)
        {
            int nL = (int)(combTune[c] * k);
            int nR = nL + spread;
            combL_[c].init(nL);
            combR_[c].init(nR);
        }
        for(int a = 0; a < kNumAp; ++a)
        {
            int nL = (int)(apTune[a] * k);
            int nR = nL + spread;
            apL_[a].init(nL, 0.5f);
            apR_[a].init(nR, 0.5f);
        }
        for(int d = 0; d < kNumDiff; ++d)
            diff_[d].init((int)(diffTune[d] * k), 0.7f);
        // pre-delay (~8 ms) — a touch of space before the tail builds, short
        // enough not to leave an audible gap when the mix is fully wet.
        preLen_ = (int)(0.008f * sampleRate) + 1;
        pre_.assign((size_t)preLen_, 0.0f);
        prePos_ = 0;
        reset();
    }

    void reset() noexcept
    {
        for(int c = 0; c < kNumComb; ++c) { combL_[c].clear(); combR_[c].clear(); }
        for(int a = 0; a < kNumAp; ++a)   { apL_[a].clear();   apR_[a].clear(); }
        for(int d = 0; d < kNumDiff; ++d) diff_[d].clear();
        std::fill(pre_.begin(), pre_.end(), 0.0f);
        prePos_ = 0;
    }

    // size 0..1 -> room feedback; damp 0..1 -> HF absorption; mix 0..1 wet.
    void setSize(float s) noexcept { roomfb_ = 0.7f + 0.28f * clamp01(s); }
    void setDamp(float d) noexcept { damp_ = clamp01(d); }
    void setMix(float m) noexcept { mix_ = clamp01(m); }

    void process(float* L, float* R, int n) noexcept
    {
        if(mix_ < 0.001f)
            return;
        const float d1 = damp_ * 0.4f;       // comb damping
        const float d2 = 1.0f - d1;
        const float in_gain = 0.015f;        // Freeverb fixed input scale
        const float modRate = 0.7f / sr_;    // ~0.7 Hz tail modulation
        const float modDepth = 7.0f;         // +/- samples
        // The comb bank's gain is ~1/(1-roomfb), so without this the tail gets
        // many times louder than the dry as size rises and even a small mix
        // buries the signal. Normalise the wet so its loudness is ~constant vs
        // size, then equal-power crossfade so low mix stays subtle and full wet
        // sits near the dry level instead of a quiet/overpowering wash.
        const float wetNorm = 3.6f * (1.0f - roomfb_);
        const float wetG = std::sin(mix_ * 1.5707963f) * wetNorm;
        const float dryG = std::cos(mix_ * 1.5707963f);
        for(int i = 0; i < n; ++i)
        {
            modPhase_ += modRate;
            if(modPhase_ >= 1.0f) modPhase_ -= 1.0f;
            // pre-delay the mono send, then diffuse it through the input allpasses
            float send = (L[i] + R[i]) * in_gain;
            pre_[(size_t)prePos_] = send;
            int rp = prePos_ - (preLen_ - 1);
            if(rp < 0) rp += preLen_;
            send = pre_[(size_t)rp];
            if(++prePos_ >= preLen_) prePos_ = 0;
            for(int d = 0; d < kNumDiff; ++d) send = diff_[d].process(send);
            const float input = send;
            float wl = 0.0f, wr = 0.0f;
            for(int c = 0; c < kNumComb; ++c)
            {
                const float ml = modDepth * std::sin(6.2831853f * (modPhase_ + (float) c * 0.13f));
                const float mr = modDepth * std::sin(6.2831853f * (modPhase_ + (float) c * 0.13f + 0.5f));
                wl += combL_[c].process(input, roomfb_, d1, d2, ml);
                wr += combR_[c].process(input, roomfb_, d1, d2, mr);
            }
            for(int a = 0; a < kNumAp; ++a)
            {
                wl = apL_[a].process(wl);
                wr = apR_[a].process(wr);
            }
            L[i] = L[i] * dryG + wl * wetG;
            R[i] = R[i] * dryG + wr * wetG;
        }
    }

  private:
    static constexpr int kNumComb = 8;
    static constexpr int kNumAp   = 4;
    static constexpr int kNumDiff = 4; // input-diffusion allpasses

    static inline float clamp01(float v) noexcept { return v < 0 ? 0 : (v > 1 ? 1 : v); }

    struct Comb
    {
        std::vector<float> buf;
        int    len = 0, pos = 0;
        float  store = 0.0f;
        void   init(int n)
        {
            len = n < 1 ? 1 : n;
            buf.assign((size_t)len, 0.0f);
            pos = 0;
            store = 0.0f;
        }
        void clear()
        {
            std::fill(buf.begin(), buf.end(), 0.0f);
            store = 0.0f;
        }
        inline float process(float in, float fb, float d1, float d2, float modSamp) noexcept
        {
            // fractional, slowly-modulated read -> detunes the tail so it shimmers
            // like a real room instead of ringing metallically. modSamp is +/- a
            // few samples, so rp must be wrapped on BOTH sides: a negative modSamp
            // pushes rp past the buffer end, and an unbounded i0 there reads heap
            // garbage straight into the feedback -> metallic self-oscillation.
            float rp = (float) pos - modSamp;
            while(rp < 0.0f)         rp += (float) len;
            while(rp >= (float) len) rp -= (float) len;
            int i0 = (int) rp;
            if(i0 >= len) i0 = len - 1; // guard the fp boundary
            const int i1 = (i0 + 1 == len) ? 0 : i0 + 1;
            const float out = buf[i0] + (buf[i1] - buf[i0]) * (rp - (float) i0);
            store = out * d2 + store * d1; // damping low-pass
            buf[pos] = in + store * fb;
            if(++pos >= len) pos = 0;
            return out;
        }
    };

    struct Allpass
    {
        std::vector<float> buf;
        int    len = 0, pos = 0;
        float  coef = 0.5f;
        void   init(int n, float c)
        {
            len = n < 1 ? 1 : n;
            buf.assign((size_t)len, 0.0f);
            pos = 0;
            coef = c;
        }
        void clear() { std::fill(buf.begin(), buf.end(), 0.0f); }
        inline float process(float in) noexcept
        {
            const float bufout = buf[pos];
            const float out    = -in + bufout;
            buf[pos] = in + bufout * coef;
            if(++pos >= len) pos = 0;
            return out;
        }
    };

    float   sr_     = 48000.0f;
    Comb    combL_[kNumComb], combR_[kNumComb];
    Allpass apL_[kNumAp], apR_[kNumAp];
    Allpass diff_[kNumDiff];           // input diffusion (mono, pre-tank)
    std::vector<float> pre_;           // pre-delay line
    int     preLen_ = 1, prePos_ = 0;
    float   roomfb_ = 0.84f;
    float   damp_   = 0.4f;
    float   mix_    = 0.0f;
    float   modPhase_ = 0.0f; // slow tail-modulation phase
};
} // namespace jove

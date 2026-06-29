/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "../dub/Svf.h"
#include <cmath>
#include <vector>

namespace jove
{
// Clean stereo delay for the synth FX. Unlike the tape engine, the delay time is
// FIXED (only smoothed gently on a tempo change), so the echoes never pitch-shift
// — the modulated tape time was what made the old echoes warble out of tune. A
// one-pole low-pass in the feedback path darkens successive repeats (the musical
// "analog-ish" decay) without any modulation. Optional ping-pong. Buffers live in
// SDRAM. mix 0 = true bypass.
class SynthDelay
{
  public:
    void prepare(float sampleRate) noexcept
    {
        sr_     = sampleRate;
        maxLen_ = (int)(2.0f * sampleRate) + 4; // up to 2 s
        bufL_.assign((size_t)maxLen_, 0.0f);
        bufR_.assign((size_t)maxLen_, 0.0f);
        pfL_.prepare(sampleRate);
        pfR_.prepare(sampleRate);
        reset();
    }

    // pre-filter in the feedback path: type 0 LP / 1 HP / 2 BP, freq Hz, q 0..1
    void setFilter(int type, float freqHz, float q) noexcept
    {
        pfType_ = (type < 0 || type > 2) ? 0 : type;
        pfL_.setParams(freqHz, q < 0.0f ? 0.0f : (q > 0.95f ? 0.95f : q));
        pfR_.copyCoefsFrom(pfL_);
    }

    void reset() noexcept
    {
        wr_ = 0;
        if(!bufL_.empty())
            for(int i = 0; i < maxLen_; ++i) { bufL_[i] = 0.0f; bufR_[i] = 0.0f; }
        lpL_ = lpR_ = 0.0f;
        curL_ = curR_ = 0.0f;
    }

    // target delay time per channel, in samples
    void setTime(float samplesL, float samplesR) noexcept
    {
        tgtL_ = clampLen(samplesL);
        tgtR_ = clampLen(samplesR);
    }
    void setFeedback(float f) noexcept { fb_ = f < 0.0f ? 0.0f : (f > 0.95f ? 0.95f : f); }
    void setDamp(float d) noexcept { damp_ = d < 0.0f ? 0.0f : (d > 1.0f ? 1.0f : d); }
    void setMix(float m) noexcept { mix_ = m < 0.0f ? 0.0f : (m > 1.0f ? 1.0f : m); }
    void setPingPong(bool p) noexcept { ping_ = p; }
    // colour: 0 DIGITAL (clean), 1 ANALOG (BBD: darker + grittier feedback),
    // 2 TAPE (darker + soft sat + subtle wow/flutter on the read head).
    void setMode(int m) noexcept { mode_ = (m < 0 || m > 2) ? 0 : m; }

    void process(float* L, float* R, int n) noexcept
    {
        if(bufL_.empty() || mix_ < 0.001f)
            return;
        // damping coefficient (one-pole LP); more damp = darker repeats. Analog /
        // tape modes darken the feedback further and drive it a touch harder.
        const float dcoef = 0.05f + 0.9f * (1.0f - damp_);
        const float dc = (mode_ == 1) ? dcoef * 0.6f : (mode_ == 2 ? dcoef * 0.7f : dcoef);
        const float satDrive = (mode_ == 1) ? 1.3f : (mode_ == 2 ? 1.18f : 1.0f);
        constexpr float twoPi = 6.2831853f;
        for(int i = 0; i < n; ++i)
        {
            // glide the delay time slowly toward the target (click-free on a
            // tempo change; constant otherwise so there's no pitch artifact)
            curL_ += 0.0008f * (tgtL_ - curL_);
            curR_ += 0.0008f * (tgtR_ - curR_);

            // TAPE: a subtle wow (~0.7 Hz) + flutter (~6.3 Hz) on the read head
            // for tape character. Kept to ~2 samples so it colours without the
            // audible out-of-tune warble the old tape engine had.
            float wowL = 0.0f, wowR = 0.0f;
            if(mode_ == 2)
            {
                wowPh_ += 0.7f / sr_;  if(wowPh_ >= 1.0f) wowPh_ -= 1.0f;
                flPh_  += 6.3f / sr_;  if(flPh_  >= 1.0f) flPh_  -= 1.0f;
                const float w = std::sin(twoPi * wowPh_) * 1.6f + std::sin(twoPi * flPh_) * 0.4f;
                wowL = w; wowR = w * 0.92f;
            }

            const float dL = readInterp(bufL_.data(), curL_ + wowL);
            const float dR = readInterp(bufR_.data(), curR_ + wowR);

            // feedback path: gentle one-pole damp, then the LP/BP/HP pre-filter
            lpL_ += dc * (dL - lpL_);
            lpR_ += dc * (dR - lpR_);
            const float fL = pfL_.process(lpL_, pfType_);
            const float fR = pfR_.process(lpR_, pfType_);

            float wL, wR;
            if(ping_)
            {
                // cross the feedback so repeats bounce L<->R
                wL = L[i] + fb_ * fR;
                wR = R[i] + fb_ * fL;
            }
            else
            {
                wL = L[i] + fb_ * fL;
                wR = R[i] + fb_ * fR;
            }
            // Soft-saturate the recirculating signal. fb < 1 is geometrically
            // stable for a bounded input, but a sustained near-self-oscillating
            // resonant voice can pile repeats into a howl; clamping what re-enters
            // the line bounds the loop unconditionally and adds analog-tape feel.
            bufL_[wr_] = sat(wL * satDrive);
            bufR_[wr_] = sat(wR * satDrive);
            if(++wr_ >= maxLen_) wr_ = 0;

            // Wet tap is the FILTERED signal (fL/fR), not the raw read — so the
            // LP/HP/BP type, FREQ and Q audibly shape every echo, not just the
            // recirculating tail. (Previously the first echo bypassed the filter,
            // which made the controls feel inert at high cutoffs.)
            L[i] = L[i] * (1.0f - 0.5f * mix_) + fL * mix_;
            R[i] = R[i] * (1.0f - 0.5f * mix_) + fR * mix_;
        }
    }

  private:
    // cubic soft-clip: ~linear below 1, smoothly saturating to +/-1 past ~1.5
    static inline float sat(float x) noexcept
    {
        if(x < -1.5f) return -1.0f;
        if(x >  1.5f) return  1.0f;
        return x - (4.0f / 27.0f) * x * x * x;
    }
    float clampLen(float s) const noexcept
    {
        const float hi = (float)(maxLen_ - 4);
        return s < 4.0f ? 4.0f : (s > hi ? hi : s);
    }
    inline float readInterp(const float* buf, float delaySamples) const noexcept
    {
        // bound the (possibly wow-modulated) read distance into the valid range
        if(delaySamples < 1.0f) delaySamples = 1.0f;
        const float maxd = (float)(maxLen_ - 2);
        if(delaySamples > maxd) delaySamples = maxd;
        float rp = (float)wr_ - delaySamples;
        while(rp < 0.0f) rp += (float)maxLen_;
        const int   i0 = (int)rp;
        const int   i1 = (i0 + 1) % maxLen_;
        const float fr = rp - (float)i0;
        return buf[i0] + (buf[i1] - buf[i0]) * fr;
    }

    float  sr_     = 48000.0f;
    int    maxLen_ = 0;
    std::vector<float> bufL_;
    std::vector<float> bufR_;
    int    wr_     = 0;
    float  tgtL_ = 12000.0f, tgtR_ = 12000.0f;
    float  curL_ = 12000.0f, curR_ = 12000.0f;
    float  lpL_ = 0.0f, lpR_ = 0.0f;
    doobie::Svf pfL_, pfR_;       // feedback pre-filter (LP/BP/HP)
    int    pfType_ = 0;
    float  fb_   = 0.35f;
    float  damp_ = 0.4f;
    float  mix_  = 0.0f;
    bool   ping_ = true;
    int    mode_ = 0;                  // 0 digital, 1 analog, 2 tape
    float  wowPh_ = 0.0f, flPh_ = 0.0f; // tape wow/flutter phases
};
} // namespace jove

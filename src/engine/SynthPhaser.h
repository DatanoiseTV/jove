/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cmath>

namespace jove
{
// Classic 6-stage first-order all-pass phaser with feedback, ported from the
// Doobie dub-delay (doobie::Phaser). A StereoPhaser runs two mono cores with the
// right channel a half-cycle out of phase, so the notches sweep oppositely for
// width — the e-piano / clav / pad "swoosh".
//
// The sweep coefficient uses the bilinear map without prewarp (a = (1-w)/(1+w));
// for a moving notch that error is inaudible and it keeps tan() out of the
// sample loop. mix 0 = true bypass at the StereoPhaser level.
class PhaserCore
{
  public:
    static constexpr int kStages = 6;

    void prepare(float sampleRate) noexcept
    {
        sr_ = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for(int i = 0; i < kStages; ++i)
            z_[i] = 0.0f;
        last_  = 0.0f;
        phase_ = phaseOffset_;
    }

    void setPhaseOffset(float cycles) noexcept { phaseOffset_ = cycles; }

    // per block: rate in Hz, depth/fb/mix 0..1 (fb capped below runaway)
    void setParams(float rateHz, float depth, float fb, float mix) noexcept
    {
        if(rateHz < 0.01f) rateHz = 0.01f;
        if(rateHz > 8.0f)  rateHz = 8.0f;
        inc_   = rateHz / sr_;
        depth_ = depth < 0.0f ? 0.0f : (depth > 1.0f ? 1.0f : depth);
        fb_    = fb < 0.0f ? 0.0f : (fb > 0.9f ? 0.9f : fb);
        mix_   = mix < 0.0f ? 0.0f : (mix > 1.0f ? 1.0f : mix);
    }

    // Evaluate the all-pass coefficient for the current LFO phase, then advance
    // the phase by a whole block of n samples. The sweep is slow (a fraction of a
    // Hz), so stepping the coefficient once per block is inaudible and keeps the
    // one exp2f out of the per-sample loop — matching how the voice filter only
    // re-evaluates its transcendentals at block rate.
    float blockCoeff(int n) noexcept
    {
        // triangle LFO (the classic phaser shape) sweeping the all-pass corner
        // around 800 Hz by up to +/-2 octaves (200 Hz .. 3.2 kHz at full depth)
        const float lfo = 4.0f * std::fabs(phase_ - 0.5f) - 1.0f;
        const float f   = 800.0f * exp2f(depth_ * 2.0f * lfo);
        const float w   = 3.14159265f * f / sr_;
        coeff_ = (1.0f - w) / (1.0f + w);
        phase_ += inc_ * (float)n;
        while(phase_ >= 1.0f)
            phase_ -= 1.0f;
        return coeff_;
    }

    inline float process(float in) noexcept
    {
        const float a = coeff_;
        float x = in + fb_ * last_;
        for(int i = 0; i < kStages; ++i)
        {
            const float y = -a * x + z_[i];
            z_[i]         = x + a * y;
            x             = y;
        }
        if(!std::isfinite(x)) // never let the feedback path run away
        {
            x = 0.0f;
            reset();
        }
        last_ = x;
        return in * (1.0f - mix_) + x * mix_;
    }

  private:
    float sr_ = 48000.0f;
    float z_[kStages] = {};
    float last_ = 0.0f, phase_ = 0.0f, phaseOffset_ = 0.0f;
    float inc_ = 0.0f, depth_ = 0.7f, fb_ = 0.4f, mix_ = 0.5f;
    float coeff_ = 0.0f; // all-pass coefficient, refreshed once per block
};

// Stereo wrapper: two cores, R offset half a cycle for width. Fixed musical
// rate/depth/feedback; the patch's one fxPhaser macro sets the wet mix.
class StereoPhaser
{
  public:
    void prepare(float sampleRate) noexcept
    {
        l_.prepare(sampleRate);
        r_.prepare(sampleRate);
        r_.setPhaseOffset(0.5f);
        l_.reset();
        r_.reset();
    }

    void setMix(float mix) noexcept { mix_ = mix; }

    void process(float* bl, float* br, int n) noexcept
    {
        if(mix_ < 0.001f) // true bypass — also resets so re-engaging is clickless
        {
            if(active_) { l_.reset(); r_.reset(); active_ = false; }
            return;
        }
        active_ = true;
        // slow, fairly deep, moderate feedback — the vintage e-piano/pad sweep.
        // Coefficient is evaluated once per block (the sweep is slow), then held
        // across the sample loop so there's no per-sample exp2f.
        l_.setParams(0.25f, 0.85f, 0.45f, mix_);
        r_.setParams(0.25f, 0.85f, 0.45f, mix_);
        l_.blockCoeff(n);
        r_.blockCoeff(n);
        for(int i = 0; i < n; ++i)
        {
            bl[i] = l_.process(bl[i]);
            br[i] = r_.process(br[i]);
        }
    }

  private:
    PhaserCore l_, r_;
    float      mix_    = 0.0f;
    bool       active_ = false;
};
} // namespace jove

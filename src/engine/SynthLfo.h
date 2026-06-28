/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "SynthParams.h"
#include <cmath>
#include <cstdint>

namespace jove
{
// Block-rate LFO. Outputs bipolar [-1, 1] scaled by depth, with an optional
// fade-in and note-retrigger. S&H samples new noise at each cycle wrap. A small
// one-pole smoother on the output keeps stepped waves (square / S&H) from
// clicking when they drive audio-path destinations like cutoff.
class SynthLfo
{
  public:
    void prepare(float sampleRate, float blockRate) noexcept
    {
        sr_        = sampleRate;
        blockRate_ = blockRate;
        reset();
    }

    void reset() noexcept
    {
        phase_  = 0.0f;
        held_   = 0.0f;
        out_    = 0.0f;
        smooth_ = 0.0f;
        fadeGain_ = 1.0f;
        rng_    = 0x1234567u;
    }

    void retrigger(float startPhase = 0.0f) noexcept
    {
        phase_      = startPhase;
        fadeGain_   = 0.0f;          // restart the fade-in
        smooth_     = 0.0f;
        delayCount_ = delayBlocks_;  // hold silent for the delay, then fade in
    }

    void setWave(int w) noexcept { wave_ = w; }
    void setRate(float hz) noexcept { rate_ = hz < 0.001f ? 0.001f : hz; }
    void setDepth(float d) noexcept { depth_ = d; }
    void setFade(float seconds) noexcept { fadeInc_ = seconds < 0.001f ? 1.0f : (1.0f / (seconds * blockRate_)); }
    // Delayed vibrato: the LFO stays silent for `seconds` after a note starts,
    // then fades in over the fade time. Phase holds at its retrigger value during
    // the delay so the wobble grows smoothly from centre (no jump).
    void setDelay(float seconds) noexcept { delayBlocks_ = seconds < 0.0f ? 0.0f : seconds * blockRate_; }
    // Bipolar DC bias added to the output so the LFO can sweep around a point
    // other than centre (e.g. an upward-only filter wobble).
    void setOffset(float o) noexcept { offset_ = o; }

    // Advance one control block; returns the smoothed bipolar value * depth + offset.
    float advance() noexcept
    {
        if(delayCount_ > 0.0f) // still in the pre-fade delay -> hold at the bias
        {
            delayCount_ -= 1.0f;
            out_ = offset_;
            return out_;
        }
        const float prev = phase_;
        phase_ += rate_ / blockRate_;
        const bool wrap = phase_ >= 1.0f;
        if(wrap)
            phase_ -= std::floor(phase_);
        if(wave_ == (int)LfoWave::SampleHold && (wrap || prev == 0.0f))
            held_ = noise();

        float v = shape();
        // smoother (~one block of glide) to de-zipper stepped shapes
        smooth_ += 0.5f * (v - smooth_);

        if(fadeGain_ < 1.0f)
        {
            fadeGain_ += fadeInc_;
            if(fadeGain_ > 1.0f)
                fadeGain_ = 1.0f;
        }
        out_ = smooth_ * depth_ * fadeGain_ + offset_;
        return out_;
    }

    float value() const noexcept { return out_; }
    float phase() const noexcept { return phase_; }

  private:
    inline float shape() const noexcept
    {
        constexpr float twoPi = 6.2831853f;
        switch((LfoWave)wave_)
        {
            case LfoWave::Sine: return std::sin(twoPi * phase_);
            case LfoWave::Triangle:
                return phase_ < 0.5f ? (4.0f * phase_ - 1.0f) : (3.0f - 4.0f * phase_);
            case LfoWave::SawUp: return 2.0f * phase_ - 1.0f;
            case LfoWave::SawDown: return 1.0f - 2.0f * phase_;
            case LfoWave::Square: return phase_ < 0.5f ? 1.0f : -1.0f;
            case LfoWave::SampleHold: return held_;
            default: return 0.0f;
        }
    }

    inline float noise() noexcept
    {
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        return (float)(int32_t)rng_ * (1.0f / 2147483648.0f);
    }

    float    sr_        = 48000.0f;
    float    blockRate_ = 1000.0f;
    int      wave_      = 0;
    float    rate_      = 1.0f;
    float    depth_     = 1.0f;
    float    phase_     = 0.0f;
    float    held_      = 0.0f;
    float    out_       = 0.0f;
    float    offset_    = 0.0f;
    float    smooth_    = 0.0f;
    float    fadeGain_  = 1.0f;
    float    fadeInc_   = 1.0f;
    float    delayBlocks_ = 0.0f; // pre-fade hold length, in control blocks
    float    delayCount_  = 0.0f; // remaining delay blocks after a retrigger
    uint32_t rng_       = 0x1234567u;
};
} // namespace jove

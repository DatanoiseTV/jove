/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "Arpeggiator.h" // kArpDivQuarters / kNumArpDiv
#include "SynthParams.h"
#include <cmath>
#include <cstdint>

namespace jove
{
// Block-rate step/curve sequencer used as a modulation source. Same shape as
// SynthLfo: advanced once per control block, exposes a smoothed bipolar value
// that feeds the mod matrix. Per-step values are bipolar [-1, 1]; the curve
// selects sample-and-hold, linear ramp, or a smoothstep glide between steps.
// Tempo-synced (per-step duration from a note division) or free (steps/sec),
// with optional swing on odd steps. A one-pole output smoother keeps STEP edges
// click-free when driving audio-path destinations like cutoff.
class Sequencer
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
        cur_    = 0;
        smooth_ = 0.0f;
        out_    = 0.0f;
    }

    void retrigger() noexcept { phase_ = 0.0f; cur_ = 0; pendDir_ = 1; } // restart at step 0

    void setLength(int n) noexcept { len_ = n < 1 ? 1 : (n > kSeqMaxSteps ? kSeqMaxSteps : n); }
    void setDir(int d) noexcept { dir_ = d; }
    void setMode(int m) noexcept { mode_ = m; }
    void setCurve(int c) noexcept { curve_ = c; }
    void setDepth(float d) noexcept { depth_ = d; }
    void setSwing(float s) noexcept { swing_ = s; }
    void setStep(int i, float v) noexcept { if(i >= 0 && i < kSeqMaxSteps) step_[i] = v; }

    // Advance one control block of n samples; returns the smoothed value scaled by
    // depth. tempoBpm + (sync, div) or the free rate (steps/sec) set the step
    // period. Direction governs how the step index moves; melodic mode quantises
    // the per-step values to semitones (1/12 grid) so a pitch route plays in tune.
    float advance(int n, float tempoBpm, bool sync, int div, float rateHz) noexcept
    {
        double stepSec;
        if(sync)
        {
            const double secQ = 60.0 / (tempoBpm < 1.0f ? 120.0 : tempoBpm);
            int di = div < 0 ? 0 : (div >= kNumArpDiv ? kNumArpDiv - 1 : div);
            stepSec = secQ * kArpDivQuarters[di];
        }
        else
        {
            stepSec = 1.0 / (rateHz < 0.001f ? 0.001f : rateHz);
        }
        // swing: lengthen even steps, shorten odd (matches the arp)
        double thisSec = stepSec;
        if(swing_ > 0.001f)
            thisSec = (cur_ & 1) ? stepSec * (1.0 - swing_) : stepSec * (1.0 + swing_);

        phase_ += (float)((double) n / (thisSec * (double) sr_));
        while(phase_ >= 1.0f) { phase_ -= 1.0f; stepAdvance(); }

        float a = step_[cur_], b = step_[nextIndex(cur_)];
        if(mode_ == (int) SeqMode::Melodic) { a = quantize(a); b = quantize(b); }
        float f;
        switch((SeqCurve) curve_)
        {
            case SeqCurve::Linear: f = phase_; break;
            case SeqCurve::Smooth: f = phase_ * phase_ * (3.0f - 2.0f * phase_); break;
            default:               f = 0.0f; // STEP: hold the current value
        }
        const float v = a + (b - a) * f;
        smooth_ += 0.5f * (v - smooth_); // de-zipper stepped edges
        out_ = smooth_ * depth_;
        return out_;
    }

    float value() const noexcept { return out_; }
    int   stepIndex() const noexcept { return cur_; } // UI playhead

  private:
    // semitone grid: round to 1/12 so a route to ModDest::Pitch (which scales by
    // 12) lands on whole semitones across a +/-1 octave (x2 range with amount).
    static inline float quantize(float v) noexcept { return std::round(v * 12.0f) / 12.0f; }

    // index that follows c in the current direction (interpolation target)
    int nextIndex(int c) const noexcept
    {
        switch((SeqDir) dir_)
        {
            case SeqDir::Reverse:  return (c - 1 + len_) % len_;
            case SeqDir::Pendulum: { int nx = c + pendDir_; return (nx < 0 || nx >= len_) ? c : nx; }
            case SeqDir::Random:   return c; // no meaningful glide target
            default:               return (c + 1) % len_;
        }
    }

    void stepAdvance() noexcept
    {
        switch((SeqDir) dir_)
        {
            case SeqDir::Reverse: cur_ = (cur_ - 1 + len_) % len_; break;
            case SeqDir::Pendulum:
                if(len_ <= 1) { cur_ = 0; break; }
                cur_ += pendDir_;
                if(cur_ >= len_)     { cur_ = len_ - 2; pendDir_ = -1; }
                else if(cur_ < 0)    { cur_ = 1;        pendDir_ =  1; }
                break;
            case SeqDir::Random:
            {
                rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
                cur_ = (int)((rng_ >> 8) % (uint32_t) len_);
                break;
            }
            default: cur_ = (cur_ + 1) % len_;
        }
    }

    float sr_        = 48000.0f;
    float blockRate_ = 1000.0f;
    float step_[kSeqMaxSteps] = {0};
    int   len_   = 16;
    int   dir_   = 0;   // SeqDir
    int   mode_  = 0;   // SeqMode (0 curve, 1 melodic)
    int   curve_ = 0;
    int   cur_   = 0;
    int   pendDir_ = 1; // pendulum direction (+1 / -1)
    float phase_  = 0.0f;
    float depth_  = 1.0f;
    float swing_  = 0.0f;
    float smooth_ = 0.0f;
    float out_    = 0.0f;
    uint32_t rng_ = 0x9E3779B9u; // random-direction PRNG
};
} // namespace jove

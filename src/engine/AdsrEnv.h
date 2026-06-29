/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cmath>

namespace jove
{
// Analog-style ADSR with exponential segment curves.
//
// Each stage approaches its target along a one-pole curve (the shape a real
// envelope IC's capacitor follows), which is what gives analog envelopes their
// snap on short times and their smooth tails on long ones. Crucially the output
// is *always* continuous: a note-off mid-attack releases from wherever the level
// currently is, and a re-trigger attacks from the current level — so no stage
// transition ever steps the VCA gain (the classic source of envelope clicks).
//
// Times are in seconds; the per-sample coefficient is exp(-1/(t*sr)). A tiny
// floor on each time keeps even a "0 ms" attack a couple of samples long, which
// removes the click a literally-instant gain step would make while still sounding
// instant.
class AdsrEnv
{
  public:
    enum class Stage
    {
        Idle = 0,
        Attack,
        Decay,
        Sustain,
        Release
    };

    void prepare(float sampleRate) noexcept
    {
        sr_ = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        stage_ = Stage::Idle;
        level_ = 0.0f;
    }

    // times in seconds, sustain 0..1
    void setAttack(float s) noexcept { aCoef_ = attCoef(s); }
    void setDecay(float s) noexcept { dCoef_ = decCoef(s); }
    void setSustain(float v) noexcept { sustain_ = clamp01(v); }
    // Release uses a coefficient that reaches near-silence (kEps) in `s` seconds,
    // so the release TIME a patch sets is the actual time-to-silence. The plain
    // time-constant coef() would ring ~9x longer (level only falls to 1/e per
    // `s`), which left notes clearly audible long after key-up.
    void setRelease(float s) noexcept { rCoef_ = relCoef(s); }

    void gateOn() noexcept { stage_ = Stage::Attack; }
    void gateOff() noexcept
    {
        if(stage_ != Stage::Idle)
            stage_ = Stage::Release;
    }

    bool active() const noexcept { return stage_ != Stage::Idle; }
    float value() const noexcept { return level_; }

    // One sample of envelope. The attack aims slightly past 1.0 (the standard
    // analog trick) so the exponential actually reaches 1.0 in finite time and
    // hands off to decay crisply instead of crawling asymptotically.
    inline float process() noexcept
    {
        switch(stage_)
        {
            case Stage::Idle: return 0.0f;
            case Stage::Attack:
                level_ = kAttackAim + (level_ - kAttackAim) * aCoef_;
                if(level_ >= 1.0f)
                {
                    level_ = 1.0f;
                    stage_ = Stage::Decay;
                }
                break;
            case Stage::Decay:
                level_ = sustain_ + (level_ - sustain_) * dCoef_;
                if(level_ <= sustain_ + kEps)
                {
                    level_ = sustain_;
                    stage_ = Stage::Sustain;
                }
                break;
            case Stage::Sustain: level_ = sustain_; break;
            case Stage::Release:
                level_ = level_ * rCoef_;
                if(level_ <= kEps)
                {
                    level_ = 0.0f;
                    stage_ = Stage::Idle;
                }
                break;
        }
        return level_;
    }

  private:
    static constexpr float kAttackAim = 1.2f; // overshoot target for a finite attack
    static constexpr float kEps       = 1.0e-4f;

    static inline float clamp01(float v) noexcept
    {
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    inline float coef(float seconds) noexcept
    {
        if(seconds < 0.0005f)
            seconds = 0.0005f; // ~0.5 ms floor -> click-free "instant"
        return std::exp(-1.0f / (seconds * sr_));
    }

    // Attack reaches 1.0 in `seconds` (target time, not time-constant). The curve
    // aims past 1.0 at kAttackAim, so when it crosses 1.0 the remaining span is
    // (kAttackAim-1)/kAttackAim; size the coefficient to consume that in `seconds`.
    inline float attCoef(float seconds) noexcept
    {
        if(seconds < 0.0005f) seconds = 0.0005f;
        const float frac = (kAttackAim - 1.0f) / kAttackAim; // remaining at level 1.0
        return std::exp(std::log(frac) / (seconds * sr_));
    }

    // Decay reaches ~99% of the way to sustain in `seconds` (target time), so the
    // decay number is the audible decay length rather than a ~5x-longer constant.
    inline float decCoef(float seconds) noexcept
    {
        if(seconds < 0.0005f) seconds = 0.0005f;
        return std::exp(std::log(0.01f) / (seconds * sr_));
    }

    // One-pole coefficient that decays from 1.0 to the silence floor kEps in
    // `seconds` (target-time, not time-constant) — used for release so the tail
    // length matches the patch's release number.
    inline float relCoef(float seconds) noexcept
    {
        if(seconds < 0.0005f)
            seconds = 0.0005f;
        return std::exp(std::log(kEps) / (seconds * sr_));
    }

    float sr_      = 48000.0f;
    float level_   = 0.0f;
    float sustain_ = 0.7f;
    float aCoef_   = 0.0f;
    float dCoef_   = 0.0f;
    float rCoef_   = 0.0f;
    Stage stage_   = Stage::Idle;
};
} // namespace jove

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
// Band-limited morphing oscillator.
//
// A single phase accumulator feeds three individually band-limited shapes —
// triangle, sawtooth, variable-width pulse — and the MORPH control crossfades
// across them:
//
//     morph 0.00 .............. 0.50 .............. 1.00
//           TRIANGLE ---------- SAW ---------- PULSE(width)
//
// Crossfading already-band-limited signals stays band-limited (it's a linear
// combination), so morphing never introduces aliasing. The saw and pulse edges
// are anti-aliased with PolyBLEP; the triangle uses PolyBLAMP (the integral of
// PolyBLEP) on its slope discontinuities, so it has no high-frequency hash even
// at the top of the keyboard. The pulse is synthesised as the difference of two
// sawtooths offset by the duty cycle, which is inherently DC-free at any width
// (so PWM never pumps the DC of the voice).
//
// Everything is computed per sample; hard sync and an external phase-mod input
// (for through-zero-ish FM / cross-mod) are supported. No branches on the audio
// hot path beyond the cheap edge tests.
class BlepOsc
{
  public:
    void prepare(float sampleRate) noexcept
    {
        sr_    = sampleRate;
        invSr_ = 1.0f / sampleRate;
        reset();
    }

    void reset() noexcept { phase_ = 0.0f; }

    // Randomise the start phase a touch so stacked unison voices and re-triggered
    // notes don't phase-align into an unnaturally static, comb-filtered attack.
    void setPhase(float p) noexcept { phase_ = p - std::floor(p); }

    void setFrequency(float hz) noexcept
    {
        // dt = phase increment per sample (the normalised frequency). Clamp below
        // Nyquist so the BLEP residual math stays well-defined at extreme pitch
        // modulation.
        float dt = hz * invSr_;
        if(dt > 0.45f)
            dt = 0.45f;
        if(dt < 0.0f)
            dt = 0.0f;
        dt_ = dt;
    }

    void setMorph(float m) noexcept
    {
        morph_ = m < 0.0f ? 0.0f : (m > 1.0f ? 1.0f : m);
    }

    // Pulse duty cycle, kept away from the 0/1 rails where a pulse degenerates to
    // DC and the two-saw construction loses a full edge per cycle.
    void setPulseWidth(float w) noexcept
    {
        pw_ = w < 0.05f ? 0.05f : (w > 0.95f ? 0.95f : w);
    }

    // Render one sample. phaseMod is an external offset (in cycles) summed into
    // the phase for FM / cross-mod; pass 0 for none. syncOut is set true on the
    // sample where this oscillator wrapped, so a follower can hard-sync to it.
    inline float process(float phaseMod, bool& wrapped) noexcept
    {
        wrapped = false;
        phase_ += dt_;
        if(phase_ >= 1.0f)
        {
            phase_ -= 1.0f;
            wrapped = true;
        }

        // Effective read phase (with FM offset) wrapped into [0,1).
        float p = phase_ + phaseMod;
        p -= std::floor(p);

        // Morph axis: SINE (0) -> TRIANGLE (0.25) -> SAW (0.5) -> PULSE (1).
        // Sine (inherently band-limited) is the carrier shape FM e-pianos, bells
        // and sub-bass want. Only the TWO shapes being blended are computed.
        if(morph_ < 0.001f) // pure sine fast path (e-pianos, sub) — no triangle
            return std::sin(6.2831853f * p);
        if(morph_ < 0.25f) // sine -> triangle
        {
            const float sine = std::sin(6.2831853f * p);
            const float tri  = triSample(p);
            const float t    = morph_ * 4.0f;
            return sine + (tri - sine) * t;
        }
        if(morph_ < 0.5f) // triangle -> saw
        {
            const float tri = triSample(p);
            const float saw = sawSample(p);
            const float t   = (morph_ - 0.25f) * 4.0f;
            return tri + (saw - tri) * t;
        }
        // saw -> pulse
        const float saw   = sawSample(p);
        const float pulse = pulseSample(p);
        const float t     = (morph_ - 0.5f) * 2.0f;
        return saw + (pulse - saw) * t;
    }

    // Force the phase (hard sync from a master oscillator). Resets the integrator
    // edge so the synced triangle doesn't ring.
    inline void hardSync(float masterFrac) noexcept
    {
        phase_ = masterFrac;
    }

    // Soft sync: only reset when the slave is past mid-cycle. The master's reset
    // is ignored during the slave's first half-cycle, so the slave free-runs more
    // often — fewer reset discontinuities and a gentler, less harmonically
    // aggressive timbre than hard sync.
    inline void softSync(float masterFrac) noexcept
    {
        if(phase_ >= 0.5f)
            phase_ = masterFrac;
    }

    float phase() const noexcept { return phase_; }

  private:
    // PolyBLEP: correction added around a step discontinuity to band-limit it.
    inline float polyBlep(float t) const noexcept
    {
        const float dt = dt_;
        if(dt <= 0.0f)
            return 0.0f;
        if(t < dt) // just after the wrap
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if(t > 1.0f - dt) // just before the wrap
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    // PolyBLAMP: integral of PolyBLEP, corrects slope discontinuities (triangle).
    inline float polyBlamp(float t) const noexcept
    {
        const float dt = dt_;
        if(dt <= 0.0f)
            return 0.0f;
        if(t < dt)
        {
            t = t / dt - 1.0f;
            return -0.33333333f * t * t * t;
        }
        if(t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt + 1.0f;
            return 0.33333333f * t * t * t;
        }
        return 0.0f;
    }

    // Band-limited sawtooth in [-1, 1].
    inline float sawSample(float p) const noexcept
    {
        float s = 2.0f * p - 1.0f;
        s -= polyBlep(p);
        return s;
    }

    // Band-limited variable-width pulse: difference of two sawtooths offset by the
    // duty cycle. DC-free at any width; PolyBLEP on both edges.
    inline float pulseSample(float p) const noexcept
    {
        float p2 = p + pw_;
        p2 -= std::floor(p2);
        float s1 = 2.0f * p - 1.0f - polyBlep(p);
        float s2 = 2.0f * p2 - 1.0f - polyBlep(p2);
        // (saw - saw_shifted) is a band-limited pulse that is already DC-free at
        // any width: it alternates between -2*pw and 2-2*pw, whose mean is exactly
        // zero. (Adding a 2*pw-1 "DC correction" would *introduce* offset.)
        return s1 - s2;
    }

    // Direct anti-aliased triangle: an ideal triangle is continuous (no steps)
    // but has slope discontinuities at phase 0 and 0.5. PolyBLAMP — the integral
    // of PolyBLEP — corrects exactly those corners, so the result is *exactly
    // periodic* (no integrator leak, no startup transient, no low-end coloration)
    // and clean to the top of the keyboard. The corner at 0 changes slope by +8
    // (units of output per cycle), the corner at 0.5 by -8.
    inline float triSample(float p) noexcept
    {
        float tri = p < 0.5f ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p); // naive +/-1
        float p2  = p + 0.5f;
        p2 -= std::floor(p2);
        tri += 8.0f * dt_ * polyBlamp(p);  // corner at 0/1
        tri -= 8.0f * dt_ * polyBlamp(p2); // corner at 0.5
        return tri;
    }

    float sr_    = 48000.0f;
    float invSr_ = 1.0f / 48000.0f;
    float dt_    = 0.0f;   // normalised frequency (cycles/sample)
    float phase_ = 0.0f;   // [0,1)
    float morph_ = 0.5f;   // 0 tri .. 0.5 saw .. 1 pulse
    float pw_    = 0.5f;   // pulse duty
};
} // namespace jove

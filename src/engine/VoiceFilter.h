/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "../dub/Svf.h"
#include <cmath>

namespace jove
{
// Cheap, well-behaved odd saturator (~tanh shape) for the ladder feedback. tanh
// per sample per stage per voice is too costly on the M7; this rational approx
// is monotonic, smooth, and clamps to +/-1 so the resonance can self-oscillate
// without railing into hard clipping.
static inline float softSat(float x) noexcept
{
    if(x < -3.0f)
        return -1.0f;
    if(x > 3.0f)
        return 1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Per-voice filter: either a Moog-style 4-pole transistor-ladder lowpass or the
// shared topology-preserving SVF (LP/HP/BP/Notch). Selected per patch.
//
// Ladder: a 4-stage one-pole cascade (Stilson-Smith style) with a soft-saturated
// feedback path. Resonance reaches self-oscillation; an output gain-makeup term
// offsets the passband "bass loss" as resonance rises, and the saturator keeps it
// musical instead of exploding. Runs at the native rate (no oversampling) to save
// CPU on the M7 — the convex one-pole updates plus the ±1 saturator keep it
// numerically bounded across the whole cutoff/resonance range; the trade-off is
// some aliasing of the resonant peak near the top of the range.
class VoiceFilter
{
  public:
    enum class Mode
    {
        LadderLP = 0, // Moog 24 dB/oct lowpass
        SvfLP,
        SvfHP,
        SvfBP,
        SvfNotch,
        Lpg,      // low-pass gate: output level tracks cutoff (vactrol-ish)
        Steiner   // Steiner-Parker-ish: peaky, soft-saturated 2-pole lowpass
    };

    void prepare(float sampleRate) noexcept
    {
        sr_  = sampleRate;
        svf_.prepare(sampleRate);
        reset();
    }

    void reset() noexcept
    {
        z_[0] = z_[1] = z_[2] = z_[3] = 0.0f;
        svf_.reset();
    }

    void setMode(Mode m) noexcept { mode_ = m; }

    // cutoff Hz, res 0..1 (1 = self-oscillation), drive 0..1 pre-filter overdrive.
    void setParams(float cutoffHz, float res, float drive) noexcept
    {
        res_   = res < 0.0f ? 0.0f : (res > 1.0f ? 1.0f : res);
        drive_ = 1.0f + drive * 4.0f; // 1x .. 5x pre-gain

        // Per-mode brightness match. The Moog ladder's effective -3 dB corner sits
        // well below its nominal cutoff at low resonance (measured ~0.44x the dial),
        // rising toward ~1x as the feedback lifts the corner. The other lowpass-family
        // modes read much brighter at the same dial (Steiner ~0.86x, SVF ~0.64x at
        // res 0), so switching models jumps in brightness. Reference = Moog (left
        // untouched so its ~120 presets are unchanged); scale the LP-family modes onto
        // its curve. Taper to ~1x as resonance rises — every mode's resonant peak
        // already converges on the dial frequency up top, so no compensation is needed
        // there. Constants measured against the ladder (filtmeas harness).
        if(mode_ == Mode::Steiner || mode_ == Mode::SvfLP || mode_ == Mode::Lpg)
        {
            const float base = (mode_ == Mode::Steiner) ? 0.52f : 0.69f;
            cutoffHz *= base + (1.0f - base) * std::pow(res_, 0.4f);
        }

        if(cutoffHz < 20.0f)
            cutoffHz = 20.0f;
        const float maxHz = 0.45f * sr_;
        if(cutoffHz > maxHz)
            cutoffHz = maxHz;
        cutoff_ = cutoffHz;

        if(mode_ == Mode::LadderLP)
        {
            // One-pole cascade coefficient. g = 1 - exp(-2*pi*fc/fs) is the
            // matched one-pole pole; the cascade of four plus the feedback gives
            // the 24 dB/oct resonant ladder. Runs at the native rate (1x) to save
            // CPU — the cutoff is clamped below ~0.45*fs so the 1x cascade stays
            // stable even under fast cutoff modulation.
            float gg = 1.0f - std::exp(-2.0f * 3.14159265f * cutoffHz / sr_);
            if(gg > 0.99f)
                gg = 0.99f;
            g_ = gg;
            // feedback amount: ~4 reaches self-oscillation at res=1.
            k_ = 4.0f * res_;
        }
        else
        {
            int t = 0;
            switch(mode_)
            {
                case Mode::SvfHP: t = 1; break;
                case Mode::SvfBP: t = 2; break;
                case Mode::SvfNotch: t = 3; break;
                default: t = 0; break; // SvfLP, Lpg, Steiner -> lowpass
            }
            svfType_ = t;
            // Steiner-Parker: a peakier resonance curve than the clean SVF LP.
            float q = (mode_ == Mode::Steiner) ? (0.2f + res_ * 0.78f) : (res_ * 0.97f);
            svf_.setParams(cutoffHz, q);

            // Low-pass gate: amplitude follows cutoff (darker == quieter), so an
            // envelope/LFO -> cutoff route gives the plucky vactrol "bongo" duck.
            if(mode_ == Mode::Lpg)
            {
                const float maxHz = 0.45f * sr_;
                float norm = std::log2(cutoffHz / 20.0f) / std::log2(maxHz / 20.0f);
                norm = norm < 0.0f ? 0.0f : (norm > 1.0f ? 1.0f : norm);
                gate_ = 0.12f + 0.88f * norm * norm; // never fully shut; quadratic open
            }
            else
                gate_ = 1.0f;
        }
    }

    inline float process(float in) noexcept
    {
        if(mode_ != Mode::LadderLP)
        {
            in *= drive_;
            float out = (svfType_ == 3) ? (in - svf_.process(in, 2)) // notch = in - BP
                                        : svf_.process(in, svfType_);
            if(mode_ == Mode::Steiner)
                out = softSat(out * 1.5f); // diode-ish soft clip for the SP grit
            return out * gate_;            // gate_ is 1.0 except in LPG mode
        }

        // Moog ladder at native rate (1x). Input drive, then the resonant
        // cascade. A gentle output makeup (not an input boost — that would just
        // overdrive the saturator) restores some of the body the feedback removes.
        in *= drive_;
        // resonance feedback, saturated so self-oscillation stays bounded and
        // smooth instead of railing.
        const float u = softSat(in - k_ * z_[3]);
        z_[0] += g_ * (u - z_[0]);
        z_[1] += g_ * (z_[0] - z_[1]);
        z_[2] += g_ * (z_[1] - z_[2]);
        z_[3] += g_ * (z_[2] - z_[3]);
        return z_[3] * (1.0f + 0.5f * k_);
    }

  private:
    float        sr_     = 48000.0f;
    Mode         mode_   = Mode::LadderLP;
    float        cutoff_ = 1000.0f;
    float        res_    = 0.0f;
    float        drive_  = 1.0f;
    float        g_      = 0.1f;
    float        k_      = 0.0f;
    float        gate_   = 1.0f; // LPG amplitude (tracks cutoff); 1.0 otherwise
    double       z_[4]   = {0, 0, 0, 0}; // ladder integrators — double for high-res stability
    int          svfType_ = 0;
    doobie::Svf  svf_;
};
} // namespace jove

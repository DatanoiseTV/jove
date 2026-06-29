/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "Wavetables.h"
#include <cmath>

namespace jove
{
// Single-cycle wavetable oscillator. Linear interpolation within a table and a
// linear morph between adjacent tables (so the WT MORPH knob sweeps the bank).
// The mip level is chosen from the playback frequency so the read is always
// band-limited (no aliasing). Double phase accumulator, matching the rest of the
// double-precision DSP pass.
class WavetableOsc
{
  public:
    void prepare(float sampleRate) noexcept { sr_ = sampleRate; }
    void reset() noexcept { phase_ = 0.0; }
    void setPhase(float p) noexcept { phase_ = p - std::floor(p); }

    // tableF in [0, kNumWt-1], fractional part morphs toward the next table
    void setTable(float tableF) noexcept
    {
        if(tableF < 0.0f) tableF = 0.0f;
        if(tableF > (float) (kNumWt - 1)) tableF = (float) (kNumWt - 1);
        table_ = tableF;
    }

    void setFrequency(float hz) noexcept
    {
        if(hz < 0.0001f) hz = 0.0001f;
        dt_ = hz / sr_;
        // pick the smallest mip whose top harmonic stays below Nyquist
        const float maxSafe = (sr_ * 0.5f) / hz;
        int m = 0;
        while(m < kWtMips - 1 && (float) (256 >> m) > maxSafe) ++m;
        mip_ = m;
    }

    inline float process() noexcept
    {
        const WtBank& bank = wtBank();
        const int w0 = (int) table_;
        const int w1 = (w0 + 1 < kNumWt) ? w0 + 1 : w0;
        const float frac = table_ - (float) w0;

        const float p = (float) phase_ * (float) kWtLen;
        const int i0 = (int) p & (kWtLen - 1);
        const int i1 = (i0 + 1) & (kWtLen - 1);
        const float f = p - std::floor(p);

        const float a = bank.t[w0][mip_][i0] * (1.0f - f) + bank.t[w0][mip_][i1] * f;
        const float b = bank.t[w1][mip_][i0] * (1.0f - f) + bank.t[w1][mip_][i1] * f;
        const float out = a * (1.0f - frac) + b * frac;

        phase_ += dt_;
        if(phase_ >= 1.0) phase_ -= std::floor(phase_);
        return out;
    }

  private:
    float  sr_    = 48000.0f;
    float  dt_    = 0.0f;
    float  table_ = 0.0f;
    double phase_ = 0.0;
    int    mip_   = 0;
};
} // namespace jove

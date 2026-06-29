/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cmath>

namespace jove
{
// Three-band saturator on the filtered voice bus. A complementary one-pole
// crossover splits low / mid / high (the three sum back to the input, so at
// zero drive it reconstructs perfectly), each band gets its own soft-clip
// drive, then they recombine. Band-targeted grit — warm up the lows, bite the
// mids, fizz the highs — without muddying the whole signal. JUCE-free.
class MultibandSat
{
  public:
    void prepare(float sr) noexcept { sr_ = sr; setCrossovers(220.0f, 2400.0f); reset(); }
    void reset() noexcept { lpA_[0] = lpA_[1] = lpB_[0] = lpB_[1] = 0.0f; }

    void setCrossovers(float fLow, float fHigh) noexcept
    {
        aA_ = 1.0f - std::exp(-2.0f * 3.14159265f * fLow / sr_);
        aB_ = 1.0f - std::exp(-2.0f * 3.14159265f * fHigh / sr_);
    }
    void setDrive(float low, float mid, float high) noexcept { dl_ = low; dm_ = mid; dh_ = high; }
    bool active() const noexcept { return dl_ > 0.001f || dm_ > 0.001f || dh_ > 0.001f; }

    inline float process(int ch, float x) noexcept
    {
        lpA_[ch] += aA_ * (x - lpA_[ch]);
        const float low  = lpA_[ch];
        const float rest = x - low;
        lpB_[ch] += aB_ * (rest - lpB_[ch]);
        const float mid  = lpB_[ch];
        const float high = rest - mid;
        return sat(low, dl_) + sat(mid, dm_) + sat(high, dh_);
    }

  private:
    static inline float sat(float x, float d) noexcept
    {
        if(d < 0.001f) return x; // band passes clean
        const float g = 1.0f + d * 5.0f;
        const float y = x * g;
        const float s = y < -3.0f ? -1.0f : (y > 3.0f ? 1.0f : y * (27.0f + y * y) / (27.0f + 9.0f * y * y));
        return s / (1.0f + d * 1.6f); // gentle makeup so band level stays sane
    }

    float sr_ = 48000.0f, aA_ = 0.1f, aB_ = 0.5f;
    float dl_ = 0.0f, dm_ = 0.0f, dh_ = 0.0f;
    float lpA_[2] = {0, 0}, lpB_[2] = {0, 0};
};
} // namespace jove

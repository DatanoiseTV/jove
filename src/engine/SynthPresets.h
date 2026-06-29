/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "SynthParams.h"

namespace jove
{
// Number of factory patches. The bank spans the categories the UI groups by
// (PAD / LEAD / BASS / ARP / STAB / KEYS / PLUCK / FX / DRONE / PERC / AMB / SEQ).
inline constexpr int kNumFactoryPresets = 140;

// Load factory patch `index` into `p`. Out-of-range -> the INIT patch.
void LoadFactoryPreset(int index, SynthPatch& p) noexcept;

// The patch name for `index` without loading the whole patch (for the browser).
const char* FactoryPresetName(int index) noexcept;

// The category index for `index` (for grouped browsing / colour coding).
int FactoryPresetCategory(int index) noexcept;

} // namespace jove

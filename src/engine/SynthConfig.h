/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cstddef>
#include <cstdint>

namespace jove
{
// Maximum simultaneous voices (allocation ceiling). The desktop build runs up to
// 16; the live polyphony the allocator uses is set by the Max Polyphony param and
// can be lower (UNISON spends several voices per note).
inline constexpr int kMaxVoices = 16;

// Per-voice unison ceiling (UNISON mode stacks this many detuned sub-voices on a
// single MIDI note; POLY uses 1). Kept modest so a held chord in UNISON can't
// exceed kMaxVoices * ... — the allocator caps total active sub-voices.
inline constexpr int kMaxUnison = 7;

// Oscillators per voice. Sub + noise are separate sources. Desktop build runs 5
// (osc1-3 carry the FM/sync/ring relationships; osc4-5 are extra detuned layers
// for supersaw width). osc4-5 default OFF so the factory bank is unchanged.
inline constexpr int kNumOsc = 5;

// LFO count (1 per-voice + 2 global by convention; all selectable destinations).
inline constexpr int kNumLfo = 3;

// Envelope count: ENV1 amp (hard-wired to VCA), ENV2 filter, ENV3 aux/free.
inline constexpr int kNumEnv = 3;

// Mod-matrix routable slots shown in the UI.
inline constexpr int kNumModSlots = 10;

// A4 reference for MIDI-note -> Hz.
inline constexpr float kA4Hz   = 440.0f;
inline constexpr int   kA4Note = 69;

} // namespace jove

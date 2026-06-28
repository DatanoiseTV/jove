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
// Maximum simultaneous voices. The STM32H750 at 480 MHz runs the per-voice
// engine (3 BLEP oscs + sub + noise + ladder/SVF filter + 3 envelopes) plus the
// full Doobie FX chain. This is the allocation ceiling; the live polyphony the
// allocator actually uses can be lower (UNISON spends several voices per note).
// Tuned to budget in Phase 10 — start generous, profile, then settle.
inline constexpr int kMaxVoices = 8;

// Per-voice unison ceiling (UNISON mode stacks this many detuned sub-voices on a
// single MIDI note; POLY uses 1). Kept modest so a held chord in UNISON can't
// exceed kMaxVoices * ... — the allocator caps total active sub-voices.
inline constexpr int kMaxUnison = 7;

// Oscillators per voice (osc1/osc2/osc3). Sub + noise are separate sources.
inline constexpr int kNumOsc = 3;

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

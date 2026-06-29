/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cstddef>

// LISTEN/audition demo phrases, one per preset category, so a patch is auditioned
// with material that fits its voice: basslines for BASS, lush swells for PAD,
// staccato stabs for STAB, a held chord for ARP/SEQ (the patch's own arp/seq does
// the work), etc. Notes are in C minor so any patch sounds musical. Each phrase is
// a list of timed note on/off events and a loop length; the processor schedules
// them on the audio thread and loops.
namespace jove
{
struct AuditionEvent
{
    double t;     // seconds from loop start
    bool   on;    // true = note on, false = note off
    int    note;  // MIDI note
    float  vel;   // velocity 0..1 (ignored on note-off)
};

struct AuditionPhrase
{
    const AuditionEvent* ev;
    int                  count;
    double               loop; // seconds
};

namespace audition_detail
{
// 0 PAD — two lush sustained chords, soft swells (Cm add9 -> Abmaj9)
inline constexpr AuditionEvent kPad[] = {
    {0.00, true, 48, 0.55f}, {0.00, true, 55, 0.55f}, {0.00, true, 60, 0.55f}, {0.00, true, 62, 0.55f},
    {2.55, false, 48, 0}, {2.55, false, 55, 0}, {2.55, false, 60, 0}, {2.55, false, 62, 0},
    {2.62, true, 44, 0.55f}, {2.62, true, 51, 0.55f}, {2.62, true, 55, 0.55f}, {2.62, true, 60, 0.55f},
    {4.92, false, 44, 0}, {4.92, false, 51, 0}, {4.92, false, 55, 0}, {4.92, false, 60, 0}};

// 1 LEAD — expressive monophonic riff, mid-high, long final note
inline constexpr AuditionEvent kLead[] = {
    {0.00, true, 67, 0.90f}, {0.27, false, 67, 0}, {0.30, true, 70, 0.90f}, {0.50, false, 70, 0},
    {0.52, true, 72, 0.95f}, {0.85, false, 72, 0}, {0.88, true, 70, 0.88f}, {1.05, false, 70, 0},
    {1.08, true, 67, 0.90f}, {1.45, false, 67, 0}, {1.50, true, 63, 0.85f}, {1.70, false, 63, 0},
    {1.72, true, 65, 0.90f}, {1.95, false, 65, 0}, {2.00, true, 67, 0.95f}, {3.30, false, 67, 0}};

// 2 BASS — low driving groove, root/octave/fifth, eighth+sixteenth feel
inline constexpr AuditionEvent kBass[] = {
    {0.00, true, 36, 1.00f}, {0.20, false, 36, 0}, {0.25, true, 36, 0.78f}, {0.42, false, 36, 0},
    {0.50, true, 48, 0.90f}, {0.68, false, 48, 0}, {0.75, true, 36, 1.00f}, {0.95, false, 36, 0},
    {1.00, true, 43, 0.95f}, {1.18, false, 43, 0}, {1.25, true, 36, 0.88f}, {1.42, false, 36, 0},
    {1.50, true, 39, 0.92f}, {1.68, false, 39, 0}, {1.75, true, 36, 1.00f}, {1.95, false, 36, 0},
    {2.00, true, 48, 0.85f}, {2.18, false, 48, 0}, {2.25, true, 43, 0.90f}, {2.38, false, 43, 0}};

// 3 ARP — hold a Cm7 chord for the loop; the patch's own arp plays the pattern
inline constexpr AuditionEvent kArp[] = {
    {0.00, true, 48, 0.85f}, {0.00, true, 51, 0.85f}, {0.00, true, 55, 0.85f}, {0.00, true, 58, 0.85f},
    {3.90, false, 48, 0}, {3.90, false, 51, 0}, {3.90, false, 55, 0}, {3.90, false, 58, 0}};

// 4 STAB — rhythmic staccato chord stabs (Cm)
inline constexpr AuditionEvent kStab[] = {
    {0.00, true, 48, 0.95f}, {0.00, true, 55, 0.95f}, {0.00, true, 60, 0.95f},
    {0.16, false, 48, 0}, {0.16, false, 55, 0}, {0.16, false, 60, 0},
    {0.50, true, 48, 0.90f}, {0.50, true, 55, 0.90f}, {0.50, true, 60, 0.90f},
    {0.66, false, 48, 0}, {0.66, false, 55, 0}, {0.66, false, 60, 0},
    {1.25, true, 44, 0.95f}, {1.25, true, 51, 0.95f}, {1.25, true, 56, 0.95f},
    {1.41, false, 44, 0}, {1.41, false, 51, 0}, {1.41, false, 56, 0},
    {1.75, true, 48, 0.92f}, {1.75, true, 55, 0.92f}, {1.75, true, 60, 0.92f},
    {1.91, false, 48, 0}, {1.91, false, 55, 0}, {1.91, false, 60, 0}};

// 5 KEYS — comping: a held chord with a little melody over it, twice
inline constexpr AuditionEvent kKeys[] = {
    {0.00, true, 48, 0.70f}, {0.00, true, 51, 0.70f}, {0.00, true, 55, 0.70f},
    {0.00, true, 60, 0.85f}, {0.30, false, 60, 0}, {0.35, true, 63, 0.85f}, {0.62, false, 63, 0},
    {0.68, true, 67, 0.88f}, {1.05, false, 67, 0}, {1.10, true, 63, 0.82f}, {1.45, false, 63, 0},
    {1.65, false, 48, 0}, {1.65, false, 51, 0}, {1.65, false, 55, 0},
    {1.80, true, 44, 0.70f}, {1.80, true, 48, 0.70f}, {1.80, true, 53, 0.70f},
    {1.80, true, 65, 0.85f}, {2.15, false, 65, 0}, {2.20, true, 68, 0.85f}, {2.55, false, 68, 0},
    {2.60, true, 67, 0.88f}, {3.45, false, 67, 0},
    {3.50, false, 44, 0}, {3.50, false, 48, 0}, {3.50, false, 53, 0}};

// 6 PLUCK — ascending then descending staccato plucks
inline constexpr AuditionEvent kPluck[] = {
    {0.00, true, 48, 0.85f}, {0.14, false, 48, 0}, {0.20, true, 55, 0.85f}, {0.34, false, 55, 0},
    {0.40, true, 60, 0.88f}, {0.54, false, 60, 0}, {0.60, true, 63, 0.88f}, {0.74, false, 63, 0},
    {0.80, true, 67, 0.90f}, {0.94, false, 67, 0}, {1.00, true, 72, 0.92f}, {1.14, false, 72, 0},
    {1.30, true, 67, 0.85f}, {1.44, false, 67, 0}, {1.50, true, 63, 0.85f}, {1.64, false, 63, 0},
    {1.70, true, 60, 0.85f}, {1.84, false, 60, 0}, {1.90, true, 55, 0.82f}, {2.04, false, 55, 0},
    {2.10, true, 48, 0.85f}, {2.34, false, 48, 0}};

// 7 FX — a single long evolving note (then a second) to show the texture
inline constexpr AuditionEvent kFx[] = {
    {0.00, true, 55, 0.80f}, {2.00, true, 67, 0.65f}, {4.05, false, 55, 0}, {4.05, false, 67, 0}};

// 8 DRONE — sustained low interval
inline constexpr AuditionEvent kDrone[] = {
    {0.00, true, 36, 0.80f}, {0.00, true, 43, 0.68f}, {4.90, false, 36, 0}, {4.90, false, 43, 0}};

// 9 PERC — rapid rhythmic staccato hits
inline constexpr AuditionEvent kPerc[] = {
    {0.00, true, 48, 1.00f}, {0.08, false, 48, 0}, {0.20, true, 48, 0.80f}, {0.28, false, 48, 0},
    {0.40, true, 55, 0.95f}, {0.48, false, 55, 0}, {0.60, true, 48, 0.85f}, {0.68, false, 48, 0},
    {0.80, true, 51, 0.92f}, {0.88, false, 51, 0}, {1.00, true, 48, 0.80f}, {1.08, false, 48, 0},
    {1.20, true, 55, 0.95f}, {1.28, false, 55, 0}, {1.40, true, 60, 0.90f}, {1.48, false, 60, 0}};

// 10 AMB — sparse, slow, evolving swells (soft)
inline constexpr AuditionEvent kAmb[] = {
    {0.00, true, 60, 0.50f}, {0.00, true, 67, 0.45f}, {2.45, false, 60, 0}, {2.45, false, 67, 0},
    {2.60, true, 63, 0.50f}, {2.60, true, 70, 0.45f}, {5.05, false, 63, 0}, {5.05, false, 70, 0}};

// 11 SEQ — hold a note + octave for the loop; the patch's own sequencer plays
inline constexpr AuditionEvent kSeq[] = {
    {0.00, true, 48, 0.85f}, {0.00, true, 60, 0.78f}, {3.30, false, 48, 0}, {3.30, false, 60, 0}};
} // namespace audition_detail

// Pick the phrase for a category index (matches the UI's CATEGORIES order). Out of
// range -> PAD.
inline AuditionPhrase auditionPhraseFor(int category) noexcept
{
    using namespace audition_detail;
    auto P = [](const AuditionEvent* e, int n, double loop) {
        return AuditionPhrase{e, n, loop};
    };
    switch(category)
    {
        case 1:  return P(kLead,  (int)(sizeof(kLead)  / sizeof(kLead[0])),  3.40);
        case 2:  return P(kBass,  (int)(sizeof(kBass)  / sizeof(kBass[0])),  2.40);
        case 3:  return P(kArp,   (int)(sizeof(kArp)   / sizeof(kArp[0])),   4.00);
        case 4:  return P(kStab,  (int)(sizeof(kStab)  / sizeof(kStab[0])),  2.40);
        case 5:  return P(kKeys,  (int)(sizeof(kKeys)  / sizeof(kKeys[0])),  3.70);
        case 6:  return P(kPluck, (int)(sizeof(kPluck) / sizeof(kPluck[0])), 2.60);
        case 7:  return P(kFx,    (int)(sizeof(kFx)    / sizeof(kFx[0])),    4.20);
        case 8:  return P(kDrone, (int)(sizeof(kDrone) / sizeof(kDrone[0])), 5.00);
        case 9:  return P(kPerc,  (int)(sizeof(kPerc)  / sizeof(kPerc[0])),  1.60);
        case 10: return P(kAmb,   (int)(sizeof(kAmb)   / sizeof(kAmb[0])),   5.20);
        case 11: return P(kSeq,   (int)(sizeof(kSeq)   / sizeof(kSeq[0])),   3.40);
        default: return P(kPad,   (int)(sizeof(kPad)   / sizeof(kPad[0])),   5.00);
    }
}
} // namespace jove

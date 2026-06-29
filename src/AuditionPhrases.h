/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cstddef>

// LISTEN/audition demo phrases. 12 preset categories x 5 varieties, each fitting
// the patch type. EVERY note is on the 16th-note grid and every phrase is a whole
// number of bars (1 or 2) at a 120 BPM reference (1 bar = 2.0 s, a 16th = 0.125 s),
// so timing is tight, notes trigger in sync, and the loop stays locked to the beat
// grid. The processor tempo-scales to the host BPM. Tonal centre C minor.
namespace jove
{
struct AuditionEvent { double t; bool on; int note; float vel; };
struct AuditionPhrase { const AuditionEvent* ev; int count; double loop; };

namespace audition_detail
{
// PAD
inline constexpr AuditionEvent kPad0[] = {
    {0.000, true , 48, 0.55f}, {0.000, true , 51, 0.55f}, {0.000, true , 55, 0.55f}, {0.000, true , 62, 0.55f},
    {2.000, false, 48, 0.0f}, {2.000, false, 51, 0.0f}, {2.000, false, 55, 0.0f}, {2.000, false, 62, 0.0f},
    {2.000, true , 44, 0.55f}, {2.000, true , 48, 0.55f}, {2.000, true , 51, 0.55f}, {2.000, true , 58, 0.55f},
    {4.000, false, 44, 0.0f}, {4.000, false, 48, 0.0f}, {4.000, false, 51, 0.0f}, {4.000, false, 58, 0.0f} };
inline constexpr AuditionEvent kPad1[] = {
    {0.000, true , 48, 0.55f}, {0.000, true , 51, 0.55f}, {0.000, true , 55, 0.55f}, {0.000, true , 58, 0.55f},
    {2.000, false, 48, 0.0f}, {2.000, false, 51, 0.0f}, {2.000, false, 55, 0.0f}, {2.000, false, 58, 0.0f},
    {2.000, true , 53, 0.55f}, {2.000, true , 56, 0.55f}, {2.000, true , 60, 0.55f}, {2.000, true , 63, 0.55f},
    {4.000, false, 53, 0.0f}, {4.000, false, 56, 0.0f}, {4.000, false, 60, 0.0f}, {4.000, false, 63, 0.0f} };
inline constexpr AuditionEvent kPad2[] = {
    {0.000, true , 48, 0.55f}, {0.000, true , 51, 0.55f}, {0.000, true , 55, 0.55f}, {0.000, true , 62, 0.55f},
    {2.000, false, 48, 0.0f}, {2.000, false, 51, 0.0f}, {2.000, false, 55, 0.0f}, {2.000, false, 62, 0.0f},
    {2.000, true , 46, 0.55f}, {2.000, true , 50, 0.55f}, {2.000, true , 53, 0.55f}, {2.000, true , 57, 0.55f},
    {4.000, false, 46, 0.0f}, {4.000, false, 50, 0.0f}, {4.000, false, 53, 0.0f}, {4.000, false, 57, 0.0f} };
inline constexpr AuditionEvent kPad3[] = {
    {0.000, true , 48, 0.55f}, {0.000, true , 51, 0.55f}, {0.000, true , 55, 0.55f}, {0.000, true , 62, 0.55f},
    {2.000, false, 48, 0.0f}, {2.000, false, 51, 0.0f}, {2.000, false, 55, 0.0f}, {2.000, false, 62, 0.0f},
    {2.000, true , 44, 0.55f}, {2.000, true , 48, 0.55f}, {2.000, true , 51, 0.55f}, {2.000, true , 55, 0.55f},
    {4.000, false, 44, 0.0f}, {4.000, false, 48, 0.0f}, {4.000, false, 51, 0.0f}, {4.000, false, 55, 0.0f} };
inline constexpr AuditionEvent kPad4[] = {
    {0.000, true , 48, 0.55f}, {0.000, true , 50, 0.55f}, {0.000, true , 55, 0.55f}, {2.000, false, 48, 0.0f},
    {2.000, false, 50, 0.0f}, {2.000, false, 55, 0.0f}, {2.000, true , 48, 0.55f}, {2.000, true , 51, 0.55f},
    {2.000, true , 55, 0.55f}, {4.000, false, 48, 0.0f}, {4.000, false, 51, 0.0f}, {4.000, false, 55, 0.0f} };
inline constexpr AuditionPhrase kPadV[5] = {
    {kPad0, (int)(sizeof(kPad0)/sizeof(kPad0[0])), 4.00},
    {kPad1, (int)(sizeof(kPad1)/sizeof(kPad1[0])), 4.00},
    {kPad2, (int)(sizeof(kPad2)/sizeof(kPad2[0])), 4.00},
    {kPad3, (int)(sizeof(kPad3)/sizeof(kPad3[0])), 4.00},
    {kPad4, (int)(sizeof(kPad4)/sizeof(kPad4[0])), 4.00},
};

// LEAD
inline constexpr AuditionEvent kLead0[] = {
    {0.000, true , 67, 0.90f}, {0.250, false, 67, 0.0f}, {0.250, true , 70, 0.90f}, {0.500, false, 70, 0.0f},
    {0.500, true , 72, 0.90f}, {0.875, false, 72, 0.0f}, {0.875, true , 70, 0.90f}, {1.000, false, 70, 0.0f},
    {1.000, true , 67, 0.90f}, {1.250, false, 67, 0.0f}, {1.250, true , 63, 0.90f}, {1.500, false, 63, 0.0f},
    {1.500, true , 67, 0.90f}, {2.000, false, 67, 0.0f}, {2.000, true , 72, 0.90f}, {2.250, false, 72, 0.0f},
    {2.250, true , 75, 0.90f}, {2.500, false, 75, 0.0f}, {2.500, true , 79, 0.90f}, {3.000, false, 79, 0.0f},
    {3.000, true , 75, 0.90f}, {3.250, false, 75, 0.0f}, {3.250, true , 72, 0.90f}, {3.500, false, 72, 0.0f},
    {3.500, true , 70, 0.90f}, {4.000, false, 70, 0.0f} };
inline constexpr AuditionEvent kLead1[] = {
    {0.000, true , 72, 0.90f}, {0.125, false, 72, 0.0f}, {0.125, true , 75, 0.90f}, {0.250, false, 75, 0.0f},
    {0.250, true , 70, 0.90f}, {0.375, false, 70, 0.0f}, {0.375, true , 72, 0.90f}, {0.500, false, 72, 0.0f},
    {0.500, true , 67, 0.90f}, {0.750, false, 67, 0.0f}, {0.750, true , 70, 0.90f}, {1.000, false, 70, 0.0f},
    {1.000, true , 72, 0.90f}, {1.125, false, 72, 0.0f}, {1.125, true , 70, 0.90f}, {1.250, false, 70, 0.0f},
    {1.250, true , 67, 0.90f}, {1.500, false, 67, 0.0f}, {1.500, true , 63, 0.90f}, {1.750, false, 63, 0.0f},
    {1.750, true , 67, 0.90f}, {2.000, false, 67, 0.0f}, {2.000, true , 72, 0.90f}, {2.250, false, 72, 0.0f},
    {2.250, true , 75, 0.90f}, {2.500, false, 75, 0.0f}, {2.500, true , 79, 0.90f}, {2.750, false, 79, 0.0f},
    {2.750, true , 75, 0.90f}, {3.000, false, 75, 0.0f}, {3.000, true , 72, 0.90f}, {3.250, false, 72, 0.0f},
    {3.250, true , 70, 0.90f}, {3.500, false, 70, 0.0f}, {3.500, true , 60, 0.90f}, {4.000, false, 60, 0.0f} };
inline constexpr AuditionEvent kLead2[] = {
    {0.000, true , 67, 0.90f}, {0.375, false, 67, 0.0f}, {0.500, true , 70, 0.90f}, {0.875, false, 70, 0.0f},
    {1.000, true , 72, 0.90f}, {1.500, false, 72, 0.0f}, {1.750, true , 66, 0.90f}, {2.000, false, 66, 0.0f},
    {2.000, true , 75, 0.90f}, {2.500, false, 75, 0.0f}, {2.750, true , 72, 0.90f}, {3.000, false, 72, 0.0f},
    {3.000, true , 70, 0.90f}, {3.250, false, 70, 0.0f}, {3.250, true , 67, 0.90f}, {3.500, false, 67, 0.0f},
    {3.500, true , 72, 0.90f}, {4.000, false, 72, 0.0f} };
inline constexpr AuditionEvent kLead3[] = {
    {0.000, true , 60, 0.90f}, {0.125, false, 60, 0.0f}, {0.125, true , 63, 0.90f}, {0.250, false, 63, 0.0f},
    {0.250, true , 67, 0.90f}, {0.375, false, 67, 0.0f}, {0.375, true , 72, 0.90f}, {0.500, false, 72, 0.0f},
    {0.500, true , 70, 0.90f}, {0.875, false, 70, 0.0f}, {1.000, true , 67, 0.90f}, {1.125, false, 67, 0.0f},
    {1.125, true , 72, 0.90f}, {1.250, false, 72, 0.0f}, {1.250, true , 75, 0.90f}, {1.375, false, 75, 0.0f},
    {1.375, true , 79, 0.90f}, {1.500, false, 79, 0.0f}, {1.500, true , 75, 0.90f}, {2.000, false, 75, 0.0f},
    {2.000, true , 72, 0.90f}, {2.125, false, 72, 0.0f}, {2.125, true , 75, 0.90f}, {2.250, false, 75, 0.0f},
    {2.250, true , 79, 0.90f}, {2.375, false, 79, 0.0f}, {2.375, true , 82, 0.90f}, {2.500, false, 82, 0.0f},
    {2.500, true , 79, 0.90f}, {3.000, false, 79, 0.0f}, {3.000, true , 72, 0.90f}, {3.250, false, 72, 0.0f},
    {3.250, true , 75, 0.90f}, {3.500, false, 75, 0.0f}, {3.500, true , 84, 0.90f}, {4.000, false, 84, 0.0f} };
inline constexpr AuditionEvent kLead4[] = {
    {0.000, true , 67, 0.90f}, {0.250, false, 67, 0.0f}, {0.250, true , 70, 0.90f}, {0.500, false, 70, 0.0f},
    {0.500, true , 72, 0.90f}, {1.000, false, 72, 0.0f}, {1.250, true , 68, 0.90f}, {1.500, false, 68, 0.0f},
    {1.500, true , 72, 0.90f}, {1.750, false, 72, 0.0f}, {2.000, true , 75, 0.90f}, {2.500, false, 75, 0.0f},
    {2.750, true , 72, 0.90f}, {3.000, false, 72, 0.0f}, {3.000, true , 75, 0.90f}, {3.250, false, 75, 0.0f},
    {3.250, true , 79, 0.90f}, {3.500, false, 79, 0.0f}, {3.500, true , 72, 0.90f}, {4.000, false, 72, 0.0f} };
inline constexpr AuditionPhrase kLeadV[5] = {
    {kLead0, (int)(sizeof(kLead0)/sizeof(kLead0[0])), 4.00},
    {kLead1, (int)(sizeof(kLead1)/sizeof(kLead1[0])), 4.00},
    {kLead2, (int)(sizeof(kLead2)/sizeof(kLead2[0])), 4.00},
    {kLead3, (int)(sizeof(kLead3)/sizeof(kLead3[0])), 4.00},
    {kLead4, (int)(sizeof(kLead4)/sizeof(kLead4[0])), 4.00},
};

// BASS
inline constexpr AuditionEvent kBass0[] = {
    {0.000, true , 36, 1.00f}, {0.125, false, 36, 0.0f}, {0.250, true , 48, 0.80f}, {0.375, false, 48, 0.0f},
    {0.500, true , 36, 1.00f}, {0.625, false, 36, 0.0f}, {0.750, true , 48, 0.80f}, {0.875, false, 48, 0.0f},
    {1.000, true , 36, 1.00f}, {1.125, false, 36, 0.0f}, {1.250, true , 48, 0.80f}, {1.375, false, 48, 0.0f},
    {1.500, true , 39, 0.95f}, {1.625, false, 39, 0.0f}, {1.750, true , 43, 0.90f}, {1.875, false, 43, 0.0f} };
inline constexpr AuditionEvent kBass1[] = {
    {0.000, true , 36, 1.00f}, {0.250, false, 36, 0.0f}, {0.375, true , 36, 0.55f}, {0.500, false, 36, 0.0f},
    {0.750, true , 36, 0.90f}, {1.000, false, 36, 0.0f}, {1.000, true , 36, 0.55f}, {1.125, false, 36, 0.0f},
    {1.250, true , 39, 0.95f}, {1.500, false, 39, 0.0f}, {1.625, true , 36, 0.55f}, {1.750, false, 36, 0.0f},
    {1.750, true , 34, 0.90f}, {2.000, false, 34, 0.0f} };
inline constexpr AuditionEvent kBass2[] = {
    {0.000, true , 36, 0.95f}, {0.250, false, 36, 0.0f}, {0.250, true , 39, 0.85f}, {0.500, false, 39, 0.0f},
    {0.500, true , 41, 0.90f}, {0.750, false, 41, 0.0f}, {0.750, true , 43, 0.88f}, {1.000, false, 43, 0.0f},
    {1.000, true , 44, 0.90f}, {1.250, false, 44, 0.0f}, {1.250, true , 43, 0.85f}, {1.500, false, 43, 0.0f},
    {1.500, true , 41, 0.90f}, {1.750, false, 41, 0.0f}, {1.750, true , 39, 0.85f}, {2.000, false, 39, 0.0f} };
inline constexpr AuditionEvent kBass3[] = {
    {0.000, true , 36, 1.00f}, {0.125, false, 36, 0.0f}, {0.125, true , 36, 0.70f}, {0.250, false, 36, 0.0f},
    {0.250, true , 48, 0.95f}, {0.375, false, 48, 0.0f}, {0.375, true , 36, 0.80f}, {0.500, false, 36, 0.0f},
    {0.500, true , 39, 0.90f}, {0.750, false, 39, 0.0f}, {0.750, true , 36, 0.80f}, {0.875, false, 36, 0.0f},
    {0.875, true , 36, 0.70f}, {1.000, false, 36, 0.0f}, {1.000, true , 48, 0.95f}, {1.125, false, 48, 0.0f},
    {1.125, true , 36, 0.80f}, {1.250, false, 36, 0.0f}, {1.250, true , 41, 0.90f}, {1.500, false, 41, 0.0f},
    {1.500, true , 36, 0.80f}, {1.625, false, 36, 0.0f}, {1.625, true , 43, 0.90f}, {1.750, false, 43, 0.0f},
    {1.750, true , 36, 0.85f}, {2.000, false, 36, 0.0f} };
inline constexpr AuditionEvent kBass4[] = {
    {0.000, true , 36, 1.00f}, {0.750, false, 36, 0.0f}, {1.000, true , 34, 0.90f}, {1.500, false, 34, 0.0f},
    {1.500, true , 39, 0.95f}, {2.000, false, 39, 0.0f} };
inline constexpr AuditionPhrase kBassV[5] = {
    {kBass0, (int)(sizeof(kBass0)/sizeof(kBass0[0])), 2.00},
    {kBass1, (int)(sizeof(kBass1)/sizeof(kBass1[0])), 2.00},
    {kBass2, (int)(sizeof(kBass2)/sizeof(kBass2[0])), 2.00},
    {kBass3, (int)(sizeof(kBass3)/sizeof(kBass3[0])), 2.00},
    {kBass4, (int)(sizeof(kBass4)/sizeof(kBass4[0])), 2.00},
};

// ARP
inline constexpr AuditionEvent kArp0[] = {
    {0.000, true , 48, 0.85f}, {0.000, true , 51, 0.85f}, {0.000, true , 55, 0.85f}, {4.000, false, 48, 0.0f},
    {4.000, false, 51, 0.0f}, {4.000, false, 55, 0.0f} };
inline constexpr AuditionEvent kArp1[] = {
    {0.000, true , 48, 0.85f}, {0.000, true , 51, 0.85f}, {0.000, true , 55, 0.85f}, {0.000, true , 58, 0.85f},
    {4.000, false, 48, 0.0f}, {4.000, false, 51, 0.0f}, {4.000, false, 55, 0.0f}, {4.000, false, 58, 0.0f} };
inline constexpr AuditionEvent kArp2[] = {
    {0.000, true , 48, 0.85f}, {0.000, true , 51, 0.85f}, {0.000, true , 55, 0.85f}, {2.000, false, 48, 0.0f},
    {2.000, false, 51, 0.0f}, {2.000, false, 55, 0.0f}, {2.000, true , 44, 0.85f}, {2.000, true , 48, 0.85f},
    {2.000, true , 51, 0.85f}, {4.000, false, 44, 0.0f}, {4.000, false, 48, 0.0f}, {4.000, false, 51, 0.0f} };
inline constexpr AuditionEvent kArp3[] = {
    {0.000, true , 48, 0.85f}, {0.000, true , 51, 0.85f}, {0.000, true , 55, 0.85f}, {0.000, true , 62, 0.85f},
    {4.000, false, 48, 0.0f}, {4.000, false, 51, 0.0f}, {4.000, false, 55, 0.0f}, {4.000, false, 62, 0.0f} };
inline constexpr AuditionEvent kArp4[] = {
    {0.000, true , 48, 0.85f}, {0.000, true , 50, 0.85f}, {0.000, true , 55, 0.85f}, {2.000, false, 48, 0.0f},
    {2.000, false, 50, 0.0f}, {2.000, false, 55, 0.0f}, {2.000, true , 48, 0.85f}, {2.000, true , 51, 0.85f},
    {2.000, true , 55, 0.85f}, {4.000, false, 48, 0.0f}, {4.000, false, 51, 0.0f}, {4.000, false, 55, 0.0f} };
inline constexpr AuditionPhrase kArpV[5] = {
    {kArp0, (int)(sizeof(kArp0)/sizeof(kArp0[0])), 4.00},
    {kArp1, (int)(sizeof(kArp1)/sizeof(kArp1[0])), 4.00},
    {kArp2, (int)(sizeof(kArp2)/sizeof(kArp2[0])), 4.00},
    {kArp3, (int)(sizeof(kArp3)/sizeof(kArp3[0])), 4.00},
    {kArp4, (int)(sizeof(kArp4)/sizeof(kArp4[0])), 4.00},
};

// STAB
inline constexpr AuditionEvent kStab0[] = {
    {0.250, true , 60, 0.95f}, {0.250, true , 63, 0.95f}, {0.250, true , 67, 0.95f}, {0.375, false, 60, 0.0f},
    {0.375, false, 63, 0.0f}, {0.375, false, 67, 0.0f}, {0.750, true , 60, 0.95f}, {0.750, true , 63, 0.95f},
    {0.750, true , 67, 0.95f}, {0.875, false, 60, 0.0f}, {0.875, false, 63, 0.0f}, {0.875, false, 67, 0.0f},
    {1.250, true , 60, 0.95f}, {1.250, true , 63, 0.95f}, {1.250, true , 67, 0.95f}, {1.375, false, 60, 0.0f},
    {1.375, false, 63, 0.0f}, {1.375, false, 67, 0.0f}, {1.750, true , 60, 0.95f}, {1.750, true , 63, 0.95f},
    {1.750, true , 67, 0.95f}, {1.875, false, 60, 0.0f}, {1.875, false, 63, 0.0f}, {1.875, false, 67, 0.0f} };
inline constexpr AuditionEvent kStab1[] = {
    {0.000, true , 48, 0.95f}, {0.000, true , 51, 0.95f}, {0.000, true , 55, 0.95f}, {0.000, true , 63, 0.95f},
    {0.125, false, 48, 0.0f}, {0.125, false, 51, 0.0f}, {0.125, false, 55, 0.0f}, {0.125, false, 63, 0.0f},
    {0.500, true , 48, 0.95f}, {0.500, true , 51, 0.95f}, {0.500, true , 55, 0.95f}, {0.500, true , 63, 0.95f},
    {0.625, false, 48, 0.0f}, {0.625, false, 51, 0.0f}, {0.625, false, 55, 0.0f}, {0.625, false, 63, 0.0f},
    {1.000, true , 48, 0.95f}, {1.000, true , 51, 0.95f}, {1.000, true , 55, 0.95f}, {1.000, true , 63, 0.95f},
    {1.125, false, 48, 0.0f}, {1.125, false, 51, 0.0f}, {1.125, false, 55, 0.0f}, {1.125, false, 63, 0.0f},
    {1.500, true , 48, 0.95f}, {1.500, true , 51, 0.95f}, {1.500, true , 55, 0.95f}, {1.500, true , 63, 0.95f},
    {1.625, false, 48, 0.0f}, {1.625, false, 51, 0.0f}, {1.625, false, 55, 0.0f}, {1.625, false, 63, 0.0f} };
inline constexpr AuditionEvent kStab2[] = {
    {0.250, true , 63, 0.85f}, {0.250, true , 67, 0.85f}, {0.250, true , 70, 0.85f}, {0.375, false, 63, 0.0f},
    {0.375, false, 67, 0.0f}, {0.375, false, 70, 0.0f}, {0.375, true , 63, 0.85f}, {0.375, true , 67, 0.85f},
    {0.375, true , 70, 0.85f}, {0.500, false, 63, 0.0f}, {0.500, false, 67, 0.0f}, {0.500, false, 70, 0.0f},
    {0.750, true , 63, 0.85f}, {0.750, true , 67, 0.85f}, {0.750, true , 70, 0.85f}, {0.875, false, 63, 0.0f},
    {0.875, false, 67, 0.0f}, {0.875, false, 70, 0.0f}, {0.875, true , 63, 0.85f}, {0.875, true , 67, 0.85f},
    {0.875, true , 70, 0.85f}, {1.000, false, 63, 0.0f}, {1.000, false, 67, 0.0f}, {1.000, false, 70, 0.0f},
    {1.250, true , 63, 0.85f}, {1.250, true , 67, 0.85f}, {1.250, true , 70, 0.85f}, {1.375, false, 63, 0.0f},
    {1.375, false, 67, 0.0f}, {1.375, false, 70, 0.0f}, {1.375, true , 63, 0.85f}, {1.375, true , 67, 0.85f},
    {1.375, true , 70, 0.85f}, {1.500, false, 63, 0.0f}, {1.500, false, 67, 0.0f}, {1.500, false, 70, 0.0f},
    {1.750, true , 63, 0.85f}, {1.750, true , 67, 0.85f}, {1.750, true , 70, 0.85f}, {1.875, false, 63, 0.0f},
    {1.875, false, 67, 0.0f}, {1.875, false, 70, 0.0f}, {1.875, true , 63, 0.85f}, {1.875, true , 67, 0.85f},
    {1.875, true , 70, 0.85f}, {2.000, false, 63, 0.0f}, {2.000, false, 67, 0.0f}, {2.000, false, 70, 0.0f} };
inline constexpr AuditionEvent kStab3[] = {
    {0.000, true , 56, 0.95f}, {0.000, true , 60, 0.95f}, {0.000, true , 63, 0.95f}, {0.125, false, 56, 0.0f},
    {0.125, false, 60, 0.0f}, {0.125, false, 63, 0.0f}, {0.375, true , 56, 0.95f}, {0.375, true , 60, 0.95f},
    {0.375, true , 63, 0.95f}, {0.500, false, 56, 0.0f}, {0.500, false, 60, 0.0f}, {0.500, false, 63, 0.0f},
    {0.750, true , 56, 0.95f}, {0.750, true , 60, 0.95f}, {0.750, true , 63, 0.95f}, {0.875, false, 56, 0.0f},
    {0.875, false, 60, 0.0f}, {0.875, false, 63, 0.0f}, {1.000, true , 56, 0.95f}, {1.000, true , 60, 0.95f},
    {1.000, true , 63, 0.95f}, {1.125, false, 56, 0.0f}, {1.125, false, 60, 0.0f}, {1.125, false, 63, 0.0f},
    {1.375, true , 56, 0.95f}, {1.375, true , 60, 0.95f}, {1.375, true , 63, 0.95f}, {1.500, false, 56, 0.0f},
    {1.500, false, 60, 0.0f}, {1.500, false, 63, 0.0f}, {1.750, true , 56, 0.95f}, {1.750, true , 60, 0.95f},
    {1.750, true , 63, 0.95f}, {1.875, false, 56, 0.0f}, {1.875, false, 60, 0.0f}, {1.875, false, 63, 0.0f} };
inline constexpr AuditionEvent kStab4[] = {
    {0.000, true , 60, 0.95f}, {0.000, true , 63, 0.95f}, {0.000, true , 67, 0.95f}, {0.125, false, 60, 0.0f},
    {0.125, false, 63, 0.0f}, {0.125, false, 67, 0.0f}, {0.500, true , 60, 0.95f}, {0.500, true , 63, 0.95f},
    {0.500, true , 67, 0.95f}, {0.625, false, 60, 0.0f}, {0.625, false, 63, 0.0f}, {0.625, false, 67, 0.0f},
    {1.000, true , 65, 0.92f}, {1.000, true , 68, 0.92f}, {1.000, true , 72, 0.92f}, {1.125, false, 65, 0.0f},
    {1.125, false, 68, 0.0f}, {1.125, false, 72, 0.0f}, {1.500, true , 65, 0.92f}, {1.500, true , 68, 0.92f},
    {1.500, true , 72, 0.92f}, {1.625, false, 65, 0.0f}, {1.625, false, 68, 0.0f}, {1.625, false, 72, 0.0f} };
inline constexpr AuditionPhrase kStabV[5] = {
    {kStab0, (int)(sizeof(kStab0)/sizeof(kStab0[0])), 2.00},
    {kStab1, (int)(sizeof(kStab1)/sizeof(kStab1[0])), 2.00},
    {kStab2, (int)(sizeof(kStab2)/sizeof(kStab2[0])), 2.00},
    {kStab3, (int)(sizeof(kStab3)/sizeof(kStab3[0])), 2.00},
    {kStab4, (int)(sizeof(kStab4)/sizeof(kStab4[0])), 2.00},
};

// KEYS
inline constexpr AuditionEvent kKeys0[] = {
    {0.000, true , 48, 0.68f}, {0.000, true , 51, 0.68f}, {0.000, true , 55, 0.68f}, {0.000, true , 67, 0.85f},
    {0.375, false, 67, 0.0f}, {0.500, true , 70, 0.85f}, {1.000, false, 48, 0.0f}, {1.000, false, 51, 0.0f},
    {1.000, false, 55, 0.0f}, {1.000, false, 70, 0.0f}, {1.000, true , 44, 0.68f}, {1.000, true , 48, 0.68f},
    {1.000, true , 51, 0.68f}, {1.000, true , 72, 0.85f}, {1.375, false, 72, 0.0f}, {1.500, true , 70, 0.85f},
    {2.000, false, 44, 0.0f}, {2.000, false, 48, 0.0f}, {2.000, false, 51, 0.0f}, {2.000, false, 70, 0.0f},
    {2.000, true , 46, 0.68f}, {2.000, true , 50, 0.68f}, {2.000, true , 53, 0.68f}, {2.000, true , 67, 0.85f},
    {2.375, false, 67, 0.0f}, {2.500, true , 70, 0.85f}, {3.000, false, 46, 0.0f}, {3.000, false, 50, 0.0f},
    {3.000, false, 53, 0.0f}, {3.000, false, 70, 0.0f}, {3.000, true , 43, 0.68f}, {3.000, true , 47, 0.68f},
    {3.000, true , 50, 0.68f}, {3.000, true , 75, 0.85f}, {3.500, false, 75, 0.0f}, {3.500, true , 72, 0.85f},
    {4.000, false, 43, 0.0f}, {4.000, false, 47, 0.0f}, {4.000, false, 50, 0.0f}, {4.000, false, 72, 0.0f} };
inline constexpr AuditionEvent kKeys1[] = {
    {0.000, true , 48, 0.68f}, {0.000, true , 51, 0.68f}, {0.000, true , 55, 0.68f}, {0.000, true , 58, 0.68f},
    {0.000, true , 67, 0.85f}, {0.250, false, 67, 0.0f}, {0.250, true , 70, 0.85f}, {0.500, false, 70, 0.0f},
    {0.500, true , 72, 0.85f}, {1.000, false, 48, 0.0f}, {1.000, false, 51, 0.0f}, {1.000, false, 55, 0.0f},
    {1.000, false, 58, 0.0f}, {1.000, false, 72, 0.0f}, {1.000, true , 53, 0.68f}, {1.000, true , 56, 0.68f},
    {1.000, true , 60, 0.68f}, {1.000, true , 63, 0.68f}, {1.000, true , 70, 0.85f}, {1.250, false, 70, 0.0f},
    {1.250, true , 72, 0.85f}, {1.500, false, 72, 0.0f}, {1.500, true , 75, 0.85f}, {2.000, false, 53, 0.0f},
    {2.000, false, 56, 0.0f}, {2.000, false, 60, 0.0f}, {2.000, false, 63, 0.0f}, {2.000, false, 75, 0.0f},
    {2.000, true , 46, 0.68f}, {2.000, true , 50, 0.68f}, {2.000, true , 53, 0.68f}, {2.000, true , 57, 0.68f},
    {2.000, true , 74, 0.85f}, {2.500, false, 74, 0.0f}, {2.500, true , 72, 0.85f}, {3.000, false, 46, 0.0f},
    {3.000, false, 50, 0.0f}, {3.000, false, 53, 0.0f}, {3.000, false, 57, 0.0f}, {3.000, false, 72, 0.0f},
    {3.000, true , 43, 0.68f}, {3.000, true , 47, 0.68f}, {3.000, true , 50, 0.68f}, {3.000, true , 53, 0.68f},
    {3.000, true , 70, 0.85f}, {3.500, false, 70, 0.0f}, {3.500, true , 67, 0.85f}, {4.000, false, 43, 0.0f},
    {4.000, false, 47, 0.0f}, {4.000, false, 50, 0.0f}, {4.000, false, 53, 0.0f}, {4.000, false, 67, 0.0f} };
inline constexpr AuditionEvent kKeys2[] = {
    {0.000, true , 50, 0.68f}, {0.000, true , 53, 0.68f}, {0.000, true , 56, 0.68f}, {0.000, true , 60, 0.68f},
    {0.000, true , 65, 0.85f}, {0.500, false, 65, 0.0f}, {0.500, true , 68, 0.85f}, {1.000, false, 50, 0.0f},
    {1.000, false, 53, 0.0f}, {1.000, false, 56, 0.0f}, {1.000, false, 60, 0.0f}, {1.000, false, 68, 0.0f},
    {1.000, true , 43, 0.68f}, {1.000, true , 47, 0.68f}, {1.000, true , 50, 0.68f}, {1.000, true , 53, 0.68f},
    {1.000, true , 67, 0.85f}, {1.500, false, 67, 0.0f}, {1.500, true , 65, 0.85f}, {2.000, false, 43, 0.0f},
    {2.000, false, 47, 0.0f}, {2.000, false, 50, 0.0f}, {2.000, false, 53, 0.0f}, {2.000, false, 65, 0.0f},
    {2.000, true , 48, 0.68f}, {2.000, true , 51, 0.68f}, {2.000, true , 55, 0.68f}, {2.000, true , 62, 0.68f},
    {2.000, true , 72, 0.85f}, {2.375, false, 72, 0.0f}, {2.500, true , 70, 0.85f}, {2.875, false, 70, 0.0f},
    {3.000, true , 67, 0.85f}, {3.500, false, 67, 0.0f}, {3.500, true , 72, 0.85f}, {4.000, false, 48, 0.0f},
    {4.000, false, 51, 0.0f}, {4.000, false, 55, 0.0f}, {4.000, false, 62, 0.0f}, {4.000, false, 72, 0.0f} };
inline constexpr AuditionEvent kKeys3[] = {
    {0.000, true , 48, 0.68f}, {0.000, true , 51, 0.68f}, {0.000, true , 55, 0.68f}, {0.000, true , 62, 0.68f},
    {0.000, true , 67, 0.85f}, {0.250, false, 67, 0.0f}, {0.250, true , 72, 0.85f}, {0.500, false, 72, 0.0f},
    {0.500, true , 75, 0.85f}, {0.750, false, 75, 0.0f}, {0.750, true , 72, 0.85f}, {1.000, false, 72, 0.0f},
    {1.000, true , 70, 0.85f}, {1.500, false, 70, 0.0f}, {1.500, true , 67, 0.85f}, {2.000, false, 48, 0.0f},
    {2.000, false, 51, 0.0f}, {2.000, false, 55, 0.0f}, {2.000, false, 62, 0.0f}, {2.000, false, 67, 0.0f},
    {2.000, true , 44, 0.68f}, {2.000, true , 48, 0.68f}, {2.000, true , 51, 0.68f}, {2.000, true , 58, 0.68f},
    {2.000, true , 68, 0.85f}, {2.250, false, 68, 0.0f}, {2.250, true , 72, 0.85f}, {2.500, false, 72, 0.0f},
    {2.500, true , 75, 0.85f}, {2.750, false, 75, 0.0f}, {2.750, true , 79, 0.85f}, {3.000, false, 79, 0.0f},
    {3.000, true , 75, 0.85f}, {3.500, false, 75, 0.0f}, {3.500, true , 72, 0.85f}, {4.000, false, 44, 0.0f},
    {4.000, false, 48, 0.0f}, {4.000, false, 51, 0.0f}, {4.000, false, 58, 0.0f}, {4.000, false, 72, 0.0f} };
inline constexpr AuditionEvent kKeys4[] = {
    {0.000, true , 48, 0.68f}, {0.000, true , 51, 0.68f}, {0.000, true , 55, 0.68f}, {0.000, true , 67, 0.85f},
    {0.125, false, 67, 0.0f}, {0.250, true , 70, 0.85f}, {0.375, false, 70, 0.0f}, {0.500, true , 72, 0.85f},
    {0.750, false, 72, 0.0f}, {1.000, false, 48, 0.0f}, {1.000, false, 51, 0.0f}, {1.000, false, 55, 0.0f},
    {1.000, true , 46, 0.68f}, {1.000, true , 50, 0.68f}, {1.000, true , 53, 0.68f}, {1.000, true , 70, 0.85f},
    {1.125, false, 70, 0.0f}, {1.250, true , 67, 0.85f}, {1.375, false, 67, 0.0f}, {1.500, true , 63, 0.85f},
    {1.750, false, 63, 0.0f}, {2.000, false, 46, 0.0f}, {2.000, false, 50, 0.0f}, {2.000, false, 53, 0.0f},
    {2.000, true , 53, 0.68f}, {2.000, true , 56, 0.68f}, {2.000, true , 60, 0.68f}, {2.000, true , 72, 0.85f},
    {2.125, false, 72, 0.0f}, {2.250, true , 75, 0.85f}, {2.375, false, 75, 0.0f}, {2.500, true , 79, 0.85f},
    {2.750, false, 79, 0.0f}, {3.000, false, 53, 0.0f}, {3.000, false, 56, 0.0f}, {3.000, false, 60, 0.0f},
    {3.000, true , 43, 0.68f}, {3.000, true , 47, 0.68f}, {3.000, true , 50, 0.68f}, {3.000, true , 75, 0.85f},
    {3.125, false, 75, 0.0f}, {3.250, true , 72, 0.85f}, {3.375, false, 72, 0.0f}, {3.500, true , 70, 0.85f},
    {3.750, false, 70, 0.0f}, {4.000, false, 43, 0.0f}, {4.000, false, 47, 0.0f}, {4.000, false, 50, 0.0f} };
inline constexpr AuditionPhrase kKeysV[5] = {
    {kKeys0, (int)(sizeof(kKeys0)/sizeof(kKeys0[0])), 4.00},
    {kKeys1, (int)(sizeof(kKeys1)/sizeof(kKeys1[0])), 4.00},
    {kKeys2, (int)(sizeof(kKeys2)/sizeof(kKeys2[0])), 4.00},
    {kKeys3, (int)(sizeof(kKeys3)/sizeof(kKeys3[0])), 4.00},
    {kKeys4, (int)(sizeof(kKeys4)/sizeof(kKeys4[0])), 4.00},
};

// PLUCK
inline constexpr AuditionEvent kPluck0[] = {
    {0.000, true , 48, 0.85f}, {0.125, false, 48, 0.0f}, {0.250, true , 55, 0.85f}, {0.375, false, 55, 0.0f},
    {0.500, true , 60, 0.85f}, {0.625, false, 60, 0.0f}, {0.750, true , 63, 0.85f}, {0.875, false, 63, 0.0f},
    {1.000, true , 67, 0.85f}, {1.125, false, 67, 0.0f}, {1.250, true , 72, 0.85f}, {1.375, false, 72, 0.0f},
    {1.500, true , 67, 0.85f}, {1.625, false, 67, 0.0f}, {1.750, true , 63, 0.85f}, {1.875, false, 63, 0.0f} };
inline constexpr AuditionEvent kPluck1[] = {
    {0.000, true , 67, 0.85f}, {0.125, false, 67, 0.0f}, {0.375, true , 70, 0.85f}, {0.500, false, 70, 0.0f},
    {0.500, true , 67, 0.85f}, {0.625, false, 67, 0.0f}, {0.750, true , 75, 0.85f}, {0.875, false, 75, 0.0f},
    {1.125, true , 72, 0.85f}, {1.250, false, 72, 0.0f}, {1.250, true , 70, 0.85f}, {1.375, false, 70, 0.0f},
    {1.500, true , 67, 0.85f}, {1.625, false, 67, 0.0f}, {1.875, true , 63, 0.85f}, {2.000, false, 63, 0.0f} };
inline constexpr AuditionEvent kPluck2[] = {
    {0.000, true , 60, 0.85f}, {0.125, false, 60, 0.0f}, {0.125, true , 63, 0.85f}, {0.250, false, 63, 0.0f},
    {0.250, true , 65, 0.85f}, {0.375, false, 65, 0.0f}, {0.375, true , 67, 0.85f}, {0.500, false, 67, 0.0f},
    {0.500, true , 70, 0.85f}, {0.625, false, 70, 0.0f}, {0.625, true , 72, 0.85f}, {0.750, false, 72, 0.0f},
    {0.750, true , 75, 0.85f}, {0.875, false, 75, 0.0f}, {0.875, true , 79, 0.85f}, {1.000, false, 79, 0.0f},
    {1.000, true , 77, 0.85f}, {1.125, false, 77, 0.0f}, {1.125, true , 75, 0.85f}, {1.250, false, 75, 0.0f},
    {1.250, true , 72, 0.85f}, {1.375, false, 72, 0.0f}, {1.375, true , 70, 0.85f}, {1.500, false, 70, 0.0f},
    {1.500, true , 67, 0.85f}, {1.625, false, 67, 0.0f}, {1.625, true , 65, 0.85f}, {1.750, false, 65, 0.0f},
    {1.750, true , 63, 0.85f}, {1.875, false, 63, 0.0f}, {1.875, true , 60, 0.85f}, {2.000, false, 60, 0.0f} };
inline constexpr AuditionEvent kPluck3[] = {
    {0.000, true , 67, 0.85f}, {0.125, false, 67, 0.0f}, {0.250, true , 70, 0.85f}, {0.375, false, 70, 0.0f},
    {0.500, true , 75, 0.85f}, {0.625, false, 75, 0.0f}, {1.000, true , 72, 0.85f}, {1.125, false, 72, 0.0f},
    {1.250, true , 70, 0.85f}, {1.375, false, 70, 0.0f}, {1.500, true , 67, 0.85f}, {1.625, false, 67, 0.0f},
    {1.750, true , 60, 0.85f}, {1.875, false, 60, 0.0f} };
inline constexpr AuditionEvent kPluck4[] = {
    {0.000, true , 48, 0.85f}, {0.125, false, 48, 0.0f}, {0.250, true , 72, 0.85f}, {0.375, false, 72, 0.0f},
    {0.500, true , 55, 0.85f}, {0.625, false, 55, 0.0f}, {0.750, true , 75, 0.85f}, {0.875, false, 75, 0.0f},
    {1.000, true , 60, 0.85f}, {1.125, false, 60, 0.0f}, {1.250, true , 79, 0.85f}, {1.375, false, 79, 0.0f},
    {1.500, true , 55, 0.85f}, {1.625, false, 55, 0.0f}, {1.750, true , 72, 0.85f}, {1.875, false, 72, 0.0f} };
inline constexpr AuditionPhrase kPluckV[5] = {
    {kPluck0, (int)(sizeof(kPluck0)/sizeof(kPluck0[0])), 2.00},
    {kPluck1, (int)(sizeof(kPluck1)/sizeof(kPluck1[0])), 2.00},
    {kPluck2, (int)(sizeof(kPluck2)/sizeof(kPluck2[0])), 2.00},
    {kPluck3, (int)(sizeof(kPluck3)/sizeof(kPluck3[0])), 2.00},
    {kPluck4, (int)(sizeof(kPluck4)/sizeof(kPluck4[0])), 2.00},
};

// FX
inline constexpr AuditionEvent kFx0[] = {
    {0.000, true , 57, 0.62f}, {4.000, false, 57, 0.0f} };
inline constexpr AuditionEvent kFx1[] = {
    {0.000, true , 38, 0.70f}, {0.125, true , 74, 0.58f}, {4.000, false, 38, 0.0f}, {4.000, false, 74, 0.0f} };
inline constexpr AuditionEvent kFx2[] = {
    {0.000, true , 48, 0.55f}, {0.750, true , 55, 0.60f}, {1.750, true , 63, 0.66f}, {4.000, false, 48, 0.0f},
    {4.000, false, 55, 0.0f}, {4.000, false, 63, 0.0f} };
inline constexpr AuditionEvent kFx3[] = {
    {0.000, true , 40, 0.80f}, {0.500, false, 40, 0.0f}, {1.500, true , 52, 0.74f}, {2.000, false, 52, 0.0f},
    {3.000, true , 45, 0.85f}, {3.625, false, 45, 0.0f} };
inline constexpr AuditionEvent kFx4[] = {
    {0.000, true , 60, 0.62f}, {0.000, true , 62, 0.58f}, {0.125, true , 64, 0.55f}, {4.000, false, 60, 0.0f},
    {4.000, false, 62, 0.0f}, {4.000, false, 64, 0.0f} };
inline constexpr AuditionPhrase kFxV[5] = {
    {kFx0, (int)(sizeof(kFx0)/sizeof(kFx0[0])), 4.00},
    {kFx1, (int)(sizeof(kFx1)/sizeof(kFx1[0])), 4.00},
    {kFx2, (int)(sizeof(kFx2)/sizeof(kFx2[0])), 4.00},
    {kFx3, (int)(sizeof(kFx3)/sizeof(kFx3[0])), 4.00},
    {kFx4, (int)(sizeof(kFx4)/sizeof(kFx4[0])), 4.00},
};

// DRONE
inline constexpr AuditionEvent kDrone0[] = {
    {0.000, true , 24, 0.78f}, {0.000, true , 36, 0.70f}, {4.000, false, 24, 0.0f}, {4.000, false, 36, 0.0f} };
inline constexpr AuditionEvent kDrone1[] = {
    {0.000, true , 24, 0.80f}, {0.000, true , 31, 0.70f}, {4.000, false, 24, 0.0f}, {4.000, false, 31, 0.0f} };
inline constexpr AuditionEvent kDrone2[] = {
    {0.000, true , 24, 0.80f}, {0.000, true , 36, 0.72f}, {0.000, true , 43, 0.66f}, {4.000, false, 24, 0.0f},
    {4.000, false, 36, 0.0f}, {4.000, false, 43, 0.0f} };
inline constexpr AuditionEvent kDrone3[] = {
    {0.000, true , 26, 0.82f}, {1.750, true , 33, 0.66f}, {4.000, false, 26, 0.0f}, {4.000, false, 33, 0.0f} };
inline constexpr AuditionEvent kDrone4[] = {
    {0.000, true , 24, 0.80f}, {0.000, true , 48, 0.62f}, {2.000, false, 48, 0.0f}, {2.000, true , 51, 0.64f},
    {4.000, false, 24, 0.0f}, {4.000, false, 51, 0.0f} };
inline constexpr AuditionPhrase kDroneV[5] = {
    {kDrone0, (int)(sizeof(kDrone0)/sizeof(kDrone0[0])), 4.00},
    {kDrone1, (int)(sizeof(kDrone1)/sizeof(kDrone1[0])), 4.00},
    {kDrone2, (int)(sizeof(kDrone2)/sizeof(kDrone2[0])), 4.00},
    {kDrone3, (int)(sizeof(kDrone3)/sizeof(kDrone3[0])), 4.00},
    {kDrone4, (int)(sizeof(kDrone4)/sizeof(kDrone4[0])), 4.00},
};

// PERC
inline constexpr AuditionEvent kPerc0[] = {
    {0.000, true , 36, 1.00f}, {0.125, false, 36, 0.0f}, {0.250, true , 50, 0.60f}, {0.375, false, 50, 0.0f},
    {0.500, true , 50, 0.80f}, {0.625, false, 50, 0.0f}, {0.750, true , 52, 0.60f}, {0.875, false, 52, 0.0f},
    {1.000, true , 36, 0.90f}, {1.125, false, 36, 0.0f}, {1.250, true , 50, 0.60f}, {1.375, false, 50, 0.0f},
    {1.500, true , 52, 0.85f}, {1.625, false, 52, 0.0f}, {1.750, true , 52, 0.60f}, {1.875, false, 52, 0.0f} };
inline constexpr AuditionEvent kPerc1[] = {
    {0.000, true , 36, 1.00f}, {0.125, false, 36, 0.0f}, {0.375, true , 42, 0.70f}, {0.500, false, 42, 0.0f},
    {0.750, true , 38, 0.95f}, {0.875, false, 38, 0.0f}, {1.000, true , 36, 0.80f}, {1.125, false, 36, 0.0f},
    {1.250, true , 38, 0.90f}, {1.375, false, 38, 0.0f}, {1.375, true , 42, 0.70f}, {1.500, false, 42, 0.0f},
    {1.750, true , 38, 0.85f}, {1.875, false, 38, 0.0f} };
inline constexpr AuditionEvent kPerc2[] = {
    {0.000, true , 60, 0.95f}, {0.125, false, 60, 0.0f}, {0.250, true , 57, 0.85f}, {0.375, false, 57, 0.0f},
    {0.500, true , 53, 0.80f}, {0.625, false, 53, 0.0f}, {0.750, true , 50, 0.90f}, {0.875, false, 50, 0.0f},
    {1.000, true , 48, 1.00f}, {1.125, false, 48, 0.0f}, {1.250, true , 53, 0.80f}, {1.375, false, 53, 0.0f},
    {1.500, true , 50, 0.85f}, {1.625, false, 50, 0.0f}, {1.750, true , 48, 0.90f}, {1.875, false, 48, 0.0f} };
inline constexpr AuditionEvent kPerc3[] = {
    {0.000, true , 42, 1.00f}, {0.125, false, 42, 0.0f}, {0.125, true , 42, 0.70f}, {0.250, false, 42, 0.0f},
    {0.250, true , 42, 0.70f}, {0.375, false, 42, 0.0f}, {0.375, true , 42, 0.70f}, {0.500, false, 42, 0.0f},
    {0.500, true , 42, 1.00f}, {0.625, false, 42, 0.0f}, {0.625, true , 42, 0.70f}, {0.750, false, 42, 0.0f},
    {0.750, true , 42, 0.70f}, {0.875, false, 42, 0.0f}, {0.875, true , 42, 0.70f}, {1.000, false, 42, 0.0f},
    {1.000, true , 46, 1.00f}, {1.125, false, 46, 0.0f}, {1.125, true , 42, 0.70f}, {1.250, false, 42, 0.0f},
    {1.250, true , 42, 0.70f}, {1.375, false, 42, 0.0f}, {1.375, true , 42, 0.70f}, {1.500, false, 42, 0.0f},
    {1.500, true , 42, 1.00f}, {1.625, false, 42, 0.0f}, {1.625, true , 42, 0.70f}, {1.750, false, 42, 0.0f},
    {1.750, true , 42, 0.70f}, {1.875, false, 42, 0.0f}, {1.875, true , 42, 0.70f}, {2.000, false, 42, 0.0f} };
inline constexpr AuditionEvent kPerc4[] = {
    {0.000, true , 48, 0.90f}, {0.125, false, 48, 0.0f}, {0.250, true , 60, 0.80f}, {0.375, false, 60, 0.0f},
    {0.500, true , 55, 0.85f}, {0.625, false, 55, 0.0f}, {0.750, true , 60, 0.70f}, {0.875, false, 60, 0.0f},
    {1.000, true , 48, 0.95f}, {1.125, false, 48, 0.0f}, {1.250, true , 60, 0.80f}, {1.375, false, 60, 0.0f},
    {1.500, true , 55, 0.85f}, {1.625, false, 55, 0.0f}, {1.750, true , 60, 0.90f}, {1.875, false, 60, 0.0f} };
inline constexpr AuditionPhrase kPercV[5] = {
    {kPerc0, (int)(sizeof(kPerc0)/sizeof(kPerc0[0])), 2.00},
    {kPerc1, (int)(sizeof(kPerc1)/sizeof(kPerc1[0])), 2.00},
    {kPerc2, (int)(sizeof(kPerc2)/sizeof(kPerc2[0])), 2.00},
    {kPerc3, (int)(sizeof(kPerc3)/sizeof(kPerc3[0])), 2.00},
    {kPerc4, (int)(sizeof(kPerc4)/sizeof(kPerc4[0])), 2.00},
};

// AMB
inline constexpr AuditionEvent kAmb0[] = {
    {0.000, true , 72, 0.45f}, {2.000, false, 72, 0.0f}, {2.250, true , 75, 0.50f}, {4.000, false, 75, 0.0f} };
inline constexpr AuditionEvent kAmb1[] = {
    {0.250, true , 60, 0.42f}, {0.500, true , 79, 0.48f}, {4.000, false, 60, 0.0f}, {4.000, false, 79, 0.0f} };
inline constexpr AuditionEvent kAmb2[] = {
    {0.000, true , 60, 0.44f}, {1.250, true , 67, 0.50f}, {2.000, false, 60, 0.0f}, {2.500, true , 75, 0.52f},
    {3.000, false, 67, 0.0f}, {4.000, false, 75, 0.0f} };
inline constexpr AuditionEvent kAmb3[] = {
    {0.250, true , 64, 0.46f}, {1.750, false, 64, 0.0f}, {2.500, true , 71, 0.43f}, {4.000, false, 71, 0.0f} };
inline constexpr AuditionEvent kAmb4[] = {
    {0.000, true , 77, 0.40f}, {0.500, true , 79, 0.46f}, {1.000, true , 82, 0.50f}, {4.000, false, 77, 0.0f},
    {4.000, false, 79, 0.0f}, {4.000, false, 82, 0.0f} };
inline constexpr AuditionPhrase kAmbV[5] = {
    {kAmb0, (int)(sizeof(kAmb0)/sizeof(kAmb0[0])), 4.00},
    {kAmb1, (int)(sizeof(kAmb1)/sizeof(kAmb1[0])), 4.00},
    {kAmb2, (int)(sizeof(kAmb2)/sizeof(kAmb2[0])), 4.00},
    {kAmb3, (int)(sizeof(kAmb3)/sizeof(kAmb3[0])), 4.00},
    {kAmb4, (int)(sizeof(kAmb4)/sizeof(kAmb4[0])), 4.00},
};

// SEQ
inline constexpr AuditionEvent kSeq0[] = {
    {0.000, true , 48, 0.84f}, {4.000, false, 48, 0.0f} };
inline constexpr AuditionEvent kSeq1[] = {
    {0.000, true , 48, 0.84f}, {0.000, true , 60, 0.80f}, {4.000, false, 48, 0.0f}, {4.000, false, 60, 0.0f} };
inline constexpr AuditionEvent kSeq2[] = {
    {0.000, true , 48, 0.86f}, {0.000, true , 55, 0.80f}, {4.000, false, 48, 0.0f}, {4.000, false, 55, 0.0f} };
inline constexpr AuditionEvent kSeq3[] = {
    {0.000, true , 36, 0.88f}, {4.000, false, 36, 0.0f} };
inline constexpr AuditionEvent kSeq4[] = {
    {0.000, true , 48, 0.84f}, {2.000, false, 48, 0.0f}, {2.000, true , 44, 0.82f}, {4.000, false, 44, 0.0f} };
inline constexpr AuditionPhrase kSeqV[5] = {
    {kSeq0, (int)(sizeof(kSeq0)/sizeof(kSeq0[0])), 4.00},
    {kSeq1, (int)(sizeof(kSeq1)/sizeof(kSeq1[0])), 4.00},
    {kSeq2, (int)(sizeof(kSeq2)/sizeof(kSeq2[0])), 4.00},
    {kSeq3, (int)(sizeof(kSeq3)/sizeof(kSeq3[0])), 4.00},
    {kSeq4, (int)(sizeof(kSeq4)/sizeof(kSeq4[0])), 4.00},
};

inline constexpr const AuditionPhrase* kVariants[12] = { kPadV, kLeadV, kBassV, kArpV, kStabV, kKeysV, kPluckV, kFxV, kDroneV, kPercV, kAmbV, kSeqV };
} // namespace audition_detail

inline AuditionPhrase auditionPhraseFor(int category, int variant) noexcept
{
    if(category < 0 || category > 11) category = 0;
    int v = ((variant % 5) + 5) % 5;
    return audition_detail::kVariants[category][v];
}
} // namespace jove

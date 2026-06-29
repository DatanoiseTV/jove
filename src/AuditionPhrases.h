/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <cstddef>

// LISTEN/audition demo phrases. 12 preset categories x 5 musical varieties each,
// so a patch is auditioned with material that fits its voice (basslines for BASS,
// lush swells for PAD, stabs for STAB, a held chord for ARP/SEQ so the patch's own
// arp/sequencer plays, ...) and a different variety each time a preset is loaded.
//
// Event times are authored at a 120 BPM reference (a beat = 0.5 s); the processor
// tempo-scales them to the host BPM so the demo plays in the DAW's tempo. Every
// phrase's loop length is snapped to a whole number of beats at that reference, so
// the loop period stays locked to the beat grid and repeats perfectly in time.
// Tonal centre C minor so any patch sounds musical.
namespace jove
{
struct AuditionEvent { double t; bool on; int note; float vel; };
struct AuditionPhrase { const AuditionEvent* ev; int count; double loop; };

namespace audition_detail
{
// PAD
inline constexpr AuditionEvent kPad0[] = {
    {0.000, true , 48, 0.55f}, {0.010, true , 55, 0.53f}, {0.020, true , 63, 0.50f}, {0.030, true , 70, 0.48f},
    {1.180, false, 48, 0.0f}, {1.180, false, 55, 0.0f}, {1.180, false, 63, 0.0f}, {1.180, false, 70, 0.0f},
    {1.200, true , 56, 0.56f}, {1.210, true , 63, 0.54f}, {1.220, true , 68, 0.51f}, {1.230, true , 72, 0.49f},
    {2.380, false, 56, 0.0f}, {2.380, false, 63, 0.0f}, {2.380, false, 68, 0.0f}, {2.380, false, 72, 0.0f},
    {2.400, true , 51, 0.55f}, {2.410, true , 58, 0.52f}, {2.420, true , 63, 0.50f}, {2.430, true , 67, 0.48f},
    {3.580, false, 51, 0.0f}, {3.580, false, 58, 0.0f}, {3.580, false, 63, 0.0f}, {3.580, false, 67, 0.0f},
    {3.600, true , 58, 0.54f}, {3.610, true , 62, 0.52f}, {3.620, true , 65, 0.50f}, {3.630, true , 70, 0.47f},
    {4.900, false, 58, 0.0f}, {4.900, false, 62, 0.0f}, {4.900, false, 65, 0.0f}, {4.900, false, 70, 0.0f} };
inline constexpr AuditionEvent kPad1[] = {
    {0.000, true , 48, 0.54f}, {0.010, true , 55, 0.52f}, {0.020, true , 60, 0.50f}, {0.030, true , 63, 0.48f},
    {1.280, false, 48, 0.0f}, {1.280, false, 55, 0.0f}, {1.280, false, 60, 0.0f}, {1.280, false, 63, 0.0f},
    {1.300, true , 53, 0.56f}, {1.310, true , 56, 0.53f}, {1.320, true , 60, 0.51f}, {1.330, true , 65, 0.49f},
    {2.580, false, 53, 0.0f}, {2.580, false, 56, 0.0f}, {2.580, false, 60, 0.0f}, {2.580, false, 65, 0.0f},
    {2.600, true , 58, 0.55f}, {2.610, true , 65, 0.52f}, {2.620, true , 70, 0.50f}, {2.630, true , 74, 0.47f},
    {3.880, false, 58, 0.0f}, {3.880, false, 65, 0.0f}, {3.880, false, 70, 0.0f}, {3.880, false, 74, 0.0f},
    {3.900, true , 51, 0.54f}, {3.910, true , 58, 0.51f}, {3.920, true , 67, 0.49f}, {3.930, true , 75, 0.46f},
    {5.100, false, 51, 0.0f}, {5.100, false, 58, 0.0f}, {5.100, false, 67, 0.0f}, {5.100, false, 75, 0.0f} };
inline constexpr AuditionEvent kPad2[] = {
    {0.000, true , 48, 0.52f}, {0.010, true , 51, 0.50f}, {0.020, true , 55, 0.49f}, {0.030, true , 58, 0.47f},
    {0.040, true , 63, 0.60f}, {1.180, false, 63, 0.0f}, {1.200, true , 65, 0.58f}, {2.380, false, 65, 0.0f},
    {2.400, true , 67, 0.57f}, {3.580, false, 67, 0.0f}, {3.600, true , 70, 0.55f}, {4.740, false, 48, 0.0f},
    {4.740, false, 51, 0.0f}, {4.740, false, 55, 0.0f}, {4.740, false, 58, 0.0f}, {4.740, false, 70, 0.0f} };
inline constexpr AuditionEvent kPad3[] = {
    {0.000, true , 48, 0.55f}, {0.010, true , 55, 0.52f}, {0.020, true , 63, 0.50f}, {0.030, true , 67, 0.48f},
    {1.380, false, 48, 0.0f}, {1.380, false, 55, 0.0f}, {1.380, false, 63, 0.0f}, {1.380, false, 67, 0.0f},
    {1.400, true , 56, 0.56f}, {1.410, true , 63, 0.53f}, {1.420, true , 68, 0.51f}, {1.430, true , 72, 0.48f},
    {2.780, false, 56, 0.0f}, {2.780, false, 63, 0.0f}, {2.780, false, 68, 0.0f}, {2.780, false, 72, 0.0f},
    {2.800, true , 53, 0.57f}, {2.810, true , 60, 0.54f}, {2.820, true , 65, 0.51f}, {2.830, true , 68, 0.49f},
    {4.180, false, 53, 0.0f}, {4.180, false, 60, 0.0f}, {4.180, false, 65, 0.0f}, {4.180, false, 68, 0.0f},
    {4.200, true , 55, 0.58f}, {4.210, true , 59, 0.55f}, {4.220, true , 67, 0.52f}, {4.230, true , 74, 0.50f},
    {5.500, false, 55, 0.0f}, {5.500, false, 59, 0.0f}, {5.500, false, 67, 0.0f}, {5.500, false, 74, 0.0f} };
inline constexpr AuditionEvent kPad4[] = {
    {0.000, true , 48, 0.53f}, {0.010, true , 55, 0.51f}, {0.020, true , 63, 0.49f}, {0.030, true , 74, 0.46f},
    {2.230, false, 48, 0.0f}, {2.230, false, 55, 0.0f}, {2.230, false, 63, 0.0f}, {2.230, false, 74, 0.0f},
    {2.300, true , 57, 0.58f}, {2.310, true , 60, 0.55f}, {2.320, true , 65, 0.52f}, {2.330, true , 69, 0.50f},
    {4.500, false, 57, 0.0f}, {4.500, false, 60, 0.0f}, {4.500, false, 65, 0.0f}, {4.500, false, 69, 0.0f} };
inline constexpr AuditionPhrase kPadV[5] = {
    {kPad0, (int)(sizeof(kPad0)/sizeof(kPad0[0])), 5.00},
    {kPad1, (int)(sizeof(kPad1)/sizeof(kPad1[0])), 5.50},
    {kPad2, (int)(sizeof(kPad2)/sizeof(kPad2[0])), 5.00},
    {kPad3, (int)(sizeof(kPad3)/sizeof(kPad3[0])), 5.50},
    {kPad4, (int)(sizeof(kPad4)/sizeof(kPad4[0])), 4.50},
};

// LEAD
inline constexpr AuditionEvent kLead0[] = {
    {0.000, true , 67, 0.92f}, {0.220, false, 67, 0.0f}, {0.250, true , 70, 0.80f}, {0.450, false, 70, 0.0f},
    {0.500, true , 72, 0.88f}, {0.800, false, 72, 0.0f}, {0.850, true , 75, 0.82f}, {1.050, false, 75, 0.0f},
    {1.100, true , 72, 0.78f}, {1.300, false, 72, 0.0f}, {1.350, true , 70, 0.85f}, {1.700, false, 70, 0.0f},
    {1.750, true , 67, 0.80f}, {1.950, false, 67, 0.0f}, {2.000, true , 75, 0.90f}, {2.250, false, 75, 0.0f},
    {2.300, true , 79, 0.85f}, {2.550, false, 79, 0.0f}, {2.600, true , 84, 0.96f}, {3.350, false, 84, 0.0f} };
inline constexpr AuditionEvent kLead1[] = {
    {0.000, true , 72, 0.90f}, {0.100, false, 72, 0.0f}, {0.125, true , 75, 0.78f}, {0.225, false, 75, 0.0f},
    {0.250, true , 70, 0.82f}, {0.350, false, 70, 0.0f}, {0.375, true , 72, 0.80f}, {0.475, false, 72, 0.0f},
    {0.500, true , 67, 0.88f}, {0.600, false, 67, 0.0f}, {0.625, true , 70, 0.78f}, {0.725, false, 70, 0.0f},
    {0.750, true , 75, 0.85f}, {0.850, false, 75, 0.0f}, {0.875, true , 77, 0.80f}, {0.975, false, 77, 0.0f},
    {1.000, true , 79, 0.92f}, {1.200, false, 79, 0.0f}, {1.250, true , 75, 0.80f}, {1.350, false, 75, 0.0f},
    {1.375, true , 72, 0.82f}, {1.475, false, 72, 0.0f}, {1.500, true , 70, 0.85f}, {1.600, false, 70, 0.0f},
    {1.625, true , 67, 0.80f}, {1.725, false, 67, 0.0f}, {1.750, true , 70, 0.84f}, {1.850, false, 70, 0.0f},
    {1.875, true , 72, 0.86f}, {1.975, false, 72, 0.0f}, {2.000, true , 75, 0.90f}, {2.400, false, 75, 0.0f},
    {2.450, true , 72, 0.82f}, {2.550, false, 72, 0.0f}, {2.575, true , 70, 0.80f}, {2.675, false, 70, 0.0f},
    {2.700, true , 67, 0.84f}, {2.800, false, 67, 0.0f}, {2.825, true , 63, 0.80f}, {2.900, false, 63, 0.0f},
    {2.925, true , 60, 0.94f}, {3.150, false, 60, 0.0f} };
inline constexpr AuditionEvent kLead2[] = {
    {0.000, true , 67, 0.88f}, {0.450, false, 67, 0.0f}, {0.700, true , 70, 0.82f}, {1.150, false, 70, 0.0f},
    {1.200, true , 72, 0.90f}, {2.000, false, 72, 0.0f}, {2.200, true , 66, 0.80f}, {2.350, false, 66, 0.0f},
    {2.360, true , 67, 0.85f}, {2.550, false, 67, 0.0f}, {2.800, true , 75, 0.92f}, {3.100, false, 75, 0.0f},
    {3.150, true , 72, 0.84f}, {3.400, false, 72, 0.0f}, {3.450, true , 70, 0.86f}, {3.740, false, 70, 0.0f} };
inline constexpr AuditionEvent kLead3[] = {
    {0.000, true , 60, 0.90f}, {0.180, false, 60, 0.0f}, {0.200, true , 63, 0.80f}, {0.380, false, 63, 0.0f},
    {0.400, true , 67, 0.84f}, {0.580, false, 67, 0.0f}, {0.600, true , 72, 0.92f}, {1.000, false, 72, 0.0f},
    {1.050, true , 68, 0.84f}, {1.230, false, 68, 0.0f}, {1.250, true , 72, 0.80f}, {1.430, false, 72, 0.0f},
    {1.450, true , 75, 0.84f}, {1.630, false, 75, 0.0f}, {1.650, true , 80, 0.90f}, {1.900, false, 80, 0.0f},
    {1.950, true , 70, 0.86f}, {2.130, false, 70, 0.0f}, {2.150, true , 74, 0.82f}, {2.330, false, 74, 0.0f},
    {2.350, true , 77, 0.84f}, {2.530, false, 77, 0.0f}, {2.550, true , 82, 0.92f}, {2.800, false, 82, 0.0f},
    {2.850, true , 79, 0.82f}, {3.010, false, 79, 0.0f}, {3.050, true , 72, 0.94f}, {3.350, false, 72, 0.0f} };
inline constexpr AuditionEvent kLead4[] = {
    {0.000, true , 67, 0.90f}, {0.200, false, 67, 0.0f}, {0.250, true , 70, 0.82f}, {0.450, false, 70, 0.0f},
    {0.500, true , 72, 0.88f}, {0.950, false, 72, 0.0f}, {1.300, true , 68, 0.86f}, {1.500, false, 68, 0.0f},
    {1.550, true , 72, 0.84f}, {1.750, false, 72, 0.0f}, {1.800, true , 75, 0.92f}, {2.350, false, 75, 0.0f},
    {2.550, true , 72, 0.84f}, {2.720, false, 72, 0.0f}, {2.750, true , 70, 0.80f}, {2.920, false, 70, 0.0f},
    {3.100, true , 75, 0.88f}, {3.300, false, 75, 0.0f}, {3.320, true , 79, 0.94f}, {3.500, false, 79, 0.0f} };
inline constexpr AuditionPhrase kLeadV[5] = {
    {kLead0, (int)(sizeof(kLead0)/sizeof(kLead0[0])), 3.50},
    {kLead1, (int)(sizeof(kLead1)/sizeof(kLead1[0])), 3.50},
    {kLead2, (int)(sizeof(kLead2)/sizeof(kLead2[0])), 4.00},
    {kLead3, (int)(sizeof(kLead3)/sizeof(kLead3[0])), 3.50},
    {kLead4, (int)(sizeof(kLead4)/sizeof(kLead4[0])), 3.50},
};

// BASS
inline constexpr AuditionEvent kBass0[] = {
    {0.000, true , 36, 1.00f}, {0.220, false, 36, 0.0f}, {0.250, true , 48, 0.85f}, {0.470, false, 48, 0.0f},
    {0.500, true , 36, 1.00f}, {0.720, false, 36, 0.0f}, {0.750, true , 48, 0.85f}, {0.970, false, 48, 0.0f},
    {1.000, true , 36, 1.00f}, {1.220, false, 36, 0.0f}, {1.250, true , 48, 0.85f}, {1.470, false, 48, 0.0f},
    {1.500, true , 39, 0.95f}, {1.700, false, 39, 0.0f}, {1.730, true , 43, 0.90f}, {1.930, false, 43, 0.0f} };
inline constexpr AuditionEvent kBass1[] = {
    {0.000, true , 36, 1.00f}, {0.100, false, 36, 0.0f}, {0.375, true , 36, 0.50f}, {0.440, false, 36, 0.0f},
    {0.750, true , 36, 0.92f}, {0.850, false, 36, 0.0f}, {0.875, true , 36, 0.50f}, {0.935, false, 36, 0.0f},
    {1.250, true , 39, 0.95f}, {1.350, false, 39, 0.0f}, {1.375, true , 36, 0.50f}, {1.435, false, 36, 0.0f},
    {1.625, true , 41, 0.90f}, {1.720, false, 41, 0.0f}, {1.800, true , 36, 0.50f}, {1.860, false, 36, 0.0f} };
inline constexpr AuditionEvent kBass2[] = {
    {0.000, true , 36, 0.95f}, {0.270, false, 36, 0.0f}, {0.300, true , 39, 0.85f}, {0.570, false, 39, 0.0f},
    {0.600, true , 41, 0.90f}, {0.870, false, 41, 0.0f}, {0.900, true , 43, 0.88f}, {1.170, false, 43, 0.0f},
    {1.200, true , 44, 0.90f}, {1.470, false, 44, 0.0f}, {1.500, true , 43, 0.85f}, {1.770, false, 43, 0.0f},
    {1.800, true , 41, 0.90f}, {2.070, false, 41, 0.0f}, {2.100, true , 39, 0.85f}, {2.340, false, 39, 0.0f} };
inline constexpr AuditionEvent kBass3[] = {
    {0.000, true , 36, 1.00f}, {0.115, false, 36, 0.0f}, {0.125, true , 36, 0.80f}, {0.240, false, 36, 0.0f},
    {0.250, true , 48, 0.95f}, {0.365, false, 48, 0.0f}, {0.375, true , 36, 0.85f}, {0.490, false, 36, 0.0f},
    {0.500, true , 39, 0.90f}, {0.615, true , 38, 0.85f}, {0.625, false, 39, 0.0f}, {0.740, false, 38, 0.0f},
    {0.750, true , 36, 0.85f}, {0.865, false, 36, 0.0f}, {0.875, true , 36, 0.80f}, {0.990, false, 36, 0.0f},
    {1.000, true , 48, 0.95f}, {1.115, false, 48, 0.0f}, {1.125, true , 36, 0.80f}, {1.240, false, 36, 0.0f},
    {1.250, true , 39, 0.90f}, {1.365, true , 41, 0.90f}, {1.375, false, 39, 0.0f}, {1.490, false, 41, 0.0f},
    {1.500, true , 36, 0.85f}, {1.615, false, 36, 0.0f}, {1.625, true , 43, 0.90f}, {1.740, false, 43, 0.0f},
    {1.750, true , 36, 0.85f}, {1.865, false, 36, 0.0f} };
inline constexpr AuditionEvent kBass4[] = {
    {0.000, true , 36, 1.00f}, {0.750, false, 36, 0.0f}, {1.050, true , 34, 0.90f}, {1.700, false, 34, 0.0f},
    {1.950, true , 39, 0.95f}, {2.450, false, 39, 0.0f} };
inline constexpr AuditionPhrase kBassV[5] = {
    {kBass0, (int)(sizeof(kBass0)/sizeof(kBass0[0])), 2.00},
    {kBass1, (int)(sizeof(kBass1)/sizeof(kBass1[0])), 2.00},
    {kBass2, (int)(sizeof(kBass2)/sizeof(kBass2[0])), 2.50},
    {kBass3, (int)(sizeof(kBass3)/sizeof(kBass3[0])), 2.00},
    {kBass4, (int)(sizeof(kBass4)/sizeof(kBass4[0])), 2.50},
};

// ARP
inline constexpr AuditionEvent kArp0[] = {
    {0.000, true , 48, 0.82f}, {0.006, true , 51, 0.80f}, {0.012, true , 55, 0.84f}, {3.500, false, 48, 0.0f},
    {3.500, false, 51, 0.0f}, {3.500, false, 55, 0.0f} };
inline constexpr AuditionEvent kArp1[] = {
    {0.000, true , 48, 0.84f}, {0.006, true , 51, 0.82f}, {0.012, true , 55, 0.80f}, {0.018, true , 58, 0.86f},
    {3.500, false, 48, 0.0f}, {3.500, false, 51, 0.0f}, {3.500, false, 55, 0.0f}, {3.500, false, 58, 0.0f} };
inline constexpr AuditionEvent kArp2[] = {
    {0.000, true , 48, 0.82f}, {0.006, true , 51, 0.84f}, {0.012, true , 55, 0.80f}, {1.950, false, 48, 0.0f},
    {1.954, false, 51, 0.0f}, {1.958, false, 55, 0.0f}, {2.000, true , 44, 0.86f}, {2.006, true , 48, 0.84f},
    {2.012, true , 51, 0.82f}, {2.018, true , 56, 0.80f}, {3.900, false, 44, 0.0f}, {3.904, false, 48, 0.0f},
    {3.908, false, 51, 0.0f}, {3.912, false, 56, 0.0f} };
inline constexpr AuditionEvent kArp3[] = {
    {0.000, true , 48, 0.84f}, {0.006, true , 55, 0.82f}, {0.012, true , 58, 0.86f}, {0.018, true , 62, 0.80f},
    {3.500, false, 48, 0.0f}, {3.500, false, 55, 0.0f}, {3.500, false, 58, 0.0f}, {3.500, false, 62, 0.0f} };
inline constexpr AuditionEvent kArp4[] = {
    {0.000, true , 48, 0.82f}, {0.006, true , 50, 0.84f}, {0.012, true , 55, 0.80f}, {1.950, false, 48, 0.0f},
    {1.954, false, 50, 0.0f}, {1.958, false, 55, 0.0f}, {2.000, true , 48, 0.84f}, {2.006, true , 51, 0.82f},
    {2.012, true , 55, 0.86f}, {3.900, false, 48, 0.0f}, {3.904, false, 51, 0.0f}, {3.908, false, 55, 0.0f} };
inline constexpr AuditionPhrase kArpV[5] = {
    {kArp0, (int)(sizeof(kArp0)/sizeof(kArp0[0])), 3.50},
    {kArp1, (int)(sizeof(kArp1)/sizeof(kArp1[0])), 3.50},
    {kArp2, (int)(sizeof(kArp2)/sizeof(kArp2[0])), 4.00},
    {kArp3, (int)(sizeof(kArp3)/sizeof(kArp3[0])), 3.50},
    {kArp4, (int)(sizeof(kArp4)/sizeof(kArp4[0])), 4.00},
};

// STAB
inline constexpr AuditionEvent kStab0[] = {
    {0.250, true , 60, 0.90f}, {0.253, true , 63, 0.90f}, {0.256, true , 67, 0.90f}, {0.400, false, 60, 0.0f},
    {0.400, false, 63, 0.0f}, {0.400, false, 67, 0.0f}, {0.750, true , 60, 1.00f}, {0.753, true , 63, 1.00f},
    {0.756, true , 67, 1.00f}, {0.900, false, 60, 0.0f}, {0.900, false, 63, 0.0f}, {0.900, false, 67, 0.0f},
    {1.250, true , 60, 0.90f}, {1.253, true , 63, 0.90f}, {1.256, true , 67, 0.90f}, {1.400, false, 60, 0.0f},
    {1.400, false, 63, 0.0f}, {1.400, false, 67, 0.0f}, {1.750, true , 60, 1.00f}, {1.753, true , 63, 1.00f},
    {1.756, true , 67, 1.00f}, {1.900, false, 60, 0.0f}, {1.900, false, 63, 0.0f}, {1.900, false, 67, 0.0f} };
inline constexpr AuditionEvent kStab1[] = {
    {0.000, true , 48, 0.88f}, {0.003, true , 55, 0.88f}, {0.006, true , 63, 0.88f}, {0.160, false, 48, 0.0f},
    {0.160, false, 55, 0.0f}, {0.160, false, 63, 0.0f}, {0.500, true , 48, 1.00f}, {0.503, true , 55, 1.00f},
    {0.506, true , 63, 1.00f}, {0.660, false, 48, 0.0f}, {0.660, false, 55, 0.0f}, {0.660, false, 63, 0.0f},
    {1.000, true , 48, 0.88f}, {1.003, true , 55, 0.88f}, {1.006, true , 63, 0.88f}, {1.160, false, 48, 0.0f},
    {1.160, false, 55, 0.0f}, {1.160, false, 63, 0.0f}, {1.500, true , 48, 1.00f}, {1.503, true , 55, 1.00f},
    {1.506, true , 63, 1.00f}, {1.660, false, 48, 0.0f}, {1.660, false, 55, 0.0f}, {1.660, false, 63, 0.0f} };
inline constexpr AuditionEvent kStab2[] = {
    {0.500, true , 63, 0.85f}, {0.503, true , 67, 0.85f}, {0.506, true , 70, 0.85f}, {0.590, false, 63, 0.0f},
    {0.590, false, 67, 0.0f}, {0.590, false, 70, 0.0f}, {0.625, true , 63, 0.88f}, {0.628, true , 67, 0.88f},
    {0.631, true , 70, 0.88f}, {0.715, false, 63, 0.0f}, {0.715, false, 67, 0.0f}, {0.715, false, 70, 0.0f},
    {1.000, true , 63, 0.85f}, {1.003, true , 67, 0.85f}, {1.006, true , 70, 0.85f}, {1.090, false, 63, 0.0f},
    {1.090, false, 67, 0.0f}, {1.090, false, 70, 0.0f}, {1.125, true , 63, 0.88f}, {1.128, true , 67, 0.88f},
    {1.131, true , 70, 0.88f}, {1.215, false, 63, 0.0f}, {1.215, false, 67, 0.0f}, {1.215, false, 70, 0.0f},
    {1.500, true , 63, 0.85f}, {1.503, true , 67, 0.85f}, {1.506, true , 70, 0.85f}, {1.590, false, 63, 0.0f},
    {1.590, false, 67, 0.0f}, {1.590, false, 70, 0.0f}, {1.625, true , 63, 0.88f}, {1.628, true , 67, 0.88f},
    {1.631, true , 70, 0.88f}, {1.715, false, 63, 0.0f}, {1.715, false, 67, 0.0f}, {1.715, false, 70, 0.0f} };
inline constexpr AuditionEvent kStab3[] = {
    {0.000, true , 56, 1.00f}, {0.003, true , 60, 1.00f}, {0.006, true , 63, 1.00f}, {0.130, false, 56, 0.0f},
    {0.130, false, 60, 0.0f}, {0.130, false, 63, 0.0f}, {0.375, true , 56, 0.90f}, {0.378, true , 60, 0.90f},
    {0.381, true , 63, 0.90f}, {0.505, false, 56, 0.0f}, {0.505, false, 60, 0.0f}, {0.505, false, 63, 0.0f},
    {0.750, true , 56, 0.90f}, {0.753, true , 60, 0.90f}, {0.756, true , 63, 0.90f}, {0.880, false, 56, 0.0f},
    {0.880, false, 60, 0.0f}, {0.880, false, 63, 0.0f}, {1.000, true , 56, 1.00f}, {1.003, true , 60, 1.00f},
    {1.006, true , 63, 1.00f}, {1.130, false, 56, 0.0f}, {1.130, false, 60, 0.0f}, {1.130, false, 63, 0.0f},
    {1.375, true , 56, 0.90f}, {1.378, true , 60, 0.90f}, {1.381, true , 63, 0.90f}, {1.505, false, 56, 0.0f},
    {1.505, false, 60, 0.0f}, {1.505, false, 63, 0.0f}, {1.750, true , 56, 0.90f}, {1.753, true , 60, 0.90f},
    {1.756, true , 63, 0.90f}, {1.880, false, 56, 0.0f}, {1.880, false, 60, 0.0f}, {1.880, false, 63, 0.0f} };
inline constexpr AuditionEvent kStab4[] = {
    {0.000, true , 60, 1.00f}, {0.003, true , 63, 1.00f}, {0.006, true , 67, 1.00f}, {0.160, false, 60, 0.0f},
    {0.160, false, 63, 0.0f}, {0.160, false, 67, 0.0f}, {0.500, true , 60, 0.90f}, {0.503, true , 63, 0.90f},
    {0.506, true , 67, 0.90f}, {0.660, false, 60, 0.0f}, {0.660, false, 63, 0.0f}, {0.660, false, 67, 0.0f},
    {1.000, true , 65, 1.00f}, {1.003, true , 68, 1.00f}, {1.006, true , 72, 1.00f}, {1.160, false, 65, 0.0f},
    {1.160, false, 68, 0.0f}, {1.160, false, 72, 0.0f}, {1.500, true , 65, 0.90f}, {1.503, true , 68, 0.90f},
    {1.506, true , 72, 0.90f}, {1.660, false, 65, 0.0f}, {1.660, false, 68, 0.0f}, {1.660, false, 72, 0.0f},
    {2.000, true , 60, 0.95f}, {2.003, true , 63, 0.95f}, {2.006, true , 67, 0.95f}, {2.160, false, 60, 0.0f},
    {2.160, false, 63, 0.0f}, {2.160, false, 67, 0.0f} };
inline constexpr AuditionPhrase kStabV[5] = {
    {kStab0, (int)(sizeof(kStab0)/sizeof(kStab0[0])), 2.00},
    {kStab1, (int)(sizeof(kStab1)/sizeof(kStab1[0])), 2.00},
    {kStab2, (int)(sizeof(kStab2)/sizeof(kStab2[0])), 2.00},
    {kStab3, (int)(sizeof(kStab3)/sizeof(kStab3[0])), 2.00},
    {kStab4, (int)(sizeof(kStab4)/sizeof(kStab4[0])), 2.50},
};

// KEYS
inline constexpr AuditionEvent kKeys0[] = {
    {0.000, true , 48, 0.70f}, {0.000, true , 51, 0.70f}, {0.000, true , 55, 0.70f}, {0.000, true , 67, 0.85f},
    {0.450, false, 67, 0.0f}, {0.500, true , 70, 0.85f}, {0.950, false, 48, 0.0f}, {0.950, false, 51, 0.0f},
    {0.950, false, 55, 0.0f}, {0.950, false, 70, 0.0f}, {1.000, true , 56, 0.70f}, {1.000, true , 60, 0.70f},
    {1.000, true , 63, 0.70f}, {1.000, true , 72, 0.88f}, {1.450, false, 72, 0.0f}, {1.500, true , 70, 0.85f},
    {1.950, false, 56, 0.0f}, {1.950, false, 60, 0.0f}, {1.950, false, 63, 0.0f}, {1.950, false, 70, 0.0f},
    {2.000, true , 51, 0.70f}, {2.000, true , 55, 0.70f}, {2.000, true , 58, 0.70f}, {2.000, true , 67, 0.85f},
    {2.450, false, 67, 0.0f}, {2.500, true , 68, 0.82f}, {2.950, false, 51, 0.0f}, {2.950, false, 55, 0.0f},
    {2.950, false, 58, 0.0f}, {2.950, false, 68, 0.0f}, {3.000, true , 53, 0.70f}, {3.000, true , 58, 0.70f},
    {3.000, true , 62, 0.70f}, {3.000, true , 65, 0.85f}, {3.450, false, 65, 0.0f}, {3.500, true , 67, 0.85f},
    {3.900, false, 53, 0.0f}, {3.900, false, 58, 0.0f}, {3.900, false, 62, 0.0f}, {3.900, false, 67, 0.0f} };
inline constexpr AuditionEvent kKeys1[] = {
    {0.000, true , 51, 0.70f}, {0.000, true , 55, 0.70f}, {0.000, true , 58, 0.70f}, {0.000, true , 60, 0.70f},
    {0.000, true , 67, 0.85f}, {0.350, false, 67, 0.0f}, {0.400, false, 51, 0.0f}, {0.400, false, 55, 0.0f},
    {0.400, false, 58, 0.0f}, {0.400, false, 60, 0.0f}, {0.400, true , 70, 0.85f}, {0.700, false, 70, 0.0f},
    {0.750, true , 51, 0.70f}, {0.750, true , 55, 0.70f}, {0.750, true , 58, 0.70f}, {0.750, true , 60, 0.70f},
    {1.000, true , 72, 0.88f}, {1.150, false, 51, 0.0f}, {1.150, false, 55, 0.0f}, {1.150, false, 58, 0.0f},
    {1.150, false, 60, 0.0f}, {1.350, false, 72, 0.0f}, {1.500, true , 51, 0.70f}, {1.500, true , 55, 0.70f},
    {1.500, true , 58, 0.70f}, {1.500, true , 60, 0.70f}, {1.500, true , 66, 0.85f}, {1.850, false, 66, 0.0f},
    {1.900, false, 51, 0.0f}, {1.900, false, 55, 0.0f}, {1.900, false, 58, 0.0f}, {1.900, false, 60, 0.0f},
    {1.900, true , 67, 0.85f}, {2.150, false, 67, 0.0f}, {2.250, true , 53, 0.70f}, {2.250, true , 56, 0.70f},
    {2.250, true , 60, 0.70f}, {2.250, true , 63, 0.70f}, {2.500, true , 65, 0.85f}, {2.650, false, 53, 0.0f},
    {2.650, false, 56, 0.0f}, {2.650, false, 60, 0.0f}, {2.650, false, 63, 0.0f}, {2.850, false, 65, 0.0f},
    {3.000, true , 53, 0.70f}, {3.000, true , 56, 0.70f}, {3.000, true , 60, 0.70f}, {3.000, true , 63, 0.70f},
    {3.000, true , 75, 0.88f}, {3.350, false, 75, 0.0f}, {3.400, false, 53, 0.0f}, {3.400, false, 56, 0.0f},
    {3.400, false, 60, 0.0f}, {3.400, false, 63, 0.0f}, {3.400, true , 72, 0.85f}, {3.500, false, 72, 0.0f} };
inline constexpr AuditionEvent kKeys2[] = {
    {0.000, true , 50, 0.70f}, {0.000, true , 53, 0.70f}, {0.000, true , 56, 0.70f}, {0.000, true , 60, 0.70f},
    {0.000, true , 65, 0.85f}, {0.450, false, 65, 0.0f}, {0.500, true , 68, 0.85f}, {0.950, false, 50, 0.0f},
    {0.950, false, 53, 0.0f}, {0.950, false, 56, 0.0f}, {0.950, false, 60, 0.0f}, {0.950, false, 68, 0.0f},
    {1.000, true , 53, 0.70f}, {1.000, true , 55, 0.70f}, {1.000, true , 59, 0.70f}, {1.000, true , 62, 0.70f},
    {1.000, true , 65, 0.85f}, {1.450, false, 65, 0.0f}, {1.500, true , 67, 0.85f}, {1.950, false, 53, 0.0f},
    {1.950, false, 55, 0.0f}, {1.950, false, 59, 0.0f}, {1.950, false, 62, 0.0f}, {1.950, false, 67, 0.0f},
    {2.000, true , 48, 0.70f}, {2.000, true , 51, 0.70f}, {2.000, true , 55, 0.70f}, {2.000, true , 58, 0.70f},
    {2.000, true , 75, 0.88f}, {2.450, false, 75, 0.0f}, {2.500, true , 72, 0.88f}, {2.950, false, 72, 0.0f},
    {3.000, true , 67, 0.85f}, {3.450, false, 67, 0.0f}, {3.500, true , 75, 0.88f}, {3.900, false, 48, 0.0f},
    {3.900, false, 51, 0.0f}, {3.900, false, 55, 0.0f}, {3.900, false, 58, 0.0f}, {3.900, false, 75, 0.0f} };
inline constexpr AuditionEvent kKeys3[] = {
    {0.000, true , 48, 0.70f}, {0.000, true , 67, 0.88f}, {0.220, false, 48, 0.0f}, {0.250, true , 51, 0.70f},
    {0.470, false, 51, 0.0f}, {0.500, true , 55, 0.70f}, {0.720, false, 55, 0.0f}, {0.750, true , 60, 0.70f},
    {0.970, false, 60, 0.0f}, {1.000, true , 55, 0.70f}, {1.220, false, 55, 0.0f}, {1.250, true , 51, 0.70f},
    {1.470, false, 51, 0.0f}, {1.500, true , 48, 0.70f}, {1.700, false, 67, 0.0f}, {1.720, false, 48, 0.0f},
    {1.750, true , 56, 0.70f}, {1.750, true , 72, 0.88f}, {1.970, false, 56, 0.0f}, {2.000, true , 60, 0.70f},
    {2.220, false, 60, 0.0f}, {2.250, true , 63, 0.70f}, {2.470, false, 63, 0.0f}, {2.500, true , 56, 0.70f},
    {2.720, false, 56, 0.0f}, {2.750, true , 60, 0.70f}, {2.970, false, 60, 0.0f}, {3.000, true , 63, 0.70f},
    {3.220, false, 63, 0.0f}, {3.250, true , 56, 0.70f}, {3.500, false, 56, 0.0f}, {3.500, false, 72, 0.0f} };
inline constexpr AuditionEvent kKeys4[] = {
    {0.000, true , 51, 0.70f}, {0.000, true , 55, 0.70f}, {0.000, true , 58, 0.70f}, {0.000, true , 60, 0.70f},
    {0.250, false, 51, 0.0f}, {0.250, false, 55, 0.0f}, {0.250, false, 58, 0.0f}, {0.250, false, 60, 0.0f},
    {0.250, true , 67, 0.85f}, {0.360, false, 67, 0.0f}, {0.375, true , 70, 0.85f}, {0.485, false, 70, 0.0f},
    {0.500, true , 72, 0.85f}, {0.610, false, 72, 0.0f}, {0.625, true , 70, 0.85f}, {0.735, false, 70, 0.0f},
    {0.750, true , 56, 0.70f}, {0.750, true , 60, 0.70f}, {0.750, true , 63, 0.70f}, {1.000, false, 56, 0.0f},
    {1.000, false, 60, 0.0f}, {1.000, false, 63, 0.0f}, {1.000, true , 67, 0.85f}, {1.110, false, 67, 0.0f},
    {1.125, true , 68, 0.85f}, {1.235, false, 68, 0.0f}, {1.250, true , 70, 0.85f}, {1.360, false, 70, 0.0f},
    {1.375, true , 72, 0.85f}, {1.485, false, 72, 0.0f}, {1.500, true , 51, 0.70f}, {1.500, true , 55, 0.70f},
    {1.500, true , 58, 0.70f}, {1.500, true , 60, 0.70f}, {1.750, false, 51, 0.0f}, {1.750, false, 55, 0.0f},
    {1.750, false, 58, 0.0f}, {1.750, false, 60, 0.0f}, {1.750, true , 75, 0.85f}, {1.860, false, 75, 0.0f},
    {1.875, true , 72, 0.85f}, {1.985, false, 72, 0.0f}, {2.000, true , 70, 0.85f}, {2.110, false, 70, 0.0f},
    {2.125, true , 67, 0.85f}, {2.235, false, 67, 0.0f}, {2.250, true , 56, 0.70f}, {2.250, true , 60, 0.70f},
    {2.250, true , 63, 0.70f}, {2.500, false, 56, 0.0f}, {2.500, false, 60, 0.0f}, {2.500, false, 63, 0.0f},
    {2.500, true , 67, 0.85f}, {2.610, false, 67, 0.0f}, {2.625, true , 70, 0.85f}, {2.735, false, 70, 0.0f},
    {2.750, true , 72, 0.85f}, {2.860, false, 72, 0.0f}, {2.875, true , 75, 0.85f}, {2.985, false, 75, 0.0f},
    {3.000, true , 72, 0.85f}, {3.110, false, 72, 0.0f}, {3.125, true , 70, 0.85f}, {3.150, false, 70, 0.0f} };
inline constexpr AuditionPhrase kKeysV[5] = {
    {kKeys0, (int)(sizeof(kKeys0)/sizeof(kKeys0[0])), 4.00},
    {kKeys1, (int)(sizeof(kKeys1)/sizeof(kKeys1[0])), 3.50},
    {kKeys2, (int)(sizeof(kKeys2)/sizeof(kKeys2[0])), 4.00},
    {kKeys3, (int)(sizeof(kKeys3)/sizeof(kKeys3[0])), 3.50},
    {kKeys4, (int)(sizeof(kKeys4)/sizeof(kKeys4[0])), 3.50},
};

// PLUCK
inline constexpr AuditionEvent kPluck0[] = {
    {0.000, true , 60, 0.92f}, {0.110, false, 60, 0.0f}, {0.125, true , 63, 0.82f}, {0.235, false, 63, 0.0f},
    {0.250, true , 67, 0.85f}, {0.360, false, 67, 0.0f}, {0.375, true , 72, 0.84f}, {0.485, false, 72, 0.0f},
    {0.500, true , 75, 0.90f}, {0.610, false, 75, 0.0f}, {0.625, true , 72, 0.80f}, {0.735, false, 72, 0.0f},
    {0.750, true , 67, 0.82f}, {0.860, false, 67, 0.0f}, {0.875, true , 63, 0.79f}, {0.985, false, 63, 0.0f},
    {1.000, true , 60, 0.92f}, {1.110, false, 60, 0.0f}, {1.125, true , 63, 0.82f}, {1.235, false, 63, 0.0f},
    {1.250, true , 67, 0.85f}, {1.360, false, 67, 0.0f}, {1.375, true , 72, 0.84f}, {1.485, false, 72, 0.0f},
    {1.500, true , 75, 0.90f}, {1.610, false, 75, 0.0f}, {1.625, true , 79, 0.95f}, {1.735, false, 79, 0.0f},
    {1.750, true , 75, 0.86f}, {1.860, false, 75, 0.0f}, {1.875, true , 72, 0.82f}, {1.985, false, 72, 0.0f} };
inline constexpr AuditionEvent kPluck1[] = {
    {0.000, true , 67, 0.90f}, {0.120, false, 67, 0.0f}, {0.375, true , 70, 0.88f}, {0.490, false, 70, 0.0f},
    {0.500, true , 67, 0.80f}, {0.620, false, 67, 0.0f}, {0.750, true , 75, 0.92f}, {0.900, false, 75, 0.0f},
    {1.125, true , 72, 0.86f}, {1.240, false, 72, 0.0f}, {1.250, true , 70, 0.79f}, {1.370, false, 70, 0.0f},
    {1.500, true , 67, 0.85f}, {1.620, false, 67, 0.0f}, {1.875, true , 63, 0.82f}, {1.990, false, 63, 0.0f},
    {2.000, true , 67, 0.88f}, {2.120, false, 67, 0.0f}, {2.250, true , 72, 0.93f}, {2.350, false, 72, 0.0f} };
inline constexpr AuditionEvent kPluck2[] = {
    {0.000, true , 60, 0.84f}, {0.100, false, 60, 0.0f}, {0.125, true , 63, 0.80f}, {0.225, false, 63, 0.0f},
    {0.250, true , 65, 0.82f}, {0.350, false, 65, 0.0f}, {0.375, true , 67, 0.84f}, {0.475, false, 67, 0.0f},
    {0.500, true , 70, 0.86f}, {0.600, false, 70, 0.0f}, {0.625, true , 72, 0.88f}, {0.725, false, 72, 0.0f},
    {0.750, true , 75, 0.90f}, {0.850, false, 75, 0.0f}, {0.875, true , 79, 0.95f}, {0.975, false, 79, 0.0f},
    {1.000, true , 77, 0.90f}, {1.100, false, 77, 0.0f}, {1.125, true , 75, 0.88f}, {1.225, false, 75, 0.0f},
    {1.250, true , 72, 0.86f}, {1.350, false, 72, 0.0f}, {1.375, true , 70, 0.84f}, {1.475, false, 70, 0.0f},
    {1.500, true , 67, 0.82f}, {1.600, false, 67, 0.0f}, {1.625, true , 65, 0.80f}, {1.725, false, 65, 0.0f},
    {1.750, true , 63, 0.79f}, {1.850, false, 63, 0.0f}, {1.875, true , 60, 0.84f}, {1.975, false, 60, 0.0f} };
inline constexpr AuditionEvent kPluck3[] = {
    {0.000, true , 67, 0.90f}, {0.150, false, 67, 0.0f}, {0.250, true , 70, 0.86f}, {0.400, false, 70, 0.0f},
    {0.500, true , 75, 0.93f}, {0.720, false, 75, 0.0f}, {1.300, true , 72, 0.85f}, {1.450, false, 72, 0.0f},
    {1.550, true , 70, 0.82f}, {1.700, false, 70, 0.0f}, {1.800, true , 67, 0.84f}, {1.950, false, 67, 0.0f},
    {2.050, true , 60, 0.88f}, {2.300, false, 60, 0.0f} };
inline constexpr AuditionEvent kPluck4[] = {
    {0.000, true , 60, 0.86f}, {0.120, false, 60, 0.0f}, {0.250, true , 72, 0.90f}, {0.370, false, 72, 0.0f},
    {0.500, true , 63, 0.82f}, {0.620, false, 63, 0.0f}, {0.750, true , 75, 0.92f}, {0.870, false, 75, 0.0f},
    {1.000, true , 67, 0.84f}, {1.120, false, 67, 0.0f}, {1.250, true , 79, 0.95f}, {1.370, false, 79, 0.0f},
    {1.500, true , 63, 0.80f}, {1.620, false, 63, 0.0f}, {1.750, true , 75, 0.90f}, {1.870, false, 75, 0.0f},
    {2.000, true , 60, 0.86f}, {2.120, false, 60, 0.0f}, {2.250, true , 72, 0.91f}, {2.350, false, 72, 0.0f} };
inline constexpr AuditionPhrase kPluckV[5] = {
    {kPluck0, (int)(sizeof(kPluck0)/sizeof(kPluck0[0])), 2.00},
    {kPluck1, (int)(sizeof(kPluck1)/sizeof(kPluck1[0])), 2.50},
    {kPluck2, (int)(sizeof(kPluck2)/sizeof(kPluck2[0])), 2.00},
    {kPluck3, (int)(sizeof(kPluck3)/sizeof(kPluck3[0])), 2.50},
    {kPluck4, (int)(sizeof(kPluck4)/sizeof(kPluck4[0])), 2.50},
};

// FX
inline constexpr AuditionEvent kFx0[] = {
    {0.000, true , 57, 0.62f}, {4.100, false, 57, 0.0f} };
inline constexpr AuditionEvent kFx1[] = {
    {0.000, true , 38, 0.70f}, {0.180, true , 74, 0.58f}, {4.400, false, 38, 0.0f}, {4.420, false, 74, 0.0f} };
inline constexpr AuditionEvent kFx2[] = {
    {0.000, true , 48, 0.55f}, {0.900, true , 53, 0.60f}, {1.900, true , 58, 0.66f}, {2.900, true , 63, 0.72f},
    {4.850, false, 48, 0.0f}, {4.880, false, 53, 0.0f}, {4.910, false, 58, 0.0f}, {4.940, false, 63, 0.0f} };
inline constexpr AuditionEvent kFx3[] = {
    {0.000, true , 40, 0.80f}, {0.550, false, 40, 0.0f}, {1.800, true , 52, 0.74f}, {2.350, false, 52, 0.0f},
    {3.600, true , 45, 0.85f}, {4.300, false, 45, 0.0f} };
inline constexpr AuditionEvent kFx4[] = {
    {0.000, true , 60, 0.64f}, {0.060, true , 62, 0.60f}, {0.120, true , 64, 0.58f}, {4.100, false, 60, 0.0f},
    {4.130, false, 62, 0.0f}, {4.160, false, 64, 0.0f} };
inline constexpr AuditionPhrase kFxV[5] = {
    {kFx0, (int)(sizeof(kFx0)/sizeof(kFx0[0])), 4.50},
    {kFx1, (int)(sizeof(kFx1)/sizeof(kFx1[0])), 4.50},
    {kFx2, (int)(sizeof(kFx2)/sizeof(kFx2[0])), 5.00},
    {kFx3, (int)(sizeof(kFx3)/sizeof(kFx3[0])), 4.50},
    {kFx4, (int)(sizeof(kFx4)/sizeof(kFx4[0])), 4.50},
};

// DRONE
inline constexpr AuditionEvent kDrone0[] = {
    {0.000, true , 24, 0.78f}, {0.000, true , 36, 0.72f}, {4.950, false, 24, 0.0f}, {4.950, false, 36, 0.0f} };
inline constexpr AuditionEvent kDrone1[] = {
    {0.000, true , 24, 0.80f}, {0.000, true , 31, 0.70f}, {4.950, false, 24, 0.0f}, {4.950, false, 31, 0.0f} };
inline constexpr AuditionEvent kDrone2[] = {
    {0.000, true , 24, 0.82f}, {0.000, true , 36, 0.74f}, {0.000, true , 43, 0.68f}, {5.450, false, 24, 0.0f},
    {5.450, false, 36, 0.0f}, {5.450, false, 43, 0.0f} };
inline constexpr AuditionEvent kDrone3[] = {
    {0.000, true , 26, 0.84f}, {2.200, true , 33, 0.66f}, {5.150, false, 26, 0.0f}, {5.150, false, 33, 0.0f} };
inline constexpr AuditionEvent kDrone4[] = {
    {0.000, true , 24, 0.80f}, {0.000, true , 48, 0.64f}, {2.800, false, 48, 0.0f}, {2.800, true , 51, 0.66f},
    {5.500, false, 24, 0.0f}, {5.500, false, 51, 0.0f} };
inline constexpr AuditionPhrase kDroneV[5] = {
    {kDrone0, (int)(sizeof(kDrone0)/sizeof(kDrone0[0])), 5.00},
    {kDrone1, (int)(sizeof(kDrone1)/sizeof(kDrone1[0])), 5.00},
    {kDrone2, (int)(sizeof(kDrone2)/sizeof(kDrone2[0])), 5.50},
    {kDrone3, (int)(sizeof(kDrone3)/sizeof(kDrone3[0])), 5.50},
    {kDrone4, (int)(sizeof(kDrone4)/sizeof(kDrone4[0])), 5.50},
};

// PERC
inline constexpr AuditionEvent kPerc0[] = {
    {0.000, true , 36, 1.00f}, {0.100, false, 36, 0.0f}, {0.250, true , 50, 0.80f}, {0.350, false, 50, 0.0f},
    {0.500, true , 36, 0.90f}, {0.600, false, 36, 0.0f}, {0.750, true , 52, 0.85f}, {0.850, false, 52, 0.0f},
    {1.000, true , 36, 1.00f}, {1.100, false, 36, 0.0f}, {1.250, true , 50, 0.80f}, {1.350, false, 50, 0.0f},
    {1.500, true , 36, 0.90f}, {1.600, false, 36, 0.0f}, {1.750, true , 52, 0.85f}, {1.850, false, 52, 0.0f} };
inline constexpr AuditionEvent kPerc1[] = {
    {0.000, true , 36, 1.00f}, {0.080, false, 36, 0.0f}, {0.250, true , 42, 0.70f}, {0.310, false, 42, 0.0f},
    {0.500, true , 38, 0.95f}, {0.580, false, 38, 0.0f}, {0.750, true , 36, 0.80f}, {0.830, false, 36, 0.0f},
    {0.875, true , 42, 0.70f}, {0.935, false, 42, 0.0f}, {1.000, true , 38, 0.90f}, {1.080, false, 38, 0.0f},
    {1.250, true , 36, 0.85f}, {1.310, false, 36, 0.0f}, {1.500, true , 38, 0.95f}, {1.580, false, 38, 0.0f},
    {1.625, true , 42, 0.70f}, {1.685, false, 42, 0.0f}, {1.750, true , 36, 0.90f}, {1.810, false, 36, 0.0f},
    {1.875, true , 38, 0.80f}, {1.935, false, 38, 0.0f} };
inline constexpr AuditionEvent kPerc2[] = {
    {0.000, true , 60, 0.95f}, {0.100, false, 60, 0.0f}, {0.200, true , 57, 0.85f}, {0.300, false, 57, 0.0f},
    {0.400, true , 53, 0.80f}, {0.500, false, 53, 0.0f}, {0.600, true , 50, 0.90f}, {0.700, false, 50, 0.0f},
    {0.800, true , 48, 1.00f}, {0.900, false, 48, 0.0f}, {1.000, true , 57, 0.85f}, {1.100, false, 57, 0.0f},
    {1.200, true , 53, 0.80f}, {1.300, false, 53, 0.0f}, {1.400, true , 48, 0.90f}, {1.500, false, 48, 0.0f} };
inline constexpr AuditionEvent kPerc3[] = {
    {0.000, true , 42, 1.00f}, {0.060, false, 42, 0.0f}, {0.125, true , 42, 0.70f}, {0.185, false, 42, 0.0f},
    {0.250, true , 42, 0.75f}, {0.310, false, 42, 0.0f}, {0.375, true , 42, 0.70f}, {0.435, false, 42, 0.0f},
    {0.500, true , 46, 0.95f}, {0.560, false, 46, 0.0f}, {0.625, true , 42, 0.70f}, {0.685, false, 42, 0.0f},
    {0.750, true , 42, 0.75f}, {0.810, false, 42, 0.0f}, {0.875, true , 42, 0.70f}, {0.935, false, 42, 0.0f},
    {1.000, true , 42, 1.00f}, {1.060, false, 42, 0.0f}, {1.125, true , 42, 0.70f}, {1.185, false, 42, 0.0f},
    {1.250, true , 42, 0.75f}, {1.310, false, 42, 0.0f}, {1.375, true , 42, 0.70f}, {1.435, false, 42, 0.0f},
    {1.500, true , 46, 0.95f}, {1.560, false, 46, 0.0f}, {1.625, true , 42, 0.70f}, {1.685, false, 42, 0.0f},
    {1.750, true , 42, 0.75f}, {1.810, false, 42, 0.0f}, {1.875, true , 42, 0.80f}, {1.935, false, 42, 0.0f} };
inline constexpr AuditionEvent kPerc4[] = {
    {0.000, true , 48, 0.90f}, {0.100, false, 48, 0.0f}, {0.250, true , 60, 0.80f}, {0.330, false, 60, 0.0f},
    {0.500, true , 55, 0.85f}, {0.580, false, 55, 0.0f}, {0.625, true , 60, 0.70f}, {0.685, false, 60, 0.0f},
    {0.750, true , 48, 0.95f}, {0.850, false, 48, 0.0f}, {1.000, true , 60, 0.80f}, {1.080, false, 60, 0.0f},
    {1.250, true , 55, 0.85f}, {1.330, false, 55, 0.0f}, {1.500, true , 48, 0.90f}, {1.600, false, 48, 0.0f},
    {1.750, true , 60, 1.00f}, {1.830, false, 60, 0.0f}, {1.875, true , 60, 0.70f}, {1.935, false, 60, 0.0f},
    {2.000, true , 55, 0.85f}, {2.080, false, 55, 0.0f}, {2.250, true , 48, 0.90f}, {2.350, false, 48, 0.0f} };
inline constexpr AuditionPhrase kPercV[5] = {
    {kPerc0, (int)(sizeof(kPerc0)/sizeof(kPerc0[0])), 2.00},
    {kPerc1, (int)(sizeof(kPerc1)/sizeof(kPerc1[0])), 2.00},
    {kPerc2, (int)(sizeof(kPerc2)/sizeof(kPerc2[0])), 1.50},
    {kPerc3, (int)(sizeof(kPerc3)/sizeof(kPerc3[0])), 2.00},
    {kPerc4, (int)(sizeof(kPerc4)/sizeof(kPerc4[0])), 2.50},
};

// AMB
inline constexpr AuditionEvent kAmb0[] = {
    {0.100, true , 72, 0.45f}, {2.400, false, 72, 0.0f}, {2.900, true , 75, 0.50f}, {4.900, false, 75, 0.0f} };
inline constexpr AuditionEvent kAmb1[] = {
    {0.200, true , 60, 0.42f}, {0.600, true , 79, 0.48f}, {4.000, false, 79, 0.0f}, {4.400, false, 60, 0.0f} };
inline constexpr AuditionEvent kAmb2[] = {
    {0.150, true , 60, 0.44f}, {1.600, true , 67, 0.50f}, {2.200, false, 60, 0.0f}, {3.100, true , 75, 0.52f},
    {3.700, false, 67, 0.0f}, {5.200, false, 75, 0.0f} };
inline constexpr AuditionEvent kAmb3[] = {
    {0.300, true , 64, 0.46f}, {1.800, false, 64, 0.0f}, {3.400, true , 71, 0.43f}, {4.900, false, 71, 0.0f} };
inline constexpr AuditionEvent kAmb4[] = {
    {0.200, true , 77, 0.40f}, {0.700, true , 79, 0.46f}, {1.200, true , 82, 0.52f}, {3.900, false, 77, 0.0f},
    {4.200, false, 82, 0.0f}, {4.450, false, 79, 0.0f} };
inline constexpr AuditionPhrase kAmbV[5] = {
    {kAmb0, (int)(sizeof(kAmb0)/sizeof(kAmb0[0])), 5.00},
    {kAmb1, (int)(sizeof(kAmb1)/sizeof(kAmb1[0])), 4.50},
    {kAmb2, (int)(sizeof(kAmb2)/sizeof(kAmb2[0])), 5.50},
    {kAmb3, (int)(sizeof(kAmb3)/sizeof(kAmb3[0])), 5.00},
    {kAmb4, (int)(sizeof(kAmb4)/sizeof(kAmb4[0])), 4.50},
};

// SEQ
inline constexpr AuditionEvent kSeq0[] = {
    {0.000, true , 48, 0.84f}, {3.350, false, 48, 0.0f} };
inline constexpr AuditionEvent kSeq1[] = {
    {0.000, true , 48, 0.84f}, {0.000, true , 60, 0.80f}, {3.350, false, 48, 0.0f}, {3.350, false, 60, 0.0f} };
inline constexpr AuditionEvent kSeq2[] = {
    {0.000, true , 48, 0.86f}, {0.000, true , 55, 0.80f}, {3.350, false, 48, 0.0f}, {3.350, false, 55, 0.0f} };
inline constexpr AuditionEvent kSeq3[] = {
    {0.000, true , 36, 0.88f}, {3.350, false, 36, 0.0f} };
inline constexpr AuditionEvent kSeq4[] = {
    {0.000, true , 48, 0.84f}, {1.950, false, 48, 0.0f}, {2.000, true , 44, 0.82f}, {3.950, false, 44, 0.0f} };
inline constexpr AuditionPhrase kSeqV[5] = {
    {kSeq0, (int)(sizeof(kSeq0)/sizeof(kSeq0[0])), 3.50},
    {kSeq1, (int)(sizeof(kSeq1)/sizeof(kSeq1[0])), 3.50},
    {kSeq2, (int)(sizeof(kSeq2)/sizeof(kSeq2[0])), 3.50},
    {kSeq3, (int)(sizeof(kSeq3)/sizeof(kSeq3[0])), 3.50},
    {kSeq4, (int)(sizeof(kSeq4)/sizeof(kSeq4[0])), 4.00},
};

inline constexpr const AuditionPhrase* kVariants[12] = { kPadV, kLeadV, kBassV, kArpV, kStabV, kKeysV, kPluckV, kFxV, kDroneV, kPercV, kAmbV, kSeqV };
} // namespace audition_detail

// Pick a phrase for a category index (UI CATEGORIES order) and a rotating variant
// (any int; wrapped to 0..4). Out-of-range category -> PAD.
inline AuditionPhrase auditionPhraseFor(int category, int variant) noexcept
{
    if(category < 0 || category > 11) category = 0;
    int v = ((variant % 5) + 5) % 5;
    return audition_detail::kVariants[category][v];
}
} // namespace jove

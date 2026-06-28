/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.

  Factory patch bank, sound-designed against the Jove voice engine. Morph axis:
  0 = SINE, 0.25 = TRIANGLE, 0.5 = SAW, 1 = PULSE. Many patches lean on the mod
  matrix for movement (LFO->cutoff/detune), dynamics (velocity), FM bark (env->FM,
  the e-piano trick), ring-mod metallics and soft/hard osc sync. Indices are the
  stable contract with MIDI program-change and the preset store, so append —
  never reorder.
*/

#include "SynthPresets.h"

// Cold code: size-optimise so the 40 builders don't crowd the SRAM_EXEC region
// the hot DSP executes from.
#pragma GCC optimize("Os")

namespace jove
{
namespace
{
constexpr float SINE = 0.0f, TRI = 0.25f, SAW = 0.5f, PULSE = 1.0f;

void setName(SynthPatch& p, const char* nm) noexcept
{
    int i = 0;
    for(; nm[i] && i < 19; ++i)
        p.name[i] = nm[i];
    p.name[i] = '\0';
}

void mod(SynthPatch& p, int slot, ModSource s, ModDest d, float amt) noexcept
{
    if(slot < 0 || slot >= kNumModSlots)
        return;
    p.mod[slot].source = (int)s;
    p.mod[slot].dest   = (int)d;
    p.mod[slot].amount = amt;
}

// ============================ PAD (0-4) ============================
void pad_warm(SynthPatch& p)
{
    // Juno/Jupiter warm-saw poly. Two saws + sub through the Moog ladder, a gentle
    // filter-drive bloom for analog thickness, and a very slow filter-env swell.
    setName(p, "WARM ANALOG"); p.category = 0;
    p.width = 0.84f;
    p.ampGain = 0.74f; // trimmed for the two extra oscillators below
    p.chorusMode = 2; // Ensemble
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.11f; p.osc[1].level = 1.0f;
    p.osc[3].on = true; p.osc[3].morph = SAW; p.osc[3].detune = -0.07f; p.osc[3].level = 0.55f; // desktop width
    p.osc[4].on = true; p.osc[4].morph = SAW; p.osc[4].detune = 0.16f; p.osc[4].footage = 1; p.osc[4].level = 0.42f; // octave-down body
    p.oscMix = 0.5f; p.subLevel = 0.16f; p.drift = 0.28f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.38f; p.resonance = 0.10f;
    p.filterDrive = 0.22f; // 1.9x saturation -> Juno-ish body, not buzz
    p.envFilterAmt = 0.35f; p.keyTrack = 0.40f;
    p.env[0] = EnvParams{0.70f, 1.5f, 0.88f, 1.8f, 0.30f}; // soft attack, high sustain, velocity dynamics
    p.env[1] = EnvParams{1.40f, 1.8f, 0.55f, 1.6f, 0.0f}; // slow cutoff swell
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.20f; p.lfo[0].offset = 0.15f; // one-sided upward cutoff bloom
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.12f; // slow timbral axis
    mod(p, 0, ModSource::Lfo1,       ModDest::Cutoff,    0.12f);
    mod(p, 1, ModSource::ModWheel,   ModDest::Cutoff,    0.40f);
    mod(p, 2, ModSource::Lfo2,       ModDest::Detune,    0.18f);  // analog ensemble breathe
    mod(p, 3, ModSource::Lfo2,       ModDest::OscMix,    0.10f);  // slow two-saw balance drift
    mod(p, 4, ModSource::Aftertouch, ModDest::EnvFltAmt, 0.30f);  // press -> deeper swell
    p.fxChorus = 0.30f; p.fxPhaser = 0.20f; p.fxReverb = 0.32f; p.fxDelay = 0.08f;
}
void pad_strings(SynthPatch& p)
{
    // Solina/string-machine: three saws spread around unison, animated by TWO
    // LFOs on Detune at different rates so the ensemble is never static.
    setName(p, "STRINGS"); p.category = 0;
    p.width = 0.88f;
    p.ampGain = 0.66f; // five saws -> trim level
    p.chorusMode = 2; // Ensemble
    p.osc[0].morph = SAW;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.13f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = -0.11f; p.osc[2].level = 0.70f;
    p.osc[3].on = true; p.osc[3].morph = SAW; p.osc[3].detune = 0.21f; p.osc[3].level = 0.6f;  // wider beating
    p.osc[4].on = true; p.osc[4].morph = SAW; p.osc[4].detune = -0.19f; p.osc[4].level = 0.6f;
    p.oscMix = 0.5f; p.drift = 0.35f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.56f; p.resonance = 0.12f;
    p.envFilterAmt = 0.20f;
    p.env[0] = EnvParams{0.60f, 1.6f, 0.92f, 2.0f, 0.25f}; // velocity dynamics
    p.env[1] = EnvParams{1.20f, 1.8f, 0.60f, 1.6f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.28f; // detune animator A
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.19f; // detune animator B
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.13f; // slow swell
    mod(p, 0, ModSource::Lfo1,     ModDest::Detune, 0.50f);  // living ensemble motion
    mod(p, 1, ModSource::Lfo2,     ModDest::Detune, 0.30f);  // 2nd rate -> richer beating
    mod(p, 2, ModSource::Lfo1,     ModDest::Pitch,  0.008f); // gentle global shimmer
    mod(p, 3, ModSource::Lfo3,     ModDest::Cutoff, 0.12f);  // slow filter swell
    mod(p, 4, ModSource::ModWheel, ModDest::Cutoff, 0.50f);
    p.fxChorus = 0.70f; p.fxReverb = 0.42f; p.fxDelay = 0.05f;
}
void pad_glass(SynthPatch& p)
{
    // Crystalline FM-tine bell-pad: a sine carrier FM'd 2:1, soft-synced osc2 for a
    // glassy edge, and a shimmer of ring-mod that an LFO animates. Bright + airy.
    setName(p, "GLASS PAD"); p.category = 0;
    p.width = 0.80f;
    p.ampGain = 0.92f;
    p.chorusMode = 2; // Ensemble
    p.osc[0].morph = SINE;
    p.osc[1].morph = SINE; p.osc[1].footage = 3; p.osc[1].detune = 0.04f; // 2:1 tine
    p.oscMix = 0.0f;            // osc1 is the carrier; osc2 is modulator/sync source
    p.fm2to1 = 0.12f;
    p.sync2Mode = (int)SyncMode::Soft; // gentle glassy harmonic edge
    p.ringMod = 0.12f;                 // faint metallic crystal shimmer
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.72f; p.resonance = 0.10f;
    p.envFilterAmt = 0.20f;
    p.env[0] = EnvParams{1.00f, 2.0f, 0.82f, 2.4f, 0.28f}; // slow bloom, long tail, velocity dynamics
    p.env[2] = EnvParams{0.60f, 1.2f, 0.0f,  1.0f, 0.0f}; // aux fades the tine in
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 0.16f; // morph shimmer
    p.lfo[1].wave = (int)LfoWave::Sine;     p.lfo[1].rate = 0.10f; // slow drift/swell
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.23f; // ring shimmer
    mod(p, 0, ModSource::Lfo1,     ModDest::Morph1,   0.12f);
    mod(p, 1, ModSource::Lfo3,     ModDest::RingMod,  0.15f);  // animated metallic glint
    mod(p, 2, ModSource::Lfo2,     ModDest::Cutoff,   0.12f);  // airy swell
    mod(p, 3, ModSource::Lfo2,     ModDest::Pitch,    0.004f); // barely-there drift
    mod(p, 4, ModSource::EnvAux,   ModDest::FmAmount, 0.30f);  // soft tine on attack
    mod(p, 5, ModSource::ModWheel, ModDest::FmAmount, 0.30f);  // wheel adds brilliance
    p.fxChorus = 0.50f; p.fxReverb = 0.55f; p.fxDelay = 0.18f;
}
void pad_vox(SynthPatch& p)
{
    // Choir/vowel pad: two pulses through a band-pass "formant", each PWM'd by its
    // own LFO so the two move apart into a breathing ensemble; a slow LFO sweeps the
    // band like a vowel morph, and aftertouch sharpens the formant.
    setName(p, "VOX PAD"); p.category = 0;
    p.width = 0.80f;
    p.ampGain = 0.88f;
    p.chorusMode = 2; // Ensemble
    p.osc[0].morph = PULSE; p.osc[0].pw = 0.50f;
    p.osc[1].morph = PULSE; p.osc[1].pw = 0.42f; p.osc[1].detune = 0.07f;
    p.oscMix = 0.5f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.50f; p.resonance = 0.35f;
    p.envFilterAmt = 0.15f;
    p.env[0] = EnvParams{0.80f, 1.2f, 0.90f, 1.6f, 0.30f}; // velocity dynamics
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.50f; // PW1 mover
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.33f; // PW2 mover (offset rate)
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.14f; // vowel formant sweep
    mod(p, 0, ModSource::Lfo1,       ModDest::Pw1,       0.28f);
    mod(p, 1, ModSource::Lfo2,       ModDest::Pw2,       0.28f);  // 2nd pulse drifts apart
    mod(p, 2, ModSource::Lfo3,       ModDest::Cutoff,    0.18f);  // slow vowel morph
    mod(p, 3, ModSource::ModWheel,   ModDest::Cutoff,    0.30f);
    mod(p, 4, ModSource::Aftertouch, ModDest::Resonance, 0.20f);  // press -> more vocal
    p.fxChorus = 0.45f; p.fxReverb = 0.50f; p.fxDelay = 0.05f;
}
void pad_unison(SynthPatch& p)
{
    // Massive 7-voice detuned saw wall. ampGain low because unison sums voices; the
    // ladder + filter-drive keep the wall thick, with aftertouch adding bite.
    setName(p, "UNISON PAD"); p.category = 0;
    p.width = 0.86f;
    p.ampGain = 0.70f; // unison stacks 7 voices that phase-cancel; needs the makeup
    p.chorusMode = 2;  // Ensemble
    p.voiceMode = (int)VoiceMode::Unison; p.unisonCount = 7; p.unisonDetune = 0.28f; p.unisonSpread = 0.85f;
    p.osc[0].morph = SAW; p.osc[1].morph = SAW; p.osc[1].detune = 0.04f;
    p.oscMix = 0.5f; p.subLevel = 0.10f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.34f; p.resonance = 0.18f;
    p.filterDrive = 0.20f;
    p.envFilterAmt = 0.40f;
    p.env[0] = EnvParams{0.70f, 1.8f, 0.85f, 1.8f, 0.30f}; // velocity dynamics
    p.env[1] = EnvParams{1.50f, 2.0f, 0.35f, 1.6f, 0.0f}; // slow swell up through the wall
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 0.10f;
    p.lfo[1].wave = (int)LfoWave::Sine;     p.lfo[1].rate = 0.07f;
    mod(p, 0, ModSource::Lfo1,       ModDest::Cutoff,      0.26f);
    mod(p, 1, ModSource::Lfo2,       ModDest::Pitch,       0.006f); // unison breathe
    mod(p, 2, ModSource::Lfo2,       ModDest::OscMix,      0.12f);  // slow timbral drift
    mod(p, 3, ModSource::ModWheel,   ModDest::Cutoff,      0.40f);
    mod(p, 4, ModSource::Aftertouch, ModDest::FilterDrive, 0.30f);  // press -> more grit
    p.fxChorus = 0.40f; p.fxReverb = 0.45f; p.fxDelay = 0.18f;
}

// ============================ LEAD (5-9) ============================
void lead_classic(SynthPatch& p)
{
    setName(p, "CLASSIC LEAD"); p.category = 1;
    p.width = 0.45f; // leads sit mostly center
    p.voiceMode = (int)VoiceMode::Mono;
    p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.05f;
    // Two saws, second a hair sharp for the fat Minimoog-style detune.
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.06f; p.osc[1].level = 1.0f;
    p.oscMix = 0.5f; p.subLevel = 0.12f; p.drift = 0.25f; // sub adds weight under the lead
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.6f; p.resonance = 0.22f;
    p.filterDrive = 0.12f; // gentle ladder push so loud notes thicken
    p.envFilterAmt = 0.42f; p.keyTrack = 0.45f;
    p.env[0] = EnvParams{0.005f, 0.4f, 0.85f, 0.3f, 0.15f}; // amp: snappy, velocity opens level
    p.env[1] = EnvParams{0.006f, 0.32f, 0.4f, 0.28f, 0.0f}; // filter sweep on each note
    // Mod-wheel vibrato: Lfo1 depth starts at 0 and the wheel brings it up.
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 5.5f; p.lfo[0].depth = 0.0f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.22f; // slow timbral drift on holds
    mod(p, 0, ModSource::ModWheel, ModDest::Lfo1Depth, 1.0f);
    mod(p, 1, ModSource::Lfo1, ModDest::Pitch, 0.013f);    // ~+/-18 cents at full wheel
    mod(p, 2, ModSource::Lfo2, ModDest::Cutoff, 0.08f);    // movement on sustained notes
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.2f); // harder = brighter
    mod(p, 4, ModSource::KeyTrack, ModDest::Cutoff, 0.15f);// keep top end alive up high
    p.macroDest[0] = (int)MacroDest::Cutoff;
    p.macroDest[1] = (int)MacroDest::Lfo1Depth; // manual vibrato pot if no wheel
    p.macroDest[2] = (int)MacroDest::Delay;
    p.ampGain = 0.8f; p.fxDelay = 0.25f; p.fxReverb = 0.2f;
}
void lead_sync(SynthPatch& p)
{
    setName(p, "SYNC LEAD"); p.category = 1;
    p.width = 0.4f;
    p.voiceMode = (int)VoiceMode::Mono;
    p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.03f;
    p.sync2Mode = (int)SyncMode::Hard; // osc2 hard-synced to osc1 -> the classic tear
    // osc2 is the synced slave swept far above the master; only its hardsync
    // formant is heard, so its absolute pitch sets the scream's brightness.
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 7.0f; p.osc[1].level = 1.0f;
    p.oscMix = 0.55f; // bias toward the synced slave for more bite
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.72f; p.resonance = 0.18f;
    p.filterDrive = 0.2f; // grind for the aggressive screamer voicing
    p.envFilterAmt = 0.25f;
    p.env[0] = EnvParams{0.005f, 0.5f, 0.72f, 0.3f, 0.3f};  // amp
    p.env[1] = EnvParams{0.01f, 0.35f, 0.5f, 0.3f, 0.0f};   // filter
    p.env[2] = EnvParams{0.01f, 0.7f, 0.0f, 0.3f, 0.0f};    // aux: the sweeping sync-tear shape
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 5.0f; p.lfo[0].depth = 0.0f;
    mod(p, 0, ModSource::EnvAux, ModDest::Osc2Pitch, 0.8f);    // per-note sync sweep down
    mod(p, 1, ModSource::ModWheel, ModDest::Osc2Pitch, 0.45f); // wheel rides the tear live
    mod(p, 2, ModSource::Velocity, ModDest::EnvFltAmt, 0.3f);  // harder hits sweep wider
    mod(p, 3, ModSource::ModWheel, ModDest::Lfo1Depth, 1.0f);
    mod(p, 4, ModSource::Lfo1, ModDest::Osc2Pitch, 0.02f);    // wheel-in vibrato on the formant
    p.macroDest[0] = (int)MacroDest::Cutoff;
    p.macroDest[1] = (int)MacroDest::FilterEnv;
    p.macroDest[2] = (int)MacroDest::Drive;
    p.ampGain = 0.78f; p.fxDelay = 0.3f; p.fxReverb = 0.15f;
}
void lead_soft(SynthPatch& p)
{
    setName(p, "SOFT LEAD"); p.category = 1;
    p.width = 0.55f;
    p.voiceMode = (int)VoiceMode::Mono;
    p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.08f;
    // Mellow: a triangle over a near-sine partner, mixed soft, no resonance bite.
    p.osc[0].morph = TRI; p.osc[0].level = 1.0f;
    p.osc[1].on = true; p.osc[1].morph = 0.1f; p.osc[1].detune = 0.04f; p.osc[1].level = 0.7f; // sine-ish warmth
    p.oscMix = 0.4f; p.drift = 0.2f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.68f; p.resonance = 0.05f;
    p.envFilterAmt = 0.2f; p.keyTrack = 0.4f;
    p.envFilterAmt = 0.3f; // gives velocity something to open
    p.env[0] = EnvParams{0.02f, 0.4f, 0.85f, 0.45f, 0.28f}; // velocity -> level for dynamics
    p.env[1] = EnvParams{0.04f, 0.5f, 0.6f, 0.4f, 0.0f};
    // Delayed vibrato: silent ~0.45 s, then wobble fades in over ~0.9 s.
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 5.3f; p.lfo[0].depth = 1.0f;
    p.lfo[0].delay = 0.45f; p.lfo[0].fade = 0.9f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.18f;
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.012f);   // ~+/-17 cents, auto delayed vibrato
    mod(p, 1, ModSource::ModWheel, ModDest::OscMix, 0.3f);// wheel brings the soft partner in
    mod(p, 2, ModSource::Lfo2, ModDest::Cutoff, 0.07f);   // dreamy slow shimmer
    mod(p, 3, ModSource::Aftertouch, ModDest::Lfo1Depth, 0.4f); // press for extra vibrato
    mod(p, 4, ModSource::Velocity, ModDest::Cutoff, 0.3f);     // softer touch = mellower
    p.macroDest[0] = (int)MacroDest::Cutoff;
    p.macroDest[1] = (int)MacroDest::Phaser;
    p.macroDest[2] = (int)MacroDest::Reverb;
    p.ampGain = 0.78f; p.fxChorus = 0.25f; p.fxPhaser = 0.3f; p.fxDelay = 0.28f; p.fxReverb = 0.32f;
}
void lead_super(SynthPatch& p)
{
    setName(p, "SUPER SAW"); p.category = 1;
    p.width = 0.75f;
    p.chorusMode = 2; // Ensemble widens the stack further
    // 5 detuned saw voices stacked: lower ampGain because the voices sum.
    p.voiceMode = (int)VoiceMode::Unison;
    p.unisonCount = 5; p.unisonDetune = 0.2f; p.unisonSpread = 0.8f;
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.04f; p.osc[1].level = 1.0f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = -0.05f; p.osc[2].level = 0.9f; // 7-saw stack
    p.osc[3].on = true; p.osc[3].morph = SAW; p.osc[3].detune = 0.10f; p.osc[3].level = 0.8f;
    p.oscMix = 0.5f; p.subLevel = 0.1f; p.drift = 0.3f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.62f; p.resonance = 0.1f;
    p.envFilterAmt = 0.3f; p.keyTrack = 0.5f;
    p.env[0] = EnvParams{0.01f, 0.6f, 0.85f, 0.5f, 0.0f}; // smooth trance-lead amp
    p.env[1] = EnvParams{0.03f, 0.5f, 0.55f, 0.45f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 0.18f; p.lfo[0].offset = 0.12f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.11f;
    mod(p, 0, ModSource::ModWheel, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.08f);    // slow filter drift
    mod(p, 2, ModSource::Lfo2, ModDest::Detune, 0.25f);    // animated supersaw ensemble breathe
    mod(p, 3, ModSource::ModWheel, ModDest::Detune, 0.3f); // wheel widens the spread
    mod(p, 4, ModSource::Velocity, ModDest::Cutoff, 0.2f);
    p.macroDest[0] = (int)MacroDest::Cutoff;
    p.macroDest[1] = (int)MacroDest::Reverb;
    p.macroDest[2] = (int)MacroDest::Delay;
    p.ampGain = 0.42f; p.fxChorus = 0.35f; p.fxReverb = 0.32f; p.fxDelay = 0.24f; // 7-saw stack -> lower gain
}
void lead_acidld(SynthPatch& p)
{
    setName(p, "ACID LEAD"); p.category = 1;
    p.width = 0.4f;
    p.voiceMode = (int)VoiceMode::Mono;
    p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.06f;
    p.sync3Mode = (int)SyncMode::Soft; // osc3 soft-synced for extra squelch edge
    // Single screaming saw + a soft-synced osc3 harmonic shimmer; high reso + drive.
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].on = false;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = 0.0f; p.osc[2].level = 0.4f;
    p.oscMix = 0.0f; p.drift = 0.2f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.42f; p.resonance = 0.6f;
    p.filterDrive = 0.35f; // grit that self-oscillating resonance feeds into
    p.envFilterAmt = 0.55f; p.keyTrack = 0.35f;
    p.env[0] = EnvParams{0.003f, 0.4f, 0.6f, 0.2f, 0.0f};  // amp
    p.env[1] = EnvParams{0.003f, 0.28f, 0.1f, 0.18f, 0.0f};// fast snappy filter pluck -> the squelch
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 5.5f; p.lfo[0].depth = 0.0f;
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);  // env drives the acid sweep
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.5f);    // wheel rides the filter live
    mod(p, 2, ModSource::Velocity, ModDest::EnvFltAmt, 0.4f); // accent = deeper squelch
    mod(p, 3, ModSource::Velocity, ModDest::FilterDrive, 0.3f);// accent adds grit
    mod(p, 4, ModSource::ModWheel, ModDest::Resonance, 0.2f); // wheel pushes toward scream
    mod(p, 5, ModSource::ModWheel, ModDest::Lfo1Depth, 1.0f);
    mod(p, 6, ModSource::Lfo1, ModDest::Pitch, 0.01f);       // optional wheel-in vibrato
    p.macroDest[0] = (int)MacroDest::Cutoff;
    p.macroDest[1] = (int)MacroDest::Resonance;
    p.macroDest[2] = (int)MacroDest::Drive;
    p.ampGain = 0.82f; p.fxDelay = 0.3f; p.fxReverb = 0.2f;
}

// ============================ BASS (10-14) ============================
void bass_moog(SynthPatch& p)
{
    setName(p, "MOOG BASS"); p.category = 2;
    p.width = 0.4f;                                  // tight, centred low end
    p.voiceMode = (int)VoiceMode::Mono;
    // Two detuned saws an octave down + a square-ish sub for the round Moog weight.
    p.osc[0].morph = SAW; p.osc[0].footage = 1;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.03f; p.osc[1].footage = 1;
    p.oscMix = 0.5f; p.subLevel = 0.32f; p.subOctave = 1; p.drift = 0.22f;
    // Ladder LP with a little drive for body, moderate reso for the fat resonant hump.
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.32f; p.resonance = 0.30f;
    p.filterDrive = 0.2f; p.envFilterAmt = 0.55f; p.keyTrack = 0.35f;
    p.env[0] = EnvParams{0.004f, 0.30f, 0.78f, 0.18f, 0.15f}; // amp: punch, sustains
    p.env[1] = EnvParams{0.004f, 0.24f, 0.12f, 0.14f, 0.0f};  // filter: quick thump
    // Velocity opens the filter and deepens its envelope; key-track keeps highs alive.
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff,    0.22f);
    mod(p, 1, ModSource::Velocity, ModDest::EnvFltAmt, 0.25f);
    mod(p, 2, ModSource::KeyTrack, ModDest::Cutoff,    0.18f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff,    0.30f);
    p.ampGain = 0.9f;
}
void bass_acid(SynthPatch& p)
{
    setName(p, "ACID 303"); p.category = 2;
    p.width = 0.4f;
    p.voiceMode = (int)VoiceMode::Mono;
    p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.06f; // 303 slides
    // Single saw, octave down. Open the cutoff a touch so the body carries (RMS),
    // hot resonance + drive for the squelch.
    p.osc[0].morph = SAW; p.osc[0].footage = 1; p.osc[1].on = false; p.oscMix = 0.0f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.30f; p.resonance = 0.6f;
    p.filterDrive = 0.45f; p.envFilterAmt = 0.55f;
    p.env[0] = EnvParams{0.003f, 0.32f, 0.45f, 0.10f, 0.35f}; // amp: velocity -> level (accent)
    p.env[1] = EnvParams{0.003f, 0.24f, 0.0f,  0.10f, 0.0f};  // filter: snappy zap
    p.env[2] = EnvParams{0.003f, 0.16f, 0.0f,  0.08f, 0.0f};  // aux: accent transient
    // EnvFilter + EnvAux both push cutoff (the percussive squelch), accent drives
    // grit, mod-wheel sweeps the filter live, and VELOCITY = the 303 accent:
    // harder notes open brighter, saturate more, ring the resonance harder.
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff,      0.30f);
    mod(p, 1, ModSource::EnvAux,    ModDest::Cutoff,      0.30f);
    mod(p, 2, ModSource::EnvAux,    ModDest::FilterDrive, 0.25f);
    mod(p, 3, ModSource::ModWheel,  ModDest::Cutoff,      0.5f);
    mod(p, 4, ModSource::Velocity,  ModDest::Resonance,   0.2f);
    mod(p, 5, ModSource::Velocity,  ModDest::Cutoff,      0.35f); // accent = brighter
    mod(p, 6, ModSource::Velocity,  ModDest::FilterDrive, 0.30f); // accent = grittier
    p.ampGain = 0.9f; p.fxDelay = 0.18f;
}
void bass_sub(SynthPatch& p)
{
    setName(p, "SUB BASS"); p.category = 2;
    p.width = 0.35f;                                 // narrowest: pure mono weight
    p.voiceMode = (int)VoiceMode::Mono;
    // Pure sine fundamental + a hint of square sub, then low-passed hard so no odd
    // harmonics ever buzz. Felt, not heard.
    p.osc[0].morph = SINE; p.osc[0].footage = 1; p.osc[1].on = false; p.oscMix = 0.0f;
    p.subLevel = 0.25f; p.subOctave = 1; p.drift = 0.1f;
    // cutoff high enough that the fundamental passes at any played octave (a sine
    // has no harmonics to buzz, so opening it just restores level, stays clean)
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.32f; p.resonance = 0.03f;
    p.envFilterAmt = 0.0f;                           // no filter movement: stays clean
    p.env[0] = EnvParams{0.006f, 0.30f, 0.92f, 0.16f, 0.0f}; // soft attack, full hold
    // Just a little velocity dynamics — nothing that adds harmonics.
    mod(p, 0, ModSource::Velocity, ModDest::Amp, 0.2f);
    p.ampGain = 1.0f;                                // a pure sine is quiet
}
void bass_fm(SynthPatch& p)
{
    setName(p, "FM BASS"); p.category = 2;
    p.width = 0.4f;
    p.voiceMode = (int)VoiceMode::Mono;
    // 2:1 sine-on-sine FM for the funky growl; a touch of ring-mod for metallic edge.
    p.fm2to1 = 0.25f; p.ringMod = 0.1f;
    p.osc[0].morph = SINE;
    p.osc[1].morph = SINE; p.osc[1].footage = 3; p.osc[1].detune = 0.01f; // 2:1 ratio
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.45f; p.resonance = 0.25f;
    p.filterDrive = 0.22f;                           // grit on the FM tone
    p.env[0] = EnvParams{0.003f, 0.30f, 0.6f, 0.15f, 0.1f};  // amp
    p.env[2] = EnvParams{0.003f, 0.16f, 0.0f, 0.10f, 0.0f};  // aux: the FM "bark"
    // Aux env barks the FM index; velocity adds bark + opens filter; wheel adds clang.
    mod(p, 0, ModSource::EnvAux,   ModDest::FmAmount, 0.7f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.4f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff,   0.22f);
    mod(p, 3, ModSource::ModWheel, ModDest::RingMod,  0.3f);
    p.ampGain = 0.85f; p.fxDrive = 0.12f;
}
void bass_pluck(SynthPatch& p)
{
    setName(p, "PLUCKED BASS"); p.category = 2;
    p.width = 0.42f;
    p.voiceMode = (int)VoiceMode::Mono;
    // Saw + narrow pulse, octave down — bright transient that decays fully to silence.
    p.osc[0].morph = SAW;   p.osc[0].footage = 1;
    p.osc[1].morph = PULSE; p.osc[1].pw = 0.4f; p.osc[1].detune = 0.02f; p.osc[1].footage = 1;
    p.oscMix = 0.45f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.35f; p.resonance = 0.35f;
    p.filterDrive = 0.1f; p.envFilterAmt = 0.65f;
    p.env[0] = EnvParams{0.002f, 0.18f, 0.0f, 0.10f, 0.25f}; // amp: fast pluck to zero
    p.env[1] = EnvParams{0.002f, 0.12f, 0.0f, 0.08f, 0.0f};  // filter: faster decay
    // Velocity drives both brightness and filter-env depth; key-track keeps top notes
    // from going dull; mod-wheel balances the two oscs (osc1<->osc2 timbre shift).
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff,    0.30f);
    mod(p, 1, ModSource::Velocity, ModDest::EnvFltAmt, 0.22f);
    mod(p, 2, ModSource::KeyTrack, ModDest::Cutoff,    0.20f);
    mod(p, 3, ModSource::ModWheel, ModDest::OscMix,    0.3f);
    p.ampGain = 0.9f;
}

// ============================ ARP (15-19) ============================
void arp_saw(SynthPatch& p)
{
    setName(p, "SAW ARP"); p.category = 3;
    p.width = 0.55f; p.ampGain = 0.80f;
    // two bright saws + a sub octave for body under the fast Up steps
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.06f;
    p.oscMix = 0.5f; p.subLevel = 0.18f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.46f; p.resonance = 0.34f;
    p.filterDrive = 0.22f; p.envFilterAmt = 0.6f; p.keyTrack = 0.4f;
    // amp: short, tiny sustain so each step speaks then ducks
    p.env[0] = EnvParams{0.003f, 0.16f, 0.08f, 0.10f, 0.25f};
    // filter: snappy zero-sustain pluck per step
    p.env[1] = EnvParams{0.002f, 0.10f, 0.0f, 0.07f, 0.0f};
    // velocity opens the filter, deepens the env, and adds drive on accents
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff,      0.30f);
    mod(p, 1, ModSource::Velocity, ModDest::EnvFltAmt,   0.35f);
    mod(p, 2, ModSource::Velocity, ModDest::FilterDrive, 0.20f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::Up; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.45f; p.arp.swing = 0.0f; p.arp.ratchet = 1;
    p.fxDelay = 0.30f; p.fxReverb = 0.22f;
}
void arp_pluck(SynthPatch& p)
{
    setName(p, "PLUCK ARP"); p.category = 3;
    p.width = 0.6f; p.ampGain = 0.82f;
    // saw + narrow pulse, soft-synced for a glassy plucked edge
    p.osc[0].morph = SAW; p.osc[1].morph = PULSE; p.osc[1].pw = 0.28f; p.osc[1].detune = 0.04f;
    p.oscMix = 0.45f; p.sync2Mode = (int)SyncMode::Soft;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.5f; p.resonance = 0.28f;
    p.envFilterAmt = 0.65f; p.keyTrack = 0.45f;
    // very fast decay -> tight pluck on every step
    p.env[0] = EnvParams{0.001f, 0.09f, 0.0f, 0.06f, 0.3f};
    p.env[1] = EnvParams{0.001f, 0.07f, 0.0f, 0.05f, 0.0f};
    // slow LFO breathes the osc balance + a hint of detune drift for life
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 0.2f;
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    mod(p, 1, ModSource::Lfo1,     ModDest::OscMix, 0.20f);
    mod(p, 2, ModSource::Lfo1,     ModDest::Detune, 0.01f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::UpDown; p.arp.octaves = 2;
    p.arp.syncDiv = 5; p.arp.gate = 0.35f; p.arp.swing = 0.20f; p.arp.ratchet = 1;
    p.fxChorus = 0.28f; p.fxDelay = 0.34f; p.fxReverb = 0.28f;
}
void arp_bell(SynthPatch& p)
{
    setName(p, "BELL ARP"); p.category = 3;
    p.width = 0.6f; p.ampGain = 0.78f;
    // 2-op FM bell: sine carrier + sine modulator up at 2', ring-mod clang on top
    p.osc[0].morph = SINE; p.osc[1].morph = SINE; p.osc[1].footage = 4;
    p.oscMix = 0.35f; p.fm2to1 = 0.28f; p.ringMod = 0.20f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.72f; p.resonance = 0.06f;
    // long-ish bell decay on amp; fast aux env is the FM strike transient
    p.env[0] = EnvParams{0.001f, 0.45f, 0.0f, 0.30f, 0.35f};
    p.env[2] = EnvParams{0.001f, 0.11f, 0.0f, 0.09f, 0.0f};
    mod(p, 0, ModSource::EnvAux,   ModDest::FmAmount, 0.55f); // bark on each strike
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.45f); // harder = brighter
    mod(p, 2, ModSource::Velocity, ModDest::RingMod,  0.30f); // harder = more clang
    p.arp.on = true; p.arp.mode = (int)ArpMode::UpDownInc; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.60f; p.arp.swing = 0.0f; p.arp.ratchet = 1;
    p.fxDelay = 0.36f; p.fxReverb = 0.42f;
}
void arp_random(SynthPatch& p)
{
    setName(p, "RANDOM ARP"); p.category = 3;
    p.width = 0.6f; p.ampGain = 0.80f;
    p.osc[0].morph = SAW; p.osc[1].morph = TRI; p.osc[1].footage = 3; p.osc[1].detune = 0.04f;
    p.oscMix = 0.5f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.48f; p.resonance = 0.38f;
    p.filterDrive = 0.18f; p.envFilterAmt = 0.55f;
    p.env[0] = EnvParams{0.002f, 0.13f, 0.0f, 0.09f, 0.25f};
    p.env[1] = EnvParams{0.002f, 0.10f, 0.0f, 0.07f, 0.0f};
    // S&H LFO jitters cutoff per step; RAND source scatters drive + pan
    p.lfo[0].wave = (int)LfoWave::SampleHold; p.lfo[0].rate = 7.0f;
    mod(p, 0, ModSource::Lfo1,   ModDest::Cutoff,      0.30f);
    mod(p, 1, ModSource::Random, ModDest::FilterDrive, 0.25f);
    mod(p, 2, ModSource::Random, ModDest::Pan,         0.40f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::Random; p.arp.octaves = 3;
    p.arp.syncDiv = 4; p.arp.gate = 0.50f; p.arp.swing = 0.0f; p.arp.ratchet = 2;
    p.fxDelay = 0.30f; p.fxReverb = 0.32f;
}
void arp_pingpong(SynthPatch& p)
{
    setName(p, "PINGPONG ARP"); p.category = 3;
    p.width = 0.7f; p.ampGain = 0.80f;
    p.osc[0].morph = SAW; p.osc[1].morph = SAW; p.osc[1].detune = 0.08f;
    p.oscMix = 0.5f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.54f; p.resonance = 0.22f;
    p.envFilterAmt = 0.45f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{0.003f, 0.18f, 0.05f, 0.12f, 0.2f};
    p.env[1] = EnvParams{0.003f, 0.15f, 0.0f, 0.10f, 0.0f};
    // clock-synced triangle LFO swings the pan L<->R with the bouncing pattern
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 0.5f;
    p.lfo[0].sync = true; p.lfo[0].syncDiv = 6;
    mod(p, 0, ModSource::Lfo1,     ModDest::Pan,    0.70f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    mod(p, 2, ModSource::Lfo1,     ModDest::Cutoff, 0.10f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::PingPong; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.50f; p.arp.swing = 0.12f; p.arp.ratchet = 1;
    p.fxPhaser = 0.25f; p.fxDelay = 0.32f; p.fxReverb = 0.30f;
}

// ============================ STAB (20-24) ============================
void stab_house(SynthPatch& p)
{
    // Classic M1-organ/house piano stab: stacked saws + a bright sine bell
    // partial, punchy plucked amp, velocity opens the filter for dynamics.
    setName(p, "HOUSE STAB"); p.category = 4;
    p.osc[0].morph = SAW;  p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW;  p.osc[1].detune = 0.09f;
    p.osc[2].on = true; p.osc[2].morph = SINE; p.osc[2].footage = 4; p.osc[2].level = 0.45f; // 2' bell sparkle
    p.oscMix = 0.5f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.62f; p.resonance = 0.22f;
    p.envFilterAmt = 0.5f; p.keyTrack = 0.5f;
    p.env[0] = EnvParams{0.002f, 0.24f, 0.0f, 0.16f, 0.4f}; // punchy pluck
    p.env[1] = EnvParams{0.002f, 0.14f, 0.0f, 0.10f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff,    0.35f);
    mod(p, 1, ModSource::Velocity, ModDest::Amp,       0.25f);
    mod(p, 2, ModSource::Velocity, ModDest::EnvFltAmt, 0.2f);  // harder = bigger sweep
    mod(p, 3, ModSource::KeyTrack, ModDest::Cutoff,    0.15f); // brighter up the keys
    p.ampGain = 0.72f; p.fxReverb = 0.25f; p.fxDelay = 0.12f;
}
void stab_rave(SynthPatch& p)
{
    // Hoover/rave unison stab: detuned saw+pulse stack, driven ladder, a short
    // downward pitch-bark and animated detune. Lower gain since voices sum.
    setName(p, "RAVE STAB"); p.category = 4;
    p.width = 0.7f; p.ampGain = 0.5f;
    p.voiceMode = (int)VoiceMode::Unison; p.unisonCount = 5; p.unisonDetune = 0.2f; p.unisonSpread = 0.7f;
    p.osc[0].morph = SAW;   p.osc[1].morph = PULSE; p.osc[1].pw = 0.4f; p.osc[1].detune = 0.06f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].footage = 1; p.osc[2].level = 0.5f; // 16' weight
    p.oscMix = 0.5f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.5f; p.resonance = 0.3f; p.filterDrive = 0.35f;
    p.envFilterAmt = 0.5f;
    p.env[0] = EnvParams{0.003f, 0.22f, 0.0f, 0.18f, 0.3f};
    p.env[1] = EnvParams{0.003f, 0.18f, 0.0f, 0.12f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.13f, 0.0f, 0.10f, 0.0f}; // pitch-bark aux
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 0.3f;
    mod(p, 0, ModSource::EnvAux,   ModDest::Pitch,       -0.14f); // hoover down-sweep
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff,       0.3f);
    mod(p, 2, ModSource::Velocity, ModDest::FilterDrive,  0.2f);  // harder = grittier
    mod(p, 3, ModSource::Lfo1,     ModDest::Detune,       0.04f); // animated ensemble
    p.fxReverb = 0.3f; p.fxDelay = 0.2f;
}
void stab_brass(SynthPatch& p)
{
    // Synth-brass section stab: 3 saws + PWM body, slow filter-env attack swell,
    // velocity drives brightness, mod-wheel adds the section vibrato.
    setName(p, "BRASS STAB"); p.category = 4;
    p.width = 0.55f;
    p.osc[0].morph = SAW;   p.osc[1].morph = SAW; p.osc[1].detune = 0.07f;
    p.osc[2].on = true; p.osc[2].morph = PULSE; p.osc[2].pw = 0.42f; p.osc[2].level = 0.6f; // PWM body
    p.oscMix = 0.5f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.38f; p.resonance = 0.15f;
    p.envFilterAmt = 0.55f;
    p.env[0] = EnvParams{0.03f, 0.35f, 0.7f,  0.28f, 0.2f}; // brass swell
    p.env[1] = EnvParams{0.07f, 0.45f, 0.45f, 0.30f, 0.0f}; // slower filter attack
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 1.8f; // PWM motion
    p.lfo[1].wave = (int)LfoWave::Sine;     p.lfo[1].rate = 5.5f; p.lfo[1].depth = 0.0f; // vibrato (wheel)
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff,    0.4f);
    mod(p, 1, ModSource::Lfo1,     ModDest::Pw3,       0.2f);  // pulse-body PWM
    mod(p, 2, ModSource::ModWheel, ModDest::Lfo2Depth, 1.0f);
    mod(p, 3, ModSource::Lfo2,     ModDest::Pitch,     0.01f); // section vibrato
    p.ampGain = 0.7f; p.fxChorus = 0.3f; p.fxReverb = 0.25f;
}
void stab_organ(SynthPatch& p)
{
    // Drawbar-organ stab: stacked sine footages (16'/8'/4'), sustained while held,
    // Leslie-style chorus + sine vibrato, gentle velocity dynamics.
    setName(p, "ORGAN STAB"); p.category = 4;
    p.osc[0].morph = SINE; p.osc[0].footage = 2; p.osc[0].level = 1.0f;  // 8'
    p.osc[1].morph = SINE; p.osc[1].footage = 1; p.osc[1].level = 0.7f;  // 16' sub drawbar
    p.osc[2].on = true; p.osc[2].morph = SINE; p.osc[2].footage = 3; p.osc[2].detune = 0.04f; p.osc[2].level = 0.6f; // 4'
    p.oscMix = 0.5f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.72f; p.resonance = 0.05f;
    p.env[0] = EnvParams{0.003f, 0.08f, 0.85f, 0.12f, 0.0f}; // fast on, sustained
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 6.0f; // Leslie
    mod(p, 0, ModSource::Lfo1,     ModDest::Pitch, 0.006f); // chorale vibrato
    mod(p, 1, ModSource::Lfo1,     ModDest::Amp,   0.12f);  // Leslie tremolo
    mod(p, 2, ModSource::Velocity, ModDest::Amp,   0.2f);
    mod(p, 3, ModSource::KeyTrack, ModDest::Cutoff, 0.1f);
    p.ampGain = 0.75f; p.fxChorus = 0.5f; p.fxReverb = 0.22f;
}
void stab_pluckstab(SynthPatch& p)
{
    // Gated, snappy pluck stab: bright narrow pulse + saw, resonant SVF, very
    // short gate, everything velocity-reactive for a tight, alive hit.
    setName(p, "PLUCK STAB"); p.category = 4;
    p.osc[0].morph = PULSE; p.osc[0].pw = 0.3f; p.osc[1].morph = SAW; p.osc[1].detune = 0.05f;
    p.oscMix = 0.5f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.55f; p.resonance = 0.4f;
    p.envFilterAmt = 0.6f;
    p.env[0] = EnvParams{0.001f, 0.12f, 0.0f, 0.07f, 0.4f}; // snappy gate
    p.env[1] = EnvParams{0.001f, 0.09f, 0.0f, 0.06f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff,    0.4f);
    mod(p, 1, ModSource::Velocity, ModDest::Amp,       0.3f);
    mod(p, 2, ModSource::Velocity, ModDest::EnvFltAmt, 0.25f); // harder = deeper pluck
    mod(p, 3, ModSource::KeyTrack, ModDest::Cutoff,    0.2f);
    p.ampGain = 0.8f; p.fxDelay = 0.18f; p.fxReverb = 0.22f;
}

// ============================ KEYS / E-PIANO (25-29) ============================
void keys_rhodes(SynthPatch& p)
{
    setName(p, "RHODES MK1"); p.category = 5;
    // Mark-I tine e-piano: osc0 sine carrier FM'd 1:1 by osc1 (the tine), osc2 a
    // slightly-detuned sine "tone bar" so there's a real body beating under the
    // bell. Aux env gives the fast tine "bark", velocity sets how bright/loud the
    // tine sits, and a mellow SVF keeps soft notes dark & round. Mod-wheel adds
    // the classic ~5 Hz amp tremolo; a slow LFO drifts the body for chorus-y life.
    p.osc[0].morph = SINE;
    p.osc[1].morph = SINE; p.osc[1].footage = 2; p.osc[1].detune = 0.015f;      // 1:1 tine, slightly sharp -> inharmonic shimmer
    p.osc[2].on = true; p.osc[2].morph = SINE; p.osc[2].footage = 2;
    p.osc[2].detune = 0.04f; p.osc[2].level = 0.78f;                            // body bar — louder so the fundamental carries
    p.oscMix = 0.0f; p.fm2to1 = 0.08f; p.drift = 0.18f;
    p.noiseLevel = 0.0f;                                                        // the hammer "knock" is enveloped in below
    // Moog ladder (not SVF): its softSat feedback gives the warm overdrive that
    // velocity pushes into — hard hits bark, brighten AND saturate, soft hits stay
    // dark and round. That velocity-dependent grit is the heart of a real Rhodes.
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.38f; p.resonance = 0.08f;
    p.filterDrive = 0.18f;                                                      // base warmth
    p.keyTrack = 0.45f; p.envFilterAmt = 0.44f;                                 // filter spikes open at attack -> bright tine+knock, then mellows
    p.env[0] = EnvParams{0.003f, 1.7f, 0.16f, 0.55f, 0.5f};                     // bell decay
    p.env[1] = EnvParams{0.001f, 0.13f, 0.0f, 0.25f, 0.0f};                     // sharp filter spike (the bright tine attack)
    p.env[2] = EnvParams{0.001f, 0.10f, 0.0f, 0.09f, 0.0f};                     // FM bark + knock (very fast)
    mod(p, 0, ModSource::EnvAux,    ModDest::FmAmount,   0.6f);                 // tine transient
    mod(p, 1, ModSource::Velocity,  ModDest::FmAmount,   0.5f);                 // hard = brighter tine
    mod(p, 2, ModSource::Velocity,  ModDest::Cutoff,     0.35f);                // hard = open filter
    mod(p, 3, ModSource::KeyTrack,  ModDest::FmAmount,  -0.18f);                // taper tine up high
    mod(p, 4, ModSource::EnvAux,    ModDest::NoiseLevel, 0.22f);               // hammer KNOCK (filtered chiff at the strike)
    mod(p, 5, ModSource::Velocity,  ModDest::NoiseLevel, 0.14f);               // harder hits knock louder
    mod(p, 6, ModSource::ModWheel,  ModDest::Lfo1Depth,  1.0f);                // wheel arms tremolo
    mod(p, 7, ModSource::Lfo1,      ModDest::Amp,        0.25f);               // tremolo
    mod(p, 8, ModSource::Lfo2,      ModDest::Detune,     0.012f);              // slow body shimmer
    mod(p, 9, ModSource::Velocity,  ModDest::FilterDrive,0.45f);              // HARD hits -> ladder saturation/grit
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 4.8f; p.lfo[0].depth = 0.0f;
    p.lfo[1].wave = (int)LfoWave::Sine; p.lfo[1].rate = 0.18f;
    p.ampGain = 0.6f; p.width = 0.62f; p.chorusMode = 0;
    p.fxChorus = 0.45f; p.fxReverb = 0.26f; p.fxDelay = 0.07f;
}
void keys_wurli(SynthPatch& p)
{
    setName(p, "WURLITZER"); p.category = 5;
    // Reedier & barkier than the Rhodes: deeper FM index, reed grit from filter
    // drive that velocity pushes harder, the signature ~5.5 Hz amp tremolo, and a
    // phaser swoosh instead of chorus (chorus would just muddy the reed).
    p.osc[0].morph = SINE;
    p.osc[1].morph = SINE; p.osc[1].footage = 2;                               // 1:1 reed
    p.osc[2].on = true; p.osc[2].morph = SINE; p.osc[2].footage = 2;
    p.osc[2].detune = 0.06f; p.osc[2].level = 0.62f;                           // body — louder for weight
    p.oscMix = 0.0f; p.fm2to1 = 0.11f; p.drift = 0.2f;                          // less FM index -> reedy, not muddy bell
    p.noiseLevel = 0.0f;                                                        // reed "thunk" enveloped below
    // Moog ladder (not SVF): the SVF only pre-gains its input so driving it
    // hard-clips harshly across the whole band -> mud. The ladder soft-saturates
    // in its feedback, and a touch of key-track keeps the grit out of the low end.
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.46f; p.resonance = 0.06f;
    p.keyTrack = 0.5f; p.filterDrive = 0.14f; p.envFilterAmt = 0.30f;          // filter spikes open on the strike
    p.env[0] = EnvParams{0.003f, 1.3f, 0.2f, 0.42f, 0.5f};
    p.env[1] = EnvParams{0.001f, 0.16f, 0.0f, 0.22f, 0.0f};                     // fast filter pluck (attack spike)
    p.env[2] = EnvParams{0.001f, 0.1f, 0.0f, 0.09f, 0.0f};                      // fast reed bark + thunk
    mod(p, 0, ModSource::EnvAux,   ModDest::FmAmount,    0.5f);                // barkier than Rhodes, but not a slam to max
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount,    0.32f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff,      0.3f);
    mod(p, 3, ModSource::Velocity, ModDest::FilterDrive, 0.24f);               // hard = gentle ladder reed grit
    mod(p, 4, ModSource::EnvAux,   ModDest::NoiseLevel,  0.16f);               // reed THUNK at the strike
    mod(p, 5, ModSource::Velocity, ModDest::NoiseLevel,  0.12f);
    mod(p, 6, ModSource::ModWheel, ModDest::Lfo1Depth,   1.0f);
    mod(p, 7, ModSource::Lfo1,     ModDest::Amp,         0.16f);               // tremolo
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 5.5f; p.lfo[0].depth = 0.0f;
    p.ampGain = 0.62f; p.width = 0.5f;
    p.fxDrive = 0.0f; p.fxPhaser = 0.42f; p.fxReverb = 0.22f; p.fxChorus = 0.0f; // master drive removed (ladder provides the grit)
}
void keys_dx(SynthPatch& p)
{
    setName(p, "FM PIANO"); p.category = 5;
    // Bright '80s DX digital e-piano: 2:1 tine (osc1 an octave up) for the glassy
    // bell, a body sine an octave down for the recognizable FM-piano weight, a
    // brighter filter, and lush ensemble chorus. Velocity drives the bell hard;
    // a slow LFO shimmers the FM index and detunes the body for movement.
    p.osc[0].morph = SINE;
    p.osc[1].morph = SINE; p.osc[1].footage = 3;                               // 2:1 tine (4')
    p.osc[2].on = true; p.osc[2].morph = SINE; p.osc[2].footage = 1;
    p.osc[2].detune = 0.04f; p.osc[2].level = 0.35f;                           // sub body (16')
    p.oscMix = 0.0f; p.fm2to1 = 0.17f; p.drift = 0.15f;
    p.noiseLevel = 0.0f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.66f; p.resonance = 0.0f;
    p.keyTrack = 0.5f;
    p.env[0] = EnvParams{0.002f, 1.7f, 0.18f, 0.5f, 0.4f};
    p.env[2] = EnvParams{0.001f, 0.22f, 0.0f, 0.18f, 0.0f};                     // brighter bell decay
    mod(p, 0, ModSource::EnvAux,   ModDest::FmAmount,   0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount,   0.6f);                 // digital bell bite
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff,     0.25f);
    mod(p, 3, ModSource::ModWheel, ModDest::FmAmount,   0.18f);                // wheel = brightness
    mod(p, 4, ModSource::Lfo1,     ModDest::FmAmount,   0.06f);                // slow timbral shimmer
    mod(p, 5, ModSource::Lfo2,     ModDest::Detune,     0.01f);                // ensemble breathe
    mod(p, 6, ModSource::EnvAux,   ModDest::NoiseLevel, 0.09f);               // subtle digital key-click
    mod(p, 7, ModSource::Velocity, ModDest::NoiseLevel, 0.07f);
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 0.15f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.11f;
    p.ampGain = 0.58f; p.width = 0.72f; p.chorusMode = 2;                       // Ensemble
    p.fxChorus = 0.55f; p.fxReverb = 0.3f; p.fxDelay = 0.1f;
}
void keys_clav(SynthPatch& p)
{
    setName(p, "CLAV"); p.category = 5;
    // Funky Clavinet: two narrow-pulse oscs (one detuned) through a resonant SVF
    // band-pass for the hollow honk, a very snappy envelope, velocity that snaps
    // the cutoff/drive/filter-env for pick attack, mod-wheel auto-wah, and the
    // mandatory funk phaser (the "Higher Ground" move).
    p.osc[0].morph = PULSE; p.osc[0].pw = 0.28f;
    p.osc[1].morph = PULSE; p.osc[1].pw = 0.34f; p.osc[1].detune = 0.05f;       // detuned twang
    p.osc[2].on = false; p.oscMix = 0.45f; p.drift = 0.25f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.55f; p.resonance = 0.32f;
    p.keyTrack = 0.7f; p.envFilterAmt = 0.4f; p.filterDrive = 0.1f;
    p.env[0] = EnvParams{0.002f, 0.24f, 0.18f, 0.13f, 0.4f};                    // snappy
    p.env[1] = EnvParams{0.002f, 0.18f, 0.0f, 0.1f, 0.0f};                      // pluck pop
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff,      0.4f);
    mod(p, 1, ModSource::Velocity, ModDest::FilterDrive, 0.3f);                // hard = bite
    mod(p, 2, ModSource::Velocity, ModDest::EnvFltAmt,   0.3f);                // hard = snappier pop
    mod(p, 3, ModSource::ModWheel, ModDest::Lfo1Depth,   1.0f);                // wheel arms wah
    mod(p, 4, ModSource::Lfo1,     ModDest::Cutoff,      0.3f);                // auto-wah
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 3.2f; p.lfo[0].depth = 0.0f;
    p.ampGain = 0.6f; p.width = 0.45f;
    p.fxPhaser = 0.45f; p.fxDelay = 0.12f; p.fxReverb = 0.14f; p.fxChorus = 0.0f;
}
void keys_poly(SynthPatch& p)
{
    setName(p, "POLY KEYS"); p.category = 5;
    // Bright poly-synth pop keys for comping/chords: three detuned saw/pulse oscs,
    // a wide ensemble chorus, LFO-animated detune for a living stack, velocity that
    // opens the filter, and a slow shimmer LFO. Punchy filter env gives each chord
    // a soft pluck so block chords speak clearly.
    p.osc[0].morph = SAW;  p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW;  p.osc[1].detune = 0.07f; p.osc[1].level = 1.0f;
    p.osc[2].on = true; p.osc[2].morph = PULSE; p.osc[2].pw = 0.45f;
    p.osc[2].detune = -0.06f; p.osc[2].level = 0.6f;
    p.oscMix = 0.5f; p.subLevel = 0.1f; p.drift = 0.3f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.6f; p.resonance = 0.13f;
    p.keyTrack = 0.5f; p.envFilterAmt = 0.35f;
    p.env[0] = EnvParams{0.005f, 0.6f, 0.6f, 0.42f, 0.3f};
    p.env[1] = EnvParams{0.008f, 0.4f, 0.3f, 0.35f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff, 0.3f);                     // dynamic brightness
    mod(p, 1, ModSource::Lfo1,     ModDest::Detune, 0.02f);                    // animated ensemble
    mod(p, 2, ModSource::Lfo2,     ModDest::Cutoff, 0.07f);                    // slow shimmer
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.3f);                     // wheel opens up
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 0.28f;
    p.lfo[1].wave = (int)LfoWave::Sine; p.lfo[1].rate = 0.14f;
    p.ampGain = 0.56f; p.width = 0.8f; p.chorusMode = 2;                        // Ensemble
    p.fxChorus = 0.45f; p.fxReverb = 0.3f; p.fxDelay = 0.08f;
}

// ============================ PLUCK (30-34) ============================
void pluck_synth(SynthPatch& p)
{
    setName(p, "NEON PLUCK"); p.category = 6;
    // Deep-house synth pluck: twin detuned saws, snappy resonant ladder env,
    // a touch of drive for body, sync'd delay + width for the wide hook.
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.10f; p.osc[1].level = 1.0f;
    p.oscMix = 0.5f; p.subLevel = 0.18f; // sub adds low-end weight
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.38f; p.resonance = 0.40f;
    p.filterDrive = 0.25f; p.envFilterAmt = 0.70f; p.keyTrack = 0.45f;
    p.env[0] = EnvParams{0.002f, 0.22f, 0.0f, 0.14f, 0.35f}; // amp: fast snap to silence
    p.env[1] = EnvParams{0.002f, 0.13f, 0.0f, 0.09f, 0.0f};  // filter: snappier than amp
    mod(p, 0, ModSource::Velocity, ModDest::EnvFltAmt, 0.45f); // harder = brighter sweep
    mod(p, 1, ModSource::Velocity, ModDest::FilterDrive, 0.30f);
    mod(p, 2, ModSource::KeyTrack, ModDest::Cutoff, 0.20f);    // upper notes stay bright
    p.ampGain = 0.78f; p.width = 0.7f;
    p.fxChorus = 0.25f; p.fxDelay = 0.34f; p.fxReverb = 0.22f;
}
void pluck_koto(SynthPatch& p)
{
    setName(p, "KOTO"); p.category = 6;
    // Plucked koto: two slightly-detuned triangles for woody body + beating,
    // a noise pick transient, and an LFO-animated detune for the string shimmer.
    p.osc[0].morph = TRI; p.osc[0].level = 1.0f;
    p.osc[1].morph = TRI; p.osc[1].detune = 0.07f; p.osc[1].level = 0.85f; // body + beat
    p.oscMix = 0.45f; p.noiseLevel = 0.07f; // pick attack
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.56f; p.resonance = 0.22f;
    p.envFilterAmt = 0.55f; p.keyTrack = 0.6f;
    p.env[0] = EnvParams{0.001f, 0.45f, 0.0f, 0.22f, 0.4f}; // long-ish string decay
    p.env[1] = EnvParams{0.001f, 0.16f, 0.0f, 0.11f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    mod(p, 1, ModSource::EnvFilter, ModDest::Detune, 0.04f); // pluck splays detune then settles
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 5.5f; p.lfo[0].depth = 1.0f;
    mod(p, 2, ModSource::Lfo1, ModDest::Detune, 0.015f);     // gentle string shimmer
    p.ampGain = 0.78f; p.width = 0.55f;
    p.fxDelay = 0.22f; p.fxReverb = 0.34f;
}
void pluck_bell(SynthPatch& p)
{
    setName(p, "BELL PLUCK"); p.category = 6;
    // FM bell pluck with ring-mod clang: sine carrier, high sine modulator (2'),
    // FM tine from the aux env, ring product whose depth tracks velocity so hard
    // hits clang metallically. Both oscs ON (ring = osc0 x osc1).
    p.osc[0].morph = SINE; p.osc[0].on = true; p.osc[0].level = 1.0f;
    p.osc[1].morph = SINE; p.osc[1].on = true; p.osc[1].footage = 4; p.osc[1].level = 1.0f; // 2' clang
    p.oscMix = 0.0f; p.fm2to1 = 0.28f; p.ringMod = 0.22f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.80f; p.resonance = 0.0f;
    p.env[0] = EnvParams{0.001f, 0.62f, 0.0f, 0.42f, 0.4f}; // bell-like decay to silence
    p.env[2] = EnvParams{0.001f, 0.18f, 0.0f, 0.14f, 0.0f}; // aux: fast FM bark transient
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.6f);   // tine bark on attack
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.45f);
    mod(p, 2, ModSource::Velocity, ModDest::RingMod, 0.35f); // harder = more clang
    mod(p, 3, ModSource::EnvAux, ModDest::RingMod, 0.25f);   // clang concentrated at attack
    p.ampGain = 0.74f; p.width = 0.6f;
    p.fxReverb = 0.42f; p.fxDelay = 0.26f;
}
void pluck_mallet(SynthPatch& p)
{
    setName(p, "MALLET"); p.category = 6;
    // Marimba/mallet: warm sine bar + a high sine partial ring-modded for the
    // woody-metallic bar overtone, plus a tiny noise click for the mallet strike.
    p.osc[0].morph = SINE; p.osc[0].on = true; p.osc[0].level = 1.0f;
    p.osc[1].morph = SINE; p.osc[1].on = true; p.osc[1].footage = 4; p.osc[1].level = 0.7f; // 2' partial
    p.oscMix = 0.12f; p.ringMod = 0.14f; p.noiseLevel = 0.05f; // mallet click
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.58f; p.resonance = 0.10f;
    p.envFilterAmt = 0.45f; p.keyTrack = 0.55f;
    p.env[0] = EnvParams{0.001f, 0.34f, 0.0f, 0.18f, 0.5f}; // tight mallet decay
    p.env[1] = EnvParams{0.001f, 0.10f, 0.0f, 0.08f, 0.0f}; // very fast click open
    p.env[2] = EnvParams{0.001f, 0.07f, 0.0f, 0.05f, 0.0f}; // ring transient
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    mod(p, 1, ModSource::Velocity, ModDest::RingMod, 0.25f); // hard strikes get the bar overtone
    mod(p, 2, ModSource::EnvAux, ModDest::RingMod, 0.20f);   // overtone only on the strike
    p.ampGain = 0.80f; p.width = 0.5f;
    p.fxReverb = 0.30f; p.fxDelay = 0.08f;
}
void pluck_nylon(SynthPatch& p)
{
    setName(p, "NYLON PLUCK"); p.category = 6;
    // Nylon-string guitar: soft twin triangles, a fingernail noise transient,
    // gentle ladder filter that velocity nudges open. Woody, intimate, no FX glare.
    p.osc[0].morph = TRI; p.osc[0].level = 1.0f;
    p.osc[1].morph = TRI; p.osc[1].detune = 0.04f; p.osc[1].level = 0.7f; // soft chorused body
    p.oscMix = 0.4f; p.noiseLevel = 0.09f; // fingertip pluck
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.48f; p.resonance = 0.16f;
    p.envFilterAmt = 0.55f; p.keyTrack = 0.6f;
    p.env[0] = EnvParams{0.003f, 0.42f, 0.0f, 0.22f, 0.4f}; // soft attack, natural decay
    p.env[1] = EnvParams{0.003f, 0.17f, 0.0f, 0.12f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Cutoff, 0.28f);    // dynamics open the tone
    mod(p, 1, ModSource::Velocity, ModDest::NoiseLevel, 0.10f); // harder picks click more
    mod(p, 2, ModSource::KeyTrack, ModDest::Cutoff, 0.15f);
    p.ampGain = 0.76f; p.width = 0.5f;
    p.fxChorus = 0.18f; p.fxDelay = 0.10f; p.fxReverb = 0.26f;
}

// ============================ FX / DRONE (35-39) ============================
void fx_noise(SynthPatch& p)
{
    setName(p, "NOISE SWEEP"); p.category = 7;
    // Filtered-noise wind: a resonant band-pass swept by a glacial sine, with a
    // random S&H "gust" layer and drive grit. Oscillators off — pure noise.
    p.ampGain = 0.85f; p.width = 0.92f;
    p.osc[0].level = 0.0f; p.osc[1].on = false; p.oscMix = 0.0f; p.noiseLevel = 0.85f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.34f; p.resonance = 0.62f;
    p.filterDrive = 0.22f; p.envFilterAmt = 0.45f;
    p.env[0] = EnvParams{1.2f, 1.6f, 0.85f, 2.6f, 0.0f}; // slow swell, long tail
    p.env[1] = EnvParams{2.0f, 2.0f, 0.6f, 2.0f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine;       p.lfo[0].rate = 0.07f; // slow wind sweep
    p.lfo[1].wave = (int)LfoWave::SampleHold; p.lfo[1].rate = 0.55f; // random gusts
    p.lfo[2].wave = (int)LfoWave::Triangle;   p.lfo[2].rate = 0.04f; // slow reso drift
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.16f);      // gusty band wobble
    mod(p, 2, ModSource::Lfo2, ModDest::FilterDrive, 0.2f);  // grit comes and goes
    mod(p, 3, ModSource::Lfo3, ModDest::Resonance, 0.18f);   // breathing whistle
    mod(p, 4, ModSource::ModWheel, ModDest::Cutoff, 0.4f);   // hand-swept wind
    p.fxPhaser = 0.3f; p.fxReverb = 0.6f; p.fxDelay = 0.35f;
}
void fx_drone(SynthPatch& p)
{
    setName(p, "DARK DRONE"); p.category = 7;
    // Evolving dark drone: detuned 32'/16' saw layers, a ring-mod undertone, and
    // slow LFOs crawling over detune + morph so the texture never sits still.
    p.ampGain = 0.72f; p.width = 0.88f; p.chorusMode = 2; // Ensemble
    p.voiceMode = (int)VoiceMode::Unison; p.unisonCount = 4; p.unisonDetune = 0.35f; p.unisonSpread = 0.9f;
    // BOTH oscillators on for the ring product
    p.osc[0].morph = SAW; p.osc[0].footage = 0; p.osc[0].level = 1.0f;            // 32'
    p.osc[1].on = true; p.osc[1].morph = SAW; p.osc[1].footage = 1; p.osc[1].detune = 0.12f; p.osc[1].level = 1.0f; // 16'
    p.oscMix = 0.5f; p.ringMod = 0.26f; p.subLevel = 0.2f; p.drift = 0.4f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.24f; p.resonance = 0.3f;
    p.envFilterAmt = 0.2f;
    p.env[0] = EnvParams{2.5f, 2.0f, 0.9f, 3.5f, 0.0f}; // long evolving texture
    p.env[1] = EnvParams{3.0f, 2.0f, 0.5f, 3.0f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine;       p.lfo[0].rate = 0.05f; // detune crawl
    p.lfo[1].wave = (int)LfoWave::Triangle;   p.lfo[1].rate = 0.03f; // morph crawl
    p.lfo[2].wave = (int)LfoWave::SampleHold; p.lfo[2].rate = 0.08f; // random ring swell
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.5f);    // layers slide apart/together
    mod(p, 1, ModSource::Lfo2, ModDest::Morph1, 0.3f);    // osc1 timbre morphs
    mod(p, 2, ModSource::Lfo1, ModDest::Cutoff, 0.18f);   // slow filter swell
    mod(p, 3, ModSource::Lfo3, ModDest::RingMod, 0.3f);   // metallic undertone breathes
    mod(p, 4, ModSource::Lfo2, ModDest::Pitch, 0.012f);   // barely-there pitch sway
    p.fxReverb = 0.7f; p.fxDelay = 0.45f; p.fxChorus = 0.4f;
}
void fx_riser(SynthPatch& p)
{
    setName(p, "RISER"); p.category = 7;
    // Tension build: a long aux envelope hauls pitch + cutoff up while the noise
    // bed and resonance swell in with it. SawUp LFO adds an accelerating shimmer.
    p.voiceMode = (int)VoiceMode::Mono;
    p.osc[0].morph = SAW; p.osc[1].on = false; p.oscMix = 0.0f; p.noiseLevel = 0.25f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.28f; p.resonance = 0.32f;
    p.filterDrive = 0.15f;
    p.env[0] = EnvParams{0.05f, 0.5f, 0.95f, 0.5f, 0.0f};
    p.env[2] = EnvParams{4.0f, 0.3f, 1.0f, 0.2f, 0.0f}; // slow rising aux (the build)
    p.lfo[0].wave = (int)LfoWave::SawUp;      p.lfo[0].rate = 3.5f;  // rising shimmer
    p.lfo[1].wave = (int)LfoWave::SampleHold; p.lfo[1].rate = 0.9f;  // jittery noise filter
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.0f);       // pitch climbs ~octave
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.6f);      // opens up
    mod(p, 2, ModSource::EnvAux, ModDest::NoiseLevel, 0.5f);  // wind builds in
    mod(p, 3, ModSource::EnvAux, ModDest::Resonance, 0.3f);   // sharpens toward the top
    mod(p, 4, ModSource::Lfo1, ModDest::Pitch, 0.01f);        // accelerating flutter
    mod(p, 5, ModSource::Lfo2, ModDest::Cutoff, 0.12f);       // gritty granular edge
    p.fxPhaser = 0.2f; p.fxDelay = 0.4f; p.fxReverb = 0.6f;
}
void fx_siren(SynthPatch& p)
{
    setName(p, "SIREN"); p.category = 7;
    // Alarm/siren: a few-Hz triangle wails the pitch up and down; a bright resonant
    // ladder tracks the wail for a screaming top. Mod wheel speeds the siren up.
    p.voiceMode = (int)VoiceMode::Mono;
    p.osc[0].morph = SAW; p.osc[1].on = false; p.oscMix = 0.0f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.7f; p.resonance = 0.42f;
    p.envFilterAmt = 0.3f;
    p.env[0] = EnvParams{0.03f, 0.4f, 0.95f, 0.4f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 3.0f; p.lfo[0].depth = 1.0f; // the wail
    p.lfo[1].wave = (int)LfoWave::Sine;     p.lfo[1].rate = 0.3f;  // slow brightness sweep
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.45f);        // pitch wail
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.22f);       // filter tracks the wail
    mod(p, 2, ModSource::Lfo2, ModDest::Cutoff, 0.28f);       // slow tonal sweep
    mod(p, 3, ModSource::ModWheel, ModDest::Lfo1Rate, 0.5f);  // wheel = siren speed
    p.fxPhaser = 0.25f; p.fxDelay = 0.35f; p.fxReverb = 0.4f;
}
void fx_zap(SynthPatch& p)
{
    setName(p, "LASER ZAP"); p.category = 7;
    // Short descending sci-fi zap: a fast aux env yanks pitch down an octave while
    // the osc0-saw x osc1-sine ring product (footage 2', the highest) clangs.
    p.voiceMode = (int)VoiceMode::Mono;
    // BOTH oscillators on for the ring product (clangorous sidebands)
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].on = true; p.osc[1].morph = SINE; p.osc[1].footage = 4; p.osc[1].level = 1.0f; // 2'
    p.oscMix = 0.3f; p.ringMod = 0.5f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.85f; p.resonance = 0.3f;
    p.env[0] = EnvParams{0.001f, 0.25f, 0.0f, 0.12f, 0.0f}; // instant attack, fast decay
    p.env[2] = EnvParams{0.001f, 0.12f, 0.0f, 0.08f, 0.0f}; // fast pitch-drop aux
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.0f);    // big descending swoop
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.4f);   // darkens as it falls
    mod(p, 2, ModSource::EnvAux, ModDest::RingMod, 0.4f);   // clang loudest at the strike
    p.fxDelay = 0.5f; p.fxReverb = 0.3f;
}

// ============================ PERC (9) ============================
// One-shot drum/percussion voices: amp sustain 0 + fast decay to silence, near-
// instant attack (shaker/clap excepted). EnvAux is a ~0.5 ms blip whose POSITIVE
// pitch mod makes the tone START HIGH and fall to the footage pitch (the kick
// drop). All hits auditioned at MIDI note 36, so pitch is set by footage + osc
// detune (semitones), never by the played note.
void perc_kick(SynthPatch& p)
{
    setName(p, "KICK"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.72f;
    p.osc[0].morph = SINE; p.osc[0].footage = 2; p.osc[0].detune = -3.0f; p.osc[0].level = 1.0f; // ~55 Hz body
    p.osc[1].on = false; p.oscMix = 0.0f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.55f; p.resonance = 0.08f;
    p.filterDrive = 0.35f; p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0006f, 0.30f, 0.0f, 0.05f, 0.30f}; // ~300 ms thud
    p.env[2] = EnvParams{0.0005f, 0.05f, 0.0f, 0.03f, 0.0f};  // ~50 ms pitch blip
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.6f);   // click -> thud drop
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.20f);
    p.fxReverb = 0.0f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_kick909(SynthPatch& p)
{
    setName(p, "KICK 909"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.70f;
    p.osc[0].morph = SINE; p.osc[0].footage = 2; p.osc[0].detune = -2.0f; p.osc[0].level = 1.0f; // ~58 Hz
    p.osc[1].on = false; p.oscMix = 0.0f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.60f; p.resonance = 0.10f;
    p.filterDrive = 0.30f; p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0008f, 0.50f, 0.0f, 0.08f, 0.30f}; // long 909 tail
    p.env[2] = EnvParams{0.0008f, 0.08f, 0.0f, 0.05f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.8f);
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.15f);
    p.fxDrive = 0.18f; p.fxReverb = 0.0f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_snare(SynthPatch& p)
{
    setName(p, "SNARE"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.66f;
    p.osc[0].morph = SINE; p.osc[0].footage = 3; p.osc[0].detune = 5.5f; p.osc[0].level = 0.5f; // ~180 Hz body
    p.osc[1].on = false; p.oscMix = 0.0f;
    p.noiseLevel = 0.85f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.58f; p.resonance = 0.20f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0006f, 0.18f, 0.0f, 0.05f, 0.30f};
    p.env[2] = EnvParams{0.0005f, 0.05f, 0.0f, 0.03f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.40f);
    p.fxReverb = 0.06f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_clap(SynthPatch& p)
{
    setName(p, "CLAP"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.62f;
    p.osc[0].on = false; p.osc[1].on = false; p.oscMix = 0.0f;
    p.noiseLevel = 0.90f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.60f; p.resonance = 0.25f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.001f, 0.18f, 0.0f, 0.06f, 0.30f};
    p.lfo[0].wave = (int)LfoWave::Square; p.lfo[0].rate = 55.0f; p.lfo[0].retrig = true;
    p.lfo[0].depth = 1.0f; p.lfo[0].sync = false;
    mod(p, 0, ModSource::Lfo1, ModDest::Amp, 0.90f); // 3-4 re-hit stutter
    p.fxReverb = 0.12f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_rim(SynthPatch& p)
{
    setName(p, "RIMSHOT"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.60f;
    p.osc[0].on = true; p.osc[0].morph = SINE; p.osc[0].footage = 4; p.osc[0].level = 1.0f;
    p.osc[1].on = true; p.osc[1].morph = SINE; p.osc[1].footage = 4; p.osc[1].detune = 7.0f; p.osc[1].level = 1.0f;
    p.oscMix = 0.5f; p.ringMod = 0.85f;
    p.noiseLevel = 0.12f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.80f; p.resonance = 0.25f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0004f, 0.045f, 0.0f, 0.02f, 0.30f};
    p.fxReverb = 0.05f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_hatc(SynthPatch& p)
{
    setName(p, "HAT CL"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.55f;
    p.osc[0].on = false; p.osc[1].on = false; p.oscMix = 0.0f;
    p.noiseLevel = 0.90f;
    p.filterMode = (int)FilterMode::SvfHP; p.cutoff = 0.88f; p.resonance = 0.15f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0004f, 0.045f, 0.0f, 0.02f, 0.30f};
    p.fxReverb = 0.04f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_hato(SynthPatch& p)
{
    setName(p, "HAT OP"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.55f;
    p.osc[0].on = true; p.osc[0].morph = PULSE; p.osc[0].footage = 4; p.osc[0].level = 0.5f;
    p.osc[1].on = true; p.osc[1].morph = PULSE; p.osc[1].footage = 4; p.osc[1].detune = 6.3f; p.osc[1].level = 0.5f;
    p.oscMix = 0.5f; p.ringMod = 0.5f;
    p.noiseLevel = 0.85f;
    p.filterMode = (int)FilterMode::SvfHP; p.cutoff = 0.82f; p.resonance = 0.15f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0004f, 0.35f, 0.0f, 0.10f, 0.30f};
    p.fxReverb = 0.08f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_tomhi(SynthPatch& p)
{
    setName(p, "TOM HI"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.68f;
    p.osc[0].morph = TRI; p.osc[0].footage = 3; p.osc[0].detune = 5.5f; p.osc[0].level = 1.0f; // ~180 Hz
    p.osc[1].on = false; p.oscMix = 0.0f;
    p.noiseLevel = 0.06f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.65f; p.resonance = 0.12f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0006f, 0.25f, 0.0f, 0.07f, 0.30f};
    p.env[2] = EnvParams{0.0005f, 0.10f, 0.0f, 0.05f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.40f);
    p.fxReverb = 0.07f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_tomlo(SynthPatch& p)
{
    setName(p, "TOM LO"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.70f;
    p.osc[0].morph = TRI; p.osc[0].footage = 2; p.osc[0].detune = 7.0f; p.osc[0].level = 1.0f; // ~97 Hz
    p.osc[1].on = false; p.oscMix = 0.0f;
    p.noiseLevel = 0.05f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.55f; p.resonance = 0.12f;
    p.filterDrive = 0.12f; p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0006f, 0.35f, 0.0f, 0.09f, 0.30f};
    p.env[2] = EnvParams{0.0005f, 0.14f, 0.0f, 0.06f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.55f);
    p.fxReverb = 0.07f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_cowbell(SynthPatch& p)
{
    setName(p, "COWBELL"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.62f;
    p.osc[0].on = true; p.osc[0].morph = PULSE; p.osc[0].footage = 3; p.osc[0].level = 1.0f;
    p.osc[1].on = true; p.osc[1].morph = PULSE; p.osc[1].footage = 3; p.osc[1].detune = 7.6f; p.osc[1].level = 1.0f;
    p.oscMix = 0.5f; p.ringMod = 0.7f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.70f; p.resonance = 0.28f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0008f, 0.20f, 0.0f, 0.07f, 0.30f};
    p.fxReverb = 0.08f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_clave(SynthPatch& p)
{
    setName(p, "CLAVE"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.78f;
    // bright tonal ping ~785 Hz; SvfLP cutoff ABOVE the tone so it passes at full
    // level (the old BP cutoff sat over the tone and rejected it -> silent).
    p.osc[0].morph = SINE; p.osc[0].footage = 4; p.osc[0].detune = 19.0f; p.osc[0].level = 1.0f;
    p.osc[1].on = false; p.oscMix = 0.0f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.85f; p.resonance = 0.35f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0004f, 0.06f, 0.0f, 0.03f, 0.30f};
    p.env[2] = EnvParams{0.0005f, 0.03f, 0.0f, 0.02f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.30f);
    p.fxReverb = 0.05f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_conga(SynthPatch& p)
{
    setName(p, "CONGA"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.66f;
    p.osc[0].morph = SINE; p.osc[0].footage = 3; p.osc[0].detune = 9.0f; p.osc[0].level = 1.0f; // ~220 Hz
    p.osc[1].on = false; p.oscMix = 0.0f;
    p.noiseLevel = 0.08f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.65f; p.resonance = 0.18f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0005f, 0.18f, 0.0f, 0.06f, 0.30f};
    p.env[2] = EnvParams{0.0005f, 0.07f, 0.0f, 0.04f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.30f);
    p.fxReverb = 0.06f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_cymbal(SynthPatch& p)
{
    setName(p, "CYMBAL"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.55f;
    p.osc[0].on = true; p.osc[0].morph = PULSE; p.osc[0].footage = 4; p.osc[0].level = 0.6f;
    p.osc[1].on = true; p.osc[1].morph = PULSE; p.osc[1].footage = 4; p.osc[1].detune = 5.1f; p.osc[1].level = 0.6f;
    p.oscMix = 0.5f; p.ringMod = 0.65f;
    p.noiseLevel = 0.90f;
    p.filterMode = (int)FilterMode::SvfHP; p.cutoff = 0.84f; p.resonance = 0.14f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0008f, 0.90f, 0.0f, 0.30f, 0.30f}; // long bright wash
    p.fxReverb = 0.14f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_shaker(SynthPatch& p)
{
    setName(p, "SHAKER"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.62f;
    p.osc[0].on = false; p.osc[1].on = false; p.oscMix = 0.0f;
    p.noiseLevel = 0.85f;
    p.filterMode = (int)FilterMode::SvfHP; p.cutoff = 0.78f; p.resonance = 0.12f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.012f, 0.10f, 0.0f, 0.05f, 0.30f}; // soft shh
    p.fxReverb = 0.05f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}
void perc_zap(SynthPatch& p)
{
    setName(p, "PERC ZAP"); p.category = 9;
    p.voiceMode = (int)VoiceMode::Mono;
    p.ampGain = 0.66f;
    p.osc[0].on = true; p.osc[0].morph = SAW;  p.osc[0].footage = 3; p.osc[0].level = 1.0f;
    p.osc[1].on = true; p.osc[1].morph = SINE; p.osc[1].footage = 4; p.osc[1].detune = 3.0f; p.osc[1].level = 1.0f;
    p.oscMix = 0.4f; p.ringMod = 0.6f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.82f; p.resonance = 0.28f;
    p.keyTrack = 0.0f;
    p.env[0] = EnvParams{0.0005f, 0.15f, 0.0f, 0.06f, 0.30f};
    p.env[2] = EnvParams{0.0005f, 0.10f, 0.0f, 0.05f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.5f);   // descends
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.35f);
    mod(p, 2, ModSource::EnvAux, ModDest::RingMod, 0.40f);
    p.fxReverb = 0.06f; p.fxDelay = 0.0f; p.fxChorus = 0.0f;
}

// ============================ AMBIENT / TEXTURE (10) ============================
// Slow attacks, high sustain, long honest releases (2-4 s), three slow LFOs
// animating detune/morph/cutoff/ring/pw, heavy reverb + delay, Ensemble + wide.
void amb_glacier(SynthPatch& p)
{
    setName(p, "GLACIER"); p.category = 10;
    p.width = 0.92f; p.ampGain = 0.78f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.10f; p.osc[1].level = 1.0f;
    p.oscMix = 0.5f; p.subLevel = 0.10f; p.drift = 0.35f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.42f; p.resonance = 0.12f;
    p.envFilterAmt = 0.18f; p.keyTrack = 0.45f;
    p.env[0] = EnvParams{2.20f, 2.0f, 0.92f, 3.6f, 0.15f};
    p.env[1] = EnvParams{2.60f, 2.2f, 0.55f, 3.0f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.05f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.08f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.03f;
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.40f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.22f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pitch,  0.006f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.40f; p.fxDelay = 0.22f; p.fxReverb = 0.72f;
}
void amb_drift(SynthPatch& p)
{
    setName(p, "DRIFT"); p.category = 10;
    p.width = 0.90f; p.ampGain = 0.54f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.12f; p.osc[1].level = 1.0f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = -0.09f; p.osc[2].level = 0.65f;
    p.oscMix = 0.5f; p.drift = 0.40f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.52f; p.resonance = 0.10f;
    p.envFilterAmt = 0.16f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{1.60f, 2.0f, 0.90f, 3.0f, 0.18f};
    p.env[1] = EnvParams{2.00f, 2.2f, 0.60f, 2.6f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.06f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.10f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.07f;
    mod(p, 0, ModSource::Lfo1, ModDest::Morph1, 0.30f);
    mod(p, 1, ModSource::Lfo1, ModDest::Morph2, 0.22f);
    mod(p, 2, ModSource::Lfo2, ModDest::Detune, 0.28f);
    mod(p, 3, ModSource::Lfo3, ModDest::OscMix, 0.18f);
    mod(p, 4, ModSource::ModWheel, ModDest::Cutoff, 0.40f);
    p.fxChorus = 0.55f; p.fxDelay = 0.18f; p.fxReverb = 0.58f;
}
void amb_choir(SynthPatch& p)
{
    setName(p, "VOX CHOIR"); p.category = 10;
    p.width = 0.90f; p.ampGain = 0.5f; p.chorusMode = 2;
    p.osc[0].morph = PULSE; p.osc[0].pw = 0.50f;
    p.osc[1].morph = PULSE; p.osc[1].pw = 0.44f; p.osc[1].detune = 0.08f;
    p.osc[3].on = true; p.osc[3].morph = PULSE; p.osc[3].pw = 0.52f; p.osc[3].detune = -0.10f; p.osc[3].level = 0.7f; // wider choir
    p.osc[4].on = true; p.osc[4].morph = SAW;   p.osc[4].footage = 3; p.osc[4].detune = 0.13f; p.osc[4].level = 0.35f; // airy octave
    p.oscMix = 0.5f; p.drift = 0.30f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.48f; p.resonance = 0.40f;
    p.envFilterAmt = 0.12f;
    p.env[0] = EnvParams{1.40f, 1.8f, 0.92f, 3.0f, 0.20f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.07f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.20f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.16f;
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.26f);
    mod(p, 1, ModSource::Lfo2, ModDest::Pw1,    0.24f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pw2,    0.24f);
    mod(p, 3, ModSource::Lfo1, ModDest::Detune, 0.14f);
    mod(p, 4, ModSource::ModWheel, ModDest::Resonance, 0.20f);
    p.fxChorus = 0.50f; p.fxDelay = 0.15f; p.fxReverb = 0.70f;
}
void amb_granular(SynthPatch& p)
{
    setName(p, "GRANULAR"); p.category = 10;
    p.width = 0.90f; p.ampGain = 0.74f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[0].footage = 3; p.osc[0].level = 1.0f;
    p.osc[1].morph = TRI; p.osc[1].detune = 0.06f; p.osc[1].level = 0.8f;
    p.oscMix = 0.5f; p.drift = 0.25f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.55f; p.resonance = 0.30f;
    p.envFilterAmt = 0.10f;
    p.env[0] = EnvParams{1.20f, 1.8f, 0.85f, 2.8f, 0.20f};
    p.lfo[0].wave = (int)LfoWave::SampleHold; p.lfo[0].rate = 0.30f;
    p.lfo[1].wave = (int)LfoWave::Sine;       p.lfo[1].rate = 7.5f;
    p.lfo[2].wave = (int)LfoWave::SampleHold; p.lfo[2].rate = 0.22f;
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.40f);
    mod(p, 1, ModSource::Lfo2, ModDest::Amp,    0.45f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pan,    0.35f);
    mod(p, 3, ModSource::ModWheel, ModDest::Lfo2Rate, 0.40f);
    p.fxChorus = 0.40f; p.fxDelay = 0.35f; p.fxReverb = 0.62f;
}
void amb_deepspace(SynthPatch& p)
{
    setName(p, "DEEPSPACE"); p.category = 10;
    p.width = 0.88f; p.ampGain = 0.8f; p.chorusMode = 2;
    p.osc[0].morph = SINE; p.osc[0].footage = 1; p.osc[0].level = 1.0f;
    p.osc[1].morph = SINE; p.osc[1].footage = 1; p.osc[1].detune = 0.20f; p.osc[1].level = 1.0f;
    p.oscMix = 0.5f; p.subLevel = 0.22f; p.subOctave = 2; p.drift = 0.30f;
    p.ringMod = 0.20f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.30f; p.resonance = 0.14f;
    p.envFilterAmt = 0.10f;
    p.env[0] = EnvParams{2.40f, 2.4f, 0.94f, 4.0f, 0.10f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.03f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.05f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.04f;
    mod(p, 0, ModSource::Lfo1, ModDest::RingMod, 0.22f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff,  0.12f);
    mod(p, 2, ModSource::Lfo3, ModDest::Detune,  0.18f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.30f; p.fxDelay = 0.30f; p.fxReverb = 0.78f;
}
void amb_shimmer(SynthPatch& p)
{
    setName(p, "SHIMMER"); p.category = 10;
    p.width = 0.90f; p.ampGain = 0.72f; p.chorusMode = 2;
    p.osc[0].morph = SINE; p.osc[0].footage = 3; p.osc[0].level = 1.0f;
    p.osc[1].morph = SINE; p.osc[1].footage = 4; p.osc[1].detune = 0.03f;
    p.oscMix = 0.0f; p.fm2to1 = 0.14f;
    p.ringMod = 0.16f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.74f; p.resonance = 0.10f;
    p.envFilterAmt = 0.12f;
    p.env[0] = EnvParams{1.50f, 2.0f, 0.85f, 3.4f, 0.22f};
    p.env[2] = EnvParams{1.20f, 1.6f, 0.0f,  1.4f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.12f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.08f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.05f;
    mod(p, 0, ModSource::Lfo1, ModDest::RingMod,  0.20f);
    mod(p, 1, ModSource::Lfo2, ModDest::FmAmount, 0.18f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pitch,    0.005f);
    mod(p, 3, ModSource::EnvAux,  ModDest::FmAmount, 0.30f);
    mod(p, 4, ModSource::ModWheel, ModDest::FmAmount, 0.30f);
    p.fxChorus = 0.40f; p.fxDelay = 0.42f; p.fxReverb = 0.72f;
}
void amb_underwater(SynthPatch& p)
{
    setName(p, "UNDERWTR"); p.category = 10;
    p.width = 0.85f; p.ampGain = 0.85f; p.chorusMode = 2;
    p.osc[0].morph = TRI; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.09f; p.osc[1].level = 0.85f;
    p.oscMix = 0.45f; p.subLevel = 0.16f; p.drift = 0.35f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.30f; p.resonance = 0.18f;
    p.filterDrive = 0.12f; p.envFilterAmt = 0.14f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{1.30f, 1.8f, 0.90f, 3.0f, 0.18f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.09f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.14f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.06f;
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch,  0.010f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::Lfo3, ModDest::Detune, 0.22f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.70f; p.fxDelay = 0.20f; p.fxReverb = 0.60f;
}
void amb_windscape(SynthPatch& p)
{
    setName(p, "WIND"); p.category = 10;
    p.width = 0.92f; p.ampGain = 0.82f; p.chorusMode = 2;
    p.osc[0].morph = TRI; p.osc[0].level = 0.45f;
    p.osc[1].morph = SINE; p.osc[1].detune = 0.06f; p.osc[1].level = 0.40f;
    p.oscMix = 0.5f; p.noiseLevel = 0.70f; p.drift = 0.30f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.45f; p.resonance = 0.45f;
    p.envFilterAmt = 0.10f;
    p.env[0] = EnvParams{2.00f, 2.0f, 0.90f, 3.6f, 0.15f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.08f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.05f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.11f;
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff,    0.35f);
    mod(p, 1, ModSource::Lfo2, ModDest::Amp,       0.30f);
    mod(p, 2, ModSource::Lfo3, ModDest::Resonance, 0.20f);
    mod(p, 3, ModSource::ModWheel, ModDest::NoiseLevel, 0.40f);
    p.fxChorus = 0.30f; p.fxDelay = 0.25f; p.fxReverb = 0.74f;
}
void amb_bowedglass(SynthPatch& p)
{
    setName(p, "BOWGLASS"); p.category = 10;
    p.width = 0.88f; p.ampGain = 0.82f; p.chorusMode = 2;
    p.osc[0].morph = SINE; p.osc[0].level = 1.0f;
    p.osc[1].morph = TRI;  p.osc[1].footage = 3; p.osc[1].detune = 0.05f; p.osc[1].level = 0.85f;
    p.oscMix = 0.5f; p.ringMod = 0.14f; p.drift = 0.22f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.62f; p.resonance = 0.55f;
    p.envFilterAmt = 0.12f;
    p.env[0] = EnvParams{1.80f, 1.8f, 0.88f, 3.0f, 0.20f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.13f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.09f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.06f;
    mod(p, 0, ModSource::Lfo1, ModDest::RingMod, 0.16f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff,  0.16f);
    mod(p, 2, ModSource::Lfo3, ModDest::Detune,  0.14f);
    mod(p, 3, ModSource::Aftertouch, ModDest::Resonance, 0.20f);
    p.fxChorus = 0.35f; p.fxDelay = 0.30f; p.fxReverb = 0.74f;
}
void amb_meadow(SynthPatch& p)
{
    setName(p, "MEADOW"); p.category = 10;
    p.width = 0.86f; p.ampGain = 0.58f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[0].level = 0.9f;
    p.osc[1].morph = TRI; p.osc[1].detune = 0.07f; p.osc[1].level = 1.0f;
    p.osc[2].on = true; p.osc[2].morph = SINE; p.osc[2].footage = 3; p.osc[2].detune = -0.05f; p.osc[2].level = 0.45f;
    p.oscMix = 0.45f; p.subLevel = 0.10f; p.drift = 0.28f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.50f; p.resonance = 0.08f;
    p.filterDrive = 0.10f; p.envFilterAmt = 0.16f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{1.00f, 1.6f, 0.90f, 2.6f, 0.20f};
    p.env[1] = EnvParams{1.40f, 1.8f, 0.60f, 2.2f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 0.10f;
    p.lfo[1].wave = (int)LfoWave::Sine;     p.lfo[1].rate = 0.07f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.05f;
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.14f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.20f);
    mod(p, 2, ModSource::Lfo3, ModDest::OscMix, 0.12f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.55f; p.fxDelay = 0.16f; p.fxReverb = 0.56f;
}
void amb_darkfog(SynthPatch& p)
{
    setName(p, "DARK FOG"); p.category = 10;
    p.width = 0.88f; p.ampGain = 0.78f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[0].footage = 1; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].footage = 1; p.osc[1].detune = 0.18f; p.osc[1].level = 1.0f;
    p.osc[2].on = true; p.osc[2].morph = TRI; p.osc[2].footage = 1; p.osc[2].detune = -0.13f; p.osc[2].level = 0.6f;
    p.oscMix = 0.5f; p.subLevel = 0.18f; p.drift = 0.40f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.28f; p.resonance = 0.22f;
    p.filterDrive = 0.30f;
    p.envFilterAmt = 0.12f; p.keyTrack = 0.35f;
    p.env[0] = EnvParams{2.00f, 2.2f, 0.92f, 3.8f, 0.12f};
    p.env[1] = EnvParams{2.40f, 2.4f, 0.45f, 3.0f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.04f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.06f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.03f;
    mod(p, 0, ModSource::Lfo1, ModDest::Detune,      0.45f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff,      0.14f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pitch,       0.008f);
    mod(p, 3, ModSource::Aftertouch, ModDest::FilterDrive, 0.30f);
    p.fxChorus = 0.30f; p.fxDelay = 0.28f; p.fxReverb = 0.76f;
}
void amb_aurora(SynthPatch& p)
{
    setName(p, "AURORA"); p.category = 10;
    p.width = 0.94f; p.ampGain = 0.8f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].footage = 3; p.osc[1].detune = 0.08f; p.osc[1].level = 0.8f;
    p.oscMix = 0.5f; p.drift = 0.30f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.40f; p.resonance = 0.22f;
    p.envFilterAmt = 0.14f; p.keyTrack = 0.45f;
    p.env[0] = EnvParams{1.60f, 1.8f, 0.90f, 3.2f, 0.18f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.05f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.09f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.07f;
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff,    0.42f);
    mod(p, 1, ModSource::Lfo2, ModDest::Resonance, 0.16f);
    mod(p, 2, ModSource::Lfo3, ModDest::Detune,    0.18f);
    p.fxChorus = 0.35f; p.fxPhaser = 0.45f; p.fxDelay = 0.24f; p.fxReverb = 0.68f;
}
void amb_dronechoir(SynthPatch& p)
{
    setName(p, "DRN CHOIR"); p.category = 10;
    p.width = 0.90f; p.ampGain = 0.46f; p.chorusMode = 2;
    p.voiceMode = (int)VoiceMode::Unison; p.unisonCount = 6; p.unisonDetune = 0.16f; p.unisonSpread = 0.85f;
    p.osc[0].morph = PULSE; p.osc[0].pw = 0.50f;
    p.osc[1].morph = PULSE; p.osc[1].pw = 0.46f; p.osc[1].detune = 0.05f;
    p.oscMix = 0.5f; p.subLevel = 0.10f; p.drift = 0.35f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.50f; p.resonance = 0.30f;
    p.envFilterAmt = 0.10f;
    p.env[0] = EnvParams{1.80f, 2.0f, 0.92f, 3.4f, 0.15f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.06f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.10f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.04f;
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.30f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pw1,    0.16f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.45f; p.fxDelay = 0.22f; p.fxReverb = 0.76f;
}
void amb_textwash(SynthPatch& p)
{
    setName(p, "TEXT WASH"); p.category = 10;
    p.width = 0.94f; p.ampGain = 0.5f; p.chorusMode = 2;
    p.osc[0].morph = TRI;   p.osc[0].level = 1.0f;
    p.osc[1].morph = PULSE; p.osc[1].pw = 0.45f; p.osc[1].detune = 0.22f; p.osc[1].level = 0.9f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].footage = 3; p.osc[2].detune = -0.30f; p.osc[2].level = 0.55f;
    p.osc[3].on = true; p.osc[3].morph = SAW; p.osc[3].footage = 1; p.osc[3].detune = 0.18f; p.osc[3].level = 0.5f;  // deep layer
    p.osc[4].on = true; p.osc[4].morph = TRI; p.osc[4].detune = -0.4f; p.osc[4].level = 0.4f;
    p.oscMix = 0.5f; p.ringMod = 0.10f; p.drift = 0.45f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.52f; p.resonance = 0.30f;
    p.envFilterAmt = 0.10f;
    p.env[0] = EnvParams{2.20f, 2.2f, 0.88f, 3.6f, 0.15f};
    p.lfo[0].wave = (int)LfoWave::Sine;       p.lfo[0].rate = 0.06f;
    p.lfo[1].wave = (int)LfoWave::Triangle;   p.lfo[1].rate = 0.09f;
    p.lfo[2].wave = (int)LfoWave::SampleHold; p.lfo[2].rate = 0.07f;
    mod(p, 0, ModSource::Lfo1, ModDest::Morph1, 0.30f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::Lfo2, ModDest::Pw2,    0.30f);
    mod(p, 3, ModSource::Lfo3, ModDest::Detune, 0.40f);
    mod(p, 4, ModSource::Lfo2, ModDest::RingMod, 0.14f);
    mod(p, 5, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.45f; p.fxDelay = 0.40f; p.fxReverb = 0.74f;
}
void amb_subdrone(SynthPatch& p)
{
    setName(p, "SUB DRONE"); p.category = 10;
    p.width = 0.80f; p.ampGain = 0.82f; p.chorusMode = 2;
    p.osc[0].morph = SINE; p.osc[0].footage = 1; p.osc[0].level = 1.0f;
    p.osc[1].morph = TRI;  p.osc[1].footage = 3; p.osc[1].detune = 0.04f; p.osc[1].level = 0.0f;
    p.oscMix = 0.5f; p.subLevel = 0.28f; p.subOctave = 2; p.drift = 0.18f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.34f; p.resonance = 0.10f;
    p.envFilterAmt = 0.08f;
    p.env[0] = EnvParams{2.40f, 2.4f, 0.94f, 4.0f, 0.10f};
    p.lfo[0].wave = (int)LfoWave::Sine;     p.lfo[0].rate = 0.04f;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.03f;
    p.lfo[2].wave = (int)LfoWave::Sine;     p.lfo[2].rate = 0.05f;
    mod(p, 0, ModSource::Lfo1, ModDest::OscMix, 0.30f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.14f);
    mod(p, 2, ModSource::Lfo2, ModDest::Cutoff, 0.08f);
    mod(p, 3, ModSource::Lfo3, ModDest::Pitch,  0.004f);
    p.fxChorus = 0.30f; p.fxDelay = 0.18f; p.fxReverb = 0.70f;
}

// ============================ SEQ / MOTION (11) ============================
// Self-animating: hold one chord/note and the patch moves via clock-synced LFOs
// (sync=true + syncDiv: 4=1/16, 7=1/8, 9=1/8., 10=1/4, 11=1/4., 12=1/2), S&H
// steppers, or the arp. Gate: Square LFO->Amp at -1.0 ducks to silence on alternate
// halves, so gated patches keep ampGain low (~0.5) with high amp-env sustain.
void seq_trancegate(SynthPatch& p)
{
    setName(p, "TRANCEGATE"); p.category = 11;
    p.width = 0.92f; p.ampGain = 0.50f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[0].level = 1.0f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.12f; p.osc[1].level = 1.0f;
    p.oscMix = 0.5f; p.subLevel = 0.10f; p.drift = 0.25f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.62f; p.resonance = 0.14f;
    p.envFilterAmt = 0.10f; p.keyTrack = 0.35f;
    p.env[0] = EnvParams{0.010f, 0.6f, 0.92f, 0.20f, 0.0f};
    p.env[1] = EnvParams{0.80f, 1.2f, 0.6f, 1.0f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Square; p.lfo[0].sync = true; p.lfo[0].syncDiv = 4;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    p.lfo[1].wave = (int)LfoWave::Sine; p.lfo[1].rate = 0.18f;
    mod(p, 0, ModSource::Lfo1,     ModDest::Amp,    -1.0f);
    mod(p, 1, ModSource::Lfo2,     ModDest::Cutoff, 0.10f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.30f; p.fxReverb = 0.42f; p.fxDelay = 0.22f;
}
void seq_pulse(SynthPatch& p)
{
    setName(p, "PULSE"); p.category = 11;
    p.width = 0.55f; p.ampGain = 0.78f;
    p.osc[0].morph = SAW; p.osc[1].morph = SAW; p.osc[1].detune = 0.07f;
    p.oscMix = 0.5f; p.subLevel = 0.14f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.30f; p.resonance = 0.42f;
    p.filterDrive = 0.18f; p.envFilterAmt = 0.0f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{0.006f, 0.5f, 0.90f, 0.25f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::SawDown; p.lfo[0].sync = true; p.lfo[0].syncDiv = 7;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    mod(p, 0, ModSource::Lfo1,     ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.20f);
    mod(p, 2, ModSource::ModWheel, ModDest::Resonance, 0.20f);
    p.fxDelay = 0.26f; p.fxReverb = 0.24f;
}
void seq_wobble(SynthPatch& p)
{
    setName(p, "WOBBLE"); p.category = 11;
    p.width = 0.35f; p.ampGain = 0.82f;
    p.voiceMode = (int)VoiceMode::Mono; p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.04f;
    p.osc[0].footage = 1; p.osc[0].morph = SAW;
    p.osc[1].footage = 1; p.osc[1].morph = SAW; p.osc[1].detune = 0.10f;
    p.oscMix = 0.5f; p.subLevel = 0.5f; p.subOctave = 1; p.drift = 0.2f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.30f; p.resonance = 0.55f;
    p.filterDrive = 0.45f; p.envFilterAmt = 0.0f; p.keyTrack = 0.2f;
    p.env[0] = EnvParams{0.006f, 0.4f, 0.95f, 0.10f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].sync = true; p.lfo[0].syncDiv = 7;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    mod(p, 0, ModSource::Lfo1,     ModDest::Cutoff, 0.60f);
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Rate, 0.30f);
    mod(p, 2, ModSource::Velocity, ModDest::FilterDrive, 0.25f);
    p.fxReverb = 0.12f; p.fxDelay = 0.10f;
}
void seq_sh_motion(SynthPatch& p)
{
    setName(p, "S&H MOVE"); p.category = 11;
    p.width = 0.6f; p.ampGain = 0.80f;
    p.osc[0].morph = SAW; p.osc[1].morph = PULSE; p.osc[1].pw = 0.32f; p.osc[1].detune = 0.05f;
    p.oscMix = 0.45f; p.sync2Mode = (int)SyncMode::Soft;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.42f; p.resonance = 0.34f;
    p.envFilterAmt = 0.30f; p.keyTrack = 0.45f;
    p.env[0] = EnvParams{0.003f, 0.5f, 0.70f, 0.30f, 0.25f};
    p.env[1] = EnvParams{0.004f, 0.30f, 0.4f, 0.25f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::SampleHold; p.lfo[0].sync = true; p.lfo[0].syncDiv = 4;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].rate = 0.2f;
    mod(p, 0, ModSource::Lfo1,   ModDest::Cutoff, 0.50f);
    mod(p, 1, ModSource::Lfo2,   ModDest::Detune, 0.10f);
    mod(p, 2, ModSource::Random, ModDest::Pan,    0.20f);
    p.fxDelay = 0.28f; p.fxReverb = 0.30f;
}
void seq_arpdrive(SynthPatch& p)
{
    setName(p, "ARP DRIVE"); p.category = 11;
    p.width = 0.6f; p.ampGain = 0.80f;
    p.osc[0].morph = SAW; p.osc[1].morph = SAW; p.osc[1].detune = 0.07f;
    p.oscMix = 0.5f; p.subLevel = 0.16f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.34f; p.resonance = 0.36f;
    p.filterDrive = 0.24f; p.envFilterAmt = 0.45f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{0.002f, 0.14f, 0.05f, 0.10f, 0.25f};
    p.env[1] = EnvParams{0.002f, 0.10f, 0.0f, 0.07f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].sync = true; p.lfo[0].syncDiv = 12;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    mod(p, 0, ModSource::Lfo1,     ModDest::Cutoff,    0.45f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff,    0.25f);
    mod(p, 2, ModSource::Velocity, ModDest::EnvFltAmt, 0.20f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::Up; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.50f; p.arp.swing = 0.0f; p.arp.ratchet = 1;
    p.fxDelay = 0.30f; p.fxReverb = 0.26f;
}
void seq_chordstab(SynthPatch& p)
{
    setName(p, "CHORDSEQ"); p.category = 11;
    p.width = 0.85f; p.ampGain = 0.52f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[1].morph = SAW; p.osc[1].detune = 0.11f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = -0.09f; p.osc[2].level = 0.7f;
    p.oscMix = 0.5f; p.subLevel = 0.08f; p.drift = 0.28f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.58f; p.resonance = 0.16f;
    p.envFilterAmt = 0.12f;
    p.env[0] = EnvParams{0.010f, 0.5f, 0.90f, 0.18f, 0.0f};
    p.env[1] = EnvParams{0.40f, 1.0f, 0.6f, 0.8f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Square; p.lfo[0].sync = true; p.lfo[0].syncDiv = 7;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    p.lfo[1].wave = (int)LfoWave::Sine; p.lfo[1].rate = 0.16f;
    mod(p, 0, ModSource::Lfo1,     ModDest::Amp,    -1.0f);
    mod(p, 1, ModSource::Lfo2,     ModDest::Detune, 0.18f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.45f; p.fxReverb = 0.40f; p.fxDelay = 0.18f;
}
void seq_acidseq(SynthPatch& p)
{
    setName(p, "ACID SEQ"); p.category = 11;
    p.width = 0.40f; p.ampGain = 0.84f;
    p.voiceMode = (int)VoiceMode::Mono; p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.05f;
    p.osc[0].morph = SAW; p.osc[1].on = false;
    p.oscMix = 0.0f; p.subLevel = 0.0f; p.drift = 0.15f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.22f; p.resonance = 0.62f;
    p.filterDrive = 0.40f; p.envFilterAmt = 0.85f; p.keyTrack = 0.5f;
    p.env[0] = EnvParams{0.002f, 0.30f, 0.30f, 0.12f, 0.20f};
    p.env[1] = EnvParams{0.002f, 0.16f, 0.0f, 0.10f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff,      0.45f);
    mod(p, 1, ModSource::Velocity,  ModDest::EnvFltAmt,   0.40f);
    mod(p, 2, ModSource::Velocity,  ModDest::FilterDrive, 0.25f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::Up; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.55f; p.arp.swing = 0.10f; p.arp.ratchet = 1;
    p.fxDelay = 0.24f; p.fxReverb = 0.14f;
}
void seq_blade(SynthPatch& p)
{
    setName(p, "BLADE"); p.category = 11;
    p.width = 0.90f; p.ampGain = 0.56f; p.chorusMode = 2;
    p.osc[0].morph = PULSE; p.osc[0].pw = 0.5f;
    p.osc[1].morph = SAW;   p.osc[1].detune = 0.12f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = -0.10f; p.osc[2].level = 0.8f;
    p.osc[3].on = true; p.osc[3].morph = SAW; p.osc[3].detune = 0.22f; p.osc[3].level = 0.6f;  // wider blade
    p.osc[4].on = true; p.osc[4].morph = SAW; p.osc[4].footage = 1; p.osc[4].detune = -0.05f; p.osc[4].level = 0.5f; // sub-octave body
    p.oscMix = 0.5f; p.subLevel = 0.10f; p.drift = 0.30f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.55f; p.resonance = 0.24f;
    p.envFilterAmt = 0.20f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{0.012f, 0.6f, 0.88f, 0.40f, 0.10f};
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].sync = true; p.lfo[0].syncDiv = 9;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    p.lfo[1].wave = (int)LfoWave::Sine; p.lfo[1].sync = true; p.lfo[1].syncDiv = 11;
    p.lfo[1].depth = 1.0f; p.lfo[1].retrig = false;
    mod(p, 0, ModSource::Lfo1,     ModDest::Pw1,    0.45f);
    mod(p, 1, ModSource::Lfo2,     ModDest::Cutoff, 0.35f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.40f; p.fxPhaser = 0.20f; p.fxReverb = 0.34f; p.fxDelay = 0.20f;
}
void seq_dubchord(SynthPatch& p)
{
    setName(p, "DUB CHORD"); p.category = 11;
    p.width = 0.7f; p.ampGain = 0.52f;
    p.osc[0].morph = PULSE; p.osc[0].pw = 0.45f;
    p.osc[1].morph = SAW; p.osc[1].detune = 0.06f;
    p.oscMix = 0.5f; p.subLevel = 0.06f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.50f; p.resonance = 0.20f;
    p.envFilterAmt = 0.10f;
    p.env[0] = EnvParams{0.006f, 0.30f, 0.85f, 0.12f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Square; p.lfo[0].sync = true; p.lfo[0].syncDiv = 7;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    p.lfo[1].wave = (int)LfoWave::Sine; p.lfo[1].rate = 0.15f;
    mod(p, 0, ModSource::Lfo1,     ModDest::Amp,    -1.0f);
    mod(p, 1, ModSource::Lfo2,     ModDest::Cutoff, 0.10f);
    p.fxChorus = 0.20f; p.fxDelay = 0.55f; p.fxReverb = 0.30f;
}
void seq_pluckmotion(SynthPatch& p)
{
    setName(p, "PLUCKMOV"); p.category = 11;
    p.width = 0.95f; p.ampGain = 0.80f;
    p.osc[0].morph = SAW; p.osc[1].morph = PULSE; p.osc[1].pw = 0.30f; p.osc[1].detune = 0.04f;
    p.oscMix = 0.45f; p.sync2Mode = (int)SyncMode::Soft;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.55f; p.resonance = 0.26f;
    p.envFilterAmt = 0.55f; p.keyTrack = 0.45f;
    p.env[0] = EnvParams{0.001f, 0.10f, 0.0f, 0.07f, 0.30f};
    p.env[1] = EnvParams{0.001f, 0.08f, 0.0f, 0.06f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::SampleHold; p.lfo[0].sync = true; p.lfo[0].syncDiv = 4;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    mod(p, 0, ModSource::Lfo1,     ModDest::Pan,    0.75f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::UpDown; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.40f; p.arp.swing = 0.15f; p.arp.ratchet = 1;
    p.fxChorus = 0.25f; p.fxDelay = 0.34f; p.fxReverb = 0.30f;
}
void seq_8bit(SynthPatch& p)
{
    setName(p, "8-BIT"); p.category = 11;
    p.width = 0.18f; p.ampGain = 0.82f;
    p.osc[0].morph = PULSE; p.osc[0].pw = 0.25f;
    p.osc[1].on = false; p.oscMix = 0.0f; p.subLevel = 0.0f; p.drift = 0.05f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.85f; p.resonance = 0.05f;
    p.envFilterAmt = 0.0f; p.keyTrack = 0.3f;
    p.env[0] = EnvParams{0.001f, 0.06f, 0.45f, 0.04f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].sync = true; p.lfo[0].syncDiv = 10;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    mod(p, 0, ModSource::Lfo1, ModDest::Pw1, 0.40f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::Up; p.arp.octaves = 2;
    p.arp.syncDiv = 2; p.arp.gate = 0.50f; p.arp.swing = 0.0f; p.arp.ratchet = 1;
    p.fxChorus = 0.0f; p.fxDelay = 0.12f; p.fxReverb = 0.10f;
}
void seq_rhythmbass(SynthPatch& p)
{
    setName(p, "RHY BASS"); p.category = 11;
    p.width = 0.30f; p.ampGain = 0.55f;
    p.voiceMode = (int)VoiceMode::Mono; p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.03f;
    p.osc[0].footage = 1; p.osc[0].morph = SAW;
    p.osc[1].footage = 1; p.osc[1].morph = SAW; p.osc[1].detune = 0.04f;
    p.oscMix = 0.5f; p.subLevel = 0.55f; p.subOctave = 1; p.drift = 0.12f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.40f; p.resonance = 0.22f;
    p.filterDrive = 0.30f; p.envFilterAmt = 0.20f; p.keyTrack = 0.3f;
    p.env[0] = EnvParams{0.004f, 0.4f, 0.90f, 0.06f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Square; p.lfo[0].sync = true; p.lfo[0].syncDiv = 4;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    mod(p, 0, ModSource::Lfo1,     ModDest::Amp,    -1.0f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxReverb = 0.08f; p.fxDelay = 0.06f;
}
void seq_glitch(SynthPatch& p)
{
    setName(p, "GLITCH"); p.category = 11;
    p.width = 0.9f; p.ampGain = 0.74f;
    p.osc[0].morph = SAW; p.osc[1].morph = PULSE; p.osc[1].pw = 0.4f; p.osc[1].detune = 0.06f;
    p.oscMix = 0.5f; p.ringMod = 0.10f;
    p.filterMode = (int)FilterMode::SvfBP; p.cutoff = 0.5f; p.resonance = 0.40f;
    p.envFilterAmt = 0.20f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{0.002f, 0.4f, 0.65f, 0.12f, 0.20f};
    p.lfo[0].wave = (int)LfoWave::SampleHold; p.lfo[0].sync = true; p.lfo[0].syncDiv = 2;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    p.lfo[1].wave = (int)LfoWave::SampleHold; p.lfo[1].sync = true; p.lfo[1].syncDiv = 3;
    p.lfo[1].depth = 1.0f; p.lfo[1].retrig = false;
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Lfo1, ModDest::Pitch,  0.015f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pan,    0.60f);
    mod(p, 3, ModSource::Lfo2, ModDest::RingMod, 0.30f);
    p.fxDelay = 0.30f; p.fxReverb = 0.28f;
}
void seq_motionpad(SynthPatch& p)
{
    setName(p, "MOTIONPAD"); p.category = 11;
    p.width = 0.88f; p.ampGain = 0.84f; p.chorusMode = 2;
    p.osc[0].morph = SAW; p.osc[1].morph = SAW; p.osc[1].detune = 0.12f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = -0.10f; p.osc[2].level = 0.7f;
    p.oscMix = 0.5f; p.subLevel = 0.10f; p.drift = 0.32f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.48f; p.resonance = 0.18f;
    p.envFilterAmt = 0.18f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{0.80f, 1.6f, 0.88f, 1.8f, 0.0f};
    p.env[1] = EnvParams{1.20f, 1.8f, 0.6f, 1.6f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].sync = true; p.lfo[0].syncDiv = 11;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    p.lfo[1].wave = (int)LfoWave::Triangle; p.lfo[1].sync = true; p.lfo[1].syncDiv = 8;
    p.lfo[1].depth = 1.0f; p.lfo[1].retrig = false;
    mod(p, 0, ModSource::Lfo1,     ModDest::Cutoff, 0.30f);
    mod(p, 1, ModSource::Lfo2,     ModDest::Detune, 0.40f);
    mod(p, 2, ModSource::Lfo2,     ModDest::OscMix, 0.10f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.45f; p.fxPhaser = 0.18f; p.fxReverb = 0.46f; p.fxDelay = 0.22f;
}
void seq_hoover_seq(SynthPatch& p)
{
    setName(p, "HOOVERSEQ"); p.category = 11;
    p.width = 0.78f; p.ampGain = 0.6f;
    p.osc[0].morph = SAW; p.osc[0].detune = 0.0f;
    p.osc[1].morph = PULSE; p.osc[1].pw = 0.4f; p.osc[1].detune = 0.18f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = -0.16f; p.osc[2].level = 0.85f;
    p.osc[3].on = true; p.osc[3].morph = SAW; p.osc[3].detune = 0.30f; p.osc[3].level = 0.7f;   // fatter hoover
    p.osc[4].on = true; p.osc[4].morph = PULSE; p.osc[4].pw = 0.35f; p.osc[4].detune = -0.28f; p.osc[4].level = 0.6f;
    p.oscMix = 0.5f; p.subLevel = 0.14f; p.drift = 0.30f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.40f; p.resonance = 0.34f;
    p.filterDrive = 0.40f; p.envFilterAmt = 0.35f; p.keyTrack = 0.4f;
    p.env[0] = EnvParams{0.003f, 0.20f, 0.30f, 0.14f, 0.20f};
    p.env[1] = EnvParams{0.004f, 0.18f, 0.10f, 0.12f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].sync = true; p.lfo[0].syncDiv = 12;
    p.lfo[0].depth = 1.0f; p.lfo[0].retrig = false;
    p.lfo[1].wave = (int)LfoWave::Sine; p.lfo[1].rate = 6.0f;
    mod(p, 0, ModSource::Lfo1,     ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Lfo2,     ModDest::Pw2,    0.30f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    p.arp.on = true; p.arp.mode = (int)ArpMode::Up; p.arp.octaves = 1;
    p.arp.syncDiv = 4; p.arp.gate = 0.70f; p.arp.swing = 0.0f; p.arp.ratchet = 1;
    p.fxChorus = 0.30f; p.fxDelay = 0.28f; p.fxReverb = 0.30f;
}

// ===== showcase presets for the 5-oscillator desktop voice (indices 85-87) =====
void lead_supersaw(SynthPatch& p)
{
    setName(p, "SUPERSAW"); p.category = 1; // LEAD
    p.width = 0.92f; p.ampGain = 0.6f;
    const float det[5] = {0.0f, 0.11f, -0.10f, 0.19f, -0.18f};
    for(int i = 0; i < 5; ++i)
    { p.osc[i].on = true; p.osc[i].morph = SAW; p.osc[i].detune = det[i]; p.osc[i].level = 0.85f; }
    p.oscMix = 0.5f; p.drift = 0.30f;
    p.filterMode = (int)FilterMode::SvfLP; p.cutoff = 0.70f; p.resonance = 0.12f;
    p.envFilterAmt = 0.25f;
    p.env[0] = EnvParams{0.01f, 0.6f, 0.9f, 0.4f, 0.0f};
    p.env[1] = EnvParams{0.02f, 0.5f, 0.5f, 0.4f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Triangle; p.lfo[0].rate = 0.30f;
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.30f); // animate the saw fan
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.20f; p.fxReverb = 0.25f; p.fxDelay = 0.15f;
}
void pad_fiveosc(SynthPatch& p)
{
    setName(p, "PENTA PAD"); p.category = 0; // PAD
    p.width = 0.86f; p.ampGain = 0.58f; p.chorusMode = 2;
    p.osc[0].on = true; p.osc[0].morph = SAW;   p.osc[0].footage = 2;
    p.osc[1].on = true; p.osc[1].morph = SAW;   p.osc[1].detune = 0.12f;
    p.osc[2].on = true; p.osc[2].morph = TRI;   p.osc[2].footage = 1; p.osc[2].level = 0.6f;
    p.osc[3].on = true; p.osc[3].morph = SAW;   p.osc[3].detune = -0.13f; p.osc[3].footage = 3; p.osc[3].level = 0.5f;
    p.osc[4].on = true; p.osc[4].morph = PULSE; p.osc[4].detune = 0.07f; p.osc[4].level = 0.4f;
    p.subLevel = 0.10f; p.drift = 0.35f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.45f; p.resonance = 0.10f; p.filterDrive = 0.15f;
    p.envFilterAmt = 0.30f; p.keyTrack = 0.40f;
    p.env[0] = EnvParams{0.8f, 1.6f, 0.85f, 1.8f, 0.2f};
    p.env[1] = EnvParams{1.2f, 1.6f, 0.5f, 1.5f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 0.18f;
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.12f);
    mod(p, 1, ModSource::Lfo1, ModDest::Detune, 0.20f);
    p.fxChorus = 0.35f; p.fxReverb = 0.40f;
}
void bass_reese5(SynthPatch& p)
{
    setName(p, "REESE 5"); p.category = 2; // BASS
    p.width = 0.45f; p.ampGain = 0.7f; p.voiceMode = (int)VoiceMode::Mono;
    p.glideMode = (int)GlideMode::Legato; p.glideTime = 0.06f;
    p.osc[0].on = true; p.osc[0].morph = SAW;
    p.osc[1].on = true; p.osc[1].morph = SAW; p.osc[1].detune = 0.22f;
    p.osc[2].on = true; p.osc[2].morph = SAW; p.osc[2].detune = -0.20f; p.osc[2].level = 0.9f;
    p.osc[3].on = true; p.osc[3].morph = SAW; p.osc[3].detune = 0.40f; p.osc[3].level = 0.7f;
    p.subLevel = 0.40f; p.subOctave = 1; p.drift = 0.20f;
    p.filterMode = (int)FilterMode::LadderLP; p.cutoff = 0.30f; p.resonance = 0.20f; p.filterDrive = 0.30f;
    p.envFilterAmt = 0.20f; p.keyTrack = 0.30f;
    p.env[0] = EnvParams{0.005f, 0.4f, 0.85f, 0.2f, 0.0f};
    p.env[1] = EnvParams{0.01f, 0.3f, 0.4f, 0.2f, 0.0f};
    p.lfo[0].wave = (int)LfoWave::Sine; p.lfo[0].rate = 0.25f;
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.30f); // the classic reese sweep
    p.fxChorus = 0.15f; p.fxReverb = 0.15f;
}

using Builder = void (*)(SynthPatch&);
const Builder kBuilders[kNumFactoryPresets] = {
    pad_warm, pad_strings, pad_glass, pad_vox, pad_unison,
    lead_classic, lead_sync, lead_soft, lead_super, lead_acidld,
    bass_moog, bass_acid, bass_sub, bass_fm, bass_pluck,
    arp_saw, arp_pluck, arp_bell, arp_random, arp_pingpong,
    stab_house, stab_rave, stab_brass, stab_organ, stab_pluckstab,
    keys_rhodes, keys_wurli, keys_dx, keys_clav, keys_poly,
    pluck_synth, pluck_koto, pluck_bell, pluck_mallet, pluck_nylon,
    fx_noise, fx_drone, fx_riser, fx_siren, fx_zap,
    perc_kick, perc_kick909, perc_snare, perc_clap, perc_rim,
    perc_hatc, perc_hato, perc_tomhi, perc_tomlo, perc_cowbell,
    perc_clave, perc_conga, perc_cymbal, perc_shaker, perc_zap,
    amb_glacier, amb_drift, amb_choir, amb_granular, amb_deepspace,
    amb_shimmer, amb_underwater, amb_windscape, amb_bowedglass, amb_meadow,
    amb_darkfog, amb_aurora, amb_dronechoir, amb_textwash, amb_subdrone,
    seq_trancegate, seq_pulse, seq_wobble, seq_sh_motion, seq_arpdrive,
    seq_chordstab, seq_acidseq, seq_blade, seq_dubchord, seq_pluckmotion,
    seq_8bit, seq_rhythmbass, seq_glitch, seq_motionpad, seq_hoover_seq,
    lead_supersaw, pad_fiveosc, bass_reese5, // 85-87: 5-oscillator showcases
};
} // namespace

// Desktop voicing pass applied to every factory patch after its builder runs:
// gives the (now parameterised) delay + reverb a character that fits the patch
// category instead of the one hard-coded global setting the hardware used. Only
// the wet FX voicing is touched, so a patch's dry tone, mix levels and mod
// routing are exactly as designed — but every preset now exercises the new
// delay/reverb controls.
void enhanceForDesktop(SynthPatch& p) noexcept
{
    switch(p.category)
    {
        case 0:  // PAD
        case 8:  // DRONE
        case 10: // AMB
            p.reverbSize = 0.84f; p.reverbTone = 0.32f;
            p.delaySync = true; p.delayDiv = 10; p.delayFeedback = 0.40f; p.delayTone = 0.55f; p.delayPing = true;
            break;
        case 1:  // LEAD
        case 4:  // STAB
            p.reverbSize = 0.55f; p.reverbTone = 0.45f;
            p.delaySync = true; p.delayDiv = 9; p.delayFeedback = 0.45f; p.delayTone = 0.45f; p.delayPing = true;
            break;
        case 2:  // BASS
            p.reverbSize = 0.34f; p.reverbTone = 0.6f;
            p.delaySync = true; p.delayDiv = 7; p.delayFeedback = 0.24f; p.delayTone = 0.6f; p.delayPing = false;
            break;
        case 3:  // ARP
        case 11: // SEQ
            p.reverbSize = 0.5f; p.reverbTone = 0.42f;
            p.delaySync = true; p.delayDiv = 7; p.delayFeedback = 0.5f; p.delayTone = 0.4f; p.delayPing = true;
            break;
        case 5:  // KEYS
        case 6:  // PLUCK
            p.reverbSize = 0.56f; p.reverbTone = 0.4f;
            p.delaySync = true; p.delayDiv = 7; p.delayFeedback = 0.3f; p.delayTone = 0.45f; p.delayPing = true;
            break;
        case 9:  // PERC
            p.reverbSize = 0.44f; p.reverbTone = 0.5f;
            p.delaySync = true; p.delayDiv = 4; p.delayFeedback = 0.32f; p.delayTone = 0.5f; p.delayPing = true;
            break;
        default: // FX (7) etc. keep the lush default
            p.reverbSize = 0.7f; p.reverbTone = 0.35f;
            break;
    }
}

void LoadFactoryPreset(int index, SynthPatch& p) noexcept
{
    InitDefaultPatch(p);
    if(index < 0 || index >= kNumFactoryPresets)
        return;
    kBuilders[index](p);
    enhanceForDesktop(p);
}

const char* FactoryPresetName(int index) noexcept
{
    static SynthPatch tmp;
    if(index < 0 || index >= kNumFactoryPresets)
        return "----";
    InitDefaultPatch(tmp);
    kBuilders[index](tmp);
    return tmp.name;
}

int FactoryPresetCategory(int index) noexcept
{
    static SynthPatch tmp;
    if(index < 0 || index >= kNumFactoryPresets)
        return 0;
    InitDefaultPatch(tmp);
    kBuilders[index](tmp);
    return tmp.category;
}

} // namespace jove

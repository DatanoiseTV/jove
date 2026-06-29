/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "SynthConfig.h"
#include <cmath>
#include <cstdint>

namespace jove
{
// ---- enums (stored as plain int in the patch so the generic UI editor and the
// web param registry can drive them; cast on use) ----------------------------

enum class VoiceMode : int
{
    Poly = 0,
    Mono,   // single voice, last-note priority, legato glide
    Unison, // one note, kMaxUnison detuned stacked voices
    Count
};
inline constexpr const char* kVoiceModeNames[] = {"POLY", "MONO", "UNISON"};

enum class GlideMode : int
{
    Off = 0,
    Always, // always portamento
    Legato, // only glide when notes overlap (fingered)
    Count
};
inline constexpr const char* kGlideModeNames[] = {"OFF", "ON", "LEGATO"};

// Oscillator sync: OFF, SOFT (slave resets only past mid-cycle — gentler, fewer
// sync harmonics) or HARD (reset every master cycle — the classic aggressive tear).
enum class SyncMode : int
{
    Off = 0,
    Soft,
    Hard,
    Count
};
inline constexpr const char* kSyncModeNames[] = {"OFF", "SOFT", "HARD"};

// Oscillator octave range (Juno/Jupiter footage labels).
inline constexpr const char* kFootageNames[] = {"32'", "16'", "8'", "4'", "2'"};
inline constexpr int         kNumFootage     = 5;

enum class FilterMode : int
{
    LadderLP = 0,
    SvfLP,
    SvfHP,
    SvfBP,
    SvfNotch,
    Lpg,      // low-pass gate: cutoff + amplitude track together (vactrol-ish)
    Steiner,  // Steiner-Parker-ish: peaky, soft-saturated lowpass
    Count
};
inline constexpr const char* kFilterModeNames[] = {"MOOG", "LP", "HP", "BP", "NOTCH", "LPG", "STEINER"};

enum class LfoWave : int
{
    Sine = 0,
    Triangle,
    SawUp,
    SawDown,
    Square,
    SampleHold,
    Count
};
inline constexpr const char* kLfoWaveNames[] = {"SINE", "TRI", "SAW+", "SAW-", "SQR", "S&H"};

// Arpeggiator patterns (Phase 6 implements the playback; the enum is the stable
// stored contract for presets + UI labels).
enum class ArpMode : int
{
    Up = 0,
    Down,
    UpDown,    // includes the top/bottom notes once (1 3 5 3)
    UpDownInc, // includes endpoints twice (1 3 5 5 3 1)
    DownUp,
    PingPong,  // bounce off endpoints
    Converge,  // outside-in
    Diverge,   // inside-out
    ConDiverge,
    AsPlayed,
    Random,
    Chord,     // all held notes at once, rhythmically
    Count
};
inline constexpr const char* kArpModeNames[] = {
    "UP",   "DOWN", "UP-DN", "UP-DN+", "DN-UP",  "PINGPONG",
    "CONV", "DIV",  "CON-DIV", "ASPLAYED", "RANDOM", "CHORD"};

// ---- mod matrix source/dest (Phase 5 reads these) ---------------------------
enum class ModSource : int
{
    Off = 0,
    Lfo1, Lfo2, Lfo3,
    EnvAmp, EnvFilter, EnvAux,
    Velocity, KeyTrack, ModWheel, Aftertouch, PitchBend,
    Random, Note,
    Count
};
inline constexpr const char* kModSourceNames[] = {
    "OFF",  "LFO1", "LFO2", "LFO3", "AMP EG", "FLT EG", "AUX EG",
    "VEL",  "KEY",  "MWHEEL", "ATOUCH", "BEND", "RAND", "NOTE"};

enum class ModDest : int
{
    Off = 0,
    Pitch,      // all oscillators, semitone-ish
    Osc2Pitch,
    Osc3Pitch,
    Morph1, Morph2, Morph3,
    Pw1, Pw2, Pw3,
    OscMix,     // osc1<->osc2 balance
    SubLevel, NoiseLevel,
    Cutoff, Resonance, FilterDrive,
    Amp, Pan,
    Lfo1Rate, Lfo2Rate, Lfo3Rate,
    Lfo1Depth, Lfo2Depth, Lfo3Depth,
    FxSend, FxParam,
    FmAmount, // osc2->osc1 FM index (env->FM gives the e-piano "bark")
    RingMod,  // osc1 x osc2 ring product depth (env/LFO -> metallic swell)
    EnvFltAmt,// filter-envelope -> cutoff depth
    Detune,   // osc2 (+) / osc3 (-) detune spread (LFO -> animated ensemble)
    Count
};
inline constexpr const char* kModDestNames[] = {
    "OFF",   "PITCH", "O2 PIT", "O3 PIT", "MORPH1", "MORPH2", "MORPH3",
    "PW1",   "PW2",   "PW3",    "OSCMIX", "SUB",    "NOISE",  "CUTOFF",
    "RESO",  "FDRIVE","AMP",    "PAN",    "L1 RATE","L2 RATE","L3 RATE",
    "L1 DPT","L2 DPT","L3 DPT", "FXSEND", "FXPARM", "FM",     "RING",
    "ENVFLT","DETUNE"};

struct ModSlot
{
    int   source = 0;
    int   dest   = 0;
    float amount = 0.0f; // -1..+1
};

// ---- assignable performance macros (the 3 live panel pots) ------------------
// Each pot writes one curated patch field absolutely (with soft-takeover on
// preset load). The destination is per-patch, so a bass patch can put RESO on a
// knob while a pad puts REVERB there. Order is the stored contract — append only.
enum class MacroDest : int
{
    Cutoff = 0,
    Resonance,
    Morph,     // osc1+osc2 morph together (timbre sweep)
    FilterEnv, // filter-env -> cutoff amount
    Attack,    // amp+filter attack time
    Release,   // amp+filter release time
    Chorus,
    Delay,
    Reverb,
    Drive,
    Lfo1Depth,
    Glide,
    Phaser,
    Ring,
    Count
};
inline constexpr const char* kMacroDestNames[] = {
    "CUTOFF", "RESO",  "MORPH",  "FLT ENV", "ATTACK", "RELEASE",
    "CHORUS", "DELAY", "REVERB", "DRIVE",   "LFO1 D", "GLIDE",
    "PHASER", "RING"};

// ---- per-oscillator settings ------------------------------------------------
struct OscParams
{
    int   footage = 2;    // index into kFootageNames (8' default)
    float morph   = 0.5f; // 0 tri .. 0.5 saw .. 1 pulse
    float pw      = 0.5f; // pulse width
    float detune  = 0.0f; // fine detune in semitones (+/-)
    float level   = 1.0f;
    float crush   = 0.0f; // bit-crush amount (0 clean .. 1 heavy) for DCO grit
    bool  on      = true;
};

// ---- LFO settings -----------------------------------------------------------
struct LfoParams
{
    int   wave    = 0;     // LfoWave
    float rate    = 1.0f;  // Hz (free), or division index when synced
    float depth   = 1.0f;
    bool  sync     = false; // sync rate to clock division
    int   syncDiv  = 10;
    bool  retrig   = true;  // restart phase + delay/fade on note-on
    bool  perVoice = false; // independent phase per voice vs one global phase
    float fade     = 0.0f;  // fade-in time (seconds) after the delay
    float delay    = 0.0f;  // delay before the fade (seconds) — delayed vibrato
    float offset   = 0.0f;  // bipolar DC bias added to the output (-1..+1)
    float phase    = 0.0f;  // start phase (0..1) applied on retrigger
};

// ---- envelope settings ------------------------------------------------------
struct EnvParams
{
    float attack  = 0.005f;
    float decay   = 0.20f;
    float sustain = 0.7f;
    float release = 0.30f;
    float velToLevel = 0.0f; // velocity -> peak level depth
};

// ---- arpeggiator settings ---------------------------------------------------
struct ArpParams
{
    bool  on      = false;
    int   mode    = 0;   // ArpMode
    int   octaves = 1;   // 1..4
    int   syncDiv = 4;   // division index into kArpDivNames (4 = 1/16). MUST be in
                         // [0, kNumArpDiv-1]; the old default of 14 was out of range
                         // and the engine silently clamped it to 12 (1/2 note).
    float gate    = 0.5f; // note length fraction 0..1
    float swing   = 0.0f; // 0..0.66
    bool  latch   = false;
    int   ratchet = 1;   // 1..4 repeats per step
};

// ---- the full patch ---------------------------------------------------------
struct SynthPatch
{
    char  name[20] = "INIT";
    int   category = 0; // index into kCategoryNames

    // voicing
    int   voiceMode = 0; // VoiceMode
    int   unisonCount = 5;
    float unisonDetune = 0.15f;
    float unisonSpread = 0.6f; // stereo spread
    int   glideMode = 0;       // GlideMode
    float glideTime = 0.08f;   // seconds
    int   bendRange = 2;       // semitones

    // oscillators + sources
    OscParams osc[kNumOsc];
    float     oscMix    = 0.5f; // osc1<->osc2 balance (osc3 added on top)
    float     subLevel  = 0.0f;
    int       subOctave = 1;    // 1 or 2 octaves below osc1
    float     noiseLevel = 0.0f;
    int       sync2Mode = 0;     // SyncMode: osc2 -> osc1 (off/soft/hard)
    int       sync3Mode = 0;     // SyncMode: osc3 -> osc1
    float     fm2to1    = 0.0f;  // osc2 -> osc1 cross-mod / FM depth
    float     ringMod   = 0.0f;  // osc1 x osc2 ring product blended in (metallic)
    float     drift     = 0.2f;  // analog pitch drift amount

    // filter
    int   filterMode = 0; // FilterMode
    float cutoff     = 0.6f; // 0..1 (maps log to Hz)
    float resonance  = 0.1f;
    float filterDrive = 0.0f;
    float keyTrack   = 0.5f;  // 0..1 (1 = full keyboard tracking)
    float envFilterAmt = 0.4f; // filter-env -> cutoff depth (bipolar via UI)

    // amp
    float ampGain  = 0.8f;
    float pan      = 0.0f;
    float masterTune = 0.0f; // global fine tune, cents

    // modulation
    LfoParams lfo[kNumLfo];
    EnvParams env[kNumEnv]; // 0 amp, 1 filter, 2 aux
    ModSlot   mod[kNumModSlots];

    // arp
    ArpParams arp;

    // performance macros: the 3 live pots each drive one of these destinations
    int macroDest[3] = {(int)MacroDest::Cutoff, (int)MacroDest::Resonance, (int)MacroDest::Reverb};

    // FX section send levels (the built-in clean chorus/delay/reverb).
    float fxChorus = 0.3f;
    int   chorusMode = 0;  // 0 Chorus I, 1 Chorus II, 2 Ensemble
    float fxPhaser = 0.0f; // 6-stage all-pass phaser wet mix (0 = bypass)
    float fxDelay  = 0.2f;
    float fxReverb = 0.25f;
    float fxDrive  = 0.0f;
    float width    = 0.5f;  // master stereo width (0 mono, 0.5 normal, 1 wide)

    // Delay voicing (the built-in clean delay; mix is fxDelay above).
    bool  delaySync     = true;   // lock time to host tempo
    int   delayDiv      = 9;      // division index (kArpDivNames; 9 = 1/8 dotted)
    float delayTimeMs   = 350.0f; // free time when delaySync == false (20..2000)
    float delayFeedback = 0.42f;
    float delayTone     = 0.45f;  // feedback-path damping (0 bright .. 1 dark)
    bool  delayPing     = true;   // cross feedback L<->R

    // Reverb voicing (mix is fxReverb above).
    float reverbSize    = 0.6f;   // tail length / room feedback
    float reverbTone    = 0.4f;   // HF damping (0 bright .. 1 dark)
};

inline constexpr const char* kCategoryNames[] = {
    "PAD", "LEAD", "BASS", "ARP", "STAB", "KEYS", "PLUCK", "FX", "DRONE",
    "PERC", "AMB", "SEQ"};
inline constexpr int kNumCategories = 12;

// Apply a 0..1 macro-pot value `v` to its destination patch field. Absolute
// control (the pot sets the value), with sensible per-destination ranges. Used
// by the live pot handler each block.
inline void MacroApply(SynthPatch& p, int dest, float v) noexcept
{
    v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    switch((MacroDest)dest)
    {
        case MacroDest::Cutoff: p.cutoff = v; break;
        case MacroDest::Resonance: p.resonance = v; break;
        case MacroDest::Morph: p.osc[0].morph = p.osc[1].morph = v; break;
        case MacroDest::FilterEnv: p.envFilterAmt = v; break;
        case MacroDest::Attack:
            p.env[0].attack = p.env[1].attack = v * v * 3.0f; break; // 0..3 s
        case MacroDest::Release:
            p.env[0].release = p.env[1].release = v * v * 4.0f; break; // 0..4 s
        case MacroDest::Chorus: p.fxChorus = v; break;
        case MacroDest::Delay: p.fxDelay = v; break;
        case MacroDest::Reverb: p.fxReverb = v; break;
        case MacroDest::Drive: p.fxDrive = v; break;
        case MacroDest::Lfo1Depth: p.lfo[0].depth = v; break;
        case MacroDest::Glide:
            p.glideTime = v * 0.6f;
            p.glideMode = v > 0.01f ? (int)GlideMode::Always : (int)GlideMode::Off;
            break;
        case MacroDest::Phaser: p.fxPhaser = v; break;
        case MacroDest::Ring: p.ringMod = v; break;
        default: break;
    }
}

// Current normalized value of a macro destination (for soft-takeover + overlay).
inline float MacroRead(const SynthPatch& p, int dest) noexcept
{
    switch((MacroDest)dest)
    {
        case MacroDest::Cutoff: return p.cutoff;
        case MacroDest::Resonance: return p.resonance;
        case MacroDest::Morph: return p.osc[0].morph;
        case MacroDest::FilterEnv: return p.envFilterAmt;
        case MacroDest::Attack: return std::sqrt(p.env[0].attack / 3.0f);
        case MacroDest::Release: return std::sqrt(p.env[0].release / 4.0f);
        case MacroDest::Chorus: return p.fxChorus;
        case MacroDest::Delay: return p.fxDelay;
        case MacroDest::Reverb: return p.fxReverb;
        case MacroDest::Drive: return p.fxDrive;
        case MacroDest::Lfo1Depth: return p.lfo[0].depth;
        case MacroDest::Glide: return p.glideTime / 0.6f;
        case MacroDest::Phaser: return p.fxPhaser;
        case MacroDest::Ring: return p.ringMod;
        default: return 0.0f;
    }
}

// A musical INIT patch: two slightly-detuned saws through the Moog ladder with a
// gentle filter envelope — the classic "is it on?" starting point. Used at boot
// and as the base every factory preset overrides.
inline void InitDefaultPatch(SynthPatch& p) noexcept
{
    p = SynthPatch{}; // struct defaults
    // name
    const char* nm = "INIT SAW";
    int         i  = 0;
    for(; nm[i] && i < 19; ++i)
        p.name[i] = nm[i];
    p.name[i] = '\0';

    p.category  = (int)1; // LEAD
    p.voiceMode = (int)VoiceMode::Poly;

    p.osc[0].morph = 0.5f;  // saw
    p.osc[0].level = 1.0f;
    p.osc[1].morph = 0.5f;  // saw
    p.osc[1].detune = 0.08f; // a touch sharp -> analog fatness
    p.osc[1].level = 1.0f;
    p.osc[2].on    = false;
    for(int i = 3; i < kNumOsc; ++i) p.osc[i].on = false; // osc4-5 off by default
    p.oscMix       = 0.5f;
    p.subLevel     = 0.0f;
    p.noiseLevel   = 0.0f;

    p.filterMode   = (int)FilterMode::LadderLP;
    p.cutoff       = 0.55f;
    p.resonance    = 0.12f;
    p.envFilterAmt = 0.45f;
    p.keyTrack     = 0.5f;

    p.env[0] = EnvParams{0.003f, 0.6f, 0.8f, 0.25f, 0.0f}; // amp
    p.env[1] = EnvParams{0.004f, 0.35f, 0.2f, 0.30f, 0.0f}; // filter
    p.env[2] = EnvParams{0.01f, 0.3f, 0.5f, 0.3f, 0.0f};    // aux

    p.ampGain = 0.7f;

    // one tasteful default LFO -> nothing routed yet (mod slots empty)
    p.lfo[0].wave = (int)LfoWave::Triangle;
    p.lfo[0].rate = 4.5f;
}

} // namespace jove

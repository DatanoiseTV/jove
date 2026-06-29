/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.

  Factory bank, hand-designed against the Jove voice engine. Morph axis:
  0 = SINE, 0.25 = TRIANGLE, 0.5 = SAW, 1 = PULSE. Patches lean on the 5-osc
  voice (detuned stacks for analog width), the mod matrix (LFO->cutoff/detune/pw,
  env->FM bark, velocity dynamics), ring-mod / sync / FM for character, and per-
  category delay+reverb voicing (enhanceForDesktop). Indices are the stored
  contract with MIDI program-change — append, never reorder.
*/

#include "SynthPresets.h"

namespace jove
{
namespace
{
constexpr float SINE = 0.0f, TRI = 0.25f, SAW = 0.5f, PULSE = 1.0f;

// Division indices into kArpDivNames: 4=1/16, 7=1/8, 9=1/8., 10=1/4, 12=1/2.

void setName(SynthPatch& p, const char* nm) noexcept
{
    int i = 0;
    for(; nm[i] && i < 19; ++i) p.name[i] = nm[i];
    p.name[i] = '\0';
}
void mod(SynthPatch& p, int slot, ModSource s, ModDest d, float amt) noexcept
{
    if(slot < 0 || slot >= kNumModSlots) return;
    p.mod[slot] = ModSlot{(int) s, (int) d, amt};
}
// enable + voice oscillator i (footage 2 == 8', the natural octave)
void osc(SynthPatch& p, int i, float morph, float det, float lvl, int foot = 2) noexcept
{
    p.osc[i].on = true; p.osc[i].morph = morph; p.osc[i].detune = det;
    p.osc[i].level = lvl; p.osc[i].footage = foot;
}
void lfo(SynthPatch& p, int i, LfoWave w, float rate, float depth = 1.0f, float off = 0.0f) noexcept
{
    p.lfo[i].wave = (int) w; p.lfo[i].rate = rate; p.lfo[i].depth = depth; p.lfo[i].offset = off;
}
void lfoSync(SynthPatch& p, int i, LfoWave w, int div, float depth = 1.0f) noexcept
{
    p.lfo[i].wave = (int) w; p.lfo[i].sync = true; p.lfo[i].syncDiv = div;
    p.lfo[i].depth = depth; p.lfo[i].retrig = false;
}
void filt(SynthPatch& p, FilterMode m, float cut, float res, float envAmt, float drive = 0.0f, float key = 0.4f) noexcept
{
    p.filterMode = (int) m; p.cutoff = cut; p.resonance = res;
    p.envFilterAmt = envAmt; p.filterDrive = drive; p.keyTrack = key;
}

// ================================ PAD (0-4) ================================
void pad_warm(SynthPatch& p)
{
    setName(p, "WARM JUNO"); p.category = 0;
    p.width = 0.85f; p.ampGain = 0.72f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.11f, 1.0f);
    osc(p, 3, SAW, -0.08f, 0.55f); osc(p, 4, SAW, 0.17f, 0.42f, 1);
    p.subLevel = 0.16f; p.drift = 0.30f;
    filt(p, FilterMode::LadderLP, 0.40f, 0.10f, 0.36f, 0.22f);
    p.env[0] = EnvParams{0.65f, 1.5f, 0.88f, 1.8f, 0.30f};
    p.env[1] = EnvParams{1.40f, 1.8f, 0.55f, 1.6f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.20f, 1.0f, 0.18f);   // one-sided cutoff bloom
    lfo(p, 1, LfoWave::Triangle, 0.13f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.10f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.40f);
    mod(p, 2, ModSource::Lfo2, ModDest::Detune, 0.5f);
    mod(p, 3, ModSource::Aftertouch, ModDest::EnvFltAmt, 0.30f);
    p.fxChorus = 0.30f; p.fxPhaser = 0.18f; p.fxReverb = 0.32f;
}
void pad_glass(SynthPatch& p)
{
    setName(p, "GLASS PAD"); p.category = 0;
    p.width = 0.8f; p.ampGain = 0.78f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.02f, 1.0f, 3); // octave-up bell partial
    p.fm2to1 = 0.35f; p.ringMod = 0.18f;                          // glassy FM + ring
    p.subLevel = 0.08f; p.drift = 0.18f;
    filt(p, FilterMode::SvfLP, 0.62f, 0.12f, 0.30f, 0.0f, 0.6f);
    p.env[0] = EnvParams{0.9f, 2.0f, 0.7f, 2.4f, 0.25f};
    p.env[2] = EnvParams{0.4f, 1.2f, 0.3f, 1.0f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.16f);
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.4f);   // bell bark on attack
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::ModWheel, ModDest::RingMod, 0.4f);
    mod(p, 3, ModSource::Aftertouch, ModDest::FmAmount, 0.3f);
    p.fxChorus = 0.22f; p.fxReverb = 0.5f; p.fxDelay = 0.12f;
}
void pad_strings(SynthPatch& p)
{
    setName(p, "STRINGS"); p.category = 0;
    p.width = 0.9f; p.ampGain = 0.64f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.13f, 1.0f);
    osc(p, 2, SAW, -0.11f, 0.8f); osc(p, 3, SAW, 0.22f, 0.6f); osc(p, 4, SAW, -0.2f, 0.6f);
    p.drift = 0.38f;
    filt(p, FilterMode::SvfLP, 0.56f, 0.12f, 0.20f);
    p.env[0] = EnvParams{0.55f, 1.6f, 0.92f, 2.0f, 0.25f};
    p.env[1] = EnvParams{1.2f, 1.8f, 0.6f, 1.6f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.28f); lfo(p, 1, LfoWave::Triangle, 0.19f);
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.55f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.30f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.2f);
    p.fxChorus = 0.35f; p.fxReverb = 0.4f;
}
void pad_voices(SynthPatch& p)
{
    setName(p, "VOICES"); p.category = 0;
    p.width = 0.88f; p.ampGain = 0.58f; p.chorusMode = 2;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    osc(p, 1, PULSE, 0.08f, 1.0f); p.osc[1].pw = 0.44f;
    osc(p, 3, PULSE, -0.1f, 0.7f); p.osc[3].pw = 0.52f;
    p.drift = 0.3f;
    filt(p, FilterMode::SvfBP, 0.5f, 0.4f, 0.12f); // formant-ish BP
    p.env[0] = EnvParams{1.2f, 1.8f, 0.92f, 2.6f, 0.2f};
    lfo(p, 0, LfoWave::Sine, 0.08f); lfo(p, 1, LfoWave::Triangle, 0.2f); lfo(p, 2, LfoWave::Sine, 0.15f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.22f);
    mod(p, 1, ModSource::Lfo2, ModDest::Pw1, 0.3f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pw2, 0.3f);
    mod(p, 3, ModSource::Lfo1, ModDest::Detune, 0.16f);
    mod(p, 4, ModSource::ModWheel, ModDest::Resonance, 0.25f);
    p.fxChorus = 0.45f; p.fxReverb = 0.62f;
}
void pad_dark(SynthPatch& p)
{
    setName(p, "DARK PAD"); p.category = 0;
    p.width = 0.78f; p.ampGain = 0.78f; p.chorusMode = 0;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, -0.07f, 0.9f);
    osc(p, 3, SAW, 0.1f, 0.5f, 1); p.subLevel = 0.3f; p.subOctave = 2; p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.28f, 0.18f, 0.3f, 0.3f, 0.3f);
    p.env[0] = EnvParams{1.0f, 2.0f, 0.85f, 2.6f, 0.2f};
    p.env[1] = EnvParams{1.8f, 2.0f, 0.4f, 1.8f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.1f, 1.0f, 0.1f); lfo(p, 1, LfoWave::Triangle, 0.07f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.14f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.3f);
    mod(p, 2, ModSource::Aftertouch, ModDest::Cutoff, 0.3f);
    mod(p, 3, ModSource::ModWheel, ModDest::EnvFltAmt, 0.3f);
    p.fxReverb = 0.42f; p.fxDelay = 0.1f;
}

// ================================ LEAD (1) ================================
void lead_super(SynthPatch& p)
{
    setName(p, "SUPERSAW"); p.category = 1; p.chorusMode = 2;
    p.width = 0.8f; p.ampGain = 0.42f;
    p.voiceMode = (int) VoiceMode::Unison; p.unisonCount = 7; p.unisonDetune = 0.2f; p.unisonSpread = 0.85f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.04f, 1.0f);
    osc(p, 2, SAW, -0.05f, 0.9f); osc(p, 3, SAW, 0.1f, 0.8f);
    p.subLevel = 0.1f; p.drift = 0.3f;
    filt(p, FilterMode::LadderLP, 0.62f, 0.1f, 0.3f, 0.0f, 0.5f);
    p.env[0] = EnvParams{0.01f, 0.6f, 0.85f, 0.5f, 0.0f};
    p.env[1] = EnvParams{0.03f, 0.5f, 0.55f, 0.45f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.18f, 1.0f, 0.12f); lfo(p, 1, LfoWave::Triangle, 0.11f);
    mod(p, 0, ModSource::ModWheel, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.4f);
    mod(p, 2, ModSource::ModWheel, ModDest::Detune, 0.3f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.2f);
    p.fxChorus = 0.35f; p.fxReverb = 0.3f; p.fxDelay = 0.24f;
}
void lead_sync(SynthPatch& p)
{
    setName(p, "SYNC LEAD"); p.category = 1;
    p.width = 0.55f; p.ampGain = 0.7f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.0f, 1.0f);
    p.sync2Mode = (int) SyncMode::Hard; // osc2 hard-synced -> classic tear
    p.drift = 0.15f;
    filt(p, FilterMode::LadderLP, 0.6f, 0.18f, 0.45f, 0.25f, 0.4f);
    p.env[0] = EnvParams{0.005f, 0.3f, 0.8f, 0.25f, 0.0f};
    p.env[1] = EnvParams{0.004f, 0.4f, 0.3f, 0.3f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 5.0f); // vibrato
    mod(p, 0, ModSource::ModWheel, ModDest::Osc2Pitch, 0.6f); // wheel sweeps the sync pitch
    mod(p, 1, ModSource::EnvFilter, ModDest::Osc2Pitch, 0.4f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pitch, 0.04f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxDelay = 0.22f; p.fxReverb = 0.2f;
}
void lead_soft(SynthPatch& p)
{
    setName(p, "SOFT LEAD"); p.category = 1;
    p.width = 0.5f; p.ampGain = 0.82f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Always; p.glideTime = 0.08f;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SINE, 0.03f, 0.8f);
    p.subLevel = 0.12f; p.drift = 0.12f;
    filt(p, FilterMode::SvfLP, 0.7f, 0.08f, 0.2f);
    p.env[0] = EnvParams{0.04f, 0.4f, 0.9f, 0.4f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 4.5f); p.lfo[0].fade = 0.6f; p.lfo[0].delay = 0.3f; // delayed vibrato
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.05f);
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Depth, 0.6f);
    mod(p, 2, ModSource::Aftertouch, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.2f; p.fxDelay = 0.18f; p.fxReverb = 0.25f;
}
void lead_fm(SynthPatch& p)
{
    setName(p, "FM LEAD"); p.category = 1;
    p.width = 0.5f; p.ampGain = 0.78f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 1.0f, 3); // 2:1 ratio modulator
    p.fm2to1 = 0.6f; p.drift = 0.1f;
    filt(p, FilterMode::SvfLP, 0.8f, 0.05f, 0.1f);
    p.env[0] = EnvParams{0.005f, 0.5f, 0.7f, 0.3f, 0.1f};
    p.env[2] = EnvParams{0.002f, 0.25f, 0.2f, 0.2f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.6f);   // bark transient
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.5f); // harder = brighter
    mod(p, 2, ModSource::ModWheel, ModDest::FmAmount, 0.5f);
    p.fxDelay = 0.2f; p.fxReverb = 0.22f;
}
void lead_pwm(SynthPatch& p)
{
    setName(p, "PWM LEAD"); p.category = 1;
    p.width = 0.6f; p.ampGain = 0.72f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    osc(p, 1, PULSE, 0.06f, 0.9f); p.osc[1].pw = 0.5f;
    p.subLevel = 0.15f; p.drift = 0.15f;
    filt(p, FilterMode::LadderLP, 0.65f, 0.15f, 0.3f, 0.2f, 0.4f);
    p.env[0] = EnvParams{0.01f, 0.4f, 0.85f, 0.3f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 0.6f); lfo(p, 1, LfoWave::Triangle, 4.0f);
    mod(p, 0, ModSource::Lfo1, ModDest::Pw1, 0.45f);   // classic PWM
    mod(p, 1, ModSource::Lfo1, ModDest::Pw2, 0.4f);
    mod(p, 2, ModSource::Lfo2, ModDest::Pitch, 0.04f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.25f; p.fxDelay = 0.2f; p.fxReverb = 0.2f;
}

// ================================ BASS (2) ================================
void bass_reese(SynthPatch& p)
{
    setName(p, "REESE"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.06f; p.width = 0.45f; p.ampGain = 0.72f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.22f, 1.0f); osc(p, 2, SAW, -0.2f, 0.9f);
    osc(p, 3, SAW, 0.4f, 0.7f); p.subLevel = 0.4f; p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.30f, 0.2f, 0.2f, 0.3f, 0.3f);
    p.env[0] = EnvParams{0.005f, 0.4f, 0.85f, 0.2f, 0.0f};
    p.env[1] = EnvParams{0.01f, 0.3f, 0.4f, 0.2f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.25f);
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.35f); // the reese sweep
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.4f);
    p.fxChorus = 0.12f; p.fxReverb = 0.1f;
}
void bass_sub(SynthPatch& p)
{
    setName(p, "SUB BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f; p.width = 0.2f; p.ampGain = 0.9f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.4f); p.subLevel = 0.5f; p.drift = 0.05f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.05f, 0.25f, 0.2f, 0.2f);
    p.env[0] = EnvParams{0.004f, 0.3f, 0.9f, 0.15f, 0.0f};
    p.env[1] = EnvParams{0.003f, 0.18f, 0.2f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.3f);
    mod(p, 1, ModSource::Velocity, ModDest::Amp, 0.4f);
    p.fxReverb = 0.05f;
}
void bass_acid(SynthPatch& p)
{
    setName(p, "ACID 303"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f; p.width = 0.25f; p.ampGain = 0.78f;
    osc(p, 0, SAW, 0.0f, 1.0f); p.drift = 0.1f;
    filt(p, FilterMode::LadderLP, 0.25f, 0.42f, 0.6f, 0.45f, 0.2f); // squelch
    p.env[0] = EnvParams{0.004f, 0.25f, 0.4f, 0.12f, 0.0f};
    p.env[1] = EnvParams{0.004f, 0.22f, 0.05f, 0.12f, 0.0f}; // snappy filter env
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    mod(p, 2, ModSource::ModWheel, ModDest::Resonance, 0.3f);
    p.fxDelay = 0.18f; p.fxReverb = 0.12f;
}
void bass_fm(SynthPatch& p)
{
    setName(p, "FM BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.03f; p.width = 0.3f; p.ampGain = 0.8f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 1.0f); p.fm2to1 = 0.5f;
    p.subLevel = 0.3f; p.drift = 0.08f;
    filt(p, FilterMode::SvfLP, 0.6f, 0.1f, 0.2f);
    p.env[0] = EnvParams{0.004f, 0.3f, 0.7f, 0.15f, 0.0f};
    p.env[2] = EnvParams{0.002f, 0.12f, 0.0f, 0.1f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.7f); // DX clang on attack
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.5f);
    p.fxReverb = 0.08f;
}
void bass_growl(SynthPatch& p)
{
    setName(p, "GROWL"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f; p.width = 0.4f; p.ampGain = 0.66f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.15f, 1.0f); osc(p, 2, PULSE, -0.12f, 0.8f);
    p.subLevel = 0.35f; p.fxDrive = 0.3f; p.drift = 0.18f;
    filt(p, FilterMode::LadderLP, 0.35f, 0.35f, 0.4f, 0.5f, 0.3f);
    p.env[0] = EnvParams{0.006f, 0.4f, 0.8f, 0.2f, 0.0f};
    lfoSync(p, 0, LfoWave::Triangle, 7, 1.0f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.4f); // rhythmic growl
    mod(p, 1, ModSource::ModWheel, ModDest::FilterDrive, 0.4f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.1f;
}

// ================================ ARP (3) ================================
void arp_setup(SynthPatch& p, int div, ArpMode m = ArpMode::Up, int oct = 2)
{
    p.arp.on = true; p.arp.mode = (int) m; p.arp.octaves = oct;
    p.arp.syncDiv = div; p.arp.gate = 0.55f; p.arp.swing = 0.08f;
}
void arp_pluck(SynthPatch& p)
{
    setName(p, "ARP PLUCK"); p.category = 3; p.width = 0.6f; p.ampGain = 0.8f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.08f, 0.7f); p.subLevel = 0.1f; p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.25f, 0.55f, 0.2f, 0.4f);
    p.env[0] = EnvParams{0.003f, 0.18f, 0.0f, 0.14f, 0.1f};
    p.env[1] = EnvParams{0.003f, 0.16f, 0.0f, 0.12f, 0.0f};
    arp_setup(p, 4, ArpMode::UpDown);
    lfo(p, 0, LfoWave::Sine, 0.2f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    mod(p, 2, ModSource::Lfo1, ModDest::Cutoff, 0.1f);
    p.fxDelay = 0.28f; p.fxReverb = 0.28f;
}
void arp_bell(SynthPatch& p)
{
    setName(p, "ARP BELL"); p.category = 3; p.width = 0.7f; p.ampGain = 0.74f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.01f, 0.8f, 3); p.fm2to1 = 0.4f; p.ringMod = 0.2f;
    filt(p, FilterMode::SvfLP, 0.7f, 0.05f, 0.15f);
    p.env[0] = EnvParams{0.002f, 0.5f, 0.0f, 0.4f, 0.1f};
    p.env[2] = EnvParams{0.001f, 0.2f, 0.0f, 0.15f, 0.0f};
    arp_setup(p, 4, ArpMode::Up, 3);
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.4f);
    p.fxDelay = 0.3f; p.fxReverb = 0.4f;
}
void arp_saw(SynthPatch& p)
{
    setName(p, "ARP SAW"); p.category = 3; p.width = 0.7f; p.ampGain = 0.66f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 1.0f); osc(p, 3, SAW, -0.1f, 0.6f);
    p.subLevel = 0.12f; p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.2f, 0.4f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.3f, 0.4f, 0.2f, 0.1f};
    p.env[1] = EnvParams{0.004f, 0.25f, 0.2f, 0.18f, 0.0f};
    arp_setup(p, 4, ArpMode::UpDown, 2);
    lfoSync(p, 0, LfoWave::Triangle, 10);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.4f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.2f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.3f; p.fxDelay = 0.3f; p.fxReverb = 0.3f;
}
void arp_pwm(SynthPatch& p)
{
    setName(p, "ARP PWM"); p.category = 3; p.width = 0.65f; p.ampGain = 0.78f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f; osc(p, 1, PULSE, 0.07f, 0.8f);
    p.subLevel = 0.1f; p.drift = 0.2f;
    filt(p, FilterMode::SvfLP, 0.55f, 0.18f, 0.4f, 0.1f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.25f, 0.2f, 0.16f, 0.1f};
    p.env[1] = EnvParams{0.004f, 0.2f, 0.1f, 0.14f, 0.0f};
    arp_setup(p, 4, ArpMode::Random, 2);
    lfo(p, 0, LfoWave::Sine, 0.7f);
    mod(p, 0, ModSource::Lfo1, ModDest::Pw1, 0.45f);
    mod(p, 1, ModSource::EnvFilter, ModDest::Cutoff, 0.4f);
    p.fxDelay = 0.3f; p.fxReverb = 0.3f;
}
void arp_oct(SynthPatch& p)
{
    setName(p, "ARP OCTAVE"); p.category = 3; p.width = 0.6f; p.ampGain = 0.8f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.0f, 0.6f, 1); p.subLevel = 0.18f; p.drift = 0.18f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.22f, 0.5f, 0.2f, 0.5f);
    p.env[0] = EnvParams{0.003f, 0.16f, 0.0f, 0.12f, 0.1f};
    p.env[1] = EnvParams{0.003f, 0.16f, 0.0f, 0.12f, 0.0f};
    arp_setup(p, 7, ArpMode::UpDownInc, 3); p.arp.gate = 0.45f;
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxDelay = 0.32f; p.fxReverb = 0.3f;
}

// ================================ STAB (4) ================================
void stab_house(SynthPatch& p)
{
    setName(p, "HOUSE STAB"); p.category = 4; p.width = 0.7f; p.ampGain = 0.7f; p.chorusMode = 1;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.1f, 0.8f); osc(p, 2, SAW, -0.1f, 0.7f);
    p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.2f, 0.5f, 0.2f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.25f, 0.0f, 0.2f, 0.15f};
    p.env[1] = EnvParams{0.004f, 0.2f, 0.0f, 0.18f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.4f);
    p.fxDelay = 0.22f; p.fxReverb = 0.3f;
}
void stab_rave(SynthPatch& p)
{
    setName(p, "RAVE STAB"); p.category = 4; p.width = 0.8f; p.ampGain = 0.58f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.18f, 0.9f); osc(p, 2, SAW, -0.16f, 0.85f);
    osc(p, 3, SAW, 0.3f, 0.6f); p.subLevel = 0.12f; p.drift = 0.3f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.28f, 0.5f, 0.3f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.4f, 0.2f, 0.3f, 0.15f};
    p.env[1] = EnvParams{0.004f, 0.3f, 0.1f, 0.2f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 5.5f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::ModWheel, ModDest::Osc2Pitch, 0.5f); // hoover-ish pitch sweep
    mod(p, 2, ModSource::Lfo1, ModDest::Pitch, 0.05f);
    p.fxChorus = 0.35f; p.fxDelay = 0.2f; p.fxReverb = 0.3f;
}
void stab_brass(SynthPatch& p)
{
    setName(p, "BRASS"); p.category = 4; p.width = 0.6f; p.ampGain = 0.7f; p.chorusMode = 1;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.08f, 1.0f); p.subLevel = 0.1f; p.drift = 0.22f;
    filt(p, FilterMode::LadderLP, 0.4f, 0.12f, 0.55f, 0.25f, 0.5f);
    p.env[0] = EnvParams{0.05f, 0.3f, 0.85f, 0.25f, 0.2f};
    p.env[1] = EnvParams{0.06f, 0.4f, 0.5f, 0.3f, 0.0f}; // brass swell
    lfo(p, 0, LfoWave::Sine, 5.0f); p.lfo[0].fade = 0.4f;
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::EnvFltAmt, 0.4f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pitch, 0.03f);
    p.fxReverb = 0.28f; p.fxDelay = 0.1f;
}
void stab_organ(SynthPatch& p)
{
    setName(p, "ORGAN STAB"); p.category = 4; p.width = 0.55f; p.ampGain = 0.72f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.7f, 3); osc(p, 2, SINE, 0.0f, 0.5f, 4); // drawbar partials
    p.drift = 0.05f;
    filt(p, FilterMode::SvfLP, 0.8f, 0.05f, 0.1f);
    p.env[0] = EnvParams{0.004f, 0.1f, 0.9f, 0.1f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 6.0f); // leslie-ish
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.03f);
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Rate, 0.5f);
    p.fxChorus = 0.3f; p.fxReverb = 0.22f;
}
void stab_chord(SynthPatch& p)
{
    setName(p, "CHORD STAB"); p.category = 4; p.width = 0.75f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 0.9f); osc(p, 3, SAW, -0.1f, 0.7f);
    p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.15f, 0.4f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.5f, 0.0f, 0.4f, 0.15f};
    p.env[1] = EnvParams{0.004f, 0.4f, 0.0f, 0.3f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.4f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.3f; p.fxDelay = 0.26f; p.fxReverb = 0.4f;
}

// ================================ KEYS (5) ================================
void keys_rhodes(SynthPatch& p)
{
    setName(p, "RHODES"); p.category = 5; p.width = 0.55f; p.ampGain = 0.82f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.9f, 4); p.fm2to1 = 0.3f; // tine bark
    p.drift = 0.06f;
    filt(p, FilterMode::SvfLP, 0.7f, 0.05f, 0.2f);
    p.env[0] = EnvParams{0.003f, 1.4f, 0.35f, 0.5f, 0.3f};
    p.env[2] = EnvParams{0.001f, 0.12f, 0.0f, 0.1f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.5f);   // attack tine
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.6f); // velocity -> bark
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.3f; p.fxReverb = 0.3f; p.fxDelay = 0.06f;
}
void keys_wurli(SynthPatch& p)
{
    setName(p, "WURLI"); p.category = 5; p.width = 0.5f; p.ampGain = 0.82f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.7f, 4); p.fm2to1 = 0.45f;
    p.drift = 0.06f;
    filt(p, FilterMode::SvfLP, 0.66f, 0.06f, 0.2f);
    p.env[0] = EnvParams{0.003f, 1.0f, 0.3f, 0.4f, 0.35f};
    p.env[2] = EnvParams{0.001f, 0.1f, 0.0f, 0.08f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.6f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.7f);
    p.fxChorus = 0.2f; p.fxReverb = 0.25f;
}
void keys_dx(SynthPatch& p)
{
    setName(p, "DX EP"); p.category = 5; p.width = 0.6f; p.ampGain = 0.8f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 1.0f, 3); p.fm2to1 = 0.4f; p.ringMod = 0.1f;
    p.drift = 0.05f;
    filt(p, FilterMode::SvfLP, 0.75f, 0.04f, 0.1f);
    p.env[0] = EnvParams{0.002f, 1.6f, 0.2f, 0.6f, 0.3f};
    p.env[2] = EnvParams{0.001f, 0.15f, 0.0f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.6f);
    p.fxChorus = 0.3f; p.fxReverb = 0.35f; p.fxDelay = 0.1f;
}
void keys_clav(SynthPatch& p)
{
    setName(p, "CLAV"); p.category = 5; p.width = 0.45f; p.ampGain = 0.78f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.35f; osc(p, 1, SAW, 0.06f, 0.6f);
    p.drift = 0.08f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.3f, 0.45f, 0.25f, 0.5f);
    p.env[0] = EnvParams{0.003f, 0.2f, 0.3f, 0.12f, 0.2f};
    p.env[1] = EnvParams{0.003f, 0.16f, 0.0f, 0.1f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.45f);
    p.fxPhaser = 0.3f; p.fxDelay = 0.12f; p.fxReverb = 0.15f;
}
void keys_poly(SynthPatch& p)
{
    setName(p, "POLY KEYS"); p.category = 5; p.width = 0.7f; p.ampGain = 0.74f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.07f, 0.7f); p.osc[1].pw = 0.4f;
    p.subLevel = 0.1f; p.drift = 0.18f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.12f, 0.35f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.01f, 0.6f, 0.6f, 0.4f, 0.25f};
    p.env[1] = EnvParams{0.02f, 0.5f, 0.3f, 0.3f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 0.3f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.35f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pw2, 0.2f);
    p.fxChorus = 0.3f; p.fxReverb = 0.3f; p.fxDelay = 0.1f;
}

// ================================ PLUCK (6) ================================
void pluck_synth(SynthPatch& p)
{
    setName(p, "SYNTH PLUCK"); p.category = 6; p.width = 0.6f; p.ampGain = 0.82f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.1f, 0.6f); p.subLevel = 0.12f; p.drift = 0.15f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.22f, 0.6f, 0.2f, 0.5f);
    p.env[0] = EnvParams{0.002f, 0.22f, 0.0f, 0.16f, 0.15f};
    p.env[1] = EnvParams{0.002f, 0.18f, 0.0f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.4f);
    p.fxDelay = 0.24f; p.fxReverb = 0.3f;
}
void pluck_koto(SynthPatch& p)
{
    setName(p, "KOTO"); p.category = 6; p.width = 0.5f; p.ampGain = 0.8f;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SAW, 0.0f, 0.4f); p.ringMod = 0.12f; p.drift = 0.1f;
    filt(p, FilterMode::SvfLP, 0.6f, 0.18f, 0.5f, 0.0f, 0.5f);
    p.env[0] = EnvParams{0.002f, 0.4f, 0.0f, 0.3f, 0.2f};
    p.env[1] = EnvParams{0.002f, 0.25f, 0.0f, 0.18f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.4f);
    p.fxDelay = 0.2f; p.fxReverb = 0.35f;
}
void pluck_bell(SynthPatch& p)
{
    setName(p, "BELL PLUCK"); p.category = 6; p.width = 0.65f; p.ampGain = 0.74f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.9f, 4); p.fm2to1 = 0.45f; p.ringMod = 0.25f;
    filt(p, FilterMode::SvfLP, 0.75f, 0.04f, 0.2f);
    p.env[0] = EnvParams{0.001f, 0.5f, 0.0f, 0.4f, 0.15f};
    p.env[2] = EnvParams{0.001f, 0.15f, 0.0f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.5f);
    p.fxDelay = 0.28f; p.fxReverb = 0.45f;
}
void pluck_mallet(SynthPatch& p)
{
    setName(p, "MALLET"); p.category = 6; p.width = 0.5f; p.ampGain = 0.8f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.5f, 4); p.drift = 0.05f;
    filt(p, FilterMode::Lpg, 0.85f, 0.05f, 0.6f);   // vactrol low-pass gate pluck
    p.env[1] = EnvParams{0.001f, 0.22f, 0.0f, 0.18f, 0.0f}; // fast filter env drives the LPG duck
    p.env[0] = EnvParams{0.001f, 0.3f, 0.0f, 0.25f, 0.2f};
    p.env[2] = EnvParams{0.001f, 0.05f, 0.0f, 0.05f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.15f); // tiny attack pitch click
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.4f);
    p.fxReverb = 0.4f;
}
void pluck_harp(SynthPatch& p)
{
    setName(p, "HARP"); p.category = 6; p.width = 0.6f; p.ampGain = 0.78f; p.chorusMode = 1;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SINE, 0.04f, 0.7f); p.drift = 0.08f;
    filt(p, FilterMode::SvfLP, 0.66f, 0.08f, 0.4f, 0.0f, 0.5f);
    p.env[0] = EnvParams{0.002f, 0.6f, 0.0f, 0.5f, 0.2f};
    p.env[1] = EnvParams{0.002f, 0.3f, 0.0f, 0.2f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.35f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    p.fxDelay = 0.22f; p.fxReverb = 0.45f;
}

// ================================ FX (7) ================================
void fx_riser(SynthPatch& p)
{
    setName(p, "RISER"); p.category = 7; p.width = 0.9f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.2f, 1.0f); p.noiseLevel = 0.2f; p.drift = 0.4f;
    filt(p, FilterMode::SvfBP, 0.3f, 0.4f, 0.1f);
    p.env[0] = EnvParams{2.0f, 1.0f, 1.0f, 0.5f, 0.0f};
    p.env[2] = EnvParams{3.0f, 1.0f, 1.0f, 0.5f, 0.0f}; // slow rising aux env
    lfo(p, 0, LfoWave::Sine, 4.0f);
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.0f);   // pitch rise
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.6f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pitch, 0.06f);
    mod(p, 3, ModSource::EnvAux, ModDest::Lfo1Rate, 1.5f); // accelerating wobble
    p.fxReverb = 0.5f; p.fxDelay = 0.3f;
}
void fx_noise(SynthPatch& p)
{
    setName(p, "NOISE SWEEP"); p.category = 7; p.width = 0.9f; p.ampGain = 0.55f;
    p.noiseLevel = 0.8f; p.osc[0].on = false; p.osc[1].on = false; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.3f, 0.5f, 0.0f);
    p.env[0] = EnvParams{0.5f, 1.0f, 0.8f, 1.0f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 0.15f, 1.0f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 1.2f);   // big slow sweep
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 1.0f);
    mod(p, 2, ModSource::ModWheel, ModDest::Resonance, 0.4f);
    p.fxReverb = 0.6f; p.fxDelay = 0.3f;
}
void fx_siren(SynthPatch& p)
{
    setName(p, "SIREN"); p.category = 7; p.width = 0.7f; p.ampGain = 0.7f; p.voiceMode = (int) VoiceMode::Mono;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.0f, 0.6f); p.drift = 0.1f;
    filt(p, FilterMode::SvfBP, 0.5f, 0.3f, 0.1f);
    p.env[0] = EnvParams{0.05f, 0.5f, 1.0f, 0.3f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 1.2f, 1.0f);
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.18f); // wailing pitch (tamed)
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Rate, 1.0f);
    mod(p, 2, ModSource::Lfo1, ModDest::Cutoff, 0.3f);
    p.fxDelay = 0.4f; p.fxReverb = 0.4f;
}
void fx_zap(SynthPatch& p)
{
    setName(p, "ZAP"); p.category = 7; p.width = 0.5f; p.ampGain = 0.8f;
    osc(p, 0, SAW, 0.0f, 1.0f); p.drift = 0.0f;
    filt(p, FilterMode::SvfHP, 0.4f, 0.3f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.12f, 0.0f, 0.08f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.06f, 0.0f, 0.05f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 2.0f); // fast downward zap
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.5f);
    p.fxDelay = 0.3f; p.fxReverb = 0.3f;
}
void fx_drop(SynthPatch& p)
{
    setName(p, "DOWNLIFTER"); p.category = 7; p.width = 0.9f; p.ampGain = 0.6f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.3f, 0.8f); p.noiseLevel = 0.15f; p.drift = 0.5f;
    filt(p, FilterMode::SvfLP, 0.6f, 0.2f, 0.0f);
    p.env[0] = EnvParams{0.01f, 3.0f, 0.0f, 0.5f, 0.0f};
    p.env[2] = EnvParams{0.01f, 3.0f, 0.0f, 0.5f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, -1.5f); // falling
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.8f);
    mod(p, 2, ModSource::EnvAux, ModDest::Detune, 1.0f);
    p.fxReverb = 0.55f; p.fxDelay = 0.35f;
}

// ================================ DRONE (8) ================================
void drone_dark(SynthPatch& p)
{
    setName(p, "DARK DRONE"); p.category = 8; p.width = 0.85f; p.ampGain = 0.7f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.04f, 0.9f, 1); osc(p, 3, TRI, -0.05f, 0.6f, 1);
    p.subLevel = 0.4f; p.subOctave = 2; p.drift = 0.4f;
    filt(p, FilterMode::LadderLP, 0.25f, 0.2f, 0.2f, 0.3f, 0.2f);
    p.env[0] = EnvParams{2.0f, 2.0f, 1.0f, 3.0f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.05f, 1.0f, 0.1f); lfo(p, 1, LfoWave::Triangle, 0.03f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.15f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.4f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.4f);
    p.fxReverb = 0.6f;
}
void drone_sub(SynthPatch& p)
{
    setName(p, "SUB DRONE"); p.category = 8; p.width = 0.6f; p.ampGain = 0.8f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.03f, 0.5f); p.subLevel = 0.6f; p.subOctave = 2; p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.3f, 0.1f, 0.15f, 0.2f, 0.1f);
    p.env[0] = EnvParams{1.5f, 2.0f, 1.0f, 2.5f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.04f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.1f);
    mod(p, 1, ModSource::ModWheel, ModDest::SubLevel, 0.3f);
    p.fxReverb = 0.5f;
}
void drone_harm(SynthPatch& p)
{
    setName(p, "HARMONIC"); p.category = 8; p.width = 0.85f; p.ampGain = 0.66f; p.chorusMode = 2;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.7f, 3); osc(p, 2, SINE, 0.0f, 0.5f, 4);
    osc(p, 3, SINE, 0.02f, 0.4f, 1); p.drift = 0.15f;
    filt(p, FilterMode::SvfLP, 0.6f, 0.05f, 0.1f);
    p.env[0] = EnvParams{2.5f, 2.0f, 1.0f, 3.0f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.07f); lfo(p, 1, LfoWave::Triangle, 0.05f);
    mod(p, 0, ModSource::Lfo1, ModDest::OscMix, 0.4f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.7f; p.fxDelay = 0.2f;
}
void drone_metal(SynthPatch& p)
{
    setName(p, "METAL DRONE"); p.category = 8; p.width = 0.8f; p.ampGain = 0.6f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.0f, 1.0f); p.ringMod = 0.4f; p.fm2to1 = 0.2f; p.drift = 0.3f;
    filt(p, FilterMode::SvfBP, 0.5f, 0.4f, 0.1f);
    p.env[0] = EnvParams{1.5f, 2.0f, 1.0f, 2.5f, 0.0f};
    lfo(p, 0, LfoWave::SampleHold, 0.2f); lfo(p, 1, LfoWave::Sine, 0.08f);
    mod(p, 0, ModSource::Lfo1, ModDest::RingMod, 0.4f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.3f);
    mod(p, 2, ModSource::Lfo1, ModDest::Osc2Pitch, 0.2f);
    p.fxReverb = 0.6f; p.fxDelay = 0.3f;
}
void drone_evolve(SynthPatch& p)
{
    setName(p, "EVOLVING"); p.category = 8; p.width = 0.9f; p.ampGain = 0.62f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.06f, 0.9f); osc(p, 3, SAW, -0.08f, 0.6f);
    p.subLevel = 0.2f; p.drift = 0.45f;
    filt(p, FilterMode::LadderLP, 0.4f, 0.2f, 0.2f, 0.2f, 0.3f);
    p.env[0] = EnvParams{2.0f, 2.0f, 1.0f, 3.0f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.05f); lfo(p, 1, LfoWave::Triangle, 0.03f); lfo(p, 2, LfoWave::SampleHold, 0.1f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.2f);
    mod(p, 1, ModSource::Lfo2, ModDest::Pw2, 0.4f);
    mod(p, 2, ModSource::Lfo3, ModDest::Detune, 0.3f);
    mod(p, 3, ModSource::Lfo2, ModDest::OscMix, 0.3f);
    p.fxReverb = 0.65f; p.fxDelay = 0.3f;
}

// ================================ PERC (9) ================================
void perc_kick(SynthPatch& p)
{
    setName(p, "KICK"); p.category = 9; p.width = 0.3f; p.ampGain = 0.9f; p.fxDrive = 0.2f;
    osc(p, 0, SINE, 0.0f, 1.0f); p.drift = 0.0f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.0f, 0.0f, 0.2f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.32f, 0.0f, 0.1f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.05f, 0.0f, 0.04f, 0.0f}; // fast pitch env
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 2.0f);    // punchy drop
    p.fxReverb = 0.0f;
}
void perc_snare(SynthPatch& p)
{
    setName(p, "SNARE"); p.category = 9; p.width = 0.4f; p.ampGain = 0.78f;
    osc(p, 0, TRI, 0.0f, 0.7f); p.noiseLevel = 0.7f; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.55f, 0.2f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.18f, 0.0f, 0.1f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.04f, 0.0f, 0.03f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.2f);
    mod(p, 1, ModSource::EnvAmp, ModDest::NoiseLevel, 0.3f);
    p.fxReverb = 0.18f;
}
void perc_tom(SynthPatch& p)
{
    setName(p, "TOM"); p.category = 9; p.width = 0.4f; p.ampGain = 0.84f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.3f); p.noiseLevel = 0.05f; p.drift = 0.0f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.1f, 0.0f, 0.15f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.4f, 0.0f, 0.15f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.12f, 0.0f, 0.08f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.4f);
    p.fxReverb = 0.15f;
}
void perc_clap(SynthPatch& p)
{
    setName(p, "CLAP"); p.category = 9; p.width = 0.5f; p.ampGain = 0.72f;
    p.osc[0].on = false; p.noiseLevel = 0.9f; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.6f, 0.35f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.12f, 0.05f, 0.12f, 0.0f};
    lfo(p, 0, LfoWave::Square, 60.0f); // fast gate -> clap stutter
    mod(p, 0, ModSource::Lfo1, ModDest::Amp, 0.6f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.4f);
    p.fxReverb = 0.25f;
}
void perc_cowbell(SynthPatch& p)
{
    setName(p, "COWBELL"); p.category = 9; p.width = 0.4f; p.ampGain = 0.7f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.4f; osc(p, 1, PULSE, 7.0f, 0.8f); p.ringMod = 0.3f; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.7f, 0.2f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.25f, 0.0f, 0.15f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Amp, 0.3f);
    p.fxReverb = 0.18f;
}

// ================================ AMB (10) ================================
void amb_glacier(SynthPatch& p)
{
    setName(p, "GLACIER"); p.category = 10; p.width = 0.92f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, 0.08f, 0.9f); osc(p, 3, SAW, -0.12f, 0.5f, 3);
    p.drift = 0.5f;
    filt(p, FilterMode::SvfLP, 0.45f, 0.15f, 0.15f);
    p.env[0] = EnvParams{2.2f, 2.0f, 0.9f, 3.6f, 0.15f};
    lfo(p, 0, LfoWave::Sine, 0.05f); lfo(p, 1, LfoWave::Triangle, 0.08f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.2f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.5f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.4f);
    p.fxReverb = 0.78f; p.fxDelay = 0.3f;
}
void amb_shimmer(SynthPatch& p)
{
    setName(p, "SHIMMER"); p.category = 10; p.width = 0.92f; p.ampGain = 0.58f; p.chorusMode = 2;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.7f, 3); osc(p, 3, TRI, 0.05f, 0.4f, 4);
    p.fm2to1 = 0.15f; p.drift = 0.3f;
    filt(p, FilterMode::SvfLP, 0.6f, 0.1f, 0.15f);
    p.env[0] = EnvParams{1.8f, 2.0f, 0.9f, 3.4f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 0.06f); lfo(p, 1, LfoWave::Triangle, 0.04f);
    mod(p, 0, ModSource::Lfo1, ModDest::OscMix, 0.3f);
    mod(p, 1, ModSource::Lfo2, ModDest::FmAmount, 0.2f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.82f; p.fxDelay = 0.4f;
}
void amb_choir(SynthPatch& p)
{
    setName(p, "VOX CHOIR"); p.category = 10; p.width = 0.9f; p.ampGain = 0.5f; p.chorusMode = 2;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    osc(p, 1, PULSE, 0.08f, 1.0f); p.osc[1].pw = 0.44f;
    osc(p, 3, PULSE, -0.1f, 0.7f); p.osc[3].pw = 0.52f;
    osc(p, 4, SAW, 0.13f, 0.35f, 3); p.drift = 0.3f;
    filt(p, FilterMode::SvfBP, 0.48f, 0.4f, 0.12f);
    p.env[0] = EnvParams{1.4f, 1.8f, 0.92f, 3.0f, 0.2f};
    lfo(p, 0, LfoWave::Sine, 0.07f); lfo(p, 1, LfoWave::Triangle, 0.2f); lfo(p, 2, LfoWave::Sine, 0.16f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.26f);
    mod(p, 1, ModSource::Lfo2, ModDest::Pw1, 0.24f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pw2, 0.24f);
    mod(p, 3, ModSource::Lfo1, ModDest::Detune, 0.16f);
    mod(p, 4, ModSource::ModWheel, ModDest::Resonance, 0.2f);
    p.fxChorus = 0.5f; p.fxReverb = 0.72f;
}
void amb_deep(SynthPatch& p)
{
    setName(p, "DEEP SPACE"); p.category = 10; p.width = 0.95f; p.ampGain = 0.56f; p.chorusMode = 2;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SAW, 0.1f, 0.5f, 1); osc(p, 3, SINE, -0.06f, 0.4f, 4);
    p.subLevel = 0.2f; p.ringMod = 0.12f; p.drift = 0.45f;
    filt(p, FilterMode::SvfLP, 0.4f, 0.2f, 0.12f);
    p.env[0] = EnvParams{2.6f, 2.2f, 0.88f, 4.0f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 0.04f); lfo(p, 1, LfoWave::SampleHold, 0.08f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.2f);
    mod(p, 1, ModSource::Lfo2, ModDest::RingMod, 0.2f);
    mod(p, 2, ModSource::Lfo2, ModDest::Detune, 0.3f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxReverb = 0.84f; p.fxDelay = 0.35f;
}
void amb_aurora(SynthPatch& p)
{
    setName(p, "AURORA"); p.category = 10; p.width = 0.92f; p.ampGain = 0.58f; p.chorusMode = 2;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 0.8f); osc(p, 3, SAW, -0.14f, 0.5f);
    osc(p, 4, SINE, 0.06f, 0.4f, 3); p.drift = 0.4f;
    filt(p, FilterMode::SvfLP, 0.5f, 0.16f, 0.18f);
    p.env[0] = EnvParams{2.0f, 2.0f, 0.9f, 3.4f, 0.15f};
    lfo(p, 0, LfoWave::Sine, 0.06f, 1.0f, 0.1f); lfo(p, 1, LfoWave::Triangle, 0.09f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.22f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.4f);
    mod(p, 2, ModSource::Lfo2, ModDest::Morph1, 0.2f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxReverb = 0.78f; p.fxDelay = 0.3f;
}

// ================================ SEQ (11) ================================
void seq_trance(SynthPatch& p)
{
    setName(p, "TRANCEGATE"); p.category = 11; p.width = 0.8f; p.ampGain = 0.62f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 1.0f); osc(p, 3, SAW, -0.1f, 0.6f);
    p.subLevel = 0.12f; p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.15f, 0.2f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.4f, 1.0f, 0.9f, 0.6f, 0.0f};
    lfoSync(p, 0, LfoWave::Square, 4, 1.0f); lfo(p, 1, LfoWave::Sine, 0.2f);
    mod(p, 0, ModSource::Lfo1, ModDest::Amp, 0.9f);    // the gate
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.2f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.3f; p.fxDelay = 0.3f; p.fxReverb = 0.4f;
}
void seq_acid(SynthPatch& p)
{
    setName(p, "ACID SEQ"); p.category = 11; p.width = 0.4f; p.ampGain = 0.74f;
    osc(p, 0, SAW, 0.0f, 1.0f); p.drift = 0.1f;
    filt(p, FilterMode::LadderLP, 0.3f, 0.4f, 0.5f, 0.4f, 0.3f);
    p.env[0] = EnvParams{0.004f, 0.25f, 0.3f, 0.12f, 0.1f};
    p.env[1] = EnvParams{0.004f, 0.2f, 0.05f, 0.12f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::Up; p.arp.octaves = 2; p.arp.syncDiv = 4; p.arp.gate = 0.6f;
    lfoSync(p, 0, LfoWave::Triangle, 10);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.25f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxDelay = 0.3f; p.fxReverb = 0.2f;
}
void seq_wobble(SynthPatch& p)
{
    setName(p, "WOBBLE"); p.category = 11; p.width = 0.6f; p.ampGain = 0.66f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.15f, 1.0f); osc(p, 2, PULSE, -0.12f, 0.8f);
    p.subLevel = 0.3f; p.fxDrive = 0.25f; p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.35f, 0.4f, 0.3f, 0.4f, 0.3f);
    p.env[0] = EnvParams{0.01f, 0.5f, 0.9f, 0.2f, 0.0f};
    lfoSync(p, 0, LfoWave::Sine, 7, 1.0f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.6f);   // the wob
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Rate, 0.6f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.2f);
    p.fxReverb = 0.2f;
}
void seq_blade(SynthPatch& p)
{
    setName(p, "BLADE"); p.category = 11; p.width = 0.9f; p.ampGain = 0.56f; p.chorusMode = 2;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f; osc(p, 1, SAW, 0.12f, 1.0f);
    osc(p, 2, SAW, -0.1f, 0.8f); osc(p, 3, SAW, 0.22f, 0.6f); osc(p, 4, SAW, -0.05f, 0.5f, 1);
    p.subLevel = 0.1f; p.drift = 0.3f;
    filt(p, FilterMode::SvfLP, 0.55f, 0.24f, 0.2f, 0.0f, 0.4f);
    p.env[0] = EnvParams{0.012f, 0.6f, 0.88f, 0.4f, 0.1f};
    lfoSync(p, 0, LfoWave::Triangle, 9); lfoSync(p, 1, LfoWave::Sine, 11);
    mod(p, 0, ModSource::Lfo1, ModDest::Pw1, 0.45f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.35f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.4f; p.fxPhaser = 0.2f; p.fxReverb = 0.34f; p.fxDelay = 0.2f;
}
void seq_hoover(SynthPatch& p)
{
    setName(p, "HOOVER"); p.category = 11; p.width = 0.78f; p.ampGain = 0.6f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.18f, 0.9f); p.osc[1].pw = 0.4f;
    osc(p, 2, SAW, -0.16f, 0.85f); osc(p, 3, SAW, 0.3f, 0.7f); osc(p, 4, PULSE, -0.28f, 0.6f);
    p.subLevel = 0.14f; p.drift = 0.3f;
    filt(p, FilterMode::LadderLP, 0.4f, 0.34f, 0.35f, 0.4f, 0.4f);
    p.env[0] = EnvParams{0.01f, 0.4f, 0.85f, 0.3f, 0.1f};
    lfoSync(p, 0, LfoWave::Triangle, 12); lfo(p, 1, LfoWave::Sine, 6.0f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::ModWheel, ModDest::Osc2Pitch, 0.4f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    p.fxChorus = 0.3f; p.fxDelay = 0.28f; p.fxReverb = 0.3f;
}

// ===================== second wave (to 128) =====================
// ---- PAD ----
void pad_sweep(SynthPatch& p){ setName(p,"SWEEP PAD"); p.category=0; p.width=0.85f; p.ampGain=0.7f; p.chorusMode=3;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.14f,0.9f); osc(p,3,PULSE,-0.1f,0.5f); p.drift=0.3f;
    filt(p,FilterMode::LadderLP,0.3f,0.3f,0.5f,0.2f,0.3f); p.env[0]=EnvParams{1.0f,1.6f,0.85f,2.0f,0.2f};
    p.env[1]=EnvParams{2.2f,2.0f,0.3f,1.8f,0}; lfo(p,0,LfoWave::Sine,0.12f,1,0.1f);
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.5f); mod(p,1,ModSource::Lfo1,ModDest::Cutoff,0.15f);
    mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.4f); p.fxReverb=0.45f; p.fxDelay=0.12f; }
void pad_fm(SynthPatch& p){ setName(p,"FM PAD"); p.category=0; p.width=0.82f; p.ampGain=0.74f; p.chorusMode=1;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0.04f,1.0f,3); p.fm2to1=0.25f; p.drift=0.2f;
    filt(p,FilterMode::SvfLP,0.6f,0.08f,0.2f); p.env[0]=EnvParams{1.2f,2.0f,0.8f,2.6f,0.2f};
    lfo(p,0,LfoWave::Sine,0.1f); lfo(p,1,LfoWave::Triangle,0.07f);
    mod(p,0,ModSource::Lfo1,ModDest::FmAmount,0.3f); mod(p,1,ModSource::Lfo2,ModDest::Detune,0.3f);
    mod(p,2,ModSource::ModWheel,ModDest::FmAmount,0.4f); p.fxReverb=0.55f; p.fxChorus=0.2f; }
void pad_air(SynthPatch& p){ setName(p,"AIR PAD"); p.category=0; p.width=0.92f; p.ampGain=0.6f; p.chorusMode=3;
    osc(p,0,TRI,0,1.0f,3); osc(p,1,SINE,0.06f,0.7f,4); osc(p,3,SAW,0.1f,0.3f,3); p.noiseLevel=0.06f; p.drift=0.3f;
    filt(p,FilterMode::SvfHP,0.3f,0.1f,0.1f); p.env[0]=EnvParams{1.6f,2.0f,0.85f,3.0f,0.15f};
    lfo(p,0,LfoWave::Sine,0.08f); mod(p,0,ModSource::Lfo1,ModDest::Detune,0.4f);
    mod(p,1,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.8f; p.fxDelay=0.25f; }
void pad_jp(SynthPatch& p){ setName(p,"JP BRASS"); p.category=0; p.width=0.8f; p.ampGain=0.68f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.1f,1.0f); osc(p,2,PULSE,-0.08f,0.7f); p.subLevel=0.12f; p.drift=0.25f;
    filt(p,FilterMode::LadderLP,0.42f,0.14f,0.5f,0.25f,0.4f); p.env[0]=EnvParams{0.3f,1.0f,0.85f,0.8f,0.2f};
    p.env[1]=EnvParams{0.4f,1.0f,0.5f,0.8f,0}; lfo(p,0,LfoWave::Sine,4.5f); p.lfo[0].fade=0.5f;
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.5f); mod(p,1,ModSource::Lfo1,ModDest::Pitch,0.03f);
    mod(p,2,ModSource::Aftertouch,ModDest::Cutoff,0.3f); p.fxChorus=0.3f; p.fxReverb=0.35f; }
void pad_oct(SynthPatch& p){ setName(p,"OCTAVE PAD"); p.category=0; p.width=0.85f; p.ampGain=0.66f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.08f,0.8f,1); osc(p,3,SAW,-0.06f,0.6f,3); p.subLevel=0.18f; p.drift=0.3f;
    filt(p,FilterMode::SvfLP,0.5f,0.12f,0.25f); p.env[0]=EnvParams{0.9f,1.6f,0.88f,2.0f,0.2f};
    lfo(p,0,LfoWave::Triangle,0.15f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.18f);
    mod(p,1,ModSource::Lfo1,ModDest::Detune,0.3f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.35f);
    p.fxChorus=0.3f; p.fxReverb=0.4f; }
void pad_wide(SynthPatch& p){ setName(p,"WIDE SAWS"); p.category=0; p.width=0.95f; p.ampGain=0.6f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.2f,0.9f); osc(p,2,SAW,-0.18f,0.9f); osc(p,3,SAW,0.32f,0.6f); osc(p,4,SAW,-0.3f,0.6f);
    p.drift=0.4f; filt(p,FilterMode::SvfLP,0.55f,0.1f,0.2f); p.env[0]=EnvParams{0.7f,1.6f,0.9f,2.2f,0.2f};
    lfo(p,0,LfoWave::Sine,0.3f); lfo(p,1,LfoWave::Triangle,0.21f);
    mod(p,0,ModSource::Lfo1,ModDest::Detune,0.5f); mod(p,1,ModSource::Lfo2,ModDest::Detune,0.4f);
    mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.35f); p.fxChorus=0.4f; p.fxReverb=0.45f; }
void pad_mellow(SynthPatch& p){ setName(p,"MELLOW"); p.category=0; p.width=0.7f; p.ampGain=0.82f; p.chorusMode=1;
    osc(p,0,SINE,0,1.0f); osc(p,1,TRI,0.05f,0.8f); p.subLevel=0.15f; p.drift=0.15f;
    filt(p,FilterMode::SvfLP,0.5f,0.06f,0.2f); p.env[0]=EnvParams{0.6f,1.4f,0.9f,1.8f,0.2f};
    lfo(p,0,LfoWave::Sine,0.18f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.12f);
    mod(p,1,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxChorus=0.2f; p.fxReverb=0.4f; }
// ---- LEAD ----
void lead_saw(SynthPatch& p){ setName(p,"SAW LEAD"); p.category=1; p.width=0.5f; p.ampGain=0.8f; p.voiceMode=(int)VoiceMode::Mono;
    p.glideMode=(int)GlideMode::Legato; p.glideTime=0.04f; osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.06f,0.8f); p.subLevel=0.12f; p.drift=0.12f;
    filt(p,FilterMode::LadderLP,0.6f,0.18f,0.4f,0.2f,0.4f); p.env[0]=EnvParams{0.006f,0.3f,0.8f,0.25f,0.1f};
    lfo(p,0,LfoWave::Sine,5.0f); p.lfo[0].fade=0.4f; mod(p,0,ModSource::Lfo1,ModDest::Pitch,0.04f);
    mod(p,1,ModSource::ModWheel,ModDest::Cutoff,0.4f); mod(p,2,ModSource::Aftertouch,ModDest::Lfo1Depth,0.6f);
    p.fxDelay=0.2f; p.fxReverb=0.22f; }
void lead_square(SynthPatch& p){ setName(p,"SQUARE LEAD"); p.category=1; p.width=0.5f; p.ampGain=0.76f; p.voiceMode=(int)VoiceMode::Mono;
    p.glideMode=(int)GlideMode::Legato; p.glideTime=0.04f; osc(p,0,PULSE,0,1.0f); p.osc[0].pw=0.5f; osc(p,1,PULSE,0.04f,0.6f); p.subLevel=0.15f;
    filt(p,FilterMode::LadderLP,0.65f,0.14f,0.35f,0.15f,0.4f); p.env[0]=EnvParams{0.006f,0.3f,0.85f,0.25f,0.1f};
    lfo(p,0,LfoWave::Triangle,4.5f); mod(p,0,ModSource::Lfo1,ModDest::Pitch,0.04f);
    mod(p,1,ModSource::ModWheel,ModDest::Cutoff,0.4f); p.fxDelay=0.22f; p.fxReverb=0.2f; }
void lead_pluck(SynthPatch& p){ setName(p,"PLUCK LEAD"); p.category=1; p.width=0.55f; p.ampGain=0.82f;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0.08f,0.6f); p.drift=0.15f; filt(p,FilterMode::LadderLP,0.5f,0.25f,0.6f,0.2f,0.5f);
    p.env[0]=EnvParams{0.002f,0.25f,0.2f,0.2f,0.15f}; p.env[1]=EnvParams{0.002f,0.18f,0.0f,0.14f,0};
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.55f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.4f);
    p.fxDelay=0.26f; p.fxReverb=0.3f; }
void lead_fifth(SynthPatch& p){ setName(p,"FIFTHS"); p.category=1; p.width=0.55f; p.ampGain=0.74f; p.voiceMode=(int)VoiceMode::Mono;
    p.glideMode=(int)GlideMode::Legato; p.glideTime=0.05f; osc(p,0,SAW,0,1.0f); osc(p,1,SAW,7.02f,0.8f); osc(p,2,SAW,0.04f,0.6f,3); p.drift=0.15f;
    filt(p,FilterMode::LadderLP,0.55f,0.18f,0.4f,0.25f,0.4f); p.env[0]=EnvParams{0.006f,0.3f,0.85f,0.25f,0.1f};
    mod(p,0,ModSource::ModWheel,ModDest::Cutoff,0.4f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.3f);
    p.fxDelay=0.22f; p.fxReverb=0.25f; }
void lead_dist(SynthPatch& p){ setName(p,"DIST LEAD"); p.category=1; p.width=0.5f; p.ampGain=0.6f; p.voiceMode=(int)VoiceMode::Mono;
    p.glideMode=(int)GlideMode::Legato; p.glideTime=0.04f; osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.1f,1.0f); p.fxDrive=0.5f; p.subLevel=0.1f; p.drift=0.18f;
    filt(p,FilterMode::LadderLP,0.5f,0.28f,0.4f,0.5f,0.4f); p.env[0]=EnvParams{0.006f,0.3f,0.85f,0.25f,0.1f};
    lfo(p,0,LfoWave::Sine,5.5f); p.lfo[0].fade=0.3f; mod(p,0,ModSource::Lfo1,ModDest::Pitch,0.05f);
    mod(p,1,ModSource::ModWheel,ModDest::FilterDrive,0.4f); mod(p,2,ModSource::Velocity,ModDest::Cutoff,0.3f);
    p.fxDelay=0.2f; p.fxReverb=0.25f; }
void lead_whistle(SynthPatch& p){ setName(p,"WHISTLE"); p.category=1; p.width=0.4f; p.ampGain=0.82f; p.voiceMode=(int)VoiceMode::Mono;
    p.glideMode=(int)GlideMode::Always; p.glideTime=0.08f; osc(p,0,SINE,0,1.0f); p.drift=0.05f;
    filt(p,FilterMode::SvfLP,0.85f,0.05f,0.05f); p.env[0]=EnvParams{0.05f,0.3f,0.95f,0.3f,0.05f};
    lfo(p,0,LfoWave::Sine,5.5f); p.lfo[0].fade=0.5f; p.lfo[0].delay=0.3f; mod(p,0,ModSource::Lfo1,ModDest::Pitch,0.06f);
    mod(p,1,ModSource::Aftertouch,ModDest::Lfo1Depth,0.8f); p.fxDelay=0.25f; p.fxReverb=0.35f; }
void lead_uni(SynthPatch& p){ setName(p,"UNISON LD"); p.category=1; p.width=0.6f; p.ampGain=0.45f; p.chorusMode=2;
    p.voiceMode=(int)VoiceMode::Unison; p.unisonCount=5; p.unisonDetune=0.16f; p.unisonSpread=0.7f;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0.05f,0.8f); p.subLevel=0.1f; p.drift=0.25f;
    filt(p,FilterMode::LadderLP,0.6f,0.12f,0.35f,0.15f,0.45f); p.env[0]=EnvParams{0.01f,0.4f,0.85f,0.3f,0.1f};
    mod(p,0,ModSource::ModWheel,ModDest::Cutoff,0.45f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.3f);
    p.fxChorus=0.3f; p.fxDelay=0.22f; p.fxReverb=0.28f; }
// ---- BASS ----
void bass_pluck(SynthPatch& p){ setName(p,"PLUCK BASS"); p.category=2; p.width=0.3f; p.ampGain=0.84f; p.voiceMode=(int)VoiceMode::Mono;
    osc(p,0,SAW,0,1.0f); osc(p,1,SINE,0,0.5f,1); p.subLevel=0.3f; p.drift=0.1f;
    filt(p,FilterMode::LadderLP,0.4f,0.25f,0.55f,0.25f,0.3f); p.env[0]=EnvParams{0.002f,0.2f,0.0f,0.14f,0.1f};
    p.env[1]=EnvParams{0.002f,0.16f,0.0f,0.1f,0}; mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.55f);
    mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.4f); p.fxReverb=0.08f; }
void bass_square(SynthPatch& p){ setName(p,"SQ BASS"); p.category=2; p.width=0.3f; p.ampGain=0.82f; p.voiceMode=(int)VoiceMode::Mono;
    p.glideMode=(int)GlideMode::Legato; p.glideTime=0.04f; osc(p,0,PULSE,0,1.0f); p.osc[0].pw=0.4f; osc(p,1,SINE,0,0.5f,1); p.subLevel=0.4f; p.drift=0.08f;
    filt(p,FilterMode::LadderLP,0.4f,0.15f,0.3f,0.25f,0.2f); p.env[0]=EnvParams{0.004f,0.3f,0.85f,0.15f,0};
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.3f); mod(p,1,ModSource::Velocity,ModDest::Amp,0.3f); p.fxReverb=0.06f; }
void bass_dirty(SynthPatch& p){ setName(p,"DIRTY BASS"); p.category=2; p.width=0.35f; p.ampGain=0.6f; p.voiceMode=(int)VoiceMode::Mono;
    p.glideMode=(int)GlideMode::Legato; p.glideTime=0.04f; osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.12f,0.9f); osc(p,2,PULSE,-0.1f,0.7f); p.subLevel=0.4f; p.fxDrive=0.45f; p.drift=0.15f;
    filt(p,FilterMode::LadderLP,0.32f,0.3f,0.4f,0.5f,0.3f); p.env[0]=EnvParams{0.005f,0.4f,0.8f,0.2f,0};
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.35f); mod(p,1,ModSource::ModWheel,ModDest::FilterDrive,0.4f);
    mod(p,2,ModSource::Velocity,ModDest::Cutoff,0.3f); p.fxReverb=0.08f; }
void bass_wobble(SynthPatch& p){ setName(p,"WOB BASS"); p.category=2; p.width=0.4f; p.ampGain=0.62f; p.voiceMode=(int)VoiceMode::Mono;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.15f,0.9f); p.subLevel=0.45f; p.fxDrive=0.3f; p.drift=0.15f;
    filt(p,FilterMode::LadderLP,0.32f,0.4f,0.3f,0.4f,0.2f); p.env[0]=EnvParams{0.01f,0.5f,0.9f,0.2f,0};
    lfoSync(p,0,LfoWave::Sine,7,1.0f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.6f);
    mod(p,1,ModSource::ModWheel,ModDest::Lfo1Rate,0.6f); p.fxReverb=0.1f; }
void bass_deep(SynthPatch& p){ setName(p,"DEEP BASS"); p.category=2; p.width=0.2f; p.ampGain=0.9f; p.voiceMode=(int)VoiceMode::Mono;
    osc(p,0,SINE,0,1.0f); p.subLevel=0.6f; p.subOctave=2; p.drift=0.04f; filt(p,FilterMode::LadderLP,0.35f,0.05f,0.2f,0.15f,0.1f);
    p.env[0]=EnvParams{0.004f,0.3f,0.92f,0.18f,0}; mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.25f);
    mod(p,1,ModSource::Velocity,ModDest::Amp,0.3f); p.fxReverb=0.04f; }
void bass_house(SynthPatch& p){ setName(p,"HOUSE BASS"); p.category=2; p.width=0.4f; p.ampGain=0.8f; p.voiceMode=(int)VoiceMode::Mono;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0,0.6f,3); osc(p,2,SINE,0,0.4f,4); p.drift=0.06f; // organ bass
    filt(p,FilterMode::SvfLP,0.6f,0.08f,0.15f); p.env[0]=EnvParams{0.004f,0.2f,0.7f,0.12f,0.1f};
    mod(p,0,ModSource::Velocity,ModDest::Cutoff,0.3f); p.fxReverb=0.1f; }
void bass_retro(SynthPatch& p){ setName(p,"SYNTHWAVE"); p.category=2; p.width=0.45f; p.ampGain=0.72f; p.chorusMode=1;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.1f,0.8f); osc(p,3,SAW,0,0.5f,1); p.subLevel=0.25f; p.drift=0.2f;
    filt(p,FilterMode::LadderLP,0.42f,0.18f,0.4f,0.2f,0.3f); p.env[0]=EnvParams{0.006f,0.4f,0.8f,0.25f,0.1f};
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.35f); mod(p,1,ModSource::ModWheel,ModDest::Cutoff,0.35f);
    p.fxChorus=0.25f; p.fxReverb=0.15f; }
// ---- ARP ----
void arp_house(SynthPatch& p){ setName(p,"ARP HOUSE"); p.category=3; p.width=0.6f; p.ampGain=0.78f;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0.1f,0.7f); p.drift=0.15f; filt(p,FilterMode::LadderLP,0.5f,0.2f,0.5f,0.2f,0.4f);
    p.env[0]=EnvParams{0.003f,0.2f,0.0f,0.14f,0.15f}; p.env[1]=EnvParams{0.003f,0.16f,0.0f,0.12f,0};
    arp_setup(p,4,ArpMode::Up,1); mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.5f);
    mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.35f); p.fxDelay=0.28f; p.fxReverb=0.3f; }
void arp_dark(SynthPatch& p){ setName(p,"ARP DARK"); p.category=3; p.width=0.65f; p.ampGain=0.76f;
    osc(p,0,TRI,0,1.0f); osc(p,1,SAW,0.06f,0.6f,1); p.subLevel=0.2f; p.drift=0.2f;
    filt(p,FilterMode::LadderLP,0.35f,0.3f,0.5f,0.25f,0.3f); p.env[0]=EnvParams{0.003f,0.3f,0.0f,0.2f,0.15f};
    p.env[1]=EnvParams{0.003f,0.25f,0.0f,0.18f,0}; arp_setup(p,7,ArpMode::DownUp,2);
    lfo(p,0,LfoWave::Sine,0.2f); mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.5f);
    mod(p,1,ModSource::Lfo1,ModDest::Cutoff,0.15f); p.fxDelay=0.3f; p.fxReverb=0.35f; }
void arp_trance(SynthPatch& p){ setName(p,"ARP TRANCE"); p.category=3; p.width=0.75f; p.ampGain=0.66f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.12f,1.0f); osc(p,3,SAW,-0.1f,0.6f); p.drift=0.25f;
    filt(p,FilterMode::LadderLP,0.5f,0.2f,0.45f,0.15f,0.4f); p.env[0]=EnvParams{0.004f,0.25f,0.2f,0.18f,0.1f};
    p.env[1]=EnvParams{0.004f,0.2f,0.1f,0.16f,0}; arp_setup(p,4,ArpMode::UpDown,2);
    lfoSync(p,0,LfoWave::Triangle,10); mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.4f);
    mod(p,1,ModSource::Lfo1,ModDest::Cutoff,0.2f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.3f);
    p.fxChorus=0.3f; p.fxDelay=0.32f; p.fxReverb=0.35f; }
// ---- STAB ----
void stab_pizz(SynthPatch& p){ setName(p,"PIZZ STAB"); p.category=4; p.width=0.5f; p.ampGain=0.8f;
    osc(p,0,SAW,0,1.0f); osc(p,1,TRI,0.04f,0.5f); p.drift=0.1f; filt(p,FilterMode::SvfLP,0.55f,0.18f,0.5f,0,0.5f);
    p.env[0]=EnvParams{0.002f,0.18f,0.0f,0.14f,0.2f}; p.env[1]=EnvParams{0.002f,0.14f,0.0f,0.1f,0};
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.45f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.4f);
    p.fxReverb=0.3f; p.fxDelay=0.12f; }
void stab_syn(SynthPatch& p){ setName(p,"SYNTH STAB"); p.category=4; p.width=0.7f; p.ampGain=0.66f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0.1f,0.8f); osc(p,3,SAW,-0.12f,0.6f); p.subLevel=0.1f; p.drift=0.25f;
    filt(p,FilterMode::LadderLP,0.5f,0.22f,0.5f,0.2f,0.4f); p.env[0]=EnvParams{0.004f,0.3f,0.0f,0.24f,0.15f};
    p.env[1]=EnvParams{0.004f,0.25f,0.0f,0.2f,0}; mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.5f);
    mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.4f); p.fxChorus=0.3f; p.fxDelay=0.2f; p.fxReverb=0.35f; }
void stab_det(SynthPatch& p){ setName(p,"DETUNE STAB"); p.category=4; p.width=0.8f; p.ampGain=0.6f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.2f,1.0f); osc(p,2,SAW,-0.18f,0.9f); osc(p,3,SAW,0.35f,0.6f); p.drift=0.35f;
    filt(p,FilterMode::LadderLP,0.5f,0.18f,0.45f,0.2f,0.4f); p.env[0]=EnvParams{0.004f,0.35f,0.0f,0.28f,0.15f};
    p.env[1]=EnvParams{0.004f,0.28f,0.0f,0.2f,0}; mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.45f);
    mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.35f); p.fxChorus=0.4f; p.fxDelay=0.24f; p.fxReverb=0.4f; }
// ---- KEYS ----
void keys_grand(SynthPatch& p){ setName(p,"SYNTH GRAND"); p.category=5; p.width=0.5f; p.ampGain=0.8f;
    osc(p,0,TRI,0,1.0f); osc(p,1,SINE,0.02f,0.7f,3); p.drift=0.05f; filt(p,FilterMode::SvfLP,0.7f,0.05f,0.3f);
    p.env[0]=EnvParams{0.002f,0.9f,0.2f,0.4f,0.3f}; mod(p,0,ModSource::Velocity,ModDest::Cutoff,0.5f);
    mod(p,1,ModSource::EnvFilter,ModDest::Cutoff,0.3f); p.fxReverb=0.3f; }
void keys_bellkey(SynthPatch& p){ setName(p,"BELL KEYS"); p.category=5; p.width=0.6f; p.ampGain=0.74f; p.chorusMode=1;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0,0.8f,4); p.fm2to1=0.35f; p.ringMod=0.15f; filt(p,FilterMode::SvfLP,0.75f,0.04f,0.15f);
    p.env[0]=EnvParams{0.002f,1.0f,0.25f,0.5f,0.3f}; p.env[2]=EnvParams{0.001f,0.12f,0,0.1f,0};
    mod(p,0,ModSource::EnvAux,ModDest::FmAmount,0.5f); mod(p,1,ModSource::Velocity,ModDest::FmAmount,0.5f);
    p.fxChorus=0.25f; p.fxReverb=0.4f; }
void keys_padkey(SynthPatch& p){ setName(p,"PAD KEYS"); p.category=5; p.width=0.7f; p.ampGain=0.72f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0.08f,0.7f); p.subLevel=0.1f; p.drift=0.18f;
    filt(p,FilterMode::LadderLP,0.5f,0.12f,0.3f,0.15f,0.4f); p.env[0]=EnvParams{0.05f,0.8f,0.7f,0.6f,0.25f};
    p.env[1]=EnvParams{0.1f,0.6f,0.4f,0.4f,0}; mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.3f);
    mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.3f); p.fxChorus=0.3f; p.fxReverb=0.35f; }
void keys_organ(SynthPatch& p){ setName(p,"ORGAN"); p.category=5; p.width=0.6f; p.ampGain=0.7f;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0,0.8f,3); osc(p,2,SINE,0,0.6f,4); osc(p,3,SINE,0,0.4f,1); p.drift=0.04f;
    filt(p,FilterMode::SvfLP,0.85f,0.04f,0.05f); p.env[0]=EnvParams{0.004f,0.1f,0.95f,0.1f,0.05f};
    lfo(p,0,LfoWave::Sine,6.5f); mod(p,0,ModSource::Lfo1,ModDest::Pitch,0.03f);
    mod(p,1,ModSource::ModWheel,ModDest::Lfo1Rate,0.6f); p.fxChorus=0.35f; p.fxReverb=0.25f; }
void keys_toy(SynthPatch& p){ setName(p,"TOY PIANO"); p.category=5; p.width=0.5f; p.ampGain=0.78f;
    osc(p,0,PULSE,0,1.0f); p.osc[0].pw=0.3f; p.osc[0].crush=0.28f; osc(p,1,SINE,0,0.5f,4); p.ringMod=0.2f; p.drift=0.08f;
    filt(p,FilterMode::SvfLP,0.7f,0.08f,0.3f); p.env[0]=EnvParams{0.001f,0.5f,0.0f,0.4f,0.2f};
    mod(p,0,ModSource::Velocity,ModDest::Cutoff,0.4f); p.fxReverb=0.3f; p.fxDelay=0.1f; }
void keys_fmclassic(SynthPatch& p){ setName(p,"FM CLASSIC"); p.category=5; p.width=0.6f; p.ampGain=0.78f; p.chorusMode=1;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0,1.0f,3); p.fm2to1=0.5f; filt(p,FilterMode::SvfLP,0.75f,0.04f,0.1f);
    p.env[0]=EnvParams{0.002f,1.4f,0.3f,0.5f,0.3f}; p.env[2]=EnvParams{0.001f,0.5f,0.1f,0.3f,0};
    mod(p,0,ModSource::EnvAux,ModDest::FmAmount,0.5f); mod(p,1,ModSource::Velocity,ModDest::FmAmount,0.6f);
    p.fxChorus=0.3f; p.fxReverb=0.35f; }
void keys_softkey(SynthPatch& p){ setName(p,"SOFT KEYS"); p.category=5; p.width=0.55f; p.ampGain=0.82f; p.chorusMode=1;
    osc(p,0,TRI,0,1.0f); osc(p,1,SINE,0.04f,0.7f); p.drift=0.08f; filt(p,FilterMode::SvfLP,0.6f,0.06f,0.25f);
    p.env[0]=EnvParams{0.004f,0.7f,0.4f,0.4f,0.25f}; mod(p,0,ModSource::Velocity,ModDest::Cutoff,0.4f);
    mod(p,1,ModSource::EnvFilter,ModDest::Cutoff,0.2f); p.fxChorus=0.25f; p.fxReverb=0.35f; }
// ---- PLUCK ----
void pluck_marimba(SynthPatch& p){ setName(p,"MARIMBA"); p.category=6; p.width=0.5f; p.ampGain=0.82f;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0,0.4f,4); p.drift=0.04f; filt(p,FilterMode::SvfLP,0.65f,0.05f,0.3f);
    p.env[0]=EnvParams{0.001f,0.35f,0.0f,0.3f,0.2f}; p.env[2]=EnvParams{0.001f,0.04f,0,0.04f,0};
    mod(p,0,ModSource::EnvAux,ModDest::Pitch,0.2f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.35f); p.fxReverb=0.35f; }
void pluck_guitar(SynthPatch& p){ setName(p,"SYNTH GTR"); p.category=6; p.width=0.5f; p.ampGain=0.8f;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0.03f,0.5f); p.drift=0.1f; filt(p,FilterMode::LadderLP,0.5f,0.2f,0.5f,0.2f,0.5f);
    p.env[0]=EnvParams{0.002f,0.4f,0.0f,0.3f,0.2f}; p.env[1]=EnvParams{0.002f,0.3f,0.0f,0.2f,0};
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.5f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.4f);
    p.fxReverb=0.3f; p.fxDelay=0.15f; }
void pluck_glass(SynthPatch& p){ setName(p,"GLASS PLUCK"); p.category=6; p.width=0.6f; p.ampGain=0.74f;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0.01f,0.8f,3); p.fm2to1=0.3f; p.ringMod=0.2f; filt(p,FilterMode::SvfLP,0.78f,0.04f,0.2f);
    p.env[0]=EnvParams{0.001f,0.4f,0.0f,0.3f,0.15f}; p.env[2]=EnvParams{0.001f,0.1f,0,0.08f,0};
    mod(p,0,ModSource::EnvAux,ModDest::FmAmount,0.45f); mod(p,1,ModSource::Velocity,ModDest::RingMod,0.3f);
    p.fxDelay=0.26f; p.fxReverb=0.42f; }
void pluck_pizz(SynthPatch& p){ setName(p,"PIZZICATO"); p.category=6; p.width=0.45f; p.ampGain=0.82f;
    osc(p,0,SAW,0,1.0f); osc(p,1,TRI,0.05f,0.4f); p.drift=0.1f; filt(p,FilterMode::SvfLP,0.5f,0.2f,0.55f,0,0.5f);
    p.env[0]=EnvParams{0.001f,0.16f,0.0f,0.12f,0.2f}; p.env[1]=EnvParams{0.001f,0.12f,0.0f,0.1f,0};
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.5f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.4f); p.fxReverb=0.35f; }
void pluck_musicbox(SynthPatch& p){ setName(p,"MUSIC BOX"); p.category=6; p.width=0.6f; p.ampGain=0.74f; p.chorusMode=1;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0,0.6f,4); p.ringMod=0.25f; filt(p,FilterMode::SvfLP,0.8f,0.04f,0.2f);
    p.env[0]=EnvParams{0.001f,0.6f,0.0f,0.5f,0.2f}; p.env[2]=EnvParams{0.001f,0.06f,0,0.06f,0};
    mod(p,0,ModSource::EnvAux,ModDest::Pitch,0.15f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.3f);
    p.fxDelay=0.3f; p.fxReverb=0.5f; }
// ---- FX ----
void fx_impact(SynthPatch& p){ setName(p,"IMPACT"); p.category=7; p.width=0.7f; p.ampGain=0.8f; p.fxDrive=0.3f;
    osc(p,0,SINE,0,1.0f); p.noiseLevel=0.4f; p.drift=0; filt(p,FilterMode::LadderLP,0.5f,0.1f,0,0.2f,0);
    p.env[0]=EnvParams{0.001f,0.6f,0.0f,0.4f,0}; p.env[2]=EnvParams{0.001f,0.1f,0,0.08f,0};
    mod(p,0,ModSource::EnvAux,ModDest::Pitch,2.0f); mod(p,1,ModSource::EnvAmp,ModDest::NoiseLevel,-0.3f); p.fxReverb=0.6f; }
void fx_wind(SynthPatch& p){ setName(p,"WIND"); p.category=7; p.width=0.95f; p.ampGain=0.5f; p.osc[0].on=false; p.noiseLevel=0.9f;
    filt(p,FilterMode::SvfBP,0.4f,0.3f,0); p.env[0]=EnvParams{1.0f,1.0f,0.9f,1.5f,0};
    lfo(p,0,LfoWave::Sine,0.1f); lfo(p,1,LfoWave::Triangle,0.07f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.5f);
    mod(p,1,ModSource::Lfo2,ModDest::Cutoff,0.3f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.5f); p.fxReverb=0.7f; }
void fx_alien(SynthPatch& p){ setName(p,"ALIEN"); p.category=7; p.width=0.8f; p.ampGain=0.6f;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0,1.0f); p.ringMod=0.5f; p.fm2to1=0.3f; p.drift=0.3f;
    filt(p,FilterMode::SvfBP,0.5f,0.4f,0.1f); p.env[0]=EnvParams{0.1f,0.5f,0.8f,0.5f,0};
    lfo(p,0,LfoWave::SampleHold,4.0f); lfo(p,1,LfoWave::Sine,0.3f); mod(p,0,ModSource::Lfo1,ModDest::Pitch,0.2f);
    mod(p,1,ModSource::Lfo1,ModDest::RingMod,0.3f); mod(p,2,ModSource::Lfo2,ModDest::Cutoff,0.3f); p.fxDelay=0.4f; p.fxReverb=0.5f; }
// ---- DRONE ----
void drone_choir(SynthPatch& p){ setName(p,"DRONE CHOIR"); p.category=8; p.width=0.9f; p.ampGain=0.5f; p.chorusMode=2;
    p.voiceMode=(int)VoiceMode::Unison; p.unisonCount=6; p.unisonDetune=0.16f; p.unisonSpread=0.85f;
    osc(p,0,PULSE,0,1.0f); p.osc[0].pw=0.5f; osc(p,1,PULSE,0.05f,1.0f); p.osc[1].pw=0.46f; p.subLevel=0.1f; p.drift=0.35f;
    filt(p,FilterMode::SvfBP,0.5f,0.3f,0.1f); p.env[0]=EnvParams{1.8f,2.0f,0.92f,3.4f,0.15f};
    lfo(p,0,LfoWave::Sine,0.06f); lfo(p,1,LfoWave::Triangle,0.1f); mod(p,0,ModSource::Lfo1,ModDest::Detune,0.3f);
    mod(p,1,ModSource::Lfo2,ModDest::Cutoff,0.18f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.78f; }
void drone_warm(SynthPatch& p){ setName(p,"WARM DRONE"); p.category=8; p.width=0.85f; p.ampGain=0.72f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.06f,0.9f); osc(p,3,TRI,-0.05f,0.5f,1); p.subLevel=0.3f; p.drift=0.35f;
    filt(p,FilterMode::LadderLP,0.32f,0.15f,0.2f,0.3f,0.2f); p.env[0]=EnvParams{2.0f,2.0f,1.0f,3.0f,0};
    lfo(p,0,LfoWave::Sine,0.05f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.15f);
    mod(p,1,ModSource::ModWheel,ModDest::Cutoff,0.4f); p.fxReverb=0.6f; }
void drone_noise(SynthPatch& p){ setName(p,"NOISE DRONE"); p.category=8; p.width=0.9f; p.ampGain=0.6f;
    osc(p,0,SAW,0,0.6f); p.noiseLevel=0.5f; p.drift=0.2f; filt(p,FilterMode::SvfBP,0.4f,0.3f,0.1f);
    p.env[0]=EnvParams{1.5f,2.0f,1.0f,2.5f,0}; lfo(p,0,LfoWave::Triangle,0.08f); lfo(p,1,LfoWave::SampleHold,0.15f);
    mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.4f); mod(p,1,ModSource::Lfo2,ModDest::Cutoff,0.2f);
    mod(p,2,ModSource::ModWheel,ModDest::Resonance,0.3f); p.fxReverb=0.7f; }
// ---- PERC ----
void perc_808(SynthPatch& p){ setName(p,"808 KICK"); p.category=9; p.width=0.2f; p.ampGain=0.9f; p.fxDrive=0.15f;
    osc(p,0,SINE,0,1.0f); p.drift=0; filt(p,FilterMode::LadderLP,0.5f,0,0,0.1f,0);
    p.env[0]=EnvParams{0.001f,0.7f,0.0f,0.2f,0}; p.env[2]=EnvParams{0.001f,0.08f,0,0.06f,0};
    mod(p,0,ModSource::EnvAux,ModDest::Pitch,1.6f); p.fxReverb=0; }
void perc_hat(SynthPatch& p){ setName(p,"HIHAT"); p.category=9; p.width=0.3f; p.ampGain=0.6f; p.osc[0].on=false; p.noiseLevel=0.9f;
    filt(p,FilterMode::SvfHP,0.8f,0.2f,0); p.env[0]=EnvParams{0.001f,0.05f,0.0f,0.04f,0}; mod(p,0,ModSource::Velocity,ModDest::Amp,0.4f); p.fxReverb=0.08f; }
void perc_openhat(SynthPatch& p){ setName(p,"OPEN HAT"); p.category=9; p.width=0.35f; p.ampGain=0.58f; p.osc[0].on=false; p.noiseLevel=0.9f;
    filt(p,FilterMode::SvfHP,0.82f,0.18f,0); p.env[0]=EnvParams{0.001f,0.3f,0.0f,0.2f,0}; mod(p,0,ModSource::Velocity,ModDest::Amp,0.4f); p.fxReverb=0.12f; }
void perc_rim(SynthPatch& p){ setName(p,"RIMSHOT"); p.category=9; p.width=0.3f; p.ampGain=0.7f;
    osc(p,0,PULSE,0,1.0f); p.osc[0].pw=0.4f; p.noiseLevel=0.3f; p.ringMod=0.3f; filt(p,FilterMode::SvfBP,0.7f,0.3f,0);
    p.env[0]=EnvParams{0.001f,0.06f,0.0f,0.05f,0}; p.env[2]=EnvParams{0.001f,0.02f,0,0.02f,0};
    mod(p,0,ModSource::EnvAux,ModDest::Pitch,0.8f); p.fxReverb=0.1f; }
void perc_clave(SynthPatch& p){ setName(p,"CLAVE"); p.category=9; p.width=0.3f; p.ampGain=0.72f;
    osc(p,0,SINE,0,1.0f); p.drift=0; filt(p,FilterMode::SvfBP,0.75f,0.15f,0); p.env[0]=EnvParams{0.001f,0.07f,0,0.05f,0};
    p.env[2]=EnvParams{0.001f,0.02f,0,0.02f,0}; mod(p,0,ModSource::EnvAux,ModDest::Pitch,0.6f); p.fxReverb=0.12f; }
void perc_conga(SynthPatch& p){ setName(p,"CONGA"); p.category=9; p.width=0.4f; p.ampGain=0.82f;
    osc(p,0,SINE,0,1.0f); osc(p,1,TRI,0,0.3f); p.noiseLevel=0.03f; filt(p,FilterMode::LadderLP,0.55f,0.1f,0,0.1f,0);
    p.env[0]=EnvParams{0.001f,0.3f,0.0f,0.12f,0}; p.env[2]=EnvParams{0.001f,0.08f,0,0.06f,0};
    mod(p,0,ModSource::EnvAux,ModDest::Pitch,1.0f); p.fxReverb=0.12f; }
void perc_zap(SynthPatch& p){ setName(p,"PERC ZAP"); p.category=9; p.width=0.4f; p.ampGain=0.76f;
    osc(p,0,SAW,0,1.0f); filt(p,FilterMode::SvfBP,0.6f,0.3f,0); p.env[0]=EnvParams{0.001f,0.1f,0,0.06f,0};
    p.env[2]=EnvParams{0.001f,0.05f,0,0.04f,0}; mod(p,0,ModSource::EnvAux,ModDest::Pitch,1.8f);
    mod(p,1,ModSource::EnvAux,ModDest::Cutoff,0.5f); p.fxDelay=0.2f; p.fxReverb=0.2f; }
// ---- AMB ----
void amb_drift(SynthPatch& p){ setName(p,"DRIFT"); p.category=10; p.width=0.92f; p.ampGain=0.58f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,TRI,0.1f,0.8f); osc(p,3,SINE,-0.08f,0.4f,3); p.drift=0.5f;
    filt(p,FilterMode::SvfLP,0.45f,0.15f,0.15f); p.env[0]=EnvParams{2.4f,2.0f,0.9f,3.6f,0.1f};
    lfo(p,0,LfoWave::Sine,0.04f); lfo(p,1,LfoWave::Triangle,0.06f); mod(p,0,ModSource::Lfo1,ModDest::Detune,0.5f);
    mod(p,1,ModSource::Lfo2,ModDest::Cutoff,0.2f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.82f; p.fxDelay=0.35f; }
void amb_underwater(SynthPatch& p){ setName(p,"UNDERWATER"); p.category=10; p.width=0.9f; p.ampGain=0.6f; p.chorusMode=2;
    osc(p,0,SINE,0,1.0f); osc(p,1,TRI,0.06f,0.7f); p.drift=0.4f; filt(p,FilterMode::SvfLP,0.35f,0.25f,0.12f);
    p.env[0]=EnvParams{1.6f,2.0f,0.9f,3.0f,0.1f}; lfo(p,0,LfoWave::Sine,0.12f); lfo(p,1,LfoWave::Sine,0.18f);
    mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.3f); mod(p,1,ModSource::Lfo2,ModDest::Pitch,0.02f);
    mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.8f; p.fxDelay=0.4f; }
void amb_meadow(SynthPatch& p){ setName(p,"MEADOW"); p.category=10; p.width=0.92f; p.ampGain=0.6f; p.chorusMode=2;
    osc(p,0,TRI,0,1.0f); osc(p,1,SAW,0.1f,0.7f); osc(p,3,SINE,0.05f,0.4f,3); p.noiseLevel=0.05f; p.drift=0.4f;
    filt(p,FilterMode::SvfLP,0.55f,0.12f,0.18f); p.env[0]=EnvParams{1.8f,2.0f,0.9f,3.2f,0.15f};
    lfo(p,0,LfoWave::Sine,0.07f,1,0.1f); lfo(p,1,LfoWave::Triangle,0.11f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.2f);
    mod(p,1,ModSource::Lfo2,ModDest::Detune,0.4f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.76f; p.fxDelay=0.3f; }
void amb_dust(SynthPatch& p){ setName(p,"STARDUST"); p.category=10; p.width=0.95f; p.ampGain=0.56f; p.chorusMode=2;
    osc(p,0,SINE,0,1.0f,3); osc(p,1,SINE,0.03f,0.6f,4); p.ringMod=0.15f; p.noiseLevel=0.04f; p.drift=0.35f;
    filt(p,FilterMode::SvfHP,0.4f,0.1f,0.1f); p.env[0]=EnvParams{1.4f,2.0f,0.85f,3.4f,0.1f};
    lfo(p,0,LfoWave::SampleHold,0.3f); lfo(p,1,LfoWave::Sine,0.05f); mod(p,0,ModSource::Lfo1,ModDest::RingMod,0.2f);
    mod(p,1,ModSource::Lfo2,ModDest::Cutoff,0.2f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.85f; p.fxDelay=0.45f; }
void amb_breath(SynthPatch& p){ setName(p,"BREATH"); p.category=10; p.width=0.9f; p.ampGain=0.56f; p.chorusMode=2;
    osc(p,0,PULSE,0,1.0f); p.osc[0].pw=0.5f; osc(p,1,PULSE,0.07f,0.8f); p.noiseLevel=0.1f; p.drift=0.3f;
    filt(p,FilterMode::SvfBP,0.5f,0.35f,0.12f); p.env[0]=EnvParams{1.6f,1.8f,0.9f,3.0f,0.15f};
    lfo(p,0,LfoWave::Sine,0.1f); lfo(p,1,LfoWave::Triangle,0.15f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.25f);
    mod(p,1,ModSource::Lfo2,ModDest::Pw1,0.3f); mod(p,2,ModSource::ModWheel,ModDest::Resonance,0.2f); p.fxReverb=0.78f; }
void amb_crystal(SynthPatch& p){ setName(p,"CRYSTAL"); p.category=10; p.width=0.9f; p.ampGain=0.6f; p.chorusMode=2;
    osc(p,0,SINE,0,1.0f); osc(p,1,SINE,0.01f,0.8f,4); p.fm2to1=0.2f; p.ringMod=0.2f; p.drift=0.2f;
    filt(p,FilterMode::SvfLP,0.7f,0.06f,0.15f); p.env[0]=EnvParams{1.2f,2.0f,0.85f,3.0f,0.1f};
    p.env[2]=EnvParams{0.5f,1.0f,0.3f,0.8f,0}; lfo(p,0,LfoWave::Sine,0.08f); mod(p,0,ModSource::Lfo1,ModDest::FmAmount,0.2f);
    mod(p,1,ModSource::EnvAux,ModDest::FmAmount,0.3f); mod(p,2,ModSource::ModWheel,ModDest::RingMod,0.3f); p.fxReverb=0.84f; p.fxDelay=0.4f; }
void amb_nebula(SynthPatch& p){ setName(p,"NEBULA"); p.category=10; p.width=0.95f; p.ampGain=0.56f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.16f,0.8f); osc(p,2,PULSE,-0.14f,0.6f); osc(p,3,SINE,0.08f,0.4f,1); p.drift=0.5f;
    filt(p,FilterMode::SvfLP,0.42f,0.2f,0.15f); p.env[0]=EnvParams{2.6f,2.2f,0.88f,4.0f,0.1f};
    lfo(p,0,LfoWave::Sine,0.03f); lfo(p,1,LfoWave::Triangle,0.05f); lfo(p,2,LfoWave::SampleHold,0.08f);
    mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.2f); mod(p,1,ModSource::Lfo2,ModDest::Detune,0.5f);
    mod(p,2,ModSource::Lfo3,ModDest::Pw2,0.3f); mod(p,3,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.86f; p.fxDelay=0.4f; }
void amb_tape(SynthPatch& p){ setName(p,"TAPE PAD"); p.category=10; p.width=0.85f; p.ampGain=0.62f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,TRI,0.08f,0.8f); p.subLevel=0.12f; p.drift=0.45f; // heavy drift = tape wow
    filt(p,FilterMode::SvfLP,0.5f,0.12f,0.18f); p.env[0]=EnvParams{1.4f,1.8f,0.88f,3.0f,0.15f};
    lfo(p,0,LfoWave::Sine,0.6f,0.4f); lfo(p,1,LfoWave::Triangle,0.07f); mod(p,0,ModSource::Lfo1,ModDest::Pitch,0.02f);
    mod(p,1,ModSource::Lfo2,ModDest::Cutoff,0.2f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.74f; p.fxDelay=0.3f; }
void amb_warmth(SynthPatch& p){ setName(p,"WARMTH"); p.category=10; p.width=0.85f; p.ampGain=0.66f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.07f,0.9f); osc(p,3,TRI,-0.05f,0.5f,1); p.subLevel=0.2f; p.drift=0.35f;
    filt(p,FilterMode::LadderLP,0.4f,0.12f,0.2f,0.25f,0.3f); p.env[0]=EnvParams{1.6f,1.8f,0.9f,3.0f,0.15f};
    lfo(p,0,LfoWave::Sine,0.06f,1,0.1f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.15f);
    mod(p,1,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxReverb=0.72f; p.fxDelay=0.25f; }
// ---- SEQ ----
void seq_pulse(SynthPatch& p){ setName(p,"PULSE SEQ"); p.category=11; p.width=0.6f; p.ampGain=0.72f;
    osc(p,0,PULSE,0,1.0f); p.osc[0].pw=0.5f; osc(p,1,PULSE,0.06f,0.7f); p.subLevel=0.12f; p.drift=0.15f;
    filt(p,FilterMode::LadderLP,0.5f,0.2f,0.4f,0.15f,0.4f); p.env[0]=EnvParams{0.004f,0.2f,0.3f,0.16f,0.1f};
    p.env[1]=EnvParams{0.004f,0.18f,0.1f,0.14f,0}; lfoSync(p,0,LfoWave::Square,4); lfo(p,1,LfoWave::Sine,0.5f);
    mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.4f); mod(p,1,ModSource::Lfo2,ModDest::Pw1,0.4f);
    mod(p,2,ModSource::EnvFilter,ModDest::Cutoff,0.3f); p.fxDelay=0.3f; p.fxReverb=0.3f; }
void seq_pluckseq(SynthPatch& p){ setName(p,"PLUCK SEQ"); p.category=11; p.width=0.6f; p.ampGain=0.8f;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0.08f,0.5f); p.drift=0.15f; filt(p,FilterMode::LadderLP,0.45f,0.25f,0.6f,0.2f,0.5f);
    p.env[0]=EnvParams{0.002f,0.16f,0.0f,0.12f,0.15f}; p.env[1]=EnvParams{0.002f,0.14f,0.0f,0.1f,0};
    arp_setup(p,4,ArpMode::Up,2); mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.55f);
    mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.4f); p.fxDelay=0.32f; p.fxReverb=0.3f; }
void seq_chordseq(SynthPatch& p){ setName(p,"CHORD SEQ"); p.category=11; p.width=0.75f; p.ampGain=0.6f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.12f,0.9f); osc(p,3,SAW,-0.1f,0.6f); p.drift=0.25f;
    filt(p,FilterMode::LadderLP,0.5f,0.18f,0.4f,0.15f,0.4f); p.env[0]=EnvParams{0.004f,0.4f,0.0f,0.3f,0.15f};
    p.env[1]=EnvParams{0.004f,0.3f,0.0f,0.22f,0}; lfoSync(p,0,LfoWave::Square,4);
    mod(p,0,ModSource::Lfo1,ModDest::Amp,0.6f); mod(p,1,ModSource::EnvFilter,ModDest::Cutoff,0.4f);
    p.fxChorus=0.3f; p.fxDelay=0.3f; p.fxReverb=0.4f; }
void seq_sh(SynthPatch& p){ setName(p,"S&H SEQ"); p.category=11; p.width=0.6f; p.ampGain=0.74f;
    osc(p,0,SAW,0,1.0f); osc(p,1,PULSE,0.08f,0.7f); p.subLevel=0.12f; p.drift=0.2f;
    filt(p,FilterMode::LadderLP,0.45f,0.28f,0.4f,0.2f,0.4f); p.env[0]=EnvParams{0.004f,0.3f,0.4f,0.2f,0.1f};
    lfoSync(p,0,LfoWave::SampleHold,4); lfo(p,1,LfoWave::Sine,0.3f); mod(p,0,ModSource::Lfo1,ModDest::Cutoff,0.5f);
    mod(p,1,ModSource::Lfo1,ModDest::Pitch,0.0f); mod(p,2,ModSource::Lfo2,ModDest::Pw2,0.2f); p.fxDelay=0.3f; p.fxReverb=0.3f; }
void seq_gate(SynthPatch& p){ setName(p,"GATE SEQ"); p.category=11; p.width=0.8f; p.ampGain=0.62f; p.chorusMode=2;
    osc(p,0,SAW,0,1.0f); osc(p,1,SAW,0.12f,1.0f); osc(p,3,SAW,-0.1f,0.6f); p.drift=0.25f;
    filt(p,FilterMode::LadderLP,0.55f,0.15f,0.2f,0.15f,0.4f); p.env[0]=EnvParams{0.3f,1.0f,0.9f,0.5f,0};
    lfoSync(p,0,LfoWave::Square,7); lfo(p,1,LfoWave::Sine,0.2f); mod(p,0,ModSource::Lfo1,ModDest::Amp,0.9f);
    mod(p,1,ModSource::Lfo2,ModDest::Cutoff,0.2f); mod(p,2,ModSource::ModWheel,ModDest::Cutoff,0.3f); p.fxChorus=0.3f; p.fxDelay=0.3f; p.fxReverb=0.4f; }
void seq_arpbass(SynthPatch& p){ setName(p,"ARP BASS"); p.category=11; p.width=0.4f; p.ampGain=0.78f;
    osc(p,0,SAW,0,1.0f); osc(p,1,SINE,0,0.4f,1); p.subLevel=0.25f; p.drift=0.1f;
    filt(p,FilterMode::LadderLP,0.4f,0.28f,0.5f,0.25f,0.3f); p.env[0]=EnvParams{0.003f,0.18f,0.2f,0.14f,0.15f};
    p.env[1]=EnvParams{0.003f,0.16f,0.05f,0.12f,0}; arp_setup(p,4,ArpMode::Up,1); p.arp.gate=0.5f;
    mod(p,0,ModSource::EnvFilter,ModDest::Cutoff,0.5f); mod(p,1,ModSource::Velocity,ModDest::Cutoff,0.3f); p.fxDelay=0.26f; p.fxReverb=0.18f; }
void seq_8bit(SynthPatch& p){ setName(p,"8-BIT"); p.category=11; p.width=0.5f; p.ampGain=0.72f;
    osc(p,0,PULSE,0,1.0f); p.osc[0].pw=0.5f; p.osc[0].crush=0.55f; p.drift=0; filt(p,FilterMode::SvfLP,0.85f,0.05f,0.05f);
    p.env[0]=EnvParams{0.001f,0.1f,0.4f,0.08f,0}; arp_setup(p,4,ArpMode::UpDown,2); p.arp.gate=0.5f;
    lfo(p,0,LfoWave::Square,8.0f); mod(p,0,ModSource::Lfo1,ModDest::Pw1,0.3f); mod(p,1,ModSource::Lfo1,ModDest::Pitch,0.02f);
    p.fxDelay=0.28f; p.fxReverb=0.15f; }

using Builder = void (*)(SynthPatch&);
const Builder kBuilders[kNumFactoryPresets] = {
    // PAD (12)
    pad_warm, pad_glass, pad_strings, pad_voices, pad_dark, pad_sweep, pad_fm, pad_air, pad_jp, pad_oct, pad_wide, pad_mellow,
    // LEAD (12)
    lead_super, lead_sync, lead_soft, lead_fm, lead_pwm, lead_saw, lead_square, lead_pluck, lead_fifth, lead_dist, lead_whistle, lead_uni,
    // BASS (12)
    bass_reese, bass_sub, bass_acid, bass_fm, bass_growl, bass_pluck, bass_square, bass_dirty, bass_wobble, bass_deep, bass_house, bass_retro,
    // ARP (8)
    arp_pluck, arp_bell, arp_saw, arp_pwm, arp_oct, arp_house, arp_dark, arp_trance,
    // STAB (8)
    stab_house, stab_rave, stab_brass, stab_organ, stab_chord, stab_pizz, stab_syn, stab_det,
    // KEYS (12)
    keys_rhodes, keys_wurli, keys_dx, keys_clav, keys_poly, keys_grand, keys_bellkey, keys_padkey, keys_organ, keys_toy, keys_fmclassic, keys_softkey,
    // PLUCK (10)
    pluck_synth, pluck_koto, pluck_bell, pluck_mallet, pluck_harp, pluck_marimba, pluck_guitar, pluck_glass, pluck_pizz, pluck_musicbox,
    // FX (8)
    fx_riser, fx_noise, fx_siren, fx_zap, fx_drop, fx_impact, fx_wind, fx_alien,
    // DRONE (8)
    drone_dark, drone_sub, drone_harm, drone_metal, drone_evolve, drone_choir, drone_warm, drone_noise,
    // PERC (12)
    perc_kick, perc_snare, perc_tom, perc_clap, perc_cowbell, perc_808, perc_hat, perc_openhat, perc_rim, perc_clave, perc_conga, perc_zap,
    // AMB (14)
    amb_glacier, amb_shimmer, amb_choir, amb_deep, amb_aurora, amb_drift, amb_underwater, amb_meadow, amb_dust, amb_breath, amb_crystal, amb_nebula, amb_tape, amb_warmth,
    // SEQ (12)
    seq_trance, seq_acid, seq_wobble, seq_blade, seq_hoover, seq_pulse, seq_pluckseq, seq_chordseq, seq_sh, seq_gate, seq_arpbass, seq_8bit,
};
} // namespace

// Per-category delay + reverb voicing applied after each builder. Only the wet FX
// character is set, so a patch's dry tone, mixes and mod routing are as designed,
// while every preset still exercises the new delay/reverb controls.
void enhanceForDesktop(SynthPatch& p) noexcept
{
    switch(p.category)
    {
        case 0: case 8: case 10: // PAD / DRONE / AMB
            p.reverbSize = 0.84f; p.reverbTone = 0.32f;
            p.delaySync = true; p.delayDiv = 10; p.delayFeedback = 0.40f; p.delayTone = 0.55f; p.delayPing = true;
            break;
        case 1: case 4: // LEAD / STAB
            p.reverbSize = 0.55f; p.reverbTone = 0.45f;
            p.delaySync = true; p.delayDiv = 9; p.delayFeedback = 0.45f; p.delayTone = 0.45f; p.delayPing = true;
            break;
        case 2: // BASS
            p.reverbSize = 0.34f; p.reverbTone = 0.6f;
            p.delaySync = true; p.delayDiv = 7; p.delayFeedback = 0.24f; p.delayTone = 0.6f; p.delayPing = false;
            break;
        case 3: case 11: // ARP / SEQ
            p.reverbSize = 0.5f; p.reverbTone = 0.42f;
            p.delaySync = true; p.delayDiv = 7; p.delayFeedback = 0.5f; p.delayTone = 0.4f; p.delayPing = true;
            break;
        case 5: case 6: // KEYS / PLUCK
            p.reverbSize = 0.56f; p.reverbTone = 0.4f;
            p.delaySync = true; p.delayDiv = 7; p.delayFeedback = 0.3f; p.delayTone = 0.45f; p.delayPing = true;
            break;
        case 9: // PERC
            p.reverbSize = 0.44f; p.reverbTone = 0.5f;
            p.delaySync = true; p.delayDiv = 4; p.delayFeedback = 0.3f; p.delayTone = 0.5f; p.delayPing = true;
            break;
        default: // FX (7)
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

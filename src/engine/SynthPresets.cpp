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

// ============== factory bank (rewritten — full feature pass) ==============
// ================================ PAD (0) ================================
void pad_warm(SynthPatch& p)
{
    setName(p, "WARM ANALOG"); p.category = 0;
    p.width = 0.85f; p.ampGain = 0.70f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.10f, 1.0f);
    osc(p, 3, SAW, -0.08f, 0.55f); osc(p, 4, SAW, 0.16f, 0.42f, 1);
    p.subLevel = 0.16f; p.drift = 0.30f;
    filt(p, FilterMode::LadderLP, 0.42f, 0.10f, 0.34f, 0.20f, 0.4f);
    p.env[0] = EnvParams{0.55f, 1.6f, 0.88f, 1.9f, 0.30f};
    p.env[1] = EnvParams{1.30f, 1.8f, 0.55f, 1.6f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.20f, 1.0f, 0.18f);   // one-sided cutoff bloom
    lfo(p, 1, LfoWave::Triangle, 0.13f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.10f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.40f);
    mod(p, 2, ModSource::Lfo2, ModDest::Detune, 0.50f);
    mod(p, 3, ModSource::Aftertouch, ModDest::EnvFltAmt, 0.30f);
    p.fxChorus = 0.32f; p.fxPhaser = 0.16f; p.fxReverb = 0.34f;
}
void pad_glass(SynthPatch& p)
{
    setName(p, "GLASS PAD"); p.category = 0;
    p.width = 0.82f; p.ampGain = 0.74f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f);
    osc(p, 1, SINE, 0.02f, 0.9f, 3);                 // octave-up bell partial
    p.osc[1].oscType = 1; p.osc[1].wtTable = 6; p.osc[1].wtMorph = 0.35f; // BRIGHT WT shimmer
    p.fm2to1 = 0.30f; p.ringMod = 0.16f;             // glassy FM + ring
    p.subLevel = 0.06f; p.drift = 0.16f;
    filt(p, FilterMode::SvfLP, 0.64f, 0.12f, 0.28f, 0.0f, 0.6f);
    p.env[0] = EnvParams{0.85f, 2.0f, 0.70f, 2.4f, 0.25f};
    p.env[2] = EnvParams{0.40f, 1.2f, 0.30f, 1.0f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.16f);
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.40f);   // bell bark on attack
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::ModWheel, ModDest::RingMod, 0.40f);
    mod(p, 3, ModSource::Aftertouch, ModDest::Morph2, 0.30f);
    p.fxChorus = 0.22f; p.fxReverb = 0.52f; p.fxDelay = 0.12f;
}
void pad_strings(SynthPatch& p)
{
    setName(p, "STRING MACHINE"); p.category = 0;
    p.width = 0.92f; p.ampGain = 0.62f; p.chorusMode = 2; p.chorusRate = 1.2f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.13f, 1.0f);
    osc(p, 2, SAW, -0.11f, 0.8f); osc(p, 3, SAW, 0.22f, 0.6f); osc(p, 4, SAW, -0.20f, 0.6f);
    p.drift = 0.38f;
    filt(p, FilterMode::SvfLP, 0.56f, 0.12f, 0.20f);
    p.env[0] = EnvParams{0.50f, 1.6f, 0.92f, 2.0f, 0.25f};
    p.env[1] = EnvParams{1.10f, 1.8f, 0.60f, 1.6f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.28f); lfo(p, 1, LfoWave::Triangle, 0.19f);
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.55f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.30f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.20f);
    p.fxChorus = 0.40f; p.fxReverb = 0.42f;
}
void pad_voices(SynthPatch& p)
{
    setName(p, "VOX CHOIR"); p.category = 0;
    p.width = 0.88f; p.ampGain = 0.60f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f);
    p.osc[0].oscType = 1; p.osc[0].wtTable = 15; p.osc[0].wtMorph = 0.25f;  // VOCAL
    osc(p, 1, SAW, 0.08f, 1.0f);
    p.osc[1].oscType = 1; p.osc[1].wtTable = 9; p.osc[1].wtMorph = 0.40f;   // FORMANT A
    osc(p, 3, SAW, -0.10f, 0.7f);
    p.osc[3].oscType = 1; p.osc[3].wtTable = 10; p.osc[3].wtMorph = 0.30f;  // FORMANT B
    p.drift = 0.30f;
    filt(p, FilterMode::SvfBP, 0.52f, 0.38f, 0.12f);  // formant-ish BP
    p.env[0] = EnvParams{1.10f, 1.8f, 0.92f, 2.6f, 0.20f};
    lfo(p, 0, LfoWave::Sine, 0.08f); lfo(p, 1, LfoWave::Triangle, 0.20f); lfo(p, 2, LfoWave::Sine, 0.15f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.22f);
    mod(p, 1, ModSource::Lfo2, ModDest::Morph1, 0.30f);
    mod(p, 2, ModSource::Lfo3, ModDest::Morph2, 0.30f);
    mod(p, 3, ModSource::Lfo1, ModDest::Detune, 0.16f);
    mod(p, 4, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.45f; p.fxReverb = 0.62f;
}
void pad_dark(SynthPatch& p)
{
    setName(p, "DARK PAD"); p.category = 0;
    p.width = 0.78f; p.ampGain = 0.76f; p.chorusMode = 0;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, -0.07f, 0.9f);
    osc(p, 3, SAW, 0.10f, 0.5f, 1); p.subLevel = 0.30f; p.subOctave = 2; p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.26f, 0.18f, 0.30f, 0.30f, 0.3f);
    p.env[0] = EnvParams{1.00f, 2.0f, 0.85f, 2.6f, 0.20f};
    p.env[1] = EnvParams{1.80f, 2.0f, 0.40f, 1.8f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.10f, 1.0f, 0.10f); lfo(p, 1, LfoWave::Triangle, 0.07f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.14f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.30f);
    mod(p, 2, ModSource::Aftertouch, ModDest::Cutoff, 0.30f);
    mod(p, 3, ModSource::ModWheel, ModDest::EnvFltAmt, 0.30f);
    p.mbLow = 0.20f; p.fxReverb = 0.44f; p.fxDelay = 0.10f;
}
void pad_sweep(SynthPatch& p)
{
    setName(p, "SWEEP PAD"); p.category = 0;
    p.width = 0.90f; p.ampGain = 0.66f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 1.0f); osc(p, 2, PULSE, -0.10f, 0.7f);
    p.subLevel = 0.14f; p.drift = 0.28f;
    p.filterRouting = 1;                              // serial: ladder then SVF-HP
    filt(p, FilterMode::LadderLP, 0.30f, 0.22f, 0.55f, 0.18f, 0.3f);
    p.filter2Mode = (int) FilterMode::SvfHP; p.filter2Cutoff = 0.18f; p.filter2Reso = 0.20f;
    p.filter2EnvAmt = 0.25f;
    p.env[0] = EnvParams{0.80f, 1.8f, 0.85f, 2.2f, 0.20f};
    p.env[1] = EnvParams{2.00f, 2.4f, 0.30f, 2.0f, 0.0f};  // slow rising sweep
    lfo(p, 0, LfoWave::Triangle, 0.09f, 1.0f, 0.0f); lfo(p, 1, LfoWave::Sine, 0.14f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.6f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.25f);
    mod(p, 2, ModSource::Lfo2, ModDest::Detune, 0.30f);
    mod(p, 3, ModSource::ModWheel, ModDest::EnvFltAmt, 0.40f);
    p.fxChorus = 0.30f; p.fxPhaser = 0.25f; p.fxReverb = 0.45f;
}
void pad_fm(SynthPatch& p)
{
    setName(p, "FM CRYSTAL"); p.category = 0;
    p.width = 0.84f; p.ampGain = 0.70f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 1.0f, 4);  // 4:1 high modulator
    p.fm2to1 = 0.40f; p.ringMod = 0.12f; p.drift = 0.12f;
    filt(p, FilterMode::SvfLP, 0.66f, 0.08f, 0.20f, 0.0f, 0.5f);
    p.env[0] = EnvParams{0.70f, 1.6f, 0.75f, 2.2f, 0.25f};
    p.env[2] = EnvParams{0.50f, 1.4f, 0.25f, 1.2f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.18f);
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.45f);
    mod(p, 1, ModSource::Lfo1, ModDest::FmAmount, 0.20f);
    mod(p, 2, ModSource::ModWheel, ModDest::FmAmount, 0.5f);
    mod(p, 3, ModSource::Aftertouch, ModDest::RingMod, 0.30f);
    p.fxChorus = 0.24f; p.fxDelay = 0.18f; p.fxReverb = 0.50f;
}
void pad_air(SynthPatch& p)
{
    setName(p, "AIR PAD"); p.category = 0;
    p.width = 0.95f; p.ampGain = 0.64f; p.chorusMode = 2;
    osc(p, 0, SINE, 0.0f, 1.0f);
    osc(p, 1, TRI, 0.06f, 0.8f, 3);
    p.osc[1].oscType = 1; p.osc[1].wtTable = 6; p.osc[1].wtMorph = 0.5f;  // BRIGHT air
    p.noiseLevel = 0.05f; p.drift = 0.20f;
    filt(p, FilterMode::SvfHP, 0.30f, 0.10f, 0.10f);  // airy, low end rolled off
    p.env[0] = EnvParams{1.40f, 2.2f, 0.85f, 3.0f, 0.20f};
    lfo(p, 0, LfoWave::Sine, 0.07f, 1.0f, 0.0f); lfo(p, 1, LfoWave::Triangle, 0.11f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.18f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.25f);
    mod(p, 2, ModSource::ModWheel, ModDest::NoiseLevel, 0.30f);
    mod(p, 3, ModSource::Velocity, ModDest::Amp, 0.30f);
    p.fxChorus = 0.35f; p.fxReverb = 0.60f; p.fxDelay = 0.14f;
}
void pad_jp(SynthPatch& p)
{
    setName(p, "VINTAGE BRASS"); p.category = 0;
    p.width = 0.80f; p.ampGain = 0.68f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.10f, 1.0f);
    osc(p, 2, PULSE, -0.08f, 0.8f); p.osc[2].pw = 0.4f;
    osc(p, 3, SAW, 0.18f, 0.5f, 1); p.subLevel = 0.12f; p.drift = 0.28f;
    filt(p, FilterMode::LadderLP, 0.40f, 0.14f, 0.45f, 0.22f, 0.5f);
    p.env[0] = EnvParams{0.40f, 1.4f, 0.82f, 1.6f, 0.25f};
    p.env[1] = EnvParams{0.60f, 1.2f, 0.45f, 1.2f, 0.0f};  // brassy swell
    lfo(p, 0, LfoWave::Sine, 4.8f); p.lfo[0].fade = 0.6f; p.lfo[0].delay = 0.3f;
    lfo(p, 1, LfoWave::Triangle, 0.15f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Lfo1, ModDest::Pitch, 0.008f);
    mod(p, 2, ModSource::Lfo2, ModDest::Pw3, 0.30f);
    mod(p, 3, ModSource::Velocity, ModDest::EnvFltAmt, 0.35f);
    p.fxChorus = 0.34f; p.fxReverb = 0.36f;
}
void pad_oct(SynthPatch& p)
{
    setName(p, "OCTAVE PAD"); p.category = 0;
    p.width = 0.86f; p.ampGain = 0.66f; p.chorusMode = 1;
    osc(p, 0, SAW, 0.0f, 1.0f, 2); osc(p, 1, SAW, 0.10f, 0.9f, 1);  // 8' + 16'
    osc(p, 2, TRI, -0.08f, 0.7f, 3); osc(p, 3, SAW, 0.14f, 0.5f, 0); // 4' + 32'
    p.subLevel = 0.18f; p.drift = 0.26f;
    filt(p, FilterMode::LadderLP, 0.48f, 0.12f, 0.30f, 0.18f, 0.45f);
    p.env[0] = EnvParams{0.60f, 1.6f, 0.88f, 2.0f, 0.20f};
    p.env[1] = EnvParams{1.00f, 1.6f, 0.50f, 1.4f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.18f, 1.0f, 0.12f); lfo(p, 1, LfoWave::Triangle, 0.12f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.14f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.35f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.20f);
    p.fxChorus = 0.30f; p.fxReverb = 0.40f;
}
void pad_wide(SynthPatch& p)
{
    setName(p, "HYPER WIDE"); p.category = 0;
    p.width = 1.0f; p.ampGain = 0.58f; p.chorusMode = 3;
    p.voiceMode = (int) VoiceMode::Unison; p.unisonCount = 7; p.unisonDetune = 0.16f; p.unisonSpread = 1.0f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.08f, 1.0f); osc(p, 2, SAW, -0.07f, 0.9f);
    p.subLevel = 0.10f; p.drift = 0.34f;
    filt(p, FilterMode::SvfLP, 0.58f, 0.12f, 0.25f, 0.0f, 0.4f);
    p.env[0] = EnvParams{0.70f, 1.8f, 0.90f, 2.4f, 0.20f};
    p.env[1] = EnvParams{1.40f, 1.8f, 0.55f, 1.6f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.12f, 1.0f, 0.10f); lfo(p, 1, LfoWave::Triangle, 0.09f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.16f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.30f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.40f);
    mod(p, 3, ModSource::Aftertouch, ModDest::Detune, 0.25f);
    p.fxChorus = 0.45f; p.fxPhaser = 0.20f; p.fxReverb = 0.50f;
}
void pad_mellow(SynthPatch& p)
{
    setName(p, "MELLOW PAD"); p.category = 0;
    p.width = 0.80f; p.ampGain = 0.74f; p.chorusMode = 1;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SINE, 0.05f, 0.9f);
    osc(p, 3, TRI, -0.06f, 0.5f, 1); p.subLevel = 0.20f; p.drift = 0.18f;
    filt(p, FilterMode::LadderLP, 0.44f, 0.08f, 0.22f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.90f, 1.8f, 0.90f, 2.4f, 0.20f};
    p.env[1] = EnvParams{1.40f, 1.6f, 0.45f, 1.4f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.10f, 1.0f, 0.08f); lfo(p, 1, LfoWave::Triangle, 0.13f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.10f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.22f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    mod(p, 3, ModSource::Aftertouch, ModDest::Cutoff, 0.25f);
    p.fxChorus = 0.30f; p.fxReverb = 0.40f; p.fxDelay = 0.08f;
}

// ================================ LEAD (1) ================================
void lead_super(SynthPatch& p)
{
    setName(p, "SUPER STACK"); p.category = 1; p.chorusMode = 2;
    p.width = 0.85f; p.ampGain = 0.42f;
    p.voiceMode = (int) VoiceMode::Unison; p.unisonCount = 7; p.unisonDetune = 0.20f; p.unisonSpread = 0.9f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.04f, 1.0f);
    osc(p, 2, SAW, -0.05f, 0.9f); osc(p, 3, SAW, 0.10f, 0.8f);
    p.subLevel = 0.10f; p.drift = 0.30f;
    filt(p, FilterMode::LadderLP, 0.64f, 0.10f, 0.30f, 0.0f, 0.5f);
    p.env[0] = EnvParams{0.02f, 0.6f, 0.85f, 0.5f, 0.0f};
    p.env[1] = EnvParams{0.05f, 0.5f, 0.55f, 0.45f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.18f, 1.0f, 0.12f); lfo(p, 1, LfoWave::Triangle, 0.11f);
    mod(p, 0, ModSource::ModWheel, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.40f);
    mod(p, 2, ModSource::ModWheel, ModDest::Detune, 0.30f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.20f);
    p.fxChorus = 0.35f; p.fxReverb = 0.30f; p.fxDelay = 0.24f;
}
void lead_sync(SynthPatch& p)
{
    setName(p, "SYNC LEAD"); p.category = 1;
    p.width = 0.55f; p.ampGain = 0.68f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.0f, 1.0f);
    p.sync2Mode = (int) SyncMode::Hard; p.drift = 0.15f;  // classic tear
    filt(p, FilterMode::LadderLP, 0.60f, 0.18f, 0.45f, 0.25f, 0.4f);
    p.env[0] = EnvParams{0.005f, 0.3f, 0.8f, 0.25f, 0.0f};
    p.env[1] = EnvParams{0.004f, 0.4f, 0.3f, 0.30f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 5.0f);  // vibrato
    mod(p, 0, ModSource::ModWheel, ModDest::Osc2Pitch, 0.60f);  // wheel sweeps sync pitch
    mod(p, 1, ModSource::EnvFilter, ModDest::Osc2Pitch, 0.12f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pitch, 0.010f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDrive = 0.15f; p.driveMode = 1; p.fxDelay = 0.22f; p.fxReverb = 0.20f;
}
void lead_soft(SynthPatch& p)
{
    setName(p, "SOFT LEAD"); p.category = 1;
    p.width = 0.5f; p.ampGain = 0.80f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Always; p.glideTime = 0.08f;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SINE, 0.03f, 0.8f);
    p.subLevel = 0.12f; p.drift = 0.12f;
    filt(p, FilterMode::SvfLP, 0.70f, 0.08f, 0.20f);
    p.env[0] = EnvParams{0.04f, 0.4f, 0.9f, 0.4f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 4.5f); p.lfo[0].fade = 0.6f; p.lfo[0].delay = 0.3f;  // delayed vibrato
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.012f);
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Depth, 0.60f);
    mod(p, 2, ModSource::Aftertouch, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.20f; p.fxDelay = 0.18f; p.fxReverb = 0.28f;
}
void lead_fm(SynthPatch& p)
{
    setName(p, "FM LEAD"); p.category = 1;
    p.width = 0.5f; p.ampGain = 0.78f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 1.0f, 3);  // 2:1 ratio modulator
    p.fm2to1 = 0.6f; p.drift = 0.10f;
    filt(p, FilterMode::SvfLP, 0.80f, 0.05f, 0.10f);
    p.env[0] = EnvParams{0.005f, 0.5f, 0.7f, 0.30f, 0.1f};
    p.env[2] = EnvParams{0.002f, 0.25f, 0.2f, 0.20f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.60f);   // bark transient
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.50f);
    mod(p, 2, ModSource::ModWheel, ModDest::FmAmount, 0.50f);
    p.fxDelay = 0.20f; p.fxReverb = 0.24f;
}
void lead_pwm(SynthPatch& p)
{
    setName(p, "PWM LEAD"); p.category = 1;
    p.width = 0.6f; p.ampGain = 0.72f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    osc(p, 1, PULSE, 0.06f, 0.9f); p.osc[1].pw = 0.5f;
    p.subLevel = 0.15f; p.drift = 0.15f;
    filt(p, FilterMode::LadderLP, 0.65f, 0.15f, 0.30f, 0.20f, 0.4f);
    p.env[0] = EnvParams{0.01f, 0.4f, 0.85f, 0.30f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 0.6f); lfo(p, 1, LfoWave::Triangle, 4.0f);
    mod(p, 0, ModSource::Lfo1, ModDest::Pw1, 0.45f);   // classic PWM
    mod(p, 1, ModSource::Lfo1, ModDest::Pw2, 0.40f);
    mod(p, 2, ModSource::Lfo2, ModDest::Pitch, 0.010f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.25f; p.fxDelay = 0.20f; p.fxReverb = 0.22f;
}
void lead_saw(SynthPatch& p)
{
    setName(p, "SAW LEAD"); p.category = 1;
    p.width = 0.5f; p.ampGain = 0.72f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.07f, 0.9f);
    osc(p, 3, SAW, -0.05f, 0.5f, 1); p.subLevel = 0.12f; p.drift = 0.18f;
    filt(p, FilterMode::LadderLP, 0.62f, 0.16f, 0.40f, 0.25f, 0.45f);
    p.env[0] = EnvParams{0.008f, 0.4f, 0.8f, 0.30f, 0.1f};
    p.env[1] = EnvParams{0.01f, 0.3f, 0.4f, 0.25f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 5.2f); p.lfo[0].delay = 0.25f; p.lfo[0].fade = 0.4f;
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.010f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.45f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    mod(p, 3, ModSource::Aftertouch, ModDest::Lfo1Depth, 0.50f);
    p.fxDrive = 0.12f; p.driveMode = 1; p.fxDelay = 0.22f; p.fxReverb = 0.24f;
}
void lead_square(SynthPatch& p)
{
    setName(p, "SQUARE LEAD"); p.category = 1;
    p.width = 0.5f; p.ampGain = 0.74f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    osc(p, 1, PULSE, 0.0f, 0.6f, 1); p.osc[1].pw = 0.5f;  // hollow octave-down
    p.subLevel = 0.15f; p.drift = 0.12f;
    filt(p, FilterMode::LadderLP, 0.66f, 0.12f, 0.30f, 0.20f, 0.45f);
    p.env[0] = EnvParams{0.006f, 0.3f, 0.85f, 0.25f, 0.1f};
    p.env[1] = EnvParams{0.006f, 0.25f, 0.4f, 0.20f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 5.0f); p.lfo[0].delay = 0.3f; p.lfo[0].fade = 0.5f;
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.012f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.40f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDelay = 0.20f; p.fxReverb = 0.22f;
}
void lead_pluck(SynthPatch& p)
{
    setName(p, "PLUCK LEAD"); p.category = 1;
    p.width = 0.55f; p.ampGain = 0.80f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.06f, 0.7f); p.osc[1].pw = 0.4f;
    p.subLevel = 0.10f; p.drift = 0.16f;
    filt(p, FilterMode::LadderLP, 0.50f, 0.24f, 0.55f, 0.20f, 0.45f);
    p.env[0] = EnvParams{0.003f, 0.22f, 0.30f, 0.18f, 0.15f};
    p.env[1] = EnvParams{0.003f, 0.18f, 0.0f, 0.14f, 0.0f};  // snappy filter pluck
    lfo(p, 0, LfoWave::Sine, 0.2f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.40f);
    mod(p, 2, ModSource::Velocity, ModDest::EnvFltAmt, 0.30f);
    p.fxDelay = 0.28f; p.fxReverb = 0.30f;
}
void lead_fifth(SynthPatch& p)
{
    setName(p, "POWER FIFTH"); p.category = 1;
    p.width = 0.55f; p.ampGain = 0.66f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.0f, 0.9f);
    p.osc[1].detune = 7.02f;                          // +5th interval stack
    osc(p, 3, SAW, 0.05f, 0.5f, 1); p.subLevel = 0.14f; p.drift = 0.16f;
    filt(p, FilterMode::LadderLP, 0.58f, 0.14f, 0.40f, 0.25f, 0.5f);
    p.env[0] = EnvParams{0.008f, 0.35f, 0.82f, 0.30f, 0.1f};
    p.env[1] = EnvParams{0.01f, 0.30f, 0.40f, 0.25f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 5.0f); p.lfo[0].delay = 0.3f; p.lfo[0].fade = 0.4f;
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.010f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.40f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDrive = 0.18f; p.driveMode = 1; p.fxDelay = 0.20f; p.fxReverb = 0.26f;
}
void lead_dist(SynthPatch& p)
{
    setName(p, "DIST LEAD"); p.category = 1;
    p.width = 0.55f; p.ampGain = 0.58f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.08f, 1.0f); osc(p, 2, PULSE, -0.06f, 0.7f);
    p.subLevel = 0.14f; p.drift = 0.18f;
    p.filterRouting = 1;                              // serial drive + tone shaping
    filt(p, FilterMode::LadderLP, 0.55f, 0.20f, 0.40f, 0.40f, 0.45f);
    p.filter2Mode = (int) FilterMode::SvfLP; p.filter2Cutoff = 0.7f; p.filter2Reso = 0.10f; p.filter2Drive = 0.3f;
    p.env[0] = EnvParams{0.006f, 0.35f, 0.85f, 0.28f, 0.1f};
    p.env[1] = EnvParams{0.008f, 0.30f, 0.45f, 0.22f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 5.2f); p.lfo[0].delay = 0.25f; p.lfo[0].fade = 0.4f;
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.012f);
    mod(p, 1, ModSource::ModWheel, ModDest::FilterDrive, 0.50f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDrive = 0.55f; p.driveMode = 4; p.driveTone = 0.25f;  // FUZZ
    p.fxDelay = 0.22f; p.fxReverb = 0.24f;
}
void lead_whistle(SynthPatch& p)
{
    setName(p, "WHISTLE LEAD"); p.category = 1;
    p.width = 0.45f; p.ampGain = 0.82f; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Always; p.glideTime = 0.06f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.2f, 3);  // faint octave sparkle
    p.drift = 0.08f;
    filt(p, FilterMode::SvfLP, 0.82f, 0.06f, 0.10f);
    p.env[0] = EnvParams{0.03f, 0.3f, 0.95f, 0.30f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 5.5f); p.lfo[0].delay = 0.4f; p.lfo[0].fade = 0.5f;  // delayed vibrato
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.015f);
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Depth, 0.70f);
    mod(p, 2, ModSource::Aftertouch, ModDest::Pitch, 0.04f);
    p.fxChorus = 0.18f; p.fxDelay = 0.26f; p.fxReverb = 0.34f;
}
void lead_uni(SynthPatch& p)
{
    setName(p, "UNISON LEAD"); p.category = 1;
    p.width = 0.7f; p.ampGain = 0.50f;
    p.voiceMode = (int) VoiceMode::Unison; p.unisonCount = 5; p.unisonDetune = 0.14f; p.unisonSpread = 0.7f;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.05f, 0.8f); p.osc[1].pw = 0.45f;
    p.subLevel = 0.12f; p.drift = 0.24f;
    filt(p, FilterMode::LadderLP, 0.60f, 0.14f, 0.38f, 0.22f, 0.45f);
    p.env[0] = EnvParams{0.01f, 0.4f, 0.85f, 0.35f, 0.1f};
    p.env[1] = EnvParams{0.02f, 0.35f, 0.45f, 0.28f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 5.0f); p.lfo[0].delay = 0.3f; p.lfo[0].fade = 0.4f;
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.010f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.45f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.25f; p.fxDelay = 0.22f; p.fxReverb = 0.26f;
}

// ================================ BASS (2) ================================
void bass_reese(SynthPatch& p)
{
    setName(p, "REESE"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.06f; p.width = 0.45f; p.ampGain = 0.70f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.22f, 1.0f); osc(p, 2, SAW, -0.20f, 0.9f);
    osc(p, 3, SAW, 0.40f, 0.7f); p.subLevel = 0.40f; p.drift = 0.20f;
    filt(p, FilterMode::LadderLP, 0.30f, 0.20f, 0.20f, 0.30f, 0.3f);
    p.env[0] = EnvParams{0.005f, 0.4f, 0.85f, 0.20f, 0.0f};
    p.env[1] = EnvParams{0.01f, 0.3f, 0.4f, 0.20f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.25f);
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.35f);  // the reese sweep
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.40f);
    p.mbLow = 0.30f; p.mbMid = 0.15f; p.fxChorus = 0.12f; p.fxReverb = 0.08f;
}
void bass_sub(SynthPatch& p)
{
    setName(p, "DEEP SUB"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f; p.width = 0.2f; p.ampGain = 0.88f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.4f); p.subLevel = 0.50f; p.drift = 0.05f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.05f, 0.25f, 0.20f, 0.2f);
    p.env[0] = EnvParams{0.004f, 0.3f, 0.9f, 0.15f, 0.0f};
    p.env[1] = EnvParams{0.003f, 0.18f, 0.2f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.30f);
    mod(p, 1, ModSource::Velocity, ModDest::Amp, 0.40f);
    p.mbLow = 0.25f; p.fxReverb = 0.05f;
}
void bass_acid(SynthPatch& p)
{
    setName(p, "ACID SQUELCH"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f; p.width = 0.25f; p.ampGain = 0.74f;
    osc(p, 0, SAW, 0.0f, 1.0f); p.drift = 0.10f;
    filt(p, FilterMode::LadderLP, 0.24f, 0.44f, 0.60f, 0.45f, 0.2f);  // squelch
    p.env[0] = EnvParams{0.004f, 0.25f, 0.4f, 0.12f, 0.0f};
    p.env[1] = EnvParams{0.004f, 0.20f, 0.05f, 0.12f, 0.0f};  // snappy filter env
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.50f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    mod(p, 2, ModSource::ModWheel, ModDest::Resonance, 0.30f);
    p.fxDrive = 0.40f; p.driveMode = 2; p.driveTone = 0.20f;  // DIODE bite
    p.fxDelay = 0.18f; p.fxReverb = 0.12f;
}
void bass_fm(SynthPatch& p)
{
    setName(p, "FM E-BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.03f; p.width = 0.3f; p.ampGain = 0.80f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 1.0f); p.fm2to1 = 0.5f;
    p.subLevel = 0.30f; p.drift = 0.08f;
    filt(p, FilterMode::SvfLP, 0.60f, 0.10f, 0.20f);
    p.env[0] = EnvParams{0.004f, 0.3f, 0.7f, 0.15f, 0.0f};
    p.env[2] = EnvParams{0.002f, 0.12f, 0.0f, 0.10f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.70f);  // clang on attack
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.50f);
    p.mbLow = 0.15f; p.fxReverb = 0.08f;
}
void bass_growl(SynthPatch& p)
{
    setName(p, "GROWL BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f; p.width = 0.4f; p.ampGain = 0.64f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.15f, 1.0f); osc(p, 2, PULSE, -0.12f, 0.8f);
    p.subLevel = 0.35f; p.drift = 0.18f;
    filt(p, FilterMode::LadderLP, 0.35f, 0.35f, 0.40f, 0.50f, 0.3f);
    p.env[0] = EnvParams{0.006f, 0.4f, 0.8f, 0.20f, 0.0f};
    lfoSync(p, 0, LfoWave::Triangle, 7, 1.0f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.40f);  // rhythmic growl
    mod(p, 1, ModSource::ModWheel, ModDest::FilterDrive, 0.40f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDrive = 0.35f; p.driveMode = 1; p.mbLow = 0.20f; p.mbMid = 0.20f; p.fxReverb = 0.10f;
}
void bass_pluck(SynthPatch& p)
{
    setName(p, "PLUCK BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Off; p.width = 0.3f; p.ampGain = 0.80f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.05f, 0.6f); p.osc[1].pw = 0.4f;
    p.subLevel = 0.30f; p.drift = 0.10f;
    filt(p, FilterMode::LadderLP, 0.40f, 0.26f, 0.55f, 0.25f, 0.3f);
    p.env[0] = EnvParams{0.003f, 0.20f, 0.25f, 0.14f, 0.1f};
    p.env[1] = EnvParams{0.003f, 0.16f, 0.0f, 0.10f, 0.0f};  // snappy pluck
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.40f);
    p.mbLow = 0.20f; p.fxReverb = 0.08f;
}
void bass_square(SynthPatch& p)
{
    setName(p, "SQUARE BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f; p.width = 0.3f; p.ampGain = 0.78f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    osc(p, 1, PULSE, 0.0f, 0.6f, 1); p.osc[1].pw = 0.5f;  // octave-down body
    p.subLevel = 0.35f; p.drift = 0.10f;
    filt(p, FilterMode::LadderLP, 0.42f, 0.18f, 0.40f, 0.22f, 0.3f);
    p.env[0] = EnvParams{0.004f, 0.3f, 0.8f, 0.15f, 0.0f};
    p.env[1] = EnvParams{0.004f, 0.22f, 0.3f, 0.14f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.5f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.40f);
    mod(p, 1, ModSource::Lfo1, ModDest::Pw1, 0.20f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.mbLow = 0.20f; p.fxReverb = 0.08f;
}
void bass_dirty(SynthPatch& p)
{
    setName(p, "DIRTY BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f; p.width = 0.35f; p.ampGain = 0.58f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.10f, 0.9f); osc(p, 2, PULSE, -0.08f, 0.7f);
    p.subLevel = 0.35f; p.drift = 0.16f;
    p.filterRouting = 2;                              // parallel LP + BP for grit + body
    filt(p, FilterMode::LadderLP, 0.34f, 0.28f, 0.45f, 0.45f, 0.3f);
    p.filter2Mode = (int) FilterMode::SvfBP; p.filter2Cutoff = 0.5f; p.filter2Reso = 0.40f; p.filter2Drive = 0.3f;
    p.env[0] = EnvParams{0.005f, 0.35f, 0.8f, 0.18f, 0.0f};
    p.env[1] = EnvParams{0.006f, 0.25f, 0.35f, 0.16f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::ModWheel, ModDest::FilterDrive, 0.40f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDrive = 0.55f; p.driveMode = 4; p.driveTone = 0.20f;  // FUZZ
    p.mbLow = 0.25f; p.mbMid = 0.25f; p.fxReverb = 0.08f;
}
void bass_wobble(SynthPatch& p)
{
    setName(p, "WOBBLE BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f; p.width = 0.4f; p.ampGain = 0.62f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.16f, 1.0f); osc(p, 2, SAW, -0.14f, 0.9f);
    p.subLevel = 0.40f; p.drift = 0.18f;
    filt(p, FilterMode::LadderLP, 0.28f, 0.40f, 0.30f, 0.40f, 0.3f);
    p.env[0] = EnvParams{0.006f, 0.4f, 0.85f, 0.20f, 0.0f};
    lfoSync(p, 0, LfoWave::Sine, 7, 1.0f);            // 1/8 wobble
    lfoSync(p, 1, LfoWave::Triangle, 4, 1.0f);        // faster option via mod wheel
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Rate, 0.60f);
    mod(p, 2, ModSource::Lfo2, ModDest::Cutoff, 0.20f);
    p.fxDrive = 0.35f; p.driveMode = 1; p.mbLow = 0.25f; p.mbMid = 0.20f; p.fxReverb = 0.10f;
}
void bass_deep(SynthPatch& p)
{
    setName(p, "DEEP HOUSE"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f; p.width = 0.3f; p.ampGain = 0.80f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.04f, 0.6f); p.subLevel = 0.45f; p.drift = 0.08f;
    filt(p, FilterMode::LadderLP, 0.40f, 0.10f, 0.28f, 0.18f, 0.25f);
    p.env[0] = EnvParams{0.005f, 0.30f, 0.78f, 0.20f, 0.1f};
    p.env[1] = EnvParams{0.006f, 0.22f, 0.25f, 0.16f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.30f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    mod(p, 2, ModSource::Velocity, ModDest::Amp, 0.30f);
    p.mbLow = 0.25f; p.fxChorus = 0.10f; p.fxReverb = 0.08f;
}
void bass_house(SynthPatch& p)
{
    setName(p, "ORGAN BASS"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Off; p.width = 0.35f; p.ampGain = 0.76f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    osc(p, 1, SINE, 0.0f, 0.6f, 1);                  // sub octave organ tone
    osc(p, 2, SINE, 0.0f, 0.4f, 3); p.subLevel = 0.30f; p.drift = 0.08f;
    filt(p, FilterMode::LadderLP, 0.46f, 0.14f, 0.35f, 0.20f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.18f, 0.6f, 0.14f, 0.1f};
    p.env[1] = EnvParams{0.004f, 0.16f, 0.2f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.40f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    p.mbLow = 0.18f; p.fxChorus = 0.15f; p.fxReverb = 0.10f;
}
void bass_retro(SynthPatch& p)
{
    setName(p, "RETRO 8-BIT"); p.category = 2; p.voiceMode = (int) VoiceMode::Mono;
    p.glideMode = (int) GlideMode::Off; p.width = 0.25f; p.ampGain = 0.78f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f; p.osc[0].crush = 0.45f; p.osc[0].srReduce = 0.35f;
    osc(p, 1, PULSE, 0.0f, 0.5f, 1); p.osc[1].pw = 0.25f; p.osc[1].crush = 0.4f;
    p.subLevel = 0.25f; p.drift = 0.05f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.12f, 0.35f, 0.15f, 0.3f);
    p.env[0] = EnvParams{0.002f, 0.18f, 0.5f, 0.10f, 0.1f};
    p.env[1] = EnvParams{0.002f, 0.14f, 0.1f, 0.08f, 0.0f};
    lfo(p, 0, LfoWave::Square, 8.0f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.35f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pw1, 0.15f);
    p.fxReverb = 0.06f;
}
// ================================ ARP (3) ================================
void arp_pluck(SynthPatch& p)
{
    setName(p, "ARP PLUCK"); p.category = 3; p.width = 0.62f; p.ampGain = 0.78f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.09f, 0.7f); p.osc[1].pw = 0.42f;
    p.subLevel = 0.12f; p.drift = 0.20f;
    filt(p, FilterMode::LadderLP, 0.46f, 0.26f, 0.55f, 0.18f, 0.45f);
    p.env[0] = EnvParams{0.002f, 0.16f, 0.0f, 0.14f, 0.22f};
    p.env[1] = EnvParams{0.002f, 0.14f, 0.0f, 0.10f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::UpDown; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.50f; p.arp.swing = 0.08f;
    lfo(p, 0, LfoWave::Sine, 0.25f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    mod(p, 2, ModSource::Lfo1, ModDest::Cutoff, 0.10f);
    p.fxChorus = 0.18f; p.fxDelay = 0.30f; p.delaySync = true; p.delayDiv = 7; p.delayPing = true; p.fxReverb = 0.28f;
}
void arp_bell(SynthPatch& p)
{
    setName(p, "BELL ARP"); p.category = 3; p.width = 0.72f; p.ampGain = 0.72f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.01f, 0.85f, 4); // 2' modulator
    p.fm2to1 = 0.45f; p.ringMod = 0.22f; p.drift = 0.05f;
    filt(p, FilterMode::SvfLP, 0.74f, 0.06f, 0.16f, 0.0f, 0.5f);
    p.env[0] = EnvParams{0.001f, 0.55f, 0.0f, 0.45f, 0.20f};
    p.env[2] = EnvParams{0.001f, 0.18f, 0.0f, 0.14f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::Up; p.arp.octaves = 3;
    p.arp.syncDiv = 4; p.arp.gate = 0.45f; p.arp.swing = 0.05f;
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.45f);
    mod(p, 2, ModSource::Velocity, ModDest::RingMod, 0.30f);
    p.fxDelay = 0.32f; p.delaySync = true; p.delayDiv = 9; p.delayPing = true; p.fxReverb = 0.42f;
}
void arp_saw(SynthPatch& p)
{
    setName(p, "SAW ARP"); p.category = 3; p.width = 0.72f; p.ampGain = 0.64f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.13f, 1.0f); osc(p, 3, SAW, -0.11f, 0.6f);
    p.subLevel = 0.12f; p.drift = 0.26f;
    filt(p, FilterMode::LadderLP, 0.50f, 0.20f, 0.42f, 0.15f, 0.40f);
    p.env[0] = EnvParams{0.003f, 0.28f, 0.35f, 0.20f, 0.18f};
    p.env[1] = EnvParams{0.003f, 0.24f, 0.18f, 0.18f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::UpDown; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.55f; p.arp.swing = 0.10f;
    lfoSync(p, 0, LfoWave::Triangle, 10);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.40f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.20f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.30f; p.fxDelay = 0.30f; p.delaySync = true; p.delayDiv = 7; p.fxReverb = 0.30f;
}
void arp_pwm(SynthPatch& p)
{
    setName(p, "PULSE ARP"); p.category = 3; p.width = 0.66f; p.ampGain = 0.76f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    osc(p, 1, PULSE, 0.07f, 0.8f); p.osc[1].pw = 0.5f;
    p.subLevel = 0.10f; p.drift = 0.20f;
    filt(p, FilterMode::SvfLP, 0.55f, 0.18f, 0.40f, 0.10f, 0.40f);
    p.env[0] = EnvParams{0.003f, 0.22f, 0.15f, 0.16f, 0.18f};
    p.env[1] = EnvParams{0.003f, 0.18f, 0.08f, 0.14f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::Random; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.50f; p.arp.swing = 0.12f;
    lfo(p, 0, LfoWave::Sine, 0.7f); lfo(p, 1, LfoWave::Triangle, 0.45f);
    mod(p, 0, ModSource::Lfo1, ModDest::Pw1, 0.45f);
    mod(p, 1, ModSource::Lfo2, ModDest::Pw2, 0.40f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.40f);
    p.fxChorus = 0.22f; p.fxDelay = 0.30f; p.delaySync = true; p.delayDiv = 9; p.fxReverb = 0.30f;
}
void arp_oct(SynthPatch& p)
{
    setName(p, "OCTAVE ARP"); p.category = 3; p.width = 0.60f; p.ampGain = 0.78f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.0f, 0.6f, 1); p.osc[1].pw = 0.45f;
    p.subLevel = 0.18f; p.drift = 0.16f;
    filt(p, FilterMode::LadderLP, 0.50f, 0.22f, 0.50f, 0.20f, 0.50f);
    p.env[0] = EnvParams{0.002f, 0.15f, 0.0f, 0.12f, 0.18f};
    p.env[1] = EnvParams{0.002f, 0.15f, 0.0f, 0.12f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::UpDownInc; p.arp.octaves = 3;
    p.arp.syncDiv = 7; p.arp.gate = 0.45f; p.arp.swing = 0.06f;
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDelay = 0.32f; p.delaySync = true; p.delayDiv = 7; p.delayPing = true; p.fxReverb = 0.30f;
}
void arp_house(SynthPatch& p)
{
    setName(p, "HOUSE ARP"); p.category = 3; p.width = 0.68f; p.ampGain = 0.70f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.10f, 0.85f);
    osc(p, 2, SINE, 0.0f, 0.5f, 3); // octave-up body
    p.subLevel = 0.14f; p.drift = 0.20f;
    filt(p, FilterMode::LadderLP, 0.50f, 0.20f, 0.50f, 0.25f, 0.45f);
    p.fxDrive = 0.20f; p.driveMode = 1; p.driveTone = 0.15f; // tube warmth
    p.env[0] = EnvParams{0.002f, 0.20f, 0.0f, 0.16f, 0.20f};
    p.env[1] = EnvParams{0.002f, 0.18f, 0.0f, 0.14f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::Up; p.arp.octaves = 2;
    p.arp.syncDiv = 4; p.arp.gate = 0.48f; p.arp.swing = 0.14f;
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.50f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.28f; p.fxDelay = 0.30f; p.delaySync = true; p.delayDiv = 9; p.delayPing = true; p.fxReverb = 0.26f;
}
void arp_dark(SynthPatch& p)
{
    setName(p, "DARK ARP"); p.category = 3; p.width = 0.70f; p.ampGain = 0.72f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, -0.08f, 0.8f);
    osc(p, 3, SAW, 0.10f, 0.45f, 1); p.subLevel = 0.30f; p.subOctave = 2; p.drift = 0.22f;
    filt(p, FilterMode::Steiner, 0.30f, 0.30f, 0.45f, 0.30f, 0.35f);
    p.fxDrive = 0.25f; p.driveMode = 2; p.driveTone = -0.20f; // diode grit, dark tilt
    p.env[0] = EnvParams{0.003f, 0.30f, 0.0f, 0.22f, 0.20f};
    p.env[1] = EnvParams{0.003f, 0.26f, 0.0f, 0.20f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::Down; p.arp.octaves = 2;
    p.arp.syncDiv = 7; p.arp.gate = 0.55f; p.arp.swing = 0.10f;
    lfoSync(p, 0, LfoWave::Triangle, 7);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.50f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDelay = 0.34f; p.delaySync = true; p.delayDiv = 9; p.delayPing = true; p.fxReverb = 0.42f;
}
void arp_trance(SynthPatch& p)
{
    setName(p, "TRANCE ARP"); p.category = 3; p.width = 0.85f; p.ampGain = 0.60f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.14f, 1.0f);
    osc(p, 2, SAW, -0.12f, 0.85f); osc(p, 3, SAW, 0.28f, 0.6f);
    p.subLevel = 0.10f; p.drift = 0.30f;
    p.filterRouting = 2; // parallel LP + HP -> bright, hollow
    filt(p, FilterMode::LadderLP, 0.50f, 0.18f, 0.45f, 0.15f, 0.45f);
    p.filter2Mode = (int) FilterMode::SvfHP; p.filter2Cutoff = 0.18f; p.filter2Reso = 0.10f;
    p.env[0] = EnvParams{0.002f, 0.24f, 0.25f, 0.20f, 0.18f};
    p.env[1] = EnvParams{0.002f, 0.22f, 0.12f, 0.18f, 0.0f};
    p.arp.on = true; p.arp.mode = (int) ArpMode::Up; p.arp.octaves = 1;
    p.arp.syncDiv = 4; p.arp.gate = 0.42f; p.arp.swing = 0.0f;
    lfo(p, 0, LfoWave::Sine, 0.20f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.40f);
    mod(p, 2, ModSource::Lfo1, ModDest::Detune, 0.30f);
    p.fxChorus = 0.32f; p.fxPhaser = 0.20f; p.fxDelay = 0.34f; p.delaySync = true; p.delayDiv = 9; p.delayPing = true; p.fxReverb = 0.40f;
}

// ================================ STAB (4) ================================
void stab_house(SynthPatch& p)
{
    setName(p, "HOUSE STAB"); p.category = 4; p.width = 0.72f; p.ampGain = 0.68f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.10f, 0.8f); p.osc[1].pw = 0.40f;
    osc(p, 2, SINE, 0.0f, 0.5f, 3); p.osc[2].oscType = 1; p.osc[2].wtTable = 7; // ORGAN drawbar body
    p.drift = 0.20f;
    filt(p, FilterMode::LadderLP, 0.52f, 0.20f, 0.50f, 0.20f, 0.45f);
    p.fxDrive = 0.18f; p.driveMode = 1; p.driveTone = 0.10f;
    p.env[0] = EnvParams{0.003f, 0.24f, 0.0f, 0.20f, 0.20f};
    p.env[1] = EnvParams{0.003f, 0.20f, 0.0f, 0.18f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.50f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.40f);
    p.fxChorus = 0.28f; p.fxDelay = 0.22f; p.delaySync = true; p.delayDiv = 9; p.fxReverb = 0.32f;
}
void stab_rave(SynthPatch& p)
{
    setName(p, "RAVE STAB"); p.category = 4; p.width = 0.82f; p.ampGain = 0.54f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.18f, 0.9f); p.osc[1].pw = 0.46f;
    osc(p, 2, SAW, -0.16f, 0.85f); osc(p, 3, SAW, 0.30f, 0.6f);
    p.subLevel = 0.12f; p.drift = 0.32f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.28f, 0.50f, 0.35f, 0.40f);
    p.fxDrive = 0.40f; p.driveMode = 2; p.driveTone = 0.10f; // diode bite
    p.env[0] = EnvParams{0.003f, 0.40f, 0.20f, 0.30f, 0.15f};
    p.env[1] = EnvParams{0.003f, 0.30f, 0.10f, 0.20f, 0.0f};
    p.env[2] = EnvParams{0.0f, 0.18f, 0.0f, 0.10f, 0.0f}; // pitch-blip envelope
    lfo(p, 0, LfoWave::Sine, 5.5f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::ModWheel, ModDest::Osc2Pitch, 0.50f); // hoover pitch sweep
    mod(p, 2, ModSource::EnvAux, ModDest::Pitch, -0.10f);
    mod(p, 3, ModSource::Lfo1, ModDest::Pitch, 0.012f);
    p.fxChorus = 0.35f; p.fxDelay = 0.20f; p.delaySync = true; p.delayDiv = 7; p.fxReverb = 0.30f;
}
void stab_brass(SynthPatch& p)
{
    setName(p, "BRASS STAB"); p.category = 4; p.width = 0.60f; p.ampGain = 0.68f; p.chorusMode = 1;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.08f, 1.0f); osc(p, 2, SAW, -0.06f, 0.7f);
    p.subLevel = 0.10f; p.drift = 0.20f;
    filt(p, FilterMode::LadderLP, 0.42f, 0.12f, 0.55f, 0.30f, 0.50f);
    p.fxDrive = 0.25f; p.driveMode = 1; p.driveTone = 0.20f; // tube brass
    p.env[0] = EnvParams{0.020f, 0.30f, 0.60f, 0.25f, 0.20f};
    p.env[1] = EnvParams{0.030f, 0.35f, 0.30f, 0.28f, 0.0f}; // swell
    lfo(p, 0, LfoWave::Sine, 5.0f); p.lfo[0].fade = 0.35f;
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::EnvFltAmt, 0.40f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pitch, 0.008f);
    p.fxReverb = 0.30f; p.fxDelay = 0.12f;
}
void stab_organ(SynthPatch& p)
{
    setName(p, "ORGAN STAB"); p.category = 4; p.width = 0.58f; p.ampGain = 0.70f; p.chorusMode = 3;
    osc(p, 0, SINE, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 7; // drawbars
    osc(p, 1, SINE, 0.0f, 0.7f, 3); osc(p, 2, SINE, 0.0f, 0.5f, 4);
    p.drift = 0.05f;
    filt(p, FilterMode::SvfLP, 0.78f, 0.06f, 0.12f, 0.0f, 0.50f);
    p.fxDrive = 0.30f; p.driveMode = 1; p.driveTone = 0.10f; // tube/leslie grit
    p.env[0] = EnvParams{0.003f, 0.10f, 0.90f, 0.12f, 0.10f};
    lfo(p, 0, LfoWave::Sine, 6.2f); // leslie
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.008f);
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Rate, 0.50f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.20f);
    p.fxChorus = 0.32f; p.fxReverb = 0.24f;
}
void stab_chord(SynthPatch& p)
{
    setName(p, "CHORD HIT"); p.category = 4; p.width = 0.78f; p.ampGain = 0.58f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 0.9f);
    osc(p, 3, SAW, -0.10f, 0.7f); osc(p, 4, PULSE, 0.20f, 0.5f, 1); p.osc[4].pw = 0.45f;
    p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.15f, 0.40f, 0.18f, 0.40f);
    p.env[0] = EnvParams{0.003f, 0.50f, 0.0f, 0.40f, 0.18f};
    p.env[1] = EnvParams{0.003f, 0.40f, 0.0f, 0.30f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.42f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.30f; p.fxDelay = 0.26f; p.delaySync = true; p.delayDiv = 9; p.fxReverb = 0.42f;
}
void stab_pizz(SynthPatch& p)
{
    setName(p, "PIZZ STAB"); p.category = 4; p.width = 0.60f; p.ampGain = 0.74f;
    osc(p, 0, SAW, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 14; p.osc[0].wtMorph = 0.0f; // STRING
    osc(p, 1, TRI, 0.06f, 0.5f);
    p.drift = 0.10f;
    filt(p, FilterMode::SvfLP, 0.58f, 0.18f, 0.50f, 0.10f, 0.50f);
    p.env[0] = EnvParams{0.002f, 0.14f, 0.0f, 0.12f, 0.30f};
    p.env[1] = EnvParams{0.002f, 0.12f, 0.0f, 0.10f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.50f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.40f);
    mod(p, 2, ModSource::Velocity, ModDest::Amp, 0.40f);
    p.fxChorus = 0.15f; p.fxDelay = 0.18f; p.fxReverb = 0.40f;
}
void stab_syn(SynthPatch& p)
{
    setName(p, "SYNTH STAB"); p.category = 4; p.width = 0.72f; p.ampGain = 0.64f; p.chorusMode = 1;
    osc(p, 0, SAW, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 13; p.osc[0].wtMorph = 0.30f; // BRASS wt
    osc(p, 1, SINE, 0.0f, 0.8f, 3); p.fm2to1 = 0.30f; // FM bite
    osc(p, 2, SAW, -0.12f, 0.6f);
    p.drift = 0.18f;
    p.filterRouting = 1; // serial LP -> HP
    filt(p, FilterMode::LadderLP, 0.50f, 0.20f, 0.50f, 0.20f, 0.45f);
    p.filter2Mode = (int) FilterMode::SvfHP; p.filter2Cutoff = 0.20f; p.filter2Reso = 0.15f; p.filter2EnvAmt = 0.10f;
    p.env[0] = EnvParams{0.003f, 0.30f, 0.0f, 0.24f, 0.20f};
    p.env[1] = EnvParams{0.003f, 0.24f, 0.0f, 0.20f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.12f, 0.0f, 0.10f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.50f);
    mod(p, 1, ModSource::EnvAux, ModDest::FmAmount, 0.40f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    mod(p, 3, ModSource::ModWheel, ModDest::Morph1, 0.30f);
    p.fxChorus = 0.24f; p.fxDelay = 0.24f; p.delaySync = true; p.delayDiv = 9; p.fxReverb = 0.34f;
}
void stab_det(SynthPatch& p)
{
    setName(p, "DETUNE STAB"); p.category = 4; p.width = 0.90f; p.ampGain = 0.56f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.22f, 1.0f);
    osc(p, 2, SAW, -0.20f, 0.9f); osc(p, 3, SAW, 0.40f, 0.7f); osc(p, 4, SAW, -0.38f, 0.7f);
    p.subLevel = 0.10f; p.drift = 0.35f;
    p.filterRouting = 2; // parallel
    filt(p, FilterMode::LadderLP, 0.50f, 0.16f, 0.45f, 0.15f, 0.40f);
    p.filter2Mode = (int) FilterMode::SvfHP; p.filter2Cutoff = 0.15f; p.filter2Reso = 0.10f;
    p.env[0] = EnvParams{0.004f, 0.40f, 0.0f, 0.34f, 0.18f};
    p.env[1] = EnvParams{0.004f, 0.32f, 0.0f, 0.26f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.18f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Lfo1, ModDest::Detune, 0.30f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.40f; p.fxPhaser = 0.15f; p.fxDelay = 0.26f; p.delaySync = true; p.delayDiv = 9; p.fxReverb = 0.42f;
}

// ================================ KEYS (5) ================================
void keys_rhodes(SynthPatch& p)
{
    setName(p, "TINE KEYS"); p.category = 5; p.width = 0.56f; p.ampGain = 0.78f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.9f, 4); p.fm2to1 = 0.30f; // tine bark
    p.drift = 0.06f;
    filt(p, FilterMode::SvfLP, 0.70f, 0.05f, 0.20f, 0.0f, 0.50f);
    p.env[0] = EnvParams{0.003f, 1.40f, 0.32f, 0.50f, 0.35f};
    p.env[2] = EnvParams{0.001f, 0.12f, 0.0f, 0.10f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.50f);   // attack tine
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.60f); // velocity -> bark
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.30f; p.fxReverb = 0.30f; p.fxDelay = 0.06f;
}
void keys_wurli(SynthPatch& p)
{
    setName(p, "REED EP"); p.category = 5; p.width = 0.50f; p.ampGain = 0.78f; p.chorusMode = 0;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.7f, 4); p.fm2to1 = 0.45f;
    p.drift = 0.06f;
    filt(p, FilterMode::SvfLP, 0.66f, 0.06f, 0.20f, 0.0f, 0.50f);
    p.fxDrive = 0.18f; p.driveMode = 1; p.driveTone = 0.15f; // reedy bark
    p.env[0] = EnvParams{0.003f, 1.00f, 0.28f, 0.40f, 0.40f};
    p.env[2] = EnvParams{0.001f, 0.10f, 0.0f, 0.08f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.60f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.70f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxChorus = 0.20f; p.fxReverb = 0.25f;
}
void keys_dx(SynthPatch& p)
{
    setName(p, "CRYSTAL EP"); p.category = 5; p.width = 0.62f; p.ampGain = 0.74f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 0; p.osc[0].wtMorph = 0.15f;
    osc(p, 1, SINE, 0.0f, 0.9f, 3); p.fm2to1 = 0.42f; p.ringMod = 0.12f;
    p.drift = 0.05f;
    filt(p, FilterMode::SvfLP, 0.76f, 0.04f, 0.10f, 0.0f, 0.55f);
    p.env[0] = EnvParams{0.002f, 1.60f, 0.18f, 0.60f, 0.35f};
    p.env[2] = EnvParams{0.001f, 0.15f, 0.0f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.55f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.65f);
    mod(p, 2, ModSource::Velocity, ModDest::RingMod, 0.25f);
    p.fxChorus = 0.30f; p.fxReverb = 0.38f; p.fxDelay = 0.10f;
}
void keys_clav(SynthPatch& p)
{
    setName(p, "FUNK CLAV"); p.category = 5; p.width = 0.46f; p.ampGain = 0.76f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.34f; osc(p, 1, SAW, 0.06f, 0.6f);
    p.drift = 0.08f;
    filt(p, FilterMode::LadderLP, 0.50f, 0.30f, 0.45f, 0.28f, 0.55f);
    p.fxDrive = 0.20f; p.driveMode = 1; p.driveTone = 0.20f;
    p.env[0] = EnvParams{0.002f, 0.22f, 0.28f, 0.12f, 0.30f};
    p.env[1] = EnvParams{0.002f, 0.16f, 0.0f, 0.10f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.45f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.45f);
    p.fxPhaser = 0.30f; p.phaserRate = 0.30f; p.fxDelay = 0.12f; p.fxReverb = 0.15f;
}
void keys_poly(SynthPatch& p)
{
    setName(p, "POLY KEYS"); p.category = 5; p.width = 0.72f; p.ampGain = 0.70f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.07f, 0.7f); p.osc[1].pw = 0.40f;
    osc(p, 3, SAW, -0.08f, 0.5f); p.subLevel = 0.10f; p.drift = 0.18f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.12f, 0.35f, 0.15f, 0.45f);
    p.env[0] = EnvParams{0.006f, 0.70f, 0.55f, 0.40f, 0.30f};
    p.env[1] = EnvParams{0.010f, 0.50f, 0.30f, 0.30f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 0.30f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.35f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pw2, 0.20f);
    p.fxChorus = 0.30f; p.fxReverb = 0.30f; p.fxDelay = 0.10f;
}
void keys_grand(SynthPatch& p)
{
    setName(p, "GRAND PIANO"); p.category = 5; p.width = 0.62f; p.ampGain = 0.74f; p.chorusMode = 0;
    osc(p, 0, TRI, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 14; p.osc[0].wtMorph = 0.10f; // string body
    osc(p, 1, SINE, 0.04f, 0.7f); osc(p, 3, SINE, -0.04f, 0.5f, 3); // octave shimmer
    p.drift = 0.07f;
    filt(p, FilterMode::SvfLP, 0.72f, 0.05f, 0.25f, 0.0f, 0.60f);
    p.env[0] = EnvParams{0.002f, 1.80f, 0.0f, 0.50f, 0.40f}; // struck-string decay
    p.env[1] = EnvParams{0.002f, 1.20f, 0.0f, 0.40f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.30f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.40f);
    mod(p, 2, ModSource::Velocity, ModDest::Amp, 0.40f);
    p.fxChorus = 0.12f; p.fxReverb = 0.34f; p.fxDelay = 0.05f;
}
void keys_bellkey(SynthPatch& p)
{
    setName(p, "BELL STACK"); p.category = 5; p.width = 0.70f; p.ampGain = 0.70f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 9; p.osc[0].wtMorph = 0.20f; // FORMANT A partial
    osc(p, 1, SINE, 0.01f, 0.8f, 4); p.fm2to1 = 0.50f; p.ringMod = 0.25f;
    p.drift = 0.05f;
    filt(p, FilterMode::SvfLP, 0.78f, 0.05f, 0.12f, 0.0f, 0.55f);
    p.env[0] = EnvParams{0.001f, 2.00f, 0.0f, 1.00f, 0.30f};
    p.env[2] = EnvParams{0.001f, 0.20f, 0.0f, 0.15f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.50f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.50f);
    mod(p, 2, ModSource::Velocity, ModDest::RingMod, 0.30f);
    p.fxChorus = 0.25f; p.fxDelay = 0.18f; p.delaySync = true; p.delayDiv = 9; p.fxReverb = 0.46f;
}
void keys_padkey(SynthPatch& p)
{
    setName(p, "PAD KEYS"); p.category = 5; p.width = 0.80f; p.ampGain = 0.68f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.08f, 0.8f); p.osc[1].pw = 0.45f;
    osc(p, 3, SAW, -0.10f, 0.5f); p.subLevel = 0.10f; p.drift = 0.20f;
    filt(p, FilterMode::SvfLP, 0.60f, 0.10f, 0.30f, 0.0f, 0.45f);
    p.env[0] = EnvParams{0.050f, 1.00f, 0.70f, 1.20f, 0.30f}; // key attack into pad sustain
    p.env[1] = EnvParams{0.080f, 0.90f, 0.40f, 1.00f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.20f); lfo(p, 1, LfoWave::Triangle, 0.15f);
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.30f);
    mod(p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.12f);
    mod(p, 2, ModSource::Lfo2, ModDest::Detune, 0.30f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.35f; p.fxReverb = 0.45f; p.fxDelay = 0.12f;
}
void keys_organ(SynthPatch& p)
{
    setName(p, "DRAW ORGAN"); p.category = 5; p.width = 0.60f; p.ampGain = 0.70f; p.chorusMode = 3;
    osc(p, 0, SINE, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 7; // drawbars
    osc(p, 1, SINE, 0.0f, 0.7f, 3); osc(p, 2, SINE, 0.0f, 0.5f, 4);
    p.drift = 0.04f;
    filt(p, FilterMode::SvfLP, 0.80f, 0.05f, 0.08f, 0.0f, 0.40f);
    p.fxDrive = 0.30f; p.driveMode = 1; p.driveTone = 0.10f; // tube/leslie grit
    p.env[0] = EnvParams{0.003f, 0.08f, 1.00f, 0.10f, 0.10f};
    lfo(p, 0, LfoWave::Sine, 6.5f); // leslie fast
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.008f);
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Rate, 0.60f); // leslie speed
    mod(p, 2, ModSource::Aftertouch, ModDest::FilterDrive, 0.30f);
    p.fxChorus = 0.34f; p.fxReverb = 0.22f;
}
void keys_toy(SynthPatch& p)
{
    setName(p, "TOY PIANO"); p.category = 5; p.width = 0.55f; p.ampGain = 0.72f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 1; p.osc[0].wtMorph = 0.20f; // triangle ping
    osc(p, 1, SINE, 0.02f, 0.7f, 4); p.fm2to1 = 0.40f; p.ringMod = 0.20f;
    p.drift = 0.08f;
    filt(p, FilterMode::SvfLP, 0.78f, 0.06f, 0.15f, 0.0f, 0.60f);
    p.env[0] = EnvParams{0.001f, 0.50f, 0.0f, 0.30f, 0.30f};
    p.env[2] = EnvParams{0.001f, 0.10f, 0.0f, 0.08f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.50f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.50f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    p.fxDelay = 0.12f; p.fxReverb = 0.40f;
}
void keys_fmclassic(SynthPatch& p)
{
    setName(p, "FM KEYS"); p.category = 5; p.width = 0.60f; p.ampGain = 0.74f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 1.0f, 3); p.fm2to1 = 0.50f;
    p.drift = 0.05f;
    filt(p, FilterMode::SvfLP, 0.78f, 0.04f, 0.12f, 0.0f, 0.55f);
    p.env[0] = EnvParams{0.002f, 1.30f, 0.25f, 0.55f, 0.35f};
    p.env[2] = EnvParams{0.001f, 0.13f, 0.05f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.60f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.70f);
    mod(p, 2, ModSource::ModWheel, ModDest::FmAmount, 0.40f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    p.fxChorus = 0.28f; p.fxReverb = 0.34f; p.fxDelay = 0.08f;
}
void keys_softkey(SynthPatch& p)
{
    setName(p, "SOFT KEYS"); p.category = 5; p.width = 0.58f; p.ampGain = 0.76f; p.chorusMode = 1;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SINE, 0.03f, 0.8f); p.subLevel = 0.08f;
    p.drift = 0.08f;
    filt(p, FilterMode::SvfLP, 0.62f, 0.06f, 0.22f, 0.0f, 0.45f);
    p.env[0] = EnvParams{0.008f, 0.90f, 0.45f, 0.60f, 0.30f};
    p.env[1] = EnvParams{0.010f, 0.70f, 0.25f, 0.50f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 4.0f); p.lfo[0].fade = 0.50f; p.lfo[0].delay = 0.40f; // delayed vibrato
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.28f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.30f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pitch, 0.008f);
    p.fxChorus = 0.25f; p.fxReverb = 0.36f; p.fxDelay = 0.08f;
}
// ================================ PLUCK (6) ================================
void pluck_synth(SynthPatch& p)
{
    setName(p, "SYNTH PLUCK"); p.category = 6; p.width = 0.62f; p.ampGain = 0.8f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.1f, 0.55f); p.osc[1].pw = 0.4f;
    p.subLevel = 0.14f; p.noiseLevel = 0.1f; p.drift = 0.16f;
    filt(p, FilterMode::LadderLP, 0.46f, 0.26f, 0.62f, 0.18f, 0.5f);
    p.env[0] = EnvParams{0.002f, 0.26f, 0.0f, 0.18f, 0.4f};
    p.env[1] = EnvParams{0.002f, 0.16f, 0.0f, 0.1f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.03f, 0.0f, 0.03f, 0.0f};   // fast noise transient
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.6f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.45f);
    mod(p, 2, ModSource::EnvAux, ModDest::NoiseLevel, -0.1f); // noise click ducks as aux falls
    mod(p, 3, ModSource::Velocity, ModDest::FilterDrive, 0.2f);
    p.fxDelay = 0.22f; p.fxReverb = 0.3f;
}
void pluck_koto(SynthPatch& p)
{
    setName(p, "KOTO"); p.category = 6; p.width = 0.5f; p.ampGain = 0.8f;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SAW, 0.0f, 0.38f); p.ringMod = 0.12f; p.drift = 0.1f;
    filt(p, FilterMode::SvfLP, 0.58f, 0.2f, 0.5f, 0.0f, 0.55f);
    p.env[0] = EnvParams{0.002f, 0.42f, 0.0f, 0.32f, 0.35f};
    p.env[1] = EnvParams{0.002f, 0.26f, 0.0f, 0.18f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.04f, 0.0f, 0.04f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.48f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.4f);
    mod(p, 2, ModSource::EnvAux, ModDest::Pitch, 0.12f);      // tiny pluck bend
    p.fxDelay = 0.2f; p.fxReverb = 0.34f;
}
void pluck_bell(SynthPatch& p)
{
    setName(p, "BELL PLUCK"); p.category = 6; p.width = 0.66f; p.ampGain = 0.72f;
    osc(p, 0, SINE, 0.0f, 1.0f);
    p.osc[0].oscType = 1; p.osc[0].wtTable = 8; p.osc[0].wtMorph = 0.3f;   // ODD -> inharmonic bell partials
    osc(p, 1, SINE, 0.0f, 0.85f, 4); p.fm2to1 = 0.45f; p.ringMod = 0.28f;
    filt(p, FilterMode::SvfLP, 0.78f, 0.05f, 0.2f);
    p.env[0] = EnvParams{0.001f, 0.55f, 0.0f, 0.45f, 0.3f};
    p.env[2] = EnvParams{0.001f, 0.14f, 0.0f, 0.12f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::FmAmount, 0.5f);
    mod(p, 2, ModSource::Velocity, ModDest::RingMod, 0.25f);
    p.fxDelay = 0.28f; p.fxReverb = 0.5f;
}
void pluck_mallet(SynthPatch& p)
{
    setName(p, "MALLET"); p.category = 6; p.width = 0.5f; p.ampGain = 0.78f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.45f, 4); p.noiseLevel = 0.08f; p.drift = 0.05f;
    filt(p, FilterMode::Lpg, 0.85f, 0.06f, 0.6f);   // vactrol low-pass gate
    p.env[0] = EnvParams{0.001f, 0.3f, 0.0f, 0.24f, 0.4f};
    p.env[1] = EnvParams{0.001f, 0.2f, 0.0f, 0.16f, 0.0f};   // fast filter env drives the LPG duck
    p.env[2] = EnvParams{0.001f, 0.04f, 0.0f, 0.04f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.14f);     // attack click
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.45f);
    mod(p, 2, ModSource::EnvAux, ModDest::NoiseLevel, -0.08f);
    p.fxReverb = 0.4f;
}
void pluck_harp(SynthPatch& p)
{
    setName(p, "HARP"); p.category = 6; p.width = 0.6f; p.ampGain = 0.76f; p.chorusMode = 1;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SINE, 0.04f, 0.65f); p.drift = 0.08f;
    filt(p, FilterMode::SvfLP, 0.66f, 0.1f, 0.42f, 0.0f, 0.5f);
    p.env[0] = EnvParams{0.002f, 0.55f, 0.0f, 0.45f, 0.4f};
    p.env[1] = EnvParams{0.002f, 0.3f, 0.0f, 0.2f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.4f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.4f);
    p.fxChorus = 0.2f; p.fxDelay = 0.24f; p.fxReverb = 0.46f;
}
void pluck_marimba(SynthPatch& p)
{
    setName(p, "MARIMBA"); p.category = 6; p.width = 0.5f; p.ampGain = 0.8f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.4f, 4); p.noiseLevel = 0.06f; p.drift = 0.04f;
    filt(p, FilterMode::SvfLP, 0.64f, 0.06f, 0.3f);
    p.env[0] = EnvParams{0.001f, 0.32f, 0.0f, 0.28f, 0.4f};
    p.env[2] = EnvParams{0.001f, 0.04f, 0.0f, 0.04f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.2f);      // woody attack pitch
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    mod(p, 2, ModSource::EnvAux, ModDest::NoiseLevel, -0.06f);
    p.fxReverb = 0.36f;
}
void pluck_guitar(SynthPatch& p)
{
    setName(p, "SYNTH GTR"); p.category = 6; p.width = 0.52f; p.ampGain = 0.8f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.03f, 0.5f); p.osc[1].pw = 0.42f; p.drift = 0.1f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.22f, 0.52f, 0.22f, 0.5f);
    p.env[0] = EnvParams{0.002f, 0.4f, 0.0f, 0.3f, 0.45f};
    p.env[1] = EnvParams{0.002f, 0.3f, 0.0f, 0.2f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.45f);
    mod(p, 2, ModSource::Velocity, ModDest::FilterDrive, 0.2f);
    p.fxReverb = 0.3f; p.fxDelay = 0.16f;
}
void pluck_glass(SynthPatch& p)
{
    setName(p, "GLASS PLUCK"); p.category = 6; p.width = 0.62f; p.ampGain = 0.72f;
    osc(p, 0, SINE, 0.0f, 1.0f);
    p.osc[0].oscType = 1; p.osc[0].wtTable = 6; p.osc[0].wtMorph = 0.4f;   // BRIGHT wavetable
    osc(p, 1, SINE, 0.01f, 0.8f, 3); p.fm2to1 = 0.3f; p.ringMod = 0.2f;
    filt(p, FilterMode::SvfLP, 0.8f, 0.05f, 0.2f);
    p.env[0] = EnvParams{0.001f, 0.42f, 0.0f, 0.32f, 0.3f};
    p.env[2] = EnvParams{0.001f, 0.1f, 0.0f, 0.08f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::FmAmount, 0.45f);
    mod(p, 1, ModSource::Velocity, ModDest::RingMod, 0.3f);
    p.fxDelay = 0.26f; p.fxReverb = 0.44f;
}
void pluck_pizz(SynthPatch& p)
{
    setName(p, "PIZZICATO"); p.category = 6; p.width = 0.46f; p.ampGain = 0.82f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, 0.05f, 0.4f); p.noiseLevel = 0.1f; p.drift = 0.1f;
    filt(p, FilterMode::SvfLP, 0.5f, 0.22f, 0.55f, 0.0f, 0.5f);
    p.env[0] = EnvParams{0.001f, 0.16f, 0.0f, 0.12f, 0.45f};
    p.env[1] = EnvParams{0.001f, 0.12f, 0.0f, 0.1f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.02f, 0.0f, 0.02f, 0.0f};
    mod(p, 0, ModSource::EnvFilter, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.4f);
    mod(p, 2, ModSource::EnvAux, ModDest::NoiseLevel, -0.1f);   // bow-snap transient
    p.fxReverb = 0.34f;
}
void pluck_musicbox(SynthPatch& p)
{
    setName(p, "MUSIC BOX"); p.category = 6; p.width = 0.6f; p.ampGain = 0.72f; p.chorusMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f);
    p.osc[0].oscType = 1; p.osc[0].wtTable = 8; p.osc[0].wtMorph = 0.2f;   // ODD -> tine sparkle
    osc(p, 1, SINE, 0.0f, 0.55f, 4); p.ringMod = 0.22f;
    filt(p, FilterMode::SvfLP, 0.82f, 0.05f, 0.2f);
    p.env[0] = EnvParams{0.001f, 0.6f, 0.0f, 0.5f, 0.4f};
    p.env[2] = EnvParams{0.001f, 0.06f, 0.0f, 0.06f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.14f);
    mod(p, 1, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.18f; p.fxDelay = 0.3f; p.fxReverb = 0.5f;
}

// ================================ FX (7) ================================
void fx_riser(SynthPatch& p)
{
    setName(p, "RISER"); p.category = 7; p.width = 0.92f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.22f, 1.0f); p.noiseLevel = 0.22f; p.drift = 0.4f;
    filt(p, FilterMode::SvfBP, 0.28f, 0.45f, 0.1f);
    p.env[0] = EnvParams{2.5f, 1.0f, 1.0f, 0.4f, 0.0f};
    p.env[2] = EnvParams{3.5f, 1.0f, 1.0f, 0.4f, 0.0f};       // slow rising aux env
    lfo(p, 0, LfoWave::Sine, 5.0f);
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.7f);
    mod(p, 2, ModSource::Lfo1, ModDest::Pitch, 0.012f);
    mod(p, 3, ModSource::EnvAux, ModDest::Lfo1Rate, 2.0f);    // accelerating shimmer
    mod(p, 4, ModSource::EnvAux, ModDest::Resonance, 0.3f);
    p.fxReverb = 0.55f; p.fxDelay = 0.3f;
}
void fx_noise(SynthPatch& p)
{
    setName(p, "NOISE SWEEP"); p.category = 7; p.width = 0.92f; p.ampGain = 0.55f;
    p.noiseLevel = 0.85f; p.osc[0].on = false; p.osc[1].on = false; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.3f, 0.5f, 0.0f);
    p.env[0] = EnvParams{0.6f, 1.0f, 0.85f, 1.0f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 0.14f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 1.2f);        // big slow sweep
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 1.0f);
    mod(p, 2, ModSource::ModWheel, ModDest::Resonance, 0.4f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.6f; p.fxDelay = 0.3f;
}
void fx_siren(SynthPatch& p)
{
    setName(p, "SIREN"); p.category = 7; p.width = 0.7f; p.ampGain = 0.68f; p.voiceMode = (int) VoiceMode::Mono;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.0f, 0.6f); p.drift = 0.1f;
    filt(p, FilterMode::SvfBP, 0.5f, 0.32f, 0.1f);
    p.env[0] = EnvParams{0.05f, 0.5f, 1.0f, 0.3f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 1.2f);
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.18f);        // wail (tamed)
    mod(p, 1, ModSource::ModWheel, ModDest::Lfo1Rate, 1.0f);
    mod(p, 2, ModSource::Lfo1, ModDest::Cutoff, 0.3f);
    mod(p, 3, ModSource::Aftertouch, ModDest::Lfo1Rate, 0.5f);
    p.fxDelay = 0.4f; p.fxReverb = 0.4f;
}
void fx_zap(SynthPatch& p)
{
    setName(p, "ZAP"); p.category = 7; p.width = 0.5f; p.ampGain = 0.78f;
    osc(p, 0, SAW, 0.0f, 1.0f); p.osc[0].crush = 0.2f; p.noiseLevel = 0.1f; p.drift = 0.0f;
    filt(p, FilterMode::SvfHP, 0.4f, 0.3f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.12f, 0.0f, 0.08f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.06f, 0.0f, 0.05f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 2.0f);       // fast downward zap
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.5f);
    mod(p, 2, ModSource::Velocity, ModDest::Pitch, 0.3f);
    p.fxDelay = 0.3f; p.fxReverb = 0.3f;
}
void fx_drop(SynthPatch& p)
{
    setName(p, "DOWNLIFTER"); p.category = 7; p.width = 0.92f; p.ampGain = 0.6f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.3f, 0.8f); p.noiseLevel = 0.18f; p.drift = 0.5f;
    filt(p, FilterMode::SvfLP, 0.6f, 0.22f, 0.0f);
    p.env[0] = EnvParams{0.01f, 3.5f, 0.0f, 0.5f, 0.0f};
    p.env[2] = EnvParams{0.01f, 3.5f, 0.0f, 0.5f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 6.0f);
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, -1.5f);      // falling
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.8f);
    mod(p, 2, ModSource::EnvAux, ModDest::Detune, 1.0f);
    mod(p, 3, ModSource::EnvAux, ModDest::Lfo1Rate, 1.5f);
    p.fxReverb = 0.55f; p.fxDelay = 0.35f;
}
void fx_impact(SynthPatch& p)
{
    setName(p, "IMPACT"); p.category = 7; p.width = 0.7f; p.ampGain = 0.78f; p.fxDrive = 0.3f; p.driveMode = 1;
    osc(p, 0, SINE, 0.0f, 1.0f); p.noiseLevel = 0.45f; p.drift = 0.0f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.12f, 0.0f, 0.2f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.65f, 0.0f, 0.4f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.1f, 0.0f, 0.08f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 2.0f);       // boom pitch drop
    mod(p, 1, ModSource::EnvAux, ModDest::NoiseLevel, -0.4f); // noise crack ducks out
    mod(p, 2, ModSource::EnvAux, ModDest::Cutoff, 0.5f);
    p.fxReverb = 0.6f; p.fxDelay = 0.2f;
}
void fx_wind(SynthPatch& p)
{
    setName(p, "WIND"); p.category = 7; p.width = 0.95f; p.ampGain = 0.5f;
    p.osc[0].on = false; p.osc[1].on = false; p.noiseLevel = 0.9f; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.4f, 0.35f, 0.0f);
    p.env[0] = EnvParams{1.2f, 1.0f, 0.9f, 1.6f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.1f); lfo(p, 1, LfoWave::Triangle, 0.07f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.3f);
    mod(p, 2, ModSource::Lfo1, ModDest::Resonance, 0.2f);     // gusting whistle
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.5f);
    p.fxReverb = 0.7f;
}
void fx_alien(SynthPatch& p)
{
    setName(p, "ALIEN"); p.category = 7; p.width = 0.8f; p.ampGain = 0.6f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.0f, 1.0f); p.ringMod = 0.5f; p.fm2to1 = 0.3f; p.drift = 0.3f;
    p.osc[0].srReduce = 0.3f;
    filt(p, FilterMode::SvfBP, 0.5f, 0.42f, 0.1f);
    p.env[0] = EnvParams{0.1f, 0.5f, 0.8f, 0.5f, 0.0f};
    lfo(p, 0, LfoWave::SampleHold, 4.0f); lfo(p, 1, LfoWave::Sine, 0.3f);
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.2f);
    mod(p, 1, ModSource::Lfo1, ModDest::RingMod, 0.3f);
    mod(p, 2, ModSource::Lfo2, ModDest::Cutoff, 0.3f);
    mod(p, 3, ModSource::Lfo1, ModDest::Osc2Pitch, 0.25f);
    p.fxDelay = 0.4f; p.fxReverb = 0.5f;
}

// ================================ DRONE (8) ================================
void drone_dark(SynthPatch& p)
{
    setName(p, "DARK DRONE"); p.category = 8; p.width = 0.86f; p.ampGain = 0.68f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.04f, 0.9f, 1); osc(p, 3, TRI, -0.05f, 0.6f, 1);
    p.subLevel = 0.4f; p.subOctave = 2; p.drift = 0.4f;
    filt(p, FilterMode::LadderLP, 0.24f, 0.22f, 0.2f, 0.3f, 0.2f);
    p.env[0] = EnvParams{2.5f, 2.0f, 1.0f, 3.5f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.05f, 1.0f, 0.1f); lfo(p, 1, LfoWave::Triangle, 0.03f); lfo(p, 2, LfoWave::Sine, 0.02f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.15f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.4f);
    mod(p, 2, ModSource::Lfo3, ModDest::Morph1, 0.2f);       // slow timbre crawl
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.4f);
    p.fxReverb = 0.62f;
}
void drone_sub(SynthPatch& p)
{
    setName(p, "SUB DRONE"); p.category = 8; p.width = 0.6f; p.ampGain = 0.78f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.03f, 0.5f); p.subLevel = 0.6f; p.subOctave = 2; p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.3f, 0.12f, 0.15f, 0.2f, 0.1f);
    p.env[0] = EnvParams{1.8f, 2.0f, 1.0f, 3.0f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.04f); lfo(p, 1, LfoWave::Triangle, 0.06f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.1f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.25f);
    mod(p, 2, ModSource::ModWheel, ModDest::SubLevel, 0.3f);
    p.fxReverb = 0.5f;
}
void drone_harm(SynthPatch& p)
{
    setName(p, "HARMONIC"); p.category = 8; p.width = 0.86f; p.ampGain = 0.64f; p.chorusMode = 2;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.7f, 3); osc(p, 2, SINE, 0.0f, 0.5f, 4);
    osc(p, 3, SINE, 0.02f, 0.4f, 1); p.drift = 0.15f;
    filt(p, FilterMode::SvfLP, 0.6f, 0.06f, 0.1f);
    p.env[0] = EnvParams{3.0f, 2.0f, 1.0f, 3.5f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.07f); lfo(p, 1, LfoWave::Triangle, 0.05f); lfo(p, 2, LfoWave::Sine, 0.03f);
    mod(p, 0, ModSource::Lfo1, ModDest::OscMix, 0.4f);       // partials breathe in/out
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::Lfo3, ModDest::Detune, 0.2f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.72f; p.fxDelay = 0.2f;
}
void drone_metal(SynthPatch& p)
{
    setName(p, "METAL DRONE"); p.category = 8; p.width = 0.8f; p.ampGain = 0.58f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.0f, 1.0f); p.ringMod = 0.4f; p.fm2to1 = 0.2f; p.drift = 0.3f;
    filt(p, FilterMode::SvfBP, 0.5f, 0.42f, 0.1f);
    p.env[0] = EnvParams{1.8f, 2.0f, 1.0f, 3.0f, 0.0f};
    lfo(p, 0, LfoWave::SampleHold, 0.2f); lfo(p, 1, LfoWave::Sine, 0.08f); lfo(p, 2, LfoWave::Triangle, 0.04f);
    mod(p, 0, ModSource::Lfo1, ModDest::RingMod, 0.4f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.3f);
    mod(p, 2, ModSource::Lfo1, ModDest::Osc2Pitch, 0.2f);
    mod(p, 3, ModSource::Lfo3, ModDest::FmAmount, 0.3f);     // clangorous FM swell
    p.fxReverb = 0.6f; p.fxDelay = 0.3f;
}
void drone_evolve(SynthPatch& p)
{
    setName(p, "EVOLVING"); p.category = 8; p.width = 0.92f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f);
    p.osc[0].oscType = 1; p.osc[0].wtTable = 14; p.osc[0].wtMorph = 0.3f;  // STRING wavetable
    osc(p, 1, SAW, 0.06f, 0.9f);
    p.osc[1].oscType = 1; p.osc[1].wtTable = 9; p.osc[1].wtMorph = 0.4f;   // FORMANT A wavetable
    osc(p, 3, SAW, -0.08f, 0.6f); p.subLevel = 0.2f; p.drift = 0.45f;
    filt(p, FilterMode::LadderLP, 0.4f, 0.2f, 0.2f, 0.2f, 0.3f);
    p.env[0] = EnvParams{2.5f, 2.0f, 1.0f, 3.5f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.05f); lfo(p, 1, LfoWave::Triangle, 0.03f); lfo(p, 2, LfoWave::SampleHold, 0.08f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.2f);
    mod(p, 1, ModSource::Lfo2, ModDest::Morph1, 0.5f);       // wavetable scan osc1
    mod(p, 2, ModSource::Lfo3, ModDest::Morph2, 0.4f);       // wavetable scan osc2
    mod(p, 3, ModSource::Lfo1, ModDest::Detune, 0.3f);
    mod(p, 4, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.66f; p.fxDelay = 0.3f;
}
void drone_choir(SynthPatch& p)
{
    setName(p, "DRONE CHOIR"); p.category = 8; p.width = 0.9f; p.ampGain = 0.5f; p.chorusMode = 2;
    p.voiceMode = (int) VoiceMode::Unison; p.unisonCount = 6; p.unisonDetune = 0.16f; p.unisonSpread = 0.85f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f;
    p.osc[0].oscType = 1; p.osc[0].wtTable = 15; p.osc[0].wtMorph = 0.3f;  // VOCAL wavetable
    osc(p, 1, PULSE, 0.05f, 1.0f); p.osc[1].pw = 0.46f; p.subLevel = 0.1f; p.drift = 0.35f;
    filt(p, FilterMode::SvfBP, 0.5f, 0.32f, 0.1f);
    p.env[0] = EnvParams{2.0f, 2.0f, 0.92f, 3.5f, 0.15f};
    lfo(p, 0, LfoWave::Sine, 0.06f); lfo(p, 1, LfoWave::Triangle, 0.1f); lfo(p, 2, LfoWave::Sine, 0.04f);
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.3f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.18f);
    mod(p, 2, ModSource::Lfo3, ModDest::Morph1, 0.3f);       // vowel morph
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.78f;
}
void drone_warm(SynthPatch& p)
{
    setName(p, "WARM DRONE"); p.category = 8; p.width = 0.86f; p.ampGain = 0.7f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.06f, 0.9f); osc(p, 3, TRI, -0.05f, 0.5f, 1);
    p.subLevel = 0.3f; p.drift = 0.35f;
    filt(p, FilterMode::LadderLP, 0.32f, 0.16f, 0.2f, 0.3f, 0.2f);
    p.env[0] = EnvParams{2.5f, 2.0f, 1.0f, 3.5f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.05f); lfo(p, 1, LfoWave::Triangle, 0.07f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.15f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.25f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.4f);
    p.fxReverb = 0.6f;
}
void drone_noise(SynthPatch& p)
{
    setName(p, "NOISE DRONE"); p.category = 8; p.width = 0.9f; p.ampGain = 0.58f;
    osc(p, 0, SAW, 0.0f, 0.6f); p.noiseLevel = 0.5f; p.drift = 0.2f;
    filt(p, FilterMode::SvfBP, 0.4f, 0.32f, 0.1f);
    p.env[0] = EnvParams{1.8f, 2.0f, 1.0f, 3.0f, 0.0f};
    lfo(p, 0, LfoWave::Triangle, 0.08f); lfo(p, 1, LfoWave::SampleHold, 0.15f); lfo(p, 2, LfoWave::Sine, 0.04f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.4f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.2f);
    mod(p, 2, ModSource::Lfo3, ModDest::Detune, 0.2f);
    mod(p, 3, ModSource::ModWheel, ModDest::Resonance, 0.3f);
    p.fxReverb = 0.7f;
}
// ================================ PERC (9) ================================
void perc_kick(SynthPatch& p)
{
    setName(p, "SUB KICK"); p.category = 9; p.width = 0.28f; p.ampGain = 0.9f;
    osc(p, 0, SINE, 0.0f, 1.0f); p.drift = 0.0f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.0f, 0.0f, 0.25f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.38f, 0.0f, 0.12f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.055f, 0.0f, 0.05f, 0.0f}; // fast pitch drop
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 2.0f);      // punchy click->thud
    mod(p, 1, ModSource::Velocity, ModDest::Amp, 0.3f);
    p.fxDrive = 0.22f; p.driveMode = 1; p.driveTone = -0.2f; p.fxReverb = 0.0f;
}
void perc_snare(SynthPatch& p)
{
    setName(p, "SNAP SNARE"); p.category = 9; p.width = 0.45f; p.ampGain = 0.78f;
    osc(p, 0, TRI, 0.0f, 0.7f); p.noiseLevel = 0.75f; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.55f, 0.22f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.2f, 0.0f, 0.12f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.05f, 0.0f, 0.03f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.2f);
    mod(p, 1, ModSource::EnvAmp, ModDest::NoiseLevel, 0.3f);
    mod(p, 2, ModSource::Velocity, ModDest::Amp, 0.3f);
    p.fxReverb = 0.2f;
}
void perc_tom(SynthPatch& p)
{
    setName(p, "FLOOR TOM"); p.category = 9; p.width = 0.4f; p.ampGain = 0.84f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.3f); p.noiseLevel = 0.04f; p.drift = 0.0f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.1f, 0.0f, 0.15f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.45f, 0.0f, 0.16f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.14f, 0.0f, 0.08f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.5f);
    mod(p, 1, ModSource::Velocity, ModDest::Amp, 0.3f);
    p.fxReverb = 0.15f;
}
void perc_clap(SynthPatch& p)
{
    setName(p, "HAND CLAP"); p.category = 9; p.width = 0.5f; p.ampGain = 0.72f;
    p.osc[0].on = false; p.noiseLevel = 0.9f; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.6f, 0.4f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.14f, 0.05f, 0.14f, 0.0f};
    lfo(p, 0, LfoWave::Square, 55.0f); // fast gate -> clap stutter
    mod(p, 0, ModSource::Lfo1, ModDest::Amp, 0.6f);
    mod(p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.4f);
    p.fxReverb = 0.3f;
}
void perc_cowbell(SynthPatch& p)
{
    setName(p, "COWBELL"); p.category = 9; p.width = 0.4f; p.ampGain = 0.7f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.4f;
    osc(p, 1, PULSE, 7.0f, 0.8f); p.ringMod = 0.3f; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.72f, 0.2f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.28f, 0.0f, 0.16f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Amp, 0.3f);
    p.fxReverb = 0.18f;
}
void perc_808(SynthPatch& p)
{
    setName(p, "TRAP BOOM"); p.category = 9; p.width = 0.2f; p.ampGain = 0.9f;
    osc(p, 0, SINE, 0.0f, 1.0f); p.subLevel = 0.3f; p.subOctave = 1; p.drift = 0.0f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.0f, 0.0f, 0.2f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.9f, 0.0f, 0.32f, 0.0f}; // long booming tail
    p.env[2] = EnvParams{0.001f, 0.1f, 0.0f, 0.07f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.8f);
    mod(p, 1, ModSource::Velocity, ModDest::Amp, 0.3f);
    p.fxDrive = 0.2f; p.driveMode = 1; p.fxReverb = 0.0f;
}
void perc_hat(SynthPatch& p)
{
    setName(p, "CLOSED HAT"); p.category = 9; p.width = 0.3f; p.ampGain = 0.6f;
    osc(p, 0, PULSE, 0.0f, 0.4f, 4); p.osc[0].pw = 0.3f;
    osc(p, 1, PULSE, 7.3f, 0.4f, 4); p.ringMod = 0.5f; p.noiseLevel = 0.7f; p.drift = 0.0f;
    filt(p, FilterMode::SvfHP, 0.82f, 0.2f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.045f, 0.0f, 0.04f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Amp, 0.4f);
    p.fxReverb = 0.08f;
}
void perc_openhat(SynthPatch& p)
{
    setName(p, "OPEN HAT"); p.category = 9; p.width = 0.35f; p.ampGain = 0.58f;
    osc(p, 0, PULSE, 0.0f, 0.4f, 4); p.osc[0].pw = 0.3f;
    osc(p, 1, PULSE, 7.3f, 0.4f, 4); p.ringMod = 0.5f; p.noiseLevel = 0.7f; p.drift = 0.0f;
    filt(p, FilterMode::SvfHP, 0.82f, 0.18f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.32f, 0.0f, 0.22f, 0.0f};
    mod(p, 0, ModSource::Velocity, ModDest::Amp, 0.4f);
    p.fxReverb = 0.12f;
}
void perc_rim(SynthPatch& p)
{
    setName(p, "RIMSHOT"); p.category = 9; p.width = 0.3f; p.ampGain = 0.7f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.4f; p.noiseLevel = 0.3f; p.ringMod = 0.3f; p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.7f, 0.3f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.06f, 0.0f, 0.05f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.02f, 0.0f, 0.02f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.8f);
    p.fxReverb = 0.1f;
}
void perc_clave(SynthPatch& p)
{
    setName(p, "CLAVE"); p.category = 9; p.width = 0.3f; p.ampGain = 0.72f;
    osc(p, 0, SINE, 0.0f, 1.0f); p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.75f, 0.15f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.07f, 0.0f, 0.05f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.02f, 0.0f, 0.02f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 0.6f);
    p.fxReverb = 0.12f;
}
void perc_conga(SynthPatch& p)
{
    setName(p, "CONGA"); p.category = 9; p.width = 0.4f; p.ampGain = 0.82f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.3f); p.noiseLevel = 0.03f; p.drift = 0.0f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.1f, 0.0f, 0.1f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.3f, 0.0f, 0.12f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.09f, 0.0f, 0.06f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.0f);
    p.fxReverb = 0.12f;
}
void perc_zap(SynthPatch& p)
{
    setName(p, "LASER ZAP"); p.category = 9; p.width = 0.4f; p.ampGain = 0.76f;
    osc(p, 0, SAW, 0.0f, 1.0f); p.drift = 0.0f;
    filt(p, FilterMode::SvfBP, 0.6f, 0.3f, 0.0f);
    p.env[0] = EnvParams{0.001f, 0.12f, 0.0f, 0.07f, 0.0f};
    p.env[2] = EnvParams{0.001f, 0.06f, 0.0f, 0.04f, 0.0f};
    mod(p, 0, ModSource::EnvAux, ModDest::Pitch, 1.8f);
    mod(p, 1, ModSource::EnvAux, ModDest::Cutoff, 0.5f);
    p.fxDelay = 0.2f; p.fxReverb = 0.2f;
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
    mod(p, 2, ModSource::Lfo1, ModDest::Morph1, 0.15f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.4f);
    p.fxReverb = 0.8f; p.fxDelay = 0.3f;
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
    p.fxReverb = 0.85f; p.fxDelay = 0.4f;
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
    p.fxReverb = 0.85f; p.fxDelay = 0.35f;
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
    p.fxReverb = 0.8f; p.fxDelay = 0.3f;
}
void amb_drift(SynthPatch& p)
{
    setName(p, "DRIFT"); p.category = 10; p.width = 0.92f; p.ampGain = 0.58f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, 0.1f, 0.8f); osc(p, 3, SINE, -0.08f, 0.4f, 3);
    p.drift = 0.5f;
    filt(p, FilterMode::SvfLP, 0.45f, 0.15f, 0.15f);
    p.env[0] = EnvParams{2.4f, 2.0f, 0.9f, 3.6f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 0.04f); lfo(p, 1, LfoWave::Triangle, 0.06f);
    mod(p, 0, ModSource::Lfo1, ModDest::Detune, 0.5f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.2f);
    mod(p, 2, ModSource::Lfo1, ModDest::Morph1, 0.12f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.82f; p.fxDelay = 0.35f;
}
void amb_underwater(SynthPatch& p)
{
    setName(p, "UNDERWATER"); p.category = 10; p.width = 0.9f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.06f, 0.7f); p.subLevel = 0.15f; p.drift = 0.4f;
    filt(p, FilterMode::SvfLP, 0.32f, 0.3f, 0.12f);
    p.env[0] = EnvParams{1.6f, 2.0f, 0.9f, 3.0f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 0.12f); lfo(p, 1, LfoWave::Sine, 0.18f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.35f);
    mod(p, 1, ModSource::Lfo2, ModDest::Pitch, 0.006f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.82f; p.fxDelay = 0.4f;
}
void amb_meadow(SynthPatch& p)
{
    setName(p, "MEADOW"); p.category = 10; p.width = 0.92f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SAW, 0.1f, 0.7f); osc(p, 3, SINE, 0.05f, 0.4f, 3);
    p.noiseLevel = 0.05f; p.drift = 0.4f;
    filt(p, FilterMode::SvfLP, 0.55f, 0.12f, 0.18f);
    p.env[0] = EnvParams{1.8f, 2.0f, 0.9f, 3.2f, 0.15f};
    lfo(p, 0, LfoWave::Sine, 0.07f, 1.0f, 0.1f); lfo(p, 1, LfoWave::Triangle, 0.11f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.2f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.4f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.78f; p.fxDelay = 0.3f;
}
void amb_dust(SynthPatch& p)
{
    setName(p, "STARDUST"); p.category = 10; p.width = 0.95f; p.ampGain = 0.56f; p.chorusMode = 2;
    osc(p, 0, SINE, 0.0f, 1.0f, 3); osc(p, 1, SINE, 0.03f, 0.6f, 4);
    p.ringMod = 0.15f; p.noiseLevel = 0.04f; p.drift = 0.35f;
    filt(p, FilterMode::SvfHP, 0.4f, 0.1f, 0.1f);
    p.env[0] = EnvParams{1.4f, 2.0f, 0.85f, 3.4f, 0.1f};
    lfo(p, 0, LfoWave::SampleHold, 0.3f); lfo(p, 1, LfoWave::Sine, 0.05f);
    mod(p, 0, ModSource::Lfo1, ModDest::RingMod, 0.2f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.2f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.86f; p.fxDelay = 0.45f;
}
void amb_breath(SynthPatch& p)
{
    setName(p, "BREATH"); p.category = 10; p.width = 0.9f; p.ampGain = 0.56f; p.chorusMode = 2;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f; osc(p, 1, PULSE, 0.07f, 0.8f);
    p.noiseLevel = 0.1f; p.drift = 0.3f;
    filt(p, FilterMode::SvfBP, 0.5f, 0.35f, 0.12f);
    p.env[0] = EnvParams{1.6f, 1.8f, 0.9f, 3.0f, 0.15f};
    lfo(p, 0, LfoWave::Sine, 0.1f); lfo(p, 1, LfoWave::Triangle, 0.15f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.25f);
    mod(p, 1, ModSource::Lfo2, ModDest::Pw1, 0.3f);
    mod(p, 2, ModSource::ModWheel, ModDest::Resonance, 0.2f);
    p.fxReverb = 0.8f;
}
void amb_crystal(SynthPatch& p)
{
    setName(p, "CRYSTAL"); p.category = 10; p.width = 0.9f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.01f, 0.8f, 4);
    p.osc[1].oscType = 1; p.osc[1].wtTable = 8; p.osc[1].wtMorph = 0.3f; // glassy WT layer
    p.fm2to1 = 0.2f; p.ringMod = 0.2f; p.drift = 0.2f;
    filt(p, FilterMode::SvfLP, 0.7f, 0.06f, 0.15f);
    p.env[0] = EnvParams{1.2f, 2.0f, 0.85f, 3.0f, 0.1f};
    p.env[2] = EnvParams{0.5f, 1.0f, 0.3f, 0.8f, 0.0f};
    lfo(p, 0, LfoWave::Sine, 0.08f);
    mod(p, 0, ModSource::Lfo1, ModDest::FmAmount, 0.2f);
    mod(p, 1, ModSource::EnvAux, ModDest::FmAmount, 0.3f);
    mod(p, 2, ModSource::ModWheel, ModDest::RingMod, 0.3f);
    p.fxReverb = 0.86f; p.fxDelay = 0.4f;
}
void amb_nebula(SynthPatch& p)
{
    setName(p, "NEBULA"); p.category = 10; p.width = 0.95f; p.ampGain = 0.56f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.16f, 0.8f); osc(p, 2, PULSE, -0.14f, 0.6f);
    osc(p, 3, SINE, 0.08f, 0.4f, 1); p.drift = 0.5f;
    filt(p, FilterMode::SvfLP, 0.42f, 0.2f, 0.15f);
    p.env[0] = EnvParams{2.6f, 2.2f, 0.88f, 4.0f, 0.1f};
    lfo(p, 0, LfoWave::Sine, 0.03f); lfo(p, 1, LfoWave::Triangle, 0.05f); lfo(p, 2, LfoWave::SampleHold, 0.08f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.2f);
    mod(p, 1, ModSource::Lfo2, ModDest::Detune, 0.5f);
    mod(p, 2, ModSource::Lfo3, ModDest::Pw2, 0.3f);
    mod(p, 3, ModSource::Lfo1, ModDest::Morph1, 0.15f);
    mod(p, 4, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxReverb = 0.88f; p.fxDelay = 0.4f;
}
void amb_tape(SynthPatch& p)
{
    setName(p, "TAPE PAD"); p.category = 10; p.width = 0.85f; p.ampGain = 0.62f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, 0.08f, 0.8f);
    p.osc[0].crush = 0.12f; p.osc[0].srReduce = 0.15f; // gentle tape grain
    p.subLevel = 0.12f; p.drift = 0.45f;               // heavy drift = tape wow
    filt(p, FilterMode::SvfLP, 0.5f, 0.12f, 0.18f);
    p.env[0] = EnvParams{1.4f, 1.8f, 0.88f, 3.0f, 0.15f};
    lfo(p, 0, LfoWave::Sine, 0.6f, 0.4f); lfo(p, 1, LfoWave::Triangle, 0.07f);
    mod(p, 0, ModSource::Lfo1, ModDest::Pitch, 0.006f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.2f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.3f; p.fxReverb = 0.74f; p.fxDelay = 0.3f;
}
void amb_warmth(SynthPatch& p)
{
    setName(p, "WARMTH"); p.category = 10; p.width = 0.85f; p.ampGain = 0.66f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.07f, 0.9f); osc(p, 3, TRI, -0.05f, 0.5f, 1);
    p.subLevel = 0.2f; p.drift = 0.35f;
    filt(p, FilterMode::LadderLP, 0.4f, 0.12f, 0.2f, 0.25f, 0.3f);
    p.env[0] = EnvParams{1.6f, 1.8f, 0.9f, 3.0f, 0.15f};
    lfo(p, 0, LfoWave::Sine, 0.06f, 1.0f, 0.1f);
    mod(p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.15f);
    mod(p, 1, ModSource::Lfo1, ModDest::Detune, 0.2f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.3f; p.fxReverb = 0.72f; p.fxDelay = 0.25f;
}

// ================================ SEQ (11) ================================
void seq_trance(SynthPatch& p)
{
    setName(p, "TRANCE GATE"); p.category = 11; p.width = 0.8f; p.ampGain = 0.62f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 1.0f); osc(p, 3, SAW, -0.1f, 0.6f);
    p.subLevel = 0.12f; p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.15f, 0.2f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.4f, 1.0f, 0.9f, 0.6f, 0.0f};
    // Seq1: 16-step 1/16 amp gate
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {1,1,-1,1, -1,1,1,-1, 1,-1,1,1, -1,1,-1,1};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    lfo(p, 1, LfoWave::Sine, 0.2f);
    mod(p, 0, ModSource::Seq1, ModDest::Amp, 0.9f);   // the gate
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.2f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.3f; p.fxDelay = 0.3f; p.fxReverb = 0.4f;
}
void seq_acid(SynthPatch& p)
{
    setName(p, "ACID LINE"); p.category = 11; p.width = 0.4f; p.ampGain = 0.74f;
    p.voiceMode = (int) VoiceMode::Mono; p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.05f;
    osc(p, 0, SAW, 0.0f, 1.0f); p.drift = 0.1f;
    filt(p, FilterMode::LadderLP, 0.3f, 0.45f, 0.5f, 0.4f, 0.3f);
    p.env[0] = EnvParams{0.004f, 0.22f, 0.25f, 0.12f, 0.1f};
    p.env[1] = EnvParams{0.004f, 0.2f, 0.05f, 0.12f, 0.0f};
    // Seq1: cutoff accent line (the 303 squelch)
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0.2f,1.0f,0.2f,0.6f, 0.2f,1.0f,0.4f,0.2f, 0.6f,1.0f,0.2f,0.4f, 0.2f,0.8f,0.2f,1.0f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: melodic note line, quantised to semitones
    p.seq[1].sync = true; p.seq[1].syncDiv = 4; p.seq[1].length = 16;
    p.seq[1].mode = (int) SeqMode::Melodic; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[16] = {0,1.0f,0,0.25f, 0,0.5833f,0.8333f,0, 0,1.0f,0.25f,0, 0.5833f,0,0.8333f,1.0f};
      for(int i = 0; i < 16; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Seq2, ModDest::Pitch, 1.0f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.4f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxDrive = 0.3f; p.driveMode = 2; p.fxDelay = 0.3f; p.fxReverb = 0.2f;
}
void seq_wobble(SynthPatch& p)
{
    setName(p, "WOBBLE SEQ"); p.category = 11; p.width = 0.6f; p.ampGain = 0.66f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.15f, 1.0f); osc(p, 2, PULSE, -0.12f, 0.8f);
    p.subLevel = 0.3f; p.fxDrive = 0.25f; p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.35f, 0.4f, 0.3f, 0.4f, 0.3f);
    p.env[0] = EnvParams{0.01f, 0.5f, 0.9f, 0.2f, 0.0f};
    // Seq1: smooth 1/8 cutoff wobble
    p.seq[0].sync = true; p.seq[0].syncDiv = 7; p.seq[0].length = 8;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Smooth;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = false;
    { const float s[8] = {-1.0f,1.0f,-0.5f,1.0f, -1.0f,0.5f,-0.8f,1.0f};
      for(int i = 0; i < 8; ++i) p.seq[0].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Cutoff, 0.6f);   // the wob
    mod(p, 1, ModSource::ModWheel, ModDest::FilterDrive, 0.4f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.2f);
    p.fxReverb = 0.2f;
}
void seq_blade(SynthPatch& p)
{
    setName(p, "BLADE SEQ"); p.category = 11; p.width = 0.9f; p.ampGain = 0.56f; p.chorusMode = 2;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f; osc(p, 1, SAW, 0.12f, 1.0f);
    osc(p, 2, SAW, -0.1f, 0.8f); osc(p, 3, SAW, 0.22f, 0.6f); osc(p, 4, SAW, -0.05f, 0.5f, 1);
    p.subLevel = 0.1f; p.drift = 0.3f;
    filt(p, FilterMode::SvfLP, 0.55f, 0.24f, 0.2f, 0.0f, 0.4f);
    p.env[0] = EnvParams{0.012f, 0.6f, 0.88f, 0.4f, 0.1f};
    // Seq1: melodic riff
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0.5833f,1.0f,0.5833f, 0.25f,0.75f,1.0f,0.4167f, 0,0.5833f,1.0f,0.5833f, 0.25f,0.75f,1.0f,0.4167f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    lfoSync(p, 0, LfoWave::Triangle, 9); lfoSync(p, 1, LfoWave::Sine, 11);
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Lfo1, ModDest::Pw1, 0.45f);
    mod(p, 2, ModSource::Lfo2, ModDest::Cutoff, 0.35f);
    mod(p, 3, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.4f; p.fxPhaser = 0.2f; p.fxReverb = 0.34f; p.fxDelay = 0.2f;
}
void seq_hoover(SynthPatch& p)
{
    setName(p, "HOOVER SEQ"); p.category = 11; p.width = 0.78f; p.ampGain = 0.6f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.18f, 0.9f); p.osc[1].pw = 0.4f;
    osc(p, 2, SAW, -0.16f, 0.85f); osc(p, 3, SAW, 0.3f, 0.7f); osc(p, 4, PULSE, -0.28f, 0.6f);
    p.subLevel = 0.14f; p.drift = 0.3f;
    filt(p, FilterMode::LadderLP, 0.4f, 0.34f, 0.35f, 0.4f, 0.4f);
    p.env[0] = EnvParams{0.01f, 0.4f, 0.85f, 0.3f, 0.1f};
    // Seq1: melodic 1/8 riff
    p.seq[0].sync = true; p.seq[0].syncDiv = 7; p.seq[0].length = 8;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[8] = {0,1.0f,0.5833f,0, 0.25f,0.8333f,0.5833f,1.0f};
      for(int i = 0; i < 8; ++i) p.seq[0].step[i] = s[i]; }
    lfoSync(p, 1, LfoWave::Triangle, 12);
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.3f);
    mod(p, 2, ModSource::ModWheel, ModDest::Osc2Pitch, 0.4f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    p.fxChorus = 0.3f; p.fxDelay = 0.28f; p.fxReverb = 0.3f;
}
void seq_pulse(SynthPatch& p)
{
    setName(p, "PULSE SEQ"); p.category = 11; p.width = 0.6f; p.ampGain = 0.72f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f; osc(p, 1, PULSE, 0.06f, 0.7f);
    p.subLevel = 0.12f; p.drift = 0.15f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.2f, 0.4f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.2f, 0.3f, 0.16f, 0.1f};
    p.env[1] = EnvParams{0.004f, 0.18f, 0.1f, 0.14f, 0.0f};
    // Seq1: smooth PWM movement
    p.seq[0].sync = true; p.seq[0].syncDiv = 7; p.seq[0].length = 8;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Smooth;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = false;
    { const float s[8] = {-1.0f,0.5f,-0.3f,1.0f, -0.6f,0.4f,-1.0f,0.7f};
      for(int i = 0; i < 8; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: 1/16 cutoff accents
    p.seq[1].sync = true; p.seq[1].syncDiv = 4; p.seq[1].length = 16;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[16] = {0.2f,0.8f,0.3f,1.0f, 0.2f,0.6f,0.4f,1.0f, 0.3f,0.8f,0.2f,0.7f, 0.4f,1.0f,0.2f,0.9f};
      for(int i = 0; i < 16; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pw1, 0.4f);
    mod(p, 1, ModSource::Seq2, ModDest::Cutoff, 0.4f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.3f);
    p.fxDelay = 0.3f; p.fxReverb = 0.3f;
}
void seq_pluckseq(SynthPatch& p)
{
    setName(p, "PLUCK SEQ"); p.category = 11; p.width = 0.6f; p.ampGain = 0.8f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.08f, 0.5f); p.drift = 0.15f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.25f, 0.6f, 0.2f, 0.5f);
    p.env[0] = EnvParams{0.002f, 0.16f, 0.0f, 0.12f, 0.15f};
    p.env[1] = EnvParams{0.002f, 0.14f, 0.0f, 0.1f, 0.0f};
    // Seq1: melodic 1/16 line, retriggers per note
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0.5833f,1.0f,0.5833f, 0.25f,0.5833f,1.0f,0.75f, 0,0.4167f,0.8333f,0.5833f, 0.25f,0.5833f,0.8333f,1.0f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.4f);
    p.fxDelay = 0.32f; p.fxReverb = 0.3f;
}
void seq_chordseq(SynthPatch& p)
{
    setName(p, "CHORD SEQ"); p.category = 11; p.width = 0.75f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 0.9f); osc(p, 3, SAW, -0.1f, 0.6f);
    p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.18f, 0.4f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.4f, 0.0f, 0.3f, 0.15f};
    p.env[1] = EnvParams{0.004f, 0.3f, 0.0f, 0.22f, 0.0f};
    // Seq1: rhythmic chord gate
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {1,-1,1,1, -1,1,-1,1, 1,1,-1,1, -1,1,1,-1};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: smooth filter sweep
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Smooth;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = false;
    { const float s[8] = {0.2f,0.6f,0.4f,1.0f, 0.3f,0.8f,0.5f,1.0f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Amp, 0.7f);
    mod(p, 1, ModSource::Seq2, ModDest::Cutoff, 0.4f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.3f; p.fxDelay = 0.3f; p.fxReverb = 0.4f;
}
void seq_sh(SynthPatch& p)
{
    setName(p, "S&H SEQ"); p.category = 11; p.width = 0.6f; p.ampGain = 0.74f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.08f, 0.7f); p.subLevel = 0.12f; p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.28f, 0.4f, 0.2f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.3f, 0.4f, 0.2f, 0.1f};
    // Seq1: random-order stepped cutoff (sample & hold feel)
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].dir = (int) SeqDir::Random; p.seq[0].depth = 1.0f; p.seq[0].retrig = false;
    { const float s[16] = {0.8f,-0.4f,0.6f,-1.0f, 0.2f,1.0f,-0.6f,0.4f, -0.2f,0.9f,-0.8f,0.5f, -1.0f,0.3f,0.7f,-0.5f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    lfo(p, 1, LfoWave::Sine, 0.3f);
    mod(p, 0, ModSource::Seq1, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Lfo2, ModDest::Pw2, 0.2f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxDelay = 0.3f; p.fxReverb = 0.3f;
}
void seq_gate(SynthPatch& p)
{
    setName(p, "GATE SEQ"); p.category = 11; p.width = 0.8f; p.ampGain = 0.62f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.12f, 1.0f); osc(p, 3, SAW, -0.1f, 0.6f);
    p.drift = 0.25f;
    filt(p, FilterMode::LadderLP, 0.55f, 0.15f, 0.2f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.3f, 1.0f, 0.9f, 0.5f, 0.0f};
    // Seq1: 1/8 amp gate
    p.seq[0].sync = true; p.seq[0].syncDiv = 7; p.seq[0].length = 8;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[8] = {1,-1,1,-1, 1,1,-1,1};
      for(int i = 0; i < 8; ++i) p.seq[0].step[i] = s[i]; }
    lfo(p, 1, LfoWave::Sine, 0.2f);
    mod(p, 0, ModSource::Seq1, ModDest::Amp, 0.9f);
    mod(p, 1, ModSource::Lfo2, ModDest::Cutoff, 0.2f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.3f; p.fxDelay = 0.3f; p.fxReverb = 0.4f;
}
void seq_arpbass(SynthPatch& p)
{
    setName(p, "ARP BASS"); p.category = 11; p.width = 0.4f; p.ampGain = 0.78f;
    p.voiceMode = (int) VoiceMode::Mono; p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.03f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.4f, 1); p.subLevel = 0.25f; p.drift = 0.1f;
    filt(p, FilterMode::LadderLP, 0.4f, 0.28f, 0.5f, 0.25f, 0.3f);
    p.env[0] = EnvParams{0.003f, 0.18f, 0.2f, 0.14f, 0.15f};
    p.env[1] = EnvParams{0.003f, 0.16f, 0.05f, 0.12f, 0.0f};
    // Seq1: melodic bass line with octave jumps
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0,1.0f,0, 0,1.0f,0,0.5833f, 0,0,1.0f,0, 0.4167f,0,1.0f,0.5833f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::EnvFilter, ModDest::Cutoff, 0.5f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.3f);
    p.fxDelay = 0.26f; p.fxReverb = 0.18f;
}
void seq_8bit(SynthPatch& p)
{
    setName(p, "CHIPTUNE"); p.category = 11; p.width = 0.5f; p.ampGain = 0.72f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.5f; p.osc[0].crush = 0.55f; p.osc[0].srReduce = 0.4f;
    p.drift = 0.0f;
    filt(p, FilterMode::SvfLP, 0.85f, 0.05f, 0.05f);
    p.env[0] = EnvParams{0.001f, 0.1f, 0.4f, 0.08f, 0.0f};
    // Seq1: fast melodic chiptune arpeggio
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0.3333f,0.5833f,1.0f, 0,0.3333f,0.5833f,1.0f, 0.0833f,0.4167f,0.6667f,1.0f, 0.0833f,0.4167f,0.6667f,1.0f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    lfo(p, 0, LfoWave::Square, 8.0f);
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Lfo1, ModDest::Pw1, 0.3f);
    p.fxDelay = 0.28f; p.fxReverb = 0.15f;
}

// ===================== SEQ wave 2 (extra sequencer presets) =====================
void seq2_a(SynthPatch& p)
{
    setName(p, "SUB STEPPER"); p.category = 11; p.width = 0.35f; p.ampGain = 0.74f;
    p.voiceMode = (int) VoiceMode::Mono; p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.04f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.5f, 1); p.subLevel = 0.35f; p.drift = 0.12f;
    filt(p, FilterMode::LadderLP, 0.32f, 0.34f, 0.5f, 0.3f, 0.3f);
    p.env[0] = EnvParams{0.004f, 0.2f, 0.3f, 0.13f, 0.12f};
    p.env[1] = EnvParams{0.004f, 0.18f, 0.05f, 0.12f, 0.0f};
    // Seq1: melodic sub line, octave-down roots + fifths
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0,-0.4167f,0, 0.5833f,0,-0.4167f,0, 0,0.25f,-0.4167f,0, 0.5833f,0.8333f,-0.4167f,0};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: 1/8 cutoff accents under the line
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[8] = {0.3f,0.7f,0.2f,1.0f, 0.4f,0.8f,0.2f,0.9f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Seq2, ModDest::Cutoff, 0.45f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.4f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    p.fxDrive = 0.2f; p.driveMode = 1; p.fxDelay = 0.2f; p.fxReverb = 0.18f;
}
void seq2_b(SynthPatch& p)
{
    setName(p, "GATE RUSH"); p.category = 11; p.width = 0.85f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.14f, 1.0f); osc(p, 2, PULSE, -0.11f, 0.7f);
    osc(p, 3, SAW, 0.26f, 0.6f); p.subLevel = 0.1f; p.drift = 0.28f;
    filt(p, FilterMode::SvfLP, 0.6f, 0.18f, 0.2f, 0.0f, 0.4f);
    p.env[0] = EnvParams{0.35f, 1.0f, 0.95f, 0.55f, 0.0f};
    // Seq1: dense 1/16 amp gate, syncopated
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {1,1,-1,1, 1,-1,1,1, -1,1,1,-1, 1,-1,1,1};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: slow smooth 1/4 cutoff swell over the gate
    p.seq[1].sync = true; p.seq[1].syncDiv = 10; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Smooth;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = false;
    { const float s[8] = {-0.6f,0.2f,0.6f,1.0f, 0.6f,0.2f,-0.4f,-1.0f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Amp, 0.95f);
    mod(p, 1, ModSource::Seq2, ModDest::Cutoff, 0.5f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.35f; p.fxDelay = 0.3f; p.fxReverb = 0.42f;
}
void seq2_c(SynthPatch& p)
{
    setName(p, "POLY DRIFT"); p.category = 11; p.width = 0.7f; p.ampGain = 0.66f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.1f, 0.8f); p.osc[1].pw = 0.45f; p.drift = 0.22f;
    filt(p, FilterMode::LadderLP, 0.45f, 0.26f, 0.35f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.006f, 0.3f, 0.45f, 0.2f, 0.1f};
    // Seq1: 16-step 1/16 cutoff pattern
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0.3f,0.9f,0.5f,0.2f, 1.0f,0.4f,0.7f,0.3f, 0.8f,0.2f,1.0f,0.5f, 0.3f,0.9f,0.4f,0.6f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: 12-step melodic line at 1/16 -> phases against Seq1 (3:4 cross-rhythm)
    p.seq[1].sync = true; p.seq[1].syncDiv = 4; p.seq[1].length = 12;
    p.seq[1].mode = (int) SeqMode::Melodic; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[16] = {0,0.4167f,0.5833f,0.25f, 0.8333f,0.5833f,0.4167f,1.0f, 0.5833f,0.25f,0.4167f,0, 0,0,0,0};
      for(int i = 0; i < 16; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Seq2, ModDest::Pitch, 1.0f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.3f; p.fxDelay = 0.32f; p.fxReverb = 0.34f;
}
void seq2_d(SynthPatch& p)
{
    setName(p, "CHANCE SEQ"); p.category = 11; p.width = 0.65f; p.ampGain = 0.68f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, 0.07f, 0.6f); p.subLevel = 0.14f; p.drift = 0.2f;
    filt(p, FilterMode::Steiner, 0.42f, 0.3f, 0.4f, 0.2f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.26f, 0.35f, 0.18f, 0.12f};
    // Seq1: random-walk melodic line
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].dir = (int) SeqDir::Random; p.seq[0].depth = 1.0f; p.seq[0].retrig = false;
    { const float s[16] = {0,0.5833f,0.25f,1.0f, 0.4167f,0.8333f,0,0.5833f, 0.25f,1.0f,0.4167f,0.75f, 0,0.5833f,0.8333f,0.25f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: random stepped cutoff (independent dice roll)
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].dir = (int) SeqDir::Random; p.seq[1].depth = 1.0f; p.seq[1].retrig = false;
    { const float s[8] = {-0.6f,0.8f,-0.2f,1.0f, 0.4f,-1.0f,0.6f,-0.4f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Seq2, ModDest::Cutoff, 0.5f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.35f);
    p.fxDelay = 0.34f; p.fxReverb = 0.36f;
}
void seq2_e(SynthPatch& p)
{
    setName(p, "STAB RUSH"); p.category = 11; p.width = 0.78f; p.ampGain = 0.6f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, SAW, 0.13f, 0.9f); osc(p, 2, SAW, -0.1f, 0.7f);
    osc(p, 3, PULSE, 0.2f, 0.5f); p.drift = 0.26f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.2f, 0.45f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.35f, 0.0f, 0.26f, 0.18f};
    p.env[1] = EnvParams{0.004f, 0.28f, 0.0f, 0.2f, 0.0f};
    // Seq1: rhythmic chord-stab gate
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true; p.seq[0].swing = 0.2f;
    { const float s[16] = {1,-1,-1,1, 1,-1,1,-1, 1,1,-1,1, -1,1,-1,-1};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: smooth morph sweep on the stabs
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Smooth;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = false;
    { const float s[8] = {0.0f,0.5f,1.0f,0.5f, -0.5f,0.0f,0.5f,1.0f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Amp, 0.85f);
    mod(p, 1, ModSource::Seq2, ModDest::Morph1, 0.4f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.35f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    p.fxChorus = 0.35f; p.fxDelay = 0.3f; p.fxReverb = 0.4f;
}
void seq2_f(SynthPatch& p)
{
    setName(p, "CROSSWIRE"); p.category = 11; p.width = 0.72f; p.ampGain = 0.66f;
    osc(p, 0, PULSE, 0.0f, 1.0f); p.osc[0].pw = 0.45f; osc(p, 1, SAW, 0.1f, 0.85f);
    p.subLevel = 0.12f; p.drift = 0.2f;
    filt(p, FilterMode::LadderLP, 0.46f, 0.3f, 0.35f, 0.2f, 0.4f);
    p.env[0] = EnvParams{0.005f, 0.3f, 0.5f, 0.2f, 0.1f};
    // Seq1: 1/16 cutoff rhythm
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {1.0f,0.2f,0.6f,0.2f, 0.9f,0.2f,0.5f,0.2f, 1.0f,0.3f,0.7f,0.2f, 0.8f,0.2f,0.6f,0.4f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: 1/8 melodic line -> half the speed, cross-rhythm against Seq1
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Melodic; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[8] = {0,0.5833f,0.4167f,0.8333f, 0.5833f,1.0f,0.4167f,0.25f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Cutoff, 0.55f);
    mod(p, 1, ModSource::Seq2, ModDest::Pitch, 1.0f);
    mod(p, 2, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    p.fxChorus = 0.25f; p.fxDelay = 0.3f; p.fxReverb = 0.3f;
}
void seq2_g(SynthPatch& p)
{
    setName(p, "WT SCANNER"); p.category = 11; p.width = 0.68f; p.ampGain = 0.64f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); p.osc[0].oscType = 1; p.osc[0].wtTable = 6; p.osc[0].wtMorph = 0.3f;
    osc(p, 1, SAW, 0.08f, 0.6f); p.osc[1].oscType = 1; p.osc[1].wtTable = 18; p.drift = 0.18f;
    filt(p, FilterMode::SvfLP, 0.55f, 0.2f, 0.3f, 0.1f, 0.4f);
    p.env[0] = EnvParams{0.006f, 0.4f, 0.6f, 0.3f, 0.1f};
    // Seq1: smooth morph scan across the wavetable
    p.seq[0].sync = true; p.seq[0].syncDiv = 7; p.seq[0].length = 8;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Smooth;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = false;
    { const float s[8] = {-1.0f,-0.4f,0.3f,1.0f, 0.5f,-0.2f,-0.7f,0.2f};
      for(int i = 0; i < 8; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: 1/16 melodic line
    p.seq[1].sync = true; p.seq[1].syncDiv = 4; p.seq[1].length = 16;
    p.seq[1].mode = (int) SeqMode::Melodic; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[16] = {0,0,0.5833f,0, 0.4167f,0,0.8333f,0, 0,0.25f,0.5833f,0, 1.0f,0,0.4167f,0};
      for(int i = 0; i < 16; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Morph1, 0.8f);
    mod(p, 1, ModSource::Seq2, ModDest::Pitch, 1.0f);
    mod(p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.3f);
    p.fxChorus = 0.3f; p.fxDelay = 0.3f; p.fxReverb = 0.38f;
}
void seq2_h(SynthPatch& p)
{
    setName(p, "FOLD STEP"); p.category = 11; p.width = 0.55f; p.ampGain = 0.62f;
    p.voiceMode = (int) VoiceMode::Mono; p.glideMode = (int) GlideMode::Legato; p.glideTime = 0.03f;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.7f); p.subLevel = 0.18f; p.drift = 0.12f;
    filt(p, FilterMode::LadderLP, 0.5f, 0.22f, 0.4f, 0.3f, 0.3f);
    p.env[0] = EnvParams{0.004f, 0.24f, 0.4f, 0.16f, 0.12f};
    p.fxDrive = 0.4f; p.driveMode = 3; p.driveTone = 0.2f; // wavefolder
    // Seq1: melodic line
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0.4167f,0,0.5833f, 0,0.8333f,0.5833f,0.4167f, 0,0.4167f,0,0.5833f, 1.0f,0.8333f,0.5833f,0.25f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: 1/8 fold-drive accents -> rhythmic timbral grit
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[8] = {0.2f,1.0f,0.3f,0.8f, 0.2f,1.0f,0.4f,0.9f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Seq2, ModDest::FilterDrive, 0.5f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.35f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.25f);
    p.fxDelay = 0.24f; p.fxReverb = 0.22f;
}
void seq2_i(SynthPatch& p)
{
    setName(p, "PLUCK RUN"); p.category = 11; p.width = 0.7f; p.ampGain = 0.76f;
    osc(p, 0, TRI, 0.0f, 1.0f); osc(p, 1, SAW, 0.06f, 0.45f); p.drift = 0.12f;
    filt(p, FilterMode::LadderLP, 0.42f, 0.26f, 0.65f, 0.15f, 0.5f);
    p.env[0] = EnvParams{0.002f, 0.13f, 0.0f, 0.1f, 0.18f};
    p.env[1] = EnvParams{0.002f, 0.12f, 0.0f, 0.09f, 0.0f};
    // Seq1: fast 1/16 melodic arpeggio (ascending broken chord)
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0.3333f,0.5833f,1.0f, 0.5833f,0.3333f,0,0.3333f, 0.0833f,0.4167f,0.6667f,1.0f, 0.6667f,0.4167f,0.0833f,0.4167f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: slow pan motion for stereo movement
    p.seq[1].sync = true; p.seq[1].syncDiv = 10; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Smooth;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = false;
    { const float s[8] = {-0.8f,-0.3f,0.4f,0.9f, 0.5f,-0.1f,-0.6f,-1.0f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Seq2, ModDest::Pan, 0.6f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.55f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.35f);
    p.fxChorus = 0.25f; p.fxDelay = 0.34f; p.fxReverb = 0.34f;
}
void seq2_j(SynthPatch& p)
{
    setName(p, "FM STEPPER"); p.category = 11; p.width = 0.6f; p.ampGain = 0.66f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, SINE, 0.0f, 0.8f); p.fm2to1 = 0.4f; p.drift = 0.1f;
    filt(p, FilterMode::LadderLP, 0.6f, 0.14f, 0.3f, 0.1f, 0.5f);
    p.env[0] = EnvParams{0.003f, 0.3f, 0.3f, 0.18f, 0.12f};
    // Seq1: melodic line
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0.5833f,0.25f,0.5833f, 0.8333f,0.5833f,0.25f,0, 0.4167f,0.8333f,0.4167f,0.8333f, 1.0f,0.8333f,0.5833f,0.25f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: smooth FM-index sweep -> evolving bell/bark timbre
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Smooth;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = false;
    { const float s[8] = {-0.8f,0.2f,0.8f,1.0f, 0.4f,-0.2f,-0.6f,-1.0f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Seq2, ModDest::FmAmount, 0.6f);
    mod(p, 2, ModSource::Velocity, ModDest::FmAmount, 0.3f);
    p.fxDelay = 0.3f; p.fxReverb = 0.3f;
}
void seq2_k(SynthPatch& p)
{
    setName(p, "RING STEP"); p.category = 11; p.width = 0.66f; p.ampGain = 0.64f;
    osc(p, 0, SINE, 0.0f, 1.0f); osc(p, 1, TRI, 0.0f, 0.9f); p.ringMod = 0.3f; p.drift = 0.12f;
    filt(p, FilterMode::SvfBP, 0.55f, 0.3f, 0.25f, 0.1f, 0.4f);
    p.env[0] = EnvParams{0.004f, 0.28f, 0.3f, 0.2f, 0.12f};
    // Seq1: melodic line drives osc1 (ring partials track the note)
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Melodic; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0,0.25f,0.5833f,1.0f, 0.5833f,0.25f,0,0.4167f, 0.8333f,0.4167f,0,0.25f, 0.5833f,1.0f,0.8333f,0.5833f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: rhythmic ring-mod depth gate -> metallic on/off
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 8;
    p.seq[1].mode = (int) SeqMode::Curve; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[8] = {1.0f,-0.5f,0.6f,-1.0f, 0.8f,-0.4f,1.0f,-0.6f};
      for(int i = 0; i < 8; ++i) p.seq[1].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Pitch, 1.0f);
    mod(p, 1, ModSource::Seq2, ModDest::RingMod, 0.55f);
    mod(p, 2, ModSource::EnvFilter, ModDest::Cutoff, 0.3f);
    p.fxDelay = 0.32f; p.fxReverb = 0.4f;
}
void seq2_l(SynthPatch& p)
{
    setName(p, "TRIPLE MOD"); p.category = 11; p.width = 0.8f; p.ampGain = 0.62f; p.chorusMode = 2;
    osc(p, 0, SAW, 0.0f, 1.0f); osc(p, 1, PULSE, 0.1f, 0.8f); p.osc[1].pw = 0.4f;
    osc(p, 2, SAW, -0.12f, 0.6f); p.subLevel = 0.1f; p.drift = 0.24f;
    filt(p, FilterMode::LadderLP, 0.48f, 0.24f, 0.3f, 0.15f, 0.4f);
    p.env[0] = EnvParams{0.008f, 0.4f, 0.6f, 0.3f, 0.1f};
    // Seq1: 1/16 cutoff (length 16)
    p.seq[0].sync = true; p.seq[0].syncDiv = 4; p.seq[0].length = 16;
    p.seq[0].mode = (int) SeqMode::Curve; p.seq[0].curve = (int) SeqCurve::Step;
    p.seq[0].depth = 1.0f; p.seq[0].retrig = true;
    { const float s[16] = {0.3f,0.8f,0.4f,1.0f, 0.2f,0.7f,0.5f,0.9f, 0.3f,1.0f,0.4f,0.6f, 0.2f,0.8f,0.5f,1.0f};
      for(int i = 0; i < 16; ++i) p.seq[0].step[i] = s[i]; }
    // Seq2: 1/8 melodic (length 6) -> phases against the others
    p.seq[1].sync = true; p.seq[1].syncDiv = 7; p.seq[1].length = 6;
    p.seq[1].mode = (int) SeqMode::Melodic; p.seq[1].curve = (int) SeqCurve::Step;
    p.seq[1].depth = 1.0f; p.seq[1].retrig = true;
    { const float s[16] = {0,0.4167f,0.5833f,0.8333f, 0.5833f,0.4167f,0,0, 0,0,0,0, 0,0,0,0};
      for(int i = 0; i < 16; ++i) p.seq[1].step[i] = s[i]; }
    // Seq3: slow smooth 1/4 morph drift (length 5)
    p.seq[2].sync = true; p.seq[2].syncDiv = 10; p.seq[2].length = 5;
    p.seq[2].mode = (int) SeqMode::Curve; p.seq[2].curve = (int) SeqCurve::Smooth;
    p.seq[2].depth = 1.0f; p.seq[2].retrig = false;
    { const float s[16] = {-1.0f,-0.2f,0.6f,0.2f, 1.0f,0,0,0, 0,0,0,0, 0,0,0,0};
      for(int i = 0; i < 16; ++i) p.seq[2].step[i] = s[i]; }
    mod(p, 0, ModSource::Seq1, ModDest::Cutoff, 0.5f);
    mod(p, 1, ModSource::Seq2, ModDest::Pitch, 1.0f);
    mod(p, 2, ModSource::Seq3, ModDest::Morph1, 0.4f);
    mod(p, 3, ModSource::Velocity, ModDest::Cutoff, 0.2f);
    p.fxChorus = 0.32f; p.fxDelay = 0.32f; p.fxReverb = 0.4f;
}

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
    // SEQ wave 2 (12)
    seq2_a, seq2_b, seq2_c, seq2_d, seq2_e, seq2_f, seq2_g, seq2_h, seq2_i, seq2_j, seq2_k, seq2_l,
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

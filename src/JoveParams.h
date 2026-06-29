/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "engine/SynthParams.h"
#include "engine/SynthPresets.h"
#include "engine/Arpeggiator.h" // kArpDivNames / kNumArpDiv
#include <juce_audio_processors/juce_audio_processors.h>
#include <unordered_map>

// Single source of truth that keeps the APVTS parameter tree and the engine's
// SynthPatch struct in lock-step. The plugin owns a live SynthPatch; every audio
// block it pulls the current parameter values into that patch (readInto), and on
// a preset load it pushes a patch's values back out to the parameters so the
// host + UI follow (writeFrom). All IDs live here so the front-end and back-end
// can never drift.
namespace jove
{

// ---- ID helpers -------------------------------------------------------------
// Repeated groups (oscillators, LFOs, envelopes, mod slots) use indexed IDs of
// the form "osc2morph", "lfo1rate", "env3attack", "mod7amount". Building them in
// one place guarantees the layout, the read and the write all agree.
namespace jID
{
    // global plugin settings (NOT part of the patch — never written by a preset,
    // so they stay put as the user's standing preference)
    inline constexpr auto maxVoices    = "maxVoices";   // polyphony cap 1..kMaxVoices
    inline constexpr auto quality      = "quality";     // oversampling: Eco/HQ/Ultra
    inline constexpr auto mpeOn        = "mpeOn";        // MPE per-note expression mode
    inline constexpr auto mpeBendRange = "mpeBendRange"; // per-note bend range (st)
    // master FX enables (global quick-bypass; not patch state)
    inline constexpr auto fxDriveOn    = "fxDriveOn";
    inline constexpr auto fxChorusOn   = "fxChorusOn";
    inline constexpr auto fxPhaserOn   = "fxPhaserOn";
    inline constexpr auto fxDelayOn    = "fxDelayOn";
    inline constexpr auto fxReverbOn   = "fxReverbOn";

    // global / voicing (patch)
    inline constexpr auto voiceMode    = "voiceMode";
    inline constexpr auto unisonCount  = "unisonCount";
    inline constexpr auto unisonDetune = "unisonDetune";
    inline constexpr auto unisonSpread = "unisonSpread";
    inline constexpr auto glideMode    = "glideMode";
    inline constexpr auto glideTime    = "glideTime";
    inline constexpr auto bendRange    = "bendRange";
    inline constexpr auto ampGain      = "ampGain";
    inline constexpr auto pan          = "pan";
    inline constexpr auto width        = "width";
    inline constexpr auto drift        = "drift";
    inline constexpr auto masterTune   = "masterTune"; // global fine tune, cents

    // oscillator section
    inline constexpr auto oscMix     = "oscMix";
    inline constexpr auto subLevel   = "subLevel";
    inline constexpr auto subOctave  = "subOctave";
    inline constexpr auto noiseLevel = "noiseLevel";
    inline constexpr auto sync2Mode  = "sync2Mode";
    inline constexpr auto sync3Mode  = "sync3Mode";
    inline constexpr auto fm2to1     = "fm2to1";
    inline constexpr auto ringMod    = "ringMod";

    // filter
    inline constexpr auto filterMode   = "filterMode";
    inline constexpr auto cutoff       = "cutoff";
    inline constexpr auto resonance    = "resonance";
    inline constexpr auto filterDrive  = "filterDrive";
    inline constexpr auto keyTrack     = "keyTrack";
    inline constexpr auto envFilterAmt = "envFilterAmt";
    inline constexpr auto filterRouting = "filterRouting"; // single / serial / parallel
    inline constexpr auto filter2Mode   = "filter2Mode";
    inline constexpr auto filter2Cutoff = "filter2Cutoff";
    inline constexpr auto filter2Reso   = "filter2Reso";
    inline constexpr auto filter2Drive  = "filter2Drive";
    inline constexpr auto filter2EnvAmt = "filter2EnvAmt";

    // arp
    inline constexpr auto arpOn      = "arpOn";
    inline constexpr auto arpMode    = "arpMode";
    inline constexpr auto arpOctaves = "arpOctaves";
    inline constexpr auto arpSyncDiv = "arpSyncDiv";
    inline constexpr auto arpGate    = "arpGate";
    inline constexpr auto arpSwing   = "arpSwing";
    inline constexpr auto arpLatch   = "arpLatch";
    inline constexpr auto arpRatchet = "arpRatchet";

    // FX
    inline constexpr auto fxChorus   = "fxChorus";
    inline constexpr auto chorusMode = "chorusMode";
    inline constexpr auto chorusRate = "chorusRate";
    inline constexpr auto chorusDepth= "chorusDepth";
    inline constexpr auto fxPhaser   = "fxPhaser";
    inline constexpr auto phaserRate = "phaserRate";
    inline constexpr auto phaserDepth= "phaserDepth";
    inline constexpr auto phaserFeedback = "phaserFeedback";
    inline constexpr auto fxDelay    = "fxDelay";
    inline constexpr auto fxReverb   = "fxReverb";
    inline constexpr auto fxDrive    = "fxDrive";
    inline constexpr auto driveTone  = "driveTone";
    inline constexpr auto mbLow      = "mbLow";  // multiband saturation: low band
    inline constexpr auto mbMid      = "mbMid";
    inline constexpr auto mbHigh     = "mbHigh";
    // delay voicing
    inline constexpr auto delayMode     = "delayMode";
    inline constexpr auto delaySync     = "delaySync";
    inline constexpr auto delayDiv      = "delayDiv";
    inline constexpr auto delayTimeMs   = "delayTimeMs";
    inline constexpr auto delayFeedback = "delayFeedback";
    inline constexpr auto delayTone     = "delayTone";
    inline constexpr auto delayPing     = "delayPing";
    inline constexpr auto delayFltType  = "delayFltType";
    inline constexpr auto delayFltFreq  = "delayFltFreq";
    inline constexpr auto delayFltQ     = "delayFltQ";
    // reverb voicing
    inline constexpr auto reverbSize    = "reverbSize";
    inline constexpr auto reverbTone    = "reverbTone";

    // indexed groups
    inline juce::String osc (int i, const char* f)  { return "osc"  + juce::String(i + 1) + f; }
    inline juce::String lfo (int i, const char* f)  { return "lfo"  + juce::String(i + 1) + f; }
    inline juce::String env (int i, const char* f)  { return "env"  + juce::String(i + 1) + f; }
    inline juce::String mod (int i, const char* f)  { return "mod"  + juce::String(i + 1) + f; }
} // namespace jID

// ---- small helpers ----------------------------------------------------------
inline juce::StringArray namesToArray(const char* const* names, int count)
{
    juce::StringArray a;
    for(int i = 0; i < count; ++i)
        a.add(names[i]);
    return a;
}

inline juce::NormalisableRange<float> timeRange()
{
    // 0..8 s, skewed so the musically-useful short times get most of the travel.
    auto r = juce::NormalisableRange<float>(0.0f, 8.0f);
    r.setSkewForCentre(0.4f);
    return r;
}

// =============================================================================
// Parameter layout
// =============================================================================
inline juce::AudioProcessorValueTreeState::ParameterLayout createJoveLayout()
{
    using P  = juce::AudioParameterFloat;
    using C  = juce::AudioParameterChoice;
    using B  = juce::AudioParameterBool;
    using I  = juce::AudioParameterInt;
    using FR = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto fparam = [&](const juce::String& id, const juce::String& name, FR range,
                      float def, const juce::String& unit = {})
    {
        p.push_back(std::make_unique<P>(juce::ParameterID{id, 1}, name, range, def,
                                        juce::AudioParameterFloatAttributes().withLabel(unit)));
    };
    auto cparam = [&](const juce::String& id, const juce::String& name,
                      const juce::StringArray& choices, int def)
    {
        p.push_back(std::make_unique<C>(juce::ParameterID{id, 1}, name, choices, def));
    };
    auto bparam = [&](const juce::String& id, const juce::String& name, bool def)
    {
        p.push_back(std::make_unique<B>(juce::ParameterID{id, 1}, name, def));
    };
    auto iparam = [&](const juce::String& id, const juce::String& name, int lo, int hi, int def)
    {
        p.push_back(std::make_unique<I>(juce::ParameterID{id, 1}, name, lo, hi, def));
    };

    const auto lin01   = FR(0.0f, 1.0f);
    const auto bip     = FR(-1.0f, 1.0f);
    const auto arpDivs = namesToArray(kArpDivNames, kNumArpDiv);

    // ---- global plugin settings (not patch state) ----
    iparam(jID::maxVoices, "Max Polyphony", 1, kMaxVoices, 8); // up to 16, default 8
    cparam(jID::quality, "Quality", {"Eco", "HQ", "Ultra"}, 1); // default HQ (2x)
    // MPE: global plugin settings (not patch state — presets never change them,
    // so they stay out of PatchBinding and never touch param_roundtrip).
    bparam(jID::mpeOn, "MPE", false);
    iparam(jID::mpeBendRange, "MPE Bend Range", 1, 96, 48); // MPE default 48 st
    // master FX enables — global quick-bypass toggles (default on). Not patch
    // state, so they persist while auditioning presets and never touch round-trip.
    bparam(jID::fxDriveOn, "Drive On", true);
    bparam(jID::fxChorusOn, "Chorus On", true);
    bparam(jID::fxPhaserOn, "Phaser On", true);
    bparam(jID::fxDelayOn, "Delay On", true);
    bparam(jID::fxReverbOn, "Reverb On", true);

    // ---- global / voicing ----
    cparam(jID::voiceMode, "Voice Mode", namesToArray(kVoiceModeNames, (int)VoiceMode::Count), 0);
    iparam(jID::unisonCount, "Unison Voices", 1, kMaxUnison, 5);
    fparam(jID::unisonDetune, "Unison Detune", lin01, 0.15f);
    fparam(jID::unisonSpread, "Unison Spread", lin01, 0.6f);
    cparam(jID::glideMode, "Glide Mode", namesToArray(kGlideModeNames, (int)GlideMode::Count), 0);
    fparam(jID::glideTime, "Glide Time", [] { auto r = FR(0.0f, 2.0f); r.setSkewForCentre(0.2f); return r; }(), 0.08f, "s");
    iparam(jID::bendRange, "Bend Range", 1, 24, 2);
    fparam(jID::ampGain, "Amp", lin01, 0.8f);
    fparam(jID::pan, "Pan", bip, 0.0f);
    fparam(jID::width, "Width", lin01, 0.5f);
    fparam(jID::drift, "Analog Drift", lin01, 0.2f);
    fparam(jID::masterTune, "Master Tune", FR(-100.0f, 100.0f), 0.0f, "ct");

    // ---- oscillators ----
    const auto footage = namesToArray(kFootageNames, kNumFootage);
    for(int i = 0; i < kNumOsc; ++i)
    {
        const juce::String n = "Osc " + juce::String(i + 1) + " ";
        bparam(jID::osc(i, "On"),     n + "On", i < 2);
        cparam(jID::osc(i, "Foot"),   n + "Footage", footage, 2);
        fparam(jID::osc(i, "Morph"),  n + "Morph", lin01, 0.5f);
        fparam(jID::osc(i, "Pw"),     n + "Pulse Width", lin01, 0.5f);
        // ±24 st with a gentle symmetric skew (finer control near centre, full
        // reach at the ends). NB the skew must stay moderate: skew applies as
        // |dist|^(1/skew), so an extreme skew like 0.16 = |dist|^6.25 makes a
        // detune of 0.001 st map to ~0.6 normalised — the knob reads "0.00" yet
        // sits far off-centre. 0.5 keeps near-zero values visually centred.
        fparam(jID::osc(i, "Detune"), n + "Detune",
               [] { juce::NormalisableRange<float> r(-24.0f, 24.0f, 0.0f, 0.5f, true); return r; }(), 0.0f, "st");
        fparam(jID::osc(i, "Level"),  n + "Level", lin01, 1.0f);
        // bit-crush: 0 clean, up -> coarser quantisation for old-school DCO grit
        fparam(jID::osc(i, "Crush"),  n + "Bit Crush", lin01, 0.0f);
        fparam(jID::osc(i, "Sr"),     n + "SR Crush", lin01, 0.0f); // sample-rate reduction
        cparam(jID::osc(i, "Type"),   n + "Osc Type", {"BLEP", "WT"}, 0);
        iparam(jID::osc(i, "WtTable"), n + "WT Table", 0, kNumWt - 1, 0);
        fparam(jID::osc(i, "WtMorph"), n + "WT Morph", lin01, 0.0f);
    }
    fparam(jID::oscMix, "Osc Mix", lin01, 0.5f);
    fparam(jID::subLevel, "Sub Level", lin01, 0.0f);
    cparam(jID::subOctave, "Sub Octave", {"-1", "-2"}, 0);
    fparam(jID::noiseLevel, "Noise", lin01, 0.0f);
    const auto syncModes = namesToArray(kSyncModeNames, (int)SyncMode::Count);
    cparam(jID::sync2Mode, "Osc2 Sync", syncModes, 0);
    cparam(jID::sync3Mode, "Osc3 Sync", syncModes, 0);
    fparam(jID::fm2to1, "FM 2>1", lin01, 0.0f);
    fparam(jID::ringMod, "Ring Mod", lin01, 0.0f);

    // ---- filter ----
    cparam(jID::filterMode, "Filter Mode", namesToArray(kFilterModeNames, (int)FilterMode::Count), 0);
    fparam(jID::cutoff, "Cutoff", lin01, 0.6f);
    fparam(jID::resonance, "Resonance", lin01, 0.1f);
    fparam(jID::filterDrive, "Filter Drive", lin01, 0.0f);
    fparam(jID::keyTrack, "Key Track", lin01, 0.5f);
    fparam(jID::envFilterAmt, "Env > Cutoff", bip, 0.4f); // bipolar: -1 inverse env
    cparam(jID::filterRouting, "Filter Routing", {"SINGLE", "SERIAL", "PARALLEL"}, 0);
    cparam(jID::filter2Mode, "Filter 2 Mode", namesToArray(kFilterModeNames, (int)FilterMode::Count), 1);
    fparam(jID::filter2Cutoff, "Filter 2 Cutoff", lin01, 0.6f);
    fparam(jID::filter2Reso, "Filter 2 Resonance", lin01, 0.1f);
    fparam(jID::filter2Drive, "Filter 2 Drive", lin01, 0.0f);
    fparam(jID::filter2EnvAmt, "Filter 2 Env > Cutoff", bip, 0.0f); // bipolar

    // ---- envelopes (0 amp, 1 filter, 2 aux) ----
    for(int i = 0; i < kNumEnv; ++i)
    {
        const char* tag = (i == 0) ? "Amp " : (i == 1) ? "Filter " : "Aux ";
        const juce::String n = juce::String("Env ") + tag;
        fparam(jID::env(i, "Attack"),  n + "Attack", timeRange(), i == 0 ? 0.005f : 0.01f, "s");
        fparam(jID::env(i, "Decay"),   n + "Decay", timeRange(), 0.2f, "s");
        fparam(jID::env(i, "Sustain"), n + "Sustain", lin01, 0.7f);
        fparam(jID::env(i, "Release"), n + "Release", timeRange(), 0.3f, "s");
        fparam(jID::env(i, "Vel"),     n + "Vel>Level", lin01, 0.0f);
    }

    // ---- LFOs ----
    const auto lfoWaves = namesToArray(kLfoWaveNames, (int)LfoWave::Count);
    for(int i = 0; i < kNumLfo; ++i)
    {
        const juce::String n = "LFO " + juce::String(i + 1) + " ";
        cparam(jID::lfo(i, "Wave"), n + "Wave", lfoWaves, i == 0 ? 1 : 0);
        fparam(jID::lfo(i, "Rate"), n + "Rate", [] { auto r = FR(0.01f, 60.0f); r.setSkewForCentre(2.0f); return r; }(), 1.0f, "Hz");
        fparam(jID::lfo(i, "Depth"), n + "Depth", FR(0.0f, 2.0f), 1.0f);
        bparam(jID::lfo(i, "Sync"), n + "Sync", false);
        cparam(jID::lfo(i, "Div"), n + "Division", arpDivs, 10);
        bparam(jID::lfo(i, "Retrig"), n + "Retrig", true);
        bparam(jID::lfo(i, "PerVoice"), n + "Per Voice", false);
        fparam(jID::lfo(i, "Fade"), n + "Fade", FR(0.0f, 5.0f), 0.0f, "s");
        fparam(jID::lfo(i, "Delay"), n + "Delay", FR(0.0f, 5.0f), 0.0f, "s");
        fparam(jID::lfo(i, "Offset"), n + "Offset", bip, 0.0f);
        fparam(jID::lfo(i, "Phase"),  n + "Start Phase", lin01, 0.0f);
    }

    // ---- mod matrix ----
    const auto modSrc = namesToArray(kModSourceNames, (int)ModSource::Count);
    const auto modDst = namesToArray(kModDestNames, (int)ModDest::Count);
    for(int i = 0; i < kNumModSlots; ++i)
    {
        const juce::String n = "Mod " + juce::String(i + 1) + " ";
        cparam(jID::mod(i, "Src"), n + "Source", modSrc, 0);
        cparam(jID::mod(i, "Dst"), n + "Dest", modDst, 0);
        // ±2: slots are nominally bipolar -1..+1, but some patches push a single
        // strong route (e.g. env->pitch zap) past unity for extra throw.
        fparam(jID::mod(i, "Amt"), n + "Amount", FR(-2.0f, 2.0f), 0.0f);
    }

    // ---- arpeggiator ----
    bparam(jID::arpOn, "Arp On", false);
    cparam(jID::arpMode, "Arp Mode", namesToArray(kArpModeNames, (int)ArpMode::Count), 0);
    iparam(jID::arpOctaves, "Arp Octaves", 1, 4, 1);
    cparam(jID::arpSyncDiv, "Arp Division", arpDivs, 4); // 1/16
    fparam(jID::arpGate, "Arp Gate", FR(0.05f, 2.0f), 0.5f); // up to 200% (overlap)
    fparam(jID::arpSwing, "Arp Swing", FR(0.0f, 0.66f), 0.0f);
    bparam(jID::arpLatch, "Arp Latch", false);
    iparam(jID::arpRatchet, "Arp Ratchet", 1, 4, 1);

    // ---- FX ----
    fparam(jID::fxDrive, "Drive", lin01, 0.0f);
    fparam(jID::driveTone, "Drive Tone", bip, 0.0f);
    fparam(jID::mbLow, "MB Sat Low", lin01, 0.0f);
    fparam(jID::mbMid, "MB Sat Mid", lin01, 0.0f);
    fparam(jID::mbHigh, "MB Sat High", lin01, 0.0f);
    fparam(jID::fxChorus, "Chorus", lin01, 0.3f);
    cparam(jID::chorusMode, "Chorus Mode", {"Chorus I", "Chorus II", "Ensemble", "Combine"}, 0);
    fparam(jID::chorusRate, "Chorus Rate", [] { auto r = FR(0.25f, 4.0f); r.setSkewForCentre(1.0f); return r; }(), 1.0f);
    fparam(jID::chorusDepth, "Chorus Depth", FR(0.0f, 2.0f), 1.0f);
    fparam(jID::fxPhaser, "Phaser", lin01, 0.0f);
    fparam(jID::phaserRate, "Phaser Rate", [] { auto r = FR(0.02f, 16.0f); r.setSkewForCentre(0.6f); return r; }(), 0.25f, "Hz");
    fparam(jID::phaserDepth, "Phaser Depth", lin01, 0.85f);
    fparam(jID::phaserFeedback, "Phaser Feedback", lin01, 0.45f);
    fparam(jID::fxDelay, "Delay", lin01, 0.2f);
    cparam(jID::delayMode, "Delay Mode", {"DIGITAL", "ANALOG", "TAPE"}, 0);
    bparam(jID::delaySync, "Delay Sync", true);
    cparam(jID::delayDiv, "Delay Division", arpDivs, 9); // 1/8.
    fparam(jID::delayTimeMs, "Delay Time", [] { auto r = FR(20.0f, 2000.0f); r.setSkewForCentre(350.0f); return r; }(), 350.0f, "ms");
    fparam(jID::delayFeedback, "Delay Feedback", lin01, 0.42f);
    fparam(jID::delayTone, "Delay Tone", lin01, 0.45f);
    bparam(jID::delayPing, "Delay Ping-Pong", true);
    cparam(jID::delayFltType, "Delay Filter", {"LP", "HP", "BP"}, 0);
    fparam(jID::delayFltFreq, "Delay Filter Freq", [] { auto r = FR(40.0f, 18000.0f); r.setSkewForCentre(1200.0f); return r; }(), 12000.0f, "Hz");
    fparam(jID::delayFltQ, "Delay Filter Q", lin01, 0.2f);
    fparam(jID::fxReverb, "Reverb", lin01, 0.25f);
    fparam(jID::reverbSize, "Reverb Size", lin01, 0.6f);
    fparam(jID::reverbTone, "Reverb Tone", lin01, 0.4f);

    return { p.begin(), p.end() };
}

// =============================================================================
// PatchBinding — caches the raw atomic pointers (RT read) and the ranged
// parameters (preset write), and maps both directions.
// =============================================================================
class PatchBinding
{
  public:
    void connect(juce::AudioProcessorValueTreeState& s)
    {
        apvts_ = &s;
        // cache every parameter once; lookups by id below are O(1)
        for(auto* param : s.processor.getParameters())
            if(auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param))
                ranged_[rp->getParameterID()] = rp;
    }

    // RT-safe: pull the current parameter values into `p`. Called once per block.
    void readInto(SynthPatch& p) const noexcept
    {
        using namespace jID;
        p.voiceMode    = (int)get(voiceMode);
        p.unisonCount  = (int)get(unisonCount);
        p.unisonDetune = get(unisonDetune);
        p.unisonSpread = get(unisonSpread);
        p.glideMode    = (int)get(glideMode);
        p.glideTime    = get(glideTime);
        p.bendRange    = (int)get(bendRange);
        p.ampGain      = get(ampGain);
        p.pan          = get(pan);
        p.width        = get(width);
        p.drift        = get(drift);
        p.masterTune   = get(masterTune);

        for(int i = 0; i < kNumOsc; ++i)
        {
            p.osc[i].on     = get(osc(i, "On")) > 0.5f;
            p.osc[i].footage= (int)get(osc(i, "Foot"));
            p.osc[i].morph  = get(osc(i, "Morph"));
            p.osc[i].pw     = get(osc(i, "Pw"));
            p.osc[i].detune = get(osc(i, "Detune"));
            p.osc[i].level  = get(osc(i, "Level"));
            p.osc[i].crush  = get(osc(i, "Crush"));
            p.osc[i].srReduce = get(osc(i, "Sr"));
            p.osc[i].oscType  = (int) get(osc(i, "Type"));
            p.osc[i].wtTable  = (int) get(osc(i, "WtTable"));
            p.osc[i].wtMorph  = get(osc(i, "WtMorph"));
        }
        p.oscMix     = get(oscMix);
        p.subLevel   = get(subLevel);
        p.subOctave  = (int)get(subOctave) + 1; // choice 0/1 -> octave 1/2
        p.noiseLevel = get(noiseLevel);
        p.sync2Mode  = (int)get(sync2Mode);
        p.sync3Mode  = (int)get(sync3Mode);
        p.fm2to1     = get(fm2to1);
        p.ringMod    = get(ringMod);

        p.filterMode   = (int)get(filterMode);
        p.cutoff       = get(cutoff);
        p.resonance    = get(resonance);
        p.filterDrive  = get(filterDrive);
        p.keyTrack     = get(keyTrack);
        p.envFilterAmt = get(envFilterAmt);
        p.filterRouting = (int) get(filterRouting);
        p.filter2Mode   = (int) get(filter2Mode);
        p.filter2Cutoff = get(filter2Cutoff);
        p.filter2Reso   = get(filter2Reso);
        p.filter2Drive  = get(filter2Drive);
        p.filter2EnvAmt = get(filter2EnvAmt);

        for(int i = 0; i < kNumEnv; ++i)
        {
            p.env[i].attack     = get(env(i, "Attack"));
            p.env[i].decay      = get(env(i, "Decay"));
            p.env[i].sustain    = get(env(i, "Sustain"));
            p.env[i].release    = get(env(i, "Release"));
            p.env[i].velToLevel = get(env(i, "Vel"));
        }

        for(int i = 0; i < kNumLfo; ++i)
        {
            p.lfo[i].wave     = (int)get(lfo(i, "Wave"));
            p.lfo[i].rate     = get(lfo(i, "Rate"));
            p.lfo[i].depth    = get(lfo(i, "Depth"));
            p.lfo[i].sync     = get(lfo(i, "Sync")) > 0.5f;
            p.lfo[i].syncDiv  = (int)get(lfo(i, "Div"));
            p.lfo[i].retrig   = get(lfo(i, "Retrig")) > 0.5f;
            p.lfo[i].perVoice = get(lfo(i, "PerVoice")) > 0.5f;
            p.lfo[i].fade     = get(lfo(i, "Fade"));
            p.lfo[i].delay    = get(lfo(i, "Delay"));
            p.lfo[i].offset   = get(lfo(i, "Offset"));
            p.lfo[i].phase    = get(lfo(i, "Phase"));
        }

        for(int i = 0; i < kNumModSlots; ++i)
        {
            p.mod[i].source = (int)get(mod(i, "Src"));
            p.mod[i].dest   = (int)get(mod(i, "Dst"));
            p.mod[i].amount = get(mod(i, "Amt"));
        }

        p.arp.on      = get(arpOn) > 0.5f;
        p.arp.mode    = (int)get(arpMode);
        p.arp.octaves = (int)get(arpOctaves);
        p.arp.syncDiv = (int)get(arpSyncDiv);
        p.arp.gate    = get(arpGate);
        p.arp.swing   = get(arpSwing);
        p.arp.latch   = get(arpLatch) > 0.5f;
        p.arp.ratchet = (int)get(arpRatchet);

        p.fxDrive    = get(fxDrive);
        p.driveTone  = get(driveTone);
        p.mbLow      = get(mbLow);
        p.mbMid      = get(mbMid);
        p.mbHigh     = get(mbHigh);
        p.fxChorus   = get(fxChorus);
        p.chorusMode = (int)get(chorusMode);
        p.chorusRate = get(chorusRate);
        p.chorusDepth= get(chorusDepth);
        p.fxPhaser   = get(fxPhaser);
        p.phaserRate = get(phaserRate);
        p.phaserDepth= get(phaserDepth);
        p.phaserFeedback = get(phaserFeedback);
        p.fxDelay    = get(fxDelay);
        p.fxReverb   = get(fxReverb);
        p.delayMode     = (int) get(delayMode);
        p.delaySync     = get(delaySync) > 0.5f;
        p.delayDiv      = (int)get(delayDiv);
        p.delayTimeMs   = get(delayTimeMs);
        p.delayFeedback = get(delayFeedback);
        p.delayTone     = get(delayTone);
        p.delayPing     = get(delayPing) > 0.5f;
        p.delayFltType  = (int) get(delayFltType);
        p.delayFltFreq  = get(delayFltFreq);
        p.delayFltQ     = get(delayFltQ);
        p.reverbSize    = get(reverbSize);
        p.reverbTone    = get(reverbTone);
    }

    // Message thread: push a patch's values out to the parameters (preset load).
    void writeFrom(const SynthPatch& p) noexcept
    {
        using namespace jID;
        set(voiceMode, (float)p.voiceMode);
        set(unisonCount, (float)p.unisonCount);
        set(unisonDetune, p.unisonDetune);
        set(unisonSpread, p.unisonSpread);
        set(glideMode, (float)p.glideMode);
        set(glideTime, p.glideTime);
        set(bendRange, (float)p.bendRange);
        set(ampGain, p.ampGain);
        set(pan, p.pan);
        set(width, p.width);
        set(drift, p.drift);
        set(masterTune, p.masterTune);

        for(int i = 0; i < kNumOsc; ++i)
        {
            set(osc(i, "On"), p.osc[i].on ? 1.0f : 0.0f);
            set(osc(i, "Foot"), (float)p.osc[i].footage);
            set(osc(i, "Morph"), p.osc[i].morph);
            set(osc(i, "Pw"), p.osc[i].pw);
            set(osc(i, "Detune"), p.osc[i].detune);
            set(osc(i, "Level"), p.osc[i].level);
            set(osc(i, "Crush"), p.osc[i].crush);
            set(osc(i, "Sr"), p.osc[i].srReduce);
            set(osc(i, "Type"), (float) p.osc[i].oscType);
            set(osc(i, "WtTable"), (float) p.osc[i].wtTable);
            set(osc(i, "WtMorph"), p.osc[i].wtMorph);
        }
        set(oscMix, p.oscMix);
        set(subLevel, p.subLevel);
        set(subOctave, (float)(p.subOctave - 1));
        set(noiseLevel, p.noiseLevel);
        set(sync2Mode, (float)p.sync2Mode);
        set(sync3Mode, (float)p.sync3Mode);
        set(fm2to1, p.fm2to1);
        set(ringMod, p.ringMod);

        set(filterMode, (float)p.filterMode);
        set(cutoff, p.cutoff);
        set(resonance, p.resonance);
        set(filterDrive, p.filterDrive);
        set(keyTrack, p.keyTrack);
        set(envFilterAmt, p.envFilterAmt);
        set(filterRouting, (float) p.filterRouting);
        set(filter2Mode, (float) p.filter2Mode);
        set(filter2Cutoff, p.filter2Cutoff);
        set(filter2Reso, p.filter2Reso);
        set(filter2Drive, p.filter2Drive);
        set(filter2EnvAmt, p.filter2EnvAmt);

        for(int i = 0; i < kNumEnv; ++i)
        {
            set(env(i, "Attack"), p.env[i].attack);
            set(env(i, "Decay"), p.env[i].decay);
            set(env(i, "Sustain"), p.env[i].sustain);
            set(env(i, "Release"), p.env[i].release);
            set(env(i, "Vel"), p.env[i].velToLevel);
        }

        for(int i = 0; i < kNumLfo; ++i)
        {
            set(lfo(i, "Wave"), (float)p.lfo[i].wave);
            set(lfo(i, "Rate"), p.lfo[i].rate);
            set(lfo(i, "Depth"), p.lfo[i].depth);
            set(lfo(i, "Sync"), p.lfo[i].sync ? 1.0f : 0.0f);
            set(lfo(i, "Div"), (float)p.lfo[i].syncDiv);
            set(lfo(i, "Retrig"), p.lfo[i].retrig ? 1.0f : 0.0f);
            set(lfo(i, "PerVoice"), p.lfo[i].perVoice ? 1.0f : 0.0f);
            set(lfo(i, "Fade"), p.lfo[i].fade);
            set(lfo(i, "Delay"), p.lfo[i].delay);
            set(lfo(i, "Offset"), p.lfo[i].offset);
            set(lfo(i, "Phase"), p.lfo[i].phase);
        }

        for(int i = 0; i < kNumModSlots; ++i)
        {
            set(mod(i, "Src"), (float)p.mod[i].source);
            set(mod(i, "Dst"), (float)p.mod[i].dest);
            set(mod(i, "Amt"), p.mod[i].amount);
        }

        set(arpOn, p.arp.on ? 1.0f : 0.0f);
        set(arpMode, (float)p.arp.mode);
        set(arpOctaves, (float)p.arp.octaves);
        set(arpSyncDiv, (float)p.arp.syncDiv);
        set(arpGate, p.arp.gate);
        set(arpSwing, p.arp.swing);
        set(arpLatch, p.arp.latch ? 1.0f : 0.0f);
        set(arpRatchet, (float)p.arp.ratchet);

        set(fxDrive, p.fxDrive);
        set(driveTone, p.driveTone);
        set(mbLow, p.mbLow);
        set(mbMid, p.mbMid);
        set(mbHigh, p.mbHigh);
        set(fxChorus, p.fxChorus);
        set(chorusMode, (float)p.chorusMode);
        set(chorusRate, p.chorusRate);
        set(chorusDepth, p.chorusDepth);
        set(fxPhaser, p.fxPhaser);
        set(phaserRate, p.phaserRate);
        set(phaserDepth, p.phaserDepth);
        set(phaserFeedback, p.phaserFeedback);
        set(fxDelay, p.fxDelay);
        set(fxReverb, p.fxReverb);
        set(delayMode, (float) p.delayMode);
        set(delaySync, p.delaySync ? 1.0f : 0.0f);
        set(delayDiv, (float)p.delayDiv);
        set(delayTimeMs, p.delayTimeMs);
        set(delayFeedback, p.delayFeedback);
        set(delayTone, p.delayTone);
        set(delayPing, p.delayPing ? 1.0f : 0.0f);
        set(delayFltType, (float) p.delayFltType);
        set(delayFltFreq, p.delayFltFreq);
        set(delayFltQ, p.delayFltQ);
        set(reverbSize, p.reverbSize);
        set(reverbTone, p.reverbTone);
    }

  private:
    float get(const juce::String& id) const noexcept
    {
        auto it = ranged_.find(id);
        return it != ranged_.end()
                   ? it->second->convertFrom0to1(it->second->getValue())
                   : 0.0f;
    }
    void set(const juce::String& id, float denorm) noexcept
    {
        auto it = ranged_.find(id);
        if(it == ranged_.end())
            return;
        auto* rp = it->second;
        rp->setValueNotifyingHost(rp->convertTo0to1(denorm));
    }

    juce::AudioProcessorValueTreeState* apvts_ = nullptr;
    std::unordered_map<juce::String, juce::RangedAudioParameter*> ranged_;
};

} // namespace jove

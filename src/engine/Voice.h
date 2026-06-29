/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "AdsrEnv.h"
#include "BlepOsc.h"
#include "SynthConfig.h"
#include "SynthParams.h"
#include "VoiceFilter.h"
#include "WavetableOsc.h"
#include <cmath>
#include <cstdint>

namespace jove
{
inline float midiToHz(float note) noexcept
{
    return kA4Hz * std::exp2((note - (float)kA4Note) * (1.0f / 12.0f));
}

// Per-block modulation context handed to each voice by the engine: the current
// values of the global/per-voice mod sources, already resolved. The voice mixes
// these onto its patch values per sample where it matters (pitch, cutoff) so
// modulation never zippers.
struct VoiceMod
{
    float pitchSemis = 0.0f; // additive pitch mod (semitones) for all oscs
    float osc2Semis  = 0.0f;
    float osc3Semis  = 0.0f;
    float cutoffOct  = 0.0f; // additive cutoff mod in octaves
    float resAdd     = 0.0f;
    float ampMul     = 1.0f; // multiplicative amp (tremolo)
    float panAdd     = 0.0f;
    float morphAdd[kNumOsc] = {0, 0, 0};
    float pwAdd[kNumOsc]    = {0, 0, 0};
    float subAdd   = 0.0f;
    float noiseAdd = 0.0f;
    float fmAdd    = 0.0f; // osc2->osc1 FM index offset (e-piano bark)
    float ringAdd  = 0.0f; // ring-mod depth offset (env/LFO -> metallic swell)
    float oscMixAdd = 0.0f; // osc1<->osc2 balance offset
    float driveAdd  = 0.0f; // filter pre-drive offset
    float envFltAdd = 0.0f; // filter-env depth offset
    float detuneAdd = 0.0f; // osc2 (+) / osc3 (-) detune spread offset
};

// One synth voice: 3 morphing BLEP oscillators + sub + noise -> selectable
// ladder/SVF filter -> VCA, driven by 3 envelopes (amp / filter / aux). Glide,
// hard sync, cross-mod and analog drift live here. The engine owns voice
// allocation and the mod sources; the voice renders click-free audio into a
// stereo accumulator.
class Voice
{
  public:
    void prepare(float sampleRate, float blockRate) noexcept
    {
        sr_        = sampleRate;
        blockRate_ = blockRate;
        for(int i = 0; i < kNumOsc; ++i)
        {
            osc_[i].prepare(sampleRate);
            wt_[i].prepare(sampleRate);
        }
        sub_.prepare(sampleRate);
        filter_.prepare(sampleRate);
        env_[0].prepare(sampleRate);
        env_[1].prepare(sampleRate);
        env_[2].prepare(sampleRate);
        rng_ = 0x2545F491u;
        reset();
    }

    void reset() noexcept
    {
        for(int i = 0; i < kNumOsc; ++i)
        {
            osc_[i].reset();
            wt_[i].reset();
            srPhase_[i] = 0.0f;
            srHold_[i] = 0.0f;
        }
        sub_.reset();
        filter_.reset();
        for(int i = 0; i < kNumEnv; ++i)
            env_[i].reset();
        active_  = false;
        note_    = 60;
        gateOn_  = false;
        curHz_   = midiToHz(60);
        targetHz_ = curHz_;
        ampSmooth_ = 0.0f;
        driftPhase_ = 0.0f;
        driftVal_   = 0.0f;
        driftTarget_ = 0.0f;
    }

    bool  active() const noexcept { return active_; }
    bool  gateOn() const noexcept { return gateOn_; }
    int   note() const noexcept { return note_; }
    uint32_t age() const noexcept { return age_; }
    float ampEnvLevel() const noexcept { return env_[0].value(); }

    // ---- per-voice mod-source values (read by the engine's matrix each block;
    // env values lag one block, inaudible at the 1 ms control rate) ----------
    float velocity() const noexcept { return vel_; }
    float keyTrackSource() const noexcept { return ((float)note_ - 60.0f) / 60.0f; } // ~-1..+1
    float noteSource() const noexcept { return ((float)note_ - 64.0f) / 64.0f; }
    float ampEnvValue() const noexcept { return env_[0].value(); }
    float filterEnvValue() const noexcept { return env_[1].value(); }
    // auxEnvValue() defined below; random latched at note-on
    float randomSource() const noexcept { return random_; }

    // Note start. `glide` true slides to the new pitch; false snaps. glideFromHz,
    // when > 0 and gliding, is the pitch to start the slide FROM — used for poly
    // portamento (each new voice glides from the previously-played note). For
    // mono legato pass <=0 to keep the voice's current pitch. Randomised osc start
    // phases avoid a static, comb-filtered unison attack.
    void noteOn(int note, float velocity, const SynthPatch& p, bool glide,
                float glideFromHz = -1.0f) noexcept
    {
        note_     = note;
        vel_      = velocity;
        targetHz_ = midiToHz((float)note + detuneCents_);
        const bool doGlide = glide && p.glideMode != (int)GlideMode::Off && p.glideTime > 0.001f;
        if(!doGlide)
            curHz_ = targetHz_;
        else if(glideFromHz > 0.0f)
            curHz_ = glideFromHz; // poly portamento: from the previous note
        else if(!active_)
            curHz_ = targetHz_;   // no source pitch -> snap
        // else: mono legato — keep curHz_ and slide from where we are
        active_ = true;
        gateOn_ = true;
        age_    = 0;
        random_ = rngf() * 2.0f - 1.0f; // per-note S&H random mod source
        if(!doGlide)
        {
            for(int i = 0; i < kNumOsc; ++i)
                osc_[i].setPhase(rngf() * 0.5f);
        }
        for(int i = 0; i < kNumEnv; ++i)
            env_[i].gateOn();
    }

    void noteOff() noexcept
    {
        gateOn_ = false;
        for(int i = 0; i < kNumEnv; ++i)
            env_[i].gateOff();
    }

    // Steal: fast-release everything; the allocator will re-trigger next block.
    void steal() noexcept { noteOff(); }

    // Per-voice unison detune in cents (engine assigns when spreading a note).
    void setUnisonDetune(float cents) noexcept { detuneCents_ = cents / 100.0f; }
    void setPan(float pan) noexcept { basePan_ = pan; }

    // Configure from the patch once per block (cheap setup; the heavy per-sample
    // work reads the cached coefficients).
    void setPatch(const SynthPatch& p) noexcept { patch_ = &p; }

    // Render `n` samples, accumulating into stereo out. The VoiceMod carries this
    // block's resolved modulation. Returns false once the voice has gone idle so
    // the engine can free it.
    bool render(float* outL, float* outR, int n, const VoiceMod& m) noexcept
    {
        if(!active_ || patch_ == nullptr)
            return false;
        const SynthPatch& p = *patch_;

        // ---- per-block parameter setup ------------------------------------
        // glide coefficient
        const float glideCoef = (p.glideMode == (int)GlideMode::Off || p.glideTime < 0.001f)
                                    ? 0.0f
                                    : std::exp(-1.0f / (p.glideTime * blockRate_));

        // analog drift: a very slow, very shallow random walk per voice. Kept
        // tiny (max ~+/-2 cents at drift=1, and most patches use far less) so it
        // adds a touch of life WITHOUT reading as the voice slowly going out of
        // tune — the old +/-6 cents wandered audibly against the (static-pitch)
        // delay echoes and sounded like the synth was detuning over time.
        driftPhase_ += 0.25f / blockRate_; // ~0.25 Hz, slow
        if(driftPhase_ >= 1.0f)
        {
            driftPhase_ -= 1.0f;
            driftTarget_ = (rngf() * 2.0f - 1.0f);
        }
        driftVal_ += 0.03f * (driftTarget_ - driftVal_);
        const float driftSemi = driftVal_ * p.drift * 0.02f; // up to ~2 cents

        // envelope times
        for(int i = 0; i < kNumEnv; ++i)
        {
            env_[i].setAttack(p.env[i].attack);
            env_[i].setDecay(p.env[i].decay);
            env_[i].setSustain(p.env[i].sustain);
            env_[i].setRelease(p.env[i].release);
        }

        filter_.setMode((VoiceFilter::Mode)p.filterMode);

        // velocity -> amp depth per env0 setting
        const float ampVelo = 1.0f - p.env[0].velToLevel * (1.0f - vel_);

        // ---- per-block precompute (the expensive transcendentals live HERE, not
        // in the per-sample loop — pitch, detune, cutoff base and pan only change
        // at control rate, and computing exp2/exp/sqrt per sample per voice was
        // what overran the M7) ---------------------------------------------
        curHz_ = targetHz_ + (curHz_ - targetHz_) * glideCoef; // glide once/block
        const float pitchMul = std::exp2((m.pitchSemis + driftSemi + p.masterTune * 0.01f) * (1.0f / 12.0f));
        const float baseHz   = curHz_ * pitchMul;
        const float o1Hz = baseHz * footageMul(p.osc[0].footage) * semis(p.osc[0].detune);
        // detune spread: osc2 widens one way, osc3 the other (LFO -> living ensemble)
        const float o2Hz = baseHz * footageMul(p.osc[1].footage) * semis(p.osc[1].detune + m.osc2Semis + m.detuneAdd);
        const float o3Hz = baseHz * footageMul(p.osc[2].footage) * semis(p.osc[2].detune + m.osc3Semis - m.detuneAdd);
        osc_[0].setFrequency(o1Hz);
        osc_[1].setFrequency(o2Hz);
        osc_[2].setFrequency(o3Hz);
        osc_[0].setMorph(p.osc[0].morph + m.morphAdd[0]); osc_[0].setPulseWidth(p.osc[0].pw + m.pwAdd[0]);
        osc_[1].setMorph(p.osc[1].morph + m.morphAdd[1]); osc_[1].setPulseWidth(p.osc[1].pw + m.pwAdd[1]);
        osc_[2].setMorph(p.osc[2].morph + m.morphAdd[2]); osc_[2].setPulseWidth(p.osc[2].pw + m.pwAdd[2]);

        // extra oscillators (osc4..oscN): independent detuned layers summed by
        // level — stack a few slightly-detuned saws here for supersaw width. The
        // spread direction alternates so an LFO->Detune route fans them outward.
        for(int i = 3; i < kNumOsc; ++i)
        {
            const float sign = (i & 1) ? 1.0f : -1.0f;
            const float hz = baseHz * footageMul(p.osc[i].footage)
                             * semis(p.osc[i].detune + sign * m.detuneAdd);
            osc_[i].setFrequency(hz);
            osc_[i].setMorph(p.osc[i].morph + m.morphAdd[i]);
            osc_[i].setPulseWidth(p.osc[i].pw + m.pwAdd[i]);
            wt_[i].setFrequency(hz);
        }

        const bool  o0on = p.osc[0].on, o1on = p.osc[1].on, o2on = p.osc[2].on;
        const float fmAmt = clamp01(p.fm2to1 + m.fmAdd) * 2.0f; // env->FM = e-piano bark
        const float ringG = clamp01(p.ringMod + m.ringAdd);     // osc1 x osc2 metallic
        const float omix = clamp01(p.oscMix + m.oscMixAdd);
        const float g1 = (1.0f - omix) * p.osc[0].level;
        const float g2 = omix * p.osc[1].level;
        const float g3 = p.osc[2].level;
        const float subG   = clamp01(p.subLevel + m.subAdd);
        const float noiseG = clamp01(p.noiseLevel + m.noiseAdd);
        const bool  subOn  = subG > 0.0001f;
        if(subOn)
        {
            sub_.setFrequency(o1Hz * (p.subOctave >= 2 ? 0.25f : 0.5f));
            sub_.setMorph(1.0f);
            sub_.setPulseWidth(0.5f);
        }

        // wavetable oscillators run in parallel; oscType selects per oscillator.
        wt_[0].setFrequency(o1Hz); wt_[1].setFrequency(o2Hz); wt_[2].setFrequency(o3Hz);
        bool  wtO[kNumOsc];
        float crushLv[kNumOsc], srStep[kNumOsc];
        for(int i = 0; i < kNumOsc; ++i)
        {
            wt_[i].setTable((float) p.osc[i].wtTable + p.osc[i].wtMorph);
            wtO[i] = (p.osc[i].oscType == 1);
            // per-osc bit-crush: quantisation step count (0 => bypass). ~0.1 is a
            // gentle ~8-bit, 0.5 gritty ~5-bit, 1.0 a near-1-bit destroyed DCO.
            crushLv[i] = p.osc[i].crush > 0.001f
                             ? std::exp2(8.5f - p.osc[i].crush * 8.0f)
                             : 0.0f;
            // sample-rate reduction: decimation ratio (1 = full rate, down to ~SR/41)
            srStep[i] = p.osc[i].srReduce > 0.001f
                            ? 1.0f / (1.0f + p.osc[i].srReduce * 40.0f)
                            : 1.0f;
        }

        // filter: base cutoff (env-independent part) + key + matrix mod, in
        // octaves; the per-sample env contribution is applied in the loop.
        const float keyOffset  = ((float)note_ - 60.0f) / 12.0f * p.keyTrack;
        const float baseCutHz  = cutoffHz(p.cutoff) * std::exp2(m.cutoffOct + keyOffset);
        const float fenvDepth  = clamp01(p.envFilterAmt + m.envFltAdd) * 6.0f; // octaves per env unit
        const float res        = clamp01(p.resonance + m.resAdd);
        const float fdrive     = clamp01(p.filterDrive + m.driveAdd);

        // pan (equal-power), constant for the block
        float pan = basePan_ + p.pan + m.panAdd;
        pan = pan < -1.0f ? -1.0f : (pan > 1.0f ? 1.0f : pan);
        const float pr = 0.5f * (pan + 1.0f);
        const float gl = std::sqrt(1.0f - pr);
        const float gr = std::sqrt(pr);

        const float ampBase = p.ampGain * ampVelo * m.ampMul;

        // ---- per-sample render (cheap ops only) ---------------------------
        for(int s = 0; s < n; ++s)
        {
            const float ampEnv = env_[0].process();
            const float fltEnv = env_[1].process();
            env_[2].process(); // aux env runs as a mod source (read by engine)

            // filter cutoff tracks the filter envelope per sample. (On the M7 this
            // was throttled to every 16 samples to amortise the exp2; on desktop
            // the transcendental is free, and per-sample tracking removes the faint
            // stair-stepping otherwise audible on fast filter envelopes and on
            // S&H/LFO-to-cutoff at high depth.)
            {
                const float cutHz = baseCutHz * std::exp2(fenvDepth * fltEnv);
                filter_.setParams(cutHz, res, fdrive);
            }

            // sample-rate reduction (decimate) then bit-crush, per oscillator
            auto post = [&](int i, float x) -> float
            {
                if(srStep[i] < 0.999f)
                {
                    srPhase_[i] += srStep[i];
                    if(srPhase_[i] >= 1.0f) { srPhase_[i] -= 1.0f; srHold_[i] = x; }
                    x = srHold_[i];
                }
                if(crushLv[i] != 0.0f) x = std::round(x * crushLv[i]) / crushLv[i];
                return x;
            };

            // oscillators (+ osc2->osc1 cross-mod, hard sync). A wavetable osc
            // replaces the BLEP path for that oscillator; FM/sync apply to BLEP.
            bool  w0 = false, w1 = false, w2 = false;
            float o2 = !o1on ? 0.0f : (wtO[1] ? wt_[1].process() : osc_[1].process(0.0f, w1));
            float o1 = !o0on ? 0.0f : (wtO[0] ? wt_[0].process() : osc_[0].process(fmAmt * 0.5f * o2, w0));
            float o3 = !o2on ? 0.0f : (wtO[2] ? wt_[2].process() : osc_[2].process(0.0f, w2));
            if(w0)
            {
                if(p.sync2Mode == (int)SyncMode::Hard)      osc_[1].hardSync(0.0f);
                else if(p.sync2Mode == (int)SyncMode::Soft) osc_[1].softSync(0.0f);
                if(p.sync3Mode == (int)SyncMode::Hard)      osc_[2].hardSync(0.0f);
                else if(p.sync3Mode == (int)SyncMode::Soft) osc_[2].softSync(0.0f);
            }
            float subv = 0.0f;
            if(subOn) { bool ws; subv = sub_.process(0.0f, ws); }

            o1 = post(0, o1); o2 = post(1, o2); o3 = post(2, o3);

            float voice = o1 * g1 + o2 * g2 + o3 * g3
                          + subv * subG + (rngf() * 2.0f - 1.0f) * noiseG;
            for(int i = 3; i < kNumOsc; ++i)
                if(p.osc[i].on)
                {
                    bool wi; float oi = wtO[i] ? wt_[i].process() : osc_[i].process(0.0f, wi);
                    voice += post(i, oi) * p.osc[i].level;
                }
            // ring modulation: blend the osc1 x osc2 product in (bounded by 1, so
            // the master soft-clip never sees more than the dry oscs already give).
            if(ringG > 0.0001f)
                voice += (o1 * o2 - o1 * g1) * ringG; // crossfade osc1 -> ring product

            float fout = filter_.process(voice);

            float g = ampEnv * ampBase;
            ampSmooth_ += 0.25f * (g - ampSmooth_); // de-zipper block-rate gain
            fout *= ampSmooth_;

            outL[s] += fout * gl;
            outR[s] += fout * gr;
        }

        ++age_;
        if(!env_[0].active())
        {
            active_ = false;
            return false;
        }
        return true;
    }

    float auxEnvValue() const noexcept { return env_[2].value(); }

  private:
    static inline float clamp01(float v) noexcept
    {
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }
    static inline float semis(float s) noexcept
    {
        return std::exp2(s * (1.0f / 12.0f));
    }
    // 0..1 -> 20 Hz .. ~16 kHz, logarithmic
    static inline float cutoffHz(float v) noexcept
    {
        v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        return 20.0f * std::exp2(v * 9.6f); // 20 * 2^9.6 ~= 15.6 kHz
    }
    static inline float footageMul(int footage) noexcept
    {
        switch(footage)
        {
            case 0: return 0.25f; // 32'
            case 1: return 0.5f;  // 16'
            case 2: return 1.0f;  // 8'
            case 3: return 2.0f;  // 4'
            case 4: return 4.0f;  // 2'
            default: return 1.0f;
        }
    }
    inline float rngf() noexcept
    {
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        return (float)(rng_ >> 8) * (1.0f / 16777216.0f); // [0,1)
    }

    const SynthPatch* patch_ = nullptr;
    BlepOsc      osc_[kNumOsc];
    WavetableOsc wt_[kNumOsc];
    float        srPhase_[kNumOsc] = {0};  // sample-rate-reduction decimator phase
    float        srHold_[kNumOsc]  = {0};  // and held sample, per oscillator
    BlepOsc     sub_;
    VoiceFilter filter_;
    AdsrEnv     env_[kNumEnv];

    float    sr_        = 48000.0f;
    float    blockRate_ = 1000.0f;
    bool     active_    = false;
    bool     gateOn_    = false;
    int      note_      = 60;
    float    vel_       = 1.0f;
    float    detuneCents_ = 0.0f;
    float    basePan_   = 0.0f;
    float    curHz_     = 261.6f;
    float    targetHz_  = 261.6f;
    float    ampSmooth_ = 0.0f;
    float    random_    = 0.0f;    // per-note S&H random mod source
    uint32_t age_       = 0;

    // analog drift state
    float driftPhase_  = 0.0f;
    float driftVal_    = 0.0f;
    float driftTarget_ = 0.0f;

    uint32_t rng_ = 0x2545F491u;
};
} // namespace jove

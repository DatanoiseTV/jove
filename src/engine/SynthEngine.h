/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "Arpeggiator.h"
#include "Chorus.h"
#include "MultibandSat.h"
#include "SynthConfig.h"
#include "SynthDelay.h"
#include "SynthLfo.h"
#include "SynthParams.h"
#include "SynthPhaser.h"
#include "SynthReverb.h"
#include "SynthSeq.h"
#include "Voice.h"
#include <cstdint>

namespace jove
{
// The polyphonic synth engine: owns the voice pool, the voice allocator (POLY /
// MONO / UNISON), the global LFOs and the mod matrix, and renders a stereo block
// of the bare synth (pre-FX). MIDI note/controller events are fed in from the
// app's MIDI handler; the FX section consumes render()'s output downstream.
//
// Real-time discipline: noteOn/noteOff only touch lightweight allocator state
// (no allocation, no locks) so they are safe to call from the audio thread, and
// render() is allocation-free.
class SynthEngine
{
  public:
    void prepare(float sampleRate, int blockSize) noexcept;

    // The active patch (the UI / preset loader edits this in place; the engine
    // reads it each block). Held by pointer so a preset load can swap atomically.
    void setPatch(const SynthPatch* p) noexcept { patch_ = p; }
    const SynthPatch* patch() const noexcept { return patch_; }

    // ---- MIDI-ish events (call from the audio thread / MIDI handler) -------
    void noteOn(int note, float velocity) noexcept;
    void noteOff(int note) noexcept;
    void allNotesOff() noexcept;
    void onPresetLoaded() noexcept; // release orphaned (stuck) voices, keep held
    // Snapshot the currently-held keys (note + velocity) so the processor can
    // re-trigger them after a re-prepare (quality change) keeps sound going.
    int snapshotHeld(int* notes, int* vels, int maxN) const noexcept
    {
        int c = 0;
        for(int i = 0; i < heldCount_ && c < maxN; ++i) { notes[c] = held_[i]; vels[c] = heldVel_[i]; ++c; }
        return c;
    }
    void pitchBend(float norm) noexcept;     // -1..+1
    void modWheel(float norm) noexcept;      // 0..1
    void aftertouch(float norm) noexcept;    // 0..1
    void sustainPedal(bool down) noexcept;

    // ---- MPE (per-note expression on member channels 2..16) ----------------
    // Flip clears voices so a mode change can't orphan channel-tagged notes.
    void setMpe(bool on, int bendRangeSemis) noexcept
    {
        if(on != mpeOn_) allNotesOff();
        mpeOn_ = on;
        mpeBendRange_ = bendRangeSemis;
    }
    // master FX quick-bypass (global; default all on)
    void setFxEnabled(bool drive, bool chorus, bool phaser, bool delay, bool reverb) noexcept
    {
        fxDriveOn_ = drive; fxChorusOn_ = chorus; fxPhaserOn_ = phaser;
        fxDelayOn_ = delay; fxReverbOn_ = reverb;
    }
    void noteOnMpe(int channel, int note, float velocity) noexcept;
    void noteOffMpe(int channel, int note) noexcept;
    void channelBend(int channel, float norm) noexcept;      // -1..+1 -> +/- bendRange st
    void channelPressure(int channel, float norm) noexcept;  // 0..1
    void channelTimbre(int channel, float norm) noexcept;    // 0..1 (CC74)

    // External tempo for LFO/arp sync (BPM). 0 = use internal.
    void setTempo(float bpm) noexcept { tempoBpm_ = bpm > 1.0f ? bpm : 120.0f; }
    // Host transport: playing + song position (quarter-notes) at this block's
    // start. When playing, the arp locks its step grid to ppq; when stopped it
    // free-runs at tempo. Call once per processBlock before render().
    void setTransport(bool playing, double ppqQuarters) noexcept
    {
        transportPlaying_ = playing;
        transportPpq_     = ppqQuarters;
    }

    // Render one stereo block (pre-FX). Buffers are overwritten (not summed).
    void render(float* outL, float* outR, int n) noexcept;

    // Runtime polyphony cap (1..kMaxVoices). Global plugin setting (not stored in
    // the patch): the allocator only ever uses voices [0, maxVoices_), and UNISON
    // clamps its stack to it. Lowering it mid-note lets currently-ringing voices
    // finish; only new allocations honour the new cap.
    void setMaxVoices(int n) noexcept { maxVoices_ = n < 1 ? 1 : (n > kMaxVoices ? kMaxVoices : n); }
    int  maxVoices() const noexcept { return maxVoices_; }

    // ---- UI / metering -----------------------------------------------------
    int   activeVoices() const noexcept;
    // Per-voice amp-envelope level (0 if the voice is idle) for the voice LEDs.
    float voiceLevel(int i) const noexcept
    {
        return (i >= 0 && i < kMaxVoices && voice_[i].active()) ? voice_[i].ampEnvLevel() : 0.0f;
    }

    // Current value of a mod source (by ModSource index) for the live UI meters.
    // Global sources read directly; per-voice sources use the loudest active
    // voice as a representative so the meter tracks what you actually hear.
    float modSourceValue(int s) const noexcept
    {
        switch((ModSource) s)
        {
            case ModSource::Lfo1:       return lfo_[0].value();
            case ModSource::Lfo2:       return lfo_[1].value();
            case ModSource::Lfo3:       return lfo_[2].value();
            case ModSource::ModWheel:   return wheel_;
            case ModSource::Aftertouch: return at_;
            case ModSource::PitchBend:  return bend_;
            case ModSource::Seq1:       return seq_[0].value();
            case ModSource::Seq2:       return seq_[1].value();
            case ModSource::Seq3:       return seq_[2].value();
            case ModSource::Seq4:       return seq_[3].value();
            default: break;
        }
        int best = -1; float bestAmp = -1.0f;
        for(int i = 0; i < kMaxVoices; ++i)
            if(voice_[i].active() && voice_[i].ampEnvValue() > bestAmp)
            { bestAmp = voice_[i].ampEnvValue(); best = i; }
        if(best < 0) return 0.0f;
        const Voice& v = voice_[best];
        switch((ModSource) s)
        {
            case ModSource::EnvAmp:    return v.ampEnvValue();
            case ModSource::EnvFilter: return v.filterEnvValue();
            case ModSource::EnvAux:    return v.auxEnvValue();
            case ModSource::Velocity:  return v.velocity();
            case ModSource::KeyTrack:  return v.keyTrackSource();
            case ModSource::Note:      return v.noteSource();
            case ModSource::Random:    return v.randomSource();
            case ModSource::MpePressure: return v.mpePressureSource();
            case ModSource::MpeTimbre:   return v.mpeTimbreSource();
            case ModSource::MpeBend:     return v.mpeBendSource();
            default: return 0.0f;
        }
    }
    float lfoValue(int i) const noexcept { return (i >= 0 && i < kNumLfo) ? lfo_[i].value() : 0.0f; }
    float lfoPhase(int i) const noexcept { return (i >= 0 && i < kNumLfo) ? lfo_[i].phase() : 0.0f; }
    float seqValue(int i) const noexcept { return (i >= 0 && i < kNumSeq) ? seq_[i].value() : 0.0f; }
    int   seqStep(int i) const noexcept { return (i >= 0 && i < kNumSeq) ? seq_[i].stepIndex() : 0; }
    // Monotonic count of note triggers — bumps on every voice start (keyboard or
    // arp step). The UI uses the delta as an "arp pulse"; tests use it to confirm
    // the arp is actually stepping.
    uint32_t triggerCount() const noexcept { return noteTriggers_; }

    // direct note feed for the arpeggiator (Phase 6) so it can drive voices while
    // keeping its own held-note book separate from the live keyboard.
    Voice& voice(int i) noexcept { return voice_[i]; }

    // Re-align all bar-locked clocks (arp + tempo-synced LFOs/seqs) to phase 0.
    // Called by the processor on a transport stop->start edge so the arp/synced
    // mod land on the host's beat grid instead of free-running from the first note.
    void syncToBar() noexcept
    {
        arp_.syncToBar();
        for(int i = 0; i < kNumLfo; ++i)
            if(patch_ != nullptr && patch_->lfo[i].sync) lfo_[i].retrigger();
        for(int i = 0; i < kNumSeq; ++i)
            if(patch_ != nullptr && patch_->seq[i].sync) seq_[i].retrigger();
    }

  private:
    int  findFreeVoice() const noexcept;
    int  stealVoice() const noexcept;
    void startVoice(int idx, int note, float velocity, bool glide, float detuneCents,
                    float pan, float glideFromHz) noexcept;
    void evalSlots(const ModSlot* slots, int count, const float* src, VoiceMod& m) noexcept;
    void evalMatrix(const float* src, VoiceMod& m) noexcept;
    void buildVoiceMod(int voiceIdx, VoiceMod& m) noexcept;
    // shared note allocation (POLY/MONO/UNISON) used by both the live keyboard
    // and the arpeggiator's step events.
    void playNote(int note, float velocity) noexcept;
    void releaseNote(int note) noexcept;
    void serviceArp(int n) noexcept;

    const SynthPatch* patch_ = nullptr;
    Voice             voice_[kMaxVoices];
    SynthLfo          lfo_[kNumLfo];
    Sequencer         seq_[kNumSeq];      // step/curve modulation sequencers
    Arpeggiator       arp_;
    MultibandSat      mbsat_;             // 3-band saturation on the filtered bus
    Chorus            chorus_;            // Juno-style ensemble on the voice bus
    StereoPhaser      phaser_;            // 6-stage all-pass phaser (ported from Doobie)
    SynthDelay        delay_;             // clean tempo-synced delay (no pitch mod)
    SynthReverb       reverb_;            // clean Freeverb-style reverb (no mod)
    float             lastNoteHz_ = 0.0f; // for poly portamento (glide-from pitch)
    bool              arpWasOn_   = false;
    uint32_t          noteTriggers_ = 0;
    // output DC-blocker state (one-pole high-pass per channel)
    float             dcxL_ = 0.0f, dcyL_ = 0.0f, dcxR_ = 0.0f, dcyR_ = 0.0f;
    float             mdcxL_ = 0.0f, mdcyL_ = 0.0f, mdcxR_ = 0.0f, mdcyR_ = 0.0f; // master DC blocker
    float             toneZL_ = 0.0f, toneZR_ = 0.0f; // drive-tone tilt LP state
    float             limEnv_ = 0.0f; // master limiter peak-envelope follower
    float             busEnv_ = 0.0f; // voice-bus auto-gain follower (anti-chord-clip)

    float sr_        = 48000.0f;
    int   blockSize_ = 48;
    float blockRate_ = 1000.0f;
    float tempoBpm_  = 120.0f;
    bool   transportPlaying_ = false; // host transport running (arp ppq-locks)
    double transportPpq_     = 0.0;   // host song position in quarter-notes

    // controllers
    float bend_      = 0.0f; // -1..+1
    float wheel_     = 0.0f; // 0..1
    float at_        = 0.0f; // 0..1
    bool  sustain_   = false;
    bool  mpeOn_        = false; // MPE per-note expression mode
    int   mpeBendRange_ = 48;    // per-note bend range (semitones)
    bool  fxDriveOn_  = true, fxChorusOn_ = true, fxPhaserOn_ = true;
    bool  fxDelayOn_  = true, fxReverbOn_ = true; // master FX quick-bypass

    // held-note book for MONO last-note priority (and a base for the arp).
    static constexpr int kMaxHeld = 16;
    int  held_[kMaxHeld];
    int  heldVel_[kMaxHeld];
    int  heldCount_ = 0;
    void pushHeld(int note, int vel) noexcept;
    void removeHeld(int note) noexcept;

    uint32_t voiceClock_ = 0; // monotonic age stamp for stealing
    uint32_t startStamp_[kMaxVoices] = {0};
    int      maxVoices_ = kMaxVoices; // runtime polyphony cap
};
} // namespace jove

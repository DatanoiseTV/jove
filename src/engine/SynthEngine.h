/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "Arpeggiator.h"
#include "Chorus.h"
#include "SynthConfig.h"
#include "SynthDelay.h"
#include "SynthLfo.h"
#include "SynthParams.h"
#include "SynthPhaser.h"
#include "SynthReverb.h"
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
    void pitchBend(float norm) noexcept;     // -1..+1
    void modWheel(float norm) noexcept;      // 0..1
    void aftertouch(float norm) noexcept;    // 0..1
    void sustainPedal(bool down) noexcept;

    // External tempo for LFO/arp sync (BPM). 0 = use internal.
    void setTempo(float bpm) noexcept { tempoBpm_ = bpm > 1.0f ? bpm : 120.0f; }

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
    float lfoValue(int i) const noexcept { return (i >= 0 && i < kNumLfo) ? lfo_[i].value() : 0.0f; }
    float lfoPhase(int i) const noexcept { return (i >= 0 && i < kNumLfo) ? lfo_[i].phase() : 0.0f; }
    // Monotonic count of note triggers — bumps on every voice start (keyboard or
    // arp step). The UI uses the delta as an "arp pulse"; tests use it to confirm
    // the arp is actually stepping.
    uint32_t triggerCount() const noexcept { return noteTriggers_; }

    // direct note feed for the arpeggiator (Phase 6) so it can drive voices while
    // keeping its own held-note book separate from the live keyboard.
    Voice& voice(int i) noexcept { return voice_[i]; }

  private:
    int  findFreeVoice() const noexcept;
    int  stealVoice() const noexcept;
    void startVoice(int idx, int note, float velocity, bool glide, float detuneCents,
                    float pan, float glideFromHz) noexcept;
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
    Arpeggiator       arp_;
    Chorus            chorus_;            // Juno-style ensemble on the voice bus
    StereoPhaser      phaser_;            // 6-stage all-pass phaser (ported from Doobie)
    SynthDelay        delay_;             // clean tempo-synced delay (no pitch mod)
    SynthReverb       reverb_;            // clean Freeverb-style reverb (no mod)
    float             lastNoteHz_ = 0.0f; // for poly portamento (glide-from pitch)
    bool              arpWasOn_   = false;
    uint32_t          noteTriggers_ = 0;
    // output DC-blocker state (one-pole high-pass per channel)
    float             dcxL_ = 0.0f, dcyL_ = 0.0f, dcxR_ = 0.0f, dcyR_ = 0.0f;

    float sr_        = 48000.0f;
    int   blockSize_ = 48;
    float blockRate_ = 1000.0f;
    float tempoBpm_  = 120.0f;

    // controllers
    float bend_      = 0.0f; // -1..+1
    float wheel_     = 0.0f; // 0..1
    float at_        = 0.0f; // 0..1
    bool  sustain_   = false;

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

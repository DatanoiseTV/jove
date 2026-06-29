/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#include "SynthEngine.h"

namespace jove
{
void SynthEngine::prepare(float sampleRate, int blockSize) noexcept
{
    sr_        = sampleRate;
    blockSize_ = blockSize;
    blockRate_ = sampleRate / (float)blockSize;
    for(int i = 0; i < kMaxVoices; ++i)
        voice_[i].prepare(sampleRate, blockRate_);
    for(int i = 0; i < kNumLfo; ++i)
        lfo_[i].prepare(sampleRate, blockRate_);
    arp_.prepare(sampleRate);
    mbsat_.prepare(sampleRate);
    chorus_.prepare(sampleRate);
    phaser_.prepare((float)sampleRate);
    delay_.prepare(sampleRate);
    reverb_.prepare(sampleRate);
    heldCount_  = 0;
    voiceClock_ = 0;
}

// ---------------------------------------------------------------------------
// held-note book
// ---------------------------------------------------------------------------
void SynthEngine::pushHeld(int note, int vel) noexcept
{
    // de-dup
    for(int i = 0; i < heldCount_; ++i)
        if(held_[i] == note)
        {
            heldVel_[i] = vel;
            return;
        }
    if(heldCount_ < kMaxHeld)
    {
        held_[heldCount_]    = note;
        heldVel_[heldCount_] = vel;
        ++heldCount_;
    }
    else // drop the oldest
    {
        for(int i = 1; i < kMaxHeld; ++i)
        {
            held_[i - 1]    = held_[i];
            heldVel_[i - 1] = heldVel_[i];
        }
        held_[kMaxHeld - 1]    = note;
        heldVel_[kMaxHeld - 1] = vel;
    }
}

void SynthEngine::removeHeld(int note) noexcept
{
    int w = 0;
    for(int i = 0; i < heldCount_; ++i)
    {
        if(held_[i] != note)
        {
            held_[w]    = held_[i];
            heldVel_[w] = heldVel_[i];
            ++w;
        }
    }
    heldCount_ = w;
}

// ---------------------------------------------------------------------------
// allocation
// ---------------------------------------------------------------------------
int SynthEngine::findFreeVoice() const noexcept
{
    for(int i = 0; i < maxVoices_; ++i)
        if(!voice_[i].active())
            return i;
    return -1;
}

int SynthEngine::stealVoice() const noexcept
{
    // Prefer the oldest released (gate-off) voice; else the oldest of all.
    int best = -1;
    uint32_t bestStamp = 0xFFFFFFFFu;
    bool foundReleased = false;
    for(int i = 0; i < maxVoices_; ++i)
    {
        const bool released = !voice_[i].gateOn();
        if(released && !foundReleased)
        {
            foundReleased = true;
            best = i;
            bestStamp = startStamp_[i];
        }
        else if(released == foundReleased && startStamp_[i] < bestStamp)
        {
            best = i;
            bestStamp = startStamp_[i];
        }
    }
    return best < 0 ? 0 : best;
}

void SynthEngine::startVoice(int idx, int note, float velocity, bool glide,
                             float detuneCents, float pan, float glideFromHz) noexcept
{
    if(idx < 0 || idx >= kMaxVoices || patch_ == nullptr)
        return;
    voice_[idx].setPatch(*patch_);
    voice_[idx].setUnisonDetune(detuneCents);
    voice_[idx].setPan(pan);
    voice_[idx].noteOn(note, velocity, *patch_, glide, glideFromHz);
    startStamp_[idx] = ++voiceClock_;
}

// ---------------------------------------------------------------------------
// note events
// ---------------------------------------------------------------------------
// Public keyboard note-on: when the arp is engaged the keys feed the arp's held
// set (it triggers the voices on its clock); otherwise play directly.
void SynthEngine::noteOn(int note, float velocity) noexcept
{
    if(patch_ == nullptr)
        return;
    if(patch_->arp.on)
    {
        arp_.notePressed();
        arp_.keyOn(note, (int)(velocity * 127.0f), patch_->arp.latch);
        return;
    }
    playNote(note, velocity);
}

void SynthEngine::noteOff(int note) noexcept
{
    if(patch_ == nullptr)
        return;
    if(patch_->arp.on)
    {
        arp_.keyOff(note, patch_->arp.latch);
        return;
    }
    releaseNote(note);
}

// Shared allocation: POLY (with poly portamento), MONO (last-note priority,
// legato glide) and UNISON (detuned stack). Used by both the keyboard and the
// arpeggiator's step events.
void SynthEngine::playNote(int note, float velocity) noexcept
{
    ++noteTriggers_;
    // Retrigger LFOs (fade/delay) on a fresh note start — the first note of a
    // phrase or a detached note — so delayed vibrato restarts each time, but a
    // held legato/chord keeps the wobble going.
    if(activeVoices() == 0)
        for(int i = 0; i < kNumLfo; ++i)
            if(patch_->lfo[i].retrig)
                lfo_[i].retrigger(); // phase positioning is the continuous setPhase() offset
    pushHeld(note, (int)(velocity * 127.0f));
    const int   mode = patch_->voiceMode;
    const float newHz = midiToHz((float)note);

    if(mode == (int)VoiceMode::Mono)
    {
        // Single voice, last-note priority; legato glide when a note was held.
        const bool glide = heldCount_ > 1;
        startVoice(0, note, velocity, glide, 0.0f, 0.0f, -1.0f);
        lastNoteHz_ = newHz;
        return;
    }

    if(mode == (int)VoiceMode::Unison)
    {
        int n = patch_->unisonCount < 1 ? 1 : (patch_->unisonCount > kMaxUnison ? kMaxUnison : patch_->unisonCount);
        if(n > maxVoices_) n = maxVoices_; // respect the global polyphony cap
        const float det = patch_->unisonDetune;
        const float spr = patch_->unisonSpread;
        const bool  glide = lastNoteHz_ > 0.0f;
        for(int v = 0; v < n; ++v)
        {
            const float frac  = (n == 1) ? 0.0f : ((float)v / (float)(n - 1) - 0.5f) * 2.0f;
            const float cents = frac * det * 25.0f; // up to +/-25 cents at det=1
            const float pan   = frac * spr;
            startVoice(v, note, velocity, glide, cents, pan, lastNoteHz_);
        }
        lastNoteHz_ = newHz;
        return;
    }

    // POLY: allocate a voice; poly portamento glides it from the last note.
    int idx = findFreeVoice();
    if(idx < 0)
        idx = stealVoice();
    const bool glide = patch_->glideMode != (int)GlideMode::Off && lastNoteHz_ > 0.0f
                       && (patch_->glideMode != (int)GlideMode::Legato || heldCount_ > 1);
    startVoice(idx, note, velocity, glide, 0.0f, 0.0f, lastNoteHz_);
    lastNoteHz_ = newHz;
}

void SynthEngine::releaseNote(int note) noexcept
{
    removeHeld(note);

    if(sustain_)
        return; // held by pedal; release handled on pedal-up

    // Catch-all: gate off EVERY sounding voice playing this note, regardless of
    // the current voice mode. A voice may have been started under a different
    // mode (e.g. before a preset change switched Poly->Mono); a mode-specific
    // release alone would miss it and the note would hang. This loop runs in all
    // modes so a note-off always reaches the voice that played it.
    for(int i = 0; i < kMaxVoices; ++i)
        if(voice_[i].active() && voice_[i].gateOn() && voice_[i].note() == note)
            voice_[i].noteOff();

    // Mono legato: if keys are still held, fall back to the most-recent one.
    if(patch_->voiceMode == (int)VoiceMode::Mono && heldCount_ > 0)
    {
        const int n = held_[heldCount_ - 1];
        startVoice(0, n, (float)heldVel_[heldCount_ - 1] / 127.0f, true, 0.0f, 0.0f, -1.0f);
        lastNoteHz_ = midiToHz((float)n);
    }
}

void SynthEngine::allNotesOff() noexcept
{
    heldCount_ = 0;
    arp_.allOff();
    for(int i = 0; i < kMaxVoices; ++i)
        voice_[i].noteOff();
}

// Called once after a preset loads. Held notes keep sounding so a patch can be
// auditioned on a sustained note, but any orphaned voice — still gated yet no
// longer in the held-key set (a stuck note whose note-off was missed or routed
// to the wrong voice under a previous mode) — is released so it can't hang. The
// arp owns its own note lifetimes, so skip the sweep while it's running.
void SynthEngine::onPresetLoaded() noexcept
{
    if(patch_ != nullptr && patch_->arp.on)
        return;
    for(int i = 0; i < kMaxVoices; ++i)
    {
        if(!voice_[i].active() || !voice_[i].gateOn())
            continue;
        bool stillHeld = false;
        for(int h = 0; h < heldCount_; ++h)
            if(held_[h] == voice_[i].note())
                stillHeld = true;
        if(!stillHeld)
            voice_[i].noteOff();
    }
}

// Advance the arpeggiator one control block and apply its step events.
void SynthEngine::serviceArp(int n) noexcept
{
    if(patch_ == nullptr)
        return;
    const bool on = patch_->arp.on;
    if(!on)
    {
        if(arpWasOn_) // arp just turned off: drop any notes it was holding
        {
            arp_.allOff();
            allNotesOff();
            arpWasOn_ = false;
        }
        return;
    }
    arpWasOn_ = true;
    ArpEvent ev = arp_.process(n, tempoBpm_, patch_->arp);
    for(int i = 0; i < ev.offCount; ++i)
        releaseNote(ev.offNotes[i]);
    for(int i = 0; i < ev.onCount; ++i)
        playNote(ev.onNotes[i], (float)ev.onVels[i] / 127.0f);
}

void SynthEngine::pitchBend(float norm) noexcept { bend_ = norm; }
void SynthEngine::modWheel(float norm) noexcept { wheel_ = norm; }
void SynthEngine::aftertouch(float norm) noexcept { at_ = norm; }

void SynthEngine::sustainPedal(bool down) noexcept
{
    if(sustain_ && !down)
    {
        // pedal released: drop voices whose key is no longer held
        for(int i = 0; i < kMaxVoices; ++i)
        {
            if(voice_[i].active() && voice_[i].gateOn())
            {
                bool stillHeld = false;
                for(int h = 0; h < heldCount_; ++h)
                    if(held_[h] == voice_[i].note())
                        stillHeld = true;
                if(!stillHeld)
                    voice_[i].noteOff();
            }
        }
    }
    sustain_ = down;
}

// ---------------------------------------------------------------------------
// modulation
// ---------------------------------------------------------------------------
// Evaluate the full mod matrix for one voice given its complete source array,
// accumulating into the VoiceMod. Pure function of (slots, src) so the same
// routing logic serves every voice.
void SynthEngine::evalMatrix(const float* src, VoiceMod& m) noexcept
{
    const SynthPatch& p = *patch_;
    // pitch bend (range in semitones) applies to all voices as a base offset
    m.pitchSemis += bend_ * (float)p.bendRange;

    for(int i = 0; i < kNumModSlots; ++i)
    {
        const ModSlot& sl = p.mod[i];
        if(sl.source <= 0 || sl.dest <= 0 || sl.amount == 0.0f)
            continue;
        if(sl.source >= (int)ModSource::Count || sl.dest >= (int)ModDest::Count)
            continue; // guard against out-of-range indices from presets/web/UI
        const float v = src[sl.source] * sl.amount;
        switch((ModDest)sl.dest)
        {
            case ModDest::Pitch: m.pitchSemis += v * 12.0f; break;
            case ModDest::Osc2Pitch: m.osc2Semis += v * 12.0f; break;
            case ModDest::Osc3Pitch: m.osc3Semis += v * 12.0f; break;
            case ModDest::Morph1: m.morphAdd[0] += v; break;
            case ModDest::Morph2: m.morphAdd[1] += v; break;
            case ModDest::Morph3: m.morphAdd[2] += v; break;
            case ModDest::Pw1: m.pwAdd[0] += v * 0.5f; break;
            case ModDest::Pw2: m.pwAdd[1] += v * 0.5f; break;
            case ModDest::Pw3: m.pwAdd[2] += v * 0.5f; break;
            case ModDest::Cutoff: m.cutoffOct += v * 4.0f; break; // +/- 4 oct
            case ModDest::Resonance: m.resAdd += v; break;
            case ModDest::Amp: m.ampMul *= (1.0f + v); break;
            case ModDest::Pan: m.panAdd += v; break;
            case ModDest::SubLevel: m.subAdd += v; break;
            case ModDest::NoiseLevel: m.noiseAdd += v; break;
            case ModDest::FmAmount: m.fmAdd += v; break;
            case ModDest::RingMod: m.ringAdd += v; break;
            case ModDest::OscMix: m.oscMixAdd += v; break;
            case ModDest::FilterDrive: m.driveAdd += v; break;
            case ModDest::EnvFltAmt: m.envFltAdd += v; break;
            // Detune is in semitones; scale so a mod amount of 1.0 is ~15 cents of
            // spread (ensemble shimmer), not a whole-semitone warble.
            case ModDest::Detune: m.detuneAdd += v * 0.15f; break;
            // LFO-rate/depth and FX destinations are applied at the engine/FX
            // layer in later phases; ignored here.
            default: break;
        }
    }
}

// Assemble one voice's complete mod-source array (global LFOs/controllers +
// that voice's per-voice envelopes, velocity, keytrack and random) and evaluate
// the matrix into `m`.
void SynthEngine::buildVoiceMod(int voiceIdx, VoiceMod& m) noexcept
{
    m = VoiceMod{};
    const Voice& v = voice_[voiceIdx];
    float src[(int)ModSource::Count];
    src[(int)ModSource::Off]        = 0.0f;
    src[(int)ModSource::Lfo1]       = lfo_[0].value();
    src[(int)ModSource::Lfo2]       = lfo_[1].value();
    src[(int)ModSource::Lfo3]       = lfo_[2].value();
    src[(int)ModSource::EnvAmp]     = v.ampEnvValue();
    src[(int)ModSource::EnvFilter]  = v.filterEnvValue();
    src[(int)ModSource::EnvAux]     = v.auxEnvValue();
    src[(int)ModSource::Velocity]   = v.velocity();
    src[(int)ModSource::KeyTrack]   = v.keyTrackSource();
    src[(int)ModSource::ModWheel]   = wheel_;
    src[(int)ModSource::Aftertouch] = at_;
    src[(int)ModSource::PitchBend]  = bend_;
    src[(int)ModSource::Random]     = v.randomSource();
    src[(int)ModSource::Note]       = v.noteSource();
    evalMatrix(src, m);
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------
void SynthEngine::render(float* outL, float* outR, int n) noexcept
{
    for(int i = 0; i < n; ++i)
    {
        outL[i] = 0.0f;
        outR[i] = 0.0f;
    }
    if(patch_ == nullptr)
        return;
    const SynthPatch& p = *patch_;

    // arpeggiator: advance its clock and trigger/release voices for this block
    serviceArp(n);

    // value of a GLOBAL mod source (the only kind that can drive a shared LFO's
    // depth/rate); per-voice sources return 0 here.
    auto globalSrc = [&](int s) -> float {
        switch((ModSource)s)
        {
            case ModSource::Lfo1: return lfo_[0].value();
            case ModSource::Lfo2: return lfo_[1].value();
            case ModSource::Lfo3: return lfo_[2].value();
            case ModSource::ModWheel: return wheel_;
            case ModSource::Aftertouch: return at_;
            case ModSource::PitchBend: return bend_;
            default: return 0.0f;
        }
    };

    // advance global LFOs for this block (rate free in Hz or locked to tempo).
    // LFO depth/rate can be modulated by the wheel/AT/bend (e.g. mod-wheel
    // vibrato) — those destinations affect the shared LFO, so they're resolved
    // here, not in the per-voice matrix.
    for(int i = 0; i < kNumLfo; ++i)
    {
        const LfoParams& lp = p.lfo[i];
        float rateHz = lp.rate;
        if(lp.sync)
        {
            // division index -> Hz from tempo: one cycle per (quarter * div)
            int di = lp.syncDiv;
            if(di < 0) di = 0;
            if(di >= kNumArpDiv) di = kNumArpDiv - 1;
            const double secPerQuarter = 60.0 / (tempoBpm_ < 1.0f ? 120.0 : tempoBpm_);
            const double cycleSec       = secPerQuarter * kArpDivQuarters[di];
            rateHz = (cycleSec > 1e-6) ? (float)(1.0 / cycleSec) : lp.rate;
        }
        // scan for wheel/AT/etc -> this LFO's depth or rate
        float depthMod = 0.0f, rateMod = 0.0f;
        const int dDepth = (int)ModDest::Lfo1Depth + i;
        const int dRate  = (int)ModDest::Lfo1Rate + i;
        for(int s = 0; s < kNumModSlots; ++s)
        {
            const ModSlot& sl = p.mod[s];
            if(sl.source <= 0 || sl.amount == 0.0f)
                continue;
            if(sl.dest == dDepth) depthMod += globalSrc(sl.source) * sl.amount;
            else if(sl.dest == dRate) rateMod += globalSrc(sl.source) * sl.amount;
        }
        float depth = lp.depth + depthMod;
        if(depth < 0.0f) depth = 0.0f;
        if(depth > 2.0f) depth = 2.0f;
        rateHz *= (1.0f + rateMod);
        lfo_[i].setWave(lp.wave);
        lfo_[i].setRate(rateHz);
        lfo_[i].setDepth(depth);
        lfo_[i].setFade(lp.fade);
        lfo_[i].setDelay(lp.delay);
        lfo_[i].setOffset(lp.offset);
        lfo_[i].setPhase(lp.phase);
        lfo_[i].advance();
    }

    // Render each voice with its own resolved modulation (global LFOs/controllers
    // + that voice's velocity, keytrack, envelopes and random source).
    for(int i = 0; i < kMaxVoices; ++i)
    {
        if(!voice_[i].active())
            continue;
        VoiceMod vm;
        buildVoiceMod(i, vm);
        voice_[i].setPatch(p);
        voice_[i].render(outL, outR, n, vm);
    }

    // Master soft-clip on the voice bus: near-transparent for one or two voices
    // (a single voice peaks ~0.35), but smoothly bounds a stacked chord or a
    // unison/voice-steal transient under full scale instead of letting it clip
    // hard into the FX chain. This is the analog-console "glue" a real polysynth
    // mix bus has, and it keeps the artifact gate (peak < 1.0) satisfied.
    // 3-band saturation on the filtered bus (band-targeted grit), then the glue clip
    mbsat_.setDrive(p.mbLow, p.mbMid, p.mbHigh);
    const bool mbActive = mbsat_.active();
    for(int i = 0; i < n; ++i)
    {
        float l = outL[i], r = outR[i];
        // Voice-bus auto-gain: smoothly pull a hot stack down toward the glue
        // ceiling BEFORE the saturator, so a held chord (N voices summing well
        // past 1.0) is level-controlled instead of slammed into softSat — that
        // hard clip on chords was the audible "clipping" across presets. Fast
        // attack, slow release; single notes (peak ~0.35) pass untouched.
        const float bpk = std::max(std::fabs(l), std::fabs(r));
        if(bpk > busEnv_) busEnv_ += 0.30f * (bpk - busEnv_);
        else              busEnv_ += 0.001f * (bpk - busEnv_);
        constexpr float busCeil = 0.90f;
        const float bg = busEnv_ > busCeil ? busCeil / busEnv_ : 1.0f;
        l *= bg; r *= bg;
        if(mbActive) { l = mbsat_.process(0, l); r = mbsat_.process(1, r); }
        outL[i] = softSat(l);
        outR[i] = softSat(r);
    }

    // Juno-style ensemble chorus on the voice bus (true-bypass at mix 0). A fixed
    // musical rate/depth; the patch's one FX-chorus macro sets the mix.
    chorus_.setMode(p.chorusMode);
    chorus_.setRate(p.chorusRate);
    chorus_.setDepth(p.chorusDepth);
    chorus_.setMix(p.fxChorus);
    chorus_.process(outL, outR, n);

    // 6-stage all-pass phaser on the voice bus (true-bypass at mix 0). Sits after
    // the chorus so the notches sweep the ensemble-widened signal. Rate/depth/
    // feedback are patch-controllable for anything from a slow sweep to a fast,
    // resonant whoosh.
    phaser_.setParams(p.phaserRate, p.phaserDepth, p.phaserFeedback);
    phaser_.setMix(p.fxPhaser);
    phaser_.process(outL, outR, n);

    // DC blocker on the voice bus. The master soft-clip is an odd nonlinearity,
    // and an odd nonlinearity on a zero-mean but asymmetric waveform (a sawtooth)
    // leaves a small DC offset — which the delay feedback would otherwise
    // accumulate. A one-pole high-pass at ~5 Hz removes it cleanly.
    for(int i = 0; i < n; ++i)
    {
        const float xl = outL[i];
        dcyL_ = xl - dcxL_ + 0.9993f * dcyL_;
        dcxL_ = xl;
        outL[i] = dcyL_;
        const float xr = outR[i];
        dcyR_ = xr - dcxR_ + 0.9993f * dcyR_;
        dcxR_ = xr;
        outR[i] = dcyR_;
    }

    // ---- clean FX: drive -> delay -> reverb -----------------------------
    // Soft drive (fxDrive), then a tempo-synced clean delay and a pitch-stable
    // reverb. No tape, no wow/flutter, no shimmer — none of the modulation that
    // made the old chain warble out of tune.
    if(p.fxDrive > 0.001f || std::fabs(p.driveTone) > 0.001f)
    {
        const float g = 1.0f + p.fxDrive * 4.0f;
        const float tone = p.driveTone; // -1 dark .. +1 bright, tilt around a LP
        for(int i = 0; i < n; ++i)
        {
            toneZL_ += 0.25f * (outL[i] - toneZL_);
            toneZR_ += 0.25f * (outR[i] - toneZR_);
            outL[i] = softSat((outL[i] + tone * (outL[i] - toneZL_)) * g);
            outR[i] = softSat((outR[i] + tone * (outR[i] - toneZR_)) * g);
        }
    }

    // delay time: tempo-synced division, or a free time in ms.
    const float secPerQuarter = 60.0f / (tempoBpm_ < 1.0f ? 120.0f : tempoBpm_);
    float delaySec;
    if(p.delaySync)
    {
        int di = p.delayDiv;
        if(di < 0) di = 0;
        if(di >= kNumArpDiv) di = kNumArpDiv - 1;
        delaySec = (float)(secPerQuarter * kArpDivQuarters[di]);
    }
    else
    {
        float ms = p.delayTimeMs < 20.0f ? 20.0f : (p.delayTimeMs > 2000.0f ? 2000.0f : p.delayTimeMs);
        delaySec = ms * 0.001f;
    }
    const float dS = delaySec * sr_;
    delay_.setTime(dS, dS);
    delay_.setPingPong(p.delayPing);
    delay_.setFeedback(p.delayFeedback);
    delay_.setDamp(p.delayTone);
    delay_.setFilter(p.delayFltType, p.delayFltFreq, p.delayFltQ);
    delay_.setMix(p.fxDelay);
    delay_.process(outL, outR, n);

    reverb_.setSize(p.reverbSize);
    reverb_.setDamp(p.reverbTone);
    reverb_.setMix(p.fxReverb);
    reverb_.process(outL, outR, n);

    // master stereo width (mid/side): 0 = mono-summed, 0.5 = unchanged, 1 = wide.
    // Skip the work at the neutral default. Widening boosts the side signal, which
    // on hard-panned/stereo content (ensemble, ping-pong delay) can push a channel
    // past the voice-bus soft-clip — so the final master soft-clip below, applied
    // AFTER width, is what actually guarantees the DAC feed stays in [-1, 1].
    if(p.width < 0.49f || p.width > 0.51f)
    {
        const float sideGain = p.width * 2.0f; // 0..2 (1 = neutral)
        for(int i = 0; i < n; ++i)
        {
            const float mid  = 0.5f * (outL[i] + outR[i]);
            const float side = 0.5f * (outL[i] - outR[i]) * sideGain;
            outL[i] = mid + side;
            outR[i] = mid - side;
        }
    }

    // Master limiter: a transparent peak limiter (fast attack, slow release)
    // pulls loud material under the ceiling so it never reaches the clipper, then
    // a gentle soft-clip guarantees the converter feed stays in range.
    constexpr float ceiling = 0.92f;
    for(int i = 0; i < n; ++i)
    {
        const float peak = std::max(std::fabs(outL[i]), std::fabs(outR[i]));
        if(peak > limEnv_) limEnv_ += 0.5f * (peak - limEnv_);      // fast attack
        else               limEnv_ += 0.0006f * (peak - limEnv_);  // slow release (~30 ms)
        const float gr = limEnv_ > ceiling ? ceiling / limEnv_ : 1.0f;
        outL[i] = softSat(outL[i] * gr);
        outR[i] = softSat(outR[i] * gr);
    }
}

int SynthEngine::activeVoices() const noexcept
{
    int c = 0;
    for(int i = 0; i < kMaxVoices; ++i)
        if(voice_[i].active())
            ++c;
    return c;
}
} // namespace jove

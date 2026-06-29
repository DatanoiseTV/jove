/*
  Jove — analog-inspired polysynth  (Keinedelay/DFM hardware)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "SynthParams.h"
#include <cstdint>

namespace jove
{
// Tempo divisions for the arp (and synced LFOs), in quarter-note multiples,
// fastest..slowest. Index is the stored contract with kArpDivNames.
inline constexpr double kArpDivQuarters[] = {
    0.0625, 0.083333, 0.125, 0.166667, 0.25, 0.333333, 0.375,
    0.5, 0.666667, 0.75, 1.0, 1.5, 2.0};
inline constexpr const char* kArpDivNames[] = {
    "1/64", "1/32T", "1/32", "1/16T", "1/16", "1/8T", "1/16.",
    "1/8", "1/4T", "1/8.", "1/4", "1/4.", "1/2"};
inline constexpr int kNumArpDiv = 13;

// Step events the arp emits for the engine to act on this block. At a 1 ms
// control block and a step rate well under that, at most one step boundary lands
// per block. CHORD mode plays the whole held set per step, so the event carries
// note lists rather than a single note.
inline constexpr int kArpMaxStepNotes = 8;
struct ArpEvent
{
    int offNotes[kArpMaxStepNotes];
    int offCount = 0;
    int onNotes[kArpMaxStepNotes];
    int onVels[kArpMaxStepNotes];
    int onCount = 0;
};

// Pattern arpeggiator.
//
// Owns its own held-note book (so latch can outlive the physical keys), builds
// the ordered step list for the selected pattern across the octave range, and
// clocks through it locked to tempo with per-step swing, gate length and
// ratchet. The engine forwards keyboard note-on/off here when arp.on, then asks
// process() each block for the note events to play.
class Arpeggiator
{
  public:
    void prepare(float sampleRate) noexcept
    {
        sr_ = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        heldCount_  = 0;
        seqLen_     = 0;
        seqPos_     = 0;
        phase_      = 0.0f;
        soundingCount_ = 0;
        ratchetLeft_ = 0;
        running_    = false;
        bounceDir_  = 1;
    }

    // ---- keyboard feed -----------------------------------------------------
    void keyOn(int note, int vel, bool latch) noexcept
    {
        if(latch && !anyPhysicalDown_)
        {
            // first new key of a fresh latch chord -> clear the held latch set
            heldCount_ = 0;
        }
        anyPhysicalDown_ = true;
        addHeld(note, vel);
        rebuild_ = true;
        if(!running_)
        {
            // start on the next block; align phase so the first step fires at once
            running_ = true;
            phase_   = 1.0f; // force an immediate step boundary
            seqPos_  = -1;
        }
    }

    void keyOff(int note, bool latch) noexcept
    {
        physicalDown_--;
        if(physicalDown_ < 0)
            physicalDown_ = 0;
        anyPhysicalDown_ = physicalDown_ > 0;
        if(!latch)
        {
            removeHeld(note);
            rebuild_ = true;
            if(heldCount_ == 0)
                running_ = false;
        }
        // latch: keep the note in the set until a new chord starts
    }

    void allOff() noexcept { reset(); }

    // Re-align the step clock to the bar (called when host transport starts), so
    // the arp lands on the beat grid instead of free-running from the first note.
    void syncToBar() noexcept { phase_ = 0.0f; seqPos_ = 0; }

    void notePressed() noexcept { physicalDown_++; }

    // ---- clock + step ------------------------------------------------------
    // Advance one control block. Returns an event (possibly empty). tempoBpm and
    // the arp params set the step period; n is the block size in samples.
    ArpEvent process(int n, float tempoBpm, const ArpParams& ap) noexcept
    {
        ArpEvent ev;
        if(!running_ || !ap.on)
        {
            releaseSounding(ev); // release any dangling arp notes when stopped
            return ev;
        }

        if(rebuild_)
        {
            buildSequence(ap);
            rebuild_ = false;
        }
        if(seqLen_ == 0)
        {
            releaseSounding(ev); // don't leave notes stuck if the set emptied
            return ev;
        }

        // gate: each sounding note has its own remaining gate, so a >100% gate
        // can ring past the next step (overlapping/legato arps) without stealing
        // the new note's timer.
        for(int i = 0; i < soundingCount_; )
        {
            soundingGate_[i] -= n;
            if(soundingGate_[i] <= 0 && ev.offCount < kArpMaxStepNotes)
            {
                ev.offNotes[ev.offCount++] = sounding_[i];
                sounding_[i]     = sounding_[soundingCount_ - 1];
                soundingGate_[i] = soundingGate_[soundingCount_ - 1];
                --soundingCount_;
            }
            else ++i;
        }

        // step clock
        const double secPerQuarter = 60.0 / (tempoBpm < 1.0f ? 120.0 : tempoBpm);
        int          di            = ap.syncDiv;
        if(di < 0) di = 0;
        if(di >= kNumArpDiv) di = kNumArpDiv - 1;
        const double stepSec = secPerQuarter * kArpDivQuarters[di];
        // swing: lengthen even steps, shorten odd — classic shuffle
        double thisStepSec = stepSec;
        if(ap.swing > 0.001f)
        {
            const double s = (double)ap.swing; // 0..0.66
            thisStepSec = (seqPos_ & 1) ? stepSec * (1.0 - s) : stepSec * (1.0 + s);
        }
        const float inc = (float)((double)n / (thisStepSec * (double)sr_));
        phase_ += inc;

        if(phase_ >= 1.0f)
        {
            phase_ -= 1.0f;
            if(phase_ >= 1.0f)
                phase_ = 0.0f; // guard very fast divisions

            // ratchet: replay the current step until its repeats are spent
            if(ratchetLeft_ > 0)
                ratchetLeft_--;
            else
            {
                advanceSeqPos();
                ratchetLeft_ = (ap.ratchet > 1 ? ap.ratchet - 1 : 0);
            }

            // gate up to 200%: notes with gate <= 100% release before the next
            // step (clean), > 100% ring into it (overlap). emitOn handles the
            // per-note release; no bulk release here.
            const float g = ap.gate < 0.05f ? 0.05f : (ap.gate > 2.0f ? 2.0f : ap.gate);
            const int gateSamp = (int)(g * thisStepSec * (double)sr_);

            if(isChord_)
            {
                for(int i = 0; i < seqLen_ && ev.onCount < kArpMaxStepNotes; ++i)
                    emitOn(ev, seq_[i], seqVel_[i], gateSamp);
            }
            else
            {
                emitOn(ev, seq_[seqPos_], seqVel_[seqPos_], gateSamp);
            }
        }
        return ev;
    }

    bool running() const noexcept { return running_; }
    int  stepIndex() const noexcept { return seqPos_ < 0 ? 0 : seqPos_; }
    int  sequenceLen() const noexcept { return seqLen_; }

  private:
    static constexpr int kMaxHeld = 16;
    static constexpr int kMaxSeq  = kMaxHeld * 4 * 2 + 4; // notes * octaves * up+down

    // move all currently-sounding arp notes into the event's note-off list
    void releaseSounding(ArpEvent& ev) noexcept
    {
        for(int i = 0; i < soundingCount_ && ev.offCount < kArpMaxStepNotes; ++i)
            ev.offNotes[ev.offCount++] = sounding_[i];
        soundingCount_ = 0;
    }

    void emitOn(ArpEvent& ev, int note, int vel, int gateSamp) noexcept
    {
        // if this note is already sounding (overlap/retrigger), release the old
        // instance first so it can't get stuck as a duplicate.
        for(int i = 0; i < soundingCount_; ++i)
            if(sounding_[i] == note)
            {
                if(ev.offCount < kArpMaxStepNotes) ev.offNotes[ev.offCount++] = note;
                sounding_[i]     = sounding_[soundingCount_ - 1];
                soundingGate_[i] = soundingGate_[soundingCount_ - 1];
                --soundingCount_;
                break;
            }
        if(ev.onCount >= kArpMaxStepNotes || soundingCount_ >= kArpMaxStepNotes)
            return;
        ev.onNotes[ev.onCount] = note;
        ev.onVels[ev.onCount]  = vel;
        ++ev.onCount;
        sounding_[soundingCount_]     = note;
        soundingGate_[soundingCount_] = gateSamp;
        ++soundingCount_;
    }

    inline uint32_t rng() noexcept
    {
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        return rng_;
    }

    void addHeld(int note, int vel) noexcept
    {
        for(int i = 0; i < heldCount_; ++i)
            if(held_[i] == note)
            {
                heldVel_[i] = vel;
                return;
            }
        if(heldCount_ < kMaxHeld)
        {
            // insert sorted by pitch so UP/DOWN/CONVERGE are well-defined; keep a
            // parallel as-played order stamp for AS-PLAYED.
            held_[heldCount_]      = note;
            heldVel_[heldCount_]   = vel;
            heldOrder_[heldCount_] = orderStamp_++;
            ++heldCount_;
        }
    }

    void removeHeld(int note) noexcept
    {
        int w = 0;
        for(int i = 0; i < heldCount_; ++i)
            if(held_[i] != note)
            {
                held_[w]      = held_[i];
                heldVel_[w]   = heldVel_[i];
                heldOrder_[w] = heldOrder_[i];
                ++w;
            }
        heldCount_ = w;
    }

    void advanceSeqPos() noexcept
    {
        if(seqLen_ <= 0)
        {
            seqPos_ = 0;
            return;
        }
        if(isRandom_)
        {
            // pick a different step than the current one when possible (avoids a
            // note repeating, which reads as a stuck arp)
            int next = (int)(rng() % (uint32_t)seqLen_);
            if(seqLen_ > 1 && next == seqPos_)
                next = (next + 1) % seqLen_;
            seqPos_ = next;
            return;
        }
        // PINGPONG/CONDIVERGE bake the bounce into seq_ already; just wrap forward.
        seqPos_++;
        if(seqPos_ >= seqLen_)
            seqPos_ = 0;
    }

    // Build the ordered step list from the held set + pattern + octaves.
    void buildSequence(const ArpParams& ap) noexcept
    {
        // Reset the mode flags FIRST: the Random/Chord cases below 'return'
        // early, so without this a Chord->Random (or Random->Chord) switch would
        // leave the old flag set and play the wrong thing.
        isRandom_ = false;
        isChord_  = false;
        seqLen_ = 0;
        // Restart the pattern at index 0. seqPos_ = -1 so the first advanceSeqPos()
        // lands on 0 and step 0 (the root of an UP pattern) actually plays — with
        // seqPos_ = 0 the pre-increment in advanceSeqPos() skipped it to step 1.
        seqPos_ = -1;
        if(heldCount_ == 0)
            return;

        // sorted-by-pitch index list
        int idx[kMaxHeld];
        for(int i = 0; i < heldCount_; ++i)
            idx[i] = i;
        for(int a = 0; a < heldCount_; ++a)
            for(int b = a + 1; b < heldCount_; ++b)
                if(held_[idx[b]] < held_[idx[a]])
                {
                    int t = idx[a]; idx[a] = idx[b]; idx[b] = t;
                }

        const int oct = ap.octaves < 1 ? 1 : (ap.octaves > 4 ? 4 : ap.octaves);
        // base ascending list across octaves
        int  base[kMaxSeq];
        int  baseVel[kMaxSeq];
        int  baseN = 0;
        for(int o = 0; o < oct; ++o)
            for(int i = 0; i < heldCount_ && baseN < kMaxSeq; ++i)
            {
                base[baseN]    = held_[idx[i]] + 12 * o;
                baseVel[baseN] = heldVel_[idx[i]];
                ++baseN;
            }

        auto push = [&](int note, int vel) {
            if(seqLen_ < kMaxSeq)
            {
                seq_[seqLen_]    = note;
                seqVel_[seqLen_] = vel;
                ++seqLen_;
            }
        };

        switch((ArpMode)ap.mode)
        {
            case ArpMode::Up:
                for(int i = 0; i < baseN; ++i) push(base[i], baseVel[i]);
                break;
            case ArpMode::Down:
                for(int i = baseN - 1; i >= 0; --i) push(base[i], baseVel[i]);
                break;
            case ArpMode::UpDown: // endpoints once: 1 2 3 2
                for(int i = 0; i < baseN; ++i) push(base[i], baseVel[i]);
                for(int i = baseN - 2; i >= 1; --i) push(base[i], baseVel[i]);
                break;
            case ArpMode::UpDownInc: // endpoints twice: 1 2 3 3 2 1
                for(int i = 0; i < baseN; ++i) push(base[i], baseVel[i]);
                for(int i = baseN - 1; i >= 0; --i) push(base[i], baseVel[i]);
                break;
            case ArpMode::DownUp:
                for(int i = baseN - 1; i >= 0; --i) push(base[i], baseVel[i]);
                for(int i = 1; i <= baseN - 2; ++i) push(base[i], baseVel[i]);
                break;
            case ArpMode::PingPong: // like UpDown but endpoints repeated (bounce feel)
                for(int i = 0; i < baseN; ++i) push(base[i], baseVel[i]);
                for(int i = baseN - 1; i >= 0; --i) push(base[i], baseVel[i]);
                break;
            case ArpMode::Converge: // outside-in: low, high, low+1, high-1, ...
            {
                int lo = 0, hi = baseN - 1;
                while(lo <= hi)
                {
                    push(base[lo], baseVel[lo]);
                    if(hi != lo) push(base[hi], baseVel[hi]);
                    ++lo; --hi;
                }
                break;
            }
            case ArpMode::Diverge: // inside-out
            {
                int mid = (baseN - 1) / 2;
                int lo = mid, hi = mid + (baseN % 2 == 0 ? 1 : 1);
                // walk outward from the centre
                push(base[mid], baseVel[mid]);
                int step = 1;
                while(true)
                {
                    int l = mid - step, h = mid + step;
                    bool any = false;
                    if(l >= 0) { push(base[l], baseVel[l]); any = true; }
                    if(h < baseN) { push(base[h], baseVel[h]); any = true; }
                    if(!any) break;
                    ++step;
                }
                (void)lo; (void)hi;
                break;
            }
            case ArpMode::ConDiverge: // converge then diverge
            {
                int lo = 0, hi = baseN - 1;
                while(lo <= hi)
                {
                    push(base[lo], baseVel[lo]);
                    if(hi != lo) push(base[hi], baseVel[hi]);
                    ++lo; --hi;
                }
                int mid = (baseN - 1) / 2;
                for(int step = 1; ; ++step)
                {
                    int l = mid - step, h = mid + step;
                    bool any = false;
                    if(l >= 0) { push(base[l], baseVel[l]); any = true; }
                    if(h < baseN) { push(base[h], baseVel[h]); any = true; }
                    if(!any) break;
                }
                break;
            }
            case ArpMode::AsPlayed:
            {
                // order held notes by their as-played stamp, then stack octaves
                int order[kMaxHeld];
                for(int i = 0; i < heldCount_; ++i) order[i] = i;
                for(int a = 0; a < heldCount_; ++a)
                    for(int b = a + 1; b < heldCount_; ++b)
                        if(heldOrder_[order[b]] < heldOrder_[order[a]])
                        { int t = order[a]; order[a] = order[b]; order[b] = t; }
                for(int o = 0; o < oct; ++o)
                    for(int i = 0; i < heldCount_; ++i)
                        push(held_[order[i]] + 12 * o, heldVel_[order[i]]);
                break;
            }
            case ArpMode::Random:
                // fill with the base set; advanceSeqPos picks randomly via the
                // engine's choice. Here we just provide the pool in order and let
                // playback randomise (handled by random pick in advance()).
                for(int i = 0; i < baseN; ++i) push(base[i], baseVel[i]);
                isRandom_ = true;
                return;
            case ArpMode::Chord: // all notes each step (handled by engine: play set)
                for(int i = 0; i < baseN; ++i) push(base[i], baseVel[i]);
                isChord_ = true;
                return;
            default:
                for(int i = 0; i < baseN; ++i) push(base[i], baseVel[i]);
                break;
        }
        isRandom_ = false;
        isChord_  = false;
    }

    float sr_ = 48000.0f;

    int  held_[kMaxHeld];
    int  heldVel_[kMaxHeld];
    uint32_t heldOrder_[kMaxHeld];
    int  heldCount_  = 0;
    uint32_t orderStamp_ = 0;

    int  seq_[kMaxSeq];
    int  seqVel_[kMaxSeq];
    int  seqLen_ = 0;
    int  seqPos_ = 0;

    float phase_   = 0.0f;
    bool  running_ = false;
    bool  rebuild_ = false;
    int   bounceDir_ = 1;

    // notes currently sounding from the last step (1 for normal modes, the whole
    // chord for CHORD mode) — released on the next step or on gate-off.
    int   sounding_[kArpMaxStepNotes];
    int   soundingGate_[kArpMaxStepNotes] = {0}; // remaining gate samples per note
    int   soundingCount_   = 0;
    int   ratchetLeft_     = 0;

    int   physicalDown_    = 0;
    bool  anyPhysicalDown_ = false;

    uint32_t rng_ = 0x9E3779B9u;

  public:
    bool isRandom_ = false;
    bool isChord_  = false;
};
} // namespace jove

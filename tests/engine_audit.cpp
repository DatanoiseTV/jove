/*
  Jove — offline engine + preset audit harness (no JUCE).

  Loads every factory preset, plays a chord, renders the sustain + release tail,
  and checks the bare engine for the failure modes that matter in a shipping
  instrument: NaN/Inf, output that exceeds the master soft-clip ceiling, fully
  silent patches, runaway DC, and stuck (never-releasing) voices. Also spot-runs
  the arpeggiator, pitch bend, mod wheel and unison so those paths get exercised.

  Build + run:
    c++ -std=c++17 -O2 -I jove/src/engine jove/tests/engine_audit.cpp \
        jove/src/engine/SynthEngine.cpp jove/src/engine/SynthPresets.cpp -o /tmp/jove_audit
*/

#include "SynthEngine.h"
#include "SynthPresets.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace jove;

namespace
{
constexpr float kSR    = 48000.0f;
constexpr int   kBlock = 64;

struct Stats
{
    float peak = 0.0f;
    float dc   = 0.0f; // mean
    double energy = 0.0;
    long   samples = 0;
    bool   nan = false;
    bool   inf = false;
};

void accumulate(Stats& st, const float* b, int n)
{
    for(int i = 0; i < n; ++i)
    {
        const float v = b[i];
        if(std::isnan(v)) st.nan = true;
        if(std::isinf(v)) st.inf = true;
        const float a = std::fabs(v);
        if(a > st.peak) st.peak = a;
        st.dc += v;
        st.energy += (double) v * v;
        ++st.samples;
    }
}

// Render `seconds` of audio into the running stats. Optionally release at `relAt`.
void renderFor(SynthEngine& e, Stats& st, float seconds)
{
    float L[kBlock], R[kBlock];
    const int blocks = (int) (seconds * kSR / kBlock);
    for(int b = 0; b < blocks; ++b)
    {
        e.render(L, R, kBlock);
        accumulate(st, L, kBlock);
        accumulate(st, R, kBlock);
    }
}
} // namespace

int main()
{
    int failures = 0, warnings = 0, silent = 0;
    const float kCeil = 1.0001f; // master soft-clip guarantees <= 1.0

    printf("Jove engine audit — %d factory presets @ %g Hz, control block %d\n\n",
           kNumFactoryPresets, kSR, kBlock);

    for(int idx = 0; idx < kNumFactoryPresets; ++idx)
    {
        SynthPatch patch;
        LoadFactoryPreset(idx, patch);

        SynthEngine eng;
        eng.prepare(kSR, kBlock);
        eng.setPatch(&patch);
        eng.setTempo(120.0f);

        // play a 4-note chord
        const int chord[4] = {48, 55, 60, 64};
        for(int n : chord)
            eng.noteOn(n, 0.8f);

        Stats st;
        renderFor(eng, st, 1.5f);         // attack + sustain
        const uint32_t trigAfterHold = eng.triggerCount();

        for(int n : chord)
            eng.noteOff(n);
        renderFor(eng, st, 5.0f);          // release tail (pads release up to ~4 s)

        const int   activeAfterTail = eng.activeVoices();
        const float mean = st.samples ? st.dc / (float) st.samples : 0.0f;
        const float rms  = st.samples ? std::sqrt((float) (st.energy / st.samples)) : 0.0f;

        std::string flags;
        if(st.nan)               { flags += " NAN";   ++failures; }
        if(st.inf)               { flags += " INF";   ++failures; }
        if(st.peak > kCeil)      { flags += " CLIP";  ++failures; }
        if(std::fabs(mean) > 0.02f) { flags += " DC"; ++warnings; }
        if(activeAfterTail > 0 && !patch.arp.on) { flags += " STUCK"; ++warnings; }
        if(st.peak < 1e-4f)      { flags += " SILENT"; ++silent; ++warnings; }

        const bool ok = flags.empty();
        printf("%2d %-18s cat=%-2d peak=%.3f rms=%.3f dc=%+.4f voices=%d%s%s\n",
               idx, FactoryPresetName(idx), FactoryPresetCategory(idx),
               st.peak, rms, mean, activeAfterTail,
               ok ? "  ok" : "  <<", flags.c_str());
    }

    // ---- targeted path exercises (arp / bend / wheel / unison) ----
    printf("\n-- path exercises --\n");
    {
        SynthPatch p;
        LoadFactoryPreset(0, p);
        p.arp.on = true; p.arp.mode = (int) ArpMode::UpDown; p.arp.octaves = 2;
        SynthEngine e; e.prepare(kSR, kBlock); e.setPatch(&p); e.setTempo(128.0f);
        e.noteOn(48, 0.9f); e.noteOn(52, 0.9f); e.noteOn(55, 0.9f);
        Stats st; const uint32_t t0 = e.triggerCount();
        renderFor(e, st, 2.0f);
        const uint32_t t1 = e.triggerCount();
        printf("arp: triggers %u -> %u (%s), peak=%.3f%s%s\n", t0, t1,
               t1 > t0 ? "stepping" : "DEAD", st.peak,
               st.nan ? " NAN" : "", st.inf ? " INF" : "");
        if(t1 <= t0) ++failures;
        if(st.nan || st.inf) ++failures;
    }
    {
        SynthPatch p; LoadFactoryPreset(10, p);
        p.voiceMode = (int) VoiceMode::Unison; p.unisonCount = 7;
        SynthEngine e; e.prepare(kSR, kBlock); e.setPatch(&p);
        e.noteOn(60, 1.0f);
        Stats st;
        e.pitchBend(1.0f); renderFor(e, st, 0.3f);
        e.pitchBend(-1.0f); renderFor(e, st, 0.3f);
        e.modWheel(1.0f); e.aftertouch(1.0f); renderFor(e, st, 0.4f);
        printf("unison+bend+wheel: peak=%.3f%s%s\n", st.peak,
               st.nan ? " NAN" : "", st.inf ? " INF" : "");
        if(st.peak > kCeil || st.nan || st.inf) ++failures;
    }

    printf("\nRESULT: %d failures, %d warnings (%d silent)\n", failures, warnings, silent);
    return failures > 0 ? 1 : 0;
}

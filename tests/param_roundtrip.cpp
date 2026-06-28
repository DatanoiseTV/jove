/*
  Jove — APVTS <-> SynthPatch round-trip audit.

  For every factory preset: load it through the real PresetManager (which writes
  each field out to the parameter tree), then read the live tree back into a patch
  and compare field-by-field. Any mismatch means the parameter layout, the
  writeFrom mapping or the readInto mapping disagree — exactly the class of bug
  that silently corrupts a preset or a mod-matrix slot. Float fields allow a small
  epsilon for the NormalisableRange skew round-trip; ints/enums must match exactly.
*/

#include "PluginProcessor.h"
#include <cmath>
#include <cstdio>

using namespace jove;

int main()
{
    juce::ScopedJuceInitialiser_GUI gui; // File paths, message manager for APVTS
    JoveAudioProcessor proc;
    auto& pm = proc.getPresetManager();

    int totalMismatch = 0;
    const float eps = 5e-3f;

    auto F = [&](int i, const char* field, float a, float b) {
        if(std::fabs(a - b) > eps)
        {
            printf("  [%2d %-16s] %-12s ref=%.5f got=%.5f\n", i, FactoryPresetName(i), field, a, b);
            ++totalMismatch;
        }
    };
    auto I = [&](int i, const char* field, int a, int b) {
        if(a != b)
        {
            printf("  [%2d %-16s] %-12s ref=%d got=%d\n", i, FactoryPresetName(i), field, a, b);
            ++totalMismatch;
        }
    };

    for(int i = 0; i < kNumFactoryPresets; ++i)
    {
        SynthPatch r; // reference straight from the bank
        LoadFactoryPreset(i, r);
        pm.loadFactory(i);              // writes the tree
        SynthPatch g = proc.readLivePatch(); // reads the tree back

        I(i, "voiceMode", r.voiceMode, g.voiceMode);
        I(i, "unisonCount", r.unisonCount, g.unisonCount);
        F(i, "unisonDetune", r.unisonDetune, g.unisonDetune);
        F(i, "unisonSpread", r.unisonSpread, g.unisonSpread);
        I(i, "glideMode", r.glideMode, g.glideMode);
        F(i, "glideTime", r.glideTime, g.glideTime);
        I(i, "bendRange", r.bendRange, g.bendRange);
        F(i, "ampGain", r.ampGain, g.ampGain);
        F(i, "pan", r.pan, g.pan);
        F(i, "width", r.width, g.width);
        F(i, "drift", r.drift, g.drift);

        for(int o = 0; o < kNumOsc; ++o)
        {
            I(i, "oscOn", r.osc[o].on, g.osc[o].on);
            I(i, "oscFoot", r.osc[o].footage, g.osc[o].footage);
            F(i, "oscMorph", r.osc[o].morph, g.osc[o].morph);
            F(i, "oscPw", r.osc[o].pw, g.osc[o].pw);
            F(i, "oscDetune", r.osc[o].detune, g.osc[o].detune);
            F(i, "oscLevel", r.osc[o].level, g.osc[o].level);
        }
        F(i, "oscMix", r.oscMix, g.oscMix);
        F(i, "subLevel", r.subLevel, g.subLevel);
        I(i, "subOctave", r.subOctave, g.subOctave);
        F(i, "noiseLevel", r.noiseLevel, g.noiseLevel);
        I(i, "sync2", r.sync2Mode, g.sync2Mode);
        I(i, "sync3", r.sync3Mode, g.sync3Mode);
        F(i, "fm2to1", r.fm2to1, g.fm2to1);
        F(i, "ringMod", r.ringMod, g.ringMod);

        I(i, "filterMode", r.filterMode, g.filterMode);
        F(i, "cutoff", r.cutoff, g.cutoff);
        F(i, "resonance", r.resonance, g.resonance);
        F(i, "filterDrive", r.filterDrive, g.filterDrive);
        F(i, "keyTrack", r.keyTrack, g.keyTrack);
        F(i, "envFilterAmt", r.envFilterAmt, g.envFilterAmt);

        for(int e = 0; e < kNumEnv; ++e)
        {
            F(i, "envA", r.env[e].attack, g.env[e].attack);
            F(i, "envD", r.env[e].decay, g.env[e].decay);
            F(i, "envS", r.env[e].sustain, g.env[e].sustain);
            F(i, "envR", r.env[e].release, g.env[e].release);
            F(i, "envVel", r.env[e].velToLevel, g.env[e].velToLevel);
        }

        for(int l = 0; l < kNumLfo; ++l)
        {
            I(i, "lfoWave", r.lfo[l].wave, g.lfo[l].wave);
            F(i, "lfoRate", r.lfo[l].rate, g.lfo[l].rate);
            F(i, "lfoDepth", r.lfo[l].depth, g.lfo[l].depth);
            I(i, "lfoSync", r.lfo[l].sync, g.lfo[l].sync);
            I(i, "lfoDiv", r.lfo[l].syncDiv, g.lfo[l].syncDiv);
            I(i, "lfoRetrig", r.lfo[l].retrig, g.lfo[l].retrig);
            I(i, "lfoPerVoice", r.lfo[l].perVoice, g.lfo[l].perVoice);
            F(i, "lfoFade", r.lfo[l].fade, g.lfo[l].fade);
            F(i, "lfoDelay", r.lfo[l].delay, g.lfo[l].delay);
            F(i, "lfoOffset", r.lfo[l].offset, g.lfo[l].offset);
        }

        for(int s = 0; s < kNumModSlots; ++s)
        {
            I(i, "modSrc", r.mod[s].source, g.mod[s].source);
            I(i, "modDst", r.mod[s].dest, g.mod[s].dest);
            F(i, "modAmt", r.mod[s].amount, g.mod[s].amount);
        }

        I(i, "arpOn", r.arp.on, g.arp.on);
        I(i, "arpMode", r.arp.mode, g.arp.mode);
        I(i, "arpOct", r.arp.octaves, g.arp.octaves);
        I(i, "arpDiv", r.arp.syncDiv, g.arp.syncDiv);
        F(i, "arpGate", r.arp.gate, g.arp.gate);
        F(i, "arpSwing", r.arp.swing, g.arp.swing);
        I(i, "arpLatch", r.arp.latch, g.arp.latch);
        I(i, "arpRatchet", r.arp.ratchet, g.arp.ratchet);

        F(i, "fxDrive", r.fxDrive, g.fxDrive);
        F(i, "fxChorus", r.fxChorus, g.fxChorus);
        I(i, "chorusMode", r.chorusMode, g.chorusMode);
        F(i, "fxPhaser", r.fxPhaser, g.fxPhaser);
        F(i, "fxDelay", r.fxDelay, g.fxDelay);
        F(i, "fxReverb", r.fxReverb, g.fxReverb);
        I(i, "delaySync", r.delaySync, g.delaySync);
        I(i, "delayDiv", r.delayDiv, g.delayDiv);
        F(i, "delayTimeMs", r.delayTimeMs, g.delayTimeMs);
        F(i, "delayFeedback", r.delayFeedback, g.delayFeedback);
        F(i, "delayTone", r.delayTone, g.delayTone);
        I(i, "delayPing", r.delayPing, g.delayPing);
        F(i, "reverbSize", r.reverbSize, g.reverbSize);
        F(i, "reverbTone", r.reverbTone, g.reverbTone);
    }

    printf("\nROUND-TRIP: %d field mismatches across %d presets\n", totalMismatch, kNumFactoryPresets);
    return totalMismatch > 0 ? 1 : 0;
}

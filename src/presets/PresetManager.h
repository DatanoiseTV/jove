/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include "../JoveParams.h"
#include "../engine/SynthPresets.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

// Preset bank for Jove. Factory patches come from the engine's sound-designed
// bank (jove::LoadFactoryPreset); user patches are stored as XML param dumps in
// the user data directory. Loading a preset writes every parameter through the
// PatchBinding so the host + UI follow; the processor reads the patch back on its
// next block. The current preset name/category live as APVTS state properties so
// they survive save/restore.
class PresetManager
{
  public:
    struct Entry
    {
        juce::String name;
        int          category = 0;
        bool         factory  = true;
        juce::File   file;        // empty for factory
        int          factoryIdx = -1;
    };

    void init(juce::AudioProcessorValueTreeState& s, jove::PatchBinding& b);

    // Called (on the message thread) whenever a preset is loaded — the processor
    // uses it to flag a voice panic so held/stuck notes don't survive the swap.
    void setLoadCallback(std::function<void()> f) { onLoad = std::move(f); }

    // ---- catalogue -------------------------------------------------------
    void rescan();                                   // rebuild the user-preset list
    const std::vector<Entry>& entries() const { return catalogue; }
    int numPresets() const { return (int) catalogue.size(); }

    // ---- loading ---------------------------------------------------------
    void load(int catalogueIndex);
    void loadFactory(int factoryIndex);
    void loadInit();
    void next();
    void prev();

    // ---- saving ----------------------------------------------------------
    // Saves the current parameter state as a user preset. Returns false on I/O
    // failure. Overwrites a same-named user preset.
    bool saveUserPreset(const juce::String& name, int category);
    bool deleteUserPreset(const juce::String& name);

    // ---- randomizer ------------------------------------------------------
    // DICE: a fresh musical patch built from a random archetype (pad / pluck /
    // bass / lead / keys / drone) with parameters constrained to usable ranges.
    // VARY: gently perturbs the CURRENT patch's continuous parameters, keeping
    // its structure (waves, modes, routing) intact.
    void randomizeDice();
    void randomizeVary();

    // After a DAW session restore, re-point `current` at the catalogue entry
    // matching the restored preset name (prev/next continue from there).
    void resyncCurrentFromName() { current = indexOfName(currentName()); }

    // ---- current state ---------------------------------------------------
    juce::String currentName() const;
    int          currentCategory() const;
    int          currentIndex() const { return current; }

    // True when the parameters differ from the last loaded/saved reference.
    // Hash-based, so undoing every edit reads clean again. Randomized patches
    // (DICE/VARY) are born dirty — they exist nowhere on disk yet. Save clears
    // it (the saved file IS the new reference).
    bool currentIsDirty() const;
    void markClean() { cleanHash = paramStateHash(); }

    static juce::File userPresetDir();

  private:
    void applyPatch(const jove::SynthPatch& p, const juce::String& name, int category,
                    bool clean = true);
    void setNameCat(const juce::String& name, int category);
    int  indexOfName(const juce::String& name) const;
    juce::uint64 paramStateHash() const; // order-stable hash of all patch params

    juce::AudioProcessorValueTreeState* apvts = nullptr;
    jove::PatchBinding*                 binding = nullptr;
    std::vector<Entry>                  catalogue;
    int                                 current = -1;
    juce::uint64                        cleanHash = 0;
    std::function<void()>               onLoad;
};

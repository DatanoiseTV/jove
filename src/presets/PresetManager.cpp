/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#include "PresetManager.h"
#include <iterator>

using namespace jove;

namespace
{
constexpr const char* kNameProp = "presetName";
constexpr const char* kCatProp  = "presetCategory";

// Global settings live in the APVTS but must NOT travel inside a preset — they
// are the user's standing preference. Stripped from the saved file and preserved
// across a user-preset load.
const juce::StringArray kGlobalIDs { jove::jID::maxVoices, jove::jID::quality };

juce::String sanitise(const juce::String& n)
{
    return juce::File::createLegalFileName(n).trim();
}
} // namespace

juce::File PresetManager::userPresetDir()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("DatanoiseTV")
                   .getChildFile("Jove")
                   .getChildFile("Presets");
    if(!dir.exists())
        dir.createDirectory();
    return dir;
}

void PresetManager::init(juce::AudioProcessorValueTreeState& s, jove::PatchBinding& b)
{
    apvts   = &s;
    binding = &b;
    rescan();
}

void PresetManager::rescan()
{
    catalogue.clear();
    for(int i = 0; i < kNumFactoryPresets; ++i)
        catalogue.push_back({FactoryPresetName(i), FactoryPresetCategory(i), true, {}, i});

    auto files = userPresetDir().findChildFiles(juce::File::findFiles, false, "*.jove");
    files.sort();
    for(const auto& f : files)
    {
        Entry e;
        e.name    = f.getFileNameWithoutExtension();
        e.factory = false;
        e.file    = f;
        if(auto xml = juce::XmlDocument::parse(f))
            e.category = xml->getIntAttribute(kCatProp, 0);
        catalogue.push_back(std::move(e));
    }
}

void PresetManager::setNameCat(const juce::String& name, int category)
{
    if(apvts == nullptr)
        return;
    apvts->state.setProperty(kNameProp, name, nullptr);
    apvts->state.setProperty(kCatProp, category, nullptr);
}

void PresetManager::applyPatch(const SynthPatch& p, const juce::String& name, int category,
                               bool clean)
{
    if(binding == nullptr)
        return;
    binding->writeFrom(p);
    setNameCat(name, category);
    if(clean) markClean();
    else      cleanHash = 0; // born dirty (randomized patch, exists nowhere yet)
    if(onLoad) onLoad();
}

juce::uint64 PresetManager::paramStateHash() const
{
    if(apvts == nullptr)
        return 0;
    // FNV-1a over the quantised normalised value of every parameter, skipping
    // the global settings (polyphony / quality / audition) — those are user
    // preferences, not patch content, and must not read as "edited".
    juce::uint64 h = 1469598103934665603ULL;
    for(auto* param : apvts->processor.getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param);
        if(ranged == nullptr) continue;
        const auto id = ranged->getParameterID();
        if(kGlobalIDs.contains(id) || id == jID::auditionOn) continue;
        const auto q = (juce::uint32) juce::roundToInt(ranged->getValue() * 1000000.0f);
        h ^= q; h *= 1099511628211ULL;
    }
    return h;
}

bool PresetManager::currentIsDirty() const
{
    return cleanHash != 0 ? paramStateHash() != cleanHash : true;
}

void PresetManager::loadFactory(int factoryIndex)
{
    SynthPatch p;
    LoadFactoryPreset(factoryIndex, p);
    applyPatch(p, FactoryPresetName(factoryIndex), FactoryPresetCategory(factoryIndex));
    current = factoryIndex; // factory entries lead the catalogue 1:1
}

void PresetManager::loadInit()
{
    SynthPatch p;
    InitDefaultPatch(p);
    applyPatch(p, "INIT", 1);
    current = -1;
}

void PresetManager::load(int catalogueIndex)
{
    if(catalogueIndex < 0 || catalogueIndex >= (int) catalogue.size())
        return;
    const auto& e = catalogue[(size_t) catalogueIndex];
    if(e.factory)
    {
        loadFactory(e.factoryIdx);
        current = catalogueIndex;
        return;
    }
    // user preset: parse the saved param tree and swap it in atomically, while
    // preserving the global settings (polyphony / quality) across the swap.
    if(auto xml = juce::XmlDocument::parse(e.file))
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        if(tree.isValid() && apvts != nullptr)
        {
            std::vector<std::pair<juce::String, float>> globals;
            for(const auto& id : kGlobalIDs)
                if(auto* p = apvts->getParameter(id))
                    globals.emplace_back(id, p->getValue()); // normalised

            apvts->replaceState(tree);

            for(const auto& g : globals)
                if(auto* p = apvts->getParameter(g.first))
                    p->setValueNotifyingHost(g.second);

            setNameCat(e.name, e.category);
            markClean(); // freshly loaded = the new reference state
            current = catalogueIndex;
            if(onLoad) onLoad();
        }
    }
}

void PresetManager::next()
{
    if(catalogue.empty())
        return;
    load((current + 1) % (int) catalogue.size());
}

void PresetManager::prev()
{
    if(catalogue.empty())
        return;
    load((current - 1 + (int) catalogue.size()) % (int) catalogue.size());
}

bool PresetManager::saveUserPreset(const juce::String& name, int category)
{
    if(apvts == nullptr)
        return false;
    const auto clean = sanitise(name);
    if(clean.isEmpty())
        return false;

    setNameCat(clean, category);
    auto state = apvts->copyState();
    // drop global-setting params so the file only carries the patch
    for(const auto& id : kGlobalIDs)
        for(int i = state.getNumChildren(); --i >= 0;)
        {
            auto c = state.getChild(i);
            if(c.hasProperty("id") && c.getProperty("id").toString() == id)
                state.removeChild(i, nullptr);
        }
    if(auto xml = state.createXml())
    {
        xml->setAttribute(kNameProp, clean);
        xml->setAttribute(kCatProp, category);
        auto file = userPresetDir().getChildFile(clean + ".jove");
        if(!xml->writeTo(file))
            return false;
        rescan();
        current = indexOfName(clean);
        markClean(); // the saved file IS the current state — not dirty anymore
        return true;
    }
    return false;
}

namespace
{
// Random-name generator for DICE patches: two short evocative word lists give
// 30x30 combinations, so repeats are rare enough to feel fresh.
constexpr const char* kNameA[] = {
    "VELVET", "NEON",   "RUSTY",  "LUNAR",  "AMBER",  "COSMIC", "PLASTIC", "GLASS",
    "IRON",   "MISTY",  "SOLAR",  "PALE",   "ELECTRIC", "FROZEN", "GOLDEN", "HOLLOW",
    "MIDNIGHT", "CRIMSON", "SILENT", "WILD", "BROKEN", "LIQUID", "STATIC",  "DUSTY",
    "NOVA",   "RETRO",  "VAPOR",  "CHROME", "SHADOW", "PRISM"};
constexpr const char* kNameB[] = {
    "ORBIT",  "PULSE",  "DRIFT",  "BLOOM",  "WIRE",   "TIDE",   "SPARK",  "HAZE",
    "CIRCUIT", "FIELD", "WAVE",   "DUST",   "ECHO",   "MOTOR",  "GARDEN", "SIGNAL",
    "CANYON", "MIRROR", "ENGINE", "RIVER",  "TOWER",  "HALO",   "VOLT",   "EMBER",
    "PHANTOM", "COMET", "LAGOON", "PISTON", "AURORA", "RIDGE"};

float frand(juce::Random& r, float lo, float hi) { return lo + r.nextFloat() * (hi - lo); }
} // namespace

void PresetManager::randomizeDice()
{
    if(apvts == nullptr || binding == nullptr)
        return;
    auto& r = juce::Random::getSystemRandom();

    SynthPatch p; // start from the init patch: everything defined, nothing exotic
    InitDefaultPatch(p);

    // archetype drives the envelope / filter / FX ranges so results are playable
    enum { Pad, Pluck, Bass, Lead, Keys, Drone, NumArch };
    const int arch = r.nextInt(NumArch);

    // --- oscillators: 2-3 active, random waves, gentle detune spread ---------
    const int nOsc = 2 + r.nextInt(2);
    for(int i = 0; i < (int) std::size(p.osc); ++i)
    {
        auto& o = p.osc[i];
        o.on = i < nOsc;
        if(!o.on) { o.level = 0.0f; continue; }
        o.oscType = r.nextInt(10) < 3 ? 1 : 0;               // 30% wavetable
        o.morph   = frand(r, 0.15f, 1.0f);
        o.pw      = frand(r, 0.2f, 0.8f);
        o.wtTable = r.nextInt(64);
        o.wtMorph = frand(r, 0.0f, 0.4f);
        o.detune  = i == 0 ? 0.0f : frand(r, -0.12f, 0.12f); // beating, not sour
        o.level   = i == 0 ? 1.0f : frand(r, 0.4f, 0.9f);
        o.footage = i == 2 && r.nextBool() ? 1 : 2;          // osc3 sometimes -1 oct
        o.crush = o.srReduce = 0.0f;
    }
    p.subLevel   = (arch == Bass || r.nextInt(10) < 3) ? frand(r, 0.2f, 0.7f) : 0.0f;
    p.noiseLevel = r.nextInt(10) < 2 ? frand(r, 0.05f, 0.25f) : 0.0f;
    p.fm2to1     = r.nextInt(10) < 2 ? frand(r, 0.05f, 0.35f) : 0.0f;
    p.ringMod    = r.nextInt(10) < 1 ? frand(r, 0.1f, 0.4f) : 0.0f;
    p.drift      = frand(r, 0.1f, 0.45f);
    p.voiceMode  = 0;
    if((arch == Pad || arch == Lead) && r.nextBool())
    {
        p.voiceMode    = 2; // unison
        p.unisonCount  = 3 + 2 * r.nextInt(2);
        p.unisonDetune = frand(r, 0.08f, 0.25f);
        p.unisonSpread = frand(r, 0.4f, 0.9f);
    }

    // --- filter ---------------------------------------------------------------
    const int fmodes[] = {0, 0, 0, 1, 6}; // mostly MOOG, sometimes SVF-LP/Steiner
    p.filterMode  = fmodes[r.nextInt((int) std::size(fmodes))];
    p.resonance   = frand(r, 0.05f, arch == Bass ? 0.5f : 0.65f);
    p.keyTrack    = frand(r, 0.3f, 0.8f);
    p.filterRouting = 0;

    // --- per-archetype envelopes / cutoff / FX ---------------------------------
    auto& ae = p.env[0]; auto& fe = p.env[1];
    switch(arch)
    {
        case Pad:
            ae = {frand(r, 0.4f, 1.5f), frand(r, 0.5f, 1.5f), frand(r, 0.6f, 0.9f), frand(r, 0.8f, 2.5f), frand(r, 0.0f, 0.4f)};
            fe = {frand(r, 0.5f, 2.0f), frand(r, 0.8f, 2.0f), frand(r, 0.3f, 0.7f), frand(r, 1.0f, 2.5f), 0.0f};
            p.cutoff = frand(r, 0.3f, 0.55f); p.envFilterAmt = frand(r, 0.1f, 0.4f);
            p.fxChorus = frand(r, 0.2f, 0.5f); p.fxReverb = frand(r, 0.3f, 0.55f); p.fxDelay = frand(r, 0.0f, 0.3f);
            p.category = 0; break;
        case Pluck:
            ae = {0.003f, frand(r, 0.15f, 0.5f), frand(r, 0.0f, 0.25f), frand(r, 0.1f, 0.5f), frand(r, 0.2f, 0.6f)};
            fe = {0.002f, frand(r, 0.1f, 0.4f), frand(r, 0.0f, 0.2f), frand(r, 0.1f, 0.3f), frand(r, 0.2f, 0.5f)};
            p.cutoff = frand(r, 0.3f, 0.5f); p.envFilterAmt = frand(r, 0.35f, 0.75f);
            p.fxChorus = frand(r, 0.0f, 0.3f); p.fxReverb = frand(r, 0.15f, 0.4f); p.fxDelay = frand(r, 0.15f, 0.4f);
            p.category = 6; break;
        case Bass:
            ae = {0.002f, frand(r, 0.15f, 0.45f), frand(r, 0.4f, 0.8f), frand(r, 0.08f, 0.25f), frand(r, 0.1f, 0.5f)};
            fe = {0.002f, frand(r, 0.1f, 0.35f), frand(r, 0.1f, 0.4f), frand(r, 0.08f, 0.2f), frand(r, 0.2f, 0.6f)};
            p.cutoff = frand(r, 0.25f, 0.45f); p.envFilterAmt = frand(r, 0.3f, 0.65f);
            p.fxChorus = 0.0f; p.fxReverb = frand(r, 0.0f, 0.12f); p.fxDelay = frand(r, 0.0f, 0.15f);
            for(auto& o : p.osc) if(o.on) o.footage = 1; // 16'
            p.category = 2; break;
        case Lead:
            ae = {frand(r, 0.005f, 0.06f), frand(r, 0.2f, 0.6f), frand(r, 0.6f, 0.9f), frand(r, 0.15f, 0.6f), frand(r, 0.2f, 0.6f)};
            fe = {frand(r, 0.005f, 0.08f), frand(r, 0.2f, 0.6f), frand(r, 0.3f, 0.7f), frand(r, 0.2f, 0.5f), frand(r, 0.1f, 0.4f)};
            p.cutoff = frand(r, 0.45f, 0.7f); p.envFilterAmt = frand(r, 0.15f, 0.5f);
            p.fxChorus = frand(r, 0.0f, 0.3f); p.fxReverb = frand(r, 0.15f, 0.35f); p.fxDelay = frand(r, 0.2f, 0.45f);
            p.glideMode = r.nextBool() ? 2 : 0; p.glideTime = frand(r, 0.03f, 0.12f);
            p.category = 1; break;
        case Keys:
            ae = {0.004f, frand(r, 0.3f, 0.9f), frand(r, 0.3f, 0.7f), frand(r, 0.2f, 0.6f), frand(r, 0.3f, 0.6f)};
            fe = {0.003f, frand(r, 0.25f, 0.8f), frand(r, 0.2f, 0.5f), frand(r, 0.2f, 0.5f), frand(r, 0.3f, 0.6f)};
            p.cutoff = frand(r, 0.4f, 0.65f); p.envFilterAmt = frand(r, 0.2f, 0.5f);
            p.fxChorus = frand(r, 0.15f, 0.45f); p.fxReverb = frand(r, 0.2f, 0.4f); p.fxDelay = frand(r, 0.0f, 0.25f);
            p.category = 5; break;
        default: // Drone
            ae = {frand(r, 1.0f, 3.0f), 1.0f, 1.0f, frand(r, 1.5f, 4.0f), 0.0f};
            fe = {frand(r, 1.5f, 4.0f), 1.0f, frand(r, 0.4f, 0.8f), frand(r, 2.0f, 4.0f), 0.0f};
            p.cutoff = frand(r, 0.25f, 0.5f); p.envFilterAmt = frand(r, 0.05f, 0.3f);
            p.fxChorus = frand(r, 0.2f, 0.5f); p.fxReverb = frand(r, 0.4f, 0.6f); p.fxDelay = frand(r, 0.2f, 0.45f);
            p.category = 8; break;
    }
    p.env[2] = fe; // aux env: mirror the filter env as a usable default

    // --- modulation: 1-3 musical routes (vibrato / filter motion / tremolo) ----
    for(auto& m : p.mod) { m.source = 0; m.dest = 0; m.amount = 0.0f; }
    int slot = 0;
    p.lfo[0].rate = frand(r, 3.5f, 6.5f); p.lfo[0].depth = 1.0f;   // vibrato LFO
    p.lfo[1].rate = frand(r, 0.05f, 0.8f); p.lfo[1].depth = 1.0f;  // slow sweep LFO
    if(r.nextInt(10) < 6) { p.mod[slot++] = {1, 1, frand(r, 0.01f, 0.05f)}; }   // LFO1 -> PITCH
    if(r.nextInt(10) < 7) { p.mod[slot++] = {2, 13, frand(r, 0.1f, 0.45f)}; }   // LFO2 -> CUTOFF
    if(r.nextInt(10) < 3) { p.mod[slot++] = {7, 13, frand(r, 0.15f, 0.5f)}; }   // VEL -> CUTOFF

    // --- keep output civilised --------------------------------------------------
    p.ampGain = 0.7f;
    p.fxDrive = r.nextInt(10) < 3 ? frand(r, 0.1f, 0.35f) : 0.0f;
    p.driveMode = r.nextInt(3);
    p.width = frand(r, 0.4f, 0.75f);
    p.arp.on = false;
    p.patchbayOn = false;

    const auto name = juce::String(kNameA[r.nextInt((int) std::size(kNameA))]) + " "
                    + kNameB[r.nextInt((int) std::size(kNameB))];
    applyPatch(p, name, p.category, /*clean*/ false); // unsaved until the user saves it
    current = -1;
}

void PresetManager::randomizeVary()
{
    if(apvts == nullptr || binding == nullptr)
        return;
    auto& r = juce::Random::getSystemRandom();

    SynthPatch p;
    binding->readInto(p); // perturb what's playing right now

    auto jit = [&r](float v, float amt, float lo, float hi)
    { return juce::jlimit(lo, hi, v + (r.nextFloat() * 2.0f - 1.0f) * amt); };

    for(auto& o : p.osc)
    {
        if(!o.on) continue;
        o.morph   = jit(o.morph, 0.08f, 0.0f, 1.0f);
        o.pw      = jit(o.pw, 0.06f, 0.05f, 0.95f);
        o.detune  = jit(o.detune, 0.03f, -0.5f, 0.5f);
        o.wtMorph = jit(o.wtMorph, 0.08f, 0.0f, 1.0f);
        o.level   = jit(o.level, 0.06f, 0.1f, 1.0f);
    }
    p.cutoff       = jit(p.cutoff, 0.07f, 0.05f, 1.0f);
    p.resonance    = jit(p.resonance, 0.06f, 0.0f, 0.9f);
    p.envFilterAmt = jit(p.envFilterAmt, 0.07f, -1.0f, 1.0f);
    for(auto& e : p.env)
    {
        e.attack  = jit(e.attack,  0.15f * e.attack + 0.004f, 0.001f, 8.0f);
        e.decay   = jit(e.decay,   0.15f * e.decay + 0.01f,  0.01f,  8.0f);
        e.sustain = jit(e.sustain, 0.06f, 0.0f, 1.0f);
        e.release = jit(e.release, 0.15f * e.release + 0.01f, 0.01f, 8.0f);
    }
    for(auto& l : p.lfo)
    {
        l.rate  = l.sync ? l.rate : jit(l.rate, 0.12f * l.rate + 0.02f, 0.02f, 20.0f);
        l.depth = jit(l.depth, 0.08f, 0.0f, 2.0f);
    }
    p.fxChorus = jit(p.fxChorus, 0.06f, 0.0f, 1.0f);
    p.fxDelay  = jit(p.fxDelay,  0.06f, 0.0f, 0.6f);
    p.fxReverb = jit(p.fxReverb, 0.06f, 0.0f, 0.7f);
    p.drift    = jit(p.drift, 0.06f, 0.0f, 1.0f);

    applyPatch(p, currentName(), currentCategory(), /*clean*/ false); // edited by definition
}

bool PresetManager::deleteUserPreset(const juce::String& name)
{
    auto file = userPresetDir().getChildFile(sanitise(name) + ".jove");
    if(file.existsAsFile() && file.deleteFile())
    {
        rescan();
        return true;
    }
    return false;
}

int PresetManager::indexOfName(const juce::String& name) const
{
    for(int i = 0; i < (int) catalogue.size(); ++i)
        if(catalogue[(size_t) i].name == name)
            return i;
    return -1;
}

juce::String PresetManager::currentName() const
{
    if(apvts != nullptr && apvts->state.hasProperty(kNameProp))
        return apvts->state.getProperty(kNameProp).toString();
    return "INIT";
}

int PresetManager::currentCategory() const
{
    if(apvts != nullptr && apvts->state.hasProperty(kCatProp))
        return (int) apvts->state.getProperty(kCatProp);
    return 0;
}

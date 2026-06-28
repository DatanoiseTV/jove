/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#include "PresetManager.h"

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

void PresetManager::applyPatch(const SynthPatch& p, const juce::String& name, int category)
{
    if(binding == nullptr)
        return;
    binding->writeFrom(p);
    setNameCat(name, category);
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
            current = catalogueIndex;
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
        return true;
    }
    return false;
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

bool PresetManager::currentIsDirty() const
{
    return false; // wired in the UI phase (compare against load-time snapshot)
}

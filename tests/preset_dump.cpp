/*
  Jove — dump every factory preset's live APVTS parameter values to JSON.

  Used by the demo-video tooling: the JSON drives the WebView UI harness so each
  patch can be screenshotted with the real plugin UI reflecting its parameters.
  For each preset we emit, per parameter id: normalised value (0..1), the scaled
  engineering value (for controls the UI shows scaled), the choice index (combos)
  and the bool (toggles). Prints a JSON array to stdout.
*/

#include "PluginProcessor.h"
#include <cstdio>

using namespace jove;

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    JoveAudioProcessor proc;
    auto& pm = proc.getPresetManager();

    juce::Array<juce::var> presets;
    for(int i = 0; i < kNumFactoryPresets; ++i)
    {
        pm.loadFactory(i);

        auto* obj = new juce::DynamicObject();
        obj->setProperty("index", i);
        obj->setProperty("name", juce::String(FactoryPresetName(i)));
        obj->setProperty("category", FactoryPresetCategory(i));

        auto* sliders = new juce::DynamicObject();
        auto* scaled  = new juce::DynamicObject();
        auto* choices = new juce::DynamicObject();
        auto* toggles = new juce::DynamicObject();

        for(auto* param : proc.getParameters())
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param);
            if(ranged == nullptr) continue;
            const auto id = ranged->getParameterID();

            if(auto* ch = dynamic_cast<juce::AudioParameterChoice*>(param))
                choices->setProperty(id, ch->getIndex());
            else if(auto* bp = dynamic_cast<juce::AudioParameterBool*>(param))
                toggles->setProperty(id, bp->get());
            else
            {
                sliders->setProperty(id, (double) ranged->getValue()); // normalised 0..1
                scaled->setProperty(id, (double) ranged->convertFrom0to1(ranged->getValue()));
            }
        }
        obj->setProperty("sliders", juce::var(sliders));
        obj->setProperty("scaled",  juce::var(scaled));
        obj->setProperty("choices", juce::var(choices));
        obj->setProperty("toggles", juce::var(toggles));
        presets.add(juce::var(obj));
    }

    auto json = juce::JSON::toString(juce::var(presets), false);
    printf("%s\n", json.toRawUTF8());
    return 0;
}

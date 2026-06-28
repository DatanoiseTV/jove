/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#include "WebEditor.h"
#include "../PluginProcessor.h"

#include <JoveUIData.h>
#include <optional>

#ifndef JOVE_VERSION
 #define JOVE_VERSION "dev"
#endif

namespace jove
{
namespace
{
// ---- BinaryData-backed resource provider (serves the UI bundle) ----
struct ResourceTable
{
    struct Entry { juce::String filename; const char* data; int size; };
    std::vector<Entry> entries;

    ResourceTable()
    {
        for(int i = 0; i < JoveUIData::namedResourceListSize; ++i)
        {
            const char* name = JoveUIData::namedResourceList[i];
            int size = 0;
            const char* data = JoveUIData::getNamedResource(name, size);
            if(data == nullptr) continue;
            entries.push_back({JoveUIData::getNamedResourceOriginalFilename(name), data, size});
        }
    }

    static juce::String mimeFor(const juce::String& n)
    {
        if(n.endsWith(".html")) return "text/html";
        if(n.endsWith(".css"))  return "text/css";
        if(n.endsWith(".js") || n.endsWith(".jsx") || n.endsWith(".mjs")) return "application/javascript";
        if(n.endsWith(".svg"))  return "image/svg+xml";
        if(n.endsWith(".png"))  return "image/png";
        if(n.endsWith(".json")) return "application/json";
        return "application/octet-stream";
    }

    std::optional<juce::WebBrowserComponent::Resource> lookup(const juce::String& url) const
    {
        juce::String name = url.startsWithChar('/') ? url.substring(1) : url;
        if(name.isEmpty()) name = "index.html";
        const auto slash = name.lastIndexOfChar('/');
        if(slash >= 0) name = name.substring(slash + 1);
        const auto q = name.indexOfChar('?');
        if(q >= 0) name = name.substring(0, q);

        for(const auto& e : entries)
            if(e.filename == name)
            {
                juce::WebBrowserComponent::Resource r;
                r.data.assign(reinterpret_cast<const std::byte*>(e.data),
                              reinterpret_cast<const std::byte*>(e.data) + (size_t) e.size);
                r.mimeType = mimeFor(name);
                return r;
            }
        return std::nullopt;
    }
};

ResourceTable& resourceTable()
{
    static ResourceTable t;
    return t;
}
} // namespace

JoveWebEditor::JoveWebEditor(JoveAudioProcessor& proc)
    : juce::AudioProcessorEditor(&proc), processor(proc)
{
    // Lock the editor to the UI's design aspect ratio (1500x1000 = 3:2) so the
    // host window is always the right shape and the WebView's scale-to-fit fills
    // it edge-to-edge with no letterboxing.
    setResizable(true, true);
    if(auto* c = getConstrainer())
    {
        c->setFixedAspectRatio(1500.0 / 1000.0);
        c->setSizeLimits(750, 500, 3000, 2000);
    }

    auto& apvts = processor.getValueTreeState();

    juce::WebBrowserComponent::Options options;
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::defaultBackend)
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withNativeIntegrationEnabled(true)
        .withResourceProvider(
            [](const juce::String& url) { return resourceTable().lookup(url); },
            juce::URL(juce::WebBrowserComponent::getResourceProviderRoot()).getOrigin())
        .withUserScript("window.JOVE_VERSION_STR = 'v" JOVE_VERSION "';")
        .withNativeFunction(juce::Identifier{"presetPrev"},
            [this](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion c)
            { processor.getPresetManager().prev(); c(juce::var()); })
        .withNativeFunction(juce::Identifier{"presetNext"},
            [this](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion c)
            { processor.getPresetManager().next(); c(juce::var()); })
        .withNativeFunction(juce::Identifier{"listPresets"},
            [this](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion c)
            { c(presetCatalogue()); })
        .withNativeFunction(juce::Identifier{"loadPreset"},
            [this](const juce::Array<juce::var>& a, juce::WebBrowserComponent::NativeFunctionCompletion c)
            {
                if(a.size() > 0) processor.getPresetManager().load((int) a[0]);
                c(juce::var());
            })
        .withNativeFunction(juce::Identifier{"savePreset"},
            [this](const juce::Array<juce::var>& a, juce::WebBrowserComponent::NativeFunctionCompletion c)
            {
                if(a.size() >= 1)
                {
                    const auto name = a[0].toString().trim();
                    const int cat = a.size() >= 2 ? (int) a[1] : 0;
                    if(name.isNotEmpty())
                        processor.getPresetManager().saveUserPreset(name, cat);
                }
                c(juce::var());
            })
        .withNativeFunction(juce::Identifier{"initPatch"},
            [this](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion c)
            { processor.getPresetManager().loadInit(); c(juce::var()); });

    // ---- generic relay creation: one per parameter, typed by its class ----
    for(auto* param : apvts.processor.getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param);
        if(ranged == nullptr) continue;
        const auto id = ranged->getParameterID();

        if(dynamic_cast<juce::AudioParameterChoice*>(param) != nullptr)
        {
            ComboBinding b;
            b.relay = std::make_unique<juce::WebComboBoxRelay>(id);
            options = options.withOptionsFrom(*b.relay);
            comboBindings.push_back(std::move(b));
        }
        else if(dynamic_cast<juce::AudioParameterBool*>(param) != nullptr)
        {
            ToggleBinding b;
            b.relay = std::make_unique<juce::WebToggleButtonRelay>(id);
            options = options.withOptionsFrom(*b.relay);
            toggleBindings.push_back(std::move(b));
        }
        else
        {
            SliderBinding b;
            b.relay = std::make_unique<juce::WebSliderRelay>(id);
            options = options.withOptionsFrom(*b.relay);
            sliderBindings.push_back(std::move(b));
        }
    }

    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView);

    // attach each relay to its parameter (index order matches creation order)
    {
        size_t si = 0, ti = 0, ci = 0;
        for(auto* param : apvts.processor.getParameters())
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param);
            if(ranged == nullptr) continue;
            if(dynamic_cast<juce::AudioParameterChoice*>(param) != nullptr)
                comboBindings[ci].attach = std::make_unique<juce::WebComboBoxParameterAttachment>(*ranged, *comboBindings[ci].relay, apvts.undoManager), ++ci;
            else if(dynamic_cast<juce::AudioParameterBool*>(param) != nullptr)
                toggleBindings[ti].attach = std::make_unique<juce::WebToggleButtonParameterAttachment>(*ranged, *toggleBindings[ti].relay, apvts.undoManager), ++ti;
            else
                sliderBindings[si].attach = std::make_unique<juce::WebSliderParameterAttachment>(*ranged, *sliderBindings[si].relay, apvts.undoManager), ++si;
        }
    }

    setSize(1500, 1000);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz(30);
}

JoveWebEditor::~JoveWebEditor() { stopTimer(); }

void JoveWebEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour(0xff0b0d12)); }

void JoveWebEditor::resized()
{
    if(webView != nullptr)
        webView->setBounds(getLocalBounds());
}

juce::var JoveWebEditor::presetCatalogue() const
{
    juce::Array<juce::var> arr;
    const auto& entries = processor.getPresetManager().entries();
    for(int i = 0; i < (int) entries.size(); ++i)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty("index", i);
        o->setProperty("name", entries[(size_t) i].name);
        o->setProperty("category", entries[(size_t) i].category);
        o->setProperty("factory", entries[(size_t) i].factory);
        arr.add(juce::var(o));
    }
    return juce::var(arr);
}

void JoveWebEditor::timerCallback()
{
    emitMeters();
    emitPresetInfo();
}

void JoveWebEditor::emitMeters()
{
    if(webView == nullptr) return;
    auto* o = new juce::DynamicObject();
    o->setProperty("voices", processor.getActiveVoices());
    o->setProperty("outL", processor.getOutputLevel(0));
    o->setProperty("outR", processor.getOutputLevel(1));
    o->setProperty("note", processor.getLastMidiNote());
    juce::Array<juce::var> lfos, lfoPh;
    for(int i = 0; i < jove::kNumLfo; ++i)
    {
        lfos.add(processor.getLfoValue(i));
        lfoPh.add(processor.getLfoPhase(i));
    }
    o->setProperty("lfo", juce::var(lfos));
    o->setProperty("lfoPhase", juce::var(lfoPh));
    juce::Array<juce::var> vl;
    for(int i = 0; i < jove::kMaxVoices; ++i)
        vl.add(processor.getVoiceLevel(i));
    o->setProperty("voiceLevels", juce::var(vl));
    webView->emitEventIfBrowserIsVisible("meters", juce::var(o));
}

void JoveWebEditor::emitPresetInfo()
{
    if(webView == nullptr) return;
    auto& pm = processor.getPresetManager();
    const auto name = pm.currentName();
    const int idx = pm.currentIndex();
    if(name == lastPresetName && idx == lastPresetIndex) return;
    lastPresetName = name;
    lastPresetIndex = idx;
    auto* o = new juce::DynamicObject();
    o->setProperty("name", name);
    o->setProperty("category", pm.currentCategory());
    o->setProperty("index", idx);
    webView->emitEventIfBrowserIsVisible("preset", juce::var(o));
}
} // namespace jove

/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <vector>

class JoveAudioProcessor;

namespace jove
{
// WebView editor. Every APVTS parameter is two-way bound to a DOM control via a
// relay + attachment, created generically from the parameter's type (choice ->
// combo, bool -> toggle, else slider) so the C++ side never hand-lists IDs. Live
// metering + preset state are pushed to JS on a 30 Hz timer. Relays are declared
// before the WebView so they are destroyed AFTER it (the browser walks its
// listener list on teardown — see Doobie's WebEditor for the war story).
class JoveWebEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
  public:
    explicit JoveWebEditor(JoveAudioProcessor& proc);
    ~JoveWebEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

  private:
    void timerCallback() override;
    void emitMeters();
    void emitPresetInfo();
    juce::var presetCatalogue() const;

    JoveAudioProcessor& processor;

    struct SliderBinding { std::unique_ptr<juce::WebSliderRelay>       relay; std::unique_ptr<juce::WebSliderParameterAttachment>       attach; };
    struct ToggleBinding { std::unique_ptr<juce::WebToggleButtonRelay> relay; std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach; };
    struct ComboBinding  { std::unique_ptr<juce::WebComboBoxRelay>     relay; std::unique_ptr<juce::WebComboBoxParameterAttachment>     attach; };

    std::vector<SliderBinding> sliderBindings;
    std::vector<ToggleBinding> toggleBindings;
    std::vector<ComboBinding>  comboBindings;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    juce::String lastPresetName;
    int          lastPresetIndex = -2;
    // Force re-announcing the current preset for the first ~1.3 s after the editor
    // opens, so a freshly-loaded WebView (which may subscribe after the first
    // emit) gets the name instead of staying on its default "INIT".
    int          presetReannounce = 40;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JoveWebEditor)
};
} // namespace jove

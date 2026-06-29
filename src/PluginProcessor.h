/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "JoveParams.h"
#include "engine/SynthEngine.h"
#include "presets/PresetManager.h"
#include <array>
#include <atomic>
#include <memory>

// Top-level instrument plugin. Owns the APVTS parameter tree, a live SynthPatch
// the engine reads each block, and the jove::SynthEngine voice pool + FX chain.
//
// Timing model: the engine is a fixed control-block design — its LFOs and arp
// advance exactly once per render() call, stepping by rate/blockRate where
// blockRate is pinned at prepare(). To keep modulation rates correct no matter
// what buffer size the host hands us, processBlock renders in fixed kControlBlock
// chunks and carries the unread tail of a chunk across callbacks. MIDI events are
// applied at chunk boundaries (<= kControlBlock samples of quantisation).
class JoveAudioProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener
{
  public:
    JoveAudioProcessor();
    ~JoveAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Jove"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }
    PresetManager& getPresetManager() { return presetManager; }

    // Test seam: read the live parameter tree back into a patch (used by the
    // round-trip audit to prove the APVTS<->SynthPatch mapping is faithful).
    jove::SynthPatch readLivePatch() const
    {
        jove::SynthPatch p;
        binding.readInto(p);
        return p;
    }

    // ---- UI metering (lock-free atomics, published once per processBlock) ----
    int   getActiveVoices() const { return activeVoices.load(std::memory_order_relaxed); }
    float getVoiceLevel(int i) const { return (i >= 0 && i < jove::kMaxVoices) ? voiceLevelUI[(size_t) i].load(std::memory_order_relaxed) : 0.0f; }
    float getOutputLevel(int ch) const { return outputLevel[(size_t) juce::jlimit(0, 1, ch)].load(std::memory_order_relaxed); }
    float getLfoValue(int i) const { return (i >= 0 && i < jove::kNumLfo) ? lfoValueUI[(size_t) i].load(std::memory_order_relaxed) : 0.0f; }
    float getLfoPhase(int i) const { return (i >= 0 && i < jove::kNumLfo) ? lfoPhaseUI[(size_t) i].load(std::memory_order_relaxed) : 0.0f; }
    static constexpr int kNumModSources = (int) jove::ModSource::Count;
    float getModSourceValue(int i) const { return (i >= 0 && i < kNumModSources) ? modSrcUI[(size_t) i].load(std::memory_order_relaxed) : 0.0f; }
    int   getLastMidiNote() const { return lastMidiNote.load(std::memory_order_relaxed); }

  private:
    void handleMidiMessage(const juce::MidiMessage& m) noexcept;
    void publishMeters(const float* l, const float* r, int n) noexcept;
    // Render exactly `n` samples (at the engine's internal/oversampled rate) into
    // L/R, applying MIDI events whose (base-rate) position * midiScale falls in
    // range. Shared by the 1x and oversampled paths.
    void renderInto(float* l, float* r, int n, juce::MidiBuffer& midi, int midiScale) noexcept;
    void parameterChanged(const juce::String& id, float newValue) override;
    void rebuildOversampling(); // (re)builds for the current quality; (re)prepares the engine

    juce::AudioProcessorValueTreeState apvts;
    jove::PatchBinding binding;
    jove::SynthEngine engine;
    jove::SynthPatch patch;
    PresetManager presetManager;

    // fixed control-block scratch, carried across callbacks (engine-rate samples).
    // 32 (was 64) tightens arp/LFO timing to ~0.7 ms granularity on desktop.
    static constexpr int kControlBlock = 32;
    std::array<float, kControlBlock> scratchL {};
    std::array<float, kControlBlock> scratchR {};
    int controlRemaining = 0; // unread samples left in the scratch chunk
    int scratchPos       = 0;
    juce::AudioBuffer<float> work; // stereo base-rate render target (Eco path)

    // Desktop quality: oversample the whole voice path so sync/FM/ring/saturation
    // don't alias. factor 1 = Eco (bypass), 2 = HQ, 4 = Ultra. Rebuilt off the
    // audio thread on a Quality change; renderLock fences the swap.
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    int osFactor = 1;
    juce::CriticalSection renderLock;
    std::atomic<int> pendingQuality { -1 };
    std::atomic<bool> presetLoadPending { false }; // set on preset load -> release orphaned voices

    double sampleRate = 44100.0;
    int    hostBlockSize = 512;

    // MPE config (global, read each block from the APVTS; not patch state)
    bool mpeOn_        = false;
    int  mpeBendRange_ = 48;

    std::atomic<int>   activeVoices { 0 };
    std::atomic<int>   lastMidiNote { -1 };
    std::array<std::atomic<float>, 2> outputLevel {};
    std::array<std::atomic<float>, jove::kNumLfo> lfoValueUI {};
    std::array<std::atomic<float>, jove::kNumLfo> lfoPhaseUI {};
    std::array<std::atomic<float>, jove::kMaxVoices> voiceLevelUI {};
    std::array<std::atomic<float>, (size_t) jove::ModSource::Count> modSrcUI {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JoveAudioProcessor)
};

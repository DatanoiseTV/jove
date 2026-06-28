/*
  Jove — analog-inspired polysynth (JUCE instrument plugin)
  Copyright (C) 2026 DatanoiseTV

  GPL-3.0-or-later, WITHOUT ANY WARRANTY. Retain attribution to DatanoiseTV.
*/

#include "PluginProcessor.h"
#include "ui/WebEditor.h"

using namespace jove;

JoveAudioProcessor::JoveAudioProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "JOVE", createJoveLayout())
{
    binding.connect(apvts);
    presetManager.init(apvts, binding);
    presetManager.setLoadCallback([this] { panicPending.store(true, std::memory_order_relaxed); });
    InitDefaultPatch(patch);
    apvts.addParameterListener(jID::quality, this);
}

JoveAudioProcessor::~JoveAudioProcessor()
{
    apvts.removeParameterListener(jID::quality, this);
}

bool JoveAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if(out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet().isDisabled();
}

void JoveAudioProcessor::rebuildOversampling()
{
    // map Quality (Eco/HQ/Ultra) -> oversampling factor 1/2/4
    const int q = (int) apvts.getRawParameterValue(jID::quality)->load();
    osFactor = (q <= 0) ? 1 : (q == 1 ? 2 : 4);

    const juce::ScopedLock sl(renderLock);
    if(osFactor > 1)
    {
        const size_t log2 = (osFactor == 2) ? 1 : 2;
        oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
            2, log2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        oversampling->initProcessing((size_t) juce::jmax(1, hostBlockSize));
        oversampling->reset();
        setLatencySamples((int) oversampling->getLatencyInSamples());
    }
    else
    {
        oversampling.reset();
        setLatencySamples(0);
    }
    engine.prepare((float) (sampleRate * osFactor), kControlBlock);
    engine.setPatch(&patch);
    controlRemaining = 0;
    scratchPos       = 0;
}

void JoveAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate    = sr;
    hostBlockSize = samplesPerBlock;
    work.setSize(2, juce::jmax(1, samplesPerBlock), false, false, true);
    rebuildOversampling();
}

void JoveAudioProcessor::parameterChanged(const juce::String& id, float)
{
    if(id == jID::quality)
        pendingQuality.store(1, std::memory_order_relaxed);
}

void JoveAudioProcessor::handleMidiMessage(const juce::MidiMessage& m) noexcept
{
    if(m.isNoteOn())
    {
        engine.noteOn(m.getNoteNumber(), m.getFloatVelocity());
        lastMidiNote.store(m.getNoteNumber(), std::memory_order_relaxed);
    }
    else if(m.isNoteOff())
        engine.noteOff(m.getNoteNumber());
    else if(m.isAllNotesOff() || m.isAllSoundOff())
        engine.allNotesOff();
    else if(m.isPitchWheel())
        engine.pitchBend((m.getPitchWheelValue() - 8192) / 8192.0f);
    else if(m.isController())
    {
        const int cc = m.getControllerNumber();
        if(cc == 1)        engine.modWheel(m.getControllerValue() / 127.0f);
        else if(cc == 64)  engine.sustainPedal(m.getControllerValue() >= 64);
        else if(cc == 123) engine.allNotesOff();
    }
    else if(m.isChannelPressure())
        engine.aftertouch(m.getChannelPressureValue() / 127.0f);
    else if(m.isAftertouch())
        engine.aftertouch(m.getAfterTouchValue() / 127.0f);
}

void JoveAudioProcessor::renderInto(float* l, float* r, int n, juce::MidiBuffer& midi,
                                    int midiScale) noexcept
{
    auto midiIt = midi.cbegin();
    const auto midiEnd = midi.cend();
    for(int s = 0; s < n; ++s)
    {
        if(controlRemaining == 0)
        {
            while(midiIt != midiEnd && (*midiIt).samplePosition * midiScale <= s)
            {
                handleMidiMessage((*midiIt).getMessage());
                ++midiIt;
            }
            engine.render(scratchL.data(), scratchR.data(), kControlBlock);
            scratchPos       = 0;
            controlRemaining = kControlBlock;
        }
        l[s] = scratchL[(size_t) scratchPos];
        r[s] = scratchR[(size_t) scratchPos];
        ++scratchPos;
        --controlRemaining;
    }
    // drain trailing controller events
    while(midiIt != midiEnd)
    {
        handleMidiMessage((*midiIt).getMessage());
        ++midiIt;
    }
}

void JoveAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // apply a pending Quality change (allocates) on the audio thread only when the
    // lock is free — the listener just flags it. A momentary glitch on switch is
    // acceptable; it never runs during a render because of renderLock below.
    if(pendingQuality.exchange(0, std::memory_order_relaxed) == 1)
        rebuildOversampling();

    const juce::ScopedLock sl(renderLock);

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();

    if(auto* ph = getPlayHead())
        if(auto pos = ph->getPosition())
            if(auto bpm = pos->getBpm())
                engine.setTempo((float) *bpm);

    // a preset just loaded -> gate every voice off so held/stuck notes don't
    // bleed into the new patch.
    if(panicPending.exchange(false, std::memory_order_relaxed))
        engine.allNotesOff();

    binding.readInto(patch);
    engine.setPatch(&patch);
    engine.setMaxVoices((int) apvts.getRawParameterValue(jID::maxVoices)->load());

    if(osFactor <= 1 || oversampling == nullptr || numCh < 2)
    {
        // Eco (or mono output): render the stereo engine into the work buffer,
        // then copy out. Mono collapses to the left channel.
        float* wL = work.getWritePointer(0);
        float* wR = work.getWritePointer(1);
        renderInto(wL, wR, numSamples, midi, 1);
        buffer.copyFrom(0, 0, wL, numSamples);
        if(numCh > 1)
            buffer.copyFrom(1, 0, wR, numSamples);
        publishMeters(wL, numCh > 1 ? wR : nullptr, numSamples);
        return;
    }

    // HQ/Ultra: render at the oversampled rate into the up-sampled block, then
    // decimate back down through JUCE's half-band filters.
    buffer.clear();
    juce::dsp::AudioBlock<float> base(buffer);
    auto up = oversampling->processSamplesUp(base); // up.getNumSamples() == numSamples*osFactor
    const int upN = (int) up.getNumSamples();
    float* upL = up.getChannelPointer(0);
    float* upR = up.getNumChannels() > 1 ? up.getChannelPointer(1) : upL;
    renderInto(upL, upR, upN, midi, osFactor);
    oversampling->processSamplesDown(base);

    publishMeters(buffer.getWritePointer(0),
                  numCh > 1 ? buffer.getWritePointer(1) : nullptr, numSamples);
}

void JoveAudioProcessor::publishMeters(const float* l, const float* r, int n) noexcept
{
    float peakL = 0.0f, peakR = 0.0f;
    for(int i = 0; i < n; ++i)
    {
        peakL = juce::jmax(peakL, std::abs(l[i]));
        if(r != nullptr)
            peakR = juce::jmax(peakR, std::abs(r[i]));
    }
    outputLevel[0].store(peakL, std::memory_order_relaxed);
    outputLevel[1].store(r != nullptr ? peakR : peakL, std::memory_order_relaxed);
    activeVoices.store(engine.activeVoices(), std::memory_order_relaxed);
    for(int i = 0; i < kMaxVoices; ++i)
        voiceLevelUI[(size_t) i].store(engine.voiceLevel(i), std::memory_order_relaxed);
    for(int i = 0; i < kNumModSources; ++i)
        modSrcUI[(size_t) i].store(engine.modSourceValue(i), std::memory_order_relaxed);
    for(int i = 0; i < kNumLfo; ++i)
    {
        lfoValueUI[(size_t) i].store(engine.lfoValue(i), std::memory_order_relaxed);
        lfoPhaseUI[(size_t) i].store(engine.lfoPhase(i), std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* JoveAudioProcessor::createEditor()
{
    return new jove::JoveWebEditor(*this);
}

void JoveAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if(auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void JoveAudioProcessor::setStateInformation(const void* data, int size)
{
    if(auto xml = getXmlFromBinary(data, size))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JoveAudioProcessor();
}

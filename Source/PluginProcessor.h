#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SynthEngine.h"
#include "Effects.h"

// PANIC RAW TEK — procesador
class RawTekProcessor : public juce::AudioProcessor
{
public:
    RawTekProcessor();
    ~RawTekProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Panic Raw Tek"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::StringArray divisionNames();
    static juce::StringArray modSourceNames();
    static juce::StringArray modDestNames();

    // Presets
    static juce::File getPresetFolder();
    bool loadPresetFile (const juce::File&);
    bool loadPresetXml (const juce::XmlElement&, const juce::String& fallbackName);
    static int getNumFactoryPrograms();
    static juce::String getProgramShortName (int index);
    static juce::String getProgramCategory (int index);
    bool savePresetFile (const juce::File&, const juce::String& name, const juce::String& category);
    void resetParametersToDefault();
    void setCurrentPresetName (const juce::String&);
    juce::String getCurrentPresetName() const;

    juce::AudioProcessorValueTreeState apvts;

    // Para la UI
    static constexpr int kScopeSize = 4096;
    std::array<float, kScopeSize> scopeData {};
    std::atomic<int> scopeWritePos { 0 };
    std::atomic<int> activeVoices { 0 };
    std::atomic<float> outputPeak { 0.0f };

private:
    void updateEngineParams();
    float getP (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    rawtek::Engine engine;
    rawtek::Distortion distL, distR;
    rawtek::Chorus chorus;
    rawtek::DubDelay delay;
    rawtek::FdnReverb reverb;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    juce::AudioBuffer<float> work;
    int maxBlock = 512;
    double hostBpm = 120.0;
    double sampleRateHz = 44100.0;
    float dcX1[2] {}, dcY1[2] {};
    float lastGain = 0.5f;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RawTekProcessor)
};

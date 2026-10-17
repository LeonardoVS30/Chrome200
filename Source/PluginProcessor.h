#pragma once

#include <JuceHeader.h>
#include "Parameters/ParameterLayout.h"
#include "SampleEngine/SampleEngine.h"
#include "PresetManager/PresetManager.h"

class Chrome2000AudioProcessor final : public juce::AudioProcessor
{
public:
    Chrome2000AudioProcessor();
    ~Chrome2000AudioProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    chrome::SampleEngine engine;
    chrome::PresetManager presets;

private:
    bool powerWasOn = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chrome2000AudioProcessor)
};


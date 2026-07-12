#pragma once

#include "SampleLoader.h"
#include "../Engine/Voice.h"
#include "../DSP/VintageProcessor.h"
#include "../DSP/ChromeProcessor.h"
#include "../DSP/DigitalProcessor.h"
#include "../Effects/MasterEffects.h"
#include "../Parameters/ParameterLayout.h"

namespace chrome
{
class SampleEngine final : private juce::AsyncUpdater
{
public:
    explicit SampleEngine (juce::AudioProcessorValueTreeState&);
    ~SampleEngine() override;
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void loadSample (int slot, const juce::File&);
    void restoreSamplePath (int slot, const juce::String& path);
    [[nodiscard]] std::shared_ptr<const SampleData> sample (int slot) const;
    [[nodiscard]] juce::String samplePath (int slot) const;
    [[nodiscard]] float outputPeak() const noexcept { return peak.load (std::memory_order_relaxed); }
    std::function<void (int, const juce::String&)> sampleChanged;

private:
    void handleAsyncUpdate() override;
    struct ModulationValues
    {
        float pitch = 0.0f, filter = 0.0f, position = 0.0f, grain = 0.0f, density = 0.0f;
        float mix = 0.0f, fx = 0.0f, liquid = 0.0f, reflect = 0.0f, shine = 0.0f;
    };
    std::array<SlotPlayback, 3> makeSlotSettings (const ModulationValues&) const;
    ModulationValues evaluateModulation (int numSamples);
    void noteOn (int note, float velocity, const std::array<SlotPlayback, 3>&);
    void noteOff (int note);
    void processFilter (juce::AudioBuffer<float>&, float modulation);

    juce::AudioProcessorValueTreeState& state;
    struct SlotParameterPointers
    {
        std::atomic<float>* enabled = nullptr; std::atomic<float>* gain = nullptr; std::atomic<float>* tune = nullptr;
        std::atomic<float>* fine = nullptr; std::atomic<float>* start = nullptr; std::atomic<float>* end = nullptr;
        std::atomic<float>* pan = nullptr; std::atomic<float>* loop = nullptr; std::atomic<float>* reverse = nullptr;
        std::atomic<float>* solo = nullptr; std::atomic<float>* mute = nullptr; std::atomic<float>* mode = nullptr;
    };
    std::array<SlotParameterPointers, 3> slotParameters;
    juce::AudioFormatManager formats;
    SampleLoader loader;
    std::array<std::atomic<std::shared_ptr<const SampleData>>, 3> samples;
    std::array<juce::String, 3> paths, errors;
    mutable juce::CriticalSection metadataLock;
    std::array<Voice, 128> voices;
    std::array<juce::AudioBuffer<float>, 3> slotBuffers;
    std::array<VintageProcessor, 3> vintage;
    std::array<ChromeProcessor, 3> chrome;
    std::array<DigitalProcessor, 3> digital;
    std::array<std::array<juce::dsp::IIR::Filter<float>, 2>, 2> filterStages;
    MasterEffects master;
    double rate = 44100.0;
    uint64_t voiceCounter = 0;
    std::array<double, 3> lfoPhase {};
    float lastVelocity = 0.0f, aftertouch = 0.0f, randomModulation = 0.0f;
    uint32_t modulationRandom = 0xa19f2000u;
    std::atomic<float> peak { 0.0f };
};
}

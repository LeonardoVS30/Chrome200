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
    void writeSampleState (juce::ValueTree&) const;
    void restoreSampleState (const juce::ValueTree&);
    // Message-thread flush for headless clients that do not run a dispatch loop.
    void flushPendingSampleUpdate() { handleUpdateNowIfNeeded(); }
    [[nodiscard]] std::shared_ptr<const SampleData> sample (int slot) const;
    [[nodiscard]] juce::String samplePath (int slot) const;
    [[nodiscard]] float detectedRootPitch (int slot) const noexcept;
    [[nodiscard]] float rootPitch (int slot) const noexcept;
    [[nodiscard]] float outputPeak() const noexcept { return peak.load (std::memory_order_relaxed); }
    void setBpm (float bpm) noexcept { currentBpm.store (juce::jlimit (20.0f, 400.0f, bpm), std::memory_order_relaxed); }
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
    void processFilter (juce::AudioBuffer<float>&, float modulation, float filterEnv, int keytrackNote, float envAmount, float keytrackAmount);

    juce::AudioProcessorValueTreeState& state;
    struct SlotParameterPointers
    {
        std::atomic<float>* enabled = nullptr; std::atomic<float>* gain = nullptr; std::atomic<float>* tune = nullptr;
        std::atomic<float>* rootPitch = nullptr;
        std::atomic<float>* fine = nullptr; std::atomic<float>* start = nullptr; std::atomic<float>* end = nullptr;
        std::atomic<float>* pan = nullptr; std::atomic<float>* loop = nullptr; std::atomic<float>* reverse = nullptr;
        std::atomic<float>* solo = nullptr; std::atomic<float>* mute = nullptr; std::atomic<float>* mode = nullptr;
    };
    std::array<SlotParameterPointers, 3> slotParameters;
    juce::AudioFormatManager formats;
    std::array<std::atomic<std::shared_ptr<const SampleData>>, 3> samples;
    std::array<juce::String, 3> paths, errors;
    std::array<std::atomic<float>, 3> detectedRootPitches {};
    std::array<std::atomic<float>, 3> pitchConfidences {};
    std::array<std::atomic<uint64_t>, 3> sampleLoadGenerations {};
    std::array<bool, 3> applyDetectedTuning {};
    mutable juce::CriticalSection metadataLock;
    // Declared after every object touched by its callback so its worker thread
    // is stopped before those callback targets are destroyed.
    SampleLoader loader;
    std::array<Voice, 128> voices;
    std::array<juce::AudioBuffer<float>, 3> slotBuffers;
    std::array<juce::AudioBuffer<float>, 3> modeDryBuffers;
    std::array<VintageProcessor, 3> vintage;
    std::array<ChromeProcessor, 3> chrome;
    std::array<DigitalProcessor, 3> digital;
    std::array<int, 3> lastModes { -1, -1, -1 };
    std::array<int, 3> modeFadeSamples {};
    std::array<bool, 3> characterInputActive {};
    std::array<std::array<juce::dsp::IIR::Filter<float>, 2>, 2> filterStages;
    MasterEffects master;
    double rate = 44100.0;
    uint64_t voiceCounter = 0;
    juce::ADSR filterEnvelope, modEnvelope;
    int lastFilterNote = 60;
    int previousNote = -1;
    uint64_t sampleCounter = 0;
    std::array<double, 3> lfoPhase {};
    std::array<uint64_t, 3> lfoNoteOnTime {};
    std::atomic<float> currentBpm { 120.0f };
    float lastVelocity = 0.0f, aftertouch = 0.0f, randomModulation = 0.0f;
    uint32_t modulationRandom = 0xa19f2000u;
    std::atomic<float> peak { 0.0f };
};
}

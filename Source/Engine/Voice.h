#pragma once

#include "../SampleEngine/SampleData.h"
#include <array>

namespace chrome
{
struct SlotPlayback
{
    std::shared_ptr<const SampleData> sample;
    float gain = 1.0f;
    float pan = 0.0f;
    float start = 0.0f;
    float end = 1.0f;
    float tune = 0.0f;
    bool loop = true;
    bool reverse = false;
    bool audible = true;
};

class Voice final
{
public:
    void prepare (double sampleRate);
    void start (int midiNote, float velocity, uint64_t age, const std::array<SlotPlayback, 3>&, float glideFromNote, float glideTime, bool legato);
    void release();
    void kill() noexcept;
    void updateEnvelope (float attack, float decay, float sustain, float release);
    void render (std::array<juce::AudioBuffer<float>*, 3> outputs, int startSample, int numSamples,
                 const std::array<SlotPlayback, 3>& currentSlots);

    [[nodiscard]] bool active() const noexcept { return isActive; }
    [[nodiscard]] int note() const noexcept { return midiNote; }
    [[nodiscard]] uint64_t startedAt() const noexcept { return ageStamp; }
    [[nodiscard]] float envelopeLevel() const noexcept { return lastEnvelopeLevel; }

private:
    std::array<double, 3> positions {};
    std::array<bool, 3> slotActive {};
    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParameters;
    double outputRate = 44100.0;
    int midiNote = -1;
    float noteVelocity = 0.0f;
    double glideNote = 0.0;
    double glideTarget = 0.0;
    double glideStep = 0.0;
    bool gliding = false;
    bool hasPlayed = false;
    float lastEnvelopeLevel = 0.0f;
    uint64_t ageStamp = 0;
    bool isActive = false;
};
}

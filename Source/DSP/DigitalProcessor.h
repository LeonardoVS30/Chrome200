#pragma once

#include <JuceHeader.h>

namespace chrome
{
class DigitalProcessor final
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>&, float density, float texture);

private:
    juce::AudioBuffer<float> history;
    int writePosition = 0;
    std::array<float, 2> held {};
    int holdCounter = 0;
    uint32_t noiseState = 0x82a2f91u;
};
}


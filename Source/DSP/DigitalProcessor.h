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
    double rate = 44100.0;
    int writePosition = 0;
    int readPosition = 0;
    int grainPosition = 0;
    int grainLength = 0;
    int samplesWritten = 0;
    bool grainActive = false;
    uint32_t noiseState = 0x82a2f91u;
};
}

#pragma once

#include <JuceHeader.h>

namespace chrome
{
class VintageProcessor final
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>&, float amount);

private:
    juce::dsp::IIR::Filter<float> headBump[2], rollOff[2];
    juce::AudioBuffer<float> haloDelay;
    double rate = 44100.0;
    double haloSin = 0.0, haloCos = 1.0;
    int haloWritePosition = 0;
    int haloNormalizationCounter = 0;
    float compressorEnvelope = 0.0f;
};
}

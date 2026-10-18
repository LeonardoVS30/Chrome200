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
    float compressorEnvelope = 0.0f;
};
}

#pragma once

#include <JuceHeader.h>

namespace chrome
{
struct ChromeControls { float liquid, reflect, fragment, shine, density, fusion; };

class ChromeProcessor final
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>&, const ChromeControls&);

private:
    std::array<float, 2> liquidState {};
    std::array<float, 2> toneState {};
    std::array<float, 2> dynamicsEnvelope {};
    double rate = 44100.0;
};
}

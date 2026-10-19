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
    static constexpr int resonatorCount = 4;
    struct ResonatorState
    {
        float z1 = 0.0f;
        float z2 = 0.0f;
    };

    std::array<float, 2> liquidState {};
    std::array<float, 2> toneState {};
    std::array<float, 2> dynamicsEnvelope {};
    std::array<std::array<ResonatorState, resonatorCount>, 2> resonators {};
    double rate = 44100.0;
};
}

#pragma once

#include <JuceHeader.h>

namespace chrome
{
struct ChromeControls { float liquid, reflect, fragment, shine, density, fusion; };

class ChromeProcessor final
{
public:
    ChromeProcessor();
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>&, const ChromeControls&);

private:
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int modalCount = 8;
    static constexpr int diffusionCount = 4;
    struct SpectralChannel
    {
        std::array<float, fftSize * 2> fftData {};
        std::array<float, fftSize / 2 + 1> frozenMagnitude {};
        std::array<float, fftSize> output {};
        int inputPosition = 0, outputPosition = 0;
    };
    struct Resonator { float y1 = 0.0f, y2 = 0.0f; };

    float spectralSample (int channel, float input, float freeze, float shine);
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<SpectralChannel, 2> spectral;
    std::array<std::array<Resonator, modalCount>, 2> resonators {};
    std::array<juce::AudioBuffer<float>, diffusionCount> diffusion;
    std::array<int, diffusionCount> diffusionWrite {};
    juce::AudioBuffer<float> combDelay;
    int combWrite = 0;
    double rate = 44100.0;
    double shiftPhase = 0.0;
    uint32_t randomState = 0x2000cafeu;
};
}


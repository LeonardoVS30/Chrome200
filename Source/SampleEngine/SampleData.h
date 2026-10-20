#pragma once

#include "PitchDetector.h"

namespace chrome
{
class SampleData final
{
public:
    SampleData (juce::AudioBuffer<float> audio, double sourceRate, juce::File sourceFile);

    [[nodiscard]] float sample (int channel, double position) const noexcept;
    [[nodiscard]] int length() const noexcept { return buffer.getNumSamples(); }
    [[nodiscard]] int channels() const noexcept { return buffer.getNumChannels(); }
    [[nodiscard]] double sampleRate() const noexcept { return rate; }
    [[nodiscard]] const juce::File& file() const noexcept { return source; }
    [[nodiscard]] const std::vector<float>& peaks() const noexcept { return displayPeaks; }
    [[nodiscard]] const std::vector<float>& spectrum() const noexcept { return displaySpectrum; }
    [[nodiscard]] const PitchDetectionResult& detectedPitch() const noexcept { return pitch; }

private:
    juce::AudioBuffer<float> buffer;
    double rate = 44100.0;
    juce::File source;
    std::vector<float> displayPeaks;
    std::vector<float> displaySpectrum;
    PitchDetectionResult pitch;
};
}

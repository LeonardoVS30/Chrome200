#include "SampleData.h"

namespace chrome
{
SampleData::SampleData (juce::AudioBuffer<float> audio, double sourceRate, juce::File sourceFile)
    : buffer (std::move (audio)), rate (sourceRate), source (std::move (sourceFile))
{
    constexpr int peakCount = 1024;
    displayPeaks.resize (peakCount);
    const auto stride = juce::jmax (1, buffer.getNumSamples() / peakCount);
    for (int p = 0; p < peakCount; ++p)
    {
        float peak = 0.0f;
        const auto first = p * stride;
        const auto last = juce::jmin (buffer.getNumSamples(), first + stride);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            for (int i = first; i < last; ++i)
                peak = juce::jmax (peak, std::abs (buffer.getSample (channel, i)));
        displayPeaks[static_cast<size_t> (p)] = peak;
    }

    constexpr int fftOrder = 10;
    constexpr int fftSize = 1 << fftOrder;
    constexpr int spectrumBins = 128;
    std::array<float, fftSize * 2> fftData {};
    const auto first = juce::jmax (0, (buffer.getNumSamples() - fftSize) / 2);
    for (int i = 0; i < fftSize && first + i < buffer.getNumSamples(); ++i)
    {
        float sum = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) sum += buffer.getSample (channel, first + i);
        fftData[static_cast<size_t> (i)] = sum / static_cast<float> (buffer.getNumChannels());
    }
    juce::dsp::WindowingFunction<float> window (fftSize, juce::dsp::WindowingFunction<float>::hann, false);
    window.multiplyWithWindowingTable (fftData.data(), fftSize);
    juce::dsp::FFT transform (fftOrder); transform.performFrequencyOnlyForwardTransform (fftData.data());
    displaySpectrum.resize (spectrumBins);
    float maximum = 1.0e-6f;
    for (int bin = 0; bin < spectrumBins; ++bin)
    {
        const auto low = static_cast<int> (std::pow (static_cast<float> (bin) / spectrumBins, 2.0f) * (fftSize / 2 - 1));
        const auto high = juce::jmax (low + 1, static_cast<int> (std::pow (static_cast<float> (bin + 1) / spectrumBins, 2.0f) * (fftSize / 2 - 1)));
        float value = 0.0f; for (int fftBin = low; fftBin <= high; ++fftBin) value = juce::jmax (value, fftData[static_cast<size_t> (fftBin)]);
        displaySpectrum[static_cast<size_t> (bin)] = value; maximum = juce::jmax (maximum, value);
    }
    for (auto& value : displaySpectrum) value = std::sqrt (value / maximum);
}

float SampleData::sample (int channel, double position) const noexcept
{
    if (buffer.getNumSamples() < 2)
        return 0.0f;
    channel = juce::jlimit (0, buffer.getNumChannels() - 1, channel);
    const auto index = juce::jlimit (0, buffer.getNumSamples() - 2, static_cast<int> (position));
    const auto fraction = static_cast<float> (position - static_cast<double> (index));
    const auto* data = buffer.getReadPointer (channel);
    return data[index] + fraction * (data[index + 1] - data[index]);
}
}

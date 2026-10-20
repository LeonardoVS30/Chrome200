#include "PitchDetector.h"

namespace chrome
{
namespace
{
constexpr float minimumFrequency = 35.0f;
constexpr float maximumFrequency = 2400.0f;
constexpr int maximumAnalysisSamples = 8192;

PitchDetectionResult analyseWindow (const std::vector<float>& source, double sampleRate, int start, int length)
{
    PitchDetectionResult result;
    const auto minimumLag = juce::jmax (2, static_cast<int> (std::floor (sampleRate / maximumFrequency)));
    const auto maximumLag = juce::jmin (length / 3, static_cast<int> (std::ceil (sampleRate / minimumFrequency)));
    if (maximumLag <= minimumLag + 2)
        return result;

    std::vector<float> window (static_cast<size_t> (length));
    double mean = 0.0;
    for (int i = 0; i < length; ++i)
        mean += source[static_cast<size_t> (start + i)];
    mean /= static_cast<double> (length);

    double energy = 0.0;
    for (int i = 0; i < length; ++i)
    {
        const auto value = source[static_cast<size_t> (start + i)] - static_cast<float> (mean);
        window[static_cast<size_t> (i)] = value;
        energy += static_cast<double> (value) * value;
    }
    const auto rms = std::sqrt (energy / static_cast<double> (length));
    if (rms < 1.0e-5)
        return result;

    std::vector<double> difference (static_cast<size_t> (maximumLag + 2), 0.0);
    const auto comparisons = length - maximumLag - 1;
    for (int lag = 1; lag <= maximumLag + 1; ++lag)
    {
        double sum = 0.0;
        for (int i = 0; i < comparisons; ++i)
        {
            const auto delta = static_cast<double> (window[static_cast<size_t> (i)])
                             - static_cast<double> (window[static_cast<size_t> (i + lag)]);
            sum += delta * delta;
        }
        difference[static_cast<size_t> (lag)] = sum;
    }

    std::vector<double> normalised (difference.size(), 1.0);
    double runningSum = 0.0;
    for (int lag = 1; lag <= maximumLag + 1; ++lag)
    {
        runningSum += difference[static_cast<size_t> (lag)];
        if (runningSum > 1.0e-20)
            normalised[static_cast<size_t> (lag)] = difference[static_cast<size_t> (lag)]
                                                       * static_cast<double> (lag) / runningSum;
    }

    int selectedLag = -1;
    for (int lag = minimumLag; lag <= maximumLag; ++lag)
    {
        if (normalised[static_cast<size_t> (lag)] < 0.15)
        {
            while (lag < maximumLag
                   && normalised[static_cast<size_t> (lag + 1)] < normalised[static_cast<size_t> (lag)])
                ++lag;
            selectedLag = lag;
            break;
        }
    }
    if (selectedLag < 0)
    {
        selectedLag = minimumLag;
        for (int lag = minimumLag + 1; lag <= maximumLag; ++lag)
            if (normalised[static_cast<size_t> (lag)] < normalised[static_cast<size_t> (selectedLag)])
                selectedLag = lag;
    }

    const auto confidence = 1.0 - normalised[static_cast<size_t> (selectedLag)];
    if (confidence < 0.60)
        return result;

    const auto left = normalised[static_cast<size_t> (selectedLag - 1)];
    const auto centre = normalised[static_cast<size_t> (selectedLag)];
    const auto right = normalised[static_cast<size_t> (selectedLag + 1)];
    const auto denominator = left - 2.0 * centre + right;
    auto refinedLag = static_cast<double> (selectedLag);
    if (std::abs (denominator) > 1.0e-12)
        refinedLag += juce::jlimit (-0.5, 0.5, 0.5 * (left - right) / denominator);

    const auto frequency = sampleRate / refinedLag;
    const auto midiPitch = 69.0 + 12.0 * std::log2 (frequency / 440.0);
    if (! std::isfinite (midiPitch) || midiPitch < 0.0 || midiPitch > 127.0)
        return {};

    result.frequencyHz = static_cast<float> (frequency);
    result.midiPitch = static_cast<float> (midiPitch);
    result.confidence = static_cast<float> (confidence);
    return result;
}
}

PitchDetectionResult detectPitch (const juce::AudioBuffer<float>& audio, double sampleRate)
{
    if (audio.getNumChannels() <= 0 || audio.getNumSamples() < 256 || sampleRate <= 0.0)
        return {};

    const auto decimation = juce::jmax (1, static_cast<int> (std::ceil (sampleRate / 24000.0)));
    const auto analysisRate = sampleRate / static_cast<double> (decimation);
    const auto downsampledLength = audio.getNumSamples() / decimation;
    if (downsampledLength < 256)
        return {};

    std::vector<float> mono (static_cast<size_t> (downsampledLength));
    for (int outputSample = 0; outputSample < downsampledLength; ++outputSample)
    {
        double sum = 0.0;
        for (int offset = 0; offset < decimation; ++offset)
            for (int channel = 0; channel < audio.getNumChannels(); ++channel)
                sum += audio.getSample (channel, outputSample * decimation + offset);
        mono[static_cast<size_t> (outputSample)] = static_cast<float> (
            sum / static_cast<double> (decimation * audio.getNumChannels()));
    }

    const auto requiredForLowestPitch = static_cast<int> (std::ceil (analysisRate / minimumFrequency)) * 4;
    const auto windowLength = juce::jmin (downsampledLength,
                                          juce::jmin (maximumAnalysisSamples,
                                                      juce::jmax (2048, requiredForLowestPitch)));
    if (windowLength < 256)
        return {};

    constexpr std::array<double, 7> positions { 0.08, 0.20, 0.35, 0.50, 0.65, 0.80, 0.92 };
    PitchDetectionResult best;
    for (const auto position : positions)
    {
        const auto centre = juce::roundToInt (position * static_cast<double> (downsampledLength));
        const auto start = juce::jlimit (0, downsampledLength - windowLength, centre - windowLength / 2);
        const auto candidate = analyseWindow (mono, analysisRate, start, windowLength);
        if (candidate.confidence > best.confidence)
            best = candidate;
    }
    return best;
}

juce::String formatMidiPitch (float midiPitch, bool includeCents)
{
    if (! std::isfinite (midiPitch) || midiPitch < 0.0f)
        return "--";

    constexpr std::array<const char*, 12> names { "C", "C#", "D", "D#", "E", "F",
                                                  "F#", "G", "G#", "A", "A#", "B" };
    const auto nearest = juce::jlimit (0, 127, juce::roundToInt (midiPitch));
    const auto noteName = juce::String (names[static_cast<size_t> (nearest % 12)])
                        + juce::String (nearest / 12 - 1);
    if (! includeCents)
        return noteName;

    const auto cents = juce::roundToInt ((midiPitch - static_cast<float> (nearest)) * 100.0f);
    if (cents == 0)
        return noteName;
    return noteName + (cents > 0 ? " +" : " ") + juce::String (cents) + "ct";
}
}

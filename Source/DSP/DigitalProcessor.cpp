#include "DigitalProcessor.h"

namespace chrome
{
void DigitalProcessor::prepare (double sampleRate, int)
{
    rate = sampleRate;
    history.setSize (2, static_cast<int> (sampleRate * 0.6), false, true, true);
    reset();
}

void DigitalProcessor::reset()
{
    history.clear();
    writePosition = 0;
    readPosition = 0;
    grainPosition = 0;
    grainLength = 0;
    samplesWritten = 0;
    grainActive = false;
    noiseState = 0x82a2f91u;
}

void DigitalProcessor::process (juce::AudioBuffer<float>& buffer, float density, float texture)
{
    density = juce::jlimit (0.0f, 1.0f, density);
    texture = juce::jlimit (0.0f, 1.0f, texture);
    const auto historyLength = history.getNumSamples();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (grainPosition >= grainLength)
        {
            grainLength = juce::jlimit (24, historyLength / 4,
                                        static_cast<int> (rate * (0.003 + (1.0 - density) * 0.032)));
            const auto minimumLookback = grainLength + 1;
            const auto desiredLookback = static_cast<int> (rate * (0.012 + static_cast<double> (texture) * 0.12));
            const auto maximumLookback = juce::jmin (historyLength - 1, samplesWritten, desiredLookback);
            grainActive = maximumLookback >= minimumLookback;
            noiseState ^= noiseState << 13; noiseState ^= noiseState >> 17; noiseState ^= noiseState << 5;
            if (grainActive)
            {
                const auto lookbackRange = maximumLookback - minimumLookback + 1;
                const auto lookback = minimumLookback
                                    + static_cast<int> (noiseState % static_cast<uint32_t> (lookbackRange));
                readPosition = writePosition - lookback;
                while (readPosition < 0) readPosition += historyLength;
            }
            grainPosition = 0;
        }

        const auto phase = (static_cast<float> (grainPosition) + 0.5f) / static_cast<float> (grainLength);
        const auto window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase);
        const auto mix = texture * (0.35f + density * 0.35f) * window;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto dry = buffer.getSample (ch, i);
            history.setSample (ch, writePosition, dry);
            if (grainActive)
            {
                const auto fragmented = history.getSample (ch, readPosition);
                buffer.setSample (ch, i, dry + (fragmented - dry) * mix);
            }
        }
        if (++writePosition == historyLength) writePosition = 0;
        if (grainActive && ++readPosition == historyLength) readPosition = 0;
        ++grainPosition;
        samplesWritten = juce::jmin (historyLength, samplesWritten + 1);
    }
}
}

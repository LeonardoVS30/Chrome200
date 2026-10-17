#include "DigitalProcessor.h"

namespace chrome
{
void DigitalProcessor::prepare (double sampleRate, int)
{
    history.setSize (2, static_cast<int> (sampleRate * 0.6), false, true, true);
    reset();
}

void DigitalProcessor::reset()
{
    history.clear(); writePosition = 0; holdCounter = 0; held.fill (0.0f); noiseState = 0x82a2f91u;
}

void DigitalProcessor::process (juce::AudioBuffer<float>& buffer, float density, float texture)
{
    const auto historyLength = history.getNumSamples();
    const auto grainLength = juce::jlimit (24, historyLength / 3, static_cast<int> (80.0f + (1.0f - density) * 2200.0f));
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (--holdCounter <= 0)
        {
            noiseState ^= noiseState << 13; noiseState ^= noiseState >> 17; noiseState ^= noiseState << 5;
            holdCounter = juce::jmax (8, grainLength);
            const auto randomOffset = static_cast<int> (noiseState % static_cast<uint32_t> (juce::jmax (1, historyLength - grainLength)));
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto read = writePosition - grainLength - randomOffset;
                while (read < 0) read += historyLength;
                held[static_cast<size_t> (ch)] = history.getSample (ch, read);
            }
        }

        const auto window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * static_cast<float> (holdCounter) / static_cast<float> (grainLength));
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto dry = buffer.getSample (ch, i);
            history.setSample (ch, writePosition, dry);
            const auto fragmented = held[static_cast<size_t> (ch)] * window;
            buffer.setSample (ch, i, dry + (fragmented - dry) * texture * 0.65f);
        }
        if (++writePosition == historyLength) writePosition = 0;
    }
}
}


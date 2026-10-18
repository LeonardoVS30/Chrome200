#include "ChromeProcessor.h"

namespace chrome
{
void ChromeProcessor::prepare (double sampleRate, int)
{
    rate = sampleRate;
    reset();
}

void ChromeProcessor::reset()
{
    liquidState.fill (0.0f);
    toneState.fill (0.0f);
    dynamicsEnvelope.fill (0.0f);
}

void ChromeProcessor::process (juce::AudioBuffer<float>& buffer, const ChromeControls& c)
{
    const auto liquid = juce::jlimit (0.0f, 1.0f, c.liquid);
    const auto reflect = juce::jlimit (0.0f, 1.0f, c.reflect);
    const auto fragment = juce::jlimit (0.0f, 1.0f, c.fragment);
    const auto shine = juce::jlimit (0.0f, 1.0f, c.shine);
    const auto density = juce::jlimit (0.0f, 1.0f, c.density);
    const auto fusion = juce::jlimit (0.0f, 1.0f, c.fusion);
    const auto liquidCoefficient = 0.68f - liquid * 0.28f;
    const auto cutoff = 2400.0f + shine * 11600.0f;
    const auto toneCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoff
                                                   / static_cast<float> (rate));
    const auto wetMix = 0.10f + fusion * 0.18f;
    const auto outputGain = 0.92f - fusion * 0.10f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            const auto index = static_cast<size_t> (channel);
            const auto dry = buffer.getSample (channel, sample);
            auto& smoothed = liquidState[index];
            smoothed += (dry - smoothed) * liquidCoefficient;
            auto x = dry + (smoothed - dry) * liquid * 0.32f;

            auto& low = toneState[index];
            low += (x - low) * toneCoefficient;
            const auto high = x - low;
            const auto detail = dry - smoothed;
            x += high * shine * 0.12f;
            x += detail * reflect * 0.07f;
            x += (smoothed - dry) * fragment * 0.06f;

            auto& envelope = dynamicsEnvelope[index];
            const auto magnitude = std::abs (dry);
            envelope += (magnitude - envelope) * (magnitude > envelope ? 0.025f : 0.0012f);
            const auto compression = 1.0f / (1.0f + envelope * density * 0.24f);
            const auto wet = x * compression;
            buffer.setSample (channel, sample,
                              (dry + (wet - dry) * wetMix) * outputGain);
        }
    }
}
}

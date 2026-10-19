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
    for (auto& channel : resonators)
        for (auto& resonator : channel)
            resonator = {};
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
    const auto wetMix = 0.20f + fusion * 0.30f;
    const auto outputGain = 0.96f - fusion * 0.08f;
    constexpr std::array<float, resonatorCount> frequencyRatios { 1.0f, 1.497f, 2.113f, 2.879f };
    constexpr std::array<std::array<float, resonatorCount>, 2> modeWeights
    {{
        { 0.34f, -0.27f, 0.22f, -0.17f },
        { 0.30f, -0.31f, 0.18f, -0.22f }
    }};
    struct Coefficients { float b0, b2, a1, a2; };
    std::array<Coefficients, resonatorCount> coefficients {};
    const auto baseFrequency = 420.0f + shine * 480.0f;
    const auto resonanceQ = 3.0f + reflect * 9.0f;
    for (int mode = 0; mode < resonatorCount; ++mode)
    {
        const auto frequency = juce::jmin (static_cast<float> (rate * 0.42),
                                           baseFrequency * frequencyRatios[static_cast<size_t> (mode)]);
        const auto omega = juce::MathConstants<float>::twoPi * frequency / static_cast<float> (rate);
        const auto alpha = std::sin (omega) / (2.0f * resonanceQ);
        const auto inverseA0 = 1.0f / (1.0f + alpha);
        coefficients[static_cast<size_t> (mode)] = { alpha * inverseA0,
                                                      -alpha * inverseA0,
                                                      -2.0f * std::cos (omega) * inverseA0,
                                                      (1.0f - alpha) * inverseA0 };
    }

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
            x += high * shine * 0.20f;
            x += detail * reflect * 0.10f;
            x += (smoothed - dry) * fragment * 0.10f;

            float metallic = 0.0f;
            for (int mode = 0; mode < resonatorCount; ++mode)
            {
                const auto& coefficient = coefficients[static_cast<size_t> (mode)];
                auto& resonator = resonators[index][static_cast<size_t> (mode)];
                const auto resonated = coefficient.b0 * x + resonator.z1;
                resonator.z1 = -coefficient.a1 * resonated + resonator.z2;
                resonator.z2 = coefficient.b2 * x - coefficient.a2 * resonated;
                metallic += resonated * modeWeights[index][static_cast<size_t> (mode)];
            }
            x += metallic * reflect * (0.72f + density * 0.36f);

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

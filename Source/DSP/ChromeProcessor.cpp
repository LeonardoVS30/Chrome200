#include "ChromeProcessor.h"

namespace chrome
{
ChromeProcessor::ChromeProcessor()
    : fft (fftOrder), window (fftSize, juce::dsp::WindowingFunction<float>::hann, false)
{
}

void ChromeProcessor::prepare (double sampleRate, int)
{
    rate = sampleRate;
    const std::array<double, diffusionCount> milliseconds { 7.1, 11.3, 17.9, 23.7 };
    for (size_t i = 0; i < diffusion.size(); ++i)
        diffusion[i].setSize (2, static_cast<int> (milliseconds[i] * 0.001 * rate) + 2, false, true, true);
    combDelay.setSize (2, static_cast<int> (rate * 0.08) + 2, false, true, true);
    reset();
}

void ChromeProcessor::reset()
{
    for (auto& state : spectral) state = {};
    for (auto& bank : resonators) for (auto& resonator : bank) resonator = {};
    for (auto& delay : diffusion) delay.clear();
    combDelay.clear(); diffusionWrite.fill (0); combWrite = 0; shiftPhase = 0.0;
}

float ChromeProcessor::spectralSample (int channel, float input, float freeze, float shine)
{
    auto& state = spectral[static_cast<size_t> (channel)];
    const auto result = state.output[static_cast<size_t> (state.outputPosition)];
    state.output[static_cast<size_t> (state.outputPosition)] = 0.0f;
    state.outputPosition = (state.outputPosition + 1) % fftSize;
    state.fftData[static_cast<size_t> (state.inputPosition++)] = input;
    if (state.inputPosition == fftSize)
    {
        window.multiplyWithWindowingTable (state.fftData.data(), fftSize);
        fft.performRealOnlyForwardTransform (state.fftData.data(), true);
        for (int bin = 0; bin <= fftSize / 2; ++bin)
        {
            const auto realIndex = bin == fftSize / 2 ? 1 : bin * 2;
            const auto imagIndex = bin == fftSize / 2 ? 1 : realIndex + 1;
            const auto real = state.fftData[static_cast<size_t> (realIndex)];
            const auto imag = bin == 0 || bin == fftSize / 2 ? 0.0f : state.fftData[static_cast<size_t> (imagIndex)];
            const auto magnitude = std::sqrt (real * real + imag * imag);
            auto& frozen = state.frozenMagnitude[static_cast<size_t> (bin)];
            frozen += (magnitude - frozen) * (0.025f + (1.0f - freeze) * 0.45f);
            const auto target = magnitude + (frozen - magnitude) * freeze;
            const auto scale = target / juce::jmax (1.0e-7f, magnitude);
            const auto brightScale = 1.0f + shine * static_cast<float> (bin) / static_cast<float> (fftSize / 2) * 0.3f;
            state.fftData[static_cast<size_t> (realIndex)] = real * scale * brightScale;
            if (bin != 0 && bin != fftSize / 2)
                state.fftData[static_cast<size_t> (imagIndex)] = imag * scale * brightScale;
        }
        fft.performRealOnlyInverseTransform (state.fftData.data());
        for (int i = 0; i < fftSize; ++i)
            state.output[static_cast<size_t> (i)] = state.fftData[static_cast<size_t> (i)] / static_cast<float> (fftSize);
        state.inputPosition = 0;
    }
    return result;
}

void ChromeProcessor::process (juce::AudioBuffer<float>& buffer, const ChromeControls& c)
{
    const auto samples = buffer.getNumSamples();
    constexpr std::array<float, modalCount> ratios { 1.0f, 1.4142f, 1.932f, 2.717f, 3.141f, 4.113f, 5.387f, 7.071f };
    for (int i = 0; i < samples; ++i)
    {
        randomState ^= randomState << 13; randomState ^= randomState >> 17; randomState ^= randomState << 5;
        const auto fragmentGate = ((randomState & 0xffffu) / 65535.0f < c.fragment * 0.025f) ? 0.15f : 1.0f;
        const auto shiftHz = 0.35 + c.shine * 4.5;
        shiftPhase += juce::MathConstants<double>::twoPi * shiftHz / rate;
        if (shiftPhase > juce::MathConstants<double>::twoPi) shiftPhase -= juce::MathConstants<double>::twoPi;
        const auto shimmer = static_cast<float> (std::sin (shiftPhase));

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto dry = buffer.getSample (ch, i);
            const auto frozen = spectralSample (ch, dry, c.liquid * 0.82f, c.shine);
            auto x = dry + (frozen - dry) * c.liquid * 0.55f;
            x *= fragmentGate;

            float modal = 0.0f;
            for (int mode = 0; mode < modalCount; ++mode)
            {
                const auto frequency = juce::jmin (static_cast<float> (rate * 0.45), (190.0f + c.shine * 520.0f) * ratios[static_cast<size_t> (mode)]);
                const auto radius = 0.965f + c.reflect * 0.032f;
                const auto coefficient = 2.0f * radius * std::cos (juce::MathConstants<float>::twoPi * frequency / static_cast<float> (rate));
                auto& r = resonators[static_cast<size_t> (ch)][static_cast<size_t> (mode)];
                const auto excitation = x * (0.0035f + c.density * 0.005f) / static_cast<float> (mode + 1);
                const auto y = excitation + coefficient * r.y1 - radius * radius * r.y2;
                r.y2 = r.y1; r.y1 = y;
                modal += y;
            }
            x += modal * c.reflect;

            for (size_t stage = 0; stage < diffusion.size(); ++stage)
            {
                auto& delay = diffusion[stage];
                const auto p = diffusionWrite[stage];
                const auto delayed = delay.getSample (ch, p);
                const auto g = 0.35f + c.liquid * 0.32f;
                delay.setSample (ch, p, x + delayed * g);
                x = delayed - x * g;
            }

            const auto combLength = combDelay.getNumSamples();
            const auto combTime = 0.004f + c.reflect * 0.026f + c.liquid * 0.006f * shimmer;
            auto read = combWrite - static_cast<int> (combTime * static_cast<float> (rate));
            while (read < 0) read += combLength;
            const auto reflected = combDelay.getSample (ch, read);
            combDelay.setSample (ch, combWrite, dry + reflected * (0.25f + c.density * 0.45f));
            const auto shifted = x * (1.0f + shimmer * c.shine * 0.05f);
            buffer.setSample (ch, i, std::tanh (dry * (1.0f - c.fusion * 0.45f) + (shifted + reflected * c.reflect) * c.fusion));
        }
        for (size_t stage = 0; stage < diffusion.size(); ++stage)
            if (++diffusionWrite[stage] == diffusion[stage].getNumSamples()) diffusionWrite[stage] = 0;
        if (++combWrite == combDelay.getNumSamples()) combWrite = 0;
    }
}
}

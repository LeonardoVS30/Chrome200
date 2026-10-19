#include "VintageProcessor.h"

namespace chrome
{
void VintageProcessor::prepare (double sampleRate, int maxBlockSize)
{
    rate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (maxBlockSize), 1 };
    const auto processingRate = static_cast<float> (sampleRate);
    auto bump = juce::dsp::IIR::Coefficients<float>::makePeakFilter (processingRate, 82.0f, 0.65f, juce::Decibels::decibelsToGain (2.5f));
    auto lowpass = juce::dsp::IIR::Coefficients<float>::makeLowPass (processingRate,
                                                                     juce::jmin (9000.0f, processingRate * 0.42f),
                                                                     0.7f);
    for (int channel = 0; channel < 2; ++channel)
    {
        headBump[channel].prepare (spec); headBump[channel].coefficients = bump;
        rollOff[channel].prepare (spec); rollOff[channel].coefficients = lowpass;
    }
    haloDelay.setSize (2, static_cast<int> (sampleRate * 0.03) + 2, false, true, true);
    reset();
}

void VintageProcessor::reset()
{
    for (int channel = 0; channel < 2; ++channel) { headBump[channel].reset(); rollOff[channel].reset(); }
    haloDelay.clear();
    haloSin = 0.0;
    haloCos = 1.0;
    haloWritePosition = 0;
    haloNormalizationCounter = 0;
    compressorEnvelope = 0.0f;
}

void VintageProcessor::process (juce::AudioBuffer<float>& buffer, float amount)
{
    amount = juce::jlimit (0.0f, 1.0f, amount);
    const auto outputGain = 0.94f - amount * 0.04f;
    const auto wetMix = 0.24f + amount * 0.38f;
    const auto haloMix = 0.08f + amount * 0.42f;
    const auto haloRate = 0.07 + static_cast<double> (amount) * 0.05;
    const auto oscillatorDelta = juce::MathConstants<double>::twoPi * haloRate / rate;
    const auto deltaSin = std::sin (oscillatorDelta);
    const auto deltaCos = std::cos (oscillatorDelta);
    constexpr auto stereoOffset = 1.7;
    const auto stereoOffsetSin = std::sin (stereoOffset);
    const auto stereoOffsetCos = std::cos (stereoOffset);
    const auto delayLength = haloDelay.getNumSamples();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float detector = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            detector = juce::jmax (detector, std::abs (buffer.getSample (ch, i)));
        compressorEnvelope += (detector - compressorEnvelope) * (detector > compressorEnvelope ? 0.02f : 0.001f);
        const auto compression = compressorEnvelope > 0.35f
                                   ? 1.0f / (1.0f + (compressorEnvelope - 0.35f) * 0.65f * amount)
                                   : 1.0f;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto dry = buffer.getSample (ch, i);
            auto x = dry * compression;
            x = headBump[ch].processSample (x);
            x = rollOff[ch].processSample (x);
            const auto modulation = ch == 0 ? haloSin
                                            : haloSin * stereoOffsetCos + haloCos * stereoOffsetSin;
            const auto delaySeconds = (ch == 0 ? 0.008 : 0.012)
                                    + modulation * (0.0006 + static_cast<double> (amount) * 0.0012);
            auto readPosition = static_cast<double> (haloWritePosition) - delaySeconds * rate;
            while (readPosition < 0.0) readPosition += static_cast<double> (delayLength);
            const auto read0 = static_cast<int> (readPosition);
            const auto read1 = read0 + 1 == delayLength ? 0 : read0 + 1;
            const auto fraction = static_cast<float> (readPosition - static_cast<double> (read0));
            const auto delayed = haloDelay.getSample (ch, read0)
                               + (haloDelay.getSample (ch, read1) - haloDelay.getSample (ch, read0)) * fraction;
            haloDelay.setSample (ch, haloWritePosition, x);
            x += (delayed - x) * haloMix;
            buffer.setSample (ch, i, (dry * (1.0f - wetMix) + x * wetMix) * outputGain);
        }
        if (++haloWritePosition == delayLength) haloWritePosition = 0;
        const auto nextSin = haloSin * deltaCos + haloCos * deltaSin;
        haloCos = haloCos * deltaCos - haloSin * deltaSin;
        haloSin = nextSin;
        if (++haloNormalizationCounter == 4096)
        {
            const auto inverseMagnitude = 1.0 / std::sqrt (haloSin * haloSin + haloCos * haloCos);
            haloSin *= inverseMagnitude;
            haloCos *= inverseMagnitude;
            haloNormalizationCounter = 0;
        }
    }
}
}

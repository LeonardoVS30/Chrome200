#include "VintageProcessor.h"

namespace chrome
{
void VintageProcessor::prepare (double sampleRate, int maxBlockSize)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (maxBlockSize), 1 };
    auto bump = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 82.0, 0.65f, juce::Decibels::decibelsToGain (2.5f));
    auto lowpass = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 14500.0, 0.7f);
    for (int channel = 0; channel < 2; ++channel)
    {
        headBump[channel].prepare (spec); headBump[channel].coefficients = bump;
        rollOff[channel].prepare (spec); rollOff[channel].coefficients = lowpass;
    }
    reset();
}

void VintageProcessor::reset()
{
    for (int channel = 0; channel < 2; ++channel) { headBump[channel].reset(); rollOff[channel].reset(); }
    compressorEnvelope = 0.0f;
}

void VintageProcessor::process (juce::AudioBuffer<float>& buffer, float amount)
{
    amount = juce::jlimit (0.0f, 1.0f, amount);
    const auto outputGain = 1.0f - amount * 0.18f;
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
            const auto wetMix = 0.10f + amount * 0.16f;
            buffer.setSample (ch, i, (dry * (1.0f - wetMix) + x * wetMix) * outputGain);
        }
    }
}
}

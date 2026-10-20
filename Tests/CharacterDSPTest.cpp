#include <JuceHeader.h>
#include "../Source/DSP/ChromeProcessor.h"
#include "../Source/DSP/DigitalProcessor.h"
#include "../Source/DSP/VintageProcessor.h"
#include <iostream>
#include <vector>

namespace
{
constexpr int streamLength = 4096;

juce::AudioBuffer<float> makeInput (double sampleRate)
{
    juce::AudioBuffer<float> input (2, streamLength);
    uint32_t random = 0x5a17c9e3u;
    for (int sample = 0; sample < streamLength; ++sample)
    {
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        const auto noise = (static_cast<float> (random & 0xffffu) / 32767.5f - 1.0f) * 0.015f;
        const auto time = static_cast<double> (sample) / sampleRate;
        const auto value = static_cast<float> (0.18 * std::sin (juce::MathConstants<double>::twoPi * 173.0 * time)
                                                + 0.10 * std::sin (juce::MathConstants<double>::twoPi * 3197.0 * time))
                         + noise;
        input.setSample (0, sample, value);
        input.setSample (1, sample, value * 0.83f);
    }
    input.addSample (0, 0, 0.35f);
    input.addSample (1, 0, -0.28f);
    return input;
}

template <typename Processor, typename Process>
juce::AudioBuffer<float> render (Processor& processor, const juce::AudioBuffer<float>& input,
                                 const std::vector<int>& partitions, Process&& process)
{
    juce::AudioBuffer<float> output (input.getNumChannels(), input.getNumSamples());
    int position = 0;
    size_t partition = 0;
    while (position < input.getNumSamples())
    {
        const auto blockSize = juce::jmin (partitions[partition++ % partitions.size()],
                                           input.getNumSamples() - position);
        juce::AudioBuffer<float> block (input.getNumChannels(), blockSize);
        for (int channel = 0; channel < input.getNumChannels(); ++channel)
            block.copyFrom (channel, 0, input, channel, position, blockSize);
        process (processor, block);
        for (int channel = 0; channel < output.getNumChannels(); ++channel)
            output.copyFrom (channel, position, block, channel, 0, blockSize);
        position += blockSize;
    }
    return output;
}

bool isFiniteAndBounded (const juce::AudioBuffer<float>& buffer, float bound)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = buffer.getSample (channel, sample);
            if (! std::isfinite (value) || std::abs (value) > bound)
                return false;
        }
    return true;
}

float maximumDifference (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float difference = 0.0f;
    for (int channel = 0; channel < a.getNumChannels(); ++channel)
        for (int sample = 0; sample < a.getNumSamples(); ++sample)
            difference = juce::jmax (difference,
                                     std::abs (a.getSample (channel, sample) - b.getSample (channel, sample)));
    return difference;
}

double rangeEnergy (const juce::AudioBuffer<float>& buffer, int start, int end)
{
    double energy = 0.0;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = start; sample < end; ++sample)
        {
            const auto value = static_cast<double> (buffer.getSample (channel, sample));
            energy += value * value;
        }
    return energy;
}

double rangeMean (const juce::AudioBuffer<float>& buffer, int start, int end)
{
    double sum = 0.0;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = start; sample < end; ++sample)
            sum += buffer.getSample (channel, sample);
    return sum / static_cast<double> ((end - start) * buffer.getNumChannels());
}

double stereoDifferenceEnergy (const juce::AudioBuffer<float>& buffer, int start)
{
    double energy = 0.0;
    for (int sample = start; sample < buffer.getNumSamples(); ++sample)
    {
        const auto difference = static_cast<double> (buffer.getSample (0, sample)
                                                      - buffer.getSample (1, sample));
        energy += difference * difference;
    }
    return energy / static_cast<double> (buffer.getNumSamples() - start);
}

double stereoCorrelation (const juce::AudioBuffer<float>& buffer, int start)
{
    double leftEnergy = 0.0, rightEnergy = 0.0, cross = 0.0;
    for (int sample = start; sample < buffer.getNumSamples(); ++sample)
    {
        const auto left = static_cast<double> (buffer.getSample (0, sample));
        const auto right = static_cast<double> (buffer.getSample (1, sample));
        leftEnergy += left * left;
        rightEnergy += right * right;
        cross += left * right;
    }
    return cross / std::sqrt (juce::jmax (1.0e-20, leftEnergy * rightEnergy));
}

double channelRms (const juce::AudioBuffer<float>& buffer, int channel, int start)
{
    double energy = 0.0;
    for (int sample = start; sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = static_cast<double> (buffer.getSample (channel, sample));
        energy += value * value;
    }
    return std::sqrt (energy / static_cast<double> (buffer.getNumSamples() - start));
}
}

int main()
{
    constexpr chrome::ChromeControls chromeControls { 0.45f, 0.80f, 0.25f, 0.65f, 0.50f, 0.40f };
    const std::vector<int> oneSample { 1 };
    const std::vector<int> irregular { 37, 1, 113, 64, 5, 251, 19 };

    for (const auto sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        const auto input = makeInput (sampleRate);

        chrome::VintageProcessor ethernal;
        ethernal.prepare (sampleRate, 512);
        const auto ethernalOne = render (ethernal, input, oneSample,
                                        [] (auto& processor, auto& block) { processor.process (block, 0.72f); });
        ethernal.reset();
        const auto ethernalIrregular = render (ethernal, input, irregular,
                                              [] (auto& processor, auto& block) { processor.process (block, 0.72f); });

        chrome::ChromeProcessor chromeProcessor;
        chromeProcessor.prepare (sampleRate, 512);
        const auto chromeOne = render (chromeProcessor, input, oneSample,
                                      [&] (auto& processor, auto& block) { processor.process (block, chromeControls); });
        chromeProcessor.reset();
        const auto chromeIrregular = render (chromeProcessor, input, irregular,
                                            [&] (auto& processor, auto& block) { processor.process (block, chromeControls); });

        chrome::DigitalProcessor digital;
        digital.prepare (sampleRate, 512);
        const auto digitalOne = render (digital, input, oneSample,
                                       [] (auto& processor, auto& block) { processor.process (block, 0.50f, 0.25f); });
        digital.reset();
        const auto digitalIrregular = render (digital, input, irregular,
                                             [] (auto& processor, auto& block) { processor.process (block, 0.50f, 0.25f); });

        for (const auto* output : { &ethernalOne, &chromeOne, &digitalOne })
            if (! isFiniteAndBounded (*output, 1.0f))
            {
                std::cerr << "FAIL: non-finite or excessive character output at " << sampleRate << " Hz\n";
                return 1;
            }

        if (maximumDifference (ethernalOne, ethernalIrregular) > 1.0e-6f
            || maximumDifference (chromeOne, chromeIrregular) > 1.0e-6f
            || maximumDifference (digitalOne, digitalIrregular) > 1.0e-6f)
        {
            std::cerr << "FAIL: character processing changed with block partition at " << sampleRate << " Hz\n";
            return 2;
        }

        if (maximumDifference (ethernalOne, chromeOne) < 1.0e-3f
            || maximumDifference (ethernalOne, digitalOne) < 1.0e-3f
            || maximumDifference (chromeOne, digitalOne) < 1.0e-3f)
        {
            std::cerr << "FAIL: two character modes have no measurable signature at " << sampleRate << " Hz\n";
            return 3;
        }

        juce::AudioBuffer<float> dc (2, streamLength);
        for (int channel = 0; channel < dc.getNumChannels(); ++channel)
            for (int sample = 0; sample < dc.getNumSamples(); ++sample)
                dc.setSample (channel, sample, 0.1f);
        ethernal.reset();
        chromeProcessor.reset();
        digital.reset();
        const auto ethernalDc = render (ethernal, dc, irregular,
                                       [] (auto& processor, auto& block) { processor.process (block, 0.72f); });
        const auto chromeDc = render (chromeProcessor, dc, irregular,
                                     [&] (auto& processor, auto& block) { processor.process (block, chromeControls); });
        const auto digitalDc = render (digital, dc, irregular,
                                      [] (auto& processor, auto& block) { processor.process (block, 0.50f, 0.25f); });
        for (const auto* output : { &ethernalDc, &chromeDc, &digitalDc })
        {
            const auto mean = rangeMean (*output, streamLength - 512, streamLength);
            if (mean < 0.03 || mean > 0.15)
            {
                std::cerr << "FAIL: invalid steady DC response at " << sampleRate << " Hz\n";
                return 6;
            }
        }
    }

    juce::AudioBuffer<float> monoDream (2, streamLength);
    for (int sample = 0; sample < monoDream.getNumSamples(); ++sample)
    {
        const auto time = static_cast<double> (sample) / 44100.0;
        const auto value = static_cast<float> (0.22 * std::sin (juce::MathConstants<double>::twoPi * 233.0 * time)
                                                + 0.08 * std::sin (juce::MathConstants<double>::twoPi * 1864.0 * time));
        monoDream.setSample (0, sample, value);
        monoDream.setSample (1, sample, value);
    }
    chrome::VintageProcessor ethernalHalo;
    ethernalHalo.prepare (44100.0, 512);
    const auto halo = render (ethernalHalo, monoDream, irregular,
                             [] (auto& processor, auto& block) { processor.process (block, 0.72f); });
    const auto haloDifference = stereoDifferenceEnergy (halo, 1024);
    const auto haloCorrelation = stereoCorrelation (halo, 1024);
    std::cout << "EthernalStereoDifference=" << haloDifference
              << " EthernalCorrelation=" << haloCorrelation << '\n';
    if (haloDifference < 8.0e-4 || haloCorrelation < 0.35)
    {
        std::cerr << "FAIL: Ethernal stereo halo is too subtle\n";
        return 8;
    }


    const auto makeTone = [] (float frequency)
    {
        juce::AudioBuffer<float> tone (2, streamLength);
        for (int sample = 0; sample < tone.getNumSamples(); ++sample)
        {
            const auto value = 0.2f * std::sin (juce::MathConstants<float>::twoPi * frequency
                                                * static_cast<float> (sample) / 44100.0f);
            tone.setSample (0, sample, value);
            tone.setSample (1, sample, value);
        }
        return tone;
    };
    chrome::VintageProcessor ethernalTone;
    ethernalTone.prepare (44100.0, 512);
    const auto lowToneInput = makeTone (500.0f);
    const auto lowTone = render (ethernalTone, lowToneInput, irregular,
                                [] (auto& processor, auto& block) { processor.process (block, 0.72f); });
    ethernalTone.reset();
    const auto highToneInput = makeTone (12000.0f);
    const auto highTone = render (ethernalTone, highToneInput, irregular,
                                 [] (auto& processor, auto& block) { processor.process (block, 0.72f); });
    const auto lowToneGain = channelRms (lowTone, 0, 1024) / channelRms (lowToneInput, 0, 1024);
    const auto highToneGain = channelRms (highTone, 0, 1024) / channelRms (highToneInput, 0, 1024);
    std::cout << "EthernalLowGain=" << lowToneGain
              << " EthernalHighGain=" << highToneGain << '\n';
    if (highToneGain > lowToneGain * 0.85)
    {
        std::cerr << "FAIL: Ethernal soft-focus tone is too similar to the dry signal\n";
        return 10;
    }

    constexpr int dcLength = 16384;
    juce::AudioBuffer<float> digitalDcInput (2, dcLength);
    for (int channel = 0; channel < digitalDcInput.getNumChannels(); ++channel)
        for (int sample = 0; sample < digitalDcInput.getNumSamples(); ++sample)
            digitalDcInput.setSample (channel, sample, 0.1f);
    chrome::DigitalProcessor digitalDcProcessor;
    digitalDcProcessor.prepare (44100.0, 512);
    const auto stableDigitalDc = render (digitalDcProcessor, digitalDcInput, irregular,
                                        [] (auto& processor, auto& block) { processor.process (block, 0.75f, 0.90f); });
    float dcMinimum = 1.0f, dcMaximum = -1.0f;
    for (int sample = dcLength - 2048; sample < dcLength; ++sample)
    {
        dcMinimum = juce::jmin (dcMinimum, stableDigitalDc.getSample (0, sample));
        dcMaximum = juce::jmax (dcMaximum, stableDigitalDc.getSample (0, sample));
    }
    std::cout << "DigitalSteadyDcSpan=" << dcMaximum - dcMinimum << '\n';
    if (dcMaximum - dcMinimum > 1.0e-5f)
    {
        std::cerr << "FAIL: Digital turns steady DC into artificial level pulses\n";
        return 9;
    }

    // Reflect must produce a short, bounded metallic decay. This is deliberately
    // measured away from the impulse so it cannot pass through level changes alone.
    juce::AudioBuffer<float> impulse (2, 1024);
    impulse.clear();
    impulse.setSample (0, 0, 0.5f);
    impulse.setSample (1, 0, 0.5f);
    chrome::ChromeProcessor reflective;
    reflective.prepare (44100.0, 512);
    const auto reflected = render (reflective, impulse, irregular,
                                  [] (auto& processor, auto& block)
                                  {
                                      processor.process (block, { 0.25f, 1.0f, 0.0f, 0.55f, 0.50f, 0.40f });
                                  });
    reflective.reset();
    const auto unreflected = render (reflective, impulse, irregular,
                                    [] (auto& processor, auto& block)
                                    {
                                        processor.process (block, { 0.25f, 0.0f, 0.0f, 0.55f, 0.50f, 0.40f });
                                    });
    const auto reflectedTail = rangeEnergy (reflected, 64, 512);
    const auto unreflectedTail = rangeEnergy (unreflected, 64, 512);
    std::cout << "ChromeReflectTailEnergy=" << reflectedTail
              << " ChromeDryTailEnergy=" << unreflectedTail << '\n';
    if (reflectedTail < 1.0e-5 || reflectedTail < unreflectedTail * 20.0)
    {
        std::cerr << "FAIL: Chrome Reflect character is too subtle\n";
        return 4;
    }

    chrome::VintageProcessor ethernalImpulse;
    ethernalImpulse.prepare (44100.0, 512);
    const auto warmImpulse = render (ethernalImpulse, impulse, irregular,
                                    [] (auto& processor, auto& block) { processor.process (block, 0.72f); });
    chrome::DigitalProcessor digitalImpulse;
    digitalImpulse.prepare (44100.0, 512);
    const auto fragmentedImpulse = render (digitalImpulse, impulse, irregular,
                                          [] (auto& processor, auto& block) { processor.process (block, 0.50f, 0.25f); });
    if (std::abs (reflected.getSample (0, 0)) < 0.05f
        || std::abs (warmImpulse.getSample (0, 0)) < 0.05f
        || std::abs (fragmentedImpulse.getSample (0, 0)) < 0.05f)
    {
        std::cerr << "FAIL: a character mode introduced buffered latency\n";
        return 7;
    }

    reflective.reset();
    ethernalImpulse.reset();
    digitalImpulse.reset();
    juce::AudioBuffer<float> silence (2, 512);
    const auto zeroSilence = [&]
    {
        for (int channel = 0; channel < silence.getNumChannels(); ++channel)
            for (int sample = 0; sample < silence.getNumSamples(); ++sample)
                silence.setSample (channel, sample, 0.0f);
    };
    zeroSilence();
    reflective.process (silence, chromeControls);
    const auto chromeSilence = juce::jmax (silence.getMagnitude (0, 0, silence.getNumSamples()),
                                          silence.getMagnitude (1, 0, silence.getNumSamples()));
    zeroSilence();
    ethernalImpulse.process (silence, 0.72f);
    const auto ethernalSilence = juce::jmax (silence.getMagnitude (0, 0, silence.getNumSamples()),
                                            silence.getMagnitude (1, 0, silence.getNumSamples()));
    zeroSilence();
    digitalImpulse.process (silence, 0.50f, 0.25f);
    const auto digitalSilence = juce::jmax (silence.getMagnitude (0, 0, silence.getNumSamples()),
                                           silence.getMagnitude (1, 0, silence.getNumSamples()));
    if (chromeSilence != 0.0f || ethernalSilence != 0.0f || digitalSilence != 0.0f)
    {
        std::cerr << "FAIL: a reset character processor generated output from silence: "
                  << chromeSilence << ", " << ethernalSilence << ", " << digitalSilence << '\n';
        return 5;
    }

    std::cout << "PASS: character DSP is distinct, bounded, deterministic, and partition invariant\n";
    return 0;
}

#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>

namespace
{
bool writeTestWave (const juce::File& file)
{
    file.deleteFile();
    auto stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    auto writer = std::unique_ptr<juce::AudioFormatWriter> (
        wav.createWriterFor (stream.release(), 44100.0, 2, 24, {}, 0));
    if (writer == nullptr)
        return false;

    constexpr int length = 44100;
    juce::AudioBuffer<float> signal (2, length);
    for (int sample = 0; sample < length; ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi * 330.0 * static_cast<double> (sample) / 44100.0;
        const auto value = static_cast<float> (std::sin (phase) * 0.3);
        signal.setSample (0, sample, value);
        signal.setSample (1, sample, value);
    }
    return writer->writeFromAudioSampleBuffer (signal, 0, length);
}

void setParameter (juce::AudioProcessorValueTreeState& state, const juce::String& id, float actual)
{
    if (auto* parameter = state.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (actual));
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    Chrome2000AudioProcessor processor;
    processor.setRateAndBufferSizeDetails (44100.0, 512);
    processor.prepareToPlay (44100.0, 512);

    const auto wave = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("chrome2000-processor-smoke.wav");
    if (! writeTestWave (wave))
    {
        std::cerr << "FAIL: unable to create integration WAV\n";
        return 1;
    }

    processor.engine.loadSample (0, wave);
    for (int wait = 0; wait < 500 && processor.engine.sample (0) == nullptr; ++wait)
        juce::Thread::sleep (10);
    if (processor.engine.sample (0) == nullptr)
    {
        std::cerr << "FAIL: plugin processor sample load timed out\n";
        return 2;
    }

    setParameter (processor.parameters, "power", 1.0f);
    setParameter (processor.parameters, "s1_enabled", 1.0f);
    setParameter (processor.parameters, "s1_mute", 0.0f);
    setParameter (processor.parameters, "s1_solo", 1.0f);
    setParameter (processor.parameters, "s1_mode", 1.0f);
    setParameter (processor.parameters, "s1_loop", 1.0f);
    setParameter (processor.parameters, "filter_cutoff", 8720.0f);
    setParameter (processor.parameters, "filter_resonance", 0.7f);
    setParameter (processor.parameters, "master_gain", -3.0f);

    juce::AudioBuffer<float> output (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    float peak = 0.0f;
    double energy = 0.0;
    double tailSum = 0.0;
    double tailEnergy = 0.0;
    int count = 0;
    int tailCount = 0;
    constexpr int blocks = 400;
    for (int block = 0; block < blocks; ++block)
    {
        output.clear();
        processor.processBlock (output, midi);
        midi.clear();
        peak = juce::jmax (peak, output.getMagnitude (0, output.getNumSamples()),
                          output.getMagnitude (1, output.getNumSamples()));
        for (int channel = 0; channel < output.getNumChannels(); ++channel)
            for (int sample = 0; sample < output.getNumSamples(); ++sample)
            {
                const auto value = output.getSample (channel, sample);
                if (! std::isfinite (value))
                {
                    std::cerr << "FAIL: processor produced non-finite audio\n";
                    return 3;
                }
                energy += static_cast<double> (value) * static_cast<double> (value);
                ++count;
                if (block >= blocks - 80)
                {
                    tailSum += value;
                    tailEnergy += static_cast<double> (value) * static_cast<double> (value);
                    ++tailCount;
                }
            }
    }

    const auto rms = static_cast<float> (std::sqrt (energy / static_cast<double> (juce::jmax (1, count))));
    const auto tailMean = tailSum / static_cast<double> (juce::jmax (1, tailCount));
    const auto tailRms = std::sqrt (tailEnergy / static_cast<double> (juce::jmax (1, tailCount)));
    const auto tailAcRms = std::sqrt (juce::jmax (0.0, tailRms * tailRms - tailMean * tailMean));
    wave.deleteFile();
    processor.releaseResources();
    std::cout << "ProcessorPeak=" << peak
              << " ProcessorRMS=" << rms
              << " TailRMS=" << tailRms
              << " TailACRMS=" << tailAcRms
              << " TailDC=" << tailMean << '\n';
    if (peak < 1.0e-4f || rms < 1.0e-5f || tailAcRms < 1.0e-4)
    {
        std::cerr << "FAIL: full plugin processor collapsed into silence or DC\n";
        return 4;
    }
    std::cout << "PASS: VST processor remains oscillating after several seconds\n";
    return 0;
}

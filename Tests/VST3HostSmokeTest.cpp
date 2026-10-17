#include <JuceHeader.h>
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

    juce::AudioBuffer<float> signal (2, 44100);
    for (int sample = 0; sample < signal.getNumSamples(); ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi * 440.0 * static_cast<double> (sample) / 44100.0;
        const auto value = static_cast<float> (std::sin (phase) * 0.3);
        signal.setSample (0, sample, value);
        signal.setSample (1, sample, value);
    }
    return writer->writeFromAudioSampleBuffer (signal, 0, signal.getNumSamples());
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    const juce::File plugin (
        "C:\\Program Files\\Common Files\\VST3\\CHROME 2000.vst3");
    if (! plugin.isDirectory())
    {
        std::cerr << "FAIL: installed VST3 bundle not found\n";
        return 1;
    }

    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile (descriptions, plugin.getFullPathName());
    if (descriptions.isEmpty())
    {
        std::cerr << "FAIL: JUCE host could not discover the VST3\n";
        return 2;
    }

    juce::String error;
    auto instance = format.createInstanceFromDescription (*descriptions[0], 44100.0, 512, error);
    if (instance == nullptr)
    {
        std::cerr << "FAIL: VST3 instantiation failed: " << error << '\n';
        return 3;
    }
    std::cout << "AcceptsMidi=" << instance->acceptsMidi()
              << " Inputs=" << instance->getTotalNumInputChannels()
              << " Outputs=" << instance->getTotalNumOutputChannels()
              << " Params=" << instance->getParameters().size() << '\n';
    instance->enableAllBuses();
    for (auto* parameter : instance->getParameters())
        if (auto* identified = dynamic_cast<juce::AudioProcessorParameterWithID*> (parameter))
        {
            if (identified->paramID == "power" || identified->paramID == "s1_enabled" || identified->paramID == "s1_solo")
                parameter->setValueNotifyingHost (1.0f);
            else if (identified->paramID == "s1_mute")
                parameter->setValueNotifyingHost (0.0f);
        }

    const auto wave = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("chrome2000-vst3-host-smoke.wav");
    if (! writeTestWave (wave))
    {
        std::cerr << "FAIL: unable to create host test WAV\n";
        return 4;
    }

    juce::MemoryBlock state;
    instance->getStateInformation (state);
    auto xml = juce::AudioProcessor::getXmlFromBinary (state.getData(), static_cast<int> (state.getSize()));
    if (xml == nullptr)
    {
        std::cerr << "FAIL: VST3 returned an unreadable state block\n";
        return 5;
    }
    auto* componentElement = xml->getChildByName ("IComponent");
    if (componentElement == nullptr)
    {
        std::cerr << "FAIL: VST3 state does not contain IComponent\n";
        return 5;
    }

    juce::MemoryBlock componentState;
    if (! componentState.fromBase64Encoding (componentElement->getAllSubText()))
    {
        std::cerr << "FAIL: VST3 IComponent state is not valid Base64\n";
        return 5;
    }

    auto componentXml = juce::AudioProcessor::getXmlFromBinary (
        componentState.getData(), static_cast<int> (componentState.getSize()));
    if (componentXml == nullptr)
    {
        std::cerr << "FAIL: VST3 IComponent does not contain readable plugin state\n";
        return 5;
    }

    auto tree = juce::ValueTree::fromXml (*componentXml);
    if (! tree.isValid() || ! tree.hasType ("CHROME2000_STATE"))
    {
        std::cerr << "FAIL: unexpected plugin state type: "
                  << tree.getType().toString() << '\n';
        return 5;
    }

    tree.setProperty ("samplePath0", wave.getFullPathName(), nullptr);
    std::cout << "HostStateType=" << xml->getTagName()
              << " PluginStateType=" << tree.getType().toString()
              << " SamplePath=" << tree.getProperty ("samplePath0").toString() << '\n';

    if (auto updatedComponentXml = tree.createXml())
    {
        juce::MemoryBlock updatedComponentState;
        juce::AudioProcessor::copyXmlToBinary (*updatedComponentXml, updatedComponentState);
        componentElement->deleteAllChildElements();
        componentElement->addTextElement (updatedComponentState.toBase64Encoding());

        juce::MemoryBlock updatedHostState;
        juce::AudioProcessor::copyXmlToBinary (*xml, updatedHostState);
        instance->setStateInformation (
            updatedHostState.getData(), static_cast<int> (updatedHostState.getSize()));
    }

    juce::Thread::sleep (1000);
    instance->setPlayConfigDetails (0, 2, 44100.0, 512);
    instance->prepareToPlay (44100.0, 512);
    std::cout << "PreparedInputs=" << instance->getTotalNumInputChannels()
              << " PreparedOutputs=" << instance->getTotalNumOutputChannels() << '\n';
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
        instance->processBlock (output, midi);
        midi.clear();
        peak = juce::jmax (peak, output.getMagnitude (0, output.getNumSamples()),
                          output.getMagnitude (1, output.getNumSamples()));
        for (int channel = 0; channel < output.getNumChannels(); ++channel)
            for (int sample = 0; sample < output.getNumSamples(); ++sample)
            {
                const auto value = output.getSample (channel, sample);
                if (! std::isfinite (value))
                {
                    std::cerr << "FAIL: installed VST3 produced non-finite audio\n";
                    return 6;
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
    instance->releaseResources();
    wave.deleteFile();
    const auto rms = static_cast<float> (std::sqrt (energy / static_cast<double> (juce::jmax (1, count))));
    const auto tailMean = tailSum / static_cast<double> (juce::jmax (1, tailCount));
    const auto tailRms = std::sqrt (tailEnergy / static_cast<double> (juce::jmax (1, tailCount)));
    const auto tailAcRms = std::sqrt (juce::jmax (0.0, tailRms * tailRms - tailMean * tailMean));
    std::cout << "InstalledVendor=" << descriptions[0]->manufacturerName
              << " InstalledPeak=" << peak
              << " InstalledRMS=" << rms
              << " TailRMS=" << tailRms
              << " TailACRMS=" << tailAcRms
              << " TailDC=" << tailMean << '\n';
    if (descriptions[0]->manufacturerName != "LEVSAN")
    {
        std::cerr << "FAIL: installed VST3 vendor is not LEVSAN\n";
        return 7;
    }
    if (peak < 1.0e-4f || rms < 1.0e-5f || tailAcRms < 1.0e-4)
    {
        std::cerr << "FAIL: installed VST3 collapsed into silence or DC\n";
        return 8;
    }
    std::cout << "PASS: installed VST3 remains oscillating after several seconds\n";
    return 0;
}

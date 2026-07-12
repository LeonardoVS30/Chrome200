#include "PluginProcessor.h"
#include "PluginEditor.h"

Chrome2000AudioProcessor::Chrome2000AudioProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "CHROME2000_STATE", chrome::params::createLayout()),
      engine (parameters), presets (parameters, engine)
{
}

void Chrome2000AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

void Chrome2000AudioProcessor::releaseResources() { engine.reset(); }

bool Chrome2000AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void Chrome2000AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    if (chrome::params::load (parameters, "power") < 0.5f)
    {
        buffer.clear();
        return;
    }
    engine.process (buffer, midi);
}

juce::AudioProcessorEditor* Chrome2000AudioProcessor::createEditor()
{
    return new Chrome2000AudioProcessorEditor (*this);
}

void Chrome2000AudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto tree = parameters.copyState();
    for (int i = 0; i < 3; ++i) tree.setProperty ("samplePath" + juce::String (i), engine.samplePath (i), nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destination);
}

void Chrome2000AudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        const auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (parameters.state.getType()))
        {
            for (int i = 0; i < 3; ++i) engine.restoreSamplePath (i, tree.getProperty ("samplePath" + juce::String (i)).toString());
            parameters.replaceState (tree);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Chrome2000AudioProcessor(); }

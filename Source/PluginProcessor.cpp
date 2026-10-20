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
    const bool powerOn = chrome::params::load (parameters, "power") >= 0.5f;
    if (! powerOn)
    {
        if (powerWasOn) { engine.reset(); powerWasOn = false; }
        buffer.clear();
        return;
    }
    if (! powerWasOn) powerWasOn = true;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                engine.setBpm (static_cast<float> (*bpm));
    engine.process (buffer, midi);
}

juce::AudioProcessorEditor* Chrome2000AudioProcessor::createEditor()
{
    return new Chrome2000AudioProcessorEditor (*this);
}

void Chrome2000AudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto tree = parameters.copyState();
    engine.writeSampleState (tree);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destination);
}

void Chrome2000AudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        const auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (parameters.state.getType()))
        {
            parameters.replaceState (tree);
            engine.restoreSampleState (tree);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Chrome2000AudioProcessor(); }

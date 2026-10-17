#pragma once

#include <JuceHeader.h>

namespace chrome
{
class SampleEngine;

class PresetManager final
{
public:
    PresetManager (juce::AudioProcessorValueTreeState&, SampleEngine&);
    [[nodiscard]] juce::StringArray names() const;
    [[nodiscard]] juce::String currentName() const { return current; }
    bool load (const juce::String& name);
    bool save (const juce::String& name);
    bool remove (const juce::String& name);
    juce::String step (int direction);

private:
    void applyFactory (const juce::String& name);
    void setActual (const juce::String& id, float value);
    juce::File directory() const;
    juce::AudioProcessorValueTreeState& state;
    SampleEngine& engine;
    juce::String current { "Init Patch" };
    const juce::StringArray factory { "Init Patch", "Liquid Cathedral", "Mercury Glass", "Tape Constellation", "Digital Pollen", "Frozen Alloy" };
};
}

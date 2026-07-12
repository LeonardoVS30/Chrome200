#pragma once

#include <JuceHeader.h>

namespace chrome::params
{
juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
juce::String slotId (int slot, const juce::String& name);

inline float load (const juce::AudioProcessorValueTreeState& state, juce::StringRef id)
{
    if (const auto* value = state.getRawParameterValue (id))
        return value->load (std::memory_order_relaxed);
    return 0.0f;
}
}

#pragma once

#include <JuceHeader.h>

namespace chrome::params
{
juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
juce::String slotId (int slot, const juce::String& name);
juce::StringArray delayDivisionNames();

inline float delayDivisionQuarterNotes (int index) noexcept
{
    constexpr std::array<float, 13> values { 4.0f, 3.0f, 2.0f, 1.5f, 1.0f, 0.75f, 2.0f / 3.0f,
                                              0.5f, 0.375f, 1.0f / 3.0f, 0.25f, 1.0f / 6.0f, 0.125f };
    return values[static_cast<size_t> (juce::jlimit (0, static_cast<int> (values.size()) - 1, index))];
}

inline float load (const juce::AudioProcessorValueTreeState& state, juce::StringRef id)
{
    if (const auto* value = state.getRawParameterValue (id))
        return value->load (std::memory_order_relaxed);
    return 0.0f;
}
}

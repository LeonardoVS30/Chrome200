#pragma once

#include "../SampleEngine/SampleData.h"

namespace chrome
{
class WaveformComponent final : public juce::Component, private juce::Timer
{
public:
    enum class ViewMode { wave, spectrum, modulation };
    WaveformComponent (juce::AudioProcessorValueTreeState&, int slot);
    void setSample (std::shared_ptr<const SampleData>);
    void setViewMode (ViewMode mode) { viewMode = mode; repaint(); }
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }
    juce::AudioProcessorValueTreeState& state;
    int slotIndex;
    std::shared_ptr<const SampleData> current;
    ViewMode viewMode = ViewMode::wave;
};
}

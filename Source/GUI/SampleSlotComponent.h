#pragma once

#include <JuceHeader.h>
#include "WaveformComponent.h"

namespace chrome
{
class SampleEngine;

class SampleSlotComponent final : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    SampleSlotComponent (juce::AudioProcessorValueTreeState&, SampleEngine&, int slot);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void refresh();
    void setFigmaSkinMode (bool enabled);
    void setViewMode (WaveformComponent::ViewMode view) { waveform.setViewMode (view); }

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    void configureKnob (juce::Slider&, const juce::String& suffix, const juce::String& tooltip);
    void chooseFile();

    juce::AudioProcessorValueTreeState& state;
    SampleEngine& engine;
    int slotIndex;
    juce::Label title, fileName;
    WaveformComponent waveform;
    juce::TextButton browse { "LOAD" }, loop { "LOOP" }, reverse { "REV" }, solo { "S" }, mute { "M" }, power { "ON" };
    juce::ComboBox mode;
    juce::Slider gain, tune, fine, start, end, pan;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::unique_ptr<ComboAttachment> modeAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    bool figmaSkinMode = false;
};
}

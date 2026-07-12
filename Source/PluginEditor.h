#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/ChromeLookAndFeel.h"
#include "GUI/SampleSlotComponent.h"

class Chrome2000AudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Chrome2000AudioProcessorEditor (Chrome2000AudioProcessor&);
    ~Chrome2000AudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class BoundKnob final : public juce::Component
    {
    public:
        BoundKnob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name);
        void resized() override;
    private:
        juce::Label label;
        juce::Slider slider;
        juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    };
    class LfoDisplay final : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
    };
    void timerCallback() override;
    void rebuildPresetList();
    void savePreset();
    void showAbout();

    Chrome2000AudioProcessor& processor;
    chrome::ChromeLookAndFeel lookAndFeel;
    juce::Label logo, subtitle, outputLabel, credit;
    juce::TextButton logoHitArea;
    juce::ComboBox presetBox;
    juce::TextButton previous { "<" }, next { ">" }, save { "SAVE" }, power { "POWER" };
    juce::TextButton waveTab { "WAVE" }, spectrumTab { "SPECTRUM" }, modTab { "MOD" };
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment;
    std::array<std::unique_ptr<chrome::SampleSlotComponent>, 3> slots;
    std::vector<std::unique_ptr<BoundKnob>> chromeKnobs, filterKnobs, envelopeKnobs, effectKnobs, modulationKnobs;
    juce::ComboBox filterType, modulationSource, modulationDestination, lfoShape;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment filterTypeAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment modulationSourceAttachment, modulationDestinationAttachment, lfoShapeAttachment;
    LfoDisplay lfoDisplay;
    juce::Slider morphX;
    juce::AudioProcessorValueTreeState::SliderAttachment morphXAttachment;
    float displayedPeak = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chrome2000AudioProcessorEditor)
};

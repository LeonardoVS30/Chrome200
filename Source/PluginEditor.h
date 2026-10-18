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
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    class BoundKnob final : public juce::Component, private juce::Slider::Listener
    {
    public:
        BoundKnob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name,
                   bool large = false, bool glow = false);
        void resized() override;
        void setFigmaSkinMode (bool enabled);
        void bindToParameter (const juce::String& id);
        const juce::String& getValueText() const { return valueText; }
        std::function<void()> onDragStart;
        std::function<void()> onDragEnd;
    private:
        class ParameterSlider final : public juce::Slider
        {
        public:
            bool snapToSemitones = false;
            double snapValue (double attemptedValue, DragMode dragMode) override;
        };
        void sliderValueChanged (juce::Slider*) override;
        void mouseDown (const juce::MouseEvent&) override;
        void showValueEditor();
        double valueFromText (juce::String) const;
        void updateValue();
        juce::AudioProcessorValueTreeState& state;
        juce::String parameterId;
        juce::String displayName;
        juce::String valueText;
        juce::Label label;
        juce::Label value;
        ParameterSlider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        bool figmaSkinMode = false;
    };
    class ValueBubble final : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
        void setText (const juce::String& t) { text = t; repaint(); }
    private:
        juce::String text;
    };
    class LfoDisplay final : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
    };
    class AboutOverlay final : public juce::Component
    {
    public:
        AboutOverlay();
        void paint (juce::Graphics&) override;
        void resized() override;
    private:
        juce::TextButton close { "CERRAR" };
    };
    void timerCallback() override;
    void updateModulationControls();
    void updatePowerState();
    void updateDelayControls();
    void rebuildPresetList();
    void savePreset();
    void showAbout();
    static std::unique_ptr<juce::Drawable> drawableFromData (const void*, int);

    Chrome2000AudioProcessor& processor;
    chrome::ChromeLookAndFeel lookAndFeel;
    juce::Label logo, subtitle, outputLabel, credit;
    juce::Image figmaSkin;
    std::unique_ptr<juce::Drawable> logoMark, wordmark;
    juce::TextButton logoHitArea;
    juce::ComboBox presetBox;
    juce::TextButton previous { "<" }, next { ">" }, save { "SAVE" }, power { "POWER" };
    juce::TextButton waveTab { "WAVE" }, spectrumTab { "SPECTRUM" }, modTab { "MOD" };
    juce::TextButton legato { "LEGATO" };
    juce::TextButton chorusToggle { " " }, delayToggle { " " }, reverbToggle { " " }, delaySync { "SYNC" };
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment legatoAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment chorusToggleAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment delayToggleAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment reverbToggleAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment delaySyncAttachment;
    std::array<std::unique_ptr<chrome::SampleSlotComponent>, 3> slots;
    std::vector<std::unique_ptr<BoundKnob>> filterKnobs, envelopeKnobs, pitchKnobs, effectKnobs, masterKnobs, modulationKnobs;
    juce::ComboBox filterType, modulationSource, modulationDestination, lfoShape, lfo2Shape, lfo3Shape, delayDivision;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment filterTypeAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment modulationSourceAttachment, modulationDestinationAttachment, lfoShapeAttachment, lfo2ShapeAttachment, lfo3ShapeAttachment;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment delayDivisionAttachment;
    LfoDisplay lfoDisplay;
    ValueBubble valueBubble;
    AboutOverlay aboutOverlay;
    juce::Slider morphX;
    juce::AudioProcessorValueTreeState::SliderAttachment morphXAttachment;
    float displayedPeak = 0.0f;
    bool controlsEnabled = true;
    BoundKnob* activeDragKnob = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chrome2000AudioProcessorEditor)
};

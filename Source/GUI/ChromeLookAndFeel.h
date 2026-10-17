#pragma once

#include <JuceHeader.h>

namespace chrome
{
class ChromeLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ChromeLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float position,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    juce::Font getLabelFont (juce::Label&) override;

private:
    static void drawAsset (juce::Graphics&, const std::unique_ptr<juce::Drawable>&, juce::Rectangle<float>);

    juce::Typeface::Ptr inter;
    std::unique_ptr<juce::Drawable> smallFace, smallRim, smallShadow, smallPatina, smallSpecular;
    std::unique_ptr<juce::Drawable> bigFace, bigRim, bigShadow, bigPatina, bigSpecular, bigHalo;
};
}

#include "WaveformComponent.h"
#include "../Parameters/ParameterLayout.h"

namespace chrome
{
WaveformComponent::WaveformComponent (juce::AudioProcessorValueTreeState& parameters, int slot)
    : state (parameters), slotIndex (slot)
{
    startTimerHz (20);
}

void WaveformComponent::setSample (std::shared_ptr<const SampleData> sample)
{
    current = std::move (sample); repaint();
}

void WaveformComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff242628), b.getX(), b.getY(), juce::Colour (0xff4b4d50), b.getX(), b.getBottom(), false)); g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colour (0xff8d9093)); g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);
    if (! current && viewMode != ViewMode::modulation)
    {
        g.setColour (juce::Colour (0xffb4b7ba)); g.setFont (11.0f); g.drawFittedText ("DROP WAV / AIFF / FLAC", getLocalBounds(), juce::Justification::centred, 1); return;
    }
    if (viewMode == ViewMode::spectrum)
    {
        const auto& values = current->spectrum(); const auto width = b.getWidth() / static_cast<float> (values.size());
        for (size_t i = 0; i < values.size(); ++i)
        {
            const auto height = values[i] * b.getHeight() * 0.82f;
            g.setColour (juce::Colour (0xff85b3ff).withAlpha (0.32f + values[i] * 0.55f));
            g.fillRoundedRectangle (b.getX() + static_cast<float> (i) * width, b.getBottom() - height, juce::jmax (1.0f, width - 1.0f), height, 1.0f);
        }
        return;
    }
    if (viewMode == ViewMode::modulation)
    {
        const auto shape = static_cast<int> (params::load (state, "lfo1_shape")); juce::Path path;
        for (int x = 0; x <= getWidth(); ++x)
        {
            const auto phase = static_cast<float> (x) / juce::jmax (1, getWidth()) * 3.0f;
            float value = 0.0f; const auto unit = phase - std::floor (phase);
            if (shape == 0) value = std::sin (phase * juce::MathConstants<float>::twoPi);
            else if (shape == 1) value = 1.0f - 4.0f * std::abs (unit - 0.5f);
            else if (shape == 2) value = unit * 2.0f - 1.0f;
            else if (shape == 3) value = unit < 0.5f ? 1.0f : -1.0f;
            else value = std::sin (phase * 9.73f) * 0.75f;
            const auto y = b.getCentreY() - value * b.getHeight() * 0.36f;
            if (x == 0) path.startNewSubPath (static_cast<float> (x), y); else path.lineTo (static_cast<float> (x), y);
        }
        g.setColour (juce::Colour (0xff85b3ff)); g.strokePath (path, juce::PathStrokeType (1.4f)); return;
    }
    const auto& peaks = current->peaks(); juce::Path waveform; const auto centre = b.getCentreY();
    for (size_t i = 0; i < peaks.size(); ++i)
    {
        const auto x = b.getX() + static_cast<float> (i) / static_cast<float> (peaks.size() - 1) * b.getWidth(); const auto y = peaks[i] * b.getHeight() * 0.42f;
        waveform.startNewSubPath (x, centre - y); waveform.lineTo (x, centre + y);
    }
    g.setColour (juce::Colour (0xffe2e3e5).withAlpha (0.88f)); g.strokePath (waveform, juce::PathStrokeType (1.0f));
    const auto start = params::load (state, params::slotId (slotIndex, "start"));
    const auto end = params::load (state, params::slotId (slotIndex, "end"));
    g.setColour (juce::Colour (0x66303336));
    g.fillRect (b.withRight (b.getX() + b.getWidth() * juce::jmin (start, end)));
    g.fillRect (b.withLeft (b.getX() + b.getWidth() * juce::jmax (start, end)));
    g.setColour (juce::Colour (0xff72a8ff));
    g.drawVerticalLine (static_cast<int> (b.getX() + b.getWidth() * start), b.getY(), b.getBottom());
    g.drawVerticalLine (static_cast<int> (b.getX() + b.getWidth() * end), b.getY(), b.getBottom());
}
}

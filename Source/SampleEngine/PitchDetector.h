#pragma once

#include <JuceHeader.h>

namespace chrome
{
struct PitchDetectionResult
{
    float frequencyHz = 0.0f;
    float midiPitch = -1.0f;
    float confidence = 0.0f;

    [[nodiscard]] bool isValid() const noexcept
    {
        return frequencyHz > 0.0f && midiPitch >= 0.0f && confidence >= 0.60f;
    }
};

// Intended for the sample-loader thread only. Work and memory are bounded by a
// short analysis window, irrespective of the source file's duration.
PitchDetectionResult detectPitch (const juce::AudioBuffer<float>&, double sampleRate);
juce::String formatMidiPitch (float midiPitch, bool includeCents = true);
}

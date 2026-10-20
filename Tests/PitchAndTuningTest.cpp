#include <JuceHeader.h>
#include "../Source/SampleEngine/PitchDetector.h"
#include "../Source/SampleEngine/SampleData.h"
#include "../Source/Engine/Voice.h"
#include <iostream>

namespace
{
juce::AudioBuffer<float> makeTone (double sampleRate, double frequency, bool harmonics = false)
{
    const auto length = static_cast<int> (sampleRate);
    juce::AudioBuffer<float> result (2, length);
    for (int sample = 0; sample < length; ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi * frequency * sample / sampleRate;
        auto value = 0.22 * std::sin (phase);
        if (harmonics)
            value += 0.38 * std::sin (phase * 2.0) + 0.14 * std::sin (phase * 3.0);
        result.setSample (0, sample, static_cast<float> (value));
        result.setSample (1, sample, static_cast<float> (value));
    }
    return result;
}

juce::AudioBuffer<float> renderVoice (const std::shared_ptr<const chrome::SampleData>& sample,
                                      int note, float rootPitch)
{
    chrome::Voice voice;
    voice.prepare (44100.0);
    voice.updateEnvelope (0.001f, 0.01f, 1.0f, 0.01f);
    std::array<chrome::SlotPlayback, 3> slots {};
    slots[0].sample = sample;
    slots[0].rootPitch = rootPitch;
    slots[0].loop = true;
    slots[0].start = 0.0f;
    slots[0].end = 1.0f;
    slots[1].audible = false;
    slots[2].audible = false;
    voice.start (note, 1.0f, 1, slots, static_cast<float> (note), 0.0f, false);

    juce::AudioBuffer<float> output (2, 4096);
    output.clear();
    std::array<juce::AudioBuffer<float>*, 3> destinations { &output, nullptr, nullptr };
    voice.render (destinations, 0, output.getNumSamples(), slots);
    return output;
}
}

int main()
{
    constexpr double sampleRate = 44100.0;
    constexpr double dSharp4 = 311.1269837220809;
    chrome::PitchDetectionResult clean;
    for (const auto rate : { 44100.0, 48000.0, 96000.0 })
    {
        clean = chrome::detectPitch (makeTone (rate, dSharp4), rate);
        if (! clean.isValid() || std::abs (clean.midiPitch - 63.0f) > 0.08f)
        {
            std::cerr << "FAIL: D#4 detection at " << rate << " Hz, midi=" << clean.midiPitch
                      << " confidence=" << clean.confidence << '\n';
            return 1;
        }
    }

    const auto detuned = chrome::detectPitch (makeTone (sampleRate, dSharp4 * std::exp2 (23.0 / 1200.0)), sampleRate);
    if (! detuned.isValid() || std::abs (detuned.midiPitch - 63.23f) > 0.08f)
    {
        std::cerr << "FAIL: cent offset detection midi=" << detuned.midiPitch << '\n';
        return 5;
    }

    const auto harmonic = chrome::detectPitch (makeTone (sampleRate, 261.6255653005986, true), sampleRate);
    if (! harmonic.isValid() || std::abs (harmonic.midiPitch - 60.0f) > 0.10f)
    {
        std::cerr << "FAIL: harmonic C4 detection midi=" << harmonic.midiPitch << " confidence=" << harmonic.confidence << '\n';
        return 2;
    }

    juce::AudioBuffer<float> silence (2, 44100);
    silence.clear();
    if (chrome::detectPitch (silence, sampleRate).isValid())
    {
        std::cerr << "FAIL: silence produced a root note\n";
        return 3;
    }

    auto sourceAudio = makeTone (sampleRate, 261.6255653005986);
    const auto sample = std::make_shared<const chrome::SampleData> (std::move (sourceAudio), sampleRate, juce::File());
    const auto cAtC = renderVoice (sample, 60, 60.0f);
    const auto dSharpAtDSharp = renderVoice (sample, 63, 63.0f);
    const auto dSharpAtC = renderVoice (sample, 60, 63.0f);
    float equivalentDifference = 0.0f;
    float transposedDifference = 0.0f;
    for (int i = 0; i < cAtC.getNumSamples(); ++i)
    {
        equivalentDifference = juce::jmax (equivalentDifference,
                                            std::abs (cAtC.getSample (0, i) - dSharpAtDSharp.getSample (0, i)));
        transposedDifference = juce::jmax (transposedDifference,
                                           std::abs (cAtC.getSample (0, i) - dSharpAtC.getSample (0, i)));
    }
    if (equivalentDifference > 1.0e-6f || transposedDifference < 0.05f)
    {
        std::cerr << "FAIL: per-slot root pitch is not applied independently, equivalent="
                  << equivalentDifference << " transposed=" << transposedDifference << '\n';
        return 4;
    }

    std::cout << "DetectedDSharp=" << clean.midiPitch << " confidence=" << clean.confidence
              << " HarmonicC=" << harmonic.midiPitch << " confidence=" << harmonic.confidence << '\n';
    std::cout << "PASS: bounded root detection and per-slot tuning\n";
    return 0;
}

#include "Voice.h"

namespace chrome
{
void Voice::prepare (double sampleRate)
{
    outputRate = sampleRate;
    envelope.setSampleRate (sampleRate);
    envelope.setParameters (envelopeParameters);
}

void Voice::updateEnvelope (float attack, float decay, float sustain, float releaseTime)
{
    envelopeParameters = { attack, decay, sustain, releaseTime };
    envelope.setParameters (envelopeParameters);
}

void Voice::start (int noteNumber, float velocity, uint64_t age, const std::array<SlotPlayback, 3>& slots)
{
    midiNote = noteNumber;
    noteVelocity = velocity;
    ageStamp = age;
    for (size_t i = 0; i < positions.size(); ++i)
    {
        if (const auto& sample = slots[i].sample)
        {
            const auto first = slots[i].start * static_cast<float> (sample->length() - 1);
            const auto last = slots[i].end * static_cast<float> (sample->length() - 1);
            positions[i] = slots[i].reverse ? juce::jmax (first, last) : juce::jmin (first, last);
        }
    }
    envelope.reset();
    envelope.noteOn();
    isActive = true;
}

void Voice::release() { envelope.noteOff(); }

void Voice::kill() noexcept
{
    envelope.reset();
    isActive = false;
    midiNote = -1;
}

void Voice::render (std::array<juce::AudioBuffer<float>*, 3> outputs, int startSample, int numSamples,
                    const std::array<SlotPlayback, 3>& slots)
{
    if (! isActive)
        return;

    for (int frame = 0; frame < numSamples; ++frame)
    {
        const auto amp = envelope.getNextSample() * noteVelocity;
        if (! envelope.isActive())
        {
            kill();
            break;
        }

        for (size_t slot = 0; slot < slots.size(); ++slot)
        {
            const auto& settings = slots[slot];
            const auto& source = settings.sample;
            if (! source || ! settings.audible || outputs[slot] == nullptr)
                continue;

            const auto low = static_cast<double> (juce::jmin (settings.start, settings.end) * static_cast<float> (source->length() - 1));
            const auto high = static_cast<double> (juce::jmax (settings.start, settings.end) * static_cast<float> (source->length() - 1));
            if (high - low < 2.0)
                continue;

            const auto semitones = static_cast<double> (midiNote - 60) + settings.tune;
            auto increment = source->sampleRate() / outputRate * std::exp2 (semitones / 12.0);
            if (settings.reverse)
                increment = -increment;

            const auto leftGain = settings.gain * std::sqrt (0.5f * (1.0f - settings.pan));
            const auto rightGain = settings.gain * std::sqrt (0.5f * (1.0f + settings.pan));
            const auto p = positions[slot];
            outputs[slot]->addSample (0, startSample + frame, source->sample (0, p) * amp * leftGain);
            outputs[slot]->addSample (1, startSample + frame, source->sample (source->channels() > 1 ? 1 : 0, p) * amp * rightGain);
            positions[slot] += increment;

            if (positions[slot] < low || positions[slot] >= high)
            {
                if (settings.loop)
                {
                    const auto range = high - low;
                    while (positions[slot] >= high) positions[slot] -= range;
                    while (positions[slot] < low) positions[slot] += range;
                }
                else
                {
                    positions[slot] = juce::jlimit (low, high - 1.0, positions[slot]);
                }
            }
        }
    }
}
}

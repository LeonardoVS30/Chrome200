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

void Voice::start (int noteNumber, float velocity, uint64_t age, const std::array<SlotPlayback, 3>& slots, float glideFromNote, float glideTime, bool legato)
{
    if (legato && isActive)
    {
        glideTarget = static_cast<double> (noteNumber);
        if (glideTime > 0.001f)
        {
            gliding = true;
            glideStep = (glideTarget - glideNote) / static_cast<double> (juce::jmax (1.0, glideTime * outputRate));
        }
        else
        {
            glideNote = glideTarget;
            gliding = false;
        }
        midiNote = noteNumber;
        noteVelocity = velocity;
        ageStamp = age;
        hasPlayed = true;
        return;
    }

    midiNote = noteNumber;
    noteVelocity = velocity;
    ageStamp = age;
    for (size_t i = 0; i < positions.size(); ++i)
    {
        slotActive[i] = false;
        if (const auto& sample = slots[i].sample)
        {
            const auto first = slots[i].start * static_cast<float> (sample->length() - 1);
            const auto last = slots[i].end * static_cast<float> (sample->length() - 1);
            positions[i] = slots[i].reverse ? juce::jmax (first, last) : juce::jmin (first, last);
            slotActive[i] = slots[i].audible && std::abs (last - first) >= 2.0f;
        }
    }
    envelope.reset();
    envelope.noteOn();
    isActive = true;
    if (glideTime > 0.001f && hasPlayed && glideFromNote >= 0.0f)
    {
        glideNote = static_cast<double> (glideFromNote);
        glideTarget = static_cast<double> (noteNumber);
        gliding = true;
        glideStep = (glideTarget - glideNote) / static_cast<double> (juce::jmax (1.0, glideTime * outputRate));
    }
    else
    {
        glideNote = static_cast<double> (noteNumber);
        glideTarget = glideNote;
        gliding = false;
    }
    hasPlayed = true;
}

void Voice::release() { envelope.noteOff(); }

void Voice::kill() noexcept
{
    envelope.reset();
    isActive = false;
    midiNote = -1;
    lastEnvelopeLevel = 0.0f;
    slotActive.fill (false);
}

void Voice::render (std::array<juce::AudioBuffer<float>*, 3> outputs, int startSample, int numSamples,
                    const std::array<SlotPlayback, 3>& slots)
{
    if (! isActive)
        return;

    for (int frame = 0; frame < numSamples; ++frame)
    {
        const auto envLevel = envelope.getNextSample();
        const auto amp = envLevel * noteVelocity;
        lastEnvelopeLevel = envLevel;
        if (! envelope.isActive())
        {
            kill();
            break;
        }

        for (size_t slot = 0; slot < slots.size(); ++slot)
        {
            const auto& settings = slots[slot];
            const auto& source = settings.sample;
            if (! slotActive[slot] || ! source || ! settings.audible || outputs[slot] == nullptr)
                continue;

            const auto low = static_cast<double> (juce::jmin (settings.start, settings.end) * static_cast<float> (source->length() - 1));
            const auto high = static_cast<double> (juce::jmax (settings.start, settings.end) * static_cast<float> (source->length() - 1));
            const auto range = high - low;
            if (range < 2.0)
                continue;

            const auto semitones = glideNote - 60.0 + static_cast<double> (settings.tune);
            auto increment = source->sampleRate() / outputRate * std::exp2 (semitones / 12.0);
            if (settings.reverse)
                increment = -increment;

            const auto leftGain = settings.gain * std::sqrt (0.5f * (1.0f - settings.pan));
            const auto rightGain = settings.gain * std::sqrt (0.5f * (1.0f + settings.pan));
            const auto p = positions[slot];
            const auto crossfadeLength = settings.loop
                                           ? juce::jmin (source->sampleRate() * 0.005, range * 0.20)
                                           : 0.0;
            const auto loopSample = [&] (int channel)
            {
                const auto primary = source->sample (channel, p);
                if (crossfadeLength < 2.0)
                    return primary;

                if (! settings.reverse)
                {
                    const auto crossfadeStart = high - crossfadeLength;
                    if (p >= crossfadeStart)
                    {
                        const auto mix = juce::jlimit (0.0, 1.0, (p - crossfadeStart) / crossfadeLength);
                        const auto wrappedPosition = low + (p - crossfadeStart);
                        return static_cast<float> (primary + (source->sample (channel, wrappedPosition) - primary) * mix);
                    }
                }
                else
                {
                    const auto crossfadeEnd = low + crossfadeLength;
                    if (p <= crossfadeEnd)
                    {
                        const auto mix = juce::jlimit (0.0, 1.0, (crossfadeEnd - p) / crossfadeLength);
                        const auto wrappedPosition = high - crossfadeLength + (p - low);
                        return static_cast<float> (primary + (source->sample (channel, wrappedPosition) - primary) * mix);
                    }
                }
                return primary;
            };
            outputs[slot]->addSample (0, startSample + frame, loopSample (0) * amp * leftGain);
            outputs[slot]->addSample (1, startSample + frame, loopSample (source->channels() > 1 ? 1 : 0) * amp * rightGain);
            positions[slot] += increment;

            if (positions[slot] < low || positions[slot] >= high)
            {
                if (settings.loop)
                {
                    const auto loopSpan = range - crossfadeLength;
                    while (positions[slot] >= high) positions[slot] -= loopSpan;
                    while (positions[slot] < low) positions[slot] += loopSpan;
                }
                else
                {
                    slotActive[slot] = false;
                }
            }
        }

        if (gliding)
        {
            glideNote += glideStep;
            if (glideStep > 0.0 && glideNote >= glideTarget) { glideNote = glideTarget; gliding = false; }
            else if (glideStep < 0.0 && glideNote <= glideTarget) { glideNote = glideTarget; gliding = false; }
        }

        if (std::none_of (slotActive.begin(), slotActive.end(), [] (bool active) { return active; }))
        {
            kill();
            break;
        }
    }
}
}

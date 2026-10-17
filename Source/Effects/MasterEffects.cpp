#include "MasterEffects.h"

namespace chrome
{
void MasterEffects::prepare (double sampleRate, int maxBlockSize)
{
    rate = sampleRate;
    juce::dsp::ProcessSpec stereo { sampleRate, static_cast<juce::uint32> (maxBlockSize), 2 };
    chorus.prepare (stereo); delay.prepare (stereo); reverb.prepare (stereo);
    compressor.prepare (stereo); limiter.prepare (stereo);
    juce::dsp::ProcessSpec mono { sampleRate, static_cast<juce::uint32> (maxBlockSize), 1 };
    const auto dcCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 12.0, 0.70710678f);
    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf[ch].prepare (mono);
        highShelf[ch].prepare (mono);
        dcBlocker[ch].prepare (mono);
        dcBlocker[ch].coefficients = dcCoefficients;
    }
    wetBuffer.setSize (2, maxBlockSize, false, true, true);
    oversampling2x.initProcessing (static_cast<size_t> (maxBlockSize));
    oversampling4x.initProcessing (static_cast<size_t> (maxBlockSize));
    chorus.setRate (0.28f); chorus.setDepth (0.22f); chorus.setCentreDelay (11.0f); chorus.setFeedback (0.08f);
    delay.setDelay (static_cast<float> (sampleRate * 0.375));
    compressor.setAttack (12.0f); compressor.setRelease (120.0f); compressor.setRatio (3.0f);
    limiter.setThreshold (-0.3f); limiter.setRelease (60.0f);
    reset();
}

void MasterEffects::reset()
{
    chorus.reset(); delay.reset(); reverb.reset(); compressor.reset(); limiter.reset();
    oversampling2x.reset(); oversampling4x.reset();
    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf[ch].reset();
        highShelf[ch].reset();
        dcBlocker[ch].reset();
    }
}

void MasterEffects::process (juce::AudioBuffer<float>& buffer, const MasterControls& c)
{
    buffer.applyGain (juce::Decibels::decibelsToGain (c.inputGain));
    auto block = juce::dsp::AudioBlock<float> (buffer);
    chorus.setMix (c.chorus);
    chorus.process (juce::dsp::ProcessContextReplacing<float> (block));

    delay.setDelay (static_cast<float> (rate) * c.delayTime);
    const auto feedback = c.delayFeedback;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto dry = buffer.getSample (ch, i);
            const auto echo = delay.popSample (ch);
            delay.pushSample (ch, dry + echo * feedback);
            buffer.setSample (ch, i, dry + echo * c.delay);
        }

    juce::dsp::Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = c.reverbSize;
    reverbParameters.damping = c.reverbDamp;
    reverbParameters.wetLevel = c.reverb;
    reverbParameters.dryLevel = 1.0f;
    reverbParameters.width = 1.0f;
    reverb.setParameters (reverbParameters);
    reverb.process (juce::dsp::ProcessContextReplacing<float> (block));

    const auto threshold = -2.0f - c.compressor * 22.0f;
    compressor.setThreshold (threshold);
    compressor.process (juce::dsp::ProcessContextReplacing<float> (block));

    const auto low = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (rate, 180.0f, 0.7f, juce::Decibels::decibelsToGain (c.lowEq));
    const auto high = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (rate, 6000.0f, 0.7f, juce::Decibels::decibelsToGain (c.highEq));
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        *lowShelf[static_cast<size_t> (ch)].coefficients = low;
        *highShelf[static_cast<size_t> (ch)].coefficients = high;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            auto x = lowShelf[static_cast<size_t> (ch)].processSample (buffer.getSample (ch, i));
            x = highShelf[static_cast<size_t> (ch)].processSample (x);
            buffer.setSample (ch, i, x);
        }
    }

    if (buffer.getNumChannels() >= 2)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto mid = (buffer.getSample (0, i) + buffer.getSample (1, i)) * 0.5f;
            const auto side = (buffer.getSample (0, i) - buffer.getSample (1, i)) * 0.5f * c.width;
            buffer.setSample (0, i, mid + side);
            buffer.setSample (1, i, mid - side);
        }

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample (ch, i, dcBlocker[static_cast<size_t> (ch)].processSample (buffer.getSample (ch, i)));

    buffer.applyGain (juce::Decibels::decibelsToGain (c.gain));
    if (c.softClip)
    {
        auto saturate = [] (juce::dsp::AudioBlock<float> target)
        {
            for (size_t ch = 0; ch < target.getNumChannels(); ++ch)
                for (size_t i = 0; i < target.getNumSamples(); ++i)
                    target.setSample (static_cast<int> (ch), static_cast<int> (i), std::tanh (target.getSample (static_cast<int> (ch), static_cast<int> (i)) * 1.15f) / std::tanh (1.15f));
        };
        if (c.oversampling == 1)
        {
            auto highRate = oversampling2x.processSamplesUp (block); saturate (highRate); oversampling2x.processSamplesDown (block);
        }
        else if (c.oversampling >= 2)
        {
            auto highRate = oversampling4x.processSamplesUp (block); saturate (highRate); oversampling4x.processSamplesDown (block);
        }
        else saturate (block);
    }
    limiter.process (juce::dsp::ProcessContextReplacing<float> (block));
}
}

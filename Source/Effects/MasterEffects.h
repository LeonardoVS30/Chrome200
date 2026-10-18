#pragma once

#include <JuceHeader.h>

namespace chrome
{
struct MasterControls
{
    float chorus, delay, delayTime, delayFeedback, reverb, reverbSize, reverbDamp;
    float compressor, width, lowEq, highEq, inputGain, gain;
    bool softClip;
    int oversampling;
};

class MasterEffects final
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>&, const MasterControls&);

private:
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 1 };
    juce::dsp::Reverb reverb;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::Oversampling<float> oversampling2x { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::Oversampling<float> oversampling4x { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    std::array<juce::dsp::IIR::Filter<float>, 2> lowShelf, highShelf, dcBlocker;
    juce::AudioBuffer<float> wetBuffer;
    double rate = 44100.0;
    float maximumDelaySamples = 1.0f;
};
}

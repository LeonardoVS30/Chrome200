#include <JuceHeader.h>
#include "../Source/Parameters/ParameterLayout.h"
#include "../Source/SampleEngine/SampleEngine.h"
#include <iostream>

namespace
{
class HarnessProcessor final : public juce::AudioProcessor
{
public:
    HarnessProcessor()
        : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    const juce::String getName() const override { return "ChromeEngineHarness"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
};

bool writeTestWave (const juce::File& file)
{
    file.deleteFile();
    auto stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wav;
    auto writer = std::unique_ptr<juce::AudioFormatWriter> (
        wav.createWriterFor (stream.release(), 44100.0, 2, 24, {}, 0));
    if (writer == nullptr)
        return false;

    constexpr int length = 44100;
    juce::AudioBuffer<float> signal (2, length);
    for (int sample = 0; sample < length; ++sample)
    {
        // Deliberately non-periodic over one second so hard loop wrapping would
        // produce an obvious discontinuity and broadband click.
        const auto phase = juce::MathConstants<double>::twoPi * 220.37 * static_cast<double> (sample) / 44100.0;
        const auto value = static_cast<float> (std::sin (phase) * 0.35);
        signal.setSample (0, sample, value);
        signal.setSample (1, sample, value);
    }
    return writer->writeFromAudioSampleBuffer (signal, 0, length);
}

void setParameter (juce::AudioProcessorValueTreeState& state, const juce::String& id, float actual)
{
    if (auto* parameter = state.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (actual));
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    HarnessProcessor processor;
    juce::AudioProcessorValueTreeState state (processor, nullptr, "ENGINE_TEST", chrome::params::createLayout());
    chrome::SampleEngine engine (state);
    engine.prepare (44100.0, 512);

    const auto wave = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("chrome2000-engine-smoke.wav");
    if (! writeTestWave (wave))
    {
        std::cerr << "FAIL: unable to create test WAV\n";
        return 1;
    }

    engine.loadSample (0, wave);
    for (int wait = 0; wait < 500 && engine.sample (0) == nullptr; ++wait)
        juce::Thread::sleep (10);
    if (engine.sample (0) == nullptr)
    {
        std::cerr << "FAIL: asynchronous sample loading timed out\n";
        return 2;
    }

    engine.reset();

    setParameter (state, "s1_enabled", 1.0f);
    setParameter (state, "s1_mute", 0.0f);
    setParameter (state, "s1_solo", 1.0f);
    setParameter (state, "s1_mode", 1.0f);
    setParameter (state, "s1_start", 0.0f);
    setParameter (state, "s1_end", 1.0f);
    setParameter (state, "s1_loop", 1.0f);
    setParameter (state, "filter_cutoff", 8720.0f);
    setParameter (state, "filter_resonance", 0.7f);
    setParameter (state, "chorus", 0.0f);
    setParameter (state, "delay", 0.0f);
    setParameter (state, "reverb", 0.0f);
    setParameter (state, "compressor", 0.0f);
    setParameter (state, "master_gain", -3.0f);

    juce::AudioBuffer<float> output (2, 512);
    struct RenderStats
    {
        float peak = 0.0f;
        double tailSum = 0.0;
        double tailEnergy = 0.0;
        double tailLeftEnergy = 0.0;
        double tailRightEnergy = 0.0;
        double tailCross = 0.0;
        int tailSamples = 0;
        float maximumTailDelta = 0.0f;
    };
    const auto render = [&] (int blocks, bool triggerNote = true)
    {
        RenderStats stats;
        std::array<float, 2> previous {};
        juce::MidiBuffer midi;
        if (triggerNote)
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        for (int block = 0; block < blocks; ++block)
        {
            output.clear();
            engine.process (output, midi);
            midi.clear();
            stats.peak = juce::jmax (stats.peak, output.getMagnitude (0, output.getNumSamples()));
            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                for (int sample = 0; sample < output.getNumSamples(); ++sample)
                {
                    const auto value = output.getSample (channel, sample);
                    if (! std::isfinite (value))
                    {
                        std::cerr << "FAIL: non-finite output sample\n";
                        std::exit (3);
                    }
                    if (block >= blocks - 80)
                    {
                        stats.maximumTailDelta = juce::jmax (stats.maximumTailDelta,
                                                             std::abs (value - previous[static_cast<size_t> (channel)]));
                        stats.tailSum += value;
                        stats.tailEnergy += static_cast<double> (value) * static_cast<double> (value);
                        if (channel == 0)
                            stats.tailLeftEnergy += static_cast<double> (value) * static_cast<double> (value);
                        else
                            stats.tailRightEnergy += static_cast<double> (value) * static_cast<double> (value);
                        ++stats.tailSamples;
                    }
                    previous[static_cast<size_t> (channel)] = value;
                }
            if (block >= blocks - 80 && output.getNumChannels() > 1)
                for (int sample = 0; sample < output.getNumSamples(); ++sample)
                    stats.tailCross += static_cast<double> (output.getSample (0, sample))
                                     * static_cast<double> (output.getSample (1, sample));
        }
        return stats;
    };
    const auto tailRms = [] (const RenderStats& stats)
    {
        return std::sqrt (stats.tailEnergy / static_cast<double> (juce::jmax (1, stats.tailSamples)));
    };
    const auto tailAcRms = [&] (const RenderStats& stats)
    {
        const auto rms = tailRms (stats);
        const auto mean = stats.tailSum / static_cast<double> (juce::jmax (1, stats.tailSamples));
        return std::sqrt (juce::jmax (0.0, rms * rms - mean * mean));
    };
    const auto stereoCorrelation = [] (const RenderStats& stats)
    {
        return stats.tailCross / std::sqrt (juce::jmax (1.0e-20, stats.tailLeftEnergy * stats.tailRightEnergy));
    };

    const auto sustained = render (400);
    const auto sustainedMean = sustained.tailSum / static_cast<double> (juce::jmax (1, sustained.tailSamples));
    const auto sustainedRms = std::sqrt (sustained.tailEnergy / static_cast<double> (juce::jmax (1, sustained.tailSamples)));
    const auto sustainedAcRms = std::sqrt (juce::jmax (0.0, sustainedRms * sustainedRms - sustainedMean * sustainedMean));
    std::cout << "LoopOnPeak=" << sustained.peak
              << " TailRMS=" << sustainedRms
              << " TailACRMS=" << sustainedAcRms
              << " TailDC=" << sustainedMean << '\n';
    if (sustained.peak < 1.0e-4f || sustainedAcRms < 1.0e-4)
    {
        std::cerr << "FAIL: looped voice collapsed into silence or DC\n";
        return 4;
    }

    engine.reset();
    setParameter (state, "s1_mode", 2.0f);
    setParameter (state, "s1_loop", 0.0f);
    const auto oneShot = render (400);
    const auto oneShotRms = std::sqrt (oneShot.tailEnergy / static_cast<double> (juce::jmax (1, oneShot.tailSamples)));
    const auto oneShotMean = oneShot.tailSum / static_cast<double> (juce::jmax (1, oneShot.tailSamples));
    std::cout << "LoopOffTailRMS=" << oneShotRms
              << " LoopOffTailDC=" << oneShotMean << '\n';
    if (oneShotRms > 1.0e-4)
    {
        std::cerr << "FAIL: completed one-shot voice emitted a fixed/DC tail\n";
        return 5;
    }

    setParameter (state, "s1_loop", 1.0f);
    constexpr std::array<const char*, 3> modeNames { "Ethernal", "Chrome", "Digital" };
    std::array<double, 3> modeTailRms {};
    std::array<float, 3> modeTailDelta {};
    std::array<float, 3> modePeak {};
    for (int mode = 0; mode < 3; ++mode)
    {
        engine.reset();
        setParameter (state, "s1_mode", static_cast<float> (mode));
        const auto modeStats = render (300);
        const auto modeMean = modeStats.tailSum / static_cast<double> (juce::jmax (1, modeStats.tailSamples));
        const auto modeRms = std::sqrt (modeStats.tailEnergy / static_cast<double> (juce::jmax (1, modeStats.tailSamples)));
        modeTailRms[static_cast<size_t> (mode)] = modeRms;
        modeTailDelta[static_cast<size_t> (mode)] = modeStats.maximumTailDelta;
        modePeak[static_cast<size_t> (mode)] = modeStats.peak;
        const auto modeAcRms = std::sqrt (juce::jmax (0.0, modeRms * modeRms - modeMean * modeMean));
        std::cout << modeNames[static_cast<size_t> (mode)]
                  << "Peak=" << modeStats.peak
                  << " TailACRMS=" << modeAcRms
                  << " TailDelta=" << modeStats.maximumTailDelta
                  << " TailDC=" << modeMean << '\n';
        if (modeStats.peak < 1.0e-4f || modeAcRms < 1.0e-4)
        {
            std::cerr << "FAIL: character mode " << modeNames[static_cast<size_t> (mode)]
                      << " collapsed into silence or DC\n";
            return 6 + mode;
        }
    }
    if (std::abs (modeTailRms[0] - modeTailRms[2]) < 1.0e-4
        || std::abs (modeTailRms[1] - modeTailRms[2]) < 1.0e-4)
    {
        std::cerr << "FAIL: clean character modes became indistinguishable from Digital\n";
        return 17;
    }
    if (modeTailDelta[0] > 0.08f || modeTailDelta[1] > 0.08f)
    {
        std::cerr << "FAIL: sustained character mode exposed a loop discontinuity\n";
        return 18;
    }
    if (modePeak[0] > modePeak[2] * 1.15f || modePeak[1] > modePeak[2] * 1.15f
        || modeTailRms[0] > modeTailRms[2] * 1.15 || modeTailRms[1] > modeTailRms[2] * 1.15)
    {
        std::cerr << "FAIL: character mode introduced an excessive level increase\n";
        return 19;
    }

    engine.reset();
    setParameter (state, "s1_loop", 1.0f);
    setParameter (state, "s1_mode", 2.0f);
    const auto transitionDigital = render (80);
    setParameter (state, "s1_mode", 1.0f);
    const auto transitionChrome = render (160, false);
    setParameter (state, "s1_mode", 0.0f);
    const auto transitionEthernal = render (160, false);
    const auto ethernalCorrelation = stereoCorrelation (transitionEthernal);
    std::cout << "TransitionDigitalPeak=" << transitionDigital.peak
              << " TransitionChromePeak=" << transitionChrome.peak
              << " TransitionEthernalPeak=" << transitionEthernal.peak
              << " EthernalCorrelation=" << ethernalCorrelation << '\n';
    if (transitionChrome.peak < 1.0e-4f || transitionChrome.peak > 1.0f
        || transitionEthernal.peak < 1.0e-4f || transitionEthernal.peak > 1.0f
        || tailAcRms (transitionChrome) < 1.0e-4 || tailAcRms (transitionEthernal) < 1.0e-4)
    {
        std::cerr << "FAIL: live character-mode transition became silent, fixed, or unstable\n";
        return 9;
    }
    if (ethernalCorrelation < 0.25)
    {
        std::cerr << "FAIL: Ethernal mode produced destructive stereo phase correlation\n";
        return 10;
    }

    setParameter (state, "s1_loop", 0.0f);
    for (int mode = 0; mode < 3; ++mode)
    {
        engine.reset();
        setParameter (state, "s1_mode", static_cast<float> (mode));
        const auto tail = render (800);
        const auto rms = tailRms (tail);
        std::cout << modeNames[static_cast<size_t> (mode)] << "OneShotTailRMS=" << rms << '\n';
        if (rms > 1.0e-4)
        {
            std::cerr << "FAIL: character mode " << modeNames[static_cast<size_t> (mode)]
                      << " left a sustained one-shot tail\n";
            return 11 + mode;
        }
    }

    // A released looped voice must not leave character processors producing
    // noise indefinitely after their input has become silent.
    setParameter (state, "s1_loop", 1.0f);
    for (int mode = 0; mode < 3; ++mode)
    {
        engine.reset();
        setParameter (state, "s1_mode", static_cast<float> (mode));
        if (mode == 1)
        {
            for (const auto* id : { "liquid", "reflect", "shine", "density", "fusion" })
                setParameter (state, id, 1.0f);
        }
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        for (int block = 0; block < 400; ++block)
        {
            output.clear();
            engine.process (output, midi);
            midi.clear();
        }

        midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
        double releaseEnergy = 0.0;
        int releaseSamples = 0;
        float releaseTailPeak = 0.0f;
        const auto measurementStart = mode < 2 ? 40 : 120;
        for (int block = 0; block < 160; ++block)
        {
            output.clear();
            engine.process (output, midi);
            midi.clear();
            if (block >= measurementStart)
            {
                releaseTailPeak = juce::jmax (releaseTailPeak, output.getMagnitude (0, output.getNumSamples()));
                for (int channel = 0; channel < output.getNumChannels(); ++channel)
                    for (int sample = 0; sample < output.getNumSamples(); ++sample)
                    {
                        const auto value = output.getSample (channel, sample);
                        releaseEnergy += static_cast<double> (value) * value;
                        ++releaseSamples;
                    }
            }
        }
        const auto releaseTailRms = std::sqrt (releaseEnergy / static_cast<double> (juce::jmax (1, releaseSamples)));
        std::cout << modeNames[static_cast<size_t> (mode)] << "ReleasedTailRMS=" << releaseTailRms
                  << " ReleasedTailPeak=" << releaseTailPeak << '\n';
        if (releaseTailRms > 1.0e-7 || releaseTailPeak > 1.0e-6f)
        {
            std::cerr << "FAIL: character mode " << modeNames[static_cast<size_t> (mode)]
                      << " left noise after note-off\n";
            return 14 + mode;
        }
        if (mode == 1)
        {
            setParameter (state, "liquid", 0.45f);
            setParameter (state, "reflect", 0.35f);
            setParameter (state, "shine", 0.55f);
            setParameter (state, "density", 0.5f);
            setParameter (state, "fusion", 0.4f);
        }
    }

    wave.deleteFile();
    std::cout << "PASS: looped audio, live mode changes, phase, and all one-shot tails are stable\n";
    return 0;
}

#include <JuceHeader.h>
#include "../Source/Parameters/ParameterLayout.h"
#include "../Source/SampleEngine/SampleEngine.h"
#include <iostream>
#include <cmath>

namespace
{
class HarnessProcessor final : public juce::AudioProcessor
{
public:
    HarnessProcessor() : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {}
    const juce::String getName() const override { return "FeatureHarness"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
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
    if (stream == nullptr) return false;
    juce::WavAudioFormat wav;
    auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.release(), 44100.0, 2, 24, {}, 0));
    if (writer == nullptr) return false;
    constexpr int length = 44100;
    juce::AudioBuffer<float> signal (2, length);
    for (int sample = 0; sample < length; ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi * 220.0 * static_cast<double> (sample) / 44100.0;
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

double renderRms (chrome::SampleEngine& engine, juce::AudioProcessorValueTreeState& state,
                  int blocks, bool triggerNote)
{
    juce::AudioBuffer<float> output (2, 512);
    juce::MidiBuffer midi;
    if (triggerNote) midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    double energy = 0.0; int count = 0;
    for (int b = 0; b < blocks; ++b)
    {
        output.clear();
        engine.process (output, midi);
        midi.clear();
        for (int ch = 0; ch < output.getNumChannels(); ++ch)
            for (int s = 0; s < output.getNumSamples(); ++s)
            {
                const auto v = output.getSample (ch, s);
                if (! std::isfinite (v)) { std::cerr << "FAIL: non-finite\n"; std::exit (3); }
                energy += static_cast<double> (v) * v; ++count;
            }
    }
    return std::sqrt (energy / static_cast<double> (juce::jmax (1, count)));
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    HarnessProcessor processor;
    juce::AudioProcessorValueTreeState state (processor, nullptr, "FEATURE_TEST", chrome::params::createLayout());
    chrome::SampleEngine engine (state);
    engine.prepare (44100.0, 512);

    const auto wave = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("chrome2000-feature-smoke.wav");
    if (! writeTestWave (wave)) { std::cerr << "FAIL: wav\n"; return 1; }
    engine.loadSample (0, wave);
    for (int wait = 0; wait < 500 && engine.sample (0) == nullptr; ++wait) juce::Thread::sleep (10);
    if (engine.sample (0) == nullptr) { std::cerr << "FAIL: load timeout\n"; return 2; }

    setParameter (state, "s1_enabled", 1.0f);
    setParameter (state, "s1_mute", 0.0f);
    setParameter (state, "s1_solo", 1.0f);
    setParameter (state, "s1_mode", 1.0f);
    setParameter (state, "s1_loop", 1.0f);
    setParameter (state, "s1_start", 0.0f);
    setParameter (state, "s1_end", 1.0f);
    setParameter (state, "filter_cutoff", 8720.0f);
    setParameter (state, "filter_resonance", 0.7f);
    setParameter (state, "chorus", 0.0f);
    setParameter (state, "delay", 0.0f);
    setParameter (state, "reverb", 0.0f);
    setParameter (state, "compressor", 0.0f);
    setParameter (state, "master_gain", -3.0f);
    setParameter (state, "glide", 0.0f);
    setParameter (state, "legato", 0.0f);

    const auto base = renderRms (engine, state, 120, true);

    // Chorus wet mix actually applied
    engine.reset();
    setParameter (state, "chorus", 1.0f);
    const auto chorusRms = renderRms (engine, state, 120, true);
    setParameter (state, "chorus", 0.0f);
    // Delay wet
    engine.reset();
    setParameter (state, "delay", 1.0f);
    const auto delayRms = renderRms (engine, state, 120, true);
    setParameter (state, "delay", 0.0f);
    // Reverb wet
    engine.reset();
    setParameter (state, "reverb", 1.0f);
    const auto reverbRms = renderRms (engine, state, 120, true);
    setParameter (state, "reverb", 0.0f);
    // Filter drive
    engine.reset();
    setParameter (state, "filter_drive", 24.0f);
    const auto driveRms = renderRms (engine, state, 120, true);
    setParameter (state, "filter_drive", 2.88f);

    std::cout << "base=" << base << " chorus=" << chorusRms << " delay=" << delayRms
              << " reverb=" << reverbRms << " drive=" << driveRms << '\n';

    bool ok = true;
    if (std::abs (chorusRms - base) < 1e-6) { std::cerr << "FAIL: chorus did not change output\n"; ok = false; }
    if (std::abs (delayRms - base) < 1e-6) { std::cerr << "FAIL: delay did not change output\n"; ok = false; }
    if (std::abs (reverbRms - base) < 1e-6) { std::cerr << "FAIL: reverb did not change output\n"; ok = false; }
    if (std::abs (driveRms - base) < 1e-6) { std::cerr << "FAIL: filter_drive did not change output\n"; ok = false; }

    // Filter env amount + keytrack
    engine.reset();
    setParameter (state, "filter_env_amount", 1.0f);
    setParameter (state, "filter_keytrack", 1.0f);
    setParameter (state, "filter_attack", 0.001f);
    const auto envRms = renderRms (engine, state, 120, true);
    std::cout << "envAmount=" << envRms << '\n';
    if (std::abs (envRms - base) < 1e-6) { std::cerr << "FAIL: filter env/keytrack did not change output\n"; ok = false; }

    // LFO sync with BPM
    setParameter (state, "mod1_destination", 0.0f);
    setParameter (state, "mod1_depth", 1.0f);
    engine.reset();
    engine.setBpm (60.0f);
    setParameter (state, "lfo1_sync", 1.0f);
    setParameter (state, "lfo1_depth", 1.0f);
    const auto sync60 = renderRms (engine, state, 120, true);
    engine.reset();
    engine.setBpm (180.0f);
    const auto sync180 = renderRms (engine, state, 120, true);
    std::cout << "sync60=" << sync60 << " sync180=" << sync180 << '\n';
    if (std::abs (sync180 - sync60) < 1e-5) { std::cerr << "FAIL: LFO sync did not respond to BPM\n"; ok = false; }

    // GLIDE: two consecutive notes with glide>0 should still produce audio
    setParameter (state, "lfo1_depth", 0.0f);
    setParameter (state, "lfo1_sync", 0.0f);
    setParameter (state, "filter_env_amount", 0.0f);
    setParameter (state, "filter_keytrack", 0.0f);
    setParameter (state, "glide", 0.5f);
    engine.reset();
    {
        juce::AudioBuffer<float> output (2, 512);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        for (int b = 0; b < 60; ++b) { output.clear(); engine.process (output, midi); midi.clear(); }
        midi.addEvent (juce::MidiMessage::noteOn (1, 72, 1.0f), 0);
        double e = 0; int c = 0;
        for (int b = 0; b < 60; ++b) { output.clear(); engine.process (output, midi); midi.clear();
            for (int ch = 0; ch < 2; ++ch) for (int s = 0; s < 512; ++s) { const auto v = output.getSample (ch, s); if (!std::isfinite(v)){std::cerr<<"FAIL: non-finite glide\n";return 9;} e += (double)v*v; ++c; }
        }
        const auto glideRms = std::sqrt (e / c);
        std::cout << "glideRms=" << glideRms << '\n';
        if (glideRms < 1e-4) { std::cerr << "FAIL: glide produced silence\n"; ok = false; }
    }

    // LEGATO
    setParameter (state, "glide", 0.0f);
    setParameter (state, "legato", 1.0f);
    engine.reset();
    {
        juce::AudioBuffer<float> output (2, 512);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        for (int b = 0; b < 40; ++b) { output.clear(); engine.process (output, midi); midi.clear(); }
        midi.addEvent (juce::MidiMessage::noteOn (1, 72, 1.0f), 0);
        double e = 0; int c = 0;
        for (int b = 0; b < 40; ++b) { output.clear(); engine.process (output, midi); midi.clear();
            for (int ch = 0; ch < 2; ++ch) for (int s = 0; s < 512; ++s) { const auto v = output.getSample (ch, s); if (!std::isfinite(v)){std::cerr<<"FAIL: non-finite legato\n";return 10;} e += (double)v*v; ++c; }
        }
        const auto legatoRms = std::sqrt (e / c);
        std::cout << "legatoRms=" << legatoRms << '\n';
        if (legatoRms < 1e-4) { std::cerr << "FAIL: legato produced silence\n"; ok = false; }
    }

    wave.deleteFile();
    if (ok) { std::cout << "PASS: feature smoke\n"; return 0; }
    return 11;
}

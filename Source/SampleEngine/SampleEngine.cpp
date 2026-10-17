#include "SampleEngine.h"

namespace chrome
{
SampleEngine::SampleEngine (juce::AudioProcessorValueTreeState& parameters)
    : state (parameters), loader (formats, [this] (int slot, std::shared_ptr<const SampleData> loaded, juce::String error)
      {
          if (slot < 0 || slot >= 3) return;
          samples[static_cast<size_t> (slot)].store (std::move (loaded), std::memory_order_release);
          {
              const juce::ScopedLock lock (metadataLock);
              errors[static_cast<size_t> (slot)] = std::move (error);
              if (const auto current = samples[static_cast<size_t> (slot)].load (std::memory_order_acquire))
                  paths[static_cast<size_t> (slot)] = current->file().getFullPathName();
          }
          triggerAsyncUpdate();
      })
{
    formats.registerBasicFormats();
    for (int slot = 0; slot < 3; ++slot)
    {
        auto& p = slotParameters[static_cast<size_t> (slot)];
        p.enabled = state.getRawParameterValue (params::slotId (slot, "enabled")); p.gain = state.getRawParameterValue (params::slotId (slot, "gain"));
        p.tune = state.getRawParameterValue (params::slotId (slot, "tune")); p.fine = state.getRawParameterValue (params::slotId (slot, "fine"));
        p.start = state.getRawParameterValue (params::slotId (slot, "start")); p.end = state.getRawParameterValue (params::slotId (slot, "end"));
        p.pan = state.getRawParameterValue (params::slotId (slot, "pan")); p.loop = state.getRawParameterValue (params::slotId (slot, "loop"));
        p.reverse = state.getRawParameterValue (params::slotId (slot, "reverse")); p.solo = state.getRawParameterValue (params::slotId (slot, "solo"));
        p.mute = state.getRawParameterValue (params::slotId (slot, "mute")); p.mode = state.getRawParameterValue (params::slotId (slot, "mode"));
    }
}

SampleEngine::~SampleEngine() { cancelPendingUpdate(); }

void SampleEngine::prepare (double sampleRate, int maxBlockSize)
{
    rate = sampleRate;
    for (auto& buffer : slotBuffers) buffer.setSize (2, maxBlockSize, false, true, true);
    for (auto& buffer : modeDryBuffers) buffer.setSize (2, maxBlockSize, false, true, true);
    for (auto& voice : voices) voice.prepare (sampleRate);
    for (int slot = 0; slot < 3; ++slot)
    {
        vintage[static_cast<size_t> (slot)].prepare (sampleRate, maxBlockSize);
        chrome[static_cast<size_t> (slot)].prepare (sampleRate, maxBlockSize);
        digital[static_cast<size_t> (slot)].prepare (sampleRate, maxBlockSize);
    }
    filterEnvelope.setSampleRate (sampleRate);
    modEnvelope.setSampleRate (sampleRate);
    juce::dsp::ProcessSpec mono { sampleRate, static_cast<juce::uint32> (maxBlockSize), 1 };
    for (auto& stage : filterStages) for (auto& channel : stage) channel.prepare (mono);
    master.prepare (sampleRate, maxBlockSize);
    reset();
}

void SampleEngine::reset()
{
    for (auto& voice : voices) voice.kill();
    for (auto& p : vintage) p.reset();
    for (auto& p : chrome) p.reset();
    for (auto& p : digital) p.reset();
    for (auto& stage : filterStages) for (auto& channel : stage) channel.reset();
    master.reset();
    filterEnvelope.reset();
    modEnvelope.reset();
    lastFilterNote = 60;
    previousNote = -1;
    sampleCounter = 0;
    lfoPhase.fill (0.0);
    lfoNoteOnTime.fill (0);
    aftertouch = 0.0f;
    lastVelocity = 0.0f;
    modulationRandom = 0xa19f2000u;
    randomModulation = 0.0f;
    lastModes.fill (-1);
    modeFadeSamples.fill (0);
    for (auto& buffer : modeDryBuffers) buffer.clear();
    {
        const juce::ScopedLock lock (metadataLock);
        paths.fill ({});
        errors.fill ({});
    }
}

void SampleEngine::loadSample (int slot, const juce::File& file)
{
    if (slot >= 0 && slot < 3 && file.existsAsFile())
        loader.load (slot, file);
}

void SampleEngine::restoreSamplePath (int slot, const juce::String& path)
{
    if (path.isNotEmpty()) loadSample (slot, juce::File (path));
}

std::shared_ptr<const SampleData> SampleEngine::sample (int slot) const
{
    if (slot < 0 || slot >= 3) return {};
    return samples[static_cast<size_t> (slot)].load (std::memory_order_acquire);
}

juce::String SampleEngine::samplePath (int slot) const
{
    const juce::ScopedLock lock (metadataLock);
    return slot >= 0 && slot < 3 ? paths[static_cast<size_t> (slot)] : juce::String();
}

void SampleEngine::handleAsyncUpdate()
{
    if (! sampleChanged) return;
    const juce::ScopedLock lock (metadataLock);
    for (int slot = 0; slot < 3; ++slot)
        sampleChanged (slot, errors[static_cast<size_t> (slot)]);
}

std::array<SlotPlayback, 3> SampleEngine::makeSlotSettings (const ModulationValues& modulation) const
{
    std::array<SlotPlayback, 3> result;
    bool anySolo = false;
    const auto read = [] (const std::atomic<float>* value) { return value->load (std::memory_order_relaxed); };
    for (const auto& p : slotParameters) anySolo |= read (p.solo) > 0.5f;
    for (int slot = 0; slot < 3; ++slot)
    {
        auto& s = result[static_cast<size_t> (slot)];
        const auto& p = slotParameters[static_cast<size_t> (slot)];
        s.sample = sample (slot);
        s.gain = juce::Decibels::decibelsToGain (read (p.gain)); s.pan = read (p.pan);
        s.start = juce::jlimit (0.0f, 0.99f, read (p.start) + modulation.position * 0.2f);
        s.end = juce::jlimit (0.01f, 1.0f, read (p.end) + modulation.position * 0.2f);
        s.tune = read (p.tune) + read (p.fine) * 0.01f
               + params::load (state, "global_pitch") + params::load (state, "global_fine") * 0.01f
               + modulation.pitch * 12.0f;
        s.loop = read (p.loop) > 0.5f; s.reverse = read (p.reverse) > 0.5f;
        const auto solo = read (p.solo) > 0.5f;
        s.audible = read (p.enabled) > 0.5f && read (p.mute) < 0.5f && (! anySolo || solo);
    }
    return result;
}

SampleEngine::ModulationValues SampleEngine::evaluateModulation (int numSamples)
{
    std::array<float, 15> sources {};
    for (int lfo = 0; lfo < 3; ++lfo)
    {
        const auto prefix = "lfo" + juce::String (lfo + 1) + "_";
        const auto shape = static_cast<int> (params::load (state, prefix + "shape"));
        float rateParam = params::load (state, prefix + "rate");
        const bool sync = params::load (state, prefix + "sync") > 0.5f;
        if (sync) rateParam *= currentBpm.load (std::memory_order_relaxed) / 60.0f;
        const auto phaseOffset = params::load (state, prefix + "phase");
        float phase = static_cast<float> (lfoPhase[static_cast<size_t> (lfo)]) + phaseOffset;
        if (phase >= 1.0f) phase -= std::floor (phase);
        float value = 0.0f;
        if (shape == 0) value = std::sin (juce::MathConstants<float>::twoPi * phase);
        else if (shape == 1) value = 1.0f - 4.0f * std::abs (phase - 0.5f);
        else if (shape == 2) value = phase * 2.0f - 1.0f;
        else if (shape == 3) value = phase < 0.5f ? 1.0f : -1.0f;
        else value = randomModulation;
        const auto fadeTime = params::load (state, prefix + "fade");
        float fadeMul = 1.0f;
        if (fadeTime > 0.001f)
        {
            const auto elapsed = static_cast<double> (sampleCounter - lfoNoteOnTime[static_cast<size_t> (lfo)]) / rate;
            fadeMul = juce::jlimit (0.0f, 1.0f, static_cast<float> (elapsed / fadeTime));
        }
        sources[static_cast<size_t> (lfo + 1)] = value * params::load (state, prefix + "depth") * fadeMul;
        lfoPhase[static_cast<size_t> (lfo)] += static_cast<double> (numSamples) * rateParam / rate;
        if (lfoPhase[static_cast<size_t> (lfo)] >= 1.0)
        {
            lfoPhase[static_cast<size_t> (lfo)] -= std::floor (lfoPhase[static_cast<size_t> (lfo)]);
            modulationRandom ^= modulationRandom << 13; modulationRandom ^= modulationRandom >> 17; modulationRandom ^= modulationRandom << 5;
            randomModulation = static_cast<float> (modulationRandom & 0xffffu) / 32767.5f - 1.0f;
        }
    }
    sources[4] = lastVelocity; sources[5] = aftertouch; sources[6] = randomModulation;
    float ampEnv = 0.0f;
    for (auto& v : voices) if (v.active()) { ampEnv = v.envelopeLevel(); break; }
    sources[7] = ampEnv;
    float modEnvVal = 0.0f;
    for (int i = 0; i < numSamples; ++i) modEnvVal = modEnvelope.getNextSample();
    sources[8] = modEnvVal;
    sources[9] = params::load (state, "liquid") * 2.0f - 1.0f; sources[10] = params::load (state, "reflect") * 2.0f - 1.0f;
    sources[11] = params::load (state, "fragment") * 2.0f - 1.0f; sources[12] = params::load (state, "shine") * 2.0f - 1.0f;
    sources[13] = params::load (state, "density") * 2.0f - 1.0f; sources[14] = params::load (state, "fusion") * 2.0f - 1.0f;

    ModulationValues values;
    std::array<float*, 10> destinations { &values.pitch, &values.filter, &values.position, &values.grain, &values.density,
                                          &values.mix, &values.fx, &values.liquid, &values.reflect, &values.shine };
    for (int route = 1; route <= 8; ++route)
    {
        const auto prefix = "mod" + juce::String (route) + "_";
        const auto source = juce::jlimit (0, 14, static_cast<int> (params::load (state, prefix + "source")));
        const auto destination = juce::jlimit (0, 9, static_cast<int> (params::load (state, prefix + "destination")));
        *destinations[static_cast<size_t> (destination)] += sources[static_cast<size_t> (source)] * params::load (state, prefix + "depth");
    }
    for (auto* destination : destinations) *destination = juce::jlimit (-1.0f, 1.0f, *destination);
    return values;
}

void SampleEngine::noteOn (int note, float velocity, const std::array<SlotPlayback, 3>& slots)
{
    lastVelocity = velocity;
    const auto maxVoices = juce::jlimit (16, 128, static_cast<int> (params::load (state, "polyphony")));
    const auto glideTime = params::load (state, "glide");
    const auto legatoEnabled = params::load (state, "legato") > 0.5f;

    Voice* target = nullptr;
    if (legatoEnabled)
    {
        for (int i = 0; i < maxVoices; ++i)
            if (voices[static_cast<size_t> (i)].active()) { target = &voices[static_cast<size_t> (i)]; break; }
    }
    if (target == nullptr)
    {
        for (int i = 0; i < maxVoices; ++i) if (! voices[static_cast<size_t> (i)].active()) { target = &voices[static_cast<size_t> (i)]; break; }
    }
    if (target == nullptr)
        target = &*std::min_element (voices.begin(), voices.begin() + maxVoices, [] (const Voice& a, const Voice& b) { return a.startedAt() < b.startedAt(); });

    const bool isLegato = legatoEnabled && target->active();
    const auto glideFromNote = previousNote >= 0 ? static_cast<float> (previousNote) : static_cast<float> (note);
    target->updateEnvelope (params::load (state, "amp_attack"), params::load (state, "amp_decay"), params::load (state, "amp_sustain"), params::load (state, "amp_release"));
    target->start (note, velocity, ++voiceCounter, slots, glideFromNote, glideTime, isLegato);
    filterEnvelope.noteOn();
    modEnvelope.noteOn();
    lastFilterNote = note;
    previousNote = note;
    lfoNoteOnTime[0] = sampleCounter;
    lfoNoteOnTime[1] = sampleCounter;
    lfoNoteOnTime[2] = sampleCounter;
}

void SampleEngine::noteOff (int note)
{
    for (auto& voice : voices) if (voice.active() && voice.note() == note) voice.release();
    bool anyActive = false;
    for (const auto& voice : voices) if (voice.active()) { anyActive = true; break; }
    if (! anyActive)
    {
        filterEnvelope.noteOff();
        modEnvelope.noteOff();
    }
}

void SampleEngine::processFilter (juce::AudioBuffer<float>& buffer, float modulation, float filterEnvValue, int keytrackNote, float envAmount, float keytrackAmount)
{
    const auto baseCutoff = params::load (state, "filter_cutoff");
    float cutoff = baseCutoff * std::exp2 (static_cast<float> (keytrackNote - 60) / 12.0f * keytrackAmount);
    cutoff = juce::jlimit (20.0f, static_cast<float> (rate * 0.45), cutoff * std::pow (2.0f, filterEnvValue * envAmount * 4.0f));
    const auto modulated = juce::jlimit (20.0f, static_cast<float> (rate * 0.45), cutoff * std::exp2 (modulation * 5.0f));
    const auto q = params::load (state, "filter_resonance");
    const auto drive = juce::Decibels::decibelsToGain (params::load (state, "filter_drive"));
    const auto type = static_cast<int> (params::load (state, "filter_type"));
    buffer.applyGain (drive);
    auto coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (rate, modulated, q);
    if (type == 1) coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (rate, modulated, q);
    else if (type == 2) coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (rate, modulated, q);
    else if (type == 3) coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeNotch (rate, modulated, q);
    const auto stages = type == 0 || type == 1 ? 2 : 1;
    for (int stage = 0; stage < stages; ++stage)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            *filterStages[static_cast<size_t> (stage)][static_cast<size_t> (ch)].coefficients = coefficients;
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample (ch, i, filterStages[static_cast<size_t> (stage)][static_cast<size_t> (ch)].processSample (buffer.getSample (ch, i)));
        }
}

void SampleEngine::process (juce::AudioBuffer<float>& output, juce::MidiBuffer& midi)
{
    output.clear();
    const auto frames = output.getNumSamples();
    sampleCounter += static_cast<uint64_t> (frames);
    for (auto& buffer : slotBuffers) buffer.clear (0, frames);
    // update envelope params
    juce::ADSR::Parameters filterParams { params::load (state, "filter_attack"), params::load (state, "filter_decay"), params::load (state, "filter_sustain"), params::load (state, "filter_release") };
    filterEnvelope.setParameters (filterParams);
    modEnvelope.setParameters ({ params::load (state, "mod_attack"), params::load (state, "mod_decay"), params::load (state, "mod_sustain"), params::load (state, "mod_release") });
    const auto modulation = evaluateModulation (frames);
    const auto slots = makeSlotSettings (modulation);
    std::array<juce::AudioBuffer<float>*, 3> destinations { &slotBuffers[0], &slotBuffers[1], &slotBuffers[2] };
    int cursor = 0;
    for (const auto metadata : midi)
    {
        const auto eventPosition = juce::jlimit (cursor, frames, metadata.samplePosition);
        const auto segment = eventPosition - cursor;
        if (segment > 0) for (auto& voice : voices) voice.render (destinations, cursor, segment, slots);
        const auto message = metadata.getMessage();
        if (message.isNoteOn()) noteOn (message.getNoteNumber(), message.getFloatVelocity(), slots);
        else if (message.isNoteOff()) noteOff (message.getNoteNumber());
        else if (message.isChannelPressure()) aftertouch = static_cast<float> (message.getChannelPressureValue()) / 127.0f;
        else if (message.isAftertouch()) aftertouch = static_cast<float> (message.getAfterTouchValue()) / 127.0f;
        else if (message.isAllNotesOff() || message.isAllSoundOff()) { for (auto& voice : voices) voice.kill(); filterEnvelope.noteOff(); modEnvelope.noteOff(); }
        cursor = eventPosition;
    }
    if (cursor < frames) for (auto& voice : voices) voice.render (destinations, cursor, frames - cursor, slots);

    const ChromeControls controls { juce::jlimit (0.0f, 1.0f, params::load (state, "liquid") + modulation.liquid * 0.5f),
                                    juce::jlimit (0.0f, 1.0f, params::load (state, "reflect") + modulation.reflect * 0.5f),
                                    juce::jlimit (0.0f, 1.0f, params::load (state, "fragment") + modulation.grain * 0.5f),
                                    juce::jlimit (0.0f, 1.0f, params::load (state, "shine") + modulation.shine * 0.5f),
                                    juce::jlimit (0.0f, 1.0f, params::load (state, "density") + modulation.density * 0.5f), params::load (state, "fusion") };
    const auto x = juce::jlimit (0.0f, 1.0f, params::load (state, "morph_x") + modulation.mix * 0.4f), y = params::load (state, "morph_y");
    std::array<float, 3> weights { juce::jmax (0.0f, 1.0f - x - y * 0.5f), x * (1.0f - y * 0.5f), y };
    const auto weightSum = weights[0] + weights[1] + weights[2] + 1.0e-6f;
    for (auto& weight : weights) weight /= weightSum;
    for (int slot = 0; slot < 3; ++slot)
    {
        auto& buffer = slotBuffers[static_cast<size_t> (slot)];
        const auto& slotSettings = slots[static_cast<size_t> (slot)];
        if (slotSettings.sample == nullptr || ! slotSettings.audible)
        {
            buffer.clear();
            vintage[static_cast<size_t> (slot)].reset();
            chrome[static_cast<size_t> (slot)].reset();
            digital[static_cast<size_t> (slot)].reset();
            lastModes[static_cast<size_t> (slot)] = -1;
            modeFadeSamples[static_cast<size_t> (slot)] = 0;
            continue;
        }
        const auto mode = juce::jlimit (0, 2, static_cast<int> (
            slotParameters[static_cast<size_t> (slot)].mode->load (std::memory_order_relaxed)));
        auto& dry = modeDryBuffers[static_cast<size_t> (slot)];
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            dry.copyFrom (ch, 0, buffer, ch, 0, frames);
        if (lastModes[static_cast<size_t> (slot)] != mode)
        {
            vintage[static_cast<size_t> (slot)].reset();
            chrome[static_cast<size_t> (slot)].reset();
            digital[static_cast<size_t> (slot)].reset();
            lastModes[static_cast<size_t> (slot)] = mode;
            modeFadeSamples[static_cast<size_t> (slot)] = 128;
        }
        if (mode == 0) vintage[static_cast<size_t> (slot)].process (buffer, 0.72f);
        else if (mode == 1) chrome[static_cast<size_t> (slot)].process (buffer, controls);
        else digital[static_cast<size_t> (slot)].process (buffer, controls.density, controls.fragment);
        auto& fadeRemaining = modeFadeSamples[static_cast<size_t> (slot)];
        if (fadeRemaining > 0)
        {
            const auto faded = juce::jmin (frames, fadeRemaining);
            const auto alreadyFaded = 128 - fadeRemaining;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int sampleIndex = 0; sampleIndex < faded; ++sampleIndex)
                {
                    const auto mix = static_cast<float> (alreadyFaded + sampleIndex + 1) / 128.0f;
                    buffer.setSample (ch, sampleIndex,
                                      dry.getSample (ch, sampleIndex) * (1.0f - mix)
                                      + buffer.getSample (ch, sampleIndex) * mix);
                }
            fadeRemaining -= faded;
        }
        for (int ch = 0; ch < output.getNumChannels(); ++ch) output.addFrom (ch, 0, buffer, ch, 0, frames, weights[static_cast<size_t> (slot)]);
    }

    float filterEnvVal = 0.0f;
    for (int i = 0; i < frames; ++i) filterEnvVal = filterEnvelope.getNextSample();
    processFilter (output, modulation.filter, filterEnvVal, lastFilterNote,
                   params::load (state, "filter_env_amount"), params::load (state, "filter_keytrack"));
    const auto fxMod = modulation.fx * 0.35f;
    const auto chorusOn = params::load (state, "chorus_enabled") > 0.5f;
    const auto delayOn = params::load (state, "delay_enabled") > 0.5f;
    const auto reverbOn = params::load (state, "reverb_enabled") > 0.5f;
    master.process (output, { juce::jlimit (0.0f, 1.0f, chorusOn ? (params::load (state, "chorus") + fxMod) : 0.0f),
                               juce::jlimit (0.0f, 1.0f, delayOn ? (params::load (state, "delay") + fxMod) : 0.0f),
                               params::load (state, "delay_time"), params::load (state, "delay_feedback"),
                               juce::jlimit (0.0f, 1.0f, reverbOn ? (params::load (state, "reverb") + fxMod) : 0.0f),
                              params::load (state, "reverb_size"), params::load (state, "reverb_damp"),
                              params::load (state, "compressor"), params::load (state, "width"), params::load (state, "eq_low"),
                              params::load (state, "eq_high"), params::load (state, "master_input_gain"), params::load (state, "master_gain"), params::load (state, "soft_clip") > 0.5f,
                              static_cast<int> (params::load (state, "oversampling")) });
    const auto currentPeak = juce::jmax (output.getMagnitude (0, frames), output.getNumChannels() > 1 ? output.getMagnitude (1, frames) : 0.0f);
    peak.store (juce::jmax (currentPeak, peak.load() * 0.92f), std::memory_order_relaxed);
    bool anyActive = false;
    for (const auto& v : voices) if (v.active()) { anyActive = true; break; }
    if (! anyActive) { filterEnvelope.noteOff(); modEnvelope.noteOff(); }
}
}

#include "PresetManager.h"
#include "../SampleEngine/SampleEngine.h"

namespace chrome
{
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& parameters, SampleEngine& sampleEngine)
    : state (parameters), engine (sampleEngine)
{
}

juce::File PresetManager::directory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("LEVSAN").getChildFile ("CHROME 2000").getChildFile ("Presets");
    // Migrate presets stored under the legacy "Chrome Audio" vendor folder.
    const auto legacy = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                            .getChildFile ("Chrome Audio").getChildFile ("CHROME 2000").getChildFile ("Presets");
    dir.createDirectory();
    if (legacy.isDirectory())
    {
        for (const auto& file : legacy.findChildFiles (juce::File::findFiles, false, "*.xml"))
        {
            const auto target = dir.getChildFile (file.getFileNameWithoutExtension() + ".c2preset");
            if (! target.existsAsFile())
                file.copyFileTo (target);
        }
    }
    // Normalize any legacy .xml presets already in the new folder.
    for (const auto& file : dir.findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        const auto target = dir.getChildFile (file.getFileNameWithoutExtension() + ".c2preset");
        if (! target.existsAsFile())
            file.moveFileTo (target);
        else
            file.deleteFile();
    }
    return dir;
}

juce::StringArray PresetManager::names() const
{
    auto result = factory;
    for (const auto& file : directory().findChildFiles (juce::File::findFiles, false, "*.c2preset"))
        result.addIfNotAlreadyThere (file.getFileNameWithoutExtension());
    return result;
}

void PresetManager::setActual (const juce::String& parameterId, float value)
{
    if (auto* parameter = state.getParameter (parameterId))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void PresetManager::applyFactory (const juce::String& name)
{
    setActual ("liquid", 0.45f); setActual ("reflect", 0.35f); setActual ("fragment", 0.25f);
    setActual ("shine", 0.55f); setActual ("density", 0.5f); setActual ("fusion", 0.4f);
    setActual ("chorus", 0.25f); setActual ("delay", 0.30f); setActual ("reverb", 0.35f);
    setActual ("filter_cutoff", 8720.0f); setActual ("filter_resonance", 0.25f); setActual ("filter_drive", 2.88f);
    setActual ("amp_attack", 0.01f); setActual ("amp_decay", 0.25f); setActual ("amp_sustain", 0.75f); setActual ("amp_release", 0.29f);
    setActual ("lfo1_depth", 0.25f); setActual ("mod1_source", 1.0f); setActual ("mod1_destination", 1.0f); setActual ("mod1_depth", 0.25f);
    setActual ("master_gain", -3.0f);
    if (name == "Liquid Cathedral") { setActual ("liquid", 0.92f); setActual ("reflect", 0.78f); setActual ("fusion", 0.76f); setActual ("reverb", 0.46f); setActual ("amp_attack", 0.8f); setActual ("amp_release", 3.8f); }
    else if (name == "Mercury Glass") { setActual ("liquid", 0.72f); setActual ("reflect", 0.88f); setActual ("shine", 0.91f); setActual ("fragment", 0.38f); setActual ("filter_cutoff", 11800.0f); }
    else if (name == "Tape Constellation") { for (int i = 0; i < 3; ++i) setActual (params::slotId (i, "mode"), 0.0f); setActual ("chorus", 0.34f); setActual ("delay", 0.28f); setActual ("eq_high", -2.4f); }
    else if (name == "Digital Pollen") { for (int i = 0; i < 3; ++i) setActual (params::slotId (i, "mode"), 2.0f); setActual ("fragment", 0.84f); setActual ("density", 0.76f); setActual ("filter_cutoff", 7200.0f); }
    else if (name == "Frozen Alloy") { setActual ("liquid", 0.98f); setActual ("fusion", 0.9f); setActual ("shine", 0.68f); setActual ("amp_attack", 1.5f); setActual ("amp_release", 6.0f); setActual ("width", 1.65f); }
}

bool PresetManager::load (const juce::String& name)
{
    if (factory.contains (name))
    {
        applyFactory (name); current = name; return true;
    }
    const auto file = directory().getChildFile (juce::File::createLegalFileName (name) + ".c2preset");
    if (auto xml = juce::XmlDocument::parse (file))
    {
        const auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid())
        {
            state.replaceState (tree); engine.restoreSampleState (tree); current = name; return true;
        }
    }
    return false;
}

bool PresetManager::save (const juce::String& name)
{
    if (name.trim().isEmpty()) return false;
    auto tree = state.copyState();
    engine.writeSampleState (tree);
    if (auto xml = tree.createXml())
    {
        const auto ok = xml->writeTo (directory().getChildFile (juce::File::createLegalFileName (name.trim()) + ".c2preset"));
        if (ok) current = name.trim();
        return ok;
    }
    return false;
}

bool PresetManager::remove (const juce::String& name)
{
    if (factory.contains (name)) return false;
    return directory().getChildFile (juce::File::createLegalFileName (name) + ".c2preset").deleteFile();
}

juce::String PresetManager::step (int direction)
{
    const auto list = names();
    if (list.isEmpty()) return {};
    auto index = list.indexOf (current);
    if (index < 0) index = 0;
    index = (index + direction + list.size()) % list.size();
    load (list[index]);
    return current;
}
}

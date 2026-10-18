#include "ParameterLayout.h"

namespace chrome::params
{
namespace
{
using APF = juce::AudioParameterFloat;
using API = juce::AudioParameterInt;
using APB = juce::AudioParameterBool;
using APC = juce::AudioParameterChoice;

auto id (const juce::String& value) { return juce::ParameterID { value, 1 }; }

void addSlot (juce::AudioProcessorValueTreeState::ParameterLayout& layout, int slot)
{
    const auto prefix = "s" + juce::String (slot + 1) + "_";
    const auto title = "Sample " + juce::String (slot + 1) + " ";
    constexpr std::array<float, 3> defaultGain { -3.2f, -1.9f, -3.0f };
    constexpr std::array<float, 3> defaultStart { 0.0f, 0.125f, 0.05f };
    constexpr std::array<float, 3> defaultEnd { 1.0f, 0.879f, 0.95f };
    constexpr std::array<int, 3> defaultMode { 2, 1, 0 };
    layout.add (std::make_unique<APB> (id (prefix + "enabled"), title + "On", true));
    layout.add (std::make_unique<APF> (id (prefix + "gain"), title + "Gain", juce::NormalisableRange<float> (-60.0f, 6.0f, 0.01f), defaultGain[static_cast<size_t> (slot)]));
    layout.add (std::make_unique<API> (id (prefix + "tune"), title + "Tune", -24, 24, 0));
    layout.add (std::make_unique<APF> (id (prefix + "fine"), title + "Fine", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<APF> (id (prefix + "start"), title + "Start", 0.0f, 1.0f, defaultStart[static_cast<size_t> (slot)]));
    layout.add (std::make_unique<APF> (id (prefix + "end"), title + "End", 0.0f, 1.0f, defaultEnd[static_cast<size_t> (slot)]));
    layout.add (std::make_unique<APF> (id (prefix + "pan"), title + "Pan", -1.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<APB> (id (prefix + "loop"), title + "Loop", true));
    layout.add (std::make_unique<APB> (id (prefix + "reverse"), title + "Reverse", false));
    layout.add (std::make_unique<APB> (id (prefix + "solo"), title + "Solo", false));
    layout.add (std::make_unique<APB> (id (prefix + "mute"), title + "Mute", false));
    layout.add (std::make_unique<APC> (id (prefix + "mode"), title + "Mode",
                                      juce::StringArray { "Ethernal", "Chrome", "Digital" },
                                      defaultMode[static_cast<size_t> (slot)]));
}

void addAdsr (juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& prefix, const juce::String& title,
              float attack, float decay, float sustain, float release)
{
    const juce::NormalisableRange<float> time (0.001f, 20.0f, 0.001f, 0.35f);
    layout.add (std::make_unique<APF> (id (prefix + "attack"), title + " Attack", time, attack));
    layout.add (std::make_unique<APF> (id (prefix + "decay"), title + " Decay", time, decay));
    layout.add (std::make_unique<APF> (id (prefix + "sustain"), title + " Sustain", 0.0f, 1.0f, sustain));
    layout.add (std::make_unique<APF> (id (prefix + "release"), title + " Release", time, release));
}
}

juce::String slotId (int slot, const juce::String& name)
{
    return "s" + juce::String (slot + 1) + "_" + name;
}

juce::StringArray delayDivisionNames()
{
    return { "1/1", "1/2.", "1/2", "1/4.", "1/4", "1/8.", "1/4T",
             "1/8", "1/16.", "1/8T", "1/16", "1/16T", "1/32" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int slot = 0; slot < 3; ++slot)
        addSlot (layout, slot);

    layout.add (std::make_unique<APB> (id ("power"), "Power", true));
    layout.add (std::make_unique<APF> (id ("global_pitch"), "Pitch", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<APF> (id ("global_fine"), "Fine Tune", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<APF> (id ("glide"), "Glide", juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.4f), 0.125f));
    layout.add (std::make_unique<APB> (id ("legato"), "Legato", false));

    layout.add (std::make_unique<APF> (id ("morph_x"), "Morph X", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<APF> (id ("morph_y"), "Morph Y", 0.0f, 1.0f, 0.35f));
    layout.add (std::make_unique<APF> (id ("liquid"), "Chrome Liquid", 0.0f, 1.0f, 0.45f));
    layout.add (std::make_unique<APF> (id ("reflect"), "Chrome Reflect", 0.0f, 1.0f, 0.35f));
    layout.add (std::make_unique<APF> (id ("fragment"), "Chrome Fragment", 0.0f, 1.0f, 0.25f));
    layout.add (std::make_unique<APF> (id ("shine"), "Chrome Shine", 0.0f, 1.0f, 0.55f));
    layout.add (std::make_unique<APF> (id ("density"), "Chrome Density", 0.0f, 1.0f, 0.5f));
    layout.add (std::make_unique<APF> (id ("fusion"), "Chrome Fusion", 0.0f, 1.0f, 0.4f));

    layout.add (std::make_unique<APC> (id ("filter_type"), "Filter Type", juce::StringArray { "Low Pass", "High Pass", "Band Pass", "Notch" }, 0));
    layout.add (std::make_unique<APF> (id ("filter_cutoff"), "Filter Cutoff", juce::NormalisableRange<float> (20.0f, 20000.0f, 0.01f, 0.22f), 8720.0f));
    layout.add (std::make_unique<APF> (id ("filter_resonance"), "Filter Resonance", 0.05f, 10.0f, 0.05f));
    layout.add (std::make_unique<APF> (id ("filter_drive"), "Filter Drive", 0.0f, 24.0f, 2.88f));
    layout.add (std::make_unique<APF> (id ("filter_env_amount"), "Filter Envelope Amount", -1.0f, 1.0f, 0.0f));
    layout.add (std::make_unique<APF> (id ("filter_keytrack"), "Filter Key Tracking", 0.0f, 1.0f, 0.0f));

    addAdsr (layout, "amp_", "Amp", 0.01f, 0.25f, 0.75f, 0.29f);
    addAdsr (layout, "filter_", "Filter", 0.01f, 0.3f, 0.5f, 0.4f);
    addAdsr (layout, "mod_", "Mod", 0.05f, 0.4f, 0.5f, 0.7f);

    for (int lfo = 1; lfo <= 3; ++lfo)
    {
        const auto p = "lfo" + juce::String (lfo) + "_";
        const auto t = "LFO " + juce::String (lfo) + " ";
        layout.add (std::make_unique<APC> (id (p + "shape"), t + "Shape", juce::StringArray { "Sine", "Triangle", "Saw", "Square", "Random", "Sample Hold" }, 0));
        layout.add (std::make_unique<APF> (id (p + "rate"), t + "Rate", juce::NormalisableRange<float> (0.01f, 40.0f, 0.001f, 0.3f), 1.0f));
        layout.add (std::make_unique<APF> (id (p + "depth"), t + "Depth", 0.0f, 1.0f, lfo == 1 ? 0.25f : 0.0f));
        layout.add (std::make_unique<APF> (id (p + "phase"), t + "Phase", 0.0f, 1.0f, 0.0f));
        layout.add (std::make_unique<APF> (id (p + "fade"), t + "Fade In", 0.0f, 10.0f, 0.0f));
        layout.add (std::make_unique<APB> (id (p + "sync"), t + "Tempo Sync", false));
    }

    const juce::StringArray sources { "Off", "LFO 1", "LFO 2", "LFO 3", "Velocity", "Aftertouch", "Random", "Amp Envelope", "Mod Envelope", "Liquid", "Reflect", "Fragment", "Shine", "Density", "Fusion" };
    const juce::StringArray destinations { "Pitch", "Filter", "Sample Position", "Grain Size", "Density", "Mix", "FX", "Chrome Liquid", "Chrome Reflect", "Chrome Shine" };
    for (int route = 1; route <= 8; ++route)
    {
        const auto p = "mod" + juce::String (route) + "_";
        layout.add (std::make_unique<APC> (id (p + "source"), "Mod " + juce::String (route) + " Source", sources, route == 1 ? 1 : 0));
        layout.add (std::make_unique<APC> (id (p + "destination"), "Mod " + juce::String (route) + " Destination", destinations, 1));
        layout.add (std::make_unique<APF> (id (p + "depth"), "Mod " + juce::String (route) + " Depth", -1.0f, 1.0f, route == 1 ? 0.25f : 0.0f));
    }

    layout.add (std::make_unique<APF> (id ("chorus"), "Chorus", 0.0f, 1.0f, 0.25f));
    layout.add (std::make_unique<APF> (id ("delay"), "Delay", 0.0f, 1.0f, 0.30f));
    layout.add (std::make_unique<APF> (id ("delay_time"), "Delay Time", juce::NormalisableRange<float> (0.03f, 1.5f, 0.001f, 0.45f), 0.375f));
    layout.add (std::make_unique<APB> (id ("delay_sync"), "Delay Sync", false));
    layout.add (std::make_unique<APC> (id ("delay_division"), "Delay Division", delayDivisionNames(), 4));
    layout.add (std::make_unique<APF> (id ("delay_feedback"), "Delay Feedback", 0.0f, 0.95f, 0.35f));
    layout.add (std::make_unique<APF> (id ("reverb"), "Reverb", 0.0f, 1.0f, 0.35f));
    layout.add (std::make_unique<APF> (id ("reverb_size"), "Reverb Size", 0.0f, 1.0f, 0.60f));
    layout.add (std::make_unique<APF> (id ("reverb_damp"), "Reverb Damping", 0.0f, 1.0f, 0.40f));
    layout.add (std::make_unique<APB> (id ("chorus_enabled"), "Chorus On", true));
    layout.add (std::make_unique<APB> (id ("delay_enabled"), "Delay On", true));
    layout.add (std::make_unique<APB> (id ("reverb_enabled"), "Reverb On", true));
    layout.add (std::make_unique<APF> (id ("compressor"), "Compressor", 0.0f, 1.0f, 0.15f));
    layout.add (std::make_unique<APF> (id ("width"), "Stereo Width", 0.0f, 2.0f, 1.0f));
    layout.add (std::make_unique<APF> (id ("eq_low"), "EQ Low", -12.0f, 12.0f, 0.0f));
    layout.add (std::make_unique<APF> (id ("eq_high"), "EQ High", -12.0f, 12.0f, 0.0f));
    layout.add (std::make_unique<APF> (id ("master_input_gain"), "Master Input Gain", -24.0f, 24.0f, 0.0f));
    layout.add (std::make_unique<APF> (id ("master_gain"), "Master Gain", -60.0f, 6.0f, -3.0f));
    layout.add (std::make_unique<APB> (id ("soft_clip"), "Soft Clip", true));
    layout.add (std::make_unique<APC> (id ("oversampling"), "Oversampling", juce::StringArray { "Off", "2x", "4x" }, 1));
    layout.add (std::make_unique<API> (id ("polyphony"), "Polyphony", 16, 128, 32));
    return layout;
}
}

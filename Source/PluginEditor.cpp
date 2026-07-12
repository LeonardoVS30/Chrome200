#include "PluginEditor.h"

namespace
{
struct EditorLayout
{
    juce::Rectangle<int> header, samples, filter, envelope, chrome, modulation, effects, master, footer;
};

EditorLayout makeEditorLayout (juce::Rectangle<int> bounds)
{
    auto inner = bounds.reduced (45, 22);
    EditorLayout layout;
    layout.header = inner.removeFromTop (70);
    layout.footer = inner.removeFromBottom (52);
    inner.reduce (0, 7);
    const auto gap = 8;
    auto top = inner.removeFromTop (static_cast<int> (inner.getHeight() * 0.40f));
    inner.removeFromTop (gap);
    auto middle = inner.removeFromTop (static_cast<int> (inner.getHeight() * 0.54f));
    inner.removeFromTop (gap);
    auto bottom = inner;
    layout.samples = top.removeFromLeft (static_cast<int> (top.getWidth() * 0.81f)); top.removeFromLeft (gap); layout.filter = top;
    layout.envelope = middle.removeFromLeft (static_cast<int> (middle.getWidth() * 0.37f)); middle.removeFromLeft (gap);
    layout.chrome = middle.removeFromLeft (static_cast<int> (middle.getWidth() * 0.36f)); middle.removeFromLeft (gap); layout.modulation = middle;
    layout.effects = bottom.removeFromLeft (static_cast<int> (bottom.getWidth() * 0.76f)); bottom.removeFromLeft (gap); layout.master = bottom;
    return layout;
}
}

Chrome2000AudioProcessorEditor::BoundKnob::BoundKnob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& name)
    : attachment (state, id, slider)
{
    label.setText (name, juce::dontSendNotification); label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 16);
    addAndMakeVisible (label); addAndMakeVisible (slider);
}

void Chrome2000AudioProcessorEditor::BoundKnob::resized()
{
    auto b = getLocalBounds(); label.setBounds (b.removeFromTop (16)); slider.setBounds (b);
}

void Chrome2000AudioProcessorEditor::LfoDisplay::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff56595d), b.getX(), b.getY(), juce::Colour (0xff303236), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 10.0f); g.setColour (juce::Colour (0xff9da0a4)); g.drawRoundedRectangle (b, 10.0f, 1.0f);
    juce::Path wave;
    for (int x = 0; x <= getWidth(); ++x)
    {
        const auto phase = static_cast<float> (x) / static_cast<float> (juce::jmax (1, getWidth())) * juce::MathConstants<float>::twoPi * 2.5f;
        const auto y = b.getCentreY() - std::sin (phase) * b.getHeight() * 0.32f;
        if (x == 0) wave.startNewSubPath (static_cast<float> (x), y); else wave.lineTo (static_cast<float> (x), y);
    }
    g.setColour (juce::Colour (0xff9fc4ff)); g.strokePath (wave, juce::PathStrokeType (1.6f));
}

Chrome2000AudioProcessorEditor::Chrome2000AudioProcessorEditor (Chrome2000AudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), powerAttachment (p.parameters, "power", power),
      filterTypeAttachment (p.parameters, "filter_type", filterType),
      modulationSourceAttachment (p.parameters, "mod1_source", modulationSource),
      modulationDestinationAttachment (p.parameters, "mod1_destination", modulationDestination),
      lfoShapeAttachment (p.parameters, "lfo1_shape", lfoShape), morphXAttachment (p.parameters, "morph_x", morphX)
{
    setLookAndFeel (&lookAndFeel);
    logo.setText ("CHROME", juce::dontSendNotification); logo.setFont (juce::Font (juce::FontOptions (32.0f, juce::Font::bold | juce::Font::italic).withHorizontalScale (1.32f))); logo.setColour (juce::Label::textColourId, juce::Colour (0xff25292d));
    subtitle.setText ("S A M P L E   S Y N T H", juce::dontSendNotification); subtitle.setFont (juce::FontOptions (10.0f)); subtitle.setColour (juce::Label::textColourId, juce::Colour (0xff31363b));
    credit.setText ("DIETER RAMS\nINSPIRED DESIGN\nY2K EDITION", juce::dontSendNotification); credit.setJustificationType (juce::Justification::centredRight); credit.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    outputLabel.setJustificationType (juce::Justification::centredRight); outputLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    for (auto* c : std::array<juce::Component*, 19> { &logo, &subtitle, &logoHitArea, &presetBox, &previous, &next, &save, &power, &outputLabel, &credit, &filterType, &modulationSource, &modulationDestination, &lfoShape, &lfoDisplay, &morphX, &waveTab, &spectrumTab, &modTab }) addAndMakeVisible (c);
    logoHitArea.getProperties().set ("logoHitArea", true); logoHitArea.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    logoHitArea.onClick = [this] { showAbout(); };
    power.setClickingTogglesState (true);
    power.getProperties().set ("chromePower", true); power.setButtonText ({});
    filterType.addItemList ({ "LP 24dB", "HP 24dB", "BP 12dB", "NOTCH" }, 1);
    modulationSource.addItemList ({ "OFF", "LFO 1", "LFO 2", "LFO 3", "VELOCITY", "AFTERTOUCH", "RANDOM", "AMP ENV", "MOD ENV", "LIQUID", "REFLECT", "FRAGMENT", "SHINE", "DENSITY", "FUSION" }, 1);
    modulationDestination.addItemList ({ "PITCH", "CUTOFF", "POSITION", "GRAIN SIZE", "DENSITY", "MIX", "FX", "LIQUID", "REFLECT", "SHINE" }, 1);
    lfoShape.addItemList ({ "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM", "S&H" }, 1);
    morphX.setSliderStyle (juce::Slider::LinearHorizontal); morphX.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphX.setColour (juce::Slider::trackColourId, juce::Colour (0xff8fb8fb)); morphX.setColour (juce::Slider::thumbColourId, juce::Colour (0xfff7f7f8));
    for (auto* tab : { &waveTab, &spectrumTab, &modTab }) { tab->setClickingTogglesState (true); tab->setRadioGroupId (2000); }
    waveTab.setToggleState (true, juce::dontSendNotification);
    waveTab.onClick = [this] { for (auto& slot : slots) slot->setViewMode (chrome::WaveformComponent::ViewMode::wave); };
    spectrumTab.onClick = [this] { for (auto& slot : slots) slot->setViewMode (chrome::WaveformComponent::ViewMode::spectrum); };
    modTab.onClick = [this] { for (auto& slot : slots) slot->setViewMode (chrome::WaveformComponent::ViewMode::modulation); };
    for (int i = 0; i < 3; ++i) { slots[static_cast<size_t> (i)] = std::make_unique<chrome::SampleSlotComponent> (p.parameters, p.engine, i); addAndMakeVisible (*slots[static_cast<size_t> (i)]); }
    const std::array<std::pair<const char*, const char*>, 6> chromeControls {{{ "liquid", "LIQUID" }, { "reflect", "REFLECT" }, { "fragment", "FRAGMENT" }, { "shine", "SHINE" }, { "density", "DENSITY" }, { "fusion", "FUSION" }}};
    const std::array<std::pair<const char*, const char*>, 4> filterControls {{{ "filter_cutoff", "CUTOFF" }, { "filter_resonance", "RESONANCE" }, { "filter_drive", "DRIVE" }, { "filter_env_amount", "ENV AMT" }}};
    const std::array<std::pair<const char*, const char*>, 4> envControls {{{ "amp_attack", "ATTACK" }, { "amp_decay", "DECAY" }, { "amp_sustain", "SUSTAIN" }, { "amp_release", "RELEASE" }}};
    const std::array<std::pair<const char*, const char*>, 6> effects {{{ "chorus", "CHORUS" }, { "delay", "DELAY" }, { "reverb", "REVERB" }, { "compressor", "COMP" }, { "width", "WIDTH" }, { "master_gain", "OUTPUT" }}};
    const std::array<std::pair<const char*, const char*>, 3> modulationControls {{{ "lfo1_rate", "RATE" }, { "lfo1_depth", "AMOUNT" }, { "mod1_depth", "DEPTH" }}};
    auto addKnobs = [this, &p] (auto& destination, const auto& definitions) { for (const auto& [id, name] : definitions) { destination.push_back (std::make_unique<BoundKnob> (p.parameters, id, name)); addAndMakeVisible (*destination.back()); } };
    addKnobs (chromeKnobs, chromeControls); addKnobs (filterKnobs, filterControls); addKnobs (envelopeKnobs, envControls); addKnobs (effectKnobs, effects); addKnobs (modulationKnobs, modulationControls);
    rebuildPresetList();
    presetBox.onChange = [this] { if (presetBox.getText().isNotEmpty()) processor.presets.load (presetBox.getText()); };
    previous.onClick = [this] { presetBox.setText (processor.presets.step (-1), juce::dontSendNotification); };
    next.onClick = [this] { presetBox.setText (processor.presets.step (1), juce::dontSendNotification); };
    save.onClick = [this] { savePreset(); };
    processor.engine.sampleChanged = [safe = juce::Component::SafePointer<Chrome2000AudioProcessorEditor> (this)] (int slot, const juce::String& error)
    {
        if (safe == nullptr) return;
        safe->slots[static_cast<size_t> (slot)]->refresh();
        if (error.isNotEmpty()) juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Sample loading", error);
    };
    for (auto& slot : slots) slot->refresh();
    // Resizability setup and setSize both invoke resized() synchronously. All child
    // components must exist before either call.
    setResizable (true, true);
    setResizeLimits (980, 692, 1900, 1342);
    getConstrainer()->setFixedAspectRatio (1493.0 / 1054.0);
    setSize (1240, 875);
    startTimerHz (30);
}

Chrome2000AudioProcessorEditor::~Chrome2000AudioProcessorEditor()
{
    processor.engine.sampleChanged = {}; setLookAndFeel (nullptr);
}

void Chrome2000AudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff191b1e)); g.fillAll();
    const auto outerRim = b.reduced (3.0f); g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffafbfc), outerRim.getX(), outerRim.getY(), juce::Colour (0xff55585c), outerRim.getRight(), outerRim.getBottom(), false)); g.fillRoundedRectangle (outerRim, 31.0f);
    const auto darkChannel = b.reduced (8.0f); g.setColour (juce::Colour (0xff242629)); g.fillRoundedRectangle (darkChannel, 27.0f);
    const auto innerRim = b.reduced (12.0f); g.setGradientFill (juce::ColourGradient (juce::Colour (0xff686c70), innerRim.getX(), innerRim.getBottom(), juce::Colour (0xfff7f8f9), innerRim.getRight(), innerRim.getY(), false)); g.fillRoundedRectangle (innerRim, 25.0f);
    g.setColour (juce::Colour (0xffffffff)); g.drawRoundedRectangle (innerRim.reduced (2.0f), 23.0f, 1.2f);
    const auto face = b.reduced (19.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeeeeef), face.getX(), face.getY(), juce::Colour (0xffd5d5d7), face.getRight(), face.getBottom(), false)); g.fillRoundedRectangle (face, 22.0f);
    g.setColour (juce::Colour (0x12ffffff)); for (int y = 24; y < getHeight() - 24; y += 3) g.drawHorizontalLine (y, 24.0f, static_cast<float> (getWidth() - 24));
    const auto layout = makeEditorLayout (getLocalBounds());
    auto panel = [&g] (juce::Rectangle<int> r, const juce::String& title, bool blue = false)
    {
        const auto f = r.toFloat(); g.setColour (juce::Colour (0x17000000)); g.fillRoundedRectangle (f.translated (0.0f, 2.0f), 18.4f);
        g.setColour (juce::Colour (0x28ffffff)); g.fillRoundedRectangle (f, 18.4f); g.setColour (juce::Colour (0x66323437)); g.drawRoundedRectangle (f, 18.4f, 1.0f);
        g.setColour (blue ? juce::Colour (0xff548ae4) : juce::Colour (0xff323437)); g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ((blue ? juce::String ("o  ") : juce::String (">  ")) + title, r.reduced (13, 7).removeFromTop (16), juce::Justification::centredLeft);
    };
    panel (layout.filter, "FILTER"); panel (layout.envelope, "AMP ENVELOPE", true); panel (layout.chrome, "CHROME ENGINE", true);
    panel (layout.modulation, "MODULATION", true); panel (layout.effects, "EFFECTS"); panel (layout.master, "MASTER");
    panel (layout.footer, "");
    auto effectModules = layout.effects.reduced (14, 27); const auto moduleWidth = effectModules.getWidth() / 4;
    for (int i = 0; i < 4; ++i) { const auto m = effectModules.removeFromLeft (moduleWidth).toFloat().reduced (3.0f); g.setColour (juce::Colour (0x14000000)); g.fillRoundedRectangle (m, 12.0f); g.setColour (juce::Colour (0xffa3a5a8)); g.drawRoundedRectangle (m, 12.0f, 0.8f); }
    auto badge = juce::Rectangle<float> (static_cast<float> (layout.header.getX() + 170), static_cast<float> (layout.header.getY() + 16), 48.0f, 20.0f);
    g.setColour (juce::Colour (0x22000000)); g.fillRoundedRectangle (badge.translated (0.0f, 1.0f), 4.0f); g.setColour (juce::Colour (0xffc8c9cb)); g.fillRoundedRectangle (badge, 4.0f);
    g.setColour (juce::Colour (0xff35393d)); g.drawRoundedRectangle (badge, 4.0f, 1.0f); g.setFont (juce::FontOptions (11.0f, juce::Font::bold)); g.drawText ("2000", badge, juce::Justification::centred);
    auto envGraph = layout.envelope.reduced (18, 18).toFloat(); envGraph.removeFromTop (envGraph.getHeight() * 0.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff53565a), envGraph.getX(), envGraph.getY(), juce::Colour (0xff292b2f), envGraph.getX(), envGraph.getBottom(), false)); g.fillRoundedRectangle (envGraph, 10.0f);
    g.setColour (juce::Colour (0xff989b9f)); g.drawRoundedRectangle (envGraph, 10.0f, 0.8f); envGraph.reduce (8.0f, 7.0f);
    juce::Path envelope; envelope.startNewSubPath (envGraph.getX(), envGraph.getBottom()); envelope.lineTo (envGraph.getX() + 25, envGraph.getY() + 8);
    envelope.lineTo (envGraph.getX() + envGraph.getWidth() * 0.42f, envGraph.getY() + 15); envelope.cubicTo (envGraph.getCentreX(), envGraph.getCentreY(), envGraph.getCentreX(), envGraph.getBottom() - 18.0f, envGraph.getRight() - 35.0f, envGraph.getBottom() - 18.0f); envelope.lineTo (envGraph.getRight(), envGraph.getBottom());
    g.setColour (juce::Colour (0xff6c9ee8)); g.strokePath (envelope, juce::PathStrokeType (1.7f));
    auto meterBounds = layout.master.reduced (12, 32); auto meterArea = meterBounds.removeFromRight (34); const auto meterLevel = juce::jlimit (0.0f, 1.0f, displayedPeak);
    for (int bar = 0; bar < 2; ++bar) for (int segment = 0; segment < 12; ++segment)
    {
        const auto lit = static_cast<float> (segment + 1) / 12.0f <= meterLevel;
        const auto h = juce::jmax (2, (meterArea.getHeight() - 11) / 12); const auto y = meterArea.getBottom() - (segment + 1) * (h + 1);
        g.setColour (lit ? juce::Colour (0xff7faeff) : juce::Colour (0xffa8aaad)); g.fillRect (meterArea.getX() + bar * 9, y, 6, h);
    }
    g.setColour (juce::Colour (0xff33373b)); g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("A", layout.footer.getX() + 365, layout.footer.getY(), 24, layout.footer.getHeight(), juce::Justification::centred);
    g.drawText ("B", layout.footer.getRight() - 45, layout.footer.getY(), 24, layout.footer.getHeight(), juce::Justification::centred);
    g.drawText ("X-FADE / MORPH", layout.footer.reduced (0, 3).removeFromTop (14), juce::Justification::centred);
}

void Chrome2000AudioProcessorEditor::resized()
{
    const auto layout = makeEditorLayout (getLocalBounds()); auto header = layout.header;
    auto logoArea = header.removeFromLeft (260); logo.setBounds (logoArea.removeFromTop (42)); subtitle.setBounds (layout.header.getX() + 10, layout.header.getY() + 39, 230, 18); logoHitArea.setBounds (layout.header.getX(), layout.header.getY(), 225, 58);
    power.setBounds (header.removeFromRight (60).reduced (3)); credit.setBounds (header.removeFromRight (145).reduced (5)); save.setBounds (header.removeFromRight (58).reduced (2, 19));
    outputLabel.setBounds (header.removeFromRight (90)); auto presetArea = header.withSizeKeepingCentre (330, 34); previous.setBounds (presetArea.removeFromLeft (35)); next.setBounds (presetArea.removeFromRight (35)); presetBox.setBounds (presetArea);
    auto sampleArea = layout.samples; const auto gap = 8; const auto slotWidth = (sampleArea.getWidth() - gap * 2) / 3;
    for (auto& slot : slots) { slot->setBounds (sampleArea.removeFromLeft (slotWidth)); sampleArea.removeFromLeft (gap); }
    auto layoutGrid = [] (auto& controls, juce::Rectangle<int> bounds, int columns)
    {
        const auto rows = static_cast<int> ((controls.size() + static_cast<size_t> (columns - 1)) / static_cast<size_t> (columns));
        const auto w = bounds.getWidth() / columns, h = bounds.getHeight() / juce::jmax (1, rows);
        for (size_t i = 0; i < controls.size(); ++i) controls[i]->setBounds (bounds.getX() + static_cast<int> (i % static_cast<size_t> (columns)) * w, bounds.getY() + static_cast<int> (i / static_cast<size_t> (columns)) * h, w, h);
    };
    auto filterArea = layout.filter.reduced (12, 24); filterType.setBounds (filterArea.removeFromBottom (30));
    filterKnobs[0]->setBounds (filterArea.removeFromTop (filterArea.getHeight() / 2).withSizeKeepingCentre (130, 120));
    const auto filterWidth = filterArea.getWidth() / 3; for (size_t i = 1; i < 4; ++i) filterKnobs[i]->setBounds (filterArea.removeFromLeft (filterWidth));
    auto envelopeArea = layout.envelope.reduced (14, 25); layoutGrid (envelopeKnobs, envelopeArea.removeFromTop (envelopeArea.getHeight() / 2), 4);
    layoutGrid (chromeKnobs, layout.chrome.reduced (12, 25), 3);
    auto modArea = layout.modulation.reduced (14, 25); auto modHeader = modArea.removeFromTop (32); modulationSource.setBounds (modHeader.removeFromLeft (110)); lfoShape.setBounds (modHeader.removeFromLeft (110).reduced (6, 0)); modulationDestination.setBounds (modHeader.removeFromRight (110));
    auto modKnobArea = modArea.removeFromTop (modArea.getHeight() / 2); layoutGrid (modulationKnobs, modKnobArea, 3); lfoDisplay.setBounds (modArea.reduced (0, 3));
    auto effectsArea = layout.effects.reduced (14, 25); const auto effectWidth = effectsArea.getWidth() / 4; for (size_t i = 0; i < 4; ++i) effectKnobs[i]->setBounds (effectsArea.removeFromLeft (effectWidth));
    auto masterArea = layout.master.reduced (14, 25); masterArea.removeFromRight (38); const auto masterWidth = masterArea.getWidth() / 2; effectKnobs[4]->setBounds (masterArea.removeFromLeft (masterWidth)); effectKnobs[5]->setBounds (masterArea);
    auto footer = layout.footer.reduced (16, 8); waveTab.setBounds (footer.removeFromLeft (92)); footer.removeFromLeft (8); spectrumTab.setBounds (footer.removeFromLeft (104)); footer.removeFromLeft (8); modTab.setBounds (footer.removeFromLeft (82)); footer.removeFromLeft (28); footer.removeFromRight (40); morphX.setBounds (footer.withTrimmedTop (7));
}

void Chrome2000AudioProcessorEditor::timerCallback()
{
    displayedPeak = juce::jmax (processor.engine.outputPeak(), displayedPeak * 0.9f);
    outputLabel.setText ("OUT  " + juce::String (juce::Decibels::gainToDecibels (displayedPeak, -80.0f), 1) + " dB", juce::dontSendNotification);
    repaint();
}

void Chrome2000AudioProcessorEditor::rebuildPresetList()
{
    presetBox.clear(); presetBox.addItemList (processor.presets.names(), 1); presetBox.setText (processor.presets.currentName(), juce::dontSendNotification);
}

void Chrome2000AudioProcessorEditor::savePreset()
{
    auto window = std::make_unique<juce::AlertWindow> ("Save preset", "Enter a name for this sound.", juce::MessageBoxIconType::NoIcon);
    window->addTextEditor ("name", processor.presets.currentName()); window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey)); window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    const auto safe = juce::Component::SafePointer<Chrome2000AudioProcessorEditor> (this); auto* raw = window.release();
    raw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, raw] (int result)
    {
        std::unique_ptr<juce::AlertWindow> owner (raw);
        if (safe != nullptr && result == 1 && safe->processor.presets.save (raw->getTextEditorContents ("name"))) safe->rebuildPresetList();
    }), false);
}

void Chrome2000AudioProcessorEditor::showAbout()
{
    juce::AlertWindow::showMessageBoxAsync (
        juce::MessageBoxIconType::InfoIcon,
        "CHROME 2000  /  LEVSAN",
        "SAMPLE SYNTH\n\n"
        "Desarrollado por Leonardo Verdesoto\n"
        "LEVSAN - 2026\n"
        "+593 - Ecuador\n\n"
        "LICENCIA\n"
        "Se prohibe la distribucion parcial o completa de este producto sin autorizacion expresa del desarrollador.\n\n"
        "Todos los derechos reservados.",
        "CERRAR");
}

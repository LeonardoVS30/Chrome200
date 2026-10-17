#include "PluginEditor.h"
#include "Parameters/ParameterLayout.h"
#include <BinaryData.h>

namespace
{
constexpr float designWidth = 2450.0f;
constexpr float designHeight = 1749.0f;

juce::Rectangle<float> designRect (const juce::Component& component, float x, float y, float width, float height)
{
    return { x * static_cast<float> (component.getWidth()) / designWidth,
             y * static_cast<float> (component.getHeight()) / designHeight,
             width * static_cast<float> (component.getWidth()) / designWidth,
             height * static_cast<float> (component.getHeight()) / designHeight };
}

juce::Rectangle<int> designBounds (const juce::Component& component, float x, float y, float width, float height)
{
    return designRect (component, x, y, width, height).getSmallestIntegerContainer();
}

void drawMicrograin (juce::Graphics& g, juce::Rectangle<float> bounds, float spacing)
{
    g.setColour (juce::Colour (0x0affffff));
    for (float y = bounds.getY() + spacing; y < bounds.getBottom(); y += spacing)
        g.drawHorizontalLine (static_cast<int> (y), bounds.getX() + 3.0f, bounds.getRight() - 3.0f);
}
}

Chrome2000AudioProcessorEditor::BoundKnob::BoundKnob (juce::AudioProcessorValueTreeState& state,
                                                       const juce::String& id,
                                                       const juce::String& name,
                                                       bool large,
                                                       bool glow)
    : parameterId (id), attachment (state, id, slider)
{
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xff292b2e));
    value.setJustificationType (juce::Justification::centred);
    value.setFont (juce::FontOptions (9.7f));
    value.setColour (juce::Label::textColourId, juce::Colour (0xff292b2e));
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.getProperties().set ("bigKnob", large);
    slider.getProperties().set ("glowKnob", glow);
    slider.addListener (this);
    slider.onDragStart = [this] { if (onDragStart) onDragStart(); };
    slider.onDragEnd = [this] { if (onDragEnd) onDragEnd(); };
    addAndMakeVisible (label);
    addAndMakeVisible (value);
    addAndMakeVisible (slider);
    if (name.isEmpty())
    {
        label.setVisible (false);
        value.setVisible (false);
    }
    updateValue();
}

void Chrome2000AudioProcessorEditor::BoundKnob::resized()
{
    auto bounds = getLocalBounds();
    if (figmaSkinMode)
    {
        slider.setBounds (bounds);
        return;
    }
    if (! label.isVisible())
    {
        slider.setBounds (bounds);
        return;
    }
    const auto valueHeight = juce::jmax (12, static_cast<int> (getHeight() * 0.15f));
    value.setBounds (bounds.removeFromBottom (valueHeight));
    label.setBounds (bounds.removeFromBottom (valueHeight));
    slider.setBounds (bounds);
}

void Chrome2000AudioProcessorEditor::BoundKnob::setFigmaSkinMode (bool enabled)
{
    figmaSkinMode = enabled;
    label.setVisible (! enabled && label.getText().isNotEmpty());
    value.setVisible (! enabled && label.getText().isNotEmpty());

    auto bounds = getLocalBounds();
    if (! enabled && label.getText().isNotEmpty())
    {
        const auto valueHeight = juce::jmax (12, static_cast<int> (getHeight() * 0.15f));
        bounds.removeFromBottom (valueHeight);
        bounds.removeFromBottom (valueHeight);
    }
    slider.setBounds (bounds);
}

void Chrome2000AudioProcessorEditor::BoundKnob::sliderValueChanged (juce::Slider*)
{
    updateValue();
}

void Chrome2000AudioProcessorEditor::BoundKnob::updateValue()
{
    const auto v = slider.getValue();
    juce::String text;
    if (parameterId == "filter_cutoff")
        text = v >= 1000.0 ? juce::String (v / 1000.0, 2) + " kHz" : juce::String (v, 0) + " Hz";
    else if (parameterId == "filter_drive")
        text = juce::String (v, 1) + " dB";
    else if (parameterId == "amp_attack" || parameterId == "amp_decay" || parameterId == "amp_release")
        text = v < 1.0 ? juce::String (v * 1000.0, 0) + " ms" : juce::String (v, 2) + " s";
    else if (parameterId == "global_pitch")
        text = juce::String (v, 2) + " st";
    else if (parameterId == "global_fine")
        text = juce::String (v, 2) + " ct";
    else if (parameterId == "glide")
        text = juce::String (v * 1000.0, 0) + " ms";
    else if (parameterId == "lfo1_rate")
        text = std::abs (v - 1.0) < 0.01 ? "1/4" : juce::String (v, 2) + " Hz";
    else if (parameterId == "delay_time")
        text = std::abs (v - 0.375) < 0.01 ? "1/4" : juce::String (v * 1000.0, 0) + " ms";
    else if (parameterId.contains ("gain"))
        text = juce::String (v, 1) + " dB";
    else if (parameterId == "chorus" || parameterId == "delay" || parameterId == "delay_feedback"
             || parameterId == "reverb" || parameterId == "reverb_size" || parameterId == "reverb_damp"
             || parameterId == "lfo1_depth")
        text = juce::String (v * 100.0, 0) + " %";
    else
        text = juce::String (v, 2);
    value.setText (text, juce::dontSendNotification);
    valueText = text;
    slider.setTooltip ((label.getText().isNotEmpty() ? label.getText() : parameterId)
                       + ": " + text);
}

void Chrome2000AudioProcessorEditor::ValueBubble::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (juce::Colour (0xee2a2c2f));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (juce::Colour (0xff9fc2ff));
    g.drawRoundedRectangle (b, 6.0f, 1.0f);
    g.setColour (juce::Colour (0xffeef0f2));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (text, b, juce::Justification::centred);
}

void Chrome2000AudioProcessorEditor::LfoDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    juce::ColourGradient background (juce::Colour (0xff57595c), bounds.getX(), bounds.getY(),
                                     juce::Colour (0xff34363a), bounds.getX(), bounds.getBottom(), false);
    background.addColour (0.48, juce::Colour (0xff45474a));
    g.setGradientFill (background);
    g.fillRoundedRectangle (bounds, 9.0f);
    g.setColour (juce::Colour (0xff8f9296));
    g.drawRoundedRectangle (bounds, 9.0f, 0.8f);

    juce::Path wave;
    for (int x = 0; x <= getWidth(); ++x)
    {
        const auto phase = static_cast<float> (x) / static_cast<float> (juce::jmax (1, getWidth()))
                         * juce::MathConstants<float>::twoPi * 2.75f;
        const auto y = bounds.getCentreY() - std::sin (phase) * bounds.getHeight() * 0.34f;
        if (x == 0) wave.startNewSubPath (static_cast<float> (x), y);
        else wave.lineTo (static_cast<float> (x), y);
    }
    g.setColour (juce::Colour (0xffa4c6ff));
    g.strokePath (wave, juce::PathStrokeType (1.45f));
}

Chrome2000AudioProcessorEditor::AboutOverlay::AboutOverlay()
{
    setInterceptsMouseClicks (true, true);
    addAndMakeVisible (close);
    close.onClick = [this] { setVisible (false); };
}

void Chrome2000AudioProcessorEditor::AboutOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xa6000000));
    auto card = getLocalBounds().toFloat().withSizeKeepingCentre (juce::jmin (520.0f, getWidth() * 0.50f),
                                                                  juce::jmin (360.0f, getHeight() * 0.48f));
    g.setColour (juce::Colour (0x66000000));
    g.fillRoundedRectangle (card.translated (0.0f, 8.0f), 22.0f);
    juce::ColourGradient metal (juce::Colour (0xfff1f0f0), card.getX(), card.getY(),
                                juce::Colour (0xffaaa9a9), card.getX(), card.getBottom(), false);
    metal.addColour (0.48, juce::Colour (0xffd1d0d0));
    g.setGradientFill (metal);
    g.fillRoundedRectangle (card, 22.0f);
    g.setColour (juce::Colour (0xff4b4d51));
    g.drawRoundedRectangle (card, 22.0f, 1.5f);
    g.setColour (juce::Colour (0x7fffffff));
    g.drawRoundedRectangle (card.reduced (3.0f), 19.0f, 1.0f);

    auto content = card.reduced (42.0f, 28.0f);
    g.setColour (juce::Colour (0xff24272b));
    g.setFont (juce::FontOptions (29.0f, juce::Font::bold));
    g.drawText ("CHROME 2000", content.removeFromTop (42.0f), juce::Justification::centred);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("LEVSAN  /  SAMPLE SYNTH", content.removeFromTop (25.0f), juce::Justification::centred);
    content.removeFromTop (14.0f);
    g.setFont (juce::FontOptions (15.0f));
    const auto middleDot = juce::String::charToString (0x00b7);
    juce::String developerInfo ("Desarrollado por Leonardo Verdesoto");
    developerInfo << "\nLEVSAN " << middleDot << " 2026"
                  << "\n+593 " << middleDot << " Ecuador";
    g.drawFittedText (developerInfo,
                      content.removeFromTop (75.0f).getSmallestIntegerContainer(),
                      juce::Justification::centred, 3);
    content.removeFromTop (8.0f);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText ("LICENCIA", content.removeFromTop (22.0f), juce::Justification::centred);
    g.setFont (juce::FontOptions (12.0f));
    juce::String licence ("Est");
    licence << juce::String::charToString (0x00e1)
            << " prohibida la distribuci"
            << juce::String::charToString (0x00f3)
            << "n parcial o completa de este producto sin autorizaci"
            << juce::String::charToString (0x00f3)
            << "n expresa del desarrollador.\nTodos los derechos reservados.";
    g.drawFittedText (licence,
                      content.removeFromTop (58.0f).getSmallestIntegerContainer(), juce::Justification::centred, 3);
}

void Chrome2000AudioProcessorEditor::AboutOverlay::resized()
{
    const auto card = getLocalBounds().toFloat().withSizeKeepingCentre (juce::jmin (520.0f, getWidth() * 0.50f),
                                                                        juce::jmin (360.0f, getHeight() * 0.48f));
    close.setBounds (card.withSizeKeepingCentre (120.0f, 32.0f).withY (card.getBottom() - 48.0f).getSmallestIntegerContainer());
}

std::unique_ptr<juce::Drawable> Chrome2000AudioProcessorEditor::drawableFromData (const void* data, int size)
{
    return juce::Drawable::createFromImageData (data, static_cast<size_t> (size));
}

Chrome2000AudioProcessorEditor::Chrome2000AudioProcessorEditor (Chrome2000AudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
       powerAttachment (p.parameters, "power", power),
       legatoAttachment (p.parameters, "legato", legato),
       chorusToggleAttachment (p.parameters, "chorus_enabled", chorusToggle),
       delayToggleAttachment (p.parameters, "delay_enabled", delayToggle),
       reverbToggleAttachment (p.parameters, "reverb_enabled", reverbToggle),
      filterTypeAttachment (p.parameters, "filter_type", filterType),
      modulationSourceAttachment (p.parameters, "mod1_source", modulationSource),
      modulationDestinationAttachment (p.parameters, "mod1_destination", modulationDestination),
       lfoShapeAttachment (p.parameters, "lfo1_shape", lfoShape),
       lfo2ShapeAttachment (p.parameters, "lfo2_shape", lfo2Shape),
       lfo3ShapeAttachment (p.parameters, "lfo3_shape", lfo3Shape),
      morphXAttachment (p.parameters, "morph_x", morphX)
{
    setLookAndFeel (&lookAndFeel);
    figmaSkin = juce::ImageFileFormat::loadFrom (
        BinaryData::chrome2000_figma_skin_png,
        static_cast<size_t> (BinaryData::chrome2000_figma_skin_pngSize));
    logoMark = drawableFromData (BinaryData::chrome_logo_png, BinaryData::chrome_logo_pngSize);
    wordmark = drawableFromData (BinaryData::chrome_wordmark_svg, BinaryData::chrome_wordmark_svgSize);

    logo.setVisible (false);
    subtitle.setText ("S A M P L E   S Y N T H", juce::dontSendNotification);
    subtitle.setFont (juce::FontOptions (5.2f, juce::Font::bold));
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xff292b2e));
    credit.setText ("DIETER RAMS\nINSPIRED DESIGN\nY2K EDITION", juce::dontSendNotification);
    credit.setJustificationType (juce::Justification::centredRight);
    credit.setFont (juce::FontOptions (6.0f, juce::Font::bold));
    outputLabel.setVisible (false);

    for (auto* component : std::initializer_list<juce::Component*> {
              &subtitle, &logoHitArea, &presetBox, &previous, &save, &power, &credit,
              &filterType, &modulationSource, &modulationDestination, &lfoShape,
              &lfo2Shape, &lfo3Shape,
              &lfoDisplay, &morphX, &waveTab, &spectrumTab, &modTab, &legato,
              &chorusToggle, &delayToggle, &reverbToggle })
        addAndMakeVisible (component);

    next.setVisible (false);
    logoHitArea.setComponentID ("chromeLogoHitArea");
    logoHitArea.getProperties().set ("logoHitArea", true);
    logoHitArea.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    logoHitArea.setTooltip ("Acerca de CHROME 2000");
    logoHitArea.setAlwaysOnTop (true);
    logoHitArea.onClick = [this] { showAbout(); };
    power.setClickingTogglesState (true);
    power.getProperties().set ("chromePower", true);
    power.setButtonText ({});
    previous.setButtonText ("<");
    save.setButtonText ({});
    save.getProperties().set ("saveIcon", true);
    filterType.addItemList ({ "LP 24dB", "HP 24dB", "BP 12dB", "NOTCH" }, 1);
    modulationSource.addItemList ({ "OFF", "LFO 1", "LFO 2", "LFO 3", "VELOCITY", "AFTERTOUCH", "RANDOM", "AMP ENV", "MOD ENV", "LIQUID", "REFLECT", "FRAGMENT", "SHINE", "DENSITY", "FUSION" }, 1);
    modulationSource.getProperties().set ("darkCombo", true);
    modulationSource.setColour (juce::ComboBox::textColourId, juce::Colour (0xffeef0f3));
    for (auto* c : { &lfo2Shape, &lfo3Shape })
    {
        c->getProperties().set ("darkCombo", true);
        c->setColour (juce::ComboBox::textColourId, juce::Colour (0xffeef0f3));
    }
    modulationDestination.addItemList ({ "PITCH", "CUTOFF", "POSITION", "GRAIN SIZE", "DENSITY", "MIX", "FX", "LIQUID", "REFLECT", "SHINE" }, 1);
    lfoShape.addItemList ({ "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM", "S&H" }, 1);
    lfo2Shape.addItemList ({ "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM", "S&H" }, 1);
    lfo3Shape.addItemList ({ "SINE", "TRIANGLE", "SAW", "SQUARE", "RANDOM", "S&H" }, 1);
    filterType.setSelectedItemIndex (static_cast<int> (chrome::params::load (p.parameters, "filter_type")), juce::dontSendNotification);
    modulationSource.setSelectedItemIndex (static_cast<int> (chrome::params::load (p.parameters, "mod1_source")), juce::dontSendNotification);
    modulationDestination.setSelectedItemIndex (static_cast<int> (chrome::params::load (p.parameters, "mod1_destination")), juce::dontSendNotification);
    lfoShape.setSelectedItemIndex (static_cast<int> (chrome::params::load (p.parameters, "lfo1_shape")), juce::dontSendNotification);
    lfo2Shape.setSelectedItemIndex (static_cast<int> (chrome::params::load (p.parameters, "lfo2_shape")), juce::dontSendNotification);
    lfo3Shape.setSelectedItemIndex (static_cast<int> (chrome::params::load (p.parameters, "lfo3_shape")), juce::dontSendNotification);
    morphX.setSliderStyle (juce::Slider::LinearHorizontal);
    morphX.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphX.setColour (juce::Slider::trackColourId, juce::Colour (0xff91b8f4));
    morphX.setColour (juce::Slider::thumbColourId, juce::Colour (0xfff7f8fa));
    legato.setClickingTogglesState (true);
    for (auto* t : { &chorusToggle, &delayToggle, &reverbToggle }) t->setClickingTogglesState (true);

    for (auto* tab : { &waveTab, &spectrumTab, &modTab })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (2000);
        tab->getProperties().set ("footerTab", true);
        tab->setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd9dadd));
        tab->setColour (juce::TextButton::textColourOnId, juce::Colour (0xfff4f6f8));
    }
    waveTab.setToggleState (true, juce::dontSendNotification);
    waveTab.onClick = [this] { for (auto& slot : slots) slot->setViewMode (chrome::WaveformComponent::ViewMode::wave); };
    spectrumTab.onClick = [this] { for (auto& slot : slots) slot->setViewMode (chrome::WaveformComponent::ViewMode::spectrum); };
    modTab.onClick = [this] { for (auto& slot : slots) slot->setViewMode (chrome::WaveformComponent::ViewMode::modulation); };

    for (int i = 0; i < 3; ++i)
    {
        slots[static_cast<size_t> (i)] = std::make_unique<chrome::SampleSlotComponent> (p.parameters, p.engine, i);
        slots[static_cast<size_t> (i)]->setFigmaSkinMode (true);
        addAndMakeVisible (*slots[static_cast<size_t> (i)]);
    }

    auto addKnob = [this, &p] (auto& destination, const char* id, const char* name, bool large = false, bool glow = false)
    {
        auto knob = std::make_unique<BoundKnob> (p.parameters, id, name, large, glow);
        BoundKnob* raw = knob.get();
        knob->onDragStart = [this, raw] { activeDragKnob = raw; valueBubble.setText (raw->getValueText()); valueBubble.setVisible (true); valueBubble.toFront (false); };
        knob->onDragEnd = [this] { activeDragKnob = nullptr; valueBubble.setVisible (false); };
        destination.push_back (std::move (knob));
        addAndMakeVisible (*destination.back());
    };

    addKnob (filterKnobs, "filter_cutoff", "CUTOFF", true, true);
    addKnob (filterKnobs, "filter_resonance", "RESO");
    addKnob (filterKnobs, "filter_drive", "DRIVE");
    addKnob (filterKnobs, "filter_env_amount", "ENV");
    addKnob (filterKnobs, "filter_keytrack", "KEYTRK");
    addKnob (envelopeKnobs, "amp_attack", "ATTACK");
    addKnob (envelopeKnobs, "amp_decay", "DECAY");
    addKnob (envelopeKnobs, "amp_sustain", "SUSTAIN");
    addKnob (envelopeKnobs, "amp_release", "RELEASE");
    addKnob (pitchKnobs, "global_pitch", " ", true, true);
    addKnob (pitchKnobs, "glide", "GLIDE");
    addKnob (pitchKnobs, "global_fine", "FINE");
    addKnob (pitchKnobs, "morph_y", "");
    addKnob (modulationKnobs, "lfo1_rate", "RATE");
    addKnob (modulationKnobs, "lfo1_depth", "AMOUNT");
    addKnob (modulationKnobs, "lfo2_rate", "RATE2");
    addKnob (modulationKnobs, "lfo2_depth", "AMT2");
    addKnob (modulationKnobs, "lfo3_rate", "RATE3");
    addKnob (modulationKnobs, "lfo3_depth", "AMT3");
    addKnob (effectKnobs, "chorus", "MIX");
    addKnob (effectKnobs, "delay_time", "TIME");
    addKnob (effectKnobs, "delay_feedback", "FEEDBACK");
    addKnob (effectKnobs, "delay", "MIX");
    addKnob (effectKnobs, "reverb_size", "SIZE");
    addKnob (effectKnobs, "reverb_damp", "DAMP");
    addKnob (effectKnobs, "reverb", "MIX");
    addKnob (masterKnobs, "master_input_gain", "GAIN");
    addKnob (masterKnobs, "master_gain", "OUTPUT");

    for (auto* collection : { &filterKnobs, &envelopeKnobs, &pitchKnobs,
                              &effectKnobs, &masterKnobs, &modulationKnobs })
        for (auto& knob : *collection) knob->setFigmaSkinMode (true);

    rebuildPresetList();
    presetBox.onChange = [this] { if (presetBox.getText().isNotEmpty()) processor.presets.load (presetBox.getText()); };
    previous.onClick = [this] { presetBox.setText (processor.presets.step (-1), juce::dontSendNotification); };
    save.onClick = [this] { savePreset(); };
    processor.engine.sampleChanged = [safe = juce::Component::SafePointer<Chrome2000AudioProcessorEditor> (this)] (int slot, const juce::String& error)
    {
        if (safe == nullptr) return;
        safe->slots[static_cast<size_t> (slot)]->refresh();
        safe->repaint();
        if (error.isNotEmpty())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Sample loading", error);
    };
    for (auto& slot : slots) slot->refresh();

    // The Figma frame is the authoritative visual layer. Existing JUCE controls
    // remain active as transparent hit targets, so automation, accessibility,
    // sample loading and parameter attachments keep working without redrawing
    // an approximation over the vector-authored skin.
    constexpr float hitTargetAlpha = 0.001f;
    for (auto* component : std::initializer_list<juce::Component*> {
              &subtitle, &presetBox, &previous, &save, &power, &credit,
              &filterType, &modulationSource, &modulationDestination, &lfoShape,
              &lfoDisplay, &morphX, &waveTab, &spectrumTab, &modTab, &legato,
              &chorusToggle, &delayToggle, &reverbToggle })
        component->setAlpha (hitTargetAlpha);

    addAndMakeVisible (aboutOverlay);
    aboutOverlay.setVisible (false);
    addAndMakeVisible (valueBubble);
    valueBubble.setVisible (false);

    setResizable (true, true);
    setResizeLimits (980, 700, 1960, 1399);
    getConstrainer()->setFixedAspectRatio (designWidth / designHeight);
    setSize (1225, 874);
    startTimerHz (30);
}

Chrome2000AudioProcessorEditor::~Chrome2000AudioProcessorEditor()
{
    processor.engine.sampleChanged = {};
    setLookAndFeel (nullptr);
}

void Chrome2000AudioProcessorEditor::paint (juce::Graphics& g)
{
    if (figmaSkin.isValid())
    {
        g.drawImage (figmaSkin, getLocalBounds().toFloat(),
                     juce::RectanglePlacement::stretchToFit);

        const auto sx = static_cast<float> (getWidth()) / designWidth;
        const auto sy = static_cast<float> (getHeight()) / designHeight;

        chrome::WaveformComponent::ViewMode viewMode = chrome::WaveformComponent::ViewMode::wave;
        if (spectrumTab.getToggleState()) viewMode = chrome::WaveformComponent::ViewMode::spectrum;
        else if (modTab.getToggleState()) viewMode = chrome::WaveformComponent::ViewMode::modulation;

        // Persistent value readouts under each knob (FL Studio style), masking the
        // baked-in numbers of the Figma skin with the sampled panel colour.
        auto sampleSkin = [this] (juce::Point<int> p) -> juce::Colour
        {
            if (! figmaSkin.isValid()) return juce::Colour (0xffc4c2c1);
            const int sxp = juce::jlimit (0, figmaSkin.getWidth() - 1,
                                          static_cast<int> (static_cast<float> (p.x) * static_cast<float> (figmaSkin.getWidth()) / static_cast<float> (juce::jmax (1, getWidth()))));
            const int syp = juce::jlimit (0, figmaSkin.getHeight() - 1,
                                          static_cast<int> (static_cast<float> (p.y) * static_cast<float> (figmaSkin.getHeight()) / static_cast<float> (juce::jmax (1, getHeight()))));
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
            int count = 0;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    const int px = juce::jlimit (0, figmaSkin.getWidth() - 1, sxp + dx);
                    const int py = juce::jlimit (0, figmaSkin.getHeight() - 1, syp + dy);
                    const auto pp = figmaSkin.getPixelAt (px, py);
                    r += pp.getFloatRed(); g += pp.getFloatGreen(); b += pp.getFloatBlue(); a += pp.getFloatAlpha();
                    ++count;
                }
            return juce::Colour (static_cast<juce::uint8> (r / count * 255.0f),
                                 static_cast<juce::uint8> (g / count * 255.0f),
                                 static_cast<juce::uint8> (b / count * 255.0f),
                                 static_cast<juce::uint8> (a / count * 255.0f));
        };

        // Replace the three illustrative Figma sample previews with the actual
        // engine state. This is the only intentionally dynamic area of the skin.
        constexpr std::array<float, 3> slotX { 78.0f, 710.0f, 1306.0f };
        constexpr std::array<float, 3> slotW { 617.0f, 583.0f, 623.0f };
        constexpr std::array<float, 3> fileX { 106.0f, 737.0f, 1332.0f };
        constexpr std::array<float, 3> fileY { 248.0f, 248.0f, 252.0f };
        constexpr std::array<float, 3> fileW { 407.0f, 361.0f, 407.0f };
        for (int slot = 0; slot < 3; ++slot)
        {
            const auto xScale = slotW[static_cast<size_t> (slot)] / 617.0f;
            const auto fileArea = designRect (*this,
                                               fileX[static_cast<size_t> (slot)],
                                               fileY[static_cast<size_t> (slot)],
                                               fileW[static_cast<size_t> (slot)],
                                               51.0f);
            const auto waveArea = designRect (*this,
                                               slotX[static_cast<size_t> (slot)] + 27.0f * xScale,
                                               309.0f,
                                               560.0f * xScale,
                                               122.0f);
            const auto metadataArea = designRect (*this,
                                                   slotX[static_cast<size_t> (slot)] + 37.2f * xScale,
                                                   443.5f,
                                                   350.0f * xScale,
                                                   28.0f);

            juce::ColourGradient fileFill (juce::Colour (0xffdedddd), fileArea.getX(), fileArea.getY(),
                                           juce::Colour (0xffb7b6b6), fileArea.getX(), fileArea.getBottom(), false);
            g.setGradientFill (fileFill);
            g.fillRoundedRectangle (fileArea, 8.0f * juce::jmin (sx, sy));
            g.setColour (juce::Colour (0xff85878a));
            g.drawRoundedRectangle (fileArea, 8.0f * juce::jmin (sx, sy), 0.8f * juce::jmin (sx, sy));

            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff242628), waveArea.getX(), waveArea.getY(),
                                                     juce::Colour (0xff4b4d50), waveArea.getX(), waveArea.getBottom(), false));
            g.fillRoundedRectangle (waveArea, 6.0f * juce::jmin (sx, sy));
            g.setColour (juce::Colour (0xff85888c));
            g.drawRoundedRectangle (waveArea, 6.0f * juce::jmin (sx, sy), 0.8f * juce::jmin (sx, sy));

            if (const auto sample = processor.engine.sample (slot))
            {
                g.setColour (juce::Colour (0xff282a2d));
                g.setFont (juce::FontOptions (23.0f * sy, juce::Font::bold));
                g.drawFittedText (sample->file().getFileName(),
                                  fileArea.reduced (42.0f * sx, 0.0f).getSmallestIntegerContainer(),
                                  juce::Justification::centredLeft, 1);

                if (viewMode == chrome::WaveformComponent::ViewMode::wave)
                {
                    const auto& peaks = sample->peaks();
                    if (peaks.size() > 1)
                    {
                        juce::Path waveform;
                        const auto centre = waveArea.getCentreY();
                        for (size_t i = 0; i < peaks.size(); ++i)
                        {
                            const auto x = waveArea.getX() + static_cast<float> (i)
                                         / static_cast<float> (peaks.size() - 1) * waveArea.getWidth();
                            const auto height = peaks[i] * waveArea.getHeight() * 0.42f;
                            waveform.startNewSubPath (x, centre - height);
                            waveform.lineTo (x, centre + height);
                        }
                        g.setColour (juce::Colour (0xffdfe1e4).withAlpha (0.90f));
                        g.strokePath (waveform, juce::PathStrokeType (1.0f * sx));

                        const auto start = chrome::params::load (processor.parameters, chrome::params::slotId (slot, "start"));
                        const auto end = chrome::params::load (processor.parameters, chrome::params::slotId (slot, "end"));
                        g.setColour (juce::Colour (0x66303336));
                        g.fillRect (waveArea.withRight (waveArea.getX() + waveArea.getWidth() * juce::jmin (start, end)));
                        g.fillRect (waveArea.withLeft (waveArea.getX() + waveArea.getWidth() * juce::jmax (start, end)));
                        g.setColour (juce::Colour (0xff91b8ff));
                        g.drawVerticalLine (static_cast<int> (waveArea.getX() + waveArea.getWidth() * start),
                                            waveArea.getY(), waveArea.getBottom());
                        g.drawVerticalLine (static_cast<int> (waveArea.getX() + waveArea.getWidth() * end),
                                            waveArea.getY(), waveArea.getBottom());
                    }
                }
                else if (viewMode == chrome::WaveformComponent::ViewMode::spectrum)
                {
                    const auto& values = sample->spectrum();
                    const auto width = waveArea.getWidth() / static_cast<float> (values.size());
                    for (size_t i = 0; i < values.size(); ++i)
                    {
                        const auto height = values[i] * waveArea.getHeight() * 0.82f;
                        g.setColour (juce::Colour (0xff85b3ff).withAlpha (0.32f + values[i] * 0.55f));
                        g.fillRoundedRectangle (waveArea.getX() + static_cast<float> (i) * width,
                                                waveArea.getBottom() - height,
                                                juce::jmax (1.0f, width - 1.0f), height, 1.0f);
                    }
                }
                else
                {
                    const auto shape = static_cast<int> (chrome::params::load (processor.parameters, "lfo1_shape"));
                    juce::Path path;
                    for (int x = 0; x <= static_cast<int> (waveArea.getWidth()); ++x)
                    {
                        const auto phase = static_cast<float> (x) / juce::jmax (1.0f, waveArea.getWidth()) * 3.0f;
                        float value = 0.0f;
                        const auto unit = phase - std::floor (phase);
                        if (shape == 0) value = std::sin (phase * juce::MathConstants<float>::twoPi);
                        else if (shape == 1) value = 1.0f - 4.0f * std::abs (unit - 0.5f);
                        else if (shape == 2) value = unit * 2.0f - 1.0f;
                        else if (shape == 3) value = unit < 0.5f ? 1.0f : -1.0f;
                        else value = std::sin (phase * 9.73f) * 0.75f;
                        const auto y = waveArea.getCentreY() - value * waveArea.getHeight() * 0.36f;
                        if (x == 0) path.startNewSubPath (waveArea.getX() + static_cast<float> (x), y);
                        else path.lineTo (waveArea.getX() + static_cast<float> (x), y);
                    }
                    g.setColour (juce::Colour (0xff85b3ff));
                    g.strokePath (path, juce::PathStrokeType (1.4f * sx));
                }

                g.setColour (juce::Colour (0xffb9b8b8));
                g.fillRect (metadataArea);
                g.setColour (juce::Colour (0xff46494c));
                g.setFont (juce::FontOptions (21.0f * sy));
                const auto metadata = juce::String (sample->sampleRate() / 1000.0, 1)
                                    + " kHz / 24 bit / "
                                    + (sample->channels() == 1 ? "Mono" : "Stereo");
                g.drawText (metadata, metadataArea, juce::Justification::centredLeft);
            }
            else
            {
                g.setColour (juce::Colour (0xff34373a));
                g.setFont (juce::FontOptions (21.0f * sy, juce::Font::bold));
                g.drawText ("CLICK TO LOAD SAMPLE", fileArea.reduced (42.0f * sx, 0.0f),
                            juce::Justification::centredLeft);
                g.setColour (juce::Colour (0xffc4c7cb));
                g.setFont (juce::FontOptions (17.0f * sy));
                g.drawText ("DROP WAV / AIFF / AIF / FLAC", waveArea,
                            juce::Justification::centred);
                g.setColour (juce::Colour (0xffb9b8b8));
                g.fillRect (metadataArea);
                g.setColour (juce::Colour (0xff4b4e51));
                g.setFont (juce::FontOptions (18.0f * sy));
                g.drawText ("NO SAMPLE LOADED", metadataArea,
                            juce::Justification::centredLeft);
            }

            // Dynamic readouts for slot knobs: mask baked numbers, draw live values.
            {
                const auto xs = slotW[static_cast<size_t> (slot)] / 617.0f;
                const auto vsx = juce::jmin (sx, sy);
                // START
                {
                    auto r = designRect (*this, slotX[static_cast<size_t> (slot)] + 23.0f * xs,
                                          624.0f, 140.0f * xs, 27.0f);
                    const auto v = chrome::params::load (processor.parameters, chrome::params::slotId (slot, "start"));
                    const auto mask = sampleSkin (r.getCentre().toInt());
                    g.setColour (mask);
                    g.fillRoundedRectangle (r.toFloat().expanded (2.0f, 1.0f), 4.0f * vsx);
                    g.setColour (juce::Colour (0xff292b2e));
                    g.setFont (juce::FontOptions (21.5f * sy));
                    g.drawFittedText (juce::String (v * 100.0, 2) + " %", r.getSmallestIntegerContainer(), juce::Justification::centred, 1);
                }
                // END
                {
                    auto r = designRect (*this, slotX[static_cast<size_t> (slot)] + 193.0f * xs,
                                          624.0f, 140.0f * xs, 27.0f);
                    const auto v = chrome::params::load (processor.parameters, chrome::params::slotId (slot, "end"));
                    const auto mask = sampleSkin (r.getCentre().toInt());
                    g.setColour (mask);
                    g.fillRoundedRectangle (r.toFloat().expanded (2.0f, 1.0f), 4.0f * vsx);
                    g.setColour (juce::Colour (0xff292b2e));
                    g.setFont (juce::FontOptions (21.5f * sy));
                    g.drawFittedText (juce::String (v * 100.0, 2) + " %", r.getSmallestIntegerContainer(), juce::Justification::centred, 1);
                }
                // GAIN
                {
                    auto r = designRect (*this, slotX[static_cast<size_t> (slot)] + 423.8f * xs,
                                          653.9f, 129.0f * xs, 33.0f);
                    const auto v = chrome::params::load (processor.parameters, chrome::params::slotId (slot, "gain"));
                    g.setColour (juce::Colour (0xff202124));
                    g.fillRoundedRectangle (r, 5.0f * vsx);
                    g.setColour (juce::Colour (0xffeef1f5));
                    g.setFont (juce::FontOptions (21.0f * sy));
                    g.drawFittedText (juce::String (v, 1) + " dB", r.getSmallestIntegerContainer(), juce::Justification::centred, 1);
                }
            }
        }

        // Tabs: neutralise baked highlight, show active tab in skin style.
        {
            const std::array<juce::Rectangle<int>, 3> tabRects {
                designRect (*this, 130.0f, 1577.0f, 160.0f, 49.0f).getSmallestIntegerContainer(),
                designRect (*this, 309.0f, 1577.0f, 173.0f, 49.0f).getSmallestIntegerContainer(),
                designRect (*this, 499.0f, 1577.0f, 160.0f, 49.0f).getSmallestIntegerContainer() };
            const std::array<juce::TextButton*, 3> tabs { &waveTab, &spectrumTab, &modTab };
            const std::array<juce::String, 3> labels { "WAVE", "SPECTRUM", "MOD" };
            for (int i = 0; i < 3; ++i)
            {
                g.setColour (sampleSkin (tabRects[static_cast<size_t> (i)].getCentre()));
                g.fillRoundedRectangle (tabRects[static_cast<size_t> (i)].toFloat(), 8.0f * juce::jmin (sx, sy));
                if (tabs[static_cast<size_t> (i)]->getToggleState())
                {
                    auto b = tabRects[static_cast<size_t> (i)].toFloat();
                    juce::ColourGradient fill (juce::Colour (0xff526f9f), b.getX(), b.getY(),
                                               juce::Colour (0xff273b5d), b.getX(), b.getBottom(), false);
                    g.setGradientFill (fill);
                    g.fillRoundedRectangle (b, 7.0f * juce::jmin (sx, sy));
                    g.setColour (juce::Colour (0xff9fc2ff));
                    g.drawRoundedRectangle (b, 7.0f * juce::jmin (sx, sy), 1.25f * juce::jmin (sx, sy));
                }
                g.setColour (juce::Colour (0xffd8dadd));
                g.setFont (juce::FontOptions (19.5f * sy, juce::Font::bold));
                g.drawFittedText (labels[static_cast<size_t> (i)], tabRects[static_cast<size_t> (i)], juce::Justification::centred, 1);
            }
        }

        // X-FADE thumb (live) over baked track.
        {
            const auto track = designRect (*this, 730.0f, 1585.0f, 1430.0f, 46.0f);
            const auto v = chrome::params::load (processor.parameters, "morph_x");
            const int tx = static_cast<int> (track.getX() + track.getWidth() * v);
            g.setColour (juce::Colour (0xfff7f8fa));
            g.fillEllipse (static_cast<float> (tx - 8), track.getCentreY() - 14.0f, 16.0f, 28.0f);
            g.setColour (juce::Colour (0xff91b8f4));
            g.drawEllipse (static_cast<float> (tx - 8), track.getCentreY() - 14.0f, 16.0f, 28.0f, 1.5f);
        }

        // Live ADSR graph overlay (mask baked, draw from amp params).
        {
            const auto gArea = designRect (*this, 116.0f, 1023.0f, 772.0f, 158.0f);
            g.setColour (sampleSkin (gArea.getCentre().toInt()));
            g.fillRoundedRectangle (gArea, 14.0f * juce::jmin (sx, sy));
            auto eg = gArea.reduced (static_cast<int> (10.0f * juce::jmin (sx, sy)));
            const auto a = chrome::params::load (processor.parameters, "amp_attack");
            const auto d = chrome::params::load (processor.parameters, "amp_decay");
            const auto s = chrome::params::load (processor.parameters, "amp_sustain");
            const auto r = chrome::params::load (processor.parameters, "amp_release");
            const auto total = juce::jmax (0.001f, a + d + r + 0.5f);
            const float xa = a / total;
            const float xd = (a + d) / total;
            const float xs = (a + d + 0.5f) / total;
            const float sustainY = juce::jlimit (eg.getY() + 2.0f, eg.getBottom() - 2.0f,
                                                  eg.getY() + eg.getHeight() * (1.0f - s));
            juce::Path env;
            env.startNewSubPath (eg.getX(), eg.getBottom());
            env.lineTo (eg.getX() + eg.getWidth() * xa, eg.getY() + eg.getHeight() * 0.06f);
            env.lineTo (eg.getX() + eg.getWidth() * xd, sustainY);
            env.lineTo (eg.getX() + eg.getWidth() * xs, sustainY);
            env.lineTo (eg.getRight(), eg.getBottom());
            g.setColour (juce::Colour (0xff9fc1f7));
            g.strokePath (env, juce::PathStrokeType (1.5f * juce::jmin (sx, sy)));
        }

        // Live LFO graph overlay (mask baked, draw from lfo1_shape).
        {
            const auto lArea = designRect (*this, 1535.0f, 1024.0f, 774.0f, 158.0f);
            g.setColour (sampleSkin (lArea.getCentre().toInt()));
            g.fillRoundedRectangle (lArea, 9.0f * juce::jmin (sx, sy));
            const auto shape = static_cast<int> (chrome::params::load (processor.parameters, "lfo1_shape"));
            auto b = lArea.reduced (static_cast<int> (4.0f * juce::jmin (sx, sy)));
            juce::Path wave;
            for (int x = 0; x <= b.getWidth(); ++x)
            {
                const auto phase = static_cast<float> (x) / static_cast<float> (juce::jmax (1, (int) b.getWidth())) * 3.0f;
                float value = 0.0f;
                const auto unit = phase - std::floor (phase);
                if (shape == 0) value = std::sin (phase * juce::MathConstants<float>::twoPi);
                else if (shape == 1) value = 1.0f - 4.0f * std::abs (unit - 0.5f);
                else if (shape == 2) value = unit * 2.0f - 1.0f;
                else if (shape == 3) value = unit < 0.5f ? 1.0f : -1.0f;
                else value = std::sin (phase * 9.73f) * 0.75f;
                const auto y = b.getCentreY() - value * b.getHeight() * 0.36f;
                if (x == 0) wave.startNewSubPath (static_cast<float> (b.getX() + x), y);
                else wave.lineTo (static_cast<float> (b.getX() + x), y);
            }
            g.setColour (juce::Colour (0xffa4c6ff));
            g.strokePath (wave, juce::PathStrokeType (1.45f * juce::jmin (sx, sy)));
        }

        // Effect enable state (dim icon when disabled).
        {
            const std::array<juce::Rectangle<int>, 3> iconRects {
                designBounds (*this, 139.0f, 1311.0f, 26.0f, 26.0f),
                designBounds (*this, 522.0f, 1311.0f, 26.0f, 26.0f),
                designBounds (*this, 1165.0f, 1311.0f, 26.0f, 26.0f) };
            const std::array<bool, 3> on {
                chrome::params::load (processor.parameters, "chorus_enabled") > 0.5f,
                chrome::params::load (processor.parameters, "delay_enabled") > 0.5f,
                chrome::params::load (processor.parameters, "reverb_enabled") > 0.5f };
            for (int i = 0; i < 3; ++i)
                if (! on[static_cast<size_t> (i)])
                {
                    g.setColour (juce::Colour (0xaa000000));
                    g.fillEllipse (iconRects[static_cast<size_t> (i)].toFloat());
                }
        }

        // Dynamic preset name (mask baked "Init Patch", draw current).
        {
            const auto r = designRect (*this, 1024.0f, 85.0f, 360.0f, 53.0f);
            g.setColour (sampleSkin (r.getCentre().toInt()));
            g.fillRoundedRectangle (r.toFloat(), 4.0f * juce::jmin (sx, sy));
            g.setColour (juce::Colour (0xff292b2e));
            g.setFont (juce::FontOptions (16.0f * sy, juce::Font::bold));
            g.drawFittedText (processor.presets.currentName(), r.getSmallestIntegerContainer(), juce::Justification::centredLeft, 1);
        }

        if (chrome::params::load (processor.parameters, "power") < 0.5f)
        {
            g.setColour (juce::Colour (0x52080a0d));
            g.fillRoundedRectangle (designRect (*this, 45, 31, 2326, 1638),
                                    74.0f * juce::jmin (sx, sy));
        }

        // Live output meter, aligned to the vector frame.
        const auto meterArea = designRect (*this, 2234, 1328, 70, 154);
        // Mask the baked-in meter so only the live one shows (no duplication).
        g.setColour (sampleSkin (meterArea.getCentre().toInt()));
        g.fillRoundedRectangle (meterArea, 4.0f * juce::jmin (sx, sy));
        const auto level = juce::jlimit (0.0f, 1.0f, displayedPeak);
        for (int channel = 0; channel < 2; ++channel)
            for (int segment = 0; segment < 15; ++segment)
            {
                const auto segmentHeight = meterArea.getHeight() / 17.0f;
                const auto y = meterArea.getBottom() - (static_cast<float> (segment) + 1.0f) * (segmentHeight + sy);
                const auto lit = static_cast<float> (segment + 1) / 15.0f <= level;
                g.setColour (lit ? juce::Colour (0xff9fc2ff) : juce::Colour (0xff5f6266));
                g.fillRect (meterArea.getX() + static_cast<float> (channel) * 22.0f * sx,
                            y, 15.0f * sx, segmentHeight);
            }
        const std::array<std::vector<std::unique_ptr<BoundKnob>>*, 6> collections {
            &filterKnobs, &envelopeKnobs, &pitchKnobs, &effectKnobs, &masterKnobs, &modulationKnobs };
        const auto vsx = juce::jmin (sx, sy);
        g.setFont (juce::FontOptions (10.5f * vsx, juce::Font::bold));
        for (auto* collection : collections)
            for (auto& knob : *collection)
            {
                const auto b = knob->getBounds();
                const int tw = juce::jmin (b.getWidth() + 12, 118);
                const int th = 16;
                auto textRect = juce::Rectangle<int> (b.getCentreX() - tw / 2, b.getBottom() + 2, tw, th);
                if (textRect.getBottom() > getHeight()) textRect = textRect.translated (0, -th - b.getHeight() - 4);
                const auto mask = sampleSkin (textRect.getCentre());
                g.setColour (mask);
                g.fillRoundedRectangle (textRect.toFloat().expanded (2.0f, 1.0f), 4.0f * vsx);
                g.setColour (juce::Colour (0xff292b2e));
                g.drawFittedText (knob->getValueText(), textRect, juce::Justification::centred, 1);
            }
        return;
    }

    g.fillAll (juce::Colour (0xff111316));
    const auto sx = static_cast<float> (getWidth()) / designWidth;
    const auto sy = static_cast<float> (getHeight()) / designHeight;
    const auto scale = juce::jmin (sx, sy);

    auto roundedGradient = [&] (juce::Rectangle<float> bounds, float radius, juce::Colour top, juce::Colour middle, juce::Colour bottom)
    {
        juce::ColourGradient fill (top, bounds.getX(), bounds.getY(), bottom, bounds.getX(), bounds.getBottom(), false);
        fill.addColour (0.52, middle);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (bounds, radius * scale);
    };

    const auto outer = designRect (*this, 10, 0, 2404, 1728);
    juce::ColourGradient bezel (juce::Colour (0xfff8f9fa), outer.getX(), outer.getY(),
                                juce::Colour (0xff383b3f), outer.getRight(), outer.getBottom(), false);
    bezel.addColour (0.12, juce::Colour (0xff7b7e82));
    bezel.addColour (0.28, juce::Colour (0xfff6f7f8));
    bezel.addColour (0.72, juce::Colour (0xff9b9da0));
    bezel.addColour (0.90, juce::Colour (0xfff3f4f5));
    g.setGradientFill (bezel);
    g.fillRoundedRectangle (outer, 98.0f * scale);
    g.setColour (juce::Colour (0xff0e1012));
    g.drawRoundedRectangle (outer.reduced (2.0f * scale), 96.0f * scale, 5.0f * scale);
    g.setColour (juce::Colour (0xfff8f9fa));
    g.drawRoundedRectangle (outer.reduced (10.0f * scale), 90.0f * scale, 4.0f * scale);
    g.setColour (juce::Colour (0xff4d5054));
    g.drawRoundedRectangle (outer.reduced (18.0f * scale), 83.0f * scale, 5.0f * scale);
    g.setColour (juce::Colour (0xffdfe1e3));
    g.drawRoundedRectangle (outer.reduced (25.0f * scale), 77.0f * scale, 3.0f * scale);

    const auto face = designRect (*this, 45, 31, 2326, 1638);
    roundedGradient (face, 74.0f, juce::Colour (0xffdfdede), juce::Colour (0xffc4c3c3), juce::Colour (0xffaaa9a9));
    g.setColour (juce::Colour (0x99ffffff));
    g.drawRoundedRectangle (face.reduced (2.0f * scale), 72.0f * scale, 2.0f * scale);
    drawMicrograin (g, face.reduced (12.0f * scale), juce::jmax (1.0f, 3.0f * sy));

    const auto backing = designRect (*this, 73, 175, 2275, 1374);
    roundedGradient (backing, 57.0f, juce::Colour (0xffd9d8d8), juce::Colour (0xffb3b1b1), juce::Colour (0xffcfcece));
    g.setColour (juce::Colour (0x4f55575a));
    g.drawRoundedRectangle (backing, 57.0f * scale, 1.0f * scale);

    auto drawPanel = [&] (float x, float y, float width, float height, float radius = 41.0f)
    {
        const auto bounds = designRect (*this, x, y, width, height);
        g.setColour (juce::Colour (0x30000000));
        g.fillRoundedRectangle (bounds.translated (0.0f, 3.0f * sy), radius * scale);
        roundedGradient (bounds, radius, juce::Colour (0xffc4c2c1), juce::Colour (0xffaaa8a7), juce::Colour (0xffb9b7b6));
        g.setColour (juce::Colour (0x8fffffff));
        g.drawRoundedRectangle (bounds.reduced (2.0f * scale), (radius - 2.0f) * scale, 1.0f * scale);
        g.setColour (juce::Colour (0x8f626467));
        g.drawRoundedRectangle (bounds, radius * scale, 1.0f * scale);
        drawMicrograin (g, bounds.reduced (5.0f * scale), juce::jmax (1.0f, 3.0f * sy));
    };
    drawPanel (1935, 190, 416, 564);
    drawPanel (78, 769, 848, 443);
    drawPanel (928, 770, 562, 443, 31.0f);
    drawPanel (1490, 770, 858, 443, 31.0f);
    drawPanel (78, 1241, 1727, 296);
    drawPanel (1805, 1241, 543, 296);
    drawPanel (77, 1550, 2261, 96, 29.0f);
    drawPanel (121, 1296, 311, 218, 31.0f);
    drawPanel (504, 1296, 591, 218, 31.0f);
    drawPanel (1147, 1296, 590, 218, 31.0f);
    const auto footerTray = designRect (*this, 94, 1562, 2227, 69);
    juce::ColourGradient tray (juce::Colour (0xff626467), footerTray.getX(), footerTray.getY(),
                               juce::Colour (0xff34363a), footerTray.getX(), footerTray.getBottom(), false);
    g.setGradientFill (tray);
    g.fillRoundedRectangle (footerTray, 22.0f * scale);
    g.setColour (juce::Colour (0xff8b8d90));
    g.drawRoundedRectangle (footerTray, 22.0f * scale, 0.8f * scale);

    if (logoMark != nullptr)
        logoMark->drawWithin (g, designRect (*this, 95, 46, 118, 118), juce::RectanglePlacement::centred, 1.0f);
    if (wordmark != nullptr)
        wordmark->drawWithin (g, designRect (*this, 248.05f, 83, 388.55f, 33.76f), juce::RectanglePlacement::stretchToFit, 1.0f);

    const auto badge = designRect (*this, 638, 78, 63, 32);
    roundedGradient (badge, 6.0f, juce::Colour (0xffd9d8d8), juce::Colour (0xffb3b2b2), juce::Colour (0xffc9c8c8));
    g.setColour (juce::Colour (0xff3d3f42));
    g.drawRoundedRectangle (badge, 6.0f * scale, 1.6f * scale);
    g.setFont (juce::FontOptions (20.0f * sy, juce::Font::bold));
    g.drawText ("2000", badge, juce::Justification::centred);

    auto panelTitle = [&] (const juce::String& title, float x, float y, float width, bool active, bool arrow)
    {
        if (active)
        {
            g.setColour (juce::Colour (0x405e92e5));
            g.fillEllipse (designRect (*this, x, y + 2.0f, 12, 12));
            g.setColour (juce::Colour (0xff7fa8ea));
            g.fillEllipse (designRect (*this, x + 4.0f, y + 6.0f, 4, 4));
            x += 26.0f;
        }
        g.setColour (juce::Colour (0xff292b2e));
        g.setFont (juce::FontOptions (19.14f * sy, juce::Font::bold));
        g.drawText ((arrow ? juce::String (">  ") : juce::String()) + title,
                    designRect (*this, x, y, width, 25), juce::Justification::centredLeft);
    };
    panelTitle ("FILTER", 1977, 213, 160, false, true);
    panelTitle ("AMP ENVELOPE", 107, 799, 230, true, false);
    panelTitle ("PITCH", 1165, 796, 150, false, true);
    panelTitle ("MODULATION", 1522, 799, 220, true, false);
    panelTitle ("EFFECTS", 107, 1268, 170, true, false);
    panelTitle ("MASTER", 1836, 1268, 180, false, true);
    panelTitle ("CHORUS", 169, 1309, 180, false, false);
    panelTitle ("DELAY", 552, 1309, 180, false, false);
    panelTitle ("REVERB", 1195, 1309, 180, false, false);

    const auto envGraph = designRect (*this, 116, 1023, 772, 158);
    juce::ColourGradient graphFill (juce::Colour (0xff55575a), envGraph.getX(), envGraph.getY(),
                                    juce::Colour (0xff303236), envGraph.getX(), envGraph.getBottom(), false);
    g.setGradientFill (graphFill);
    g.fillRoundedRectangle (envGraph, 14.0f * scale);
    g.setColour (juce::Colour (0xff8c8f93));
    g.drawRoundedRectangle (envGraph, 14.0f * scale, 1.0f * scale);
    auto eg = envGraph.reduced (10.0f * scale);
    juce::Path envelope;
    envelope.startNewSubPath (eg.getX(), eg.getBottom());
    envelope.lineTo (eg.getX() + eg.getWidth() * 0.065f, eg.getY() + eg.getHeight() * 0.08f);
    envelope.lineTo (eg.getX() + eg.getWidth() * 0.42f, eg.getY() + eg.getHeight() * 0.17f);
    envelope.cubicTo (eg.getX() + eg.getWidth() * 0.51f, eg.getY() + eg.getHeight() * 0.55f,
                      eg.getX() + eg.getWidth() * 0.55f, eg.getY() + eg.getHeight() * 0.72f,
                      eg.getX() + eg.getWidth() * 0.62f, eg.getY() + eg.getHeight() * 0.73f);
    envelope.lineTo (eg.getX() + eg.getWidth() * 0.86f, eg.getY() + eg.getHeight() * 0.73f);
    envelope.lineTo (eg.getRight(), eg.getBottom());
    g.setColour (juce::Colour (0xff9fc1f7));
    g.strokePath (envelope, juce::PathStrokeType (1.5f * scale));

    g.setColour (juce::Colour (0xffd8dadd));
    g.setFont (juce::FontOptions (19.5f * sy, juce::Font::bold));
    g.drawText ("X-FADE", designRect (*this, 1260, 1562, 250, 24), juce::Justification::centred);
    g.drawText ("A", designRect (*this, 681, 1580, 50, 40), juce::Justification::centred);
    g.drawText ("B", designRect (*this, 2192, 1580, 50, 40), juce::Justification::centred);

    const auto meterArea = designRect (*this, 2234, 1328, 70, 154);
    const auto level = juce::jlimit (0.0f, 1.0f, displayedPeak);
    for (int channel = 0; channel < 2; ++channel)
        for (int segment = 0; segment < 15; ++segment)
        {
            const auto segmentHeight = meterArea.getHeight() / 17.0f;
            const auto y = meterArea.getBottom() - (static_cast<float> (segment) + 1.0f) * (segmentHeight + sy);
            const auto lit = static_cast<float> (segment + 1) / 15.0f <= level;
            g.setColour (lit ? juce::Colour (0xff9fc2ff) : juce::Colour (0xff6a6c70));
            g.fillRect (meterArea.getX() + static_cast<float> (channel) * 22.0f * sx, y, 15.0f * sx, segmentHeight);
        }
}

void Chrome2000AudioProcessorEditor::resized()
{
    subtitle.setBounds (designBounds (*this, 254, 129, 240, 18));
    logoHitArea.setBounds (designBounds (*this, 90, 42, 620, 126));
    previous.setBounds (designBounds (*this, 955, 85, 63, 53));
    presetBox.setBounds (designBounds (*this, 1014, 85, 422, 53));
    save.setBounds (designBounds (*this, 1432, 85, 65, 53));
    credit.setBounds (designBounds (*this, 1900, 65, 242, 82));
    power.setBounds (designBounds (*this, 2204, 51, 112, 112));

    slots[0]->setBounds (designBounds (*this, 78, 190, 617, 564));
    slots[1]->setBounds (designBounds (*this, 710, 190, 583, 564));
    slots[2]->setBounds (designBounds (*this, 1306, 190, 623, 564));

    filterKnobs[0]->setBounds (designBounds (*this, 2082.0f, 261.0f, 156.7617f, 156.7617f));
    filterKnobs[1]->setBounds (designBounds (*this, 1979.0f, 502.0f, 103.3594f, 103.3594f));
    filterKnobs[2]->setBounds (designBounds (*this, 2219.0f, 500.0f, 103.3594f, 103.3594f));
    filterType.setBounds (designBounds (*this, 1978, 685, 330, 51));

    const std::array<float, 4> envelopeX { 146.0f, 339.0f, 534.0f, 727.0f };
    for (size_t i = 0; i < envelopeKnobs.size(); ++i)
        envelopeKnobs[i]->setBounds (designBounds (*this, envelopeX[i], 850.0f, 103.3594f, 103.3594f));

    pitchKnobs[0]->setBounds (designBounds (*this, 1139.0f, 835.0f, 156.7617f, 156.7617f));
    pitchKnobs[1]->setBounds (designBounds (*this, 961.0f, 967.0f, 103.3594f, 103.3594f));
    pitchKnobs[2]->setBounds (designBounds (*this, 1168.0f, 1056.0f, 103.3594f, 103.3594f));
    pitchKnobs[3]->setBounds (designBounds (*this, 1360.0f, 966.0f, 103.3594f, 103.3594f));
    legato.setBounds (designBounds (*this, 966, 1126, 151, 43));

    modulationSource.setBounds (designBounds (*this, 1535, 846, 172, 49));
    lfoShape.setBounds (designBounds (*this, 1535, 901, 172, 68));
    modulationDestination.setBounds (designBounds (*this, 2164, 896, 165, 54));
    modulationKnobs[0]->setBounds (designBounds (*this, 1795.0f, 860.0f, 103.3594f, 103.3594f));
    modulationKnobs[1]->setBounds (designBounds (*this, 1971.0f, 860.0f, 103.3594f, 103.3594f));
    modulationKnobs[2]->setBounds (designBounds (*this, 1795.0f, 975.0f, 80.0f, 80.0f));
    modulationKnobs[3]->setBounds (designBounds (*this, 1890.0f, 975.0f, 80.0f, 80.0f));
    modulationKnobs[4]->setBounds (designBounds (*this, 1795.0f, 1070.0f, 80.0f, 80.0f));
    modulationKnobs[5]->setBounds (designBounds (*this, 1890.0f, 1070.0f, 80.0f, 80.0f));

    lfo2Shape.setBounds (designBounds (*this, 1535.0f, 975.0f, 172.0f, 49.0f));
    lfo3Shape.setBounds (designBounds (*this, 1535.0f, 1070.0f, 172.0f, 49.0f));

    filterKnobs[3]->setBounds (designBounds (*this, 2082.0f, 400.0f, 90.0f, 90.0f));
    filterKnobs[4]->setBounds (designBounds (*this, 2190.0f, 400.0f, 90.0f, 90.0f));
    lfoDisplay.setBounds (designBounds (*this, 1535, 1024, 774, 158));

    const std::array<float, 7> effectX { 225.0f, 566.0f, 755.0f, 943.0f, 1203.0f, 1392.0f, 1580.0f };
    const std::array<float, 7> effectY { 1341.0f, 1338.0f, 1333.0f, 1333.0f, 1340.0f, 1335.0f, 1335.0f };
    for (size_t i = 0; i < effectKnobs.size(); ++i)
        effectKnobs[i]->setBounds (designBounds (*this, effectX[i], effectY[i], 103.3594f, 103.3594f));
    masterKnobs[0]->setBounds (designBounds (*this, 1855.2539f, 1312.9238f, 126.3281f, 126.3281f));
    masterKnobs[1]->setBounds (designBounds (*this, 2067.0156f, 1312.9238f, 126.3281f, 126.3281f));

    chorusToggle.setBounds (designBounds (*this, 139.0f, 1311.0f, 26.0f, 26.0f));
    delayToggle.setBounds (designBounds (*this, 522.0f, 1311.0f, 26.0f, 26.0f));
    reverbToggle.setBounds (designBounds (*this, 1165.0f, 1311.0f, 26.0f, 26.0f));

    waveTab.setBounds (designBounds (*this, 130, 1577, 160, 49));
    spectrumTab.setBounds (designBounds (*this, 309, 1577, 173, 49));
    modTab.setBounds (designBounds (*this, 499, 1577, 160, 49));
    morphX.setBounds (designBounds (*this, 730, 1585, 1430, 46));
    aboutOverlay.setBounds (getLocalBounds());
}

void Chrome2000AudioProcessorEditor::timerCallback()
{
    displayedPeak = juce::jmax (processor.engine.outputPeak(), displayedPeak * 0.9f);
    if (valueBubble.isVisible() && activeDragKnob != nullptr)
    {
        valueBubble.setText (activeDragKnob->getValueText());
        auto b = activeDragKnob->getBounds();
        const int bw = 96, bh = 22;
        int x = b.getCentreX() - bw / 2;
        int y = b.getY() - bh - 6;
        if (y < 0) y = b.getBottom() + 6;
        x = juce::jlimit (0, getWidth() - bw, x);
        valueBubble.setBounds (x, y, bw, bh);
    }
    repaint();
}

void Chrome2000AudioProcessorEditor::rebuildPresetList()
{
    presetBox.clear();
    presetBox.addItemList (processor.presets.names(), 1);
    presetBox.setText (processor.presets.currentName(), juce::dontSendNotification);
}

void Chrome2000AudioProcessorEditor::savePreset()
{
    auto window = std::make_unique<juce::AlertWindow> ("Save preset", "Enter a name for this sound.", juce::MessageBoxIconType::NoIcon);
    window->addTextEditor ("name", processor.presets.currentName());
    window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    const auto safe = juce::Component::SafePointer<Chrome2000AudioProcessorEditor> (this);
    auto* raw = window.release();
    raw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, raw] (int result)
    {
        std::unique_ptr<juce::AlertWindow> owner (raw);
        if (safe != nullptr && result == 1 && safe->processor.presets.save (raw->getTextEditorContents ("name")))
            safe->rebuildPresetList();
    }), false);
}

void Chrome2000AudioProcessorEditor::showAbout()
{
    aboutOverlay.setVisible (true);
    aboutOverlay.toFront (true);
}

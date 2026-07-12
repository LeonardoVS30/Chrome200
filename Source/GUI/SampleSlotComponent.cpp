#include "SampleSlotComponent.h"
#include "../SampleEngine/SampleEngine.h"
#include "../Parameters/ParameterLayout.h"

namespace chrome
{
SampleSlotComponent::SampleSlotComponent (juce::AudioProcessorValueTreeState& parameters, SampleEngine& sampleEngine, int slot)
    : state (parameters), engine (sampleEngine), slotIndex (slot), waveform (parameters, slot)
{
    title.setText ("SAMPLE SLOT " + juce::String (slot + 1), juce::dontSendNotification);
    title.setFont (juce::FontOptions (12.0f, juce::Font::bold)); title.setColour (juce::Label::textColourId, juce::Colour (0xff303438));
    fileName.setText ("No sample loaded", juce::dontSendNotification); fileName.setFont (juce::FontOptions (11.0f));
    fileName.setColour (juce::Label::textColourId, juce::Colour (0xff35393d));
    for (auto* component : std::array<juce::Component*, 16> { &title, &fileName, &waveform, &browse, &loop, &reverse, &solo, &mute, &power, &mode, &gain, &tune, &fine, &start, &end, &pan }) addAndMakeVisible (component);
    mode.addItemList ({ "VINTAGE", "CHROME", "DIGITAL" }, 1);
    modeAttachment = std::make_unique<ComboAttachment> (state, params::slotId (slot, "mode"), mode);
    configureKnob (gain, "gain", "GAIN"); configureKnob (tune, "tune", "TUNE"); configureKnob (fine, "fine", "FINE");
    configureKnob (start, "start", "START"); configureKnob (end, "end", "END"); configureKnob (pan, "pan", "PAN");
    tune.setVisible (false); fine.setVisible (false); pan.setVisible (false);
    for (auto* button : { &loop, &reverse, &solo, &mute, &power }) button->setClickingTogglesState (true);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "loop"), loop));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "reverse"), reverse));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "solo"), solo));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "mute"), mute));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "enabled"), power));
    browse.onClick = [this] { chooseFile(); };
    setWantsKeyboardFocus (false);
}

void SampleSlotComponent::configureKnob (juce::Slider& slider, const juce::String& suffix, const juce::String& tooltip)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 16);
    slider.setTooltip (tooltip);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (state, params::slotId (slotIndex, suffix), slider));
}

void SampleSlotComponent::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (juce::Colour (0x26000000)); g.fillRoundedRectangle (b.translated (0.0f, 2.0f), 16.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeeeeef), b.getX(), b.getY(), juce::Colour (0xffd5d5d7), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 18.4f);
    g.setColour (juce::Colour (0x66323437)); g.drawRoundedRectangle (b, 18.4f, 1.0f);
    g.setColour (juce::Colour (0x66ffffff)); g.drawRoundedRectangle (b.reduced (2.0f), 16.4f, 1.0f);
    const std::array<juce::Colour, 3> accents { juce::Colour (0xff70a7ff), juce::Colour (0xff87bcff), juce::Colour (0xff789fea) };
    g.setColour (accents[static_cast<size_t> (slotIndex)].withAlpha (0.35f)); g.fillEllipse (13.0f, 13.0f, 13.0f, 13.0f);
    g.setColour (accents[static_cast<size_t> (slotIndex)]); g.fillEllipse (17.0f, 17.0f, 5.0f, 5.0f);
    const auto fileField = fileName.getBounds().toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeeeeef), fileField.getX(), fileField.getY(), juce::Colour (0xffc5c7c9), fileField.getX(), fileField.getBottom(), false));
    g.fillRoundedRectangle (fileField, 7.0f); g.setColour (juce::Colour (0xff9a9da0)); g.drawRoundedRectangle (fileField, 7.0f, 0.8f);
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold)); g.setColour (juce::Colour (0xff3f4347));
    for (auto item : std::array<std::pair<juce::Slider*, const char*>, 3> {{{ &start, "START" }, { &end, "END" }, { &gain, "GAIN" }}})
    {
        const auto r = item.first->getBounds();
        g.drawText (item.second, r.getX(), r.getY() - 12, r.getWidth(), 12, juce::Justification::centred);
    }
    if (const auto loaded = engine.sample (slotIndex))
    {
        const auto metadata = juce::String (loaded->sampleRate() / 1000.0, 1) + " kHz  /  " + juce::String (loaded->channels() == 1 ? "Mono" : "Stereo");
        const auto y = waveform.getBottom() + 1; g.setFont (juce::FontOptions (9.0f)); g.setColour (juce::Colour (0xff55595d));
        g.drawText (metadata, waveform.getX() + 3, y, waveform.getWidth(), 13, juce::Justification::centredLeft);
    }
}

void SampleSlotComponent::resized()
{
    auto area = getLocalBounds().reduced (14);
    title.setBounds (area.removeFromTop (24));
    auto fileRow = area.removeFromTop (28); browse.setBounds (fileRow.removeFromRight (52).reduced (0, 2)); mode.setBounds (fileRow.removeFromRight (86).reduced (2)); fileName.setBounds (fileRow.reduced (0, 2));
    auto waveRow = area.removeFromTop (70); auto switches = waveRow.removeFromRight (25); solo.setBounds (switches.removeFromTop (switches.getHeight() / 2).reduced (1)); mute.setBounds (switches.reduced (1)); waveform.setBounds (waveRow.reduced (0, 3));
    area.removeFromTop (14);
    auto buttons = area.removeFromTop (27); reverse.setBounds (buttons.removeFromLeft (58).reduced (0, 2)); loop.setBounds (buttons.removeFromLeft (52).reduced (0, 2)); power.setBounds (buttons.removeFromRight (38).reduced (0, 2));
    area.removeFromTop (8);
    const auto primaryWidth = area.getWidth() / 3;
    auto startCell = area.removeFromLeft (primaryWidth); auto endCell = area.removeFromLeft (primaryWidth); auto gainCell = area;
    start.setBounds (startCell.withTrimmedTop (12).reduced (5, 4)); end.setBounds (endCell.withTrimmedTop (12).reduced (5, 4)); gain.setBounds (gainCell.withTrimmedTop (12));
}

bool SampleSlotComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    if (files.isEmpty()) return false;
    const auto extension = juce::File (files[0]).getFileExtension().toLowerCase();
    return extension == ".wav" || extension == ".aiff" || extension == ".aif" || extension == ".flac";
}

void SampleSlotComponent::filesDropped (const juce::StringArray& files, int, int)
{
    if (isInterestedInFileDrag (files)) engine.loadSample (slotIndex, juce::File (files[0]));
}

void SampleSlotComponent::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a sample", juce::File(), "*.wav;*.aiff;*.aif;*.flac");
    const auto safe = juce::Component::SafePointer<SampleSlotComponent> (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc) { if (safe != nullptr && fc.getResult().existsAsFile()) safe->engine.loadSample (safe->slotIndex, fc.getResult()); });
}

void SampleSlotComponent::refresh()
{
    const auto loaded = engine.sample (slotIndex);
    waveform.setSample (loaded);
    fileName.setText (loaded ? loaded->file().getFileName() : "No sample loaded", juce::dontSendNotification);
}
}

#include "SampleSlotComponent.h"
#include "../SampleEngine/SampleEngine.h"
#include "../Parameters/ParameterLayout.h"

namespace chrome
{
SampleSlotComponent::SampleSlotComponent (juce::AudioProcessorValueTreeState& parameters, SampleEngine& sampleEngine, int slot)
    : state (parameters), engine (sampleEngine), slotIndex (slot), waveform (parameters, slot)
{
    title.setText ("SAMPLE SLOT " + juce::String (slot + 1), juce::dontSendNotification);
    title.setFont (juce::FontOptions (9.6f, juce::Font::bold)); title.setColour (juce::Label::textColourId, juce::Colour (0xff292b2e));
    fileName.setText ("No sample loaded", juce::dontSendNotification); fileName.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    fileName.setColour (juce::Label::textColourId, juce::Colour (0xff292b2e));
    fileName.setJustificationType (juce::Justification::centredLeft);
    for (auto* component : std::array<juce::Component*, 16> { &title, &fileName, &waveform, &browse, &loop, &reverse, &solo, &mute, &power, &mode, &gain, &tune, &fine, &start, &end, &pan }) addAndMakeVisible (component);
    mode.addItemList ({ "Ethernal", "Chrome", "Digital" }, 1);
    mode.getProperties().set ("characterMode", true);
    mode.setColour (juce::ComboBox::textColourId, juce::Colour (0xff292b2e));
    mode.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff292b2e));
    modeAttachment = std::make_unique<ComboAttachment> (state, params::slotId (slot, "mode"), mode);
    configureKnob (gain, "gain", "GAIN"); configureKnob (tune, "tune", "TUNE"); configureKnob (fine, "fine", "FINE");
    configureKnob (start, "start", "START"); configureKnob (end, "end", "END"); configureKnob (pan, "pan", "PAN");
    tune.setVisible (false); fine.setVisible (false); pan.setVisible (false);
    browse.setButtonText ({});
    browse.getProperties().set ("logoHitArea", true);
    browse.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    reverse.setButtonText ("REVERSE");
    gain.getProperties().set ("bigKnob", true);
    gain.getProperties().set ("glowKnob", true);
    for (auto* button : { &loop, &reverse, &solo, &mute, &power }) button->setClickingTogglesState (true);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "loop"), loop));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "reverse"), reverse));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "solo"), solo));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "mute"), mute));
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (state, params::slotId (slot, "enabled"), power));
    for (auto* button : { &loop, &reverse, &solo, &mute })
        button->onStateChange = [this] { repaint(); };
    browse.onClick = [this] { chooseFile(); };
    setWantsKeyboardFocus (false);
}

void SampleSlotComponent::configureKnob (juce::Slider& slider, const juce::String& suffix, const juce::String& tooltip)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setName (tooltip);
    auto updateTooltip = [&slider, suffix, tooltip]
    {
        juce::String value;
        if (suffix == "gain")
            value = juce::String (slider.getValue(), 1) + " dB";
        else if (suffix == "start" || suffix == "end")
            value = juce::String (slider.getValue() * 100.0, 2) + " %";
        else
            value = juce::String (slider.getValue(), 2);
        slider.setTooltip (tooltip + ": " + value);
    };
    updateTooltip();
    auto* sliderPointer = &slider;
    slider.onValueChange = [this, sliderPointer, suffix, tooltip]
    {
        juce::String value;
        if (suffix == "gain")
            value = juce::String (sliderPointer->getValue(), 1) + " dB";
        else if (suffix == "start" || suffix == "end")
            value = juce::String (sliderPointer->getValue() * 100.0, 2) + " %";
        else
            value = juce::String (sliderPointer->getValue(), 2);
        sliderPointer->setTooltip (tooltip + ": " + value);
        repaint();
    };
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (state, params::slotId (slotIndex, suffix), slider));
}

void SampleSlotComponent::paint (juce::Graphics& g)
{
    if (figmaSkinMode)
    {
        constexpr std::array<float, 3> slotDesignWidth { 617.0f, 583.0f, 623.0f };
        const auto index = static_cast<size_t> (slotIndex);
        const auto sx = static_cast<float> (getWidth()) / slotDesignWidth[index];
        const auto sy = static_cast<float> (getHeight()) / 564.0f;
        const auto scale = juce::jmin (sx, sy);
        const auto slotRect = [sx, sy] (float x, float y, float width, float height)
        {
            return juce::Rectangle<float> (x * sx, y * sy, width * sx, height * sy);
        };
        const auto drawStateButton = [&] (juce::Rectangle<float> bounds,
                                          const juce::String& text,
                                          bool active,
                                          float radius)
        {
            if (active)
            {
                g.setColour (juce::Colour (0x405f91e8));
                g.fillRoundedRectangle (bounds, radius * scale);
            }
            juce::ColourGradient fill (active ? juce::Colour (0xffe2e5ea) : juce::Colour (0xffd1d1cf),
                                       bounds.getX(), bounds.getY(),
                                       active ? juce::Colour (0xff8c9bb2) : juce::Colour (0xff9a9b98),
                                       bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill (fill);
            g.fillRoundedRectangle (bounds, radius * scale);
            g.setColour (active ? juce::Colour (0xff82adf4) : juce::Colour (0xff7e8082));
            g.drawRoundedRectangle (bounds, radius * scale, (active ? 1.6f : 0.9f) * scale);
            g.setColour (juce::Colour (0xfff2f3f4));
            g.setFont (juce::FontOptions (18.0f * sy));
            g.drawText (text, bounds, juce::Justification::centred);
        };

        constexpr std::array<float, 3> reverseX { 24.5093f, 30.1499f, 30.5093f };
        constexpr std::array<float, 3> loopX { 203.9760f, 209.6166f, 209.9760f };
        drawStateButton (slotRect (reverseX[index], 496.5039f, 147.8613f, 40.6738f),
                         "REVERSE", reverse.getToggleState(), 9.0f);
        drawStateButton (slotRect (loopX[index], 497.5039f, 134.0f, 38.0f),
                         "LOOP", loop.getToggleState(), 9.0f);

        const auto drawSmallState = [&] (juce::Rectangle<float> bounds, const juce::String& text, bool active)
        {
            g.setColour (active ? juce::Colour (0xff314666) : juce::Colour (0xff26282b));
            g.fillRoundedRectangle (bounds, 6.0f * scale);
            g.setColour (active ? juce::Colour (0xff91b8ff) : juce::Colour (0xff696c70));
            g.drawRoundedRectangle (bounds, 6.0f * scale, 1.0f * scale);
            g.setColour (juce::Colour (0xffeef0f2));
            g.setFont (juce::FontOptions (20.0f * sy, juce::Font::bold));
            g.drawText (text, bounds, juce::Justification::centred);
        };
        constexpr std::array<float, 3> smallStateX { 541.0f, 502.0f, 538.0f };
        constexpr std::array<float, 3> smallStateY { 132.0f, 135.0f, 134.0f };
        drawSmallState (slotRect (smallStateX[index], smallStateY[index], 40.5f, 41.5f),
                        "S", solo.getToggleState());
        drawSmallState (slotRect (smallStateX[index], smallStateY[index] + 50.0f, 40.5f, 41.5f),
                        "M", mute.getToggleState());
        if (params::load (state, params::slotId (slotIndex, "enabled")) < 0.5f)
        {
            g.setColour (juce::Colour (0xaa000000));
            g.fillEllipse (slotRect (24.0f, 20.0f, 30.0f, 30.0f));
        }
        return;
    }

    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    const auto radius = getHeight() * (43.0f / 564.0f);
    g.setColour (juce::Colour (0x26000000)); g.fillRoundedRectangle (b.translated (0.0f, 1.5f), radius);
    juce::ColourGradient panel (juce::Colour (0xffc4c2c1), b.getX(), b.getY(), juce::Colour (0xffb8b6b5), b.getX(), b.getBottom(), false);
    panel.addColour (0.53, juce::Colour (0xffa8a6a5));
    g.setGradientFill (panel); g.fillRoundedRectangle (b, radius);
    g.setColour (juce::Colour (0x7fffffff)); g.drawRoundedRectangle (b.reduced (1.0f), radius - 1.0f, 0.8f);
    g.setColour (juce::Colour (0x7f5a5b5d)); g.drawRoundedRectangle (b, radius, 0.8f);
    g.setColour (juce::Colour (0x09ffffff));
    for (int y = 4; y < getHeight() - 4; y += 2) g.drawHorizontalLine (y, 5.0f, static_cast<float> (getWidth() - 5));
    const std::array<juce::Colour, 3> accents { juce::Colour (0xff70a7ff), juce::Colour (0xff87bcff), juce::Colour (0xff789fea) };
    const auto sx = static_cast<float> (getWidth()) / 617.0f, sy = static_cast<float> (getHeight()) / 564.0f;
    g.setColour (accents[static_cast<size_t> (slotIndex)].withAlpha (0.24f)); g.fillEllipse (25.0f * sx, 23.0f * sy, 22.0f * sx, 22.0f * sy);
    g.setColour (accents[static_cast<size_t> (slotIndex)]); g.fillEllipse (29.0f * sx, 27.0f * sy, 14.0f * sx, 14.0f * sy);
    const auto fileField = fileName.getBounds().toFloat().reduced (0.5f);
    g.setColour (juce::Colour (0x26000000)); g.fillRoundedRectangle (fileField.translated (0.0f, 1.2f), 7.0f * sx);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffdfdede), fileField.getX(), fileField.getY(), juce::Colour (0xffb4b3b3), fileField.getX(), fileField.getBottom(), false));
    g.fillRoundedRectangle (fileField, 7.0f * sx); g.setColour (juce::Colour (0xff898a8c)); g.drawRoundedRectangle (fileField, 7.0f * sx, 0.7f);
    g.setColour (juce::Colour (0xff45474a));
    const auto iconX = fileField.getX() + 17.0f * sx, iconY = fileField.getCentreY() - 7.0f * sy;
    g.drawRect (juce::Rectangle<float> (iconX, iconY, 14.0f * sx, 14.0f * sy), 1.1f);
    g.fillRect (iconX + 3.0f * sx, iconY + 1.5f * sy, 7.0f * sx, 3.0f * sy);
    juce::Path downArrow; downArrow.addTriangle (fileField.getRight() - 24.0f * sx, fileField.getCentreY() - 3.0f * sy,
                                                  fileField.getRight() - 12.0f * sx, fileField.getCentreY() - 3.0f * sy,
                                                  fileField.getRight() - 18.0f * sx, fileField.getCentreY() + 4.0f * sy);
    g.fillPath (downArrow);

    auto text = [&] (const juce::String& value, float x, float y, float w, float h, float fontHeight, bool bold = false)
    {
        g.setColour (juce::Colour (0xff292b2e));
        g.setFont (juce::FontOptions (fontHeight * sy, bold ? juce::Font::bold : juce::Font::plain));
        g.drawText (value, juce::Rectangle<float> (x * sx, y * sy, w * sx, h * sy), juce::Justification::centred);
    };
    text ("START", 23.0f, 302.0f, 140.0f, 26.0f, 20.5f, true);
    text ("END", 193.0f, 302.0f, 140.0f, 26.0f, 20.5f, true);
    text ("GAIN", 416.0f, 276.0f, 140.0f, 26.0f, 20.5f, true);
    text (juce::String (start.getValue() * 100.0, 2) + " %", 23.0f, 434.0f, 140.0f, 27.0f, 21.5f);
    text (juce::String (end.getValue() * 100.0, 2) + " %", 193.0f, 434.0f, 140.0f, 27.0f, 21.5f);
    const auto gainField = juce::Rectangle<float> (423.8f * sx, 463.9f * sy, 129.0f * sx, 33.0f * sy);
    g.setColour (juce::Colour (0xff202124)); g.fillRoundedRectangle (gainField, 5.0f * sx);
    g.setColour (juce::Colour (0xffeef1f5)); g.setFont (juce::FontOptions (21.0f * sy));
    g.drawText (juce::String (gain.getValue(), 1) + " dB", gainField, juce::Justification::centred);
    if (const auto loaded = engine.sample (slotIndex))
    {
        const auto metadata = juce::String (loaded->sampleRate() / 1000.0, 1) + " kHz / 24 bit / " + juce::String (loaded->channels() == 1 ? "Mono" : "Stereo");
        g.setFont (juce::FontOptions (21.25f * sy)); g.setColour (juce::Colour (0xff4e5053));
        g.drawText (metadata, juce::Rectangle<float> (37.2f * sx, 253.5f * sy, 290.0f * sx, 28.0f * sy), juce::Justification::centredLeft);
    }
}

void SampleSlotComponent::setFigmaSkinMode (bool enabled)
{
    figmaSkinMode = enabled;
    constexpr float hitTargetAlpha = 0.001f;
    for (auto* component : { static_cast<juce::Component*> (&title),
                             static_cast<juce::Component*> (&fileName),
                             static_cast<juce::Component*> (&waveform),
                              static_cast<juce::Component*> (&browse),
                              static_cast<juce::Component*> (&loop),
                              static_cast<juce::Component*> (&reverse),
                              static_cast<juce::Component*> (&solo),
                              static_cast<juce::Component*> (&mute),
                              static_cast<juce::Component*> (&power) })
        component->setAlpha (enabled ? hitTargetAlpha : 1.0f);
    repaint();
}

void SampleSlotComponent::resized()
{
    const auto sx = static_cast<float> (getWidth()) / 617.0f, sy = static_cast<float> (getHeight()) / 564.0f;
    auto r = [sx, sy] (float x, float y, float w, float h)
    {
        return juce::Rectangle<float> (x * sx, y * sy, w * sx, h * sy).getSmallestIntegerContainer();
    };
    title.setBounds (r (62.9f, 25.7f, 220.0f, 23.0f));
    constexpr std::array<float, 3> slotDesignWidth { 617.0f, 583.0f, 623.0f };
    constexpr std::array<float, 3> fileX { 28.0f, 27.0f, 26.0f };
    constexpr std::array<float, 3> fileY { 58.0f, 58.0f, 62.0f };
    constexpr std::array<float, 3> fileW { 407.0f, 361.0f, 407.0f };
    constexpr std::array<float, 3> modeX { 440.0f, 403.0f, 439.0f };
    constexpr std::array<float, 3> modeY { 58.0f, 60.0f, 63.0f };
    const auto index = static_cast<size_t> (slotIndex);
    const auto slotScale = static_cast<float> (getWidth()) / slotDesignWidth[index];
    const auto slotRect = [slotScale, sy] (float x, float y, float w, float h)
    {
        return juce::Rectangle<float> (x * slotScale, y * sy, w * slotScale, h * sy)
            .getSmallestIntegerContainer();
    };
    const auto fileBounds = slotRect (fileX[index], fileY[index], fileW[index], 51.0f);
    fileName.setBounds (fileBounds.reduced (static_cast<int> (54.0f * sx), 0).withRight (fileBounds.getRight() - static_cast<int> (34.0f * sx)));
    browse.setBounds (fileBounds);
    mode.setBounds (slotRect (modeX[index], modeY[index], 146.0f, 47.0f));
    constexpr std::array<float, 3> waveformX { 27.0f, 25.5f, 27.3f };
    constexpr std::array<float, 3> waveformW { 560.0f, 529.1f, 565.4f };
    waveform.setBounds (slotRect (waveformX[index], 119.0f, waveformW[index], 122.0f));
    constexpr std::array<float, 3> smallStateX { 541.0f, 502.0f, 538.0f };
    constexpr std::array<float, 3> smallStateY { 132.0f, 135.0f, 134.0f };
    solo.setBounds (slotRect (smallStateX[index], smallStateY[index], 40.5f, 41.5f));
    mute.setBounds (slotRect (smallStateX[index], smallStateY[index] + 50.0f, 40.5f, 41.5f));
    power.setBounds (slotRect (20.0f, 18.0f, 44.0f, 44.0f));
    constexpr std::array<float, 3> startX { 37.5488f, 37.1895f, 46.0f };
    constexpr std::array<float, 3> startY { 328.6094f, 328.6094f, 329.0f };
    constexpr std::array<float, 3> endX { 211.0f, 207.0625f, 217.0f };
    constexpr std::array<float, 3> endY { 329.0f, 328.6094f, 329.0f };
    constexpr std::array<float, 3> gainX { 408.4453f, 392.0f, 419.0f };
    constexpr std::array<float, 3> gainY { 301.4082f, 309.0f, 309.0f };
    start.setBounds (slotRect (startX[index], startY[index], 103.3594f, 103.3594f));
    end.setBounds (slotRect (endX[index], endY[index], 103.3594f, 103.3594f));
    gain.setBounds (slotRect (gainX[index], gainY[index], 156.7617f, 156.7617f));
    constexpr std::array<float, 3> reverseX { 24.5093f, 30.1499f, 30.5093f };
    constexpr std::array<float, 3> loopX { 203.9760f, 209.6166f, 209.9760f };
    reverse.setBounds (slotRect (reverseX[index], 496.5039f, 147.8613f, 40.6738f));
    loop.setBounds (slotRect (loopX[index], 497.5039f, 134.0f, 38.0f));
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

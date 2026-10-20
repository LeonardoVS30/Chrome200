#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    Chrome2000AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, 44100.0, 512);
    processor.prepareToPlay (44100.0, 512);

    auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());
    editor->setSize (1225, 874);
    editor->resized();
    editor->repaint();

    const std::array<juce::Point<int>, 4> interactivePoints {
        juce::Point<int> { 250, 50 },   // CHROME logo / about
        juce::Point<int> { 1130, 50 },  // power
        juce::Point<int> { 100, 132 },  // sample browser
        juce::Point<int> { 1077, 172 }  // filter cutoff
    };
    for (const auto point : interactivePoints)
    {
        if (editor->getComponentAt (point) == editor.get())
        {
            std::cerr << "FAIL: Figma hit target missing at "
                      << point.x << "," << point.y << '\n';
            return 2;
        }
    }

    auto* sampleSlot = editor->findChildWithID ("sampleSlot1");
    auto* rootEditor = sampleSlot != nullptr
                         ? dynamic_cast<juce::Button*> (sampleSlot->findChildWithID ("s1_root_note_editor"))
                         : nullptr;
    if (rootEditor == nullptr || rootEditor->getWidth() < 50 || rootEditor->getHeight() < 10)
    {
        std::cerr << "FAIL: sample root note has no usable editor target\n";
        return 17;
    }

    auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    const auto destination = juce::File ("C:\\tmp\\chrome2000-current-snapshot.png");
    destination.deleteFile();
    auto stream = destination.createOutputStream();
    juce::PNGImageFormat png;
    if (stream == nullptr || ! png.writeImageToStream (snapshot, *stream))
    {
        std::cerr << "FAIL: could not write editor snapshot\n";
        return 1;
    }

    for (const auto* id : { "chorusToggle", "delayToggle", "reverbToggle" })
    {
        auto* effectToggle = dynamic_cast<juce::Button*> (editor->findChildWithID (id));
        if (effectToggle == nullptr || effectToggle->getWidth() < 100)
        {
            std::cerr << "FAIL: effect title is not a usable bypass target: " << id << '\n';
            return 8;
        }
    }

    auto* delaySync = dynamic_cast<juce::Button*> (editor->findChildWithID ("delaySync"));
    auto* delayDivision = dynamic_cast<juce::ComboBox*> (editor->findChildWithID ("delayDivision"));
    auto* delayTime = editor->findChildWithID ("delay_time");
    auto* delaySyncParameter = processor.parameters.getParameter ("delay_sync");
    if (delaySync == nullptr || delayDivision == nullptr || delayTime == nullptr || delaySyncParameter == nullptr)
    {
        std::cerr << "FAIL: delay mode controls are missing\n";
        return 12;
    }
    delaySyncParameter->setValueNotifyingHost (1.0f);
    delaySync->onStateChange();
    if (! delayTime->isEnabled() || ! delayDivision->isEnabled())
    {
        std::cerr << "FAIL: delay sync did not switch time controls\n";
        return 13;
    }
    auto* delayTimeSlider = dynamic_cast<juce::Slider*> (delayTime->getChildComponent (2));
    if (delayTimeSlider == nullptr)
    {
        std::cerr << "FAIL: delay TIME knob has no slider\n";
        return 14;
    }
    delayTimeSlider->setValue (7.0, juce::sendNotificationSync);
    if (std::abs (chrome::params::load (processor.parameters, "delay_division") - 7.0f) > 0.01f)
    {
        std::cerr << "FAIL: delay TIME knob does not control divisions in sync mode\n";
        return 15;
    }
    delaySyncParameter->setValueNotifyingHost (0.0f);
    delaySync->onStateChange();
    delayTimeSlider->setValue (0.625, juce::sendNotificationSync);
    if (std::abs (chrome::params::load (processor.parameters, "delay_time") - 0.625f) > 0.001f)
    {
        std::cerr << "FAIL: delay TIME knob did not return to milliseconds mode\n";
        return 16;
    }

    auto* powerButton = dynamic_cast<juce::Button*> (editor->findChildWithID ("powerButton"));
    auto* cutoffKnob = editor->findChildWithID ("filter_cutoff");
    auto* powerParameter = processor.parameters.getParameter ("power");
    if (powerButton == nullptr || cutoffKnob == nullptr || powerParameter == nullptr)
    {
        std::cerr << "FAIL: power-lock controls are missing\n";
        return 9;
    }
    powerParameter->setValueNotifyingHost (0.0f);
    powerButton->onStateChange();
    if (cutoffKnob->isEnabled() || ! powerButton->isEnabled())
    {
        std::cerr << "FAIL: power off did not lock editing controls\n";
        return 10;
    }
    powerParameter->setValueNotifyingHost (1.0f);
    powerButton->onStateChange();
    if (! cutoffKnob->isEnabled())
    {
        std::cerr << "FAIL: power on did not restore editing controls\n";
        return 11;
    }

    auto* logoTarget = editor->findChildWithID ("chromeLogoHitArea");
    if (auto* logoButton = dynamic_cast<juce::Button*> (logoTarget))
        logoButton->onClick();
    else
    {
        std::cerr << "FAIL: CHROME logo is not an actionable button\n";
        return 3;
    }

    auto* modulationSource = dynamic_cast<juce::ComboBox*> (editor->findChildWithID ("modulationSource"));
    auto* lfo1Shape = editor->findChildWithID ("lfo1Shape");
    auto* lfo2Shape = editor->findChildWithID ("lfo2Shape");
    auto* lfo3Shape = editor->findChildWithID ("lfo3Shape");
    if (modulationSource == nullptr || lfo1Shape == nullptr || lfo2Shape == nullptr || lfo3Shape == nullptr)
    {
        std::cerr << "FAIL: contextual LFO controls are missing\n";
        return 5;
    }
    modulationSource->setSelectedItemIndex (2, juce::sendNotificationSync);
    if (lfo1Shape->isVisible() || ! lfo2Shape->isVisible() || lfo3Shape->isVisible())
    {
        std::cerr << "FAIL: selecting LFO 2 did not switch its controls\n";
        return 6;
    }
    modulationSource->setSelectedItemIndex (3, juce::sendNotificationSync);
    if (lfo1Shape->isVisible() || lfo2Shape->isVisible() || ! lfo3Shape->isVisible())
    {
        std::cerr << "FAIL: selecting LFO 3 did not switch its controls\n";
        return 7;
    }

    const auto aboutDestination = juce::File ("C:\\tmp\\chrome2000-about-snapshot.png");
    aboutDestination.deleteFile();
    auto aboutStream = aboutDestination.createOutputStream();
    auto aboutSnapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    if (aboutStream == nullptr || ! png.writeImageToStream (aboutSnapshot, *aboutStream))
    {
        std::cerr << "FAIL: could not write about snapshot\n";
        return 4;
    }

    std::cout << "Snapshot=" << destination.getFullPathName()
              << " Width=" << snapshot.getWidth()
              << " Height=" << snapshot.getHeight()
              << " AboutSnapshot=" << aboutDestination.getFullPathName() << '\n';
    return 0;
}

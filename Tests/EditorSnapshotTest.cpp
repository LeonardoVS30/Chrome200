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

    auto* logoTarget = editor->findChildWithID ("chromeLogoHitArea");
    if (auto* logoButton = dynamic_cast<juce::Button*> (logoTarget))
        logoButton->onClick();
    else
    {
        std::cerr << "FAIL: CHROME logo is not an actionable button\n";
        return 3;
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

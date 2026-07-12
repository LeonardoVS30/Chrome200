#pragma once

#include "SampleData.h"
#include <array>

namespace chrome
{
class SampleLoader final : private juce::Thread
{
public:
    using Completion = std::function<void (int, std::shared_ptr<const SampleData>, juce::String)>;

    SampleLoader (juce::AudioFormatManager&, Completion);
    ~SampleLoader() override;
    void load (int slot, const juce::File&);

private:
    void run() override;
    struct Request { int slot = 0; juce::File file; };
    juce::AudioFormatManager& formats;
    Completion completion;
    juce::CriticalSection queueLock;
    std::vector<Request> queue;
};
}


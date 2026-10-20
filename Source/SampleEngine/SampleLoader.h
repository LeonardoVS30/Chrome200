#pragma once

#include "SampleData.h"
#include <array>

namespace chrome
{
class SampleLoader final : private juce::Thread
{
public:
    using Completion = std::function<void (int, std::shared_ptr<const SampleData>, juce::String, bool, uint64_t)>;

    SampleLoader (juce::AudioFormatManager&, Completion);
    ~SampleLoader() override;
    void load (int slot, const juce::File&, bool applyDetectedTuning, uint64_t generation);

private:
    void run() override;
    struct Request { int slot = 0; juce::File file; bool applyDetectedTuning = false; uint64_t generation = 0; };
    juce::AudioFormatManager& formats;
    Completion completion;
    juce::CriticalSection queueLock;
    std::vector<Request> queue;
};
}

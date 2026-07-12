#include "SampleLoader.h"

namespace chrome
{
SampleLoader::SampleLoader (juce::AudioFormatManager& manager, Completion callback)
    : juce::Thread ("Chrome sample loader"), formats (manager), completion (std::move (callback))
{
    startThread (juce::Thread::Priority::low);
}

SampleLoader::~SampleLoader()
{
    signalThreadShouldExit();
    notify();
    stopThread (3000);
}

void SampleLoader::load (int slot, const juce::File& file)
{
    {
        const juce::ScopedLock lock (queueLock);
        std::erase_if (queue, [slot] (const Request& request) { return request.slot == slot; });
        queue.push_back ({ slot, file });
    }
    notify();
}

void SampleLoader::run()
{
    while (! threadShouldExit())
    {
        Request request;
        bool hasRequest = false;
        {
            const juce::ScopedLock lock (queueLock);
            if (! queue.empty())
            {
                request = queue.back();
                queue.pop_back();
                hasRequest = true;
            }
        }

        if (! hasRequest)
        {
            wait (500);
            continue;
        }

        std::shared_ptr<const SampleData> result;
        juce::String error;
        if (auto reader = std::unique_ptr<juce::AudioFormatReader> (formats.createReaderFor (request.file)))
        {
            constexpr int64_t maxFrames = 44100LL * 60LL * 20LL;
            const auto frames = static_cast<int> (juce::jmin (reader->lengthInSamples, maxFrames));
            juce::AudioBuffer<float> audio (juce::jlimit (1, 2, static_cast<int> (reader->numChannels)), frames);
            if (reader->read (&audio, 0, frames, 0, true, true))
                result = std::make_shared<SampleData> (std::move (audio), reader->sampleRate, request.file);
            else
                error = "The audio data could not be decoded.";
        }
        else
        {
            error = "Unsupported or unreadable audio file.";
        }

        if (completion)
            completion (request.slot, std::move (result), error);
    }
}
}

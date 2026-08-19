#include "AudioSDL.h"

using namespace PRUZEA;

AudioSDL::~AudioSDL()
{
    end();
}

bool AudioSDL::begin()
{
    if (started) return true;

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        return false;
    }

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_S16;
    spec.channels = 1;
    spec.freq = static_cast<int>(SAMPLE_RATE);

    stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        nullptr,
        nullptr);

    if (stream == nullptr)
    {
        return false;
    }

    if (!SDL_ResumeAudioStreamDevice(stream))
    {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
        return false;
    }

    started = true;
    return true;
}

void AudioSDL::end()
{
    if (stream != nullptr)
    {
        SDL_FlushAudioStream(stream);
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }

    started = false;
}

bool AudioSDL::toneSamples(
    int,
    int,
    uint32_t totalSamples,
    uint32_t& writtenSamples,
    float,
    float)
{
    // AudioBase currently does not use this path.
    // Consume the request without duplicating tone synthesis in the backend.
    writtenSamples = totalSamples;
    return true;
}

bool AudioSDL::pcmSamples(const int16_t* samples, uint32_t sampleCount)
{
    if (!started || stream == nullptr)
    {
        return false;
    }

    if (samples == nullptr || sampleCount == 0)
    {
        return true;
    }

    constexpr int MAX_QUEUED_BUFFERS = 4;
    constexpr uint64_t QUEUE_WAIT_TIMEOUT_MSEC = 100;
    const int sampleBytes = static_cast<int>(sampleCount * sizeof(int16_t));
    const int maximumQueuedBytes = sampleBytes * MAX_QUEUED_BUFFERS;
    const uint64_t waitStartMsec = SDL_GetTicks();
    int queuedBytes = SDL_GetAudioStreamQueued(stream);
    if (queuedBytes < 0) return false;

    while (queuedBytes + sampleBytes > maximumQueuedBytes)
    {
        if (SDL_GetTicks() - waitStartMsec >= QUEUE_WAIT_TIMEOUT_MSEC) return false;
        SDL_Delay(1);
        queuedBytes = SDL_GetAudioStreamQueued(stream);
        if (queuedBytes < 0) return false;
    }

    return SDL_PutAudioStreamData(
        stream,
        samples,
        sampleBytes);
}

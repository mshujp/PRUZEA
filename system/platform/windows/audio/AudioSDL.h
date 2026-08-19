#pragma once

#include "AudioBase.h"
#include <SDL3/SDL.h>

namespace PRUZEA {

class AudioSDL : public AudioBase
{
private:
    static constexpr uint32_t SAMPLE_RATE = 22050;

    SDL_AudioStream* stream = nullptr;
    bool started = false;

    uint32_t sampleRate() const override { return SAMPLE_RATE; }

    // Kept only to satisfy the AudioBase backend interface.
    // AudioBase currently renders SE, ToneNote music, and MIDI to PCM and
    // sends the mixed result through pcmSamples().
    bool toneSamples(
        int startFrequency,
        int endFrequency,
        uint32_t totalSamples,
        uint32_t& writtenSamples,
        float startVolumeScale,
        float endVolumeScale) override;

    bool pcmSamples(const int16_t* samples, uint32_t sampleCount) override;

public:
    AudioSDL() = default;
    ~AudioSDL() override;

    const char* getName() const override { return "SDL"; }
    uint8_t getVolumeSteps() const override { return 4; }

    bool begin() override;
    void end() override;
};

} // namespace PRUZEA

#include "AudioBase.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

using namespace PRUZEA;

namespace {

using CriticalSectionLock = SpinLockGuard;

template<std::size_t N>
constexpr Audio::Sound makeSound(const Audio::SoundStep (&steps)[N])
{
    return { steps, static_cast<uint16_t>(N) };
}

static constexpr Audio::SoundStep SE_NO_1_STEPS[]  = { {1200,1200,30,1.0f,1.0f}, {1800,1800,20,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_2_STEPS[]  = { {300,300,40,1.0f,1.0f}, {150,150,30,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_3_STEPS[]  = { {800,800,60,1.0f,1.0f}, {1200,1200,60,1.0f,1.0f}, {1600,1600,100,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_4_STEPS[]  = { {1000,1000,50,1.0f,1.0f}, {1300,1300,50,1.0f,1.0f}, {1500,1500,50,1.0f,1.0f}, {2000,2000,150,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_5_STEPS[]  = { {600,600,30,1.0f,1.0f}, {400,400,30,1.0f,1.0f}, {600,600,30,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_6_STEPS[]  = { {1500,1500,40,1.0f,1.0f}, {1000,1000,40,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_7_STEPS[]  = { {2500,2500,20,1.0f,1.0f}, {0,0,10,0.0f,0.0f}, {2500,2500,20,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_8_STEPS[]  = { {400,400,50,1.0f,1.0f}, {600,600,50,1.0f,1.0f}, {800,800,50,1.0f,1.0f}, {1000,1000,50,1.0f,1.0f}, {1200,1200,50,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_9_STEPS[]  = { {2000,2000,20,1.0f,1.0f}, {1500,1500,20,1.0f,1.0f}, {2000,2000,20,1.0f,1.0f}, {1500,1500,20,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_10_STEPS[] = { {100,100,80,1.0f,1.0f}, {80,80,120,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_11_STEPS[] = { {500,500,20,1.0f,1.0f}, {1000,1000,20,1.0f,1.0f}, {1500,1500,20,1.0f,1.0f}, {2000,2000,20,1.0f,1.0f}, {2500,2500,40,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_12_STEPS[] = { {1800,1800,100,1.0f,1.0f}, {1400,1400,100,1.0f,1.0f}, {1100,1100,100,1.0f,1.0f}, {900,900,200,1.0f,1.0f} };
static constexpr Audio::SoundStep SE_NO_13_STEPS[] = { {200,150,80,1.0f,1.0f}, {0,0,50,0.0f,0.0f}, {200,150,120,1.0f,1.0f} };

} // namespace

const Audio::Sound Audio::SE::NO_1  = makeSound(SE_NO_1_STEPS);
const Audio::Sound Audio::SE::NO_2  = makeSound(SE_NO_2_STEPS);
const Audio::Sound Audio::SE::NO_3  = makeSound(SE_NO_3_STEPS);
const Audio::Sound Audio::SE::NO_4  = makeSound(SE_NO_4_STEPS);
const Audio::Sound Audio::SE::NO_5  = makeSound(SE_NO_5_STEPS);
const Audio::Sound Audio::SE::NO_6  = makeSound(SE_NO_6_STEPS);
const Audio::Sound Audio::SE::NO_7  = makeSound(SE_NO_7_STEPS);
const Audio::Sound Audio::SE::NO_8  = makeSound(SE_NO_8_STEPS);
const Audio::Sound Audio::SE::NO_9  = makeSound(SE_NO_9_STEPS);
const Audio::Sound Audio::SE::NO_10 = makeSound(SE_NO_10_STEPS);
const Audio::Sound Audio::SE::NO_11 = makeSound(SE_NO_11_STEPS);
const Audio::Sound Audio::SE::NO_12 = makeSound(SE_NO_12_STEPS);
const Audio::Sound Audio::SE::NO_13 = makeSound(SE_NO_13_STEPS);

void AudioBase::setVolumeLevel(int8_t level)
{
    const uint8_t volumeSteps = getVolumeSteps();
    const int8_t maxLevel = volumeSteps > 0 ? static_cast<int8_t>(volumeSteps - 1) : 0;
    const int8_t clampedLevel = std::clamp(level, static_cast<int8_t>(0), maxLevel);

    volumeLevel.store(clampedLevel, std::memory_order_relaxed);
    if (clampedLevel > 0)
    {
        volumeBeforeMute.store(clampedLevel, std::memory_order_relaxed);
    }
}

void AudioBase::upVolume()
{
    setVolumeLevel(getVolumeLevel() + 1);
}

void AudioBase::downVolume()
{
    setVolumeLevel(getVolumeLevel() - 1);
}

int8_t AudioBase::getVolumeLevel() const
{
    return volumeLevel.load(std::memory_order_relaxed);
}

void AudioBase::setMute(bool mute)
{
    if (mute)
    {
        const int8_t previousLevel = volumeLevel.exchange(0, std::memory_order_relaxed);
        if (previousLevel > 0)
        {
            volumeBeforeMute.store(previousLevel, std::memory_order_relaxed);
        }
        return;
    }

    if (isMuted())
    {
        setVolumeLevel(volumeBeforeMute.load(std::memory_order_relaxed));
    }
}

bool AudioBase::isMuted() const
{
    return volumeLevel.load(std::memory_order_relaxed) == 0;
}

void AudioBase::toggleMute()
{
    setMute(!isMuted());
}

const Audio::Sound* AudioBase::takeTriggerSE(float& gain)
{
    CriticalSectionLock lock(stateLock);

    const Sound* sound = triggerSE;
    gain = triggerSEGain;

    triggerSE = nullptr;
    triggerSEGain = 1.0f;
    triggerSEPending.store(false, std::memory_order_release);

    return sound;
}

bool AudioBase::hasPendingSE() const
{
    return triggerSEPending.load(std::memory_order_acquire);
}

void AudioBase::startPendingSE()
{
    if (!hasPendingSE()) return;

    float gain = 1.0f;
    const Sound* sound = takeTriggerSE(gain);
    if (sound == nullptr) return;

    CriticalSectionLock lock(stateLock);
    const bool replacingActiveSE = activeSE != nullptr;

    activeSE = sound;
    activeSEGain = gain;
    seStepIndex = 0;
    seStepWrittenSamples = 0;
    if (!replacingActiveSE) sePhase = 0;
}

void AudioBase::resetActiveSE()
{
    activeSE = nullptr;
    activeSEGain = 1.0f;
    seStepIndex = 0;
    seStepWrittenSamples = 0;
    sePhase = 0;
}

void AudioBase::playSE(const Sound* sound, float gain)
{
    if (sound == nullptr || sound->steps == nullptr || sound->stepCount == 0) return;

    CriticalSectionLock lock(stateLock);

    triggerSE = sound;
    triggerSEGain = std::clamp(gain, 0.0f, 1.0f);
    triggerSEPending.store(true, std::memory_order_release);
}

void AudioBase::stopSE()
{
    CriticalSectionLock lock(stateLock);

    triggerSE = nullptr;
    triggerSEGain = 1.0f;
    triggerSEPending.store(false, std::memory_order_release);
    resetActiveSE();
}

void AudioBase::playMusic(const Music* newMusic)
{
    CriticalSectionLock lock(stateLock);

    midiPlayer.stop();
    midiActive.store(false, std::memory_order_release);

    music = newMusic;
    musicNoteIndex = 0;
    musicRepeatIndex = 0;
    musicNoteWrittenSamples = 0;
    musicPhase = 0;

    const bool playable = newMusic != nullptr && newMusic->notes != nullptr && newMusic->noteCount > 0;
    musicActive.store(playable, std::memory_order_release);
}

void AudioBase::playMidi(const Midi* midi)
{
    CriticalSectionLock lock(stateLock);

    music = nullptr;
    musicNoteIndex = 0;
    musicRepeatIndex = 0;
    musicNoteWrittenSamples = 0;
    musicPhase = 0;
    musicActive.store(false, std::memory_order_release);

    const bool playable = midiPlayer.play(midi, sampleRate());
    midiActive.store(playable, std::memory_order_release);
}

void AudioBase::stopMusic()
{
    CriticalSectionLock lock(stateLock);

    music = nullptr;
    musicNoteIndex = 0;
    musicRepeatIndex = 0;
    musicNoteWrittenSamples = 0;
    musicPhase = 0;
    musicActive.store(false, std::memory_order_release);

    midiPlayer.stop();
    midiActive.store(false, std::memory_order_release);
}

uint32_t AudioBase::phaseStepForFrequency(uint32_t frequency, uint32_t rate)
{
    if (frequency == 0 || rate == 0) return 0;
    return static_cast<uint32_t>((static_cast<uint64_t>(frequency) << 32) / rate);
}

int16_t AudioBase::clampSample(int32_t sample)
{
    if (sample > 32767) return 32767;
    if (sample < -32768) return -32768;
    return static_cast<int16_t>(sample);
}

int16_t AudioBase::renderTriangle(uint32_t phase, float volume)
{
    const uint16_t position = static_cast<uint16_t>(phase >> 16);
    int32_t wave;

    if (position < 32768u)
    {
        wave = -32767 + static_cast<int32_t>(position) * 2;
    }
    else
    {
        wave = 98303 - static_cast<int32_t>(position) * 2;
    }

    const float clampedVolume = std::clamp(volume, 0.0f, 1.0f);
    return static_cast<int16_t>(
        static_cast<float>(wave) *
        clampedVolume *
        (static_cast<float>(TONE_AMPLITUDE) / 32767.0f));
}

void AudioBase::renderMusicSamples(int16_t* output, uint32_t sampleCount)
{
    std::memset(output, 0, sizeof(int16_t) * sampleCount);

    if (!musicActive.load(std::memory_order_relaxed)) return;

    for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        while (musicActive.load(std::memory_order_relaxed))
        {
            if (music == nullptr || music->notes == nullptr || music->noteCount == 0 || music->bpm == 0)
            {
                music = nullptr;
                musicActive.store(false, std::memory_order_release);
                break;
            }

            if (musicNoteIndex >= music->noteCount)
            {
                if (music->playCount == 0 || musicRepeatIndex + 1 < music->playCount)
                {
                    musicNoteIndex = 0;
                    musicRepeatIndex = music->playCount == 0
                        ? 0
                        : static_cast<uint8_t>(musicRepeatIndex + 1);
                    musicNoteWrittenSamples = 0;
                    continue;
                }

                music = nullptr;
                musicNoteIndex = 0;
                musicRepeatIndex = 0;
                musicNoteWrittenSamples = 0;
                musicActive.store(false, std::memory_order_release);
                break;
            }

            const ToneNote& note = music->notes[musicNoteIndex];
            uint32_t durationUnits = static_cast<uint32_t>(note.duration);
            uint16_t nextNoteIndex = static_cast<uint16_t>(musicNoteIndex + 1);

            while (note.frequency != ToneNote::REST
                && nextNoteIndex < music->noteCount
                && music->notes[nextNoteIndex - 1].tie
                && music->notes[nextNoteIndex].frequency == note.frequency)
            {
                durationUnits += static_cast<uint32_t>(music->notes[nextNoteIndex].duration);
                ++nextNoteIndex;
            }

            const uint64_t numerator =
                static_cast<uint64_t>(sampleRate()) * 60u * durationUnits;

            const uint32_t totalSamples = static_cast<uint32_t>(
                numerator /
                (static_cast<uint64_t>(music->bpm) *
                 static_cast<uint32_t>(ToneNote::Q)));

            if (totalSamples == 0 || musicNoteWrittenSamples >= totalSamples)
            {
                musicNoteIndex = nextNoteIndex;
                musicNoteWrittenSamples = 0;
                continue;
            }

            if (note.frequency != ToneNote::REST)
            {
                output[sampleIndex] = renderTriangle(
                    musicPhase,
                    std::clamp(music->gain, 0.0f, 1.0f));

                musicPhase += phaseStepForFrequency(
                    note.frequency,
                    sampleRate());
            }

            ++musicNoteWrittenSamples;
            if (musicNoteWrittenSamples >= totalSamples)
            {
                musicNoteIndex = nextNoteIndex;
                musicNoteWrittenSamples = 0;
            }
            break;
        }
    }
}

void AudioBase::renderMidiSamples(int16_t* output, uint32_t sampleCount)
{
    std::memset(output, 0, sizeof(int16_t) * sampleCount);

    if (!midiActive.load(std::memory_order_relaxed)) return;

    midiPlayer.render(output, sampleCount);
    if (!midiPlayer.isPlaying())
    {
        midiActive.store(false, std::memory_order_release);
    }
}

void AudioBase::renderSESamples(int16_t* output, uint32_t sampleCount)
{
    std::memset(output, 0, sizeof(int16_t) * sampleCount);

    for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        while (activeSE != nullptr)
        {
            if (activeSE->steps == nullptr || seStepIndex >= activeSE->stepCount)
            {
                resetActiveSE();
                break;
            }

            const SoundStep& step = activeSE->steps[seStepIndex];
            const uint32_t totalSamples =
                (sampleRate() * static_cast<uint32_t>(step.durationMsec)) / 1000u;

            if (totalSamples == 0 || seStepWrittenSamples >= totalSamples)
            {
                ++seStepIndex;
                seStepWrittenSamples = 0;
                continue;
            }

            const uint32_t position = seStepWrittenSamples;
            const uint32_t denominator = totalSamples > 1 ? totalSamples - 1 : 1;

            const int32_t frequencyDelta =
                static_cast<int32_t>(step.endFrequency) -
                static_cast<int32_t>(step.startFrequency);

            int32_t frequency =
                static_cast<int32_t>(step.startFrequency) +
                static_cast<int32_t>(
                    (static_cast<int64_t>(frequencyDelta) * position) /
                    denominator);

            if (frequency < 0) frequency = 0;

            const float t =
                static_cast<float>(position) /
                static_cast<float>(denominator);

            const float volume =
                (step.startVolume +
                 (step.endVolume - step.startVolume) * t) *
                activeSEGain;

            if (frequency > 0)
            {
                output[sampleIndex] = renderTriangle(sePhase, volume);
                sePhase += phaseStepForFrequency(
                    static_cast<uint32_t>(frequency),
                    sampleRate());
            }

            ++seStepWrittenSamples;
            if (seStepWrittenSamples >= totalSamples)
            {
                ++seStepIndex;
                seStepWrittenSamples = 0;
            }
            break;
        }
    }
}

void AudioBase::mixSamples(
    int16_t* output,
    const int16_t* bgm,
    const int16_t* se,
    uint32_t sampleCount,
    bool hasBgm,
    bool hasSE)
{
    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        int32_t mixed = 0;

        if (hasBgm)
        {
            mixed += static_cast<int32_t>(bgm[i]);
        }

        if (hasSE)
        {
            const float seScale = hasBgm ? SE_MIX_GAIN : 1.0f;
            mixed += static_cast<int32_t>(
                static_cast<float>(se[i]) * seScale);
        }

        output[i] = clampSample(mixed);
    }
}

void AudioBase::update()
{
    startPendingSE();

    bool hasBgm = false;
    bool hasSE = false;

    {
        CriticalSectionLock lock(stateLock);

        hasBgm =
            musicActive.load(std::memory_order_relaxed) ||
            midiActive.load(std::memory_order_relaxed);

        hasSE = activeSE != nullptr;

        if (!hasBgm && !hasSE) return;

        if (musicActive.load(std::memory_order_relaxed))
        {
            renderMusicSamples(bgmBuffer, AUDIO_BUFFER_SAMPLES);
        }
        else if (midiActive.load(std::memory_order_relaxed))
        {
            renderMidiSamples(bgmBuffer, AUDIO_BUFFER_SAMPLES);
        }
        else
        {
            std::memset(bgmBuffer, 0, sizeof(bgmBuffer));
        }

        renderSESamples(seBuffer, AUDIO_BUFFER_SAMPLES);

        hasBgm =
            musicActive.load(std::memory_order_relaxed) ||
            midiActive.load(std::memory_order_relaxed) ||
            hasBgm;

        hasSE = activeSE != nullptr || hasSE;

        mixSamples(
            mixBuffer,
            bgmBuffer,
            seBuffer,
            AUDIO_BUFFER_SAMPLES,
            hasBgm,
            hasSE);
    }

    pcmSamples(mixBuffer, AUDIO_BUFFER_SAMPLES);
}

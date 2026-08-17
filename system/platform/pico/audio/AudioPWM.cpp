#include "AudioPWM.h"
#include "hardware/clocks.h"
#include <algorithm>

using namespace PRUZEA;

AudioPWM::AudioPWM(const Config& config)
    : pin(config.pwmPin), mode(config.mode)
{
}

bool AudioPWM::begin()
{
    if (started) return true;
    if (pin < 0) return false;

    gpio_set_function(pin, GPIO_FUNC_PWM);
    sliceNum = pwm_gpio_to_slice_num(pin);

    pwm_set_enabled(sliceNum, false);
    pwm_set_gpio_level(pin, 0);
    pwm_set_enabled(sliceNum, true);

    pcmMode = false;
    started = true;
    return true;
}

void AudioPWM::end()
{
    if (!started) return;

    silence();
    pwm_set_enabled(sliceNum, false);
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);

    started = false;
}

uint32_t AudioPWM::sampleRate() const
{
    return SAMPLE_RATE;
}

void AudioPWM::configurePcmMode()
{
    if (pcmMode) return;

    pwm_set_enabled(sliceNum, false);
    gpio_set_function(pin, GPIO_FUNC_PWM);
    pwm_set_clkdiv(sliceNum, 1.0f);
    pwm_set_wrap(sliceNum, 255);
    pwm_set_gpio_level(pin, 128);
    pwm_set_enabled(sliceNum, true);
    pcmMode = true;
}

void AudioPWM::setToneFrequency(int frequency)
{
    if (frequency <= 0)
    {
        silence();
        return;
    }

    const uint32_t clk = clock_get_hz(clk_sys);

    gpio_set_function(pin, GPIO_FUNC_PWM);

    float div = static_cast<float>(clk)
        / (static_cast<float>(frequency) * 65536.0f);
    div = std::clamp(div, 1.0f, 255.0f);

    uint32_t wrap = static_cast<uint32_t>(
        static_cast<float>(clk) / (static_cast<float>(frequency) * div));
    if (wrap == 0) wrap = 1;
    --wrap;
    if (wrap > 65535u) wrap = 65535u;

    pwm_set_enabled(sliceNum, false);
    pwm_set_clkdiv(sliceNum, div);
    pwm_set_wrap(sliceNum, static_cast<uint16_t>(wrap));
    pwm_set_enabled(sliceNum, true);
    pcmMode = false;
}

void AudioPWM::setDuty(float volumeScale)
{
    const uint16_t wrap = pwm_hw->slice[sliceNum].top;

    float base = 0.0f;
    switch (getVolumeLevel())
    {
    case 1: base = 0.18f; break;
    default: base = 0.0f; break;
    }

    const float duty = std::clamp(base * std::clamp(volumeScale, 0.0f, 1.0f), 0.0f, 0.5f);
    pwm_set_gpio_level(pin, static_cast<uint16_t>(wrap * duty));
}

void AudioPWM::silence()
{
    if (pin >= 0)
    {
        pwm_set_gpio_level(pin, 0);
        gpio_put(pin, 0);
    }
}

bool AudioPWM::toneSamples(int startFrequency, int endFrequency, uint32_t totalSamples, uint32_t& writtenSamples, float startVolumeScale, float endVolumeScale)
{
    if (!started || totalSamples == 0) return true;

    const uint32_t updateSamples = std::max<uint32_t>(
        1u,
        (static_cast<uint32_t>(SAMPLE_RATE) * UPDATE_INTERVAL_MSEC) / 1000u);

    while (writtenSamples < totalSamples)
    {
        const uint32_t remainSamples = totalSamples - writtenSamples;
        const uint32_t chunkSamples = std::min(remainSamples, updateSamples);

        // Use the center of this chunk so short sweeps and fades do not begin
        // noticeably below or above their requested values.
        const uint32_t centerSample = writtenSamples + (chunkSamples / 2u);
        const float progress = std::clamp(static_cast<float>(centerSample) / static_cast<float>(totalSamples), 0.0f, 1.0f);

        const float frequencyValue = static_cast<float>(startFrequency) + static_cast<float>(endFrequency - startFrequency) * progress;
        const float volumeValue = startVolumeScale + (endVolumeScale - startVolumeScale) * progress;
        const int frequency = static_cast<int>(frequencyValue + 0.5f);

        if (frequency > 0 && volumeValue > 0.0f)
        {
            setToneFrequency(frequency);
            setDuty(volumeValue);
        }
        else
        {
            silence();
        }

        sleep_us((chunkSamples * 1000000u) / SAMPLE_RATE);
        writtenSamples += chunkSamples;

        if (hasPendingSE())
        {
            silence();
            return false;
        }
    }

    silence();
    return true;
}

bool AudioPWM::pcmSamplesBuzzer(const int16_t* samples, uint32_t sampleCount)
{
    if (!started || samples == nullptr || sampleCount == 0) return true;

    pwm_set_enabled(sliceNum, false);
    gpio_set_function(pin, GPIO_FUNC_SIO);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
    pcmMode = false;

    uint64_t deadlineUsec = time_us_64();
    uint32_t timingRemainder = 0;

    const bool muted = getVolumeLevel() == 0;

    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        gpio_put(pin, !muted && samples[i] > 0);

        timingRemainder += 1000000u;
        deadlineUsec += timingRemainder / SAMPLE_RATE;
        timingRemainder %= SAMPLE_RATE;

        busy_wait_until(from_us_since_boot(deadlineUsec));
    }

    gpio_put(pin, 0);
    pcmMode = false;
    return true;
}

bool AudioPWM::pcmSamplesDAC(const int16_t* samples, uint32_t sampleCount)
{
    if (!started || samples == nullptr || sampleCount == 0) return true;

    configurePcmMode();

    float volumeScale = getVolumeLevel() > 0 ? 1.00f : 0.0f;
    uint64_t deadlineUsec = time_us_64();
    uint32_t timingRemainder = 0;

    for (uint32_t i = 0; i < sampleCount; ++i)
    {
        const int32_t scaled = static_cast<int32_t>(static_cast<float>(samples[i]) * volumeScale);
        const int32_t duty = 128 + (scaled >> 8);
        pwm_set_gpio_level(pin, static_cast<uint16_t>(std::clamp<int32_t>(duty, 0, 255)));

        timingRemainder += 1000000u;
        deadlineUsec += timingRemainder / SAMPLE_RATE;
        timingRemainder %= SAMPLE_RATE;
        busy_wait_until(from_us_since_boot(deadlineUsec));

    }

    pwm_set_gpio_level(pin, 128);
    return true;
}

bool AudioPWM::pcmSamples(const int16_t* samples, uint32_t sampleCount)
{
    return mode == Mode::BUZZER
        ? pcmSamplesBuzzer(samples, sampleCount)
        : pcmSamplesDAC(samples, sampleCount);
}

#pragma once

#include "AudioBase.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

namespace PRUZEA {

class AudioPWM : public AudioBase {
public:
    enum class Mode {
        BUZZER,
        DAC
    };

    struct Config {
        int8_t pwmPin = -1;
        Mode mode = Mode::BUZZER;
    };

private:
    static constexpr uint16_t SAMPLE_RATE = 22050;
    static constexpr uint16_t UPDATE_INTERVAL_MSEC = 10;

    int8_t pin;
    Mode mode;
    uint sliceNum = 0;
    bool started = false;

    uint32_t sampleRate() const override;
    bool toneSamples(int startFrequency, int endFrequency, uint32_t totalSamples, uint32_t& writtenSamples, float startVolumeScale, float endVolumeScale) override;
    bool pcmSamples(const int16_t* samples, uint32_t sampleCount) override;
    bool pcmSamplesDAC(const int16_t* samples, uint32_t sampleCount);
    bool pcmSamplesBuzzer(const int16_t* samples, uint32_t sampleCount);

    bool pcmMode = false;
    void configurePcmMode();
    void setToneFrequency(int frequency);
    void setDuty(float volumeScale);
    void silence();

public:
    explicit AudioPWM(const Config& config);
    const char* getName() const override { return "PWM"; }
    uint8_t getVolumeSteps() const override { return 2; }
    bool begin() override;
    void end() override;
};

} // namespace PRUZEA

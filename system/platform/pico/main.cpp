#include "System.h"
#include "PRUZEAConfig.h"
#include "power/PicoBatteryConfig.h"

#if PRUZEA_INPUT_GPIO_BUTTONS
#include "input/InputGpioButtons.h"
#elif PRUZEA_INPUT_SNES
#include "input/InputSnes.h"
#elif PRUZEA_INPUT_PS
#include "input/InputPS.h"
#endif

#if PRUZEA_TOUCH_XPT2046
#include "input/InputTouch.h"
#endif

#if PRUZEA_AUDIO_I2S
#include "audio/AudioI2S.h"
#elif PRUZEA_AUDIO_PWM
#include "audio/AudioPWM.h"
#elif PRUZEA_AUDIO_NONE
#include "AudioStub.h"
#endif

#if PRUZEA_DISPLAY_ILI9341
#include "graphics/GraphicsILI9341.h"
#include "SystemUI320x240.h"
#include "SystemUI128x64Mono.h"
#elif PRUZEA_DISPLAY_SSD1306
#include "graphics/GraphicsSSD1306.h"
#include "SystemUI128x64Mono.h"
#endif

#if PRUZEA_STORAGE_SD
#include "storage/StorageSD.h"
#elif PRUZEA_STORAGE_NONE
#include "StorageStub.h"
#endif

#include PRUZEA_BOARD_CONFIG_HEADER

#include <hardware/adc.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

using namespace PRUZEA;

namespace
{

constexpr size_t AUDIO_CORE_STACK_SIZE = 2 * 1024;
constexpr uint8_t TEMPERATURE_ADC_CHANNEL = 8;
constexpr uint8_t BATTERY_ADC_SAMPLE_COUNT = 16;
constexpr float TEMPERATURE_SENSOR_VOLTAGE_27C = 0.706f;
alignas(8) uint32_t audioCoreStack[AUDIO_CORE_STACK_SIZE / sizeof(uint32_t)];

#if PRUZEA_DISPLAY_ILI9341
GraphicsILI9341 graphicsImpl(GRAPHICS_CONFIG);
SystemUI320x240 systemUIImpl;
//SystemUI128x64Mono systemUIImpl(Graphics::Color::SSD1306_ON, Graphics::Color::SSD1306_OFF);
#elif PRUZEA_DISPLAY_SSD1306
GraphicsSSD1306 graphicsImpl(GRAPHICS_CONFIG);
SystemUI128x64Mono systemUIImpl(Graphics::Color::SSD1306_ON, Graphics::Color::SSD1306_OFF);
#endif

#if PRUZEA_INPUT_GPIO_BUTTONS
#if PRUZEA_TOUCH_XPT2046
InputTouch<InputGpioButtons> inputImpl(graphicsImpl.getLGFXContext(), TOUCH_CONFIG, BUTTON_MAPPING);
#else
InputGpioButtons inputImpl(BUTTON_MAPPING);
#endif
#elif PRUZEA_INPUT_SNES
#if PRUZEA_TOUCH_XPT2046
InputTouch<InputSnes> inputImpl(graphicsImpl.getLGFXContext(), TOUCH_CONFIG, INPUT_CONFIG);
#else
InputSnes inputImpl(INPUT_CONFIG);
#endif
#elif PRUZEA_INPUT_PS
#if PRUZEA_TOUCH_XPT2046
InputTouch<InputPS> inputImpl(graphicsImpl.getLGFXContext(), TOUCH_CONFIG, INPUT_CONFIG);
#else
InputPS inputImpl(INPUT_CONFIG);
#endif
#endif

#if PRUZEA_STORAGE_SD
StorageSD storageImpl(STORAGE_CONFIG);
#elif PRUZEA_STORAGE_NONE
StorageStub storageImpl;
#endif

#if PRUZEA_AUDIO_I2S
AudioI2S audioImpl(AUDIO_CONFIG);
#elif PRUZEA_AUDIO_PWM
AudioPWM audioImpl(AUDIO_CONFIG);
#elif PRUZEA_AUDIO_NONE
AudioStub audioImpl;
#endif

System* activeSystem = nullptr;

void initPlatformHardware()
{
    if (BATTERY_CONFIG.adcPin >= 0)
    {
        const uint batteryPin = static_cast<uint>(BATTERY_CONFIG.adcPin);
        gpio_disable_pulls(batteryPin);
        adc_init();
        adc_set_temp_sensor_enabled(true);
        adc_gpio_init(batteryPin);
        gpio_set_dir(batteryPin, GPIO_IN);
    }

#if PRUZEA_DISPLAY_ILI9341
    if (GRAPHICS_CONFIG.backlightPin >= 0)
    {
        gpio_init(static_cast<uint>(GRAPHICS_CONFIG.backlightPin));
        gpio_set_dir(static_cast<uint>(GRAPHICS_CONFIG.backlightPin), GPIO_OUT);
        gpio_put(static_cast<uint>(GRAPHICS_CONFIG.backlightPin), 1);
    }
#endif
}

bool readBatteryVoltage(void*, float& voltage)
{
    if (BATTERY_CONFIG.adcPin < 0)
    {
        return false;
    }

    adc_select_input(TEMPERATURE_ADC_CHANNEL);
    adc_read();

    uint32_t temperatureRawTotal = 0;
    for (uint8_t i = 0; i < BATTERY_ADC_SAMPLE_COUNT; ++i)
    {
        temperatureRawTotal += adc_read();
    }

    adc_select_input(BATTERY_CONFIG.adcChannel);
    adc_read();

    uint32_t batteryRawTotal = 0;
    for (uint8_t i = 0; i < BATTERY_ADC_SAMPLE_COUNT; ++i)
    {
        batteryRawTotal += adc_read();
    }

    const float temperatureRaw = static_cast<float>(temperatureRawTotal) / BATTERY_ADC_SAMPLE_COUNT;
    const float batteryRaw = static_cast<float>(batteryRawTotal) / BATTERY_ADC_SAMPLE_COUNT;
    float estimatedVref = temperatureRaw > 0.0f
        ? TEMPERATURE_SENSOR_VOLTAGE_27C * 4095.0f / temperatureRaw
        : 3.3f;
    if (estimatedVref < 2.0f || estimatedVref > 3.6f) estimatedVref = 3.3f;

    voltage = (batteryRaw * estimatedVref / 4095.0f) * 2.0f *
        BATTERY_CONFIG.voltageCalibrationFactor + BATTERY_CONFIG.voltageCalibrationOffset;
    return true;
}

void __time_critical_func(audioCoreEntry)()
{
    if (activeSystem != nullptr)
    {
        activeSystem->runAudioWorker();
    }
}

bool launchAudioWorker(void*, System& system)
{
    activeSystem = &system;
    multicore_launch_core1_with_stack(audioCoreEntry, audioCoreStack, sizeof(audioCoreStack));
    return true;
}

void finishAudioWorker(void*, bool)
{
    multicore_reset_core1();
    activeSystem = nullptr;
}

void enterPowerOffState(void*, System::ShutdownReason)
{
    __asm volatile("cpsid i");
    while (true)
    {
        __wfi();
    }
}

} // namespace

int main()
{
    stdio_init_all();
    initPlatformHardware();

    const System::Callbacks callbacks{
        nullptr,
        readBatteryVoltage,
        launchAudioWorker,
        finishAudioWorker,
        enterPowerOffState
    };

    const System::BatteryConfig batteryConfig{
        .externalPowerThreshold = BATTERY_CONFIG.externalPowerThresholdVoltage,
        .warningThreshold = BATTERY_CONFIG.warningVoltage,
        .criticalThreshold = BATTERY_CONFIG.criticalVoltage,
        .cutoffThreshold = BATTERY_CONFIG.cutoffVoltage,
        .sampleIntervalFrames = 300,
        .initialSampleCount = 10,
        .initialSampleDelayMsec = 50
    };

    System system(graphicsImpl, inputImpl, storageImpl, audioImpl, systemUIImpl, callbacks, batteryConfig);

    return system.start() ? 0 : 1;
}

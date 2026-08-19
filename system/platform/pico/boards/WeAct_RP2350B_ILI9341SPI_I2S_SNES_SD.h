#pragma once

#include "InputBase.h"

//== Graphics ================================================
#if PRUZEA_DISPLAY_ILI9341
/// ILI9341 SPI LCD
constexpr PRUZEA::GraphicsILI9341::GraphicsILI9341SPIConfig GRAPHICS_CONFIG {
    // ===== SPI =====
    .spiHost = 1,
    //.spiWriteFreq = 75000000,
    .spiWriteFreq = 62500000,

    // ===== LCD Pins =====
    .clkPin = 10,
    .dataPin = 11,  // mosi
    .dcPin = 14,
    .csPin = 9,
    .resetPin = 15,
    .backlightPin = 22,

    // ===== Display =====
    .lcdRotate = 3,  // 1: Normal  3: Rotated 180 degrees

    // ===== Buffer =====
    .maxBufferWidth = PRUZEA::Graphics::ILI9341_SCREEN_BUF_W_MAX_RP2350,
    .maxBufferHeight = PRUZEA::Graphics::ILI9341_SCREEN_BUF_H_MAX_RP2350
};
#elif PRUZEA_DISPLAY_SSD1306
/// SSD1306 OLED
constexpr PRUZEA::GraphicsSSD1306::Config GRAPHICS_CONFIG {
    // ===== I2C =====
    .i2cPort = 0,
    .i2cAddr = 0x3C,

    // ===== OLED Pins =====
    .sdaPin = -1,
    .sclPin = -1,
    .resetPin = -1,

    // ===== Display =====
    .oledRotate = 0
};
#endif


//== Input ===================================================
/// Button-GPIO Mapping
constexpr PRUZEA::InputBase::ButtonMapping BUTTON_MAPPING {
    .HOME = 32,
    .VOL_UP = 29,
    .VOL_DOWN = 30,
    .MUTE = -1
};

#if PRUZEA_INPUT_SNES
/// SNES controller
constexpr PRUZEA::InputSnes::Config INPUT_CONFIG {
    // ===== GPIO Pins =====
    .gpioCLK = 27,
    .gpioLAT = 26,
    .gpioData = 28,

    // ===== Extra Buttons =====
    .buttonMapping = BUTTON_MAPPING
};
#elif PRUZEA_INPUT_PS
/// PlayStation controller (experimental; hardware-unverified)
constexpr PRUZEA::InputPS::Config INPUT_CONFIG {
    // ===== GPIO Pins =====
    .clockPin = 27,
    .commandPin = 26,
    .attentionPin = 28,
    .dataPin = 31,

    // ===== Extra Buttons =====
    .buttonMapping = BUTTON_MAPPING
};
#endif

#if PRUZEA_TOUCH_XPT2046
/// XPT2046 touchscreen
constexpr PRUZEA::InputTouchConfig TOUCH_CONFIG {
    // The LCD and touch panel may use the same or different SPI hosts.
    .spiHost = 1,
    .spiFreq = 2000000,
    .clkPin = 10,
    .mosiPin = 11,
    .misoPin = 8,
    .csPin = 21,
    .irqPin = 23,

    .minX = 250,
    .maxX = 3850,
    .minY = 250,
    .maxY = 3850,
    .minZ = 2048,
    .nativeWidth = 240,
    .nativeHeight = 320,
    .offsetRotation = 5
};
#endif

//== Audio ===================================================
#if PRUZEA_AUDIO_PWM
/// PWM
constexpr PRUZEA::AudioPWM::Config AUDIO_CONFIG {
    // ===== GPIO Pins =====
    .pwmPin = -1,
    // Use BUZZER when driving a passive buzzer directly. Use DAC when routing PWM audio through an amplifier.
    .mode = PRUZEA::AudioPWM::Mode::BUZZER
};
#elif PRUZEA_AUDIO_I2S
/// I2S
constexpr PRUZEA::AudioI2S::Config AUDIO_CONFIG {
    // ===== I2S Pins =====
    .bclkPin = 4,
    // The LRCK pin must be assigned to the BCLK + 1 pin.
    .dataPin = 2
};
#endif


//== Storage =================================================
#if PRUZEA_STORAGE_SD
// SD
constexpr PRUZEA::StorageSD::Config STORAGE_CONFIG {
    // ===== SPI =====
    .spiHost = 0,

    // ===== SD Card Pins =====
    .misoPin = 16,
    .sckPin = 18,
    .mosiPin = 19,
    .csPin = 13,

    // ===== Speed =====
    .baudRate = 12 * 1000 * 1000
};
#endif


//== Battery =================================================
constexpr PRUZEA::PicoPlatform::BatteryConfig BATTERY_CONFIG {
    // ===== ADC =====
    .adcPin = 42,
    .adcChannel = 2,
    .voltageCalibrationFactor = 1.01f,
    .voltageCalibrationOffset = 0.0f, 
    
    // ===== Voltage Thresholds =====
    .externalPowerThresholdVoltage = 2.0f,
    .warningVoltage = 3.5f,
    .criticalVoltage = 3.4f,
    .cutoffVoltage = 3.3f
};

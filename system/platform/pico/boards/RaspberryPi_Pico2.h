#pragma once

#include "InputBase.h"

// Raspberry Pi Pico 2 (RP2350) sample hardware profile.
// Change each GPIO number to match the actual wiring.
// Use -1 for hardware that is not connected.

//== Graphics ================================================
#if PRUZEA_DISPLAY_ILI9341
/// ILI9341 SPI LCD
constexpr PRUZEA::GraphicsILI9341::GraphicsILI9341SPIConfig GRAPHICS_CONFIG {
    // ===== SPI1 =====
    .spiHost = 1,
    .spiWriteFreq = 62500000,

    // ===== LCD Pins =====
    .clkPin = 10,
    .dataPin = 11,  // MOSI
    .dcPin = 14,
    .csPin = 9,
    .resetPin = 15,
    .backlightPin = 22,

    // ===== Display =====
    .lcdRotate = 1,  // 1: Normal  3: Rotated 180 degrees

    // ===== Buffer =====
    .maxBufferWidth = PRUZEA::Graphics::ILI9341_SCREEN_BUF_W_MAX_RP2350,
    .maxBufferHeight = PRUZEA::Graphics::ILI9341_SCREEN_BUF_H_MAX_RP2350
};
#elif PRUZEA_DISPLAY_ILI9341_PARALLEL
/// ILI9341 8-bit parallel LCD
constexpr PRUZEA::GraphicsILI9341::GraphicsILI9341ParallelConfig GRAPHICS_CONFIG {
    .writeFreq = 10000000,
    .dataPinBase = -1,  // The eight data pins must be connected consecutively, starting from dataPinBase.
    .wrPin = -1,
    .rdPin = -1,
    .dcPin = -1,
    .csPin = -1,
    .resetPin = -1,
    .backlightPin = -1,
    .lcdRotate = 1,
    .maxBufferWidth = PRUZEA::Graphics::ILI9341_SCREEN_BUF_W_MAX_RP2350,
    .maxBufferHeight = PRUZEA::Graphics::ILI9341_SCREEN_BUF_H_MAX_RP2350
};
#elif PRUZEA_DISPLAY_SSD1306
/// SSD1306 OLED
constexpr PRUZEA::GraphicsSSD1306::Config GRAPHICS_CONFIG {
    // ===== I2C0 =====
    .i2cPort = 0,
    .i2cAddr = 0x3C,  // 0x3C or 0x3D, depending on the module

    // ===== OLED Pins =====
    .sdaPin = -1,
    .sclPin = -1,
    .resetPin = -1,

    // ===== Display =====
    .oledRotate = 0  // 0: Normal  2: Rotated 180 degrees
};
#endif


//== Input ===================================================
/// Direct GPIO buttons and system-button mapping.
constexpr PRUZEA::InputBase::ButtonMapping BUTTON_MAPPING {
    .UP = -1,
    .DOWN = -1,
    .LEFT = -1,
    .RIGHT = -1,
    .A = -1,
    .B = -1,
    .L = -1,
    .R = -1,
    .START = -1,
    .SELECT = -1,
    .VOL_UP = -1,
    .VOL_DOWN = -1,
    .HOME = -1,
    .MUTE = -1
};

#if PRUZEA_INPUT_SNES
/// SNES controller
constexpr PRUZEA::InputSnes::Config INPUT_CONFIG {
    // ===== GPIO Pins =====
    .gpioCLK = -1,
    .gpioLAT = -1,
    .gpioData = -1,

    // ===== Extra Buttons =====
    .buttonMapping = BUTTON_MAPPING
};
#elif PRUZEA_INPUT_PS
/// PlayStation controller (experimental; hardware-unverified)
constexpr PRUZEA::InputPS::Config INPUT_CONFIG {
    // ===== GPIO Pins =====
    .clockPin = -1,
    .commandPin = -1,
    .attentionPin = -1,
    .dataPin = -1,

    // ===== Extra Buttons =====
    .buttonMapping = BUTTON_MAPPING
};
#endif

#if PRUZEA_TOUCH_XPT2046
/// XPT2046 touchscreen
constexpr PRUZEA::InputTouchConfig TOUCH_CONFIG {
    // The LCD and touch panel may use the same or different SPI hosts.
    .spiHost = 0,
    .spiFreq = 2000000,
    .clkPin = -1,
    .mosiPin = -1,
    .misoPin = -1,
    .csPin = -1,
    .irqPin = -1,
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
/// PWM speaker
constexpr PRUZEA::AudioPWM::Config AUDIO_CONFIG {
    // ===== GPIO Pins =====
    .pwmPin = -1,
    // Use BUZZER when driving a passive buzzer directly. Use DAC when routing PWM audio through an amplifier.
    .mode = PRUZEA::AudioPWM::Mode::BUZZER
};
#elif PRUZEA_AUDIO_I2S
/// I2S amplifier
constexpr PRUZEA::AudioI2S::Config AUDIO_CONFIG {
    // ===== I2S Pins =====
    .bclkPin = -1,
    // The LRCK pin must be assigned to the BCLK + 1 pin (GPIO 5).
    .dataPin = -1 
};
#endif


//== Storage =================================================
#if PRUZEA_STORAGE_SD
/// SD card over SPI0
constexpr PRUZEA::StorageSD::Config STORAGE_CONFIG {
    // ===== SPI =====
    .spiHost = 0,

    // ===== SD Card Pins =====
    .misoPin = -1,
    .sckPin = -1,
    .mosiPin = -1,
    .csPin = -1,

    // ===== Speed =====
    .baudRate = 12 * 1000 * 1000
};
#endif


//== Battery =================================================
/// Disabled in this sample. Configure an ADC pin and thresholds when needed.
constexpr PRUZEA::PicoPlatform::BatteryConfig BATTERY_CONFIG {
    // ===== ADC =====
    .adcPin = -1,
    .adcChannel = 0,
    .voltageCalibrationFactor = 1.0f,
    .voltageCalibrationOffset = 0.0f, 

    // ===== Voltage Thresholds =====
    .externalPowerThresholdVoltage = 2.0f,
    .warningVoltage = 3.6f,
    .criticalVoltage = 3.5f,
    .cutoffVoltage = 3.4f
};

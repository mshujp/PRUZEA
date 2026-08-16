#pragma once

#include "InputBase.h"

//== Graphics ================================================
#if PRUZEA_DISPLAY_ILI9341 && !PRUZEA_DISPLAY_ILI9341_PARALLEL
/// ILI9341 SPI LCD
constexpr PRUZEA::GraphicsILI9341::GraphicsILI9341SPIConfig GRAPHICS_CONFIG {
    // ===== SPI =====
    .spiHost = 1,
    .spiWriteFreq = 62500000,

    // ===== LCD Pins =====
    .clkPin   = 14,
    .dataPin  = 15,  // mosi
    .dcPin    = 3,
    .csPin    = 1,
    .resetPin = 2,
    .backlightPin = -1,

    // ===== Display =====
    .lcdRotate = 1 ,  // 1: Normal  3: Inverted

    // ===== Buffer =====
    .maxBufferWidth  = PRUZEA::Graphics::ILI9341_SCREEN_BUF_W_MAX_RP2040,
    .maxBufferHeight = PRUZEA::Graphics::ILI9341_SCREEN_BUF_H_MAX_RP2040
};
#elif PRUZEA_DISPLAY_ILI9341_PARALLEL
/// ILI9341 8-bit parallel LCD
constexpr PRUZEA::GraphicsILI9341::GraphicsILI9341ParallelConfig GRAPHICS_CONFIG {
    .writeFreq = 10000000,
    .dataPinBase = 8,  // The eight data pins must be connected consecutively, starting from dataPinBase
    .wrPin = 0,
    .rdPin = 4,
    .dcPin = 1,
    .csPin = 2,
    .resetPin = 3,
    .backlightPin = -1,
    .lcdRotate = 1,
    .maxBufferWidth = PRUZEA::Graphics::ILI9341_SCREEN_BUF_W_MAX_RP2040,
    .maxBufferHeight = PRUZEA::Graphics::ILI9341_SCREEN_BUF_H_MAX_RP2040
};
#elif PRUZEA_DISPLAY_SSD1306
/// SSD1306 OLED
constexpr PRUZEA::GraphicsSSD1306::Config GRAPHICS_CONFIG {
    // ===== I2C =====
    .i2cPort = 0,      // 0 or 1
    .i2cAddr = 0x3C,

    // ===== OLED Pins =====
    .sdaPin = 0,
    .sclPin = 1,
    .resetPin = -1,

    // ===== Display =====
    .oledRotate = 2  // 0: Normal  2: Inverted
};
#endif


//== Input ==================================================
/// Button-GPIO Mapping
constexpr PRUZEA::InputBase::ButtonMapping BUTTON_MAPPING {
    .UP        = 9,
    .DOWN      = 10,
    .LEFT      = 11,
    .RIGHT     = 12,
    .A         = 14,
    .B         = 15,
    .X         = -1,
    .Y         = -1,
    .L         = -1,
    .R         = -1,
    .L2        = -1,
    .R2        = -1,
    .L3        = -1,
    .R3        = -1,
    .START     = -1,
    .SELECT    = -1,
    .HOME      = 27,
    .VOL_UP    = -1,
    .VOL_DOWN  = -1,
    .MUTE      = -1
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

//== Audio ====================================================
#if PRUZEA_AUDIO_PWM
/// PWM
constexpr PRUZEA::AudioPWM::Config AUDIO_CONFIG {
    // ===== GPIO Pins =====
    .pwmPin = 29
};
#elif PRUZEA_AUDIO_I2S
/// I2S
constexpr PRUZEA::AudioI2S::Config AUDIO_CONFIG {
    // ===== I2S Pins =====
    .bclkPin = -1,
    // The LRCK pin must be assigned to the BCLK + 1 pin.
    .dataPin = -1
};
#endif


//== Storage ==================================================
#if PRUZEA_STORAGE_SD
// SD
constexpr PRUZEA::StorageSD::Config STORAGE_CONFIG {
    // ===== SPI =====
    .spiHost = 0,

    // ===== SD Card Pins =====
    .misoPin = 4,
    .sckPin  = 6,  // SCK or CLK
    .mosiPin = 7,
    .csPin   = 5,

    // ===== Speed =====
    .baudRate = 12 * 1000 * 1000
};
#endif


//== Battery =================================================
constexpr PRUZEA::PicoPlatform::BatteryConfig BATTERY_CONFIG {
    // ===== ADC =====
    .adcPin = -1,
    .adcChannel = 2,
    .voltageCalibrationFactor = 1.0f,
    .voltageCalibrationOffset = 0.0f, 
 
    // ===== Voltage Thresholds =====
    .externalPowerThresholdVoltage = 2.0f,
    .warningVoltage = 3.6f,
    .criticalVoltage = 3.5f,
    .cutoffVoltage = 3.4f
};

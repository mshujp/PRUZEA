#pragma once

#include "GraphicsBase.h"
#include <LovyanGFX.hpp>

namespace PRUZEA {

class LGFX_SSD1306 : public lgfx::LGFX_Device {
private:
    lgfx::Panel_SSD1306 panel;
    lgfx::Bus_I2C bus;

public:
    LGFX_SSD1306(int8_t sdaPin, int8_t sclPin, uint8_t i2cPort, uint8_t i2cAddr = 0x3C, int8_t rstPin = -1) {
        {
            auto cfg = bus.config();
            cfg.i2c_port = i2cPort;
            cfg.freq_write = 400000;
            cfg.freq_read = 400000;
            cfg.pin_sda = sdaPin;
            cfg.pin_scl = sclPin;
            cfg.i2c_addr = i2cAddr;
            bus.config(cfg);
            panel.setBus(&bus);
        }
        {
            auto cfg = panel.config();
            cfg.pin_rst = rstPin;
            cfg.panel_width = 128;
            cfg.panel_height = 64;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.invert = false;
            panel.config(cfg);
        }

        setPanel(&panel);
    }
};

class GraphicsSSD1306 : public GraphicsBase {
private:
    LGFX_SSD1306 lcd;
    LGFX_Sprite canvas;
    uint8_t rotate;

    uint16_t mono(Graphics::Color color) const;
    void setFont(const char* str, Font font);

    uint16_t* spriteSheetBuf = nullptr;
    uint32_t spriteSheetBufSize = 0;

public:
    struct Config {
        uint8_t i2cPort = 0;
        uint8_t i2cAddr = 0x3C;
        int8_t sdaPin   = -1;
        int8_t sclPin   = -1;
        int8_t resetPin   = -1;
        uint8_t oledRotate = 0;
    };
    explicit GraphicsSSD1306(const Config& config);

    uint16_t getScreenWidth() const override { return Display::SSD1306_SCREEN_W; }
    uint16_t getScreenHeight() const override { return Display::SSD1306_SCREEN_H; }
    uint16_t getLogicalScreenWidth() const override { return logicalScreenW; }
    uint16_t getLogicalScreenHeight() const override { return logicalScreenH; }
    const char* getName() const override { return "SSD1306"; }
    CatalogFilterMode getCatalogFilterMode() const override { return CatalogFilterMode::ExactMatch; }
    bool begin() override;
    void end() override;

    bool setLogicalScreenSize(uint16_t w, uint16_t h) override;

    void clearScreen() override;
    void fillScreen(Graphics::Color color) override;

    void drawPixel(int16_t x, int16_t y, Graphics::Color color) override;
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color) override;
    void fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Graphics::Color color) override {};
    void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Graphics::Color color) override;
    void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color) override;
    void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Graphics::Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color) override;
    void drawString(const char* str, int16_t x, int16_t y, Graphics::Color color, Font font) override;
    uint16_t getTextWidth(const char* text, Font font) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options) override;
    void drawSprite(const SpriteSheet& sheet, uint16_t column, uint16_t row, int16_t x, int16_t y, const SpriteOptions& options) override;
    void setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h) override;
    void getClipRect(int16_t& x, int16_t& y, uint16_t& w, uint16_t& h) override;
    void resetClipRect()  override;

    bool readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount) override;
    void push() override;
};

} // namespace PRUZEA

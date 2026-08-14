#pragma once

#include "GraphicsBase.h"
#include <LovyanGFX.hpp>
#include <memory>

namespace PRUZEA {

class LGFXContext;

class GraphicsILI9341 : public GraphicsBase {
private:
    std::unique_ptr<LGFXContext> lgfxContext;
    const char* driverName;
    uint8_t lcdRotate = 0;
    int8_t backLightPin = -1;

    const uint16_t MAX_BUF_WIDTH;
    const uint16_t MAX_BUF_HEIGHT;
 
    LGFX_Sprite canvas;

    uint8_t scale = 1;
    void setTransformInfo();
  
    void initSprite(LGFX_Sprite& sprite, uint16_t w, uint16_t h, bool psram = false);
    void setFont(const char* str, Font font);
  
    uint16_t* spriteSheetBuf = nullptr;
    uint32_t spriteSheetBufSize = 0;

public:
    struct GraphicsILI9341SPIConfig
    {
        uint8_t spiHost = 0;
        uint32_t spiWriteFreq = 60000000;
        int8_t clkPin = -1;
        int8_t dataPin = -1;
        int8_t dcPin = -1;
        int8_t csPin = -1;
        int8_t resetPin = -1;
        int8_t backlightPin = -1;
        uint8_t lcdRotate = 0;
        uint16_t maxBufferWidth = 0;
        uint16_t maxBufferHeight = 0;
    };
    struct GraphicsILI9341ParallelConfig
    {
        uint32_t writeFreq = 10000000;
        int8_t dataPinBase = -1;
        int8_t wrPin = -1;
        int8_t rdPin = -1;
        int8_t dcPin = -1;
        int8_t csPin = -1;
        int8_t resetPin = -1;
        int8_t backlightPin = -1;
        uint8_t lcdRotate = 0;
        uint16_t maxBufferWidth = 0;
        uint16_t maxBufferHeight = 0;
    };
    explicit GraphicsILI9341(const GraphicsILI9341SPIConfig& config);
    explicit GraphicsILI9341(const GraphicsILI9341ParallelConfig& config);
    ~GraphicsILI9341() override;

    LGFXContext& getLGFXContext() { return *lgfxContext; }
    const LGFXContext& getLGFXContext() const { return *lgfxContext; }
    uint16_t getScreenWidth() const override { return Display::ILI9341_SCREEN_W; }
    uint16_t getScreenHeight() const override { return Display::ILI9341_SCREEN_H; }
    uint16_t getLogicalScreenWidth() const override { return logicalScreenW; };
    uint16_t getLogicalScreenHeight() const override { return logicalScreenH; };
 
    const char* getName() const override { return driverName; }
    CatalogFilterMode getCatalogFilterMode() const override { return CatalogFilterMode::FitInside; }
    bool begin() override;
    void end() override;
 
    bool setLogicalScreenSize(uint16_t logicalScreenW, uint16_t logicalScreenH) override;
    void clearScreen() override;
    void fillScreen(Graphics::Color color) override;
    void drawPixel(int16_t x, int16_t y, Graphics::Color color) override;
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) override;
    void drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color) override;
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color) override;
    void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color) override;
    void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Graphics::Color color) override;
    void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color) override;
    void fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Graphics::Color color) override;
    void fillRectGradient(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color0, Color color1, FillStyle style) override;
    void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Graphics::Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t r, uint8_t w, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t w, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t r, uint8_t w, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t w, float angle0, float angle1, Color color) override;
    void drawString(const char* str, int16_t x, int16_t y, Graphics::Color color, Font font) override;
    uint16_t getTextWidth(const char* text, Font font) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options) override;
    void drawSprite(const SpriteSheet& sheet, uint16_t column, uint16_t row, int16_t x, int16_t y, const SpriteOptions& options) override;
    void drawImage(const Image& image, int16_t x, int16_t y) override;
    void setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h) override;
    void getClipRect(int16_t& x, int16_t& y, uint16_t& w, uint16_t& h) override;
    void resetClipRect()  override;

    bool readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount) override;
    void push() override;
};

} // namespace

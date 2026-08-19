#pragma once

#include "GraphicsBase.h"
#include <SDL3/SDL.h>

#include <cstdint>
#include <vector>

namespace PRUZEA {

class GraphicsSDL : public GraphicsBase
{
public:
    struct Config
    {
        const char* windowTitle = "PRUZEA";
        uint8_t windowScale = 3;
        uint16_t maxBufferWidth = Graphics::ILI9341_SCREEN_BUF_W_MAX_RP2350;
        uint16_t maxBufferHeight = Graphics::ILI9341_SCREEN_BUF_H_MAX_RP2350;
    };

    GraphicsSDL();
    explicit GraphicsSDL(const Config& config);
    ~GraphicsSDL() override;

    const char* getName() const override { return "SDL"; }
    CatalogFilterMode getCatalogFilterMode() const override { return CatalogFilterMode::FitInside; }

    bool begin() override;
    void end() override;

    uint16_t getScreenWidth() const override { return Display::ILI9341_SCREEN_W; }
    uint16_t getScreenHeight() const override { return Display::ILI9341_SCREEN_H; }
    uint16_t getLogicalScreenWidth() const override { return logicalScreenW; }
    uint16_t getLogicalScreenHeight() const override { return logicalScreenH; }

    bool setLogicalScreenSize(uint16_t w, uint16_t h) override;

    void clearScreen() override;
    void fillScreen(Color color) override;
    void drawPixel(int16_t x, int16_t y, Color color) override;
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) override;
    void drawWideLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, Color color) override;
    void drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) override;
    void drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color) override;
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) override;
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) override;
    void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color) override;
    void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Color color) override;
    void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color) override;
    void fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Color color) override;
    void fillRectGradient(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color0, Color color1, FillStyle style) override;
    void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t radius, Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t r, Color color) override;
    void drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t r, Color color) override;
    void fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t r, uint8_t width, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Color color) override;
    void drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t width, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t r, uint8_t width, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Color color) override;
    void fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t width, float angle0, float angle1, Color color) override;
    void drawString(const char* str, int16_t x, int16_t y, Color color, Font font) override;
    uint16_t getTextWidth(const char* text, Font font) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h) override;
    void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options) override;
    void drawSprite(const SpriteSheet& sheet, uint16_t column, uint16_t row, int16_t x, int16_t y, const SpriteOptions& options) override;
    void drawImage(const Image& image, int16_t x, int16_t y) override;

    void setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h) override;
    void getClipRect(int16_t& x, int16_t& y, uint16_t& w, uint16_t& h) override;
    void resetClipRect() override;

    bool readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount) override;
    void push() override;

private:
    Config config;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    std::vector<uint16_t> framebuffer;
    std::vector<uint16_t> presentBuffer;

    uint16_t maxBufferWidth;
    uint16_t maxBufferHeight;
    uint8_t displayScale = 1;

    bool clipEnabled = false;
    int16_t clipX = 0;
    int16_t clipY = 0;
    uint16_t clipW = 0;
    uint16_t clipH = 0;

    std::vector<uint16_t> spriteSheetBuffer;

    void updateDisplayScale();
    bool insideClip(int x, int y) const;
    bool getClipBounds(int& left, int& top, int& right, int& bottom) const;
    bool clipLine(int& x0, int& y0, int& x1, int& y1) const;
    void putPixelRaw(int x, int y, Color color);
    uint16_t getPixelRaw(int x, int y) const;
    void blendPixelRaw(int x, int y, Color color, uint8_t alpha);

    static Color blend565(Color dst, Color src, uint8_t alpha);
    static Color lerp565(Color a, Color b, float t);
    static float normalizeAngle(float angle);
    static bool angleInside(float angle, float start, float end);
    void drawLineRaw(int x0, int y0, int x1, int y1, Color color);
    void fillEllipseRaw(int cx, int cy, int rx, int ry, Color color);
    void drawEllipseRaw(int cx, int cy, int rx, int ry, Color color);
    void drawArcRaw(int cx, int cy, int rx, int ry, int width, float angle0, float angle1, Color color, bool filled);
};

} // namespace PRUZEA

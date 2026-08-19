#include "GraphicsILI9341.h"
#include "lgfx/LGFXContext.h"
#include "lgfx/LGFXContextParallel.h"
#include "lgfx/LGFXContextSPI.h"
#include <algorithm>

#include "pico/stdlib.h"

using namespace PRUZEA;

namespace
{
template<typename T>
bool intersects(const T& clip, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    return left < clip.right && right > clip.left && top < clip.bottom && bottom > clip.top;
}

template<typename T>
bool clipFilledRect(const T& clip, int32_t& x, int32_t& y, int32_t& w, int32_t& h)
{
    const int32_t right = std::min<int32_t>(x + w, clip.right);
    const int32_t bottom = std::min<int32_t>(y + h, clip.bottom);
    x = std::max<int32_t>(x, clip.left);
    y = std::max<int32_t>(y, clip.top);
    w = right - x;
    h = bottom - y;
    return w > 0 && h > 0;
}

}

GraphicsILI9341::GraphicsILI9341(const GraphicsILI9341SPIConfig& config)
    : lgfxContext(std::make_unique<LGFXContextSPI>(config)),
        driverName("ILI9341 SPI"),
        lcdRotate(config.lcdRotate), backLightPin(config.backlightPin),
        canvas(lgfxContext.get()), MAX_BUF_WIDTH(config.maxBufferWidth), MAX_BUF_HEIGHT(config.maxBufferHeight)
{
    logicalScreenW = 0;
    logicalScreenH = 0;
}
GraphicsILI9341::GraphicsILI9341(const GraphicsILI9341ParallelConfig& config)
    : lgfxContext(std::make_unique<LGFXContextParallel>(config)),
        driverName("ILI9341 Parallel"),
        lcdRotate(config.lcdRotate), backLightPin(config.backlightPin),
        canvas(lgfxContext.get()), MAX_BUF_WIDTH(config.maxBufferWidth), MAX_BUF_HEIGHT(config.maxBufferHeight)
{
    logicalScreenW = 0;
    logicalScreenH = 0;
}
GraphicsILI9341::~GraphicsILI9341() = default;

void GraphicsILI9341::setTransformInfo()
{
    uint16_t screenW = getScreenWidth();
    uint16_t screenH = getScreenHeight();

    scale = 1;
    if (logicalScreenW < screenW)
    {
        if (logicalScreenW *2 <= screenW && logicalScreenH * 2 <= screenH)
        {
            scale = 2;
        }
    }
}

void GraphicsILI9341::initSprite(LGFX_Sprite& sprite, uint16_t w, uint16_t h, bool psram)
{
    sprite.setColorDepth(lgfxContext->getColorDepth());
    if (psram) sprite.setPsram(true);
    sprite.createSprite(w, h);
}

bool GraphicsILI9341::setLogicalScreenSize(uint16_t _logicalScreenW, uint16_t _logicalScreenH)
{
    if (spriteSheetBuf != nullptr)
    {
        delete[] spriteSheetBuf;
        spriteSheetBuf = nullptr;
        spriteSheetBufSize = 0;
    }
    resetCamera();

    _logicalScreenW = std::clamp(_logicalScreenW, static_cast<uint16_t>(0), MAX_BUF_WIDTH);
    _logicalScreenH = std::clamp(_logicalScreenH, static_cast<uint16_t>(0), MAX_BUF_HEIGHT);
    if (_logicalScreenW == 0 || _logicalScreenH == 0) return true;

    if (logicalScreenW != _logicalScreenW || logicalScreenH != _logicalScreenH)
    {
        if (logicalScreenW > 0)
        {
            canvas.deleteSprite();
            lgfxContext->clear();
        }
        initSprite(canvas, _logicalScreenW, _logicalScreenH);
        lgfxContext->fillScreen(Graphics::Color::DARKGRAY);
    }
    logicalScreenW = _logicalScreenW;
    logicalScreenH = _logicalScreenH;
    setTransformInfo();
    resetClipRect();

    return canvas.getBuffer() != nullptr;
}

bool GraphicsILI9341::begin()
{
    const bool lcdInitialized = lgfxContext->init();
    if (!lcdInitialized) return false;

    lgfxContext->setRotation(lcdRotate);
    lgfxContext->finalizeTouchInitialization();
    canvas.setSwapBytes(true);

    if (backLightPin == 0)
    {
        gpio_init(backLightPin);
        gpio_set_dir(backLightPin, GPIO_OUT);
        gpio_put(backLightPin, 1);
    }

    resetCamera();
    return true;
}

void GraphicsILI9341::end()
{
    canvas.deleteSprite();
    lgfxContext->clear();
    if (backLightPin > 0) gpio_put(backLightPin, 0);
    screenDirty = false;
    delete[] spriteSheetBuf;
    spriteSheetBuf = nullptr;
    spriteSheetBufSize = 0;
}

void GraphicsILI9341::clearScreen()
{
    canvas.clear();
    screenDirty = true;
}

void GraphicsILI9341::fillScreen(Graphics::Color color)
{
    canvas.fillScreen(color);
    screenDirty = true;
}

void GraphicsILI9341::drawPixel(int16_t x, int16_t y, Graphics::Color color)
{
    const int32_t screenX = toScreenX(x);
    const int32_t screenY = toScreenY(y);
    const ClipBounds& clip = cachedClip;
    if (!intersects(clip, screenX, screenY, screenX + 1, screenY + 1)) return;

    canvas.drawPixel(screenX, screenY, color);
    screenDirty = true;
}

void GraphicsILI9341::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    const int32_t screenX1 = toScreenX(x1);
    const int32_t screenY1 = toScreenY(y1);
    const int32_t screenX2 = toScreenX(x2);
    const int32_t screenY2 = toScreenY(y2);
    const ClipBounds& clip = cachedClip;
    if (!intersects(clip, std::min(screenX1, screenX2), std::min(screenY1, screenY2),
                    std::max(screenX1, screenX2) + 1, std::max(screenY1, screenY2) + 1)) return;

    canvas.drawLine(screenX1, screenY1, screenX2, screenY2, color);
    screenDirty = true;
}

void GraphicsILI9341::drawWideLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, Graphics::Color color)
{
    if (thickness == 0) return;

    const float radius = toScreenScale(static_cast<float>(thickness)) * 0.5f;
    const int32_t screenX0 = toScreenX(x0);
    const int32_t screenY0 = toScreenY(y0);
    const int32_t screenX1 = toScreenX(x1);
    const int32_t screenY1 = toScreenY(y1);
    const int32_t margin = static_cast<int32_t>(radius + 1.0f);
    const ClipBounds& clip = cachedClip;
    if (!intersects(clip, std::min(screenX0, screenX1) - margin, std::min(screenY0, screenY1) - margin,
                    std::max(screenX0, screenX1) + margin + 1, std::max(screenY0, screenY1) + margin + 1)) return;

    canvas.drawWideLine(screenX0, screenY0, screenX1, screenY1, radius, color);
    screenDirty = true;
}

void GraphicsILI9341::drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    const int32_t sx0 = toScreenX(x0), sy0 = toScreenY(y0);
    const int32_t sx1 = toScreenX(x1), sy1 = toScreenY(y1);
    const int32_t sx2 = toScreenX(x2), sy2 = toScreenY(y2);
    if (!intersects(cachedClip, std::min({sx0, sx1, sx2}), std::min({sy0, sy1, sy2}),
                    std::max({sx0, sx1, sx2}) + 1, std::max({sy0, sy1, sy2}) + 1)) return;

    canvas.drawBezier(sx0, sy0, sx1, sy1, sx2, sy2, color);
    screenDirty = true;
}

void GraphicsILI9341::drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Graphics::Color color)
{
    const int32_t sx0 = toScreenX(x0), sy0 = toScreenY(y0);
    const int32_t sx1 = toScreenX(x1), sy1 = toScreenY(y1);
    const int32_t sx2 = toScreenX(x2), sy2 = toScreenY(y2);
    const int32_t sx3 = toScreenX(x3), sy3 = toScreenY(y3);
    if (!intersects(cachedClip, std::min({sx0, sx1, sx2, sx3}), std::min({sy0, sy1, sy2, sy3}),
                    std::max({sx0, sx1, sx2, sx3}) + 1, std::max({sy0, sy1, sy2, sy3}) + 1)) return;

    canvas.drawBezier(sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3, color);
    screenDirty = true;
}

void GraphicsILI9341::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    const int32_t sx0 = toScreenX(x0), sy0 = toScreenY(y0);
    const int32_t sx1 = toScreenX(x1), sy1 = toScreenY(y1);
    const int32_t sx2 = toScreenX(x2), sy2 = toScreenY(y2);
    const ClipBounds& clip = cachedClip;
    if (!intersects(clip, std::min({sx0, sx1, sx2}), std::min({sy0, sy1, sy2}),
                    std::max({sx0, sx1, sx2}) + 1, std::max({sy0, sy1, sy2}) + 1)) return;

    canvas.drawTriangle(sx0, sy0, sx1, sy1, sx2, sy2, color);
    screenDirty = true;
}

void GraphicsILI9341::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    const int32_t sx0 = toScreenX(x0), sy0 = toScreenY(y0);
    const int32_t sx1 = toScreenX(x1), sy1 = toScreenY(y1);
    const int32_t sx2 = toScreenX(x2), sy2 = toScreenY(y2);
    const ClipBounds& clip = cachedClip;
    if (!intersects(clip, std::min({sx0, sx1, sx2}), std::min({sy0, sy1, sy2}),
                    std::max({sx0, sx1, sx2}) + 1, std::max({sy0, sy1, sy2}) + 1)) return;

    canvas.fillTriangle(sx0, sy0, sx1, sy1, sx2, sy2, color);
    screenDirty = true;
}

void GraphicsILI9341::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color)
{
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    const int32_t screenW = toScreenW(w), screenH = toScreenH(h);
    if (!intersects(cachedClip, screenX, screenY, screenX + screenW, screenY + screenH)) return;

    canvas.drawRect(screenX, screenY, screenW, screenH, color);
    screenDirty = true;
}

void GraphicsILI9341::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Graphics::Color color)
{
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    const int32_t screenW = toScreenW(w), screenH = toScreenH(h);
    if (!intersects(cachedClip, screenX, screenY, screenX + screenW, screenY + screenH)) return;

    canvas.drawRoundRect(screenX, screenY, screenW, screenH, toScreenW(radius), color);
    screenDirty = true;
}

void GraphicsILI9341::fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color)
{
    int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    int32_t screenW = toScreenW(w), screenH = toScreenH(h);
    if (!clipFilledRect(cachedClip, screenX, screenY, screenW, screenH)) return;

    canvas.fillRect(screenX, screenY, screenW, screenH, color);
    screenDirty = true;
}

void GraphicsILI9341::fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Graphics::Color color)
{
    int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    int32_t screenW = toScreenW(w), screenH = toScreenH(h);
    if (!clipFilledRect(cachedClip, screenX, screenY, screenW, screenH)) return;

    canvas.fillRectAlpha(screenX, screenY, screenW, screenH, alpha, color);
    screenDirty = true;
}

void GraphicsILI9341::fillRectGradient(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color0,  Color color1, FillStyle style)
{
    lgfx::gradient_fill_styles::fill_style_t s;
    switch (style)
    {
    case VERTICAL_LINEAR: s = lgfx::gradient_fill_styles::vertical_linear; break;
    case RADIAL_CENTER:   s = lgfx::gradient_fill_styles::radial_center; break;
    default:              s = lgfx::gradient_fill_styles::horizontal_linear; break;
    }
    canvas.fillGradientRect(toScreenX(x), toScreenY(y), toScreenW(w), toScreenH(h), color0, color1, s);
    screenDirty = true;
}
 
void GraphicsILI9341::fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t radius, Graphics::Color color)
{
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    const int32_t screenW = toScreenW(w), screenH = toScreenH(h);
    if (!intersects(cachedClip, screenX, screenY, screenX + screenW, screenY + screenH)) return;

    canvas.fillRoundRect(screenX, screenY, screenW, screenH, toScreenW(radius), color);
    screenDirty = true;
}

void GraphicsILI9341::drawCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color)
{
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y), screenR = toScreenW(r);
    if (!intersects(cachedClip, screenX - screenR, screenY - screenR,
                    screenX + screenR + 1, screenY + screenR + 1)) return;

    canvas.drawCircle(screenX, screenY, screenR, color);
    screenDirty = true;
}

void GraphicsILI9341::drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color)
{
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    const int32_t screenRx = toScreenW(rx), screenRy = toScreenH(ry);
    if (!intersects(cachedClip, screenX - screenRx, screenY - screenRy,
                    screenX + screenRx + 1, screenY + screenRy + 1)) return;

    canvas.drawEllipse(screenX, screenY, screenRx, screenRy, color);
    screenDirty = true;
}

void GraphicsILI9341::fillCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color)
{
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y), screenR = toScreenW(r);
    if (!intersects(cachedClip, screenX - screenR, screenY - screenR,
                    screenX + screenR + 1, screenY + screenR + 1)) return;

    canvas.fillCircle(screenX, screenY, screenR, color);
    screenDirty = true;
}

void GraphicsILI9341::fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color)
{
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    const int32_t screenRx = toScreenW(rx), screenRy = toScreenH(ry);
    if (!intersects(cachedClip, screenX - screenRx, screenY - screenRy,
                    screenX + screenRx + 1, screenY + screenRy + 1)) return;

    canvas.fillEllipse(screenX, screenY, screenRx, screenRy, color);
    screenDirty = true;
}

void GraphicsILI9341::drawArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1 = toScreenW(r);
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    if (!intersects(cachedClip, screenX - r1, screenY - r1, screenX + r1 + 1, screenY + r1 + 1)) return;

    canvas.drawArc(screenX, screenY, 0, r1, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::drawArc(int16_t x, int16_t y, uint16_t r, uint8_t width, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1 = toScreenW(r);
    const int32_t w  = toScreenW(width);
    int32_t r0 = r1 - w;
    if (r0 < 0) r0 = 0;

    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    if (!intersects(cachedClip, screenX - r1, screenY - r1, screenX + r1 + 1, screenY + r1 + 1)) return;

    canvas.drawArc(screenX, screenY, r0, r1, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1x = toScreenW(rx);
    const int32_t r1y = toScreenH(ry);
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    if (!intersects(cachedClip, screenX - r1x, screenY - r1y,
                    screenX + r1x + 1, screenY + r1y + 1)) return;

    canvas.drawEllipseArc(screenX, screenY, 0, r1x, 0, r1y, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t width, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1x = toScreenW(rx);
    const int32_t r1y = toScreenH(ry);
    const int32_t wx  = toScreenW(width);
    const int32_t wy  = toScreenH(width);
    int32_t r0x = r1x - wx;
    int32_t r0y = r1y - wy;
    if (r0x < 0) r0x = 0;
    if (r0y < 0) r0y = 0;

    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    if (!intersects(cachedClip, screenX - r1x, screenY - r1y,
                    screenX + r1x + 1, screenY + r1y + 1)) return;

    canvas.drawEllipseArc(screenX, screenY, r0x, r1x, r0y, r1y, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::fillArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1 = toScreenW(r);
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    if (!intersects(cachedClip, screenX - r1, screenY - r1, screenX + r1 + 1, screenY + r1 + 1)) return;

    canvas.fillArc(screenX, screenY, 0, r1, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::fillArc(int16_t x, int16_t y, uint16_t r, uint8_t width, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1 = toScreenW(r);
    const int32_t w  = toScreenW(width);
    int32_t r0 = r1 - w;
    if (r0 < 0) r0 = 0;

    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    if (!intersects(cachedClip, screenX - r1, screenY - r1, screenX + r1 + 1, screenY + r1 + 1)) return;

    canvas.fillArc(screenX, screenY, r0, r1, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1x = toScreenW(rx);
    const int32_t r1y = toScreenH(ry);
    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    if (!intersects(cachedClip, screenX - r1x, screenY - r1y,
                    screenX + r1x + 1, screenY + r1y + 1)) return;

    canvas.fillEllipseArc(screenX, screenY, 0, r1x, 0, r1y, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t width, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1x = toScreenW(rx);
    const int32_t r1y = toScreenH(ry);
    const int32_t wx  = toScreenW(width);
    const int32_t wy  = toScreenH(width);
    int32_t r0x = r1x - wx;
    int32_t r0y = r1y - wy;
    if (r0x < 0) r0x = 0;
    if (r0y < 0) r0y = 0;

    const int32_t screenX = toScreenX(x), screenY = toScreenY(y);
    if (!intersects(cachedClip, screenX - r1x, screenY - r1y,
                    screenX + r1x + 1, screenY + r1y + 1)) return;

    canvas.fillEllipseArc(screenX, screenY, r0x, r1x, r0y, r1y, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::setFont(const char* str, Font font)
{
    const lgfx::IFont* targetFont = &fonts::DejaVu12;
    float scaleS = 1.0;

    switch (font)
    {
        case Font::SIZE_10:  targetFont = &fonts::DejaVu9; break;
        case Font::SIZE_13:  targetFont = &fonts::DejaVu12; break;
        case Font::SIZE_18:  targetFont = &fonts::DejaVu18; break;
        case Font::SIZE_22:  targetFont = &fonts::DejaVu18; scaleS = 1.2; break;
        case Font::SIZE_22B: targetFont = &fonts::FreeSansBold9pt7b; scaleS = 1.2; break;
        case Font::SIZE_25:  targetFont = &fonts::DejaVu24; scaleS = 1.0; break;
        case Font::SIZE_25B: targetFont = &fonts::FreeSansBold18pt7b; scaleS = 0.7; break;
        case Font::SIZE_32:  targetFont = &fonts::DejaVu40; scaleS = 0.8; break;
        case Font::SIZE_32B: targetFont = &fonts::FreeSansBold18pt7b; scaleS = 0.911; break;
        case Font::SIZE_42:  targetFont = &fonts::DejaVu40; scaleS = 1.04; break;
        case Font::SIZE_42B: targetFont = &fonts::FreeSansBold18pt7b; scaleS = 1.25; break;
#if PRUZEA_ENABLE_JAPANESE_FONT
        case Font::SIZE_16J: targetFont = &fonts::efontJA_16; break;
        case Font::SIZE_20J: targetFont = &fonts::efontJA_16; scaleS = 1.25; break;
        case Font::SIZE_32J: targetFont = &fonts::efontJA_16; scaleS = 2; break;
#endif
        default: targetFont = &fonts::DejaVu9; break;
    }
 
    canvas.setFont(targetFont);
    canvas.setTextSize(toScreenScale(scaleS));
}

void GraphicsILI9341::drawString(const char* str, int16_t x, int16_t y, Graphics::Color color, Font font)
{
    if (str == nullptr) return;
    setFont(str, font);
    canvas.setTextColor(color);
    canvas.drawString(str, toScreenX(x), toScreenY(y));
    screenDirty = true;
}

uint16_t GraphicsILI9341::getTextWidth(const char* text, Font font)
{
    if (text == nullptr) return 0;
    setFont(text, font);
    return canvas.textWidth(text);
}

void GraphicsILI9341::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    drawSprite(bitmap, x, y, w, h, SpriteOptions{});
}

void GraphicsILI9341::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options)
{
    float scale = static_cast<float>(options.scale);
    scale = toScreenScale(scale);

    if (bitmap == nullptr || scale == 0) return;

    const bool transformed = scale != 1 || options.angle != 0.0f || options.flipX || options.flipY;

    if (!transformed)
    {
        if (options.transparent)
        {
            canvas.pushImage(toScreenX(x), toScreenY(y), w, h, bitmap, static_cast<uint16_t>(options.transparentColor));
        }
        else
        {
            canvas.pushImage(toScreenX(x), toScreenY(y), w, h, bitmap);
        }
    }
    else
    {
        const float zoomX = options.flipX ? - scale : scale;
        const float zoomY = options.flipY ? -scale : scale;
        const float destinationX = toScreenX(x) + (w * scale) * 0.5f;
        const float destinationY = toScreenY(y) + (h * scale) * 0.5f;
        const float pivotFixX = 0.5f * (scale - 1.0f);
        const float pivotFixY = 0.5f * (scale - 1.0f);

        if (options.transparent)
        {
            canvas.pushImageRotateZoom(destinationX + pivotFixX, destinationY + pivotFixY, w * 0.5f, h * 0.5f,
                                        Math::radToDeg(options.angle), zoomX, zoomY, w, h, bitmap,
                                        static_cast<uint16_t>(options.transparentColor));
        }
        else
        {
            canvas.pushImageRotateZoom(destinationX + pivotFixX, destinationY + pivotFixY, w * 0.5f, h * 0.5f,
                                        Math::radToDeg(options.angle), zoomX, zoomY, w, h, bitmap);
        }
    }

    screenDirty = true;
}

void GraphicsILI9341::drawSprite(const SpriteSheet& sheet, uint16_t column, uint16_t row, int16_t x, int16_t y, const SpriteOptions& options)
{
    if (sheet.bitmap == nullptr || options.scale == 0) return;
    if (sheet.spriteWidth == 0 || sheet.spriteHeight == 0) return;
    if (column >= sheet.columns || row >= sheet.rows) return;

    if (spriteSheetBuf == nullptr || spriteSheetBufSize < sheet.spriteWidth * sheet.spriteHeight)
    {
        if (spriteSheetBuf != nullptr)
        {
            delete[] spriteSheetBuf;
            spriteSheetBuf = nullptr;
            spriteSheetBufSize = 0;
        }
        spriteSheetBufSize = sheet.spriteWidth * sheet.spriteHeight;
        spriteSheetBuf = new (std::nothrow) uint16_t[spriteSheetBufSize];
        if (spriteSheetBuf == nullptr)
        {
            spriteSheetBufSize = 0;
            return;
        }
    }

    const uint32_t sheetWidthPixels = sheet.columns * sheet.spriteWidth;
    const uint32_t spriteBaseOffset = row * sheet.spriteHeight * sheetWidthPixels + column * sheet.spriteWidth;
    const uint16_t* spriteBitmap = sheet.bitmap + spriteBaseOffset;

    for (uint32_t i = 0; i <  sheet.spriteHeight; ++i)
    {
        const uint16_t* sourceRow = spriteBitmap + (i * sheet.spriteWidth * sheet.columns);
        const uint32_t destIndex = i * sheet.spriteWidth;
        std::copy(sourceRow, sourceRow + sheet.spriteWidth, spriteSheetBuf + destIndex);
    }

    drawSprite(spriteSheetBuf, x, y, sheet.spriteWidth, sheet.spriteHeight, options);
}

void GraphicsILI9341::drawImage(const Image& image, int16_t x, int16_t y)
{
    drawSprite(image.getBitmap(), x, y, image.getWidth(), image.getHeight());
}

void GraphicsILI9341::setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    canvas.setClipRect(x, y, w, h);
    cachedClip.left = std::max<int32_t>(0, x);
    cachedClip.top = std::max<int32_t>(0, y);
    cachedClip.right = std::min<int32_t>(canvas.width(), static_cast<int32_t>(x) + w);
    cachedClip.bottom = std::min<int32_t>(canvas.height(), static_cast<int32_t>(y) + h);
}

void GraphicsILI9341::getClipRect(int16_t& x, int16_t& y, uint16_t& w, uint16_t& h)
{
    x = static_cast<int16_t>(cachedClip.left);
    y = static_cast<int16_t>(cachedClip.top);
    w = static_cast<uint16_t>(std::max<int32_t>(0, cachedClip.right - cachedClip.left));
    h = static_cast<uint16_t>(std::max<int32_t>(0, cachedClip.bottom - cachedClip.top));
}

void GraphicsILI9341::resetClipRect()
{
    canvas.clearClipRect();
    cachedClip.left = 0;
    cachedClip.top = 0;
    cachedClip.right = canvas.width();
    cachedClip.bottom = canvas.height();
}

void GraphicsILI9341::push()
{
    if (!screenDirty) return;

    if (scale == 1 && (getScreenWidth() <= getLogicalScreenWidth() && getScreenHeight() <= getLogicalScreenHeight()))
    {
        canvas.pushSprite(lgfxContext.get(), -viewportX, -viewportY);
    }
    else
    {
        float screenCenterX = static_cast<float>(getScreenWidth()) / 2.0f;
        float screenCenterY = static_cast<float>(getScreenHeight()) / 2.0f;
        float targetX = screenCenterX - (static_cast<float>(viewportX) * static_cast<float>(scale));
        float targetY = screenCenterY - (static_cast<float>(viewportY) * static_cast<float>(scale));

        canvas.setPivot(canvas.width() / 2, canvas.height() / 2);
        canvas.pushRotateZoom(targetX, targetY, 0.0f, static_cast<float>(scale), static_cast<float>(scale));
    }

    screenDirty = false;
}

bool GraphicsILI9341::readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount)
{
    if (outPixels == nullptr) return false;

    const uint16_t screenW = getScreenWidth();
    const uint16_t screenH = getScreenHeight();
    if (y >= screenH || pixelCount < screenW || logicalScreenW == 0 || logicalScreenH == 0) return false;

    if (scale == 1)
    {
        const int32_t sourceY = static_cast<int32_t>(y) + viewportY;
        for (uint16_t x = 0; x < screenW; ++x)
        {
            const int32_t sourceX = static_cast<int32_t>(x) + viewportX;
            if (sourceX < 0 || sourceY < 0 || sourceX >= logicalScreenW || sourceY >= logicalScreenH)
            {
                outPixels[x] = static_cast<uint16_t>(Graphics::BLACK);
            }
            else
            {
                outPixels[x] = static_cast<uint16_t>(canvas.readPixel(sourceX, sourceY));
            }
        }
        return true;
    }

    const float screenCenterX = static_cast<float>(screenW) * 0.5f;
    const float screenCenterY = static_cast<float>(screenH) * 0.5f;
    const float targetX = screenCenterX - static_cast<float>(viewportX * scale);
    const float targetY = screenCenterY - static_cast<float>(viewportY * scale);
    const float pivotX = static_cast<float>(logicalScreenW) * 0.5f;
    const float pivotY = static_cast<float>(logicalScreenH) * 0.5f;
    const int32_t sourceY = static_cast<int32_t>((static_cast<float>(y) - targetY) / scale + pivotY);

    for (uint16_t x = 0; x < screenW; ++x)
    {
        const int32_t sourceX = static_cast<int32_t>((static_cast<float>(x) - targetX) / scale + pivotX);
        if (sourceX < 0 || sourceY < 0 || sourceX >= logicalScreenW || sourceY >= logicalScreenH)
        {
            outPixels[x] = static_cast<uint16_t>(Graphics::BLACK);
        }
        else
        {
            outPixels[x] = static_cast<uint16_t>(canvas.readPixel(sourceX, sourceY));
        }
    }

    return true;
}

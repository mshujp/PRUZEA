#include "GraphicsILI9341.h"
#include "lgfx/LGFXContext.h"
#include "lgfx/LGFXContextParallel.h"
#include "lgfx/LGFXContextSPI.h"
#include <algorithm>

#include "pico/stdlib.h"

using namespace PRUZEA;

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
    canvas.drawPixel(toScreenX(x), toScreenY(y), color);
    screenDirty = true;
}

void GraphicsILI9341::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    canvas.drawLine(toScreenX(x1), toScreenY(y1), toScreenX(x2), toScreenY(y2), color);
    screenDirty = true;
}

void GraphicsILI9341::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    canvas.drawTriangle(toScreenX(x0), toScreenY(y0), toScreenX(x1), toScreenY(y1), toScreenX(x2), toScreenY(y2), color);
    screenDirty = true;
}

void GraphicsILI9341::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Graphics::Color color)
{
    canvas.fillTriangle(toScreenX(x0), toScreenY(y0), toScreenX(x1), toScreenY(y1), toScreenX(x2), toScreenY(y2), color);
    screenDirty = true;
}

void GraphicsILI9341::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color)
{
    canvas.drawRect(toScreenX(x), toScreenY(y), toScreenW(w), toScreenH(h), color);
    screenDirty = true;
}

void GraphicsILI9341::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Graphics::Color color)
{
    canvas.drawRoundRect(toScreenX(x), toScreenY(y), toScreenW(w), toScreenH(h), toScreenW(radius), color);
    screenDirty = true;
}

void GraphicsILI9341::fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Graphics::Color color)
{
    canvas.fillRect(toScreenX(x), toScreenY(y), toScreenW(w), toScreenH(h), color);
    screenDirty = true;
}

void GraphicsILI9341::fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Graphics::Color color)
{
    canvas.fillRectAlpha(toScreenX(x), toScreenY(y), toScreenW(w), toScreenH(h), alpha, color);
    screenDirty = true;
}

void GraphicsILI9341::fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t radius, Graphics::Color color)
{
    canvas.fillRoundRect(toScreenX(x), toScreenY(y), toScreenW(w), toScreenH(h), toScreenW(radius), color);
    screenDirty = true;
}

void GraphicsILI9341::drawCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color)
{
    canvas.drawCircle(toScreenX(x), toScreenY(y), toScreenW(r), color);
    screenDirty = true;
}

void GraphicsILI9341::drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color)
{
    canvas.drawEllipse(toScreenX(x), toScreenY(y), toScreenW(rx), toScreenH(ry), color);
    screenDirty = true;
}

void GraphicsILI9341::fillCircle(int16_t x, int16_t y, uint16_t r, Graphics::Color color)
{
    canvas.fillCircle(toScreenX(x), toScreenY(y), toScreenW(r), color);
    screenDirty = true;
}

void GraphicsILI9341::fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Graphics::Color color)
{
    canvas.fillEllipse(toScreenX(x), toScreenY(y), toScreenW(rx), toScreenH(ry), color);
    screenDirty = true;
}

void GraphicsILI9341::drawArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1 = toScreenW(r);
    canvas.drawArc(toScreenX(x), toScreenY(y), 0, r1, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::drawArc(int16_t x, int16_t y, uint16_t r, uint8_t width, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1 = toScreenW(r);
    const int32_t w  = toScreenW(width);
    int32_t r0 = r1 - w;
    if (r0 < 0) r0 = 0;

    canvas.drawArc(toScreenX(x), toScreenY(y), r0, r1, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1x = toScreenW(rx);
    const int32_t r1y = toScreenH(ry);
    canvas.drawEllipseArc(toScreenX(x), toScreenY(y), 0, r1x, 0, r1y, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
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

    canvas.drawEllipseArc(toScreenX(x), toScreenY(y), r0x, r1x, r0y, r1y, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::fillArc(int16_t x, int16_t y, uint16_t r, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1 = toScreenW(r);
    canvas.fillArc(toScreenX(x), toScreenY(y), 0, r1, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::fillArc(int16_t x, int16_t y, uint16_t r, uint8_t width, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1 = toScreenW(r);
    const int32_t w  = toScreenW(width);
    int32_t r0 = r1 - w;
    if (r0 < 0) r0 = 0;

    canvas.fillArc(toScreenX(x), toScreenY(y), r0, r1, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
    screenDirty = true;
}

void GraphicsILI9341::fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float angle0, float angle1, Graphics::Color color)
{
    const int32_t r1x = toScreenW(rx);
    const int32_t r1y = toScreenH(ry);
    canvas.fillEllipseArc(toScreenX(x), toScreenY(y), 0, r1x, 0, r1y, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
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

    canvas.fillEllipseArc(toScreenX(x), toScreenY(y), r0x, r1x, r0y, r1y, Math::radToDeg(angle0), Math::radToDeg(angle1), color);
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
}

void GraphicsILI9341::getClipRect(int16_t& x, int16_t& y, uint16_t& w, uint16_t& h)
{
    int32_t cx;
    int32_t cy;
    int32_t cw;
    int32_t ch;
    canvas.getClipRect(&cx, &cx, &cw, &ch);
    x = static_cast<int16_t>(cx);
    y = static_cast<int16_t>(cy);
    w = static_cast<int16_t>(cw);
    h = static_cast<int16_t>(ch);
}

void GraphicsILI9341::resetClipRect()
{
    canvas.clearClipRect();
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

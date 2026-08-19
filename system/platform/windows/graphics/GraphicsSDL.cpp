#include "GraphicsSDL.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#define PROGMEM

struct GFXglyph
{
    uint16_t bitmapOffset;
    uint8_t width;
    uint8_t height;
    uint8_t xAdvance;
    int8_t xOffset;
    int8_t yOffset;
};

struct GFXfont
{
    uint8_t* bitmap;
    GFXglyph* glyph;
    uint16_t first;
    uint16_t last;
    uint8_t yAdvance;
};

#include "../Fonts/DejaVu9.h"
#include "../Fonts/DejaVu12.h"
#include "../Fonts/DejaVu18.h"
#include "../Fonts/DejaVu24.h"
#include "../Fonts/DejaVu40.h"
#include "../Fonts/FreeSansBold9pt7b.h"
#include "../Fonts/FreeSansBold12pt7b.h"
#include "../Fonts/FreeSansBold18pt7b.h"

#undef PROGMEM

using namespace PRUZEA;

namespace {

struct FontSelection
{
    const GFXfont* font;
    float scale;
};

FontSelection selectFont(Graphics::Font font)
{
    switch (font)
    {
        case Graphics::Font::SIZE_10:  return {&DejaVu9, 1.0f};
        case Graphics::Font::SIZE_13:  return {&DejaVu12, 1.0f};
        case Graphics::Font::SIZE_18:  return {&DejaVu18, 1.0f};
        case Graphics::Font::SIZE_22:  return {&DejaVu18, 1.2f};
        case Graphics::Font::SIZE_22B: return {&FreeSansBold9pt7b, 1.2f};
        case Graphics::Font::SIZE_25:  return {&DejaVu24, 1.0f};
        case Graphics::Font::SIZE_25B: return {&FreeSansBold18pt7b, 0.7f};
        case Graphics::Font::SIZE_32:  return {&DejaVu40, 0.8f};
        case Graphics::Font::SIZE_32B: return {&FreeSansBold18pt7b, 0.911f};
        case Graphics::Font::SIZE_42:  return {&DejaVu40, 1.04f};
        case Graphics::Font::SIZE_42B: return {&FreeSansBold18pt7b, 1.25f};
        case Graphics::Font::SIZE_16J: return {&DejaVu18, 16.0f / 18.0f};
        case Graphics::Font::SIZE_20J: return {&DejaVu18, 20.0f / 18.0f};
        case Graphics::Font::SIZE_32J: return {&DejaVu24, 32.0f / 25.0f};
        default:                       return {&DejaVu9, 1.0f};
    }
}

const GFXglyph& selectGlyph(const GFXfont& font, unsigned char character)
{
    if (character < font.first || character > font.last) character = '?';
    return font.glyph[character - font.first];
}

int fontAscent(const GFXfont& font)
{
    int ascent = 0;
    for (uint16_t character = font.first; character <= font.last; ++character)
    {
        ascent = std::max(ascent, -static_cast<int>(font.glyph[character - font.first].yOffset));
    }
    return ascent;
}

int scaledGlyphAdvance(const GFXglyph& glyph, float scale)
{
    return scale >= 1.0f
        ? static_cast<int>(std::ceil(glyph.xAdvance * scale))
        : static_cast<int>(glyph.xAdvance * scale);
}

} // namespace

GraphicsSDL::GraphicsSDL()
    : GraphicsSDL(Config{})
{
}

GraphicsSDL::GraphicsSDL(const Config& cfg)
    : config(cfg),
      maxBufferWidth(cfg.maxBufferWidth),
      maxBufferHeight(cfg.maxBufferHeight)
{
}

GraphicsSDL::~GraphicsSDL()
{
    end();
}

bool GraphicsSDL::begin()
{
    if (window != nullptr) return true;

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        return false;
    }

    const int windowW = getScreenWidth() * std::max<int>(1, config.windowScale);
    const int windowH = getScreenHeight() * std::max<int>(1, config.windowScale);

    if (!SDL_CreateWindowAndRenderer(
            config.windowTitle != nullptr ? config.windowTitle : "PRUZEA",
            windowW,
            windowH,
            SDL_WINDOW_RESIZABLE,
            &window,
            &renderer))
    {
        window = nullptr;
        renderer = nullptr;
        return false;
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        getScreenWidth(),
        getScreenHeight());

    if (texture == nullptr)
    {
        end();
        return false;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    presentBuffer.assign(getScreenWidth() * getScreenHeight(), static_cast<uint16_t>(BLACK));
    resetCamera();
    resetClipRect();
    return true;
}

void GraphicsSDL::end()
{
    if (texture != nullptr)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (renderer != nullptr)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    framebuffer.clear();
    presentBuffer.clear();
    spriteSheetBuffer.clear();
    logicalScreenW = 0;
    logicalScreenH = 0;
    screenDirty = false;
}

bool GraphicsSDL::setLogicalScreenSize(uint16_t w, uint16_t h)
{
    resetCamera();

    w = std::min(w, maxBufferWidth);
    h = std::min(h, maxBufferHeight);
    if (w == 0 || h == 0) return true;

    if (logicalScreenW != w || logicalScreenH != h)
    {
        try
        {
            framebuffer.assign(static_cast<size_t>(w) * h, static_cast<uint16_t>(BLACK));
        }
        catch (...)
        {
            return false;
        }
    }

    if (!GraphicsBase::setLogicalScreenSize(w, h))
    {
        return false;
    }

    updateDisplayScale();
    resetClipRect();
    screenDirty = true;
    return true;
}

void GraphicsSDL::updateDisplayScale()
{
    displayScale = 1;
    if (logicalScreenW * 2 <= getScreenWidth() &&
        logicalScreenH * 2 <= getScreenHeight())
    {
        displayScale = 2;
    }
}

bool GraphicsSDL::insideClip(int x, int y) const
{
    if (x < 0 || y < 0 || x >= logicalScreenW || y >= logicalScreenH) return false;
    if (!clipEnabled) return true;

    return x >= clipX && y >= clipY &&
           x < clipX + static_cast<int>(clipW) &&
           y < clipY + static_cast<int>(clipH);
}

bool GraphicsSDL::getClipBounds(int& left, int& top, int& right, int& bottom) const
{
    left = 0;
    top = 0;
    right = static_cast<int>(logicalScreenW) - 1;
    bottom = static_cast<int>(logicalScreenH) - 1;
    if (clipEnabled)
    {
        left = std::max(left, static_cast<int>(clipX));
        top = std::max(top, static_cast<int>(clipY));
        right = std::min(right, static_cast<int>(clipX) + static_cast<int>(clipW) - 1);
        bottom = std::min(bottom, static_cast<int>(clipY) + static_cast<int>(clipH) - 1);
    }
    return left <= right && top <= bottom;
}

bool GraphicsSDL::clipLine(int& x0, int& y0, int& x1, int& y1) const
{
    int left, top, right, bottom;
    if (!getClipBounds(left, top, right, bottom)) return false;

    const double startX = x0;
    const double startY = y0;
    const double dx = static_cast<double>(x1) - startX;
    const double dy = static_cast<double>(y1) - startY;
    double first = 0.0;
    double last = 1.0;

    auto clipBoundary = [&](double p, double q)
    {
        if (p == 0.0) return q >= 0.0;
        const double ratio = q / p;
        if (p < 0.0)
        {
            if (ratio > last) return false;
            first = std::max(first, ratio);
        }
        else
        {
            if (ratio < first) return false;
            last = std::min(last, ratio);
        }
        return true;
    };

    if (!clipBoundary(-dx, startX - left) || !clipBoundary(dx, right - startX) ||
        !clipBoundary(-dy, startY - top) || !clipBoundary(dy, bottom - startY)) return false;

    x0 = static_cast<int>(std::round(startX + dx * first));
    y0 = static_cast<int>(std::round(startY + dy * first));
    x1 = static_cast<int>(std::round(startX + dx * last));
    y1 = static_cast<int>(std::round(startY + dy * last));
    x0 = std::clamp(x0, left, right);
    y0 = std::clamp(y0, top, bottom);
    x1 = std::clamp(x1, left, right);
    y1 = std::clamp(y1, top, bottom);
    return true;
}

void GraphicsSDL::putPixelRaw(int x, int y, Color color)
{
    if (!insideClip(x, y)) return;
    framebuffer[static_cast<size_t>(y) * logicalScreenW + x] = static_cast<uint16_t>(color);
}

uint16_t GraphicsSDL::getPixelRaw(int x, int y) const
{
    if (x < 0 || y < 0 || x >= logicalScreenW || y >= logicalScreenH)
    {
        return static_cast<uint16_t>(BLACK);
    }
    return framebuffer[static_cast<size_t>(y) * logicalScreenW + x];
}

Graphics::Color GraphicsSDL::blend565(Color dst, Color src, uint8_t alpha)
{
    if (alpha == 0) return dst;
    if (alpha == 255) return src;

    const int dr = (static_cast<uint16_t>(dst) >> 11) & 0x1f;
    const int dg = (static_cast<uint16_t>(dst) >> 5) & 0x3f;
    const int db = static_cast<uint16_t>(dst) & 0x1f;

    const int sr = (static_cast<uint16_t>(src) >> 11) & 0x1f;
    const int sg = (static_cast<uint16_t>(src) >> 5) & 0x3f;
    const int sb = static_cast<uint16_t>(src) & 0x1f;

    const int r = (dr * (255 - alpha) + sr * alpha + 127) / 255;
    const int g = (dg * (255 - alpha) + sg * alpha + 127) / 255;
    const int b = (db * (255 - alpha) + sb * alpha + 127) / 255;

    return static_cast<Color>((r << 11) | (g << 5) | b);
}

Graphics::Color GraphicsSDL::lerp565(Color a, Color b, float t)
{
    t = Math::clamp(t, 0.0f, 1.0f);
    return blend565(a, b, static_cast<uint8_t>(t * 255.0f + 0.5f));
}

void GraphicsSDL::blendPixelRaw(int x, int y, Color color, uint8_t alpha)
{
    if (!insideClip(x, y)) return;
    const size_t index = static_cast<size_t>(y) * logicalScreenW + x;
    framebuffer[index] = static_cast<uint16_t>(
        blend565(static_cast<Color>(framebuffer[index]), color, alpha));
}

void GraphicsSDL::clearScreen()
{
    fillScreen(BLACK);
}

void GraphicsSDL::fillScreen(Color color)
{
    std::fill(framebuffer.begin(), framebuffer.end(), static_cast<uint16_t>(color));
    screenDirty = true;
}

void GraphicsSDL::drawPixel(int16_t x, int16_t y, Color color)
{
    putPixelRaw(toScreenX(x), toScreenY(y), color);
    screenDirty = true;
}

void GraphicsSDL::drawLineRaw(int x0, int y0, int x1, int y1, Color color)
{
    if (!clipLine(x0, y0, x1, y1)) return;
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;)
    {
        putPixelRaw(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = err * 2;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void GraphicsSDL::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color)
{
    drawLineRaw(toScreenX(x1), toScreenY(y1), toScreenX(x2), toScreenY(y2), color);
    screenDirty = true;
}

void GraphicsSDL::drawWideLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, Color color)
{
    if (thickness == 0) return;

    const float sx0 = static_cast<float>(toScreenX(x0));
    const float sy0 = static_cast<float>(toScreenY(y0));
    const float sx1 = static_cast<float>(toScreenX(x1));
    const float sy1 = static_cast<float>(toScreenY(y1));
    const float radius = std::max(0.5f, toScreenScale(static_cast<float>(thickness)) * 0.5f);

    int clipLeft, clipTop, clipRight, clipBottom;
    if (!getClipBounds(clipLeft, clipTop, clipRight, clipBottom)) return;
    const int minX = std::max(clipLeft, static_cast<int>(Math::floor(std::min(sx0, sx1) - radius)));
    const int maxX = std::min(clipRight, static_cast<int>(Math::ceil(std::max(sx0, sx1) + radius)));
    const int minY = std::max(clipTop, static_cast<int>(Math::floor(std::min(sy0, sy1) - radius)));
    const int maxY = std::min(clipBottom, static_cast<int>(Math::ceil(std::max(sy0, sy1) + radius)));
    if (minX > maxX || minY > maxY) return;

    const float vx = sx1 - sx0;
    const float vy = sy1 - sy0;
    const float len2 = vx * vx + vy * vy;

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            float t = len2 > 0.0f ? ((x - sx0) * vx + (y - sy0) * vy) / len2 : 0.0f;
            t = Math::clamp(t, 0.0f, 1.0f);
            const float px = sx0 + vx * t;
            const float py = sy0 + vy * t;
            const float dx = x - px;
            const float dy = y - py;
            if (dx * dx + dy * dy <= radius * radius) putPixelRaw(x, y, color);
        }
    }

    screenDirty = true;
}

void GraphicsSDL::drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color)
{
    const int steps = 32;
    int lastX = toScreenX(x0);
    int lastY = toScreenY(y0);
    for (int i = 1; i <= steps; ++i)
    {
        const float t = static_cast<float>(i) / steps;
        const float u = 1.0f - t;
        const int px = static_cast<int>(Math::round(u*u*toScreenX(x0) + 2*u*t*toScreenX(x1) + t*t*toScreenX(x2)));
        const int py = static_cast<int>(Math::round(u*u*toScreenY(y0) + 2*u*t*toScreenY(y1) + t*t*toScreenY(y2)));
        drawLineRaw(lastX, lastY, px, py, color);
        lastX = px;
        lastY = py;
    }
    screenDirty = true;
}

void GraphicsSDL::drawBezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, Color color)
{
    const int steps = 40;
    int lastX = toScreenX(x0);
    int lastY = toScreenY(y0);
    for (int i = 1; i <= steps; ++i)
    {
        const float t = static_cast<float>(i) / steps;
        const float u = 1.0f - t;
        const int px = static_cast<int>(Math::round(
            u*u*u*toScreenX(x0) + 3*u*u*t*toScreenX(x1) +
            3*u*t*t*toScreenX(x2) + t*t*t*toScreenX(x3)));
        const int py = static_cast<int>(Math::round(
            u*u*u*toScreenY(y0) + 3*u*u*t*toScreenY(y1) +
            3*u*t*t*toScreenY(y2) + t*t*t*toScreenY(y3)));
        drawLineRaw(lastX, lastY, px, py, color);
        lastX = px;
        lastY = py;
    }
    screenDirty = true;
}

void GraphicsSDL::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color)
{
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void GraphicsSDL::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color)
{
    struct Vertex
    {
        int x;
        int y;
    };

    Vertex vertices[] = {
        {toScreenX(x0), toScreenY(y0)},
        {toScreenX(x1), toScreenY(y1)},
        {toScreenX(x2), toScreenY(y2)}
    };
    std::sort(std::begin(vertices), std::end(vertices), [](const Vertex& lhs, const Vertex& rhs)
    {
        return lhs.y < rhs.y;
    });

    int clipLeft = 0;
    int clipTop = 0;
    int clipRight = static_cast<int>(logicalScreenW) - 1;
    int clipBottom = static_cast<int>(logicalScreenH) - 1;
    if (clipEnabled)
    {
        clipLeft = std::max(clipLeft, static_cast<int>(clipX));
        clipTop = std::max(clipTop, static_cast<int>(clipY));
        clipRight = std::min(clipRight, static_cast<int>(clipX) + static_cast<int>(clipW) - 1);
        clipBottom = std::min(clipBottom, static_cast<int>(clipY) + static_cast<int>(clipH) - 1);
    }
    if (clipLeft > clipRight || clipTop > clipBottom) return;

    const Vertex& top = vertices[0];
    const Vertex& middle = vertices[1];
    const Vertex& bottom = vertices[2];
    const int firstY = std::max(top.y, clipTop);
    const int lastY = std::min(bottom.y, clipBottom);
    if (firstY > lastY) return;

    auto interpolateX = [](const Vertex& a, const Vertex& b, int y)
    {
        if (a.y == b.y) return static_cast<float>(std::min(a.x, b.x));
        const float ratio = static_cast<float>(y - a.y) / static_cast<float>(b.y - a.y);
        return a.x + (b.x - a.x) * ratio;
    };

    const uint16_t pixel = static_cast<uint16_t>(color);
    for (int y = firstY; y <= lastY; ++y)
    {
        const float longX = interpolateX(top, bottom, y);
        const float shortX = y < middle.y
            ? interpolateX(top, middle, y)
            : interpolateX(middle, bottom, y);
        const int spanLeft = std::max(clipLeft, static_cast<int>(std::ceil(std::min(longX, shortX))));
        const int spanRight = std::min(clipRight, static_cast<int>(std::floor(std::max(longX, shortX))));
        if (spanLeft > spanRight) continue;

        auto row = framebuffer.begin() + static_cast<size_t>(y) * logicalScreenW;
        std::fill(row + spanLeft, row + spanRight + 1, pixel);
    }
    screenDirty = true;
}

void GraphicsSDL::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color)
{
    const int sx = toScreenX(x), sy = toScreenY(y);
    const int sw = toScreenW(w), sh = toScreenH(h);
    if (sw <= 0 || sh <= 0) return;
    drawLineRaw(sx, sy, sx + sw - 1, sy, color);
    drawLineRaw(sx, sy, sx, sy + sh - 1, color);
    drawLineRaw(sx + sw - 1, sy, sx + sw - 1, sy + sh - 1, color);
    drawLineRaw(sx, sy + sh - 1, sx + sw - 1, sy + sh - 1, color);
    screenDirty = true;
}

void GraphicsSDL::fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color)
{
    const int sx = toScreenX(x), sy = toScreenY(y);
    const int sw = toScreenW(w), sh = toScreenH(h);
    if (sw <= 0 || sh <= 0) return;

    int left = std::max(0, sx);
    int top = std::max(0, sy);
    int right = std::min(static_cast<int>(logicalScreenW), sx + sw);
    int bottom = std::min(static_cast<int>(logicalScreenH), sy + sh);
    if (clipEnabled)
    {
        left = std::max(left, static_cast<int>(clipX));
        top = std::max(top, static_cast<int>(clipY));
        right = std::min(right, static_cast<int>(clipX) + static_cast<int>(clipW));
        bottom = std::min(bottom, static_cast<int>(clipY) + static_cast<int>(clipH));
    }
    if (left >= right || top >= bottom) return;

    const uint16_t pixel = static_cast<uint16_t>(color);
    for (int yy = top; yy < bottom; ++yy)
    {
        auto row = framebuffer.begin() + static_cast<size_t>(yy) * logicalScreenW;
        std::fill(row + left, row + right, pixel);
    }
    screenDirty = true;
}

void GraphicsSDL::fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Color color)
{
    const int sx = toScreenX(x), sy = toScreenY(y);
    const int sw = toScreenW(w), sh = toScreenH(h);
    if (sw <= 0 || sh <= 0) return;

    int left = std::max(0, sx);
    int top = std::max(0, sy);
    int right = std::min(static_cast<int>(logicalScreenW), sx + sw);
    int bottom = std::min(static_cast<int>(logicalScreenH), sy + sh);
    if (clipEnabled)
    {
        left = std::max(left, static_cast<int>(clipX));
        top = std::max(top, static_cast<int>(clipY));
        right = std::min(right, static_cast<int>(clipX) + static_cast<int>(clipW));
        bottom = std::min(bottom, static_cast<int>(clipY) + static_cast<int>(clipH));
    }
    if (left >= right || top >= bottom) return;

    for (int yy = top; yy < bottom; ++yy)
        for (int xx = left; xx < right; ++xx)
            blendPixelRaw(xx, yy, color, alpha);
    screenDirty = true;
}

void GraphicsSDL::fillRectGradient(int16_t x, int16_t y, uint16_t w, uint16_t h, Color c0, Color c1, FillStyle style)
{
    const int sx = toScreenX(x), sy = toScreenY(y);
    const int sw = toScreenW(w), sh = toScreenH(h);
    if (sw <= 0 || sh <= 0) return;

    const float cx = (sw - 1) * 0.5f;
    const float cy = (sh - 1) * 0.5f;
    const float maxR = Math::sqrt(cx * cx + cy * cy);

    int left = std::max(0, sx);
    int top = std::max(0, sy);
    int right = std::min(static_cast<int>(logicalScreenW), sx + sw);
    int bottom = std::min(static_cast<int>(logicalScreenH), sy + sh);
    if (clipEnabled)
    {
        left = std::max(left, static_cast<int>(clipX));
        top = std::max(top, static_cast<int>(clipY));
        right = std::min(right, static_cast<int>(clipX) + static_cast<int>(clipW));
        bottom = std::min(bottom, static_cast<int>(clipY) + static_cast<int>(clipH));
    }
    if (left >= right || top >= bottom) return;

    for (int destinationY = top; destinationY < bottom; ++destinationY)
    {
        const int yy = destinationY - sy;
        for (int destinationX = left; destinationX < right; ++destinationX)
        {
            const int xx = destinationX - sx;
            float t = 0.0f;
            if (style == VERTICAL_LINEAR)
                t = sh > 1 ? static_cast<float>(yy) / (sh - 1) : 0.0f;
            else if (style == RADIAL_CENTER)
            {
                const float dx = xx - cx, dy = yy - cy;
                t = maxR > 0.0f ? Math::sqrt(dx*dx + dy*dy) / maxR : 0.0f;
            }
            else
                t = sw > 1 ? static_cast<float>(xx) / (sw - 1) : 0.0f;

            putPixelRaw(destinationX, destinationY, lerp565(c0, c1, t));
        }
    }
    screenDirty = true;
}

void GraphicsSDL::fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t radius, Color color)
{
    const int sx = toScreenX(x), sy = toScreenY(y);
    const int sw = toScreenW(w), sh = toScreenH(h);
    int r = std::max(0, static_cast<int>(toScreenW(static_cast<uint16_t>(std::max<int16_t>(0, radius)))));
    r = std::min(r, std::min(sw, sh) / 2);

    int clipLeft, clipTop, clipRight, clipBottom;
    if (!getClipBounds(clipLeft, clipTop, clipRight, clipBottom)) return;
    const int firstX = std::max(sx, clipLeft);
    const int lastX = std::min(sx + sw - 1, clipRight);
    const int firstY = std::max(sy, clipTop);
    const int lastY = std::min(sy + sh - 1, clipBottom);
    if (firstX > lastX || firstY > lastY) return;

    for (int destinationY = firstY; destinationY <= lastY; ++destinationY)
    {
        const int yy = destinationY - sy;
        for (int destinationX = firstX; destinationX <= lastX; ++destinationX)
        {
            const int xx = destinationX - sx;
            const int dx = xx < r ? r - xx : (xx >= sw - r ? xx - (sw - r - 1) : 0);
            const int dy = yy < r ? r - yy : (yy >= sh - r ? yy - (sh - r - 1) : 0);
            if (dx == 0 || dy == 0 || dx*dx + dy*dy <= r*r)
                putPixelRaw(destinationX, destinationY, color);
        }
    }
    screenDirty = true;
}

void GraphicsSDL::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Color color)
{
    // Stable and simple first implementation: outer rounded rect minus inner is
    // approximated by lines plus quarter-circle edges.
    const int sx = toScreenX(x), sy = toScreenY(y);
    const int sw = toScreenW(w), sh = toScreenH(h);
    int r = std::min<int>(toScreenW(radius), std::min(sw, sh) / 2);

    drawLineRaw(sx + r, sy, sx + sw - r - 1, sy, color);
    drawLineRaw(sx + r, sy + sh - 1, sx + sw - r - 1, sy + sh - 1, color);
    drawLineRaw(sx, sy + r, sx, sy + sh - r - 1, color);
    drawLineRaw(sx + sw - 1, sy + r, sx + sw - 1, sy + sh - r - 1, color);

    for (int yy = 0; yy <= r; ++yy)
    {
        for (int xx = 0; xx <= r; ++xx)
        {
            const int d = xx*xx + yy*yy;
            if (std::abs(d - r*r) <= r)
            {
                putPixelRaw(sx + r - xx, sy + r - yy, color);
                putPixelRaw(sx + sw - r - 1 + xx, sy + r - yy, color);
                putPixelRaw(sx + r - xx, sy + sh - r - 1 + yy, color);
                putPixelRaw(sx + sw - r - 1 + xx, sy + sh - r - 1 + yy, color);
            }
        }
    }
    screenDirty = true;
}

void GraphicsSDL::drawEllipseRaw(int cx, int cy, int rx, int ry, Color color)
{
    if (rx <= 0 || ry <= 0) return;
    int clipLeft, clipTop, clipRight, clipBottom;
    if (!getClipBounds(clipLeft, clipTop, clipRight, clipBottom)) return;
    if (cx + rx < clipLeft || cx - rx > clipRight || cy + ry < clipTop || cy - ry > clipBottom) return;

    int x = 0;
    int y = ry;

    const long rx2 = static_cast<long>(rx) * rx;
    const long ry2 = static_cast<long>(ry) * ry;
    const long twoRx2 = 2 * rx2;
    const long twoRy2 = 2 * ry2;

    long px = 0;
    long py = twoRx2 * y;

    auto plot4 = [&](int px0, int py0)
    {
        putPixelRaw(cx + px0, cy + py0, color);
        putPixelRaw(cx - px0, cy + py0, color);
        putPixelRaw(cx + px0, cy - py0, color);
        putPixelRaw(cx - px0, cy - py0, color);
    };

    long p = static_cast<long>(
        ry2 - rx2 * ry + 0.25f * rx2);

    while (px < py)
    {
        plot4(x, y);

        ++x;
        px += twoRy2;

        if (p < 0)
        {
            p += ry2 + px;
        }
        else
        {
            --y;
            py -= twoRx2;
            p += ry2 + px - py;
        }
    }

    p = static_cast<long>(ry2 * (x + 0.5f) * (x + 0.5f) + rx2 * (y - 1.0f) * (y - 1.0f) - rx2 * ry2);

    while (y >= 0)
    {
        plot4(x, y);

        --y;
        py -= twoRx2;

        if (p > 0)
        {
            p += rx2 - py;
        }
        else
        {
            ++x;
            px += twoRy2;
            p += rx2 - py + px;
        }
    }
}

void GraphicsSDL::fillEllipseRaw(int cx, int cy, int rx, int ry, Color color)
{
    if (rx <= 0 || ry <= 0) return;
    int clipLeft, clipTop, clipRight, clipBottom;
    if (!getClipBounds(clipLeft, clipTop, clipRight, clipBottom)) return;
    const int minX = std::max(-rx, clipLeft - cx);
    const int maxX = std::min(rx, clipRight - cx);
    const int minY = std::max(-ry, clipTop - cy);
    const int maxY = std::min(ry, clipBottom - cy);
    if (minX > maxX || minY > maxY) return;

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            const float n =
                (static_cast<float>(x*x) / (rx*rx)) +
                (static_cast<float>(y*y) / (ry*ry));
            if (n <= 1.0f) putPixelRaw(cx + x, cy + y, color);
        }
    }
}

void GraphicsSDL::drawCircle(int16_t x, int16_t y, uint16_t r, Color color)
{
    drawCircle(x, y, r, r, color);
}

void GraphicsSDL::drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Color color)
{
    drawEllipseRaw(toScreenX(x), toScreenY(y), toScreenW(rx), toScreenH(ry), color);
    screenDirty = true;
}

void GraphicsSDL::fillCircle(int16_t x, int16_t y, uint16_t r, Color color)
{
    fillCircle(x, y, r, r, color);
}

void GraphicsSDL::fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Color color)
{
    fillEllipseRaw(toScreenX(x), toScreenY(y), toScreenW(rx), toScreenH(ry), color);
    screenDirty = true;
}

float GraphicsSDL::normalizeAngle(float a)
{
    while (a < 0.0f) a += Math::TWO_PI;
    while (a >= Math::TWO_PI) a -= Math::TWO_PI;
    return a;
}

bool GraphicsSDL::angleInside(float a, float start, float end)
{
    a = normalizeAngle(a);
    start = normalizeAngle(start);
    end = normalizeAngle(end);
    if (start <= end) return a >= start && a <= end;
    return a >= start || a <= end;
}

void GraphicsSDL::drawArcRaw(int cx, int cy, int rx, int ry, int width, float a0, float a1, Color color, bool filled)
{
    // a0/a1 are radians, matching the public Graphics API.
    if (rx <= 0 || ry <= 0) return;
    (void)filled;
    width = std::max(1, width);

    const float innerX = filled && width >= rx ? 0.0f : static_cast<float>(std::max(0, rx - width));
    const float innerY = filled && width >= ry ? 0.0f : static_cast<float>(std::max(0, ry - width));

    int clipLeft, clipTop, clipRight, clipBottom;
    if (!getClipBounds(clipLeft, clipTop, clipRight, clipBottom)) return;
    const int minX = std::max(-rx, clipLeft - cx);
    const int maxX = std::min(rx, clipRight - cx);
    const int minY = std::max(-ry, clipTop - cy);
    const int maxY = std::min(ry, clipBottom - cy);
    if (minX > maxX || minY > maxY) return;

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            const float outer =
                static_cast<float>(x*x) / (rx*rx) +
                static_cast<float>(y*y) / (ry*ry);
            if (outer > 1.0f) continue;

            bool ring = true;
            if (innerX > 0.0f && innerY > 0.0f)
            {
                const float inner =
                    static_cast<float>(x*x) / (innerX*innerX) +
                    static_cast<float>(y*y) / (innerY*innerY);
                ring = inner >= 1.0f;
            }

            if (!ring) continue;
            const float angle = Math::atan2(static_cast<float>(y), static_cast<float>(x));
            if (angleInside(angle, a0, a1)) putPixelRaw(cx + x, cy + y, color);
        }
    }
}

void GraphicsSDL::drawArc(int16_t x, int16_t y, uint16_t r, float a0, float a1, Color color)
{
    drawArc(x, y, r, r, 1, a0, a1, color);
}

void GraphicsSDL::drawArc(int16_t x, int16_t y, uint16_t r, uint8_t w, float a0, float a1, Color color)
{
    drawArc(x, y, r, r, w, a0, a1, color);
}

void GraphicsSDL::drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float a0, float a1, Color color)
{
    drawArc(x, y, rx, ry, 1, a0, a1, color);
}

void GraphicsSDL::drawArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t w, float a0, float a1, Color color)
{
    drawArcRaw(toScreenX(x), toScreenY(y), toScreenW(rx), toScreenH(ry),
               std::max<int>(1, toScreenW(w)), a0, a1, color, true);
    screenDirty = true;
}

void GraphicsSDL::fillArc(int16_t x, int16_t y, uint16_t r, float a0, float a1, Color color)
{
    fillArc(x, y, r, r, a0, a1, color);
}

void GraphicsSDL::fillArc(int16_t x, int16_t y, uint16_t r, uint8_t w, float a0, float a1, Color color)
{
    fillArc(x, y, r, r, w, a0, a1, color);
}

void GraphicsSDL::fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, float a0, float a1, Color color)
{
    drawArcRaw(toScreenX(x), toScreenY(y), toScreenW(rx), toScreenH(ry),
               std::max<int>(toScreenW(rx), toScreenH(ry)), a0, a1, color, true);
    screenDirty = true;
}

void GraphicsSDL::fillArc(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t w, float a0, float a1, Color color)
{
    drawArcRaw(toScreenX(x), toScreenY(y), toScreenW(rx), toScreenH(ry),
               std::max<int>(1, toScreenW(w)), a0, a1, color, true);
    screenDirty = true;
}

uint16_t GraphicsSDL::getTextWidth(const char* text, Font font)
{
    if (text == nullptr) return 0;

    const FontSelection selection = selectFont(font);
    const float scale = toScreenScale(selection.scale);
    uint32_t width = 0;
    for (const unsigned char* character = reinterpret_cast<const unsigned char*>(text); *character != '\0'; ++character)
    {
        width += static_cast<uint32_t>(scaledGlyphAdvance(selectGlyph(*selection.font, *character), scale));
    }
    return static_cast<uint16_t>(std::min<uint32_t>(width, UINT16_MAX));
}

void GraphicsSDL::drawString(const char* str, int16_t x, int16_t y, Color color, Font font)
{
    if (str == nullptr) return;

    int clipLeft, clipTop, clipRight, clipBottom;
    if (!getClipBounds(clipLeft, clipTop, clipRight, clipBottom)) return;

    const FontSelection selection = selectFont(font);
    const GFXfont& gfxFont = *selection.font;
    const float scale = toScreenScale(selection.scale);
    const float baseline = toScreenY(y) + fontAscent(gfxFont) * scale;
    int cursorX = toScreenX(x);

    for (const unsigned char* character = reinterpret_cast<const unsigned char*>(str); *character != '\0'; ++character)
    {
        const GFXglyph& glyph = selectGlyph(gfxFont, *character);
        const float glyphLeft = cursorX + glyph.xOffset * scale;
        const float glyphTop = baseline + glyph.yOffset * scale;
        const int destinationLeft = static_cast<int>(std::floor(glyphLeft));
        const int destinationTop = static_cast<int>(std::floor(glyphTop));
        const int destinationWidth = scale >= 1.0f
            ? static_cast<int>(std::ceil(glyph.width * scale))
            : static_cast<int>(glyph.width * scale);
        const int destinationHeight = scale >= 1.0f
            ? static_cast<int>(std::ceil(glyph.height * scale))
            : static_cast<int>(glyph.height * scale);

        const int firstDestinationX = std::max(0, clipLeft - destinationLeft);
        const int lastDestinationX = std::min(destinationWidth - 1, clipRight - destinationLeft);
        const int firstDestinationY = std::max(0, clipTop - destinationTop);
        const int lastDestinationY = std::min(destinationHeight - 1, clipBottom - destinationTop);

        for (int destinationY = firstDestinationY; destinationY <= lastDestinationY; ++destinationY)
        {
            const int sourceY = std::min(static_cast<int>(glyph.height) - 1, static_cast<int>(destinationY / scale));
            for (int destinationX = firstDestinationX; destinationX <= lastDestinationX; ++destinationX)
            {
                const int sourceX = std::min(static_cast<int>(glyph.width) - 1, static_cast<int>(destinationX / scale));
                const uint32_t bitmapBit = static_cast<uint32_t>(glyph.bitmapOffset) * 8 + sourceY * glyph.width + sourceX;
                if ((gfxFont.bitmap[bitmapBit >> 3] & (0x80u >> (bitmapBit & 7))) == 0) continue;
                putPixelRaw(destinationLeft + destinationX, destinationTop + destinationY, color);
            }
        }
        cursorX += scaledGlyphAdvance(glyph, scale);
    }
    screenDirty = true;
}

void GraphicsSDL::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    drawSprite(bitmap, x, y, w, h, SpriteOptions{});
}

void GraphicsSDL::drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options)
{
    if (bitmap == nullptr || w == 0 || h == 0 || options.scale == 0) return;

    const float scale = toScreenScale(static_cast<float>(options.scale));
    // PRUZEA angles are radians. Keep the same convention as GraphicsILI9341.
    const float angle = options.angle;
    const float cosA = Math::cos(angle);
    const float sinA = Math::sin(angle);

    const float outputWidth = w * scale;
    const float outputHeight = h * scale;
    const float cx = toScreenX(x) + (outputWidth - 1.0f) * 0.5f;
    const float cy = toScreenY(y) + (outputHeight - 1.0f) * 0.5f;
    const float halfPixelWidth = (outputWidth - 1.0f) * 0.5f;
    const float halfPixelHeight = (outputHeight - 1.0f) * 0.5f;
    const float extentX = Math::abs(halfPixelWidth * cosA) + Math::abs(halfPixelHeight * sinA);
    const float extentY = Math::abs(halfPixelWidth * sinA) + Math::abs(halfPixelHeight * cosA);
    int clipLeft, clipTop, clipRight, clipBottom;
    if (!getClipBounds(clipLeft, clipTop, clipRight, clipBottom)) return;
    const int left = std::max(clipLeft, static_cast<int>(Math::floor(cx - extentX)));
    const int right = std::min(clipRight, static_cast<int>(Math::ceil(cx + extentX)));
    const int top = std::max(clipTop, static_cast<int>(Math::floor(cy - extentY)));
    const int bottom = std::min(clipBottom, static_cast<int>(Math::ceil(cy + extentY)));
    if (left > right || top > bottom) return;

    for (int destinationY = top; destinationY <= bottom; ++destinationY)
    {
        for (int destinationX = left; destinationX <= right; ++destinationX)
        {
            const float dx = destinationX - cx;
            const float dy = destinationY - cy;
            const float ux = ( dx * cosA + dy * sinA) / scale + w * 0.5f;
            const float uy = (-dx * sinA + dy * cosA) / scale + h * 0.5f;

            int sx = static_cast<int>(Math::floor(ux));
            int sy = static_cast<int>(Math::floor(uy));
            if (sx < 0 || sy < 0 || sx >= w || sy >= h) continue;

            if (options.flipX) sx = w - 1 - sx;
            if (options.flipY) sy = h - 1 - sy;

            const uint16_t pixel = bitmap[static_cast<size_t>(sy) * w + sx];
            if (options.transparent &&
                pixel == static_cast<uint16_t>(options.transparentColor))
            {
                continue;
            }

            putPixelRaw(destinationX, destinationY, static_cast<Color>(pixel));
        }
    }

    screenDirty = true;
}

void GraphicsSDL::drawSprite(const SpriteSheet& sheet, uint16_t column, uint16_t row, int16_t x, int16_t y, const SpriteOptions& options)
{
    if (sheet.bitmap == nullptr || sheet.spriteWidth == 0 || sheet.spriteHeight == 0 ||
        column >= sheet.columns || row >= sheet.rows)
    {
        return;
    }

    const size_t count = static_cast<size_t>(sheet.spriteWidth) * sheet.spriteHeight;
    spriteSheetBuffer.resize(count);

    const uint32_t sheetWidth = sheet.columns * sheet.spriteWidth;
    const uint32_t base = row * sheet.spriteHeight * sheetWidth + column * sheet.spriteWidth;

    for (uint32_t yy = 0; yy < sheet.spriteHeight; ++yy)
    {
        const uint16_t* src = sheet.bitmap + base + yy * sheetWidth;
        std::copy(src, src + sheet.spriteWidth,
                  spriteSheetBuffer.begin() + static_cast<size_t>(yy) * sheet.spriteWidth);
    }

    drawSprite(spriteSheetBuffer.data(), x, y, sheet.spriteWidth, sheet.spriteHeight, options);
}

void GraphicsSDL::drawImage(const Image& image, int16_t x, int16_t y)
{
    drawSprite(image.getBitmap(), x, y, image.getWidth(), image.getHeight());
}

void GraphicsSDL::setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    clipEnabled = true;
    clipX = x;
    clipY = y;
    clipW = w;
    clipH = h;
}

void GraphicsSDL::getClipRect(int16_t& x, int16_t& y, uint16_t& w, uint16_t& h)
{
    if (!clipEnabled)
    {
        x = 0;
        y = 0;
        w = logicalScreenW;
        h = logicalScreenH;
        return;
    }

    x = clipX;
    y = clipY;
    w = clipW;
    h = clipH;
}

void GraphicsSDL::resetClipRect()
{
    clipEnabled = false;
    clipX = 0;
    clipY = 0;
    clipW = logicalScreenW;
    clipH = logicalScreenH;
}

bool GraphicsSDL::readScreenLine(uint16_t y, uint16_t* outPixels, uint16_t pixelCount)
{
    if (outPixels == nullptr || y >= getScreenHeight() ||
        pixelCount < getScreenWidth() || logicalScreenW == 0 || logicalScreenH == 0)
    {
        return false;
    }

    const float screenCenterX = getScreenWidth() * 0.5f;
    const float screenCenterY = getScreenHeight() * 0.5f;
    const float targetX = screenCenterX - static_cast<float>(viewportX * displayScale);
    const float targetY = screenCenterY - static_cast<float>(viewportY * displayScale);
    const float pivotX = logicalScreenW * 0.5f;
    const float pivotY = logicalScreenH * 0.5f;

    for (uint16_t x = 0; x < getScreenWidth(); ++x)
    {
        int sourceX;
        int sourceY;

        if (displayScale == 1 &&
            getScreenWidth() <= logicalScreenW &&
            getScreenHeight() <= logicalScreenH)
        {
            sourceX = static_cast<int>(x) + viewportX;
            sourceY = static_cast<int>(y) + viewportY;
        }
        else
        {
            sourceX = static_cast<int>((static_cast<float>(x) - targetX) / displayScale + pivotX);
            sourceY = static_cast<int>((static_cast<float>(y) - targetY) / displayScale + pivotY);
        }

        outPixels[x] =
            (sourceX < 0 || sourceY < 0 ||
             sourceX >= logicalScreenW || sourceY >= logicalScreenH)
            ? static_cast<uint16_t>(BLACK)
            : getPixelRaw(sourceX, sourceY);
    }

    return true;
}

void GraphicsSDL::push()
{
    if (!screenDirty || renderer == nullptr || texture == nullptr) return;

    for (uint16_t y = 0; y < getScreenHeight(); ++y)
    {
        readScreenLine(
            y,
            presentBuffer.data() + static_cast<size_t>(y) * getScreenWidth(),
            getScreenWidth());
    }

    SDL_UpdateTexture(
        texture,
        nullptr,
        presentBuffer.data(),
        getScreenWidth() * static_cast<int>(sizeof(uint16_t)));

    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    screenDirty = false;
}

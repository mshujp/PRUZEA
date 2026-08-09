#include "GraphicsBase.h"
#include <algorithm>
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace PRUZEA;

bool GraphicsBase::setLogicalScreenSize(uint16_t _logicalScreenW, uint16_t _logicalScreenH)
{
    logicalScreenW = _logicalScreenW;
    logicalScreenH = _logicalScreenH;
    return true;
}

uint16_t GraphicsBase::getTextHeight(const char* text, Font font)
{
    uint16_t size = 0;
    switch (font)
    {
        case Font::SIZE_10:  size = 10; break;
        case Font::SIZE_13:  size = 13; break;
        case Font::SIZE_18:  size = 18; break;
        case Font::SIZE_22B: size = 22; break;
        case Font::SIZE_25:  size = 25; break;
        case Font::SIZE_25B: size = 25; break;
        case Font::SIZE_32:  size = 32; break;
        case Font::SIZE_32B: size = 32; break;
        case Font::SIZE_42:  size = 42; break;
        case Font::SIZE_42B: size = 42; break;
        case Font::SIZE_16J: size = 16; break;
        case Font::SIZE_20J: size = 20; break;
        case Font::SIZE_32J: size = 32; break;
        default: return 0;
     }
     if (std::strpbrk(text, "gjpqy_") == nullptr) size = static_cast<uint16_t>(roundf(size * 0.8));

     return zoom == 1.0f || cameraSuspended ? size : static_cast<uint16_t>(roundf(size * zoom));
}

void GraphicsBase::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t t, Graphics::Color color)
{
    if (w <= 0 || h <= 0 || t <= 0) return;

    const int thickness = std::min(t, std::min(w, h));
    fillRect(x, y, w, thickness, color);
    fillRect(x, y + h - thickness, w, thickness, color);
    fillRect(x, y, thickness, h, color);
    fillRect(x + w - thickness, y, thickness, h, color);
}

void GraphicsBase::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t t, Graphics::Color color)
{
    if (w <= 0 || h <= 0 || t <= 0) return;

    const int thickness = std::min(t, std::min(w, h));
    for (int i = 0; i < thickness; ++i)
    {
    	if (w - i*2 <= 0) break;
    	
        drawRoundRect(
            static_cast<int16_t>(x + i),
            static_cast<int16_t>(y + i),
            static_cast<uint16_t>(w - i * 2),
            static_cast<uint16_t>(h - i * 2),
            static_cast<uint16_t>(std::max(0, static_cast<int>(r) - i)),
            color
        );
    }
}

int16_t GraphicsBase::alignedX(int16_t x, uint16_t w, HorizontalAlign ha)
{
    switch (ha)
    {
    case HorizontalAlign::CENTER: return x - w / 2;
    case HorizontalAlign::RIGHT:  return x - w;
    default:                      return x;
    }
}

int16_t GraphicsBase::alignedY(int16_t y, uint16_t h, VerticalAlign va)
{
    switch (va)
    {
    case VerticalAlign::MIDDLE: return y - h / 2;
    case VerticalAlign::BOTTOM: return y - h;
    default:                    return y;
    }
}

void GraphicsBase::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color, HorizontalAlign ha, VerticalAlign va)
{
    drawRect(alignedX(x, w, ha), alignedY(y, h, va), w, h, color);
}

void GraphicsBase::drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t thickness, Color color, HorizontalAlign ha, VerticalAlign va)
{
    drawRect(alignedX(x, w, ha), alignedY(y, h, va), w,h, thickness, color);
}

void GraphicsBase::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Color color, HorizontalAlign ha, VerticalAlign va)
{
    drawRoundRect(alignedX(x, w, ha), alignedY(y, h, va), w, h, radius, color);
}

void GraphicsBase::drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t thickness, Color color, HorizontalAlign ha, VerticalAlign va)
{
    drawRoundRect(alignedX(x, w, ha), alignedY(y, h, va), w, h, radius, thickness, color);
}

void GraphicsBase::fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color, HorizontalAlign ha, VerticalAlign va)
{
    fillRect(alignedX(x, w, ha), alignedY(y, h, va), w, h, color);
}

void GraphicsBase::fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Color color, HorizontalAlign ha, VerticalAlign va)
{
    fillRectAlpha(alignedX(x, w, ha), alignedY(y, h, va), w, h, alpha, color);
}

void GraphicsBase::fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Color color, HorizontalAlign ha, VerticalAlign va)
{
    fillRoundRect(alignedX(x, w, ha), alignedY(y, h, va), w, h, r, color);
}

void GraphicsBase::drawString(const char* str, int16_t x, int16_t y, Color color, Font font, HorizontalAlign ha, VerticalAlign va)
{
    if (str == nullptr) return;
    const int w = zoom == 1.0f ? getTextWidth(str, font) : static_cast<int>(roundf(getTextWidth(str, font) / zoom));
    const int h = zoom == 1.0f ? getTextHeight(str, font) : static_cast<int>(roundf(getTextHeight(str, font) / zoom));
    drawString(str, alignedX(x, w, ha), alignedY(y, h, va), color, font);
}

void GraphicsBase::setViewport(int16_t x, int16_t y)
{
    if (viewportX == x && viewportY == y) return;
    viewportX = x;
    viewportY = y;
    screenDirty = true;
}

int16_t GraphicsBase::getViewportX() const
{
    return viewportX;
}
int16_t GraphicsBase::getViewportY() const
{
    return viewportY;
}
 
void GraphicsBase::resetViewport()
{
    setViewport(0, 0);
}

int16_t GraphicsBase::toScreenX(int16_t x) const
{
    if (cameraSuspended) return x;
    if (zoom == 1.0f) return x + offsetX;
    return static_cast<int16_t>(roundf(x * zoom + offsetX));
}

int16_t GraphicsBase::toScreenY(int16_t y) const
{
    if (cameraSuspended) return y;
    if (zoom == 1.0f) return y + offsetY;
    return static_cast<int16_t>(roundf(y * zoom + offsetY));
}

uint16_t GraphicsBase::toScreenW(uint16_t w) const
{
    if (cameraSuspended) return w;
    if (zoom == 1.0f) return w;
    return static_cast<uint16_t>(roundf(w * zoom));
}

uint16_t GraphicsBase::toScreenH(uint16_t h) const
{
    if (cameraSuspended) return h;
    if (zoom == 1.0f) return h;
    return static_cast<uint16_t>(roundf(h * zoom));
}

float GraphicsBase::toScreenScale(float scale) const
{
    if (cameraSuspended) return scale;
    if (zoom == 1.0f) return scale;
    return scale * zoom;
}

void GraphicsBase::setCamera(const Camera& camera)
{
    if (camera.zoom == 0.0f) return;

    if (camera.zoom== 1.0f)
    {
        offsetX = -camera.x;
        offsetY = -camera.y;
        zoom = camera.zoom;
     }
    else
    {
        offsetX = camera.zoomCenterX * (1.0f - camera.zoom) - camera.x * camera.zoom;
        offsetY = camera.zoomCenterY * (1.0f - camera.zoom) - camera.y * camera.zoom;
        zoom = camera.zoom;
    }
}

void GraphicsBase::resetCamera()
{
    offsetX = 0;
    offsetY = 0;
    zoom = 1.0f;
    zoomCenterX = 0;
    zoomCenterY = 0;
}
 
void GraphicsBase::suspendCamera()
{
    cameraSuspended = true;
}
void GraphicsBase::resumeCamera()
{
    cameraSuspended = false;
}

void GraphicsBase::setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h, HorizontalAlign ha, VerticalAlign va)
{
    setClipRect(alignedX(x, w, ha), alignedY(y, h, va), w, h);
}

void GraphicsBase::suspendClipRect()
{
    getClipRect(clipRectX, clipRectY, clipRectW, clipRectH);
    resetClipRect();
    clipRectSuspended = true;
}

void GraphicsBase::resumeClipRect()
{
    if (!clipRectSuspended) return;
    setClipRect(clipRectX, clipRectY, clipRectW, clipRectH);
    clipRectSuspended = false;
}

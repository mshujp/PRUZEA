#include "CoreRing.h"
#include "Platform.h"

using namespace PRUZEA;

namespace
{
    CoreRing::Mode currentMode = CoreRing::MODE_IDLE;
    CoreRing::Mode baseMode = CoreRing::MODE_IDLE;

    uint32_t effectEndMsec = 0;

    bool powerWarning = false;
    bool powerCritical = false;
    bool criticalBlinkVisible = true;
    uint32_t criticalBlinkLastMsec = 0;

    bool dirty = true;

    constexpr uint32_t STORAGE_EFFECT_MSEC = 260;
    constexpr uint32_t CRITICAL_BLINK_INTERVAL_MSEC = 250;

    Graphics::Color dim(Graphics::Color color)
    {
        uint8_t r = static_cast<uint8_t>(((color >> 11) & 0x1F) << 3);
        uint8_t g = static_cast<uint8_t>(((color >> 5) & 0x3F) << 2);
        uint8_t b = static_cast<uint8_t>((color & 0x1F) << 3);

        return Graphics::rgb565(r / 3, g / 3, b / 3);
    }
}

void CoreRing::setMode(Mode mode)
{
    if (baseMode == mode) return;

    baseMode = mode;

    if (!isEffectActive())
    {
        currentMode = mode;
    }

    dirty = true;
}

void CoreRing::setPowerWarning(bool warning, bool critical)
{
    if (powerWarning == warning && powerCritical == critical) return;

    powerWarning = warning;
    powerCritical = critical;

    criticalBlinkVisible = true;
    criticalBlinkLastMsec = critical ? Platform::getMsec() : 0;
    dirty = true;
}

void CoreRing::notifyStorageRead()
{
    currentMode = MODE_STORAGE_READ;
    effectEndMsec = Platform::getMsec() + STORAGE_EFFECT_MSEC;
    dirty = true;
}

void CoreRing::notifyStorageWrite()
{
    currentMode = MODE_STORAGE_WRITE;
    effectEndMsec = Platform::getMsec() + STORAGE_EFFECT_MSEC;
    dirty = true;
}

void CoreRing::update()
{
    const uint32_t now = Platform::getMsec();

    if (powerCritical &&
        Platform::elapsed(now, criticalBlinkLastMsec, CRITICAL_BLINK_INTERVAL_MSEC))
    {
        criticalBlinkLastMsec = now;
        criticalBlinkVisible = !criticalBlinkVisible;
        dirty = true;
    }

    if (!isEffectActive())
    {
        if (currentMode == MODE_STORAGE_READ || currentMode == MODE_STORAGE_WRITE)
        {
            currentMode = baseMode;
            dirty = true;
        }
    }
}

bool CoreRing::isEffectActive()
{
    return Platform::getMsec() < effectEndMsec;
}

const char* CoreRing::getPowerStatusText()
{
    if (powerCritical) return "CRITICAL";
    if (powerWarning)  return "LOW";
    return "OK";
}

bool CoreRing::drawOverlay(GraphicsBase& graphics, bool requestRedraw)
{
    if (!dirty && !requestRedraw) return false;

    graphics.suspendCamera();
    graphics.suspendClipRect();

    if (baseMode == MODE_SPLASH)
    {
    }
    else if (baseMode == MODE_SYSTEM_UI)
    {
        drawSystemUI(graphics);
    }
    else if (baseMode == MODE_INFO)
    {
        drawInfoRing(graphics);
    }
    else if (baseMode == MODE_IDLE)
    {
        drawMini(graphics, 304, 232);
    }

    graphics.resumeClipRect();
    graphics.resumeCamera();

    dirty = false;
    return true;
}

void CoreRing::drawBackgroundRing(Graphics& graphics, int cx, int cy, uint8_t radius)
{
    const Graphics::Color base = Graphics::rgb565(18, 46, 72);
    const Graphics::Color line1 = Graphics::rgb565(28, 74, 110);
    const Graphics::Color line2 = Graphics::rgb565(20, 52, 82);

    graphics.fillCircle(cx, cy, radius, base);
    graphics.drawCircle(cx, cy, radius + 6, line1);
    graphics.drawCircle(cx, cy, radius + 14, line2);
}

void CoreRing::drawForegroundRing(Graphics& graphics, int cx, int cy, uint8_t radius)
{
    const Graphics::Color mainColor = getMainColor();
    const Graphics::Color subColor = getSubColor();
    const Graphics::Color darkColor = dim(mainColor);

    graphics.drawCircle(cx, cy, radius + 4, subColor);

    graphics.fillCircle(cx, cy, radius / 3, Graphics::rgb565(10, 18, 30));
    graphics.drawCircle(cx, cy, radius / 3 + 3, mainColor);

    if (currentMode == MODE_SPLASH)
    {
        graphics.drawCircle(cx, cy, radius + 10, darkColor);
        graphics.drawCircle(cx, cy, radius + 16, mainColor);
    }
}

void CoreRing::drawCoreRing(Graphics& graphics, int cx, int cy, uint8_t radius)
{
    drawBackgroundRing(graphics, cx, cy, radius);
    drawForegroundRing(graphics, cx, cy, radius);
}

void CoreRing::drawSystemUI(Graphics& graphics)
{
    drawCoreRing(graphics, 300, 20, 34);
}

void CoreRing::drawInfoRing(Graphics& graphics)
{
    drawCoreRing(graphics, 300, 20, 34);
}

void CoreRing::drawMini(GraphicsBase& graphics, int cx, int cy)
{
    const Graphics::Color mainColor = getMainColor();
    const Graphics::Color subColor = getSubColor();

    int16_t viewportX = graphics.getViewportX();
    int16_t viewportY = graphics.getViewportY();

    graphics.drawCircle(cx + viewportX, cy + viewportY, 5, subColor);
    graphics.drawCircle(cx + viewportX, cy + viewportY, 6, subColor);
    graphics.drawPixel(cx + viewportX, cy + viewportY, mainColor);

    if (currentMode == MODE_STORAGE_READ || currentMode == MODE_STORAGE_WRITE)
    {
    }
    else if (powerCritical)
    {
        if (criticalBlinkVisible)
        {
            graphics.drawCircle(cx + viewportX, cy + viewportY, 5, mainColor);
            graphics.drawCircle(cx + viewportX, cy + viewportY, 6, mainColor);
        }
    }
    else if (powerWarning)
    {
        graphics.drawCircle(cx + viewportX, cy + viewportY, 5, mainColor);
        graphics.drawCircle(cx + viewportX, cy + viewportY, 6, mainColor);
    }
}

Graphics::Color CoreRing::getMainColor()
{
    if (powerCritical) return Graphics::rgb565(255, 80, 80);
    if (powerWarning)  return Graphics::rgb565(255, 180, 64);

    switch (currentMode)
    {
        case MODE_STORAGE_WRITE: return Graphics::rgb565(255, 180, 64);
        case MODE_STORAGE_READ:  return Graphics::rgb565(80, 220, 255);
        case MODE_SPLASH:        return Graphics::rgb565(96, 220, 255);
        case MODE_INFO:          return Graphics::rgb565(140, 140, 255);
        case MODE_SYSTEM_UI:
        case MODE_IDLE:
        default:                 return Graphics::rgb565(64, 210, 255);
    }
}

Graphics::Color CoreRing::getSubColor()
{
    if (powerCritical) return Graphics::rgb565(150, 42, 42);
    if (powerWarning)  return Graphics::rgb565(180, 110, 42);

    switch (currentMode)
    {
        case MODE_STORAGE_WRITE: return Graphics::rgb565(180, 110, 42);
        case MODE_INFO:          return Graphics::rgb565(72, 72, 150);
        default:                 return Graphics::rgb565(28, 74, 110);
    }
}

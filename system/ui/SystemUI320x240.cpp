#include "SystemUI320x240.h"
#include "CoreRing.h"

#include <stdio.h>

using namespace PRUZEA;

namespace
{
    constexpr uint16_t SCREEN_W = SystemUI320x240::SCREEEN_WIDTH;
    constexpr uint16_t SCREEN_H = SystemUI320x240::SCREEEN_HIGHT;

    constexpr Graphics::Color COL_BG_BOTTOM  = Graphics::rgb565(24, 34, 58);
    constexpr Graphics::Color COL_PANEL      = Graphics::rgb565(22, 36, 62);
    constexpr Graphics::Color COL_PANEL_DARK = Graphics::rgb565(12, 20, 34);
    constexpr Graphics::Color COL_ACCENT     = Graphics::rgb565(64, 210, 255);
    constexpr Graphics::Color COL_TEXT       = Graphics::Color::WHITE;
    constexpr Graphics::Color COL_MUTED      = Graphics::rgb565(110, 120, 135);
    constexpr Graphics::Color COL_SHADOW     = Graphics::BLACK;
    constexpr Graphics::Color COL_LINE       = Graphics::rgb565(48, 74, 104);

    void drawTextShadow(Graphics& g, const char* text, int x, int y, Graphics::Color color, Graphics::Font font)
    {
        g.drawString(text, x + 1, y + 1, COL_SHADOW, font);
        g.drawString(text, x, y, color, font);
    }

    void drawTextCenterShadow(Graphics& g, const char* text, int cx, int cy, Graphics::Color color, Graphics::Font font)
    {
        g.drawString(text, cx + 1, cy + 1, COL_SHADOW, font, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        g.drawString(text, cx, cy, color, font, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }

    void drawTextRightShadow(Graphics& g, const char* text, int rx, int ry, Graphics::Color color, Graphics::Font font)
    {
        g.drawString(text, rx + 1, ry + 1, COL_SHADOW, font, Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::MIDDLE);
        g.drawString(text, rx, ry, color, font, Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::MIDDLE);
    }

    void drawBackground(Graphics& g)
    {
        g.fillScreen(COL_BG_BOTTOM);

        for (uint16_t y = 0; y < SCREEN_H; y += 8)
        {
            const uint8_t mix = static_cast<uint8_t>(y / 8);
            Graphics::Color c = (mix < 12)
                ? Graphics::rgb565(10 + mix, 22 + mix, 42 + mix)
                : COL_BG_BOTTOM;

            g.fillRect(0, y, SCREEN_W, 8, c);
        }

        for (uint16_t x = 0; x < SCREEN_W; x += 32)
        {
            g.drawLine(x, 0, x + 72, SCREEN_H - 1, Graphics::rgb565(18, 32, 54));
        }

        for (uint16_t y = 18; y < SCREEN_H; y += 34)
        {
            g.drawLine(0, y, SCREEN_W, y, Graphics::rgb565(18, 32, 54));
        }
    }

    void drawTopBar(Graphics& g)
    {
        g.fillRect(0, 0, SCREEN_W, 42, Graphics::rgb565(8, 16, 30));
        g.fillRect(0, 40, SCREEN_W, 2, COL_ACCENT);
        drawTextShadow(g, "PRUZEA", 14, 13, COL_TEXT, Graphics::Font::SIZE_18);
    }

    void drawPageDots(Graphics& g, uint16_t currentPage, uint16_t pageCount)
    {
        if (pageCount <= 1) return;

        constexpr int dotY = 26;
        constexpr int dotR = 3;
        constexpr int gap = 10;

        const int totalW = (pageCount - 1) * gap;
        const int startX = 225 - totalW;

        for (uint16_t i = 0; i < pageCount; i++)
        {
            const int x = startX + i * gap;

            if (i == currentPage)
            {
                g.fillCircle(x, dotY, dotR, COL_ACCENT);
            }
            else
            {
                g.drawCircle(x, dotY, dotR, COL_MUTED);
            }
        }
    }

    void drawCard(Graphics& g, int x, int y, int w, int h, const char* title, bool selected, bool enabled)
    {
        if (selected)
        {
            g.fillRect(x - 4, y - 3, w + 8, h + 6, Graphics::rgb565(20, 74, 108));
            g.drawRect(x - 4, y - 3, w + 8, h + 6, 2, COL_ACCENT);
            g.fillRect(x, y, w, h, Graphics::rgb565(30, 58, 92));
            g.fillRect(x, y, 6, h, COL_ACCENT);

            g.fillTriangle(
                x + 14,
                y + 8,
                x + 14,
                y + h - 8,
                x + 34,
                y + h / 2,
                COL_ACCENT);
        }
        else
        {
            g.fillRect(x, y, w, h, COL_PANEL);
            g.drawRect(x, y, w, h, 1, COL_LINE);
        }

        drawTextShadow(
            g,
            title,
            x + 42,
            y + 10,
            enabled ? COL_TEXT : COL_MUTED,
            Graphics::Font::SIZE_13);
    }
}

SystemUI320x240::SystemUI320x240()
{
}

void SystemUI320x240::drawSplash(Graphics& graphics)
{
    CoreRing::setMode(CoreRing::MODE_SPLASH);

    drawBackground(graphics);

    CoreRing::drawCoreRing(graphics, 130, 120, 95);
    CoreRing::drawCoreRing(graphics, 300, 20, 34);
    CoreRing::drawCoreRing(graphics, 290, 120, 12);

    drawTextCenterShadow(graphics, "PRUZEA", 160, 120, COL_TEXT, Graphics::Font::SIZE_42B);

    graphics.fillRect(90, 144, 144, 4, Graphics::rgb565(32, 54, 82));

    uint16_t barW = static_cast<uint16_t>((static_cast<uint32_t>(splashFrames) * 140u) / 24u);
    if (barW > 140) barW = 140;

    graphics.fillRect(90, 144, barW, 4, COL_ACCENT);
}

void SystemUI320x240::drawSelect(Graphics& graphics)
{
    CoreRing::setMode(CoreRing::MODE_SYSTEM_UI);

    drawBackground(graphics);

    drawTopBar(graphics);
    drawPageDots(graphics, pageIndex, getPageCount());

    constexpr int startX = 22;
    constexpr int startY = 58;
    constexpr int cardW = 276;
    constexpr int cardH = 34;
    constexpr int gap = 8;

    for (uint16_t i = 0; i < ITEMS_PER_PAGE; i++)
    {
        const uint16_t index = pageIndex * ITEMS_PER_PAGE + i;

        drawCard(
            graphics,
            startX,
            startY + i * (cardH + gap),
            cardW,
            cardH,
            getSlotName(index),
            index == selectedIndex,
            isSlotAvailable(index));
    }
}

void SystemUI320x240::drawInfo(Graphics& graphics)
{
    CoreRing::setMode(CoreRing::MODE_INFO);

    SystemUI::SystemInfo info = {};
    if (getSystemInfoHandler != nullptr)
    {
        getSystemInfoHandler(info, getSystemInfoContext);
    }

    drawBackground(graphics);

    graphics.fillRect(24, 24, 272, 190, COL_PANEL_DARK);
    graphics.drawRect(24, 24, 272, 189, 2, COL_ACCENT);

    drawTextCenterShadow(graphics, "PRUZEA", 160, 50, COL_TEXT, Graphics::Font::SIZE_25B);

    drawTextRightShadow(graphics, "Version", 150, 76, COL_MUTED, Graphics::Font::SIZE_13);
    drawTextShadow(graphics, info.version, 170, 76, COL_TEXT, Graphics::Font::SIZE_13);

    drawTextRightShadow(graphics, "Display", 150, 96, COL_MUTED, Graphics::Font::SIZE_13);
    drawTextShadow(graphics, info.display, 170, 96, COL_TEXT, Graphics::Font::SIZE_13);

    drawTextRightShadow(graphics, "Input", 150, 116, COL_MUTED, Graphics::Font::SIZE_13);
    drawTextShadow(graphics, info.input, 170, 116, COL_TEXT, Graphics::Font::SIZE_13);

    drawTextRightShadow(graphics, "Audio", 150, 136, COL_MUTED, Graphics::Font::SIZE_13);
    drawTextShadow(graphics, info.audio, 170, 136, COL_TEXT, Graphics::Font::SIZE_13);

    drawTextRightShadow(graphics, "Storage", 150, 156, COL_MUTED, Graphics::Font::SIZE_13);
    drawTextShadow(graphics, info.storage, 170, 156, COL_TEXT, Graphics::Font::SIZE_13);

    drawTextRightShadow(graphics, "Battery", 150, 176, COL_MUTED, Graphics::Font::SIZE_13);
    drawTextShadow(graphics, info.battery, 170, 176, COL_TEXT, Graphics::Font::SIZE_13);
}

void SystemUI320x240::drawShutdownConfirm(Graphics& graphics)
{
    CoreRing::setMode(CoreRing::MODE_IDLE);

    drawBackground(graphics);
    drawTopBar(graphics);

    constexpr int panelX = 34;
    constexpr int panelY = 54;
    constexpr int panelW = 252;
    constexpr int panelH = 132;

    graphics.fillRoundRect(panelX, panelY, panelW, panelH, 8, COL_PANEL_DARK);
    graphics.drawRoundRect(panelX, panelY, panelW, panelH, 8, 2, COL_ACCENT);

    drawTextCenterShadow(
        graphics,
        "SHUT DOWN?",
        SCREEN_W / 2,
        82,
        COL_TEXT,
        Graphics::Font::SIZE_25B);

    drawTextCenterShadow(
        graphics,
        "Exit PRUZEA safely?",
        SCREEN_W / 2,
        112,
        COL_MUTED,
        Graphics::Font::SIZE_13);

    constexpr int buttonY = 139;
    constexpr int buttonW = 82;
    constexpr int buttonH = 30;
    constexpr int yesX = 67;
    constexpr int noX = 171;

    const Graphics::Color yesFill = shutdownYesSelected
        ? Graphics::rgb565(28, 92, 126)
        : COL_PANEL;
    const Graphics::Color noFill = shutdownYesSelected
        ? COL_PANEL
        : Graphics::rgb565(28, 92, 126);

    graphics.fillRoundRect(yesX, buttonY, buttonW, buttonH, 5, yesFill);
    graphics.drawRoundRect(
        yesX,
        buttonY,
        buttonW,
        buttonH,
        5,
        shutdownYesSelected ? 2 : 1,
        shutdownYesSelected ? COL_ACCENT : COL_LINE);

    graphics.fillRoundRect(noX, buttonY, buttonW, buttonH, 5, noFill);
    graphics.drawRoundRect(
        noX,
        buttonY,
        buttonW,
        buttonH,
        5,
        shutdownYesSelected ? 1 : 2,
        shutdownYesSelected ? COL_LINE : COL_ACCENT);

    drawTextCenterShadow(
        graphics,
        "YES",
        yesX + buttonW / 2,
        buttonY + buttonH / 2,
        shutdownYesSelected ? COL_TEXT : COL_MUTED,
        Graphics::Font::SIZE_18);

    drawTextCenterShadow(
        graphics,
        "NO",
        noX + buttonW / 2,
        buttonY + buttonH / 2,
        shutdownYesSelected ? COL_MUTED : COL_TEXT,
        Graphics::Font::SIZE_18);

    drawTextCenterShadow(
        graphics,
        "A: OK    B: CANCEL",
        SCREEN_W / 2,
        204,
        COL_MUTED,
        Graphics::Font::SIZE_13);
}

void SystemUI320x240::drawDeleteGameDataConfirm(Graphics& graphics)
{
    CoreRing::setMode(CoreRing::MODE_IDLE);

    drawBackground(graphics);
    drawTopBar(graphics);

    constexpr int panelX = 34;
    constexpr int panelY = 54;
    constexpr int panelW = 252;
    constexpr int panelH = 132;

    graphics.fillRoundRect(panelX, panelY, panelW, panelH, 8, COL_PANEL_DARK);
    graphics.drawRoundRect(panelX, panelY, panelW, panelH, 8, 2, COL_ACCENT);

    drawTextCenterShadow(
        graphics,
        "DELETE GAME DATA?",
        SCREEN_W / 2,
        78,
        COL_TEXT,
        Graphics::Font::SIZE_22B);

    drawTextCenterShadow(
        graphics,
        getSlotName(selectedIndex),
        SCREEN_W / 2,
        108,
        COL_MUTED,
        Graphics::Font::SIZE_13);

    constexpr int buttonY = 139;
    constexpr int buttonW = 82;
    constexpr int buttonH = 30;
    constexpr int yesX = 67;
    constexpr int noX = 171;

    const Graphics::Color yesFill = deleteGameDataYesSelected
        ? Graphics::rgb565(28, 92, 126)
        : COL_PANEL;
    const Graphics::Color noFill = deleteGameDataYesSelected
        ? COL_PANEL
        : Graphics::rgb565(28, 92, 126);

    graphics.fillRoundRect(yesX, buttonY, buttonW, buttonH, 5, yesFill);
    graphics.drawRoundRect(
        yesX,
        buttonY,
        buttonW,
        buttonH,
        5,
        deleteGameDataYesSelected ? 2 : 1,
        deleteGameDataYesSelected ? COL_ACCENT : COL_LINE);

    graphics.fillRoundRect(noX, buttonY, buttonW, buttonH, 5, noFill);
    graphics.drawRoundRect(
        noX,
        buttonY,
        buttonW,
        buttonH,
        5,
        deleteGameDataYesSelected ? 1 : 2,
        deleteGameDataYesSelected ? COL_LINE : COL_ACCENT);

    drawTextCenterShadow(
        graphics,
        "YES",
        yesX + buttonW / 2,
        buttonY + buttonH / 2,
        deleteGameDataYesSelected ? COL_TEXT : COL_MUTED,
        Graphics::Font::SIZE_18);

    drawTextCenterShadow(
        graphics,
        "NO",
        noX + buttonW / 2,
        buttonY + buttonH / 2,
        deleteGameDataYesSelected ? COL_MUTED : COL_TEXT,
        Graphics::Font::SIZE_18);

    drawTextCenterShadow(
        graphics,
        "A: OK    B: CANCEL",
        SCREEN_W / 2,
        204,
        COL_MUTED,
        Graphics::Font::SIZE_13);
}

void SystemUI320x240::drawPowerOffReady(Graphics& graphics)
{
    graphics.fillScreen(Graphics::Color::BLACK);

    graphics.fillRect(54, 92, 212, 2, COL_ACCENT);
    graphics.fillRect(54, 148, 212, 2, COL_ACCENT);

    drawTextCenterShadow(
        graphics,
        "SAFE TO",
        SCREEN_W / 2,
        111,
        COL_TEXT,
        Graphics::Font::SIZE_25B);

    drawTextCenterShadow(
        graphics,
        "POWER OFF",
        SCREEN_W / 2,
        137,
        COL_ACCENT,
        Graphics::Font::SIZE_25B);

    drawTextCenterShadow(
        graphics,
        "You can now turn off the power.",
        SCREEN_W / 2,
        176,
        COL_MUTED,
        Graphics::Font::SIZE_13);
}


void SystemUI320x240::drawLowBttery(Graphics& graphics)
{
    graphics.fillScreen(Graphics::Color::BLACK);

    graphics.fillRect(54, 92, 212, 2, COL_ACCENT);
    graphics.fillRect(54, 148, 212, 2, COL_ACCENT);

    drawTextCenterShadow(
        graphics,
        "LOW BATTERY",
        SCREEN_W / 2,
        SCREEN_H / 2,
        COL_TEXT,
        Graphics::Font::SIZE_32B);
}

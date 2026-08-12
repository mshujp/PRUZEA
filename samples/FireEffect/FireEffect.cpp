#include "FireEffect.h"
#include <cstring>

using namespace PRUZEA;

namespace
{
constexpr uint8_t SOURCE_ROWS = 2;
constexpr uint8_t COOLING_MIN = 0;
constexpr uint8_t COOLING_MAX = 3;
}

void FireEffect::onInit(Storage& storage)
{
    (void)storage;

    buildPalette();
    clearHeat();
    burning = true;
    frameCounter = 0;
    dirty = true;
}

Game::GameState FireEffect::onUpdate(
    Input& input,
    Audio& audio,
    Storage& storage,
    float deltaSec)
{
    (void)storage;
    (void)deltaSec;

    if (input.justPressed(Input::A))
    {
        burning = true;
        audio.playSE(&Audio::SE::NO_8, 0.45f);
    }

    if (input.justPressed(Input::B))
    {
        burning = false;
        audio.playSE(&Audio::SE::NO_2, 0.45f);
    }

    if (input.justPressed(Input::START))
    {
        return GameState::TERMINATE_REQUEST;
    }

    if (burning)
    {
        seedBottom();
    }

    updateFire();
    ++frameCounter;
    dirty = true;

    return GameState::RUNNING;
}

bool FireEffect::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty)
    {
        return false;
    }

    graphics.fillScreen(Graphics::BLACK);

    graphics.drawString(
        "SOFTWARE FIRE",
        SCREEN_W / 2,
        3,
        Graphics::WHITE,
        Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::TOP);

    for (int16_t y = 0; y < FIRE_H; ++y)
    {
        for (int16_t x = 0; x < SCREEN_W; ++x)
        {
            graphics.drawPixel(
                x,
                FIRE_TOP + y,
                palette[heat[y][x]]);
        }
    }

    graphics.drawString(
        burning ? "A:IGNITE  B:EXTINGUISH" : "A:IGNITE",
        SCREEN_W / 2,
        107,
        Graphics::LIGHTGRAY,
        Graphics::SIZE_10,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::TOP);

    dirty = false;
    return true;
}

void FireEffect::onTerminate(Storage& storage)
{
    (void)storage;
}

void FireEffect::buildPalette()
{
    for (uint8_t i = 0; i <= MAX_HEAT; ++i)
    {
        const uint8_t segment = i / 16;
        const uint8_t step = static_cast<uint8_t>((i % 16) * 17);

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        switch (segment)
        {
            case 0:
                r = step;
                break;

            case 1:
                r = 255;
                g = step;
                break;

            case 2:
                r = 255;
                g = 255;
                b = step;
                break;

            default:
                r = 255;
                g = 255;
                b = 255;
                break;
        }

        palette[i] = Graphics::rgb565(r, g, b);
    }
}

void FireEffect::clearHeat()
{
    std::memset(heat, 0, sizeof(heat));
}

void FireEffect::seedBottom()
{
    for (int16_t y = FIRE_H - SOURCE_ROWS; y < FIRE_H; ++y)
    {
        for (int16_t x = 0; x < SCREEN_W; ++x)
        {
            const uint8_t flicker = static_cast<uint8_t>(Math::random(12));
            heat[y][x] = static_cast<uint8_t>(MAX_HEAT - flicker);
        }
    }
}

void FireEffect::updateFire()
{
    for (int16_t y = 0; y < FIRE_H - 1; ++y)
    {
        for (int16_t x = 0; x < SCREEN_W; ++x)
        {
            const int16_t left = (x == 0) ? SCREEN_W - 1 : x - 1;
            const int16_t right = (x == SCREEN_W - 1) ? 0 : x + 1;

            const uint16_t sum =
                static_cast<uint16_t>(heat[y + 1][left]) +
                static_cast<uint16_t>(heat[y + 1][x]) +
                static_cast<uint16_t>(heat[y + 1][right]) +
                static_cast<uint16_t>(
                    heat[(y + 2 < FIRE_H) ? y + 2 : FIRE_H - 1][x]);

            uint8_t value = static_cast<uint8_t>(sum / 4);
            const uint8_t cooling = static_cast<uint8_t>(
                Math::random(COOLING_MIN, COOLING_MAX + 1));

            value = (value > cooling) ? static_cast<uint8_t>(value - cooling) : 0;

            const int16_t drift = static_cast<int16_t>(Math::random(-1, 2));
            int16_t targetX = x + drift;
            if (targetX < 0) targetX += SCREEN_W;
            if (targetX >= SCREEN_W) targetX -= SCREEN_W;

            heat[y][targetX] = value;
        }
    }

    if (!burning)
    {
        for (int16_t y = FIRE_H - SOURCE_ROWS; y < FIRE_H; ++y)
        {
            for (int16_t x = 0; x < SCREEN_W; ++x)
            {
                heat[y][x] = 0;
            }
        }
    }
}

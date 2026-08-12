#include "TinyStarfield.h"

using namespace PRUZEA;

void TinyStarfield::onInit(Storage& storage)
{
    (void)storage;

    resetAllStars();
    warpStartMsec = 0;
    warpActive = false;
    dirty = true;
}

Game::GameState TinyStarfield::onUpdate(
    Input& input,
    Audio& audio,
    Storage& storage,
    float deltaSec)
{
    (void)storage;

    const uint32_t now = Platform::getMsec();

    if (input.justPressed(Input::A))
    {
        warpActive = true;
        warpStartMsec = now;
        audio.playSE(&Audio::SE::NO_8, 0.55f);
    }

    if (input.justPressed(Input::B))
    {
        resetAllStars();
        warpActive = false;
        warpStartMsec = 0;
        audio.playSE(&Audio::SE::NO_2, 0.45f);
    }

    if (input.justPressed(Input::START))
    {
        return GameState::TERMINATE_REQUEST;
    }

    if (warpActive &&
        Platform::elapsed(now, warpStartMsec, WARP_MSEC))
    {
        warpActive = false;
    }

    updateStars(deltaSec);
    dirty = true;

    return GameState::RUNNING;
}

bool TinyStarfield::onDraw(
    Graphics& graphics,
    bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty)
    {
        return false;
    }

    graphics.fillScreen(Graphics::BLACK);

    for (uint8_t i = 0; i < STAR_COUNT; ++i)
    {
        drawStar(graphics, stars[i]);
    }

    graphics.drawString(
        warpActive ? "WARP" : "A:WARP",
        2,
        2,
        Graphics::WHITE,
        Graphics::SIZE_10);

    graphics.drawString(
        "B:RST",
        SCREEN_W - 2,
        2,
        Graphics::WHITE,
        Graphics::SIZE_10,
        Graphics::HorizontalAlign::RIGHT,
        Graphics::VerticalAlign::TOP);

    dirty = false;
    return true;
}

void TinyStarfield::onTerminate(Storage& storage)
{
    (void)storage;
}

void TinyStarfield::resetStar(
    Star& star,
    bool randomDepth)
{
    int16_t px = static_cast<int16_t>(Math::random(SCREEN_W)) - CENTER_X;
    int16_t py = static_cast<int16_t>(Math::random(SCREEN_H)) - CENTER_Y;

    if (px > -4 && px < 4)
    {
        px = (px < 0) ? -4 : 4;
    }

    if (py > -3 && py < 3)
    {
        py = (py < 0) ? -3 : 3;
    }

    star.x = static_cast<float>(px);
    star.y = static_cast<float>(py);

    if (randomDepth)
    {
        star.z =
            Math::randomFloat(NEAR_Z, FAR_Z);
    }
    else
    {
        star.z = FAR_Z;
    }

    star.previousZ = star.z;
}

void TinyStarfield::resetAllStars()
{
    for (uint8_t i = 0; i < STAR_COUNT; ++i)
    {
        resetStar(stars[i], true);
    }

    dirty = true;
}

void TinyStarfield::updateStars(float deltaSec)
{
    const float speed =
        warpActive ? WARP_SPEED : NORMAL_SPEED;

    for (uint8_t i = 0; i < STAR_COUNT; ++i)
    {
        Star& star = stars[i];

        star.previousZ = star.z;
        star.z -= speed * deltaSec;

        if (star.z <= NEAR_Z)
        {
            resetStar(star, false);
            continue;
        }

        int16_t screenX = 0;
        int16_t screenY = 0;

        if (!project(
                star.x,
                star.y,
                star.z,
                screenX,
                screenY))
        {
            resetStar(star, false);
        }
    }
}

void TinyStarfield::drawStar(
    Graphics& graphics,
    const Star& star) const
{
    int16_t x = 0;
    int16_t y = 0;

    if (!project(star.x, star.y, star.z, x, y))
    {
        return;
    }

    if (warpActive)
    {
        int16_t previousX = 0;
        int16_t previousY = 0;

        if (project(
                star.x,
                star.y,
                star.previousZ + 2.8f,
                previousX,
                previousY))
        {
            graphics.drawLine(
                previousX,
                previousY,
                x,
                y,
                Graphics::WHITE);
            return;
        }
    }

    graphics.drawPixel(x, y, Graphics::WHITE);

    if (star.z < 14.0f)
    {
        if (x + 1 < SCREEN_W)
        {
            graphics.drawPixel(
                x + 1,
                y,
                Graphics::WHITE);
        }

        if (y + 1 < SCREEN_H)
        {
            graphics.drawPixel(
                x,
                y + 1,
                Graphics::WHITE);
        }
    }
}

bool TinyStarfield::project(
    float x,
    float y,
    float z,
    int16_t& screenX,
    int16_t& screenY) const
{
    if (z <= NEAR_Z)
    {
        return false;
    }

    const float scale = PROJECTION_SCALE / z;

    screenX = static_cast<int16_t>(
        CENTER_X + x * scale);

    screenY = static_cast<int16_t>(
        CENTER_Y + y * scale);

    return
        screenX >= 0 &&
        screenX < SCREEN_W &&
        screenY >= 0 &&
        screenY < SCREEN_H;
}

#include "WireframeTunnel.h"

#include <cstdlib>

using namespace PRUZEA;

namespace
{
constexpr float PI = 3.14159265358979323846f;
constexpr float AUTO_X_AMPLITUDE = 18.0f;
constexpr float AUTO_Y_AMPLITUDE = 10.0f;
constexpr float AUTO_X_SPEED = 0.72f;
constexpr float AUTO_Y_SPEED = 0.46f;
constexpr float TWIST_SPEED = 0.34f;
}

void WireframeTunnel::onInit(Storage& storage)
{
    (void)storage;
    resetTunnel();
    dirty = true;
}

Game::GameState WireframeTunnel::onUpdate(
    Input& input,
    Audio& audio,
    Storage& storage,
    float deltaSec)
{
    (void)storage;

    if (input.justPressed(Input::A))
    {
        autoDrift = !autoDrift;
        audio.playSE(
            autoDrift ? &Audio::SE::NO_8 : &Audio::SE::NO_2,
            0.55f);
        dirty = true;
    }

    if (input.justPressed(Input::B))
    {
        resetTunnel();
        audio.playSE(&Audio::SE::NO_2, 0.55f);
        dirty = true;
    }

    if (input.justPressed(Input::START))
    {
        return GameState::TERMINATE_REQUEST;
    }

    updateView(input, deltaSec);
    updateRings(deltaSec);

    timeSec += deltaSec;
    dirty = true;

    return GameState::RUNNING;
}

bool WireframeTunnel::onDraw(
    Graphics& graphics,
    bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty)
    {
        return false;
    }

    graphics.fillScreen(Graphics::BLACK);

    graphics.drawString(
        "WIREFRAME TUNNEL",
        CENTER_X,
        8,
        Graphics::CYAN,
        Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::TOP);

    drawTunnel(graphics);
    drawHud(graphics);

    dirty = false;
    return true;
}

void WireframeTunnel::onTerminate(Storage& storage)
{
    (void)storage;
}

void WireframeTunnel::resetTunnel()
{
    for (uint8_t i = 0; i < RING_COUNT; ++i)
    {
        rings[i].z = NEAR_Z + i * RING_SPACING;
        rings[i].twist = i * 0.13f;
    }

    cameraX = 0.0f;
    cameraY = 0.0f;
    manualX = 0.0f;
    manualY = 0.0f;
    timeSec = 0.0f;
    autoDrift = true;
}

void WireframeTunnel::updateRings(float deltaSec)
{
    for (uint8_t i = 0; i < RING_COUNT; ++i)
    {
        rings[i].z -= FORWARD_SPEED * deltaSec;
        rings[i].twist += TWIST_SPEED * deltaSec;

        if (rings[i].z < NEAR_Z)
        {
            rings[i].z += RING_SPACING * RING_COUNT;
            rings[i].twist += 0.21f;
        }
    }
}

void WireframeTunnel::updateView(
    Input& input,
    float deltaSec)
{
    float dx = 0.0f;
    float dy = 0.0f;

    if (input.pressed(Input::LEFT))  dx -= 1.0f;
    if (input.pressed(Input::RIGHT)) dx += 1.0f;
    if (input.pressed(Input::UP))    dy -= 1.0f;
    if (input.pressed(Input::DOWN))  dy += 1.0f;

    if (dx != 0.0f || dy != 0.0f)
    {
        Math::normalize(dx, dy);
        manualX += dx * VIEW_SPEED * deltaSec;
        manualY += dy * VIEW_SPEED * deltaSec;

        manualX = Math::clamp(manualX, -44.0f, 44.0f);
        manualY = Math::clamp(manualY, -30.0f, 30.0f);
    }

    float autoX = 0.0f;
    float autoY = 0.0f;

    if (autoDrift)
    {
        autoX = Math::sin(timeSec * AUTO_X_SPEED) * AUTO_X_AMPLITUDE;
        autoY = Math::sin(timeSec * AUTO_Y_SPEED) * AUTO_Y_AMPLITUDE;
    }

    cameraX = manualX + autoX;
    cameraY = manualY + autoY;
}

WireframeTunnel::Point2D WireframeTunnel::project(
    float x,
    float y,
    float z) const
{
    Point2D point = {};

    if (z <= 1.0f)
    {
        point.visible = false;
        return point;
    }

    const float scale = FOCAL / z;

    point.x = static_cast<int16_t>(
        CENTER_X + (x - cameraX) * scale);

    point.y = static_cast<int16_t>(
        CENTER_Y + (y - cameraY) * scale);

    point.visible =
        point.x > -80 &&
        point.x < SCREEN_W + 80 &&
        point.y > -80 &&
        point.y < SCREEN_H + 80;

    return point;
}

Graphics::Color WireframeTunnel::depthColor(float z) const
{
    float t = 1.0f - (z - NEAR_Z) / (FAR_Z - NEAR_Z);
    t = Math::clamp(t, 0.0f, 1.0f);

    const uint8_t r = static_cast<uint8_t>(24.0f + 52.0f * t);
    const uint8_t g = static_cast<uint8_t>(70.0f + 165.0f * t);
    const uint8_t b = static_cast<uint8_t>(92.0f + 163.0f * t);

    return Graphics::rgb565(r, g, b);
}

void WireframeTunnel::drawTunnel(Graphics& graphics)
{
    drawConnections(graphics);

    for (uint8_t i = 0; i < RING_COUNT; ++i)
    {
        drawRing(graphics, rings[i]);
    }

    graphics.drawCircle(
        CENTER_X - static_cast<int16_t>(cameraX * 0.15f),
        CENTER_Y - static_cast<int16_t>(cameraY * 0.15f),
        2,
        Graphics::WHITE);
}

void WireframeTunnel::drawRing(
    Graphics& graphics,
    const Ring& ring)
{
    const float c = Math::cos(ring.twist);
    const float s = Math::sin(ring.twist);

    const float corners[4][2] = {
        {-HALF_W, -HALF_H},
        { HALF_W, -HALF_H},
        { HALF_W,  HALF_H},
        {-HALF_W,  HALF_H}
    };

    Point2D points[4];

    for (uint8_t i = 0; i < 4; ++i)
    {
        const float x =
            corners[i][0] * c -
            corners[i][1] * s;

        const float y =
            corners[i][0] * s +
            corners[i][1] * c;

        points[i] = project(x, y, ring.z);
    }

    const Graphics::Color color = depthColor(ring.z);

    for (uint8_t i = 0; i < 4; ++i)
    {
        const uint8_t next = (i + 1) & 3;

        if (points[i].visible || points[next].visible)
        {
            graphics.drawLine(
                points[i].x,
                points[i].y,
                points[next].x,
                points[next].y,
                color);
        }
    }
}

void WireframeTunnel::drawConnections(Graphics& graphics)
{
    for (uint8_t i = 0; i < RING_COUNT; ++i)
    {
        uint8_t nextIndex = i + 1;

        if (nextIndex >= RING_COUNT)
        {
            nextIndex = 0;
        }

        const Ring& a = rings[i];
        const Ring& b = rings[nextIndex];

        if (Math::abs(a.z - b.z) > RING_SPACING * 1.5f)
        {
            continue;
        }

        const float ca = Math::cos(a.twist);
        const float sa = Math::sin(a.twist);
        const float cb = Math::cos(b.twist);
        const float sb = Math::sin(b.twist);

        const float corners[4][2] = {
            {-HALF_W, -HALF_H},
            { HALF_W, -HALF_H},
            { HALF_W,  HALF_H},
            {-HALF_W,  HALF_H}
        };

        for (uint8_t corner = 0; corner < 4; ++corner)
        {
            const float ax =
                corners[corner][0] * ca -
                corners[corner][1] * sa;

            const float ay =
                corners[corner][0] * sa +
                corners[corner][1] * ca;

            const float bx =
                corners[corner][0] * cb -
                corners[corner][1] * sb;

            const float by =
                corners[corner][0] * sb +
                corners[corner][1] * cb;

            const Point2D pa = project(ax, ay, a.z);
            const Point2D pb = project(bx, by, b.z);

            if (pa.visible || pb.visible)
            {
                graphics.drawLine(
                    pa.x,
                    pa.y,
                    pb.x,
                    pb.y,
                    depthColor((a.z + b.z) * 0.5f));
            }
        }
    }
}

void WireframeTunnel::drawHud(Graphics& graphics)
{
    graphics.fillRoundRect(
        53,
        205,
        214,
        26,
        6,
        Graphics::rgb565(4, 16, 24));

    graphics.drawRoundRect(
        53,
        205,
        214,
        26,
        6,
        1,
        Graphics::rgb565(44, 112, 132));

    graphics.drawString(
        autoDrift
            ? "D-PAD VIEW   A AUTO:ON   B RESET"
            : "D-PAD VIEW   A AUTO:OFF  B RESET",
        CENTER_X,
        212,
        Graphics::WHITE,
        Graphics::SIZE_10,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::TOP);
}

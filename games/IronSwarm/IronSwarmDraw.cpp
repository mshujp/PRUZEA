#include "IronSwarm.h"
#include <cstdio>

using namespace PRUZEA;

namespace
{
    constexpr Graphics::Color GROUND_0 = Graphics::rgb565(24, 34, 24);
    constexpr Graphics::Color GROUND_1 = Graphics::rgb565(29, 42, 28);
    constexpr Graphics::Color GRID = Graphics::rgb565(38, 53, 36);
    constexpr Graphics::Color ROAD = Graphics::rgb565(60, 59, 51);
    constexpr Graphics::Color ROAD_EDGE = Graphics::rgb565(80, 78, 66);
    constexpr Graphics::Color RIVER = Graphics::rgb565(27, 80, 115);
    constexpr Graphics::Color RIVER_LINE = Graphics::rgb565(44, 114, 150);
    constexpr Graphics::Color BRIDGE = Graphics::rgb565(116, 101, 72);
    constexpr Graphics::Color BRIDGE_EDGE = Graphics::rgb565(158, 136, 94);

    constexpr Graphics::Color ROCK = Graphics::rgb565(79, 83, 72);
    constexpr Graphics::Color ROCK_EDGE = Graphics::rgb565(112, 116, 101);
    constexpr Graphics::Color RUIN = Graphics::rgb565(88, 74, 63);
    constexpr Graphics::Color RUIN_EDGE = Graphics::rgb565(132, 111, 89);

    constexpr Graphics::Color TANK_BODY = Graphics::rgb565(76, 126, 72);
    constexpr Graphics::Color TANK_DARK = Graphics::rgb565(43, 70, 42);
    constexpr Graphics::Color TURRET = Graphics::rgb565(115, 158, 88);

    constexpr Graphics::Color SCOUT_COLOR = Graphics::rgb565(176, 65, 48);
    constexpr Graphics::Color GUNNER_COLOR = Graphics::rgb565(194, 125, 45);
    constexpr Graphics::Color HEAVY_COLOR = Graphics::rgb565(128, 68, 58);
    constexpr Graphics::Color BOSS_COLOR = Graphics::rgb565(180, 38, 45);

    constexpr Graphics::Color EXP_COLOR = Graphics::rgb565(74, 220, 190);
    constexpr Graphics::Color PANEL = Graphics::rgb565(19, 27, 31);
    constexpr Graphics::Color MISSION_COLOR = Graphics::rgb565(255, 214, 65);
    constexpr Graphics::Color BOMB_COLOR = Graphics::rgb565(255, 135, 40);
    constexpr Graphics::Color ALLY_COLOR = Graphics::rgb565(80, 170, 230);
    constexpr Graphics::Color SUPPLY_COLOR = Graphics::rgb565(230, 205, 80);

    constexpr float GRID_SIZE = 32.0f;
    constexpr float CAMERA_HALF_W = 160.0f;
    constexpr float CAMERA_HALF_H = 120.0f;
}

bool IronSwarm::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty) {
        return false;
    }

    graphics.resetCamera();
    graphics.resetClipRect();
    graphics.fillScreen(GROUND_0);

    if (mode == MODE_TITLE) {
        drawTitle(graphics);
        dirty = false;
        return true;
    }

    if (mode == MODE_RESULT) {
        drawResult(graphics);
        dirty = false;
        return true;
    }

    if (mode == MODE_RANKING) {
        drawRanking(graphics);
        dirty = false;
        return true;
    }

    Graphics::Camera camera;

    float cameraX = Math::clamp(
        player.x - CAMERA_HALF_W, 0.0f,
        static_cast<float>(WORLD_W - SCREEN_W));
    float cameraY = Math::clamp(
        player.y - CAMERA_HALF_H, 0.0f,
        static_cast<float>(WORLD_H - SCREEN_H));

    if (cameraShakeTimer > 0.0f && cameraShakeDuration > 0.0f) {
        const float fade =
            Math::clamp(cameraShakeTimer / cameraShakeDuration, 0.0f, 1.0f);
        const float strength = cameraShakeStrength * fade;
        cameraX += Math::sin(gameTime * 173.0f) * strength;
        cameraY += Math::sin(gameTime * 229.0f + 1.3f) * strength;
    }

    camera.x = sx(cameraX);
    camera.y = sy(cameraY);
    camera.zoom = 1.0f;
    camera.zoomCenterX = SCREEN_W / 2;
    camera.zoomCenterY = SCREEN_H / 2;
    graphics.setCamera(camera);

    drawWorld(graphics);

    for (int i = 0; i < MAX_ORBS; ++i) {
        if (!orbs[i].active) continue;

        Graphics::Color color = EXP_COLOR;
        uint16_t radius = 3;
        uint16_t ring = 5;

        if (orbs[i].value >= 4) {
            color = Graphics::MAGENTA;
            radius = 5;
            ring = 7;
        } else if (orbs[i].value >= 2) {
            color = Graphics::YELLOW;
            radius = 4;
            ring = 6;
        }

        graphics.fillCircle(
            sx(orbs[i].position.x), sy(orbs[i].position.y),
            radius, color);
        graphics.drawCircle(
            sx(orbs[i].position.x), sy(orbs[i].position.y),
            ring, Graphics::WHITE);
    }

    for (int i = 0; i < MAX_REPAIRS; ++i) {
        if (!repairs[i].active) continue;
        const int16_t x = sx(repairs[i].position.x);
        const int16_t y = sy(repairs[i].position.y);
        graphics.fillRect(x - 6, y - 2, 13, 5, Graphics::GREEN);
        graphics.fillRect(x - 2, y - 6, 5, 13, Graphics::GREEN);
        graphics.drawRect(x - 7, y - 7, 15, 15, Graphics::WHITE);
    }

    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) continue;
        graphics.fillCircle(
            sx(bullets[i].position.x), sy(bullets[i].position.y),
            2, Graphics::YELLOW);
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        if (!enemyBullets[i].active) continue;
        graphics.fillCircle(
            sx(enemyBullets[i].position.x), sy(enemyBullets[i].position.y),
            3, Graphics::ORANGE);
    }

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (enemies[i].active) drawEnemy(graphics, enemies[i]);
    }

    drawParticles(graphics);

    if (destroyBlastTimer > 0.0f) {
        const float t =
            Math::clamp(destroyBlastTimer / 0.48f, 0.0f, 1.0f);
        const uint16_t outerRadius =
            static_cast<uint16_t>(10.0f + (1.0f - t) * 28.0f);
        const uint16_t innerRadius =
            static_cast<uint16_t>(5.0f + (1.0f - t) * 15.0f);

        graphics.fillCircle(
            sx(destroyBlastPosition.x), sy(destroyBlastPosition.y),
            innerRadius, Graphics::YELLOW);
        graphics.drawCircle(
            sx(destroyBlastPosition.x), sy(destroyBlastPosition.y),
            outerRadius, Graphics::ORANGE);
        graphics.drawCircle(
            sx(destroyBlastPosition.x), sy(destroyBlastPosition.y),
            static_cast<uint16_t>(outerRadius + 5), Graphics::RED);
    }

    drawMissionObjects(graphics);
    drawBombs(graphics);
    drawBomber(graphics);
    drawTank(graphics);

    graphics.resetCamera();

    drawHud(graphics);
    drawRadar(graphics);
    drawMission(graphics);

    if (missionNoticeTimer > 0.0f) {
        graphics.drawString(
            "MISSION", SCREEN_W / 2, 40,
            Graphics::WHITE, Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    } else if (bomber.active && !bomber.flying) {
        graphics.drawString(
            "AIR RAID", SCREEN_W / 2, 40,
            Graphics::RED, Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    }

    if (hitFlashTimer > 0.0f) {
        const uint8_t alpha = static_cast<uint8_t>(
            Math::clamp(hitFlashTimer / 0.16f, 0.0f, 1.0f) * 85.0f);
        graphics.fillRectAlpha(
            0, 0, SCREEN_W, SCREEN_H, alpha, Graphics::RED);
    }

    if (mode == MODE_READY || mode == MODE_GO) {
        drawReady(graphics);
    } else if (mode == MODE_LEVEL_UP) {
        drawLevelUp(graphics);
    } else if (mode == MODE_PAUSED) {
        drawPause(graphics);
    } else if (mode == MODE_GAME_OVER) {
        drawGameOver(graphics);
    }

    dirty = false;
    return true;
}

void IronSwarm::drawWorld(Graphics& graphics) const
{
    drawTerrain(graphics);

    const int minGridX = Math::clamp(
        static_cast<int>((player.x - 220.0f) / GRID_SIZE) - 1,
        0, WORLD_W / 32);
    const int maxGridX = Math::clamp(
        static_cast<int>((player.x + 220.0f) / GRID_SIZE) + 1,
        0, WORLD_W / 32);
    const int minGridY = Math::clamp(
        static_cast<int>((player.y - 170.0f) / GRID_SIZE) - 1,
        0, WORLD_H / 32);
    const int maxGridY = Math::clamp(
        static_cast<int>((player.y + 170.0f) / GRID_SIZE) + 1,
        0, WORLD_H / 32);

    for (int gx = minGridX; gx <= maxGridX; ++gx) {
        const int16_t x = sx(gx * GRID_SIZE);
        graphics.drawLine(
            x, sy(player.y - 170.0f),
            x, sy(player.y + 170.0f), GRID);
    }

    for (int gy = minGridY; gy <= maxGridY; ++gy) {
        const int16_t y = sy(gy * GRID_SIZE);
        graphics.drawLine(
            sx(player.x - 220.0f), y,
            sx(player.x + 220.0f), y, GRID);
    }

    drawObstacles(graphics);
}

void IronSwarm::drawParticles(Graphics& graphics) const
{
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        const Particle& p = particles[i];
        if (!p.active) continue;

        const float ratio = p.maxLife > 0.0f
            ? Math::clamp(p.life / p.maxLife, 0.0f, 1.0f)
            : 0.0f;

        if (ratio > 0.45f) {
            graphics.fillRect(
                sx(p.position.x), sy(p.position.y),
                p.size, p.size, p.color);
        } else {
            graphics.drawPixel(
                sx(p.position.x), sy(p.position.y), p.color);
        }
    }
}

void IronSwarm::drawTerrain(Graphics& graphics) const
{
    graphics.fillRect(0, 0, WORLD_W, WORLD_H, GROUND_0);

    // Main east-west road.
    graphics.fillRect(0, 560, WORLD_W, 56, ROAD);
    graphics.drawLine(0, 560, WORLD_W, 560, ROAD_EDGE);
    graphics.drawLine(0, 615, WORLD_W, 615, ROAD_EDGE);

    // Bridge approach roads. These make both bridges visibly part of the road network.
    const float bridgeCenterY[2] = {
        BRIDGE_0_Y + BRIDGE_H * 0.5f,
        BRIDGE_1_Y + BRIDGE_H * 0.5f
    };

    for (int i = 0; i < 2; ++i) {
        const int16_t roadY = sy(bridgeCenterY[i] - 20.0f);

        graphics.fillRect(0, roadY, static_cast<uint16_t>(RIVER_X), 40, ROAD);
        graphics.fillRect(
            sx(RIVER_X + RIVER_W), roadY,
            static_cast<uint16_t>(WORLD_W - (RIVER_X + RIVER_W)), 40, ROAD);

        graphics.drawLine(0, roadY, sx(RIVER_X), roadY, ROAD_EDGE);
        graphics.drawLine(
            sx(RIVER_X + RIVER_W), roadY,
            WORLD_W, roadY, ROAD_EDGE);
    }

    // North-south connectors between the main road and bridge approaches.
    graphics.fillRect(
        590, sy(bridgeCenterY[0] - 20.0f),
        40, static_cast<uint16_t>(bridgeCenterY[1] - bridgeCenterY[0] + 40.0f),
        ROAD);
    graphics.fillRect(
        970, sy(bridgeCenterY[0] - 20.0f),
        40, static_cast<uint16_t>(bridgeCenterY[1] - bridgeCenterY[0] + 40.0f),
        ROAD);

    // River.
    graphics.fillRect(
        sx(RIVER_X), 0,
        static_cast<uint16_t>(RIVER_W), WORLD_H, RIVER);

    for (int y = 16; y < WORLD_H; y += 28) {
        graphics.drawLine(
            sx(RIVER_X + 8.0f), y,
            sx(RIVER_X + RIVER_W - 8.0f), y + 6,
            RIVER_LINE);
    }

    // Two bridges.
    const float bridgeY[2] = {BRIDGE_0_Y, BRIDGE_1_Y};
    for (int i = 0; i < 2; ++i) {
        graphics.fillRect(
            sx(RIVER_X - 7.0f), sy(bridgeY[i]),
            static_cast<uint16_t>(RIVER_W + 14.0f),
            static_cast<uint16_t>(BRIDGE_H),
            BRIDGE);
        graphics.drawRect(
            sx(RIVER_X - 7.0f), sy(bridgeY[i]),
            static_cast<uint16_t>(RIVER_W + 14.0f),
            static_cast<uint16_t>(BRIDGE_H),
            2, BRIDGE_EDGE);
        graphics.drawLine(
            sx(RIVER_X + RIVER_W * 0.5f), sy(bridgeY[i]),
            sx(RIVER_X + RIVER_W * 0.5f), sy(bridgeY[i] + BRIDGE_H),
            Graphics::DARKGRAY);
    }
}

void IronSwarm::drawObstacles(Graphics& graphics) const
{
    for (int i = 0; i < OBSTACLE_COUNT; ++i) {
        const Obstacle& o = OBSTACLES[i];

        const Graphics::Color fill =
            o.type == OBSTACLE_ROCK ? ROCK : RUIN;
        const Graphics::Color edge =
            o.type == OBSTACLE_ROCK ? ROCK_EDGE : RUIN_EDGE;

        graphics.fillRoundRect(
            sx(o.x), sy(o.y),
            static_cast<uint16_t>(o.w),
            static_cast<uint16_t>(o.h),
            o.type == OBSTACLE_ROCK ? 8 : 2,
            fill);
        graphics.drawRoundRect(
            sx(o.x), sy(o.y),
            static_cast<uint16_t>(o.w),
            static_cast<uint16_t>(o.h),
            o.type == OBSTACLE_ROCK ? 8 : 2,
            edge);

        if (o.type == OBSTACLE_RUIN) {
            graphics.drawLine(
                sx(o.x + 8.0f), sy(o.y + 6.0f),
                sx(o.x + o.w - 8.0f), sy(o.y + o.h - 6.0f),
                Graphics::DARKGRAY);
        }
    }
}

void IronSwarm::drawTank(Graphics& graphics) const
{
    if (invincibleTimer > 0.0f) {
        const int blink = static_cast<int>(invincibleTimer * 16.0f);
        if ((blink & 1) == 0) return;
    }

    const Vector2 forward(Math::cos(bodyAngle), Math::sin(bodyAngle));
    const Vector2 side(-forward.y, forward.x);

    const Vector2 p0 = player + forward * 10.0f + side * 7.0f;
    const Vector2 p1 = player + forward * 10.0f - side * 7.0f;
    const Vector2 p2 = player - forward * 10.0f - side * 7.0f;
    const Vector2 p3 = player - forward * 10.0f + side * 7.0f;

    graphics.fillTriangle(
        sx(p0.x), sy(p0.y), sx(p1.x), sy(p1.y),
        sx(p2.x), sy(p2.y), TANK_BODY);
    graphics.fillTriangle(
        sx(p0.x), sy(p0.y), sx(p2.x), sy(p2.y),
        sx(p3.x), sy(p3.y), TANK_BODY);

    const Vector2 trackA = player + side * 8.0f;
    const Vector2 trackB = player - side * 8.0f;

    graphics.drawLine(
        sx(trackA.x - forward.x * 9.0f),
        sy(trackA.y - forward.y * 9.0f),
        sx(trackA.x + forward.x * 9.0f),
        sy(trackA.y + forward.y * 9.0f),
        TANK_DARK);
    graphics.drawLine(
        sx(trackB.x - forward.x * 9.0f),
        sy(trackB.y - forward.y * 9.0f),
        sx(trackB.x + forward.x * 9.0f),
        sy(trackB.y + forward.y * 9.0f),
        TANK_DARK);

    graphics.fillCircle(sx(player.x), sy(player.y), 5, TURRET);

    const Vector2 gun(Math::cos(turretAngle), Math::sin(turretAngle));
    const Vector2 muzzle = player + gun * 14.0f;

    graphics.drawLine(
        sx(player.x), sy(player.y),
        sx(muzzle.x), sy(muzzle.y),
        Graphics::WHITE);
    graphics.fillCircle(
        sx(muzzle.x), sy(muzzle.y), 2, Graphics::YELLOW);
}

void IronSwarm::drawEnemy(Graphics& graphics, const Enemy& enemy) const
{
    Graphics::Color color = SCOUT_COLOR;
    if (enemy.type == ENEMY_GUNNER) color = GUNNER_COLOR;
    else if (enemy.type == ENEMY_HEAVY) color = HEAVY_COLOR;
    else if (enemy.type == ENEMY_BOSS) color = BOSS_COLOR;

    graphics.fillCircle(
        sx(enemy.position.x), sy(enemy.position.y),
        static_cast<uint16_t>(enemy.radius), color);
    graphics.drawCircle(
        sx(enemy.position.x), sy(enemy.position.y),
        static_cast<uint16_t>(enemy.radius), Graphics::BLACK);

    const Vector2 toPlayer = player - enemy.position;
    const Vector2 direction = toPlayer.length() > 0.001f
        ? toPlayer.normalized() : Vector2(1.0f, 0.0f);
    const Vector2 nose =
        enemy.position + direction * (enemy.radius + 4.0f);

    graphics.drawLine(
        sx(enemy.position.x), sy(enemy.position.y),
        sx(nose.x), sy(nose.y),
        enemy.type == ENEMY_BOSS ?
            Graphics::YELLOW : Graphics::ORANGE);

    if (enemy.type == ENEMY_GUNNER) {
        graphics.drawCircle(
            sx(enemy.position.x), sy(enemy.position.y),
            3, Graphics::YELLOW);
    } else if (enemy.type == ENEMY_HEAVY) {
        graphics.fillRect(
            sx(enemy.position.x - 3.0f),
            sy(enemy.position.y - 3.0f),
            6, 6, Graphics::DARKGRAY);
    } else if (enemy.type == ENEMY_BOSS) {
        graphics.drawCircle(
            sx(enemy.position.x), sy(enemy.position.y),
            static_cast<uint16_t>(enemy.radius + 4.0f),
            Graphics::RED);
    }
}

void IronSwarm::drawMissionObjects(Graphics& graphics) const
{
    if (!mission.active) return;

    if (mission.type == MISSION_BREAKTHROUGH) {
        graphics.drawCircle(
            sx(mission.targetPosition.x), sy(mission.targetPosition.y),
            18, MISSION_COLOR);
        graphics.drawCircle(
            sx(mission.targetPosition.x), sy(mission.targetPosition.y),
            8, Graphics::WHITE);
    }

    if (mission.type == MISSION_DESTROY && missionTarget.active) {
        const int16_t x = sx(missionTarget.position.x);
        const int16_t y = sy(missionTarget.position.y);

        graphics.fillRect(x - 20, y - 18, 40, 36, Graphics::DARKGRAY);
        graphics.drawRect(x - 20, y - 18, 40, 36, 2, Graphics::RED);
        graphics.fillCircle(x, y, 8, Graphics::rgb565(92, 92, 86));
        graphics.drawLine(x, y, x + 25, y - 8, Graphics::RED);
        graphics.drawCircle(x, y, 13, Graphics::ORANGE);

        const int hpW = missionTarget.maxHp > 0
            ? (44 * missionTarget.hp) / missionTarget.maxHp : 0;
        graphics.fillRect(x - 22, y - 27, 44, 4, Graphics::DARKGRAY);
        graphics.fillRect(x - 22, y - 27, hpW, 4, Graphics::RED);
    }

    if (mission.type == MISSION_ESCORT && escortVehicle.active) {
        const int16_t x = sx(escortVehicle.position.x);
        const int16_t y = sy(escortVehicle.position.y);
        graphics.fillRect(x - 8, y - 5, 16, 10, ALLY_COLOR);
        graphics.fillCircle(x + 5, y + 5, 3, Graphics::DARKGRAY);
        graphics.fillCircle(x - 5, y + 5, 3, Graphics::DARKGRAY);

        if (escortVehicle.joined) {
            graphics.drawCircle(x, y, 12, Graphics::CYAN);
            graphics.drawCircle(
                sx(mission.targetPosition.x),
                sy(mission.targetPosition.y),
                18, MISSION_COLOR);
        }
    }

    if (mission.type == MISSION_INTERCEPT && supplyVehicle.active) {
        const int16_t x = sx(supplyVehicle.position.x);
        const int16_t y = sy(supplyVehicle.position.y);

        graphics.fillRect(x - 9, y - 5, 18, 10, SUPPLY_COLOR);
        graphics.fillRect(x - 4, y - 9, 9, 5, SUPPLY_COLOR);
        graphics.fillCircle(x - 6, y + 5, 3, Graphics::DARKGRAY);
        graphics.fillCircle(x + 6, y + 5, 3, Graphics::DARKGRAY);

        const int hpW = supplyVehicle.maxHp > 0
            ? (24 * supplyVehicle.hp) / supplyVehicle.maxHp : 0;
        graphics.fillRect(x - 12, y - 16, 24, 3, Graphics::DARKGRAY);
        graphics.fillRect(x - 12, y - 16, hpW, 3, Graphics::YELLOW);
    }
}

void IronSwarm::drawBomber(Graphics& graphics) const
{
    if (!bomber.active || !bomber.flying) return;

    const Vector2 dir = bomber.direction;
    const Vector2 side(-dir.y, dir.x);

    const Vector2 nose = bomber.position + dir * 11.0f;
    const Vector2 left =
        bomber.position - dir * 8.0f + side * 8.0f;
    const Vector2 right =
        bomber.position - dir * 8.0f - side * 8.0f;

    graphics.fillTriangle(
        sx(nose.x), sy(nose.y),
        sx(left.x), sy(left.y),
        sx(right.x), sy(right.y),
        Graphics::LIGHTGRAY);

    const Vector2 wingL = bomber.position + side * 13.0f;
    const Vector2 wingR = bomber.position - side * 13.0f;
    graphics.drawLine(
        sx(wingL.x), sy(wingL.y),
        sx(wingR.x), sy(wingR.y),
        Graphics::WHITE);
}

void IronSwarm::drawBombs(Graphics& graphics) const
{
    for (int i = 0; i < MAX_BOMBS; ++i) {
        const Bomb& bomb = bombs[i];
        if (!bomb.active) continue;

        if (!bomb.exploded) {
            const float t =
                Math::clamp(bomb.fuse / 0.82f, 0.0f, 1.0f);
            const uint16_t radius = static_cast<uint16_t>(
                6.0f + (1.0f - t) * 8.0f);

            graphics.drawCircle(
                sx(bomb.position.x), sy(bomb.position.y),
                radius, BOMB_COLOR);
            graphics.drawLine(
                sx(bomb.position.x - 4.0f), sy(bomb.position.y),
                sx(bomb.position.x + 4.0f), sy(bomb.position.y),
                BOMB_COLOR);
            graphics.drawLine(
                sx(bomb.position.x), sy(bomb.position.y - 4.0f),
                sx(bomb.position.x), sy(bomb.position.y + 4.0f),
                BOMB_COLOR);
        } else {
            const float t =
                Math::clamp(bomb.explosionTimer / 0.28f, 0.0f, 1.0f);
            const uint16_t radius = static_cast<uint16_t>(
                31.0f * (1.0f - t) + 8.0f);

            graphics.fillCircle(
                sx(bomb.position.x), sy(bomb.position.y),
                radius, Graphics::ORANGE);
            graphics.drawCircle(
                sx(bomb.position.x), sy(bomb.position.y),
                31, Graphics::YELLOW);
        }
    }
}

void IronSwarm::drawHud(Graphics& graphics) const
{
    graphics.fillRectAlpha(
        0, 0, SCREEN_W, 27, 188, Graphics::BLACK);

    graphics.drawString(
        "HP", 7, 6,
        Graphics::LIGHTGRAY, Graphics::SIZE_10,
        Graphics::HorizontalAlign::LEFT,
        Graphics::VerticalAlign::TOP);

    for (int i = 0; i < maxHp && i < 8; ++i) {
        const Graphics::Color color =
            i < hp ? Graphics::GREEN : Graphics::DARKGRAY;
        graphics.fillRect(27 + i * 10, 7, 7, 7, color);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "LV %d", level);
    graphics.drawString(
        buf, 111, 6,
        Graphics::YELLOW, Graphics::SIZE_10,
        Graphics::HorizontalAlign::LEFT,
        Graphics::VerticalAlign::TOP);

    snprintf(
        buf, sizeof(buf), "%lu",
        static_cast<unsigned long>(score));
    graphics.drawString(
        buf, 230, 6,
        Graphics::CYAN, Graphics::SIZE_13,
        Graphics::HorizontalAlign::RIGHT,
        Graphics::VerticalAlign::TOP);

    const int barX = 111;
    const int barY = 19;
    const int barW = 95;

    graphics.fillRect(
        barX, barY, barW, 4, Graphics::DARKGRAY);
    const int expW = nextLevelExp > 0
        ? (barW * experience) / nextLevelExp : 0;
    graphics.fillRect(
        barX, barY, expW, 4, EXP_COLOR);
}

void IronSwarm::drawRadar(Graphics& graphics) const
{
    constexpr int RX = 244;
    constexpr int RY = 4;
    constexpr int RW = 72;
    constexpr int RH = 54;

    graphics.fillRectAlpha(
        RX, RY, RW, RH, 190, Graphics::BLACK);
    graphics.drawRect(
        RX, RY, RW, RH, Graphics::GRAY);

    auto mapX = [](float x) -> int16_t {
        return static_cast<int16_t>(
            244 + (x / static_cast<float>(WORLD_W)) * 72.0f);
    };
    auto mapY = [](float y) -> int16_t {
        return static_cast<int16_t>(
            4 + (y / static_cast<float>(WORLD_H)) * 54.0f);
    };

    for (int i = 0; i < 2; ++i) {
        const Vector2 bridge = getBridgeCenter(i);
        graphics.fillRect(
            mapX(bridge.x) - 2, mapY(bridge.y) - 1,
            5, 3, BRIDGE_EDGE);
    }

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemies[i].active) continue;

        const Graphics::Color c =
            enemies[i].type == ENEMY_BOSS
            ? Graphics::MAGENTA : Graphics::RED;
        const uint16_t r =
            enemies[i].type == ENEMY_BOSS ? 2 : 1;

        graphics.fillCircle(
            mapX(enemies[i].position.x),
            mapY(enemies[i].position.y),
            r, c);
    }

    if (mission.active) {
        Vector2 target = mission.targetPosition;

        if (mission.type == MISSION_HOLD_BRIDGE) {
            target = getBridgeCenter(mission.targetBridge);
        } else if (mission.type == MISSION_DESTROY && missionTarget.active) {
            target = missionTarget.position;
        } else if (mission.type == MISSION_ESCORT && escortVehicle.active) {
            target = escortVehicle.joined
                ? mission.targetPosition : escortVehicle.position;
        } else if (mission.type == MISSION_INTERCEPT && supplyVehicle.active) {
            target = supplyVehicle.position;
        }

        graphics.drawCircle(
            mapX(target.x), mapY(target.y),
            4, MISSION_COLOR);
    }

    if (escortVehicle.active) {
        graphics.fillCircle(
            mapX(escortVehicle.position.x),
            mapY(escortVehicle.position.y),
            2, Graphics::CYAN);
    }

    if (supplyVehicle.active) {
        graphics.fillRect(
            mapX(supplyVehicle.position.x) - 2,
            mapY(supplyVehicle.position.y) - 1,
            5, 3, Graphics::YELLOW);
    }

    graphics.fillTriangle(
        mapX(player.x), mapY(player.y) - 3,
        mapX(player.x) - 3, mapY(player.y) + 3,
        mapX(player.x) + 3, mapY(player.y) + 3,
        Graphics::GREEN);
}

void IronSwarm::drawMission(Graphics& graphics) const
{
    if (!mission.active) return;

    char buf[48];

    graphics.fillRectAlpha(
        6, 187, 230, 27, 175, Graphics::BLACK);

    graphics.drawString(
        getMissionName(), 12, 190,
        MISSION_COLOR, Graphics::SIZE_10,
        Graphics::HorizontalAlign::LEFT,
        Graphics::VerticalAlign::TOP);

    if (mission.type == MISSION_HOLD_BRIDGE) {
        const int progress = static_cast<int>(
            Math::clamp(mission.progress / 12.0f, 0.0f, 1.0f) *
            130.0f);
        graphics.fillRect(
            12, 205, 130, 4, Graphics::DARKGRAY);
        graphics.fillRect(
            12, 205, progress, 4, MISSION_COLOR);
    } else if (mission.type == MISSION_DESTROY && missionTarget.active) {
        graphics.drawString(
            "HOLD B: TARGET", 12, 203,
            Graphics::LIGHTGRAY, Graphics::SIZE_10,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::TOP);

        snprintf(
            buf, sizeof(buf), "HP %d/%d",
            missionTarget.hp, missionTarget.maxHp);
        graphics.drawString(
            buf, 226, 203,
            Graphics::LIGHTGRAY, Graphics::SIZE_10,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::TOP);
    } else if (mission.type == MISSION_ESCORT) {
        graphics.drawString(
            escortVehicle.joined ? "TO EXTRACTION" : "RENDEZVOUS",
            12, 203,
            Graphics::LIGHTGRAY, Graphics::SIZE_10,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::TOP);
    } else if (mission.type == MISSION_INTERCEPT && supplyVehicle.active) {
        graphics.drawString(
            "HOLD B: TARGET", 12, 203,
            Graphics::LIGHTGRAY, Graphics::SIZE_10,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::TOP);

        snprintf(
            buf, sizeof(buf), "HP %d/%d",
            supplyVehicle.hp, supplyVehicle.maxHp);
        graphics.drawString(
            buf, 226, 203,
            Graphics::LIGHTGRAY, Graphics::SIZE_10,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::TOP);
    }

    snprintf(
        buf, sizeof(buf), "%02d",
        static_cast<int>(mission.timeLeft));
    graphics.drawString(
        buf, 226, 190,
        Graphics::WHITE, Graphics::SIZE_10,
        Graphics::HorizontalAlign::RIGHT,
        Graphics::VerticalAlign::TOP);
}

void IronSwarm::drawTitle(Graphics& graphics) const
{
    graphics.fillRectGradient(
        0, 0, SCREEN_W, SCREEN_H,
        Graphics::rgb565(8, 13, 12),
        Graphics::rgb565(26, 42, 32),
        Graphics::VERTICAL_LINEAR);

    graphics.drawString(
        "IRON SWARM", SCREEN_W / 2, 46,
        Graphics::GREEN, Graphics::SIZE_32B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    const Graphics::Color lineDark = Graphics::rgb565(30, 70, 48);
    const Graphics::Color lineBright = Graphics::rgb565(90, 220, 125);
    graphics.fillRectGradient(
        64, 64, 96, 3,
        lineDark, lineBright,
        Graphics::HORIZONRAL_LINEAR);
    graphics.fillRectGradient(
        160, 64, 96, 3,
        lineBright, lineDark,
        Graphics::HORIZONRAL_LINEAR);

    graphics.drawString(
        "ARMORED SURVIVAL", SCREEN_W / 2, 77,
        Graphics::LIGHTGRAY, Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    for (int i = 0; i < MAX_TITLE_BULLETS; ++i) {
        if (!titleBullets[i].active) continue;
        graphics.fillCircle(
            sx(titleBullets[i].position.x),
            sy(titleBullets[i].position.y),
            2, Graphics::YELLOW);
    }

    const bool facingRight = titleTankSpeed >= 0.0f;
    const int16_t tx = sx(titleTankX);
    const int16_t ty = 110;

    graphics.fillRect(tx - 15, ty - 9, 30, 18, TANK_BODY);
    graphics.fillRect(tx - 18, ty - 11, 4, 22, TANK_DARK);
    graphics.fillRect(tx + 14, ty - 11, 4, 22, TANK_DARK);

    if (!facingRight) {
        graphics.drawLine(tx - 15, ty - 8, tx - 18, ty, TANK_DARK);
    } else {
        graphics.drawLine(tx + 15, ty - 8, tx + 18, ty, TANK_DARK);
    }

    graphics.fillCircle(tx, ty, 6, TURRET);

    const Vector2 titleGun(
        Math::cos(titleTurretAngle),
        Math::sin(titleTurretAngle));
    const Vector2 titleMuzzle =
        Vector2(titleTankX, static_cast<float>(ty)) + titleGun * 22.0f;

    graphics.drawLine(
        tx, ty,
        sx(titleMuzzle.x), sy(titleMuzzle.y),
        Graphics::WHITE);
    graphics.fillCircle(
        sx(titleMuzzle.x), sy(titleMuzzle.y),
        2, Graphics::YELLOW);

    char buf[40];
    snprintf(
        buf, sizeof(buf), "BEST  %lu",
        static_cast<unsigned long>(rankings[0]));
    graphics.drawString(
        buf, SCREEN_W / 2, 145,
        Graphics::CYAN, Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    graphics.drawString(
        "MOVE / SURVIVE / COMPLETE MISSIONS",
        SCREEN_W / 2, 174,
        Graphics::LIGHTGRAY, Graphics::SIZE_10,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    graphics.drawString(
        "START: PLAY   SELECT: RANKING",
        SCREEN_W / 2, 207,
        Graphics::WHITE, Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
}

void IronSwarm::drawReady(Graphics& graphics) const
{
    graphics.fillRectAlpha(
        0, 0, SCREEN_W, SCREEN_H, 110, Graphics::BLACK);

    const char* text = mode == MODE_READY ? "READY" : "GO!";

    graphics.drawString(
        text, SCREEN_W / 2, SCREEN_H / 2,
        mode == MODE_READY ? Graphics::WHITE : Graphics::YELLOW,
        Graphics::SIZE_42B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
}

void IronSwarm::drawLevelUp(Graphics& graphics) const
{
    graphics.fillRectAlpha(
        0, 0, SCREEN_W, SCREEN_H, 205, Graphics::BLACK);

    graphics.drawString(
        "FIELD UPGRADE", SCREEN_W / 2, 32,
        Graphics::YELLOW, Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    for (int i = 0; i < 3; ++i) {
        const int x = 8 + i * 104;
        const bool selected = i == selectedUpgrade;
        const Graphics::Color border =
            selected ? Graphics::YELLOW : Graphics::GRAY;
        const Graphics::Color title =
            selected ? Graphics::WHITE : Graphics::LIGHTGRAY;

        graphics.fillRoundRect(
            x, 67, 96, 108, 6, PANEL);
        graphics.drawRoundRect(
            x, 67, 96, 108, 6,
            selected ? 2 : 1, border);

        graphics.drawString(
            getUpgradeName(upgradeChoices[i]),
            x + 48, 88,
            title, Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            getUpgradeDescription(upgradeChoices[i]),
            x + 48, 124,
            Graphics::LIGHTGRAY, Graphics::SIZE_10,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(
            getUpgradeValue(upgradeChoices[i]),
            x + 48, 148,
            Graphics::YELLOW, Graphics::SIZE_13,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    }

    graphics.drawString(
        "LEFT / RIGHT   A: SELECT",
        SCREEN_W / 2, 203,
        Graphics::WHITE, Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
}

void IronSwarm::drawPause(Graphics& graphics) const
{
    graphics.fillRectAlpha(
        0, 0, SCREEN_W, SCREEN_H, 190, Graphics::BLACK);
    graphics.fillRoundRect(
        70, 82, 180, 76, 7, PANEL);
    graphics.drawRoundRect(
        70, 82, 180, 76, 7, 2, Graphics::CYAN);

    graphics.drawString(
        "PAUSED", SCREEN_W / 2, 108,
        Graphics::WHITE, Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    graphics.drawString(
        "START: RESUME", SCREEN_W / 2, 139,
        Graphics::LIGHTGRAY, Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
}

void IronSwarm::drawGameOver(Graphics& graphics) const
{
    graphics.fillRectAlpha(
        0, 0, SCREEN_W, SCREEN_H, 215, Graphics::BLACK);
    graphics.fillRoundRect(
        54, 68, 212, 104, 8, PANEL);
    graphics.drawRoundRect(
        54, 68, 212, 104, 8, 2, Graphics::RED);

    graphics.drawString(
        "TANK LOST", SCREEN_W / 2, 96,
        Graphics::RED, Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    graphics.drawString(
        "A / START: RESULT",
        SCREEN_W / 2, 142,
        Graphics::WHITE, Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
}

void IronSwarm::drawResult(Graphics& graphics) const
{
    graphics.fillRectGradient(
        0, 0, SCREEN_W, SCREEN_H,
        Graphics::rgb565(12, 16, 18),
        Graphics::rgb565(25, 30, 34),
        Graphics::VERTICAL_LINEAR);

    graphics.drawString(
        "RESULT", SCREEN_W / 2, 30,
        Graphics::WHITE, Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    const int labelX = 72;
    const int valueX = 248;
    const int rowY[4] = {68, 99, 130, 161};
    const char* labels[4] = {"SCORE", "KILLS", "LEVEL", "TIME"};

    for (int i = 0; i < 4; ++i) {
        graphics.drawString(
            labels[i], labelX, rowY[i],
            i == 0 ? Graphics::CYAN : Graphics::LIGHTGRAY,
            Graphics::SIZE_18,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::MIDDLE);
    }

    char buf[32];

    snprintf(
        buf, sizeof(buf), "%lu",
        static_cast<unsigned long>(score));
    graphics.drawString(
        buf, valueX, rowY[0],
        Graphics::CYAN, Graphics::SIZE_18,
        Graphics::HorizontalAlign::RIGHT,
        Graphics::VerticalAlign::MIDDLE);

    snprintf(
        buf, sizeof(buf), "%lu",
        static_cast<unsigned long>(kills));
    graphics.drawString(
        buf, valueX, rowY[1],
        Graphics::LIGHTGRAY, Graphics::SIZE_18,
        Graphics::HorizontalAlign::RIGHT,
        Graphics::VerticalAlign::MIDDLE);

    snprintf(buf, sizeof(buf), "%d", level);
    graphics.drawString(
        buf, valueX, rowY[2],
        Graphics::LIGHTGRAY, Graphics::SIZE_18,
        Graphics::HorizontalAlign::RIGHT,
        Graphics::VerticalAlign::MIDDLE);

    const int totalSec = static_cast<int>(gameTime);
    snprintf(
        buf, sizeof(buf), "%02d:%02d",
        totalSec / 60, totalSec % 60);
    graphics.drawString(
        buf, valueX, rowY[3],
        Graphics::LIGHTGRAY, Graphics::SIZE_18,
        Graphics::HorizontalAlign::RIGHT,
        Graphics::VerticalAlign::MIDDLE);

    if (lastRank == 0) {
        graphics.drawString(
            "NEW RECORD!",
            SCREEN_W / 2, 191,
            Graphics::YELLOW, Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    } else if (lastRank >= 0) {
        snprintf(
            buf, sizeof(buf), "RANK %d", lastRank + 1);
        graphics.drawString(
            buf, SCREEN_W / 2, 191,
            Graphics::YELLOW, Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER,
            Graphics::VerticalAlign::MIDDLE);
    }

    graphics.drawString(
        "A / START: RANKING",
        SCREEN_W / 2, 220,
        Graphics::WHITE, Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
}

void IronSwarm::drawRanking(Graphics& graphics) const
{
    graphics.fillRect(
        0, 0, SCREEN_W, SCREEN_H,
        Graphics::rgb565(13, 17, 20));

    graphics.drawString(
        "TOP 10", SCREEN_W / 2, 24,
        Graphics::WHITE, Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);

    char rankBuf[8];
    char scoreBuf[20];

    for (int i = 0; i < RANKING_COUNT; ++i) {
        snprintf(rankBuf, sizeof(rankBuf), "%2d", i + 1);
        snprintf(
            scoreBuf, sizeof(scoreBuf), "%lu",
            static_cast<unsigned long>(rankings[i]));

        const Graphics::Color color =
            (!rankingFromTitle && i == lastRank)
            ? Graphics::YELLOW
            : (i < 3 ? Graphics::CYAN : Graphics::LIGHTGRAY);

        const int y = 52 + i * 15;

        graphics.drawString(
            rankBuf, 88, y,
            color, Graphics::SIZE_13,
            Graphics::HorizontalAlign::LEFT,
            Graphics::VerticalAlign::TOP);

        graphics.drawString(
            scoreBuf, 232, y,
            color, Graphics::SIZE_13,
            Graphics::HorizontalAlign::RIGHT,
            Graphics::VerticalAlign::TOP);
    }

    graphics.drawString(
        rankingFromTitle
            ? "A / B / START: BACK"
            : "A / START: RETRY",
        SCREEN_W / 2, 224,
        Graphics::WHITE, Graphics::SIZE_13,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
}

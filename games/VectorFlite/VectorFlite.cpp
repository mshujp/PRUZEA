// VectorFlite.cpp
#include "VectorFlite.h"
#include <cstdio>
#include <cstring>

namespace PRUZEA
{

static const float PLAYER_ROT_SPEED = 4.0f;
static const float PLAYER_ACCEL = 180.0f;
static const float PLAYER_MAX_SPEED = 150.0f;
static const float PLAYER_FRICTION = 0.98f;
static const float BULLET_SPEED = 240.0f;
static const float HOMING_STRENGTH = 5.0f;
static const float ASTEROID_MIN_SPEED = 30.0f;
static const float ASTEROID_MAX_SPEED = 70.0f;
static const float SHAKE_DECAY = 5.0f;

static const uint32_t BULLET_LIFE_MS = 1500;

static const Graphics::Color COLOR_BG       = Graphics::BLACK;
static const Graphics::Color COLOR_GRID     = Graphics::rgb565(20, 24, 40);
static const Graphics::Color COLOR_PLAYER   = Graphics::CYAN;
static const Graphics::Color COLOR_BULLET   = Graphics::YELLOW;
static const Graphics::Color COLOR_ASTEROID = Graphics::MAGENTA;
static const Graphics::Color COLOR_HUD      = Graphics::WHITE;
static const Graphics::Color COLOR_ALERT    = Graphics::RED;

struct Vec2 { float x, y; };
static const Vec2 SHIP_MODEL[] = {
    { 12.0f,  0.0f },
    {-8.0f,  -6.0f },
    {-5.0f,   0.0f },
    {-8.0f,   6.0f }
};
static const int SHIP_VERTEX_COUNT = 4;

void VectorFlite::onInit(Storage& storage)
{
    saveData.clear();
    if (storage.isAvailable()) {
        saveData.load(storage, getId(), "save.ini");
    }
    highScore = saveData.getUInt32("HIGH_SCORE", 0);

    currentMode = MODE_TITLE;
    resetGame();
}

void VectorFlite::resetGame()
{
    score = 0;
    screenShakeTimer = 0.0f;

    player.x = 160.0f;
    player.y = 120.0f;
    player.vx = 0.0f;
    player.vy = 0.0f;
    player.radius = 6.0f;
    player.angle = -1.5707f;
    player.active = true;

    for (int i = 0; i < MAX_BULLETS; ++i) {
        bullets[i].active = false;
    }

    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        asteroids[i].active = false;
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles[i].active = false;
    }

    const uint32_t now = Platform::getMsec();
    lastShotMsec = 0;
    lastSpawnMsec = now;
    spawnIntervalMsec = 0;
    stateStartTime = now;
    blinkStartMsec = now;
}

void VectorFlite::spawnAsteroid()
{
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (!asteroids[i].active) {
            int edge = Math::random(4);
            float sx = 0, sy = 0;
            switch (edge) {
                case 0: sx = Math::randomFloat(0.0f, 320.0f); sy = -20.0f; break;
                case 1: sx = Math::randomFloat(0.0f, 320.0f); sy = 260.0f; break;
                case 2: sx = -20.0f; sy = Math::randomFloat(0.0f, 240.0f); break;
                case 3: sx = 340.0f; sy = Math::randomFloat(0.0f, 240.0f); break;
            }

            float targetX = player.x;
            float targetY = player.y;
            float dx = targetX - sx;
            float dy = targetY - sy;
            
            Math::normalize(dx, dy);
            
            float speed = Math::randomFloat(ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED);
            asteroids[i].x = sx;
            asteroids[i].y = sy;
            asteroids[i].vx = dx * speed;
            asteroids[i].vy = dy * speed;
            asteroids[i].radius = Math::randomFloat(12.0f, 20.0f);
            asteroids[i].angle = Math::randomFloat(0.0f, Math::TWO_PI);
            asteroids[i].active = true;
            break;
        }
    }
}

void VectorFlite::spawnExplosion(float x, float y, uint16_t color)
{
    int pCount = 0;
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles[i].active) {
            float angle = Math::randomFloat(0.0f, Math::TWO_PI);
            float speed = Math::randomFloat(30.0f, 80.0f);
            
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = Math::cos(angle) * speed;
            particles[i].vy = Math::sin(angle) * speed;
            particles[i].color = color;
            particles[i].life = 1.0f;
            particles[i].active = true;

            pCount++;
            if (pCount >= 5) break;
        }
    }
}

void VectorFlite::updateHoming(float deltaSec)
{
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) continue;

        float minDistSq = 999999.0f;
        int targetIdx = -1;

        for (int j = 0; j < MAX_ASTEROIDS; ++j) {
            if (!asteroids[j].active) continue;
            
            float distSq = Math::distanceSquared(bullets[i].x, bullets[i].y, asteroids[j].x, asteroids[j].y);
            if (distSq < minDistSq) {
                minDistSq = distSq;
                targetIdx = j;
            }
        }

        if (targetIdx != -1) {
            float dx = asteroids[targetIdx].x - bullets[i].x;
            float dy = asteroids[targetIdx].y - bullets[i].y;
            
            float targetAngle = Math::angle(dx, dy);

            bullets[i].angle = Math::moveTowardsAngle(bullets[i].angle, targetAngle, HOMING_STRENGTH * deltaSec);

            bullets[i].vx = Math::cos(bullets[i].angle) * BULLET_SPEED;
            bullets[i].vy = Math::sin(bullets[i].angle) * BULLET_SPEED;
        }
    }
}

Game::GameState VectorFlite::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    uint32_t now = Platform::getMsec();

    if (screenShakeTimer > 0.0f) {
        screenShakeTimer = Math::moveTowards(screenShakeTimer, 0.0f, SHAKE_DECAY * deltaSec);
        dirty = true;
    }

    if (currentMode != MODE_PLAYING &&
        Platform::elapsed(now, blinkStartMsec, 400)) {
        blinkStartMsec = now;
        dirty = true;
    }

    switch (currentMode) {
        case MODE_TITLE:
            if (input.justPressed(Input::A) || input.justPressed(Input::START)) {
                audio.playSE(&Audio::SE::NO_8, 1.0f);
                resetGame();
                currentMode = MODE_PLAYING;
                dirty = true;
            }
            break;

        case MODE_PLAYING:
            if (input.pressed(Input::LEFT)) {
                player.angle -= PLAYER_ROT_SPEED * deltaSec;
                dirty = true;
            }
            if (input.pressed(Input::RIGHT)) {
                player.angle += PLAYER_ROT_SPEED * deltaSec;
                dirty = true;
            }

            if (input.pressed(Input::A)) {
                float ax = Math::cos(player.angle) * PLAYER_ACCEL * deltaSec;
                float ay = Math::sin(player.angle) * PLAYER_ACCEL * deltaSec;
                player.vx += ax;
                player.vy += ay;

                float speed = Math::length(player.vx, player.vy);
                if (speed > PLAYER_MAX_SPEED) {
                    Math::normalize(player.vx, player.vy);
                    player.vx *= PLAYER_MAX_SPEED;
                    player.vy *= PLAYER_MAX_SPEED;
                }
                dirty = true;
            } else {
                player.vx *= Math::lerp(1.0f, PLAYER_FRICTION, deltaSec * 60.0f);
                player.vy *= Math::lerp(1.0f, PLAYER_FRICTION, deltaSec * 60.0f);
            }

            player.x += player.vx * deltaSec;
            player.y += player.vy * deltaSec;

            player.x = Math::wrap(player.x, 0.0f, 320.0f);
            player.y = Math::wrap(player.y, 0.0f, 240.0f);

            if (input.pressed(Input::B)) {
                if (Platform::elapsed(now, lastShotMsec, 200)) {
                    for (int i = 0; i < MAX_BULLETS; ++i) {
                        if (!bullets[i].active) {
                            bullets[i].x = player.x + Math::cos(player.angle) * 10.0f;
                            bullets[i].y = player.y + Math::sin(player.angle) * 10.0f;
                            bullets[i].vx = Math::cos(player.angle) * BULLET_SPEED;
                            bullets[i].vy = Math::sin(player.angle) * BULLET_SPEED;
                            bullets[i].angle = player.angle;
                            bullets[i].radius = 2.0f;
                            bullets[i].spawnTime = now;
                            bullets[i].active = true;

                            audio.playSE(&Audio::SE::NO_1, 0.6f);
                            lastShotMsec = now;
                            break;
                        }
                    }
                }
            }

            updateHoming(deltaSec);

            for (int i = 0; i < MAX_BULLETS; ++i) {
                if (bullets[i].active) {
                    bullets[i].x += bullets[i].vx * deltaSec;
                    bullets[i].y += bullets[i].vy * deltaSec;

                    bullets[i].x = Math::wrap(bullets[i].x, 0.0f, 320.0f);
                    bullets[i].y = Math::wrap(bullets[i].y, 0.0f, 240.0f);

                    if (Platform::elapsed(now, bullets[i].spawnTime, BULLET_LIFE_MS)) {
                        bullets[i].active = false;
                    }
                    dirty = true;
                }
            }

            if (Platform::elapsed(now, lastSpawnMsec, spawnIntervalMsec)) {
                spawnAsteroid();
                lastSpawnMsec = now;
                const float difficulty = Math::clamp(static_cast<float>(score) / 2000.0f, 0.0f, 1.0f);
                spawnIntervalMsec = static_cast<uint32_t>(
                    Tween::lerp(1500.0f, 500.0f, difficulty));
            }

            for (int i = 0; i < MAX_ASTEROIDS; ++i) {
                if (asteroids[i].active) {
                    asteroids[i].x += asteroids[i].vx * deltaSec;
                    asteroids[i].y += asteroids[i].vy * deltaSec;
                    asteroids[i].angle += 1.0f * deltaSec;

                    if (asteroids[i].x < -30.0f || asteroids[i].x > 350.0f ||
                        asteroids[i].y < -30.0f || asteroids[i].y > 270.0f) {
                        asteroids[i].active = false;
                    }
                    dirty = true;
                }
            }

            for (int i = 0; i < MAX_PARTICLES; ++i) {
                if (particles[i].active) {
                    particles[i].x += particles[i].vx * deltaSec;
                    particles[i].y += particles[i].vy * deltaSec;
                    particles[i].life -= 2.0f * deltaSec;
                    if (particles[i].life <= 0.0f) {
                        particles[i].active = false;
                    }
                    dirty = true;
                }
            }

            for (int i = 0; i < MAX_ASTEROIDS; ++i) {
                if (!asteroids[i].active) continue;

                for (int j = 0; j < MAX_BULLETS; ++j) {
                    if (!bullets[j].active) continue;

                    float limitDist = asteroids[i].radius + bullets[j].radius;
                    float distSq = Math::distanceSquared(asteroids[i].x, asteroids[i].y, bullets[j].x, bullets[j].y);

                    if (distSq < limitDist * limitDist) {
                        spawnExplosion(asteroids[i].x, asteroids[i].y, COLOR_ASTEROID);
                        audio.playSE(&Audio::SE::NO_6, 0.8f);
                        
                        asteroids[i].active = false;
                        bullets[j].active = false;
                        
                        score += 100;
                        screenShakeTimer = 0.25f;
                        dirty = true;
                        break;
                    }
                }

                if (asteroids[i].active) {
                    float limitDist = asteroids[i].radius + player.radius;
                    float distSq = Math::distanceSquared(asteroids[i].x, asteroids[i].y, player.x, player.y);

                    if (distSq < limitDist * limitDist) {
                        spawnExplosion(player.x, player.y, COLOR_PLAYER);
                        audio.playSE(&Audio::SE::NO_4, 1.0f);
                        screenShakeTimer = 0.6f;
                        
                        if (score > highScore) {
                            highScore = score;
                            if (storage.isAvailable()) {
                                saveData.setUInt32("HIGH_SCORE", highScore);
                                saveData.save(storage, getId(), "save.ini");
                            }
                        }

                        currentMode = MODE_GAME_OVER;
                        stateStartTime = now;
                        blinkStartMsec = now;
                        dirty = true;
                    }
                }
            }
            break;

        case MODE_GAME_OVER:
            if (!Platform::elapsed(now, stateStartTime, 250)) {
                dirty = true;
            }

            if (Platform::elapsed(now, stateStartTime, 1500)) {
                if (input.justPressed(Input::A) || input.justPressed(Input::START)) {
                    audio.playSE(&Audio::SE::NO_5, 1.0f);
                    resetGame();
                    currentMode = MODE_PLAYING;
                    dirty = true;
                }
            }
            break;
    }

    return GameState::RUNNING;
}

bool VectorFlite::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty) {
        return false;
    }

    if (screenShakeTimer > 0.0f) {
        const int16_t offsetX = static_cast<int16_t>(Math::random(-8, 9));
        const int16_t offsetY = static_cast<int16_t>(Math::random(-8, 9));
        graphics.setViewport(offsetX, offsetY);
    } else {
        graphics.resetViewport();
    }

    graphics.fillScreen(COLOR_BG);

    for (int x = 0; x < 320; x += 40) {
        graphics.drawLine(x, 0, x, 240, COLOR_GRID);
    }
    for (int y = 0; y < 240; y += 40) {
        graphics.drawLine(0, y, 320, y, COLOR_GRID);
    }

    if (currentMode == MODE_TITLE) {
        graphics.drawString("VECTOR FLITE", 160, 80, COLOR_PLAYER, Graphics::SIZE_32B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        
        if ((Platform::getMsec() / 400) % 2 == 0) {
            graphics.drawString("PRESS START BUTTON", 160, 150, COLOR_HUD, Graphics::SIZE_18, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }

        char highStr[64];
        std::snprintf(highStr, sizeof(highStr), "HIGH SCORE: %d", highScore);
        graphics.drawString(highStr, 160, 190, COLOR_BULLET, Graphics::SIZE_13, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    } else {

        for (int i = 0; i < MAX_PARTICLES; ++i) {
            if (particles[i].active) {
                graphics.drawPixel((int16_t)particles[i].x, (int16_t)particles[i].y, (Graphics::Color)particles[i].color);
            }
        }

        for (int i = 0; i < MAX_BULLETS; ++i) {
            if (bullets[i].active) {
                graphics.fillCircle((int16_t)bullets[i].x, (int16_t)bullets[i].y, (uint16_t)bullets[i].radius, COLOR_BULLET);
            }
        }

        for (int i = 0; i < MAX_ASTEROIDS; ++i) {
            if (asteroids[i].active) {
                graphics.fillCircle((int16_t)asteroids[i].x, (int16_t)asteroids[i].y, 2, COLOR_ASTEROID);
                graphics.drawCircle((int16_t)asteroids[i].x, (int16_t)asteroids[i].y, (uint16_t)asteroids[i].radius, COLOR_ASTEROID);
                
                float radX = Math::cos(asteroids[i].angle) * asteroids[i].radius;
                float radY = Math::sin(asteroids[i].angle) * asteroids[i].radius;
                graphics.drawLine((int16_t)asteroids[i].x, (int16_t)asteroids[i].y, 
                                  (int16_t)(asteroids[i].x + radX), (int16_t)(asteroids[i].y + radY), COLOR_ASTEROID);

                float faceX = Math::cos(player.angle);
                float faceY = Math::sin(player.angle);
                float toTargetX = asteroids[i].x - player.x;
                float toTargetY = asteroids[i].y - player.y;
                Math::normalize(toTargetX, toTargetY);

                float dotVal = Math::dot(faceX, faceY, toTargetX, toTargetY);
                if (dotVal > 0.866f && player.active) {
                    float dist = Math::distance(player.x, player.y, asteroids[i].x, asteroids[i].y);
                    if (dist < 100.0f) {
                        graphics.drawCircle((int16_t)asteroids[i].x, (int16_t)asteroids[i].y, (uint16_t)(asteroids[i].radius + 4), COLOR_ALERT);
                    }
                }
            }
        }

        if (currentMode == MODE_PLAYING) {
            Vec2 rotatedModel[SHIP_VERTEX_COUNT];
            for (int v = 0; v < SHIP_VERTEX_COUNT; ++v) {
                float rx, ry;
                Math::rotate(SHIP_MODEL[v].x, SHIP_MODEL[v].y, player.angle, rx, ry);
                rotatedModel[v].x = player.x + rx;
                rotatedModel[v].y = player.y + ry;
            }

            for (int v = 0; v < SHIP_VERTEX_COUNT; ++v) {
                int next = (v + 1) % SHIP_VERTEX_COUNT;
                graphics.drawLine((int16_t)rotatedModel[v].x, (int16_t)rotatedModel[v].y,
                                  (int16_t)rotatedModel[next].x, (int16_t)rotatedModel[next].y, COLOR_PLAYER);
            }
        }

        char scoreStr[32];
        std::snprintf(scoreStr, sizeof(scoreStr), "SCORE: %d", score);
        graphics.drawString(scoreStr, 10, 10, COLOR_HUD, Graphics::SIZE_13);

        if (currentMode == MODE_GAME_OVER) {
            const uint32_t now = Platform::getMsec();
            const float t = Math::clamp(
                static_cast<float>(now - stateStartTime) / 250.0f,
                0.0f,
                1.0f);
            const float eased = Tween::apply(t, Tween::Ease::EASE_OUT_BACK);
            const int16_t titleY = static_cast<int16_t>(
                Tween::lerp(72.0f, 100.0f, eased));

            graphics.fillRectAlpha(0, 0, 320, 240, 96, Graphics::BLACK);
            graphics.drawString("GAME OVER", 160, titleY, COLOR_ALERT, Graphics::SIZE_25B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

            if ((now / 400u) % 2u == 0u) {
                graphics.drawString("PRESS START TO RETRY", 160, 150, COLOR_HUD, Graphics::SIZE_18, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
            }
        }
    }

    dirty = false;
    return true;
}

void VectorFlite::onTerminate(Storage& storage)
{
}

} // namespace PRUZEA

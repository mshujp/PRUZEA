// VectorFlite.h
#pragma once
#include "PRUZEA.h"

namespace PRUZEA
{

class VectorFlite : public Game {
public:
    static constexpr int MAX_ASTEROIDS = 8;
    static constexpr int MAX_BULLETS = 10;
    static constexpr int MAX_PARTICLES = 20;

    struct Entity {
        float x, y;
        float vx, vy;
        float radius;
        float angle;
        bool active;
    };

    struct Bullet : public Entity {
        float targetAngle; // For homing bullets (Math::moveTowardsAngle test)
        uint32_t spawnTime;
    };

    struct Particle {
        float x, y;
        float vx, vy;
        uint16_t color;
        float life; // 1.0 down to 0.0
        bool active;
    };

    void onInit(Storage& storage) override;
    Game::GameState onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override;
    bool onDraw(Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(Storage& storage) override;

    const char* getId() const override { return "vectorflite"; }
    const char* getName() const override { return "VECTOR FLITE"; }
    const char* getMenuName() const override { return "VECTOR FLITE"; }

    uint16_t getLogicalScreenWidth() const override { return 320; }
    uint16_t getLogicalScreenHeight() const override { return 240; }
    uint16_t getTargetScreenWidth() const override { return 320; }
    uint16_t getTargetScreenHeight() const override { return 240; }

private:
    enum Mode {
        MODE_TITLE,
        MODE_PLAYING,
        MODE_GAME_OVER
    };

    Mode currentMode = MODE_TITLE;
    uint32_t score = 0;
    uint32_t highScore = 0;
    float screenShakeTimer = 0.0f;

    Entity player;
    Bullet bullets[MAX_BULLETS];
    Entity asteroids[MAX_ASTEROIDS];
    Particle particles[MAX_PARTICLES];

    uint32_t stateStartTime = 0;
    uint32_t lastShotMsec = 0;
    uint32_t lastSpawnMsec = 0;
    uint32_t spawnIntervalMsec = 0;
    uint32_t blinkStartMsec = 0;

    SaveData saveData;

    void resetGame();
    void spawnAsteroid();
    void spawnExplosion(float x, float y, uint16_t color);
    void updateHoming(float deltaSec);
};

} // namespace PRUZEA

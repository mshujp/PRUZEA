// ------------------------------------------------------------
// PRUZEA Sample Game
//
// GroundFront updated for PRUZEA 1.2.1.
//
// Included features:
// - Title / Pause / Result / Ranking screens
// - Five boss battles
// - Smaller, easier-to-read enemy bullets
// - Bomb, life-up, weapon item, homing weapon
// - One stage-specific enemy for each stage
// - Updated player craft silhouette (double-delta style)
// ------------------------------------------------------------

#pragma once
#include "PRUZEA.h"
#include <cstdint>

class GroundFront final : public PRUZEA::Game {
public:
    GroundFront();

    const char* getId() const override;
    const char* getName() const override;
    const char* getMenuName() const override;

    uint16_t getLogicalScreenWidth() const override;
    uint16_t getLogicalScreenHeight() const override;
    uint16_t getTargetScreenWidth() const override;
    uint16_t getTargetScreenHeight() const override;

    void onInit(PRUZEA::Storage& storage) override;
    GameState onUpdate(PRUZEA::Input& input, PRUZEA::Audio& audio, PRUZEA::Storage& storage, float deltaSec) override;
    bool onDraw(PRUZEA::Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(PRUZEA::Storage& storage) override;

private:
    enum InternalMode : uint8_t {
        MODE_TITLE,
        MODE_PLAYING,
        MODE_PAUSED,
        MODE_GAME_OVER,
        MODE_STAGE_CLEAR,
        MODE_RANKING
    };

    enum EnemyType : uint8_t {
        ENEMY_BIRD,
        ENEMY_BALLOON,
        ENEMY_TANK,
        ENEMY_DRONE,
        ENEMY_STAGE_SPECIAL
    };

    enum WeaponType : uint8_t {
        WEAPON_NORMAL,
        WEAPON_SPREAD,
        WEAPON_HOMING
    };

    enum PlayerBulletType : uint8_t {
        PLAYER_BULLET_NORMAL,
        PLAYER_BULLET_HOMING
    };

    enum ItemType : uint8_t {
        ITEM_NONE,
        ITEM_WEAPON,
        ITEM_BOMB,
        ITEM_LIFE
    };

    struct Player {
        float x;
        float y;
        int16_t hp;
        uint64_t lastShotMsec;
        uint64_t invincibleUntilMsec;
    };

    struct PlayerBullet {
        bool active;
        float x;
        float y;
        float vx;
        float vy;
        uint8_t power;
        PlayerBulletType type;
        float turnRate;
        uint64_t bornMsec;
    };

    struct Enemy {
        bool active;
        EnemyType type;
        float x;
        float y;
        float vx;
        float vy;
        int16_t hp;
        uint64_t bornMsec;
        uint64_t lastShotMsec;
        uint8_t phase;
    };

    struct EnemyBullet {
        bool active;
        float x;
        float y;
        float vx;
        float vy;
        uint8_t radius;
    };

    struct Boss {
        bool active;
        uint8_t type;
        float x;
        float y;
        float targetY;
        int16_t hp;
        int16_t maxHp;
        uint64_t bornMsec;
        uint64_t lastShotMsec;
        uint64_t patternMsec;
        uint8_t phase;
    };

    struct Particle {
        bool active;
        float x;
        float y;
        float vx;
        float vy;
        uint64_t endMsec;
    };

    struct Item {
        bool active;
        ItemType type;
        float x;
        float y;
        float vy;
        uint64_t bornMsec;
    };

    static constexpr uint8_t MAX_PLAYER_BULLETS = 36;
    static constexpr uint8_t MAX_ENEMIES = 18;
    static constexpr uint8_t MAX_ENEMY_BULLETS = 72;
    static constexpr uint8_t MAX_PARTICLES = 32;
    static constexpr uint8_t MAX_ITEMS = 8;
    static constexpr uint8_t RANKING_COUNT = 5;

    InternalMode mode_;
    Player player_;
    PlayerBullet playerBullets_[MAX_PLAYER_BULLETS];
    Enemy enemies_[MAX_ENEMIES];
    EnemyBullet enemyBullets_[MAX_ENEMY_BULLETS];
    Boss boss_;
    Particle particles_[MAX_PARTICLES];
    Item items_[MAX_ITEMS];

    uint32_t score_;
    uint32_t ranking_[RANKING_COUNT];
    uint8_t currentBossIndex_;
    uint32_t defeatedEnemies_;
    uint32_t escapedEnemies_;
    uint64_t runStartMsec_;
    uint64_t nextBossMsec_;
    uint64_t lastEnemySpawnMsec_;
    uint64_t resultStartMsec_;
    uint64_t shakeUntilMsec_;
    uint64_t flashUntilMsec_;
    uint64_t pauseStartMsec_;
    uint32_t backgroundSeed_;
    bool scoreSaved_;
    bool rankingLoaded_;

    WeaponType weaponType_;
    uint8_t weaponLevel_;
    uint8_t bombCount_;
    bool homingFireRight_;
    uint8_t shakeAmplitude_;
    const char* savePath_ = "ranking.txt";

    void resetRun();
    void startRun(PRUZEA::Audio& audio);
    void shiftGameTimers(uint64_t pausedMsec);
    void updatePlaying(PRUZEA::Input& input, PRUZEA::Audio& audio, PRUZEA::Storage& storage, uint64_t now);
    void updatePlayer(PRUZEA::Input& input, PRUZEA::Audio& audio, uint64_t now);
    void updatePlayerBullets(uint64_t now);
    void updateEnemies(PRUZEA::Audio& audio, uint64_t now);
    void updateEnemyBullets(PRUZEA::Audio& audio, uint64_t now);
    void updateBoss(PRUZEA::Audio& audio, PRUZEA::Storage& storage, uint64_t now);
    void updateParticles(uint64_t now);
    void updateItems(PRUZEA::Audio& audio, uint64_t now);

    void spawnEnemy(uint64_t now);
    void spawnBoss(uint64_t now, PRUZEA::Audio& audio);
    void firePlayerShot(PRUZEA::Audio& audio);
    void fireEnemyAimed(float x, float y, float speed, uint8_t radius = 2);
    void fireEnemyFan(float x, float y, uint8_t count, float speed, float spread, uint8_t radius = 2);
    void fireEnemyRing(float x, float y, uint8_t count, float speed, float angleOffset, uint8_t radius = 2);
    void addPlayerBullet(float x, float y, float vx, float vy, uint8_t power, PlayerBulletType type, float turnRate = 0.0f);
    void addEnemyBullet(float x, float y, float vx, float vy, uint8_t radius);
    void addExplosion(float x, float y, uint8_t count, uint64_t now);
    void spawnItem(float x, float y, ItemType type, uint64_t now);
    void maybeDropItem(float x, float y, bool guaranteedWeapon, uint64_t now);
    void useBomb(PRUZEA::Audio& audio, uint64_t now);

    bool hitCircle(float ax, float ay, float ar, float bx, float by, float br) const;
    void damagePlayer(PRUZEA::Audio& audio, uint64_t now);
    void finishRun(bool cleared, PRUZEA::Audio& audio, PRUZEA::Storage& storage, uint64_t now);

    void loadRanking(PRUZEA::Storage& storage);
    void registerScore();
    void saveRanking(PRUZEA::Storage& storage);

    void drawBackground(PRUZEA::Graphics& graphics, uint64_t now);
    void drawSidePanels(PRUZEA::Graphics& graphics, uint64_t now);
    void drawTitle(PRUZEA::Graphics& graphics, uint64_t now);
    void drawPlayfield(PRUZEA::Graphics& graphics, uint64_t now);
    void drawPlayer(PRUZEA::Graphics& graphics, uint64_t now);
    void drawEnemies(PRUZEA::Graphics& graphics, uint64_t now);
    void drawBoss(PRUZEA::Graphics& graphics, uint64_t now);
    void drawBullets(PRUZEA::Graphics& graphics);
    void drawParticles(PRUZEA::Graphics& graphics, uint64_t now);
    void drawItems(PRUZEA::Graphics& graphics, uint64_t now);
    void drawPause(PRUZEA::Graphics& graphics);
    void drawResult(PRUZEA::Graphics& graphics, uint64_t now);
    void drawRanking(PRUZEA::Graphics& graphics, uint64_t now);
};

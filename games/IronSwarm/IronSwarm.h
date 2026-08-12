#pragma once
#include "PRUZEA.h"

class IronSwarm : public PRUZEA::Game
{
public:
    const char* getId() const override { return "iron_swarm"; }
    const char* getName() const override { return "IRON SWARM"; }
    const char* getMenuName() const override { return "IRON SWARM"; }

    uint16_t getLogicalScreenWidth() const override { return 320; }
    uint16_t getLogicalScreenHeight() const override { return 240; }
    uint16_t getTargetScreenWidth() const override { return 320; }
    uint16_t getTargetScreenHeight() const override { return 240; }

protected:
    void onInit(PRUZEA::Storage& storage) override;
    Game::GameState onUpdate(PRUZEA::Input& input, PRUZEA::Audio& audio,
                             PRUZEA::Storage& storage, float deltaSec) override;
    bool onDraw(PRUZEA::Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(PRUZEA::Storage& storage) override;

private:
    enum Mode : uint8_t {
        MODE_TITLE,
        MODE_READY,
        MODE_GO,
        MODE_PLAYING,
        MODE_LEVEL_UP,
        MODE_PAUSED,
        MODE_GAME_OVER,
        MODE_RESULT,
        MODE_RANKING
    };

    enum Upgrade : uint8_t {
        UPGRADE_POWER,
        UPGRADE_RAPID,
        UPGRADE_MULTI,
        UPGRADE_PIERCE,
        UPGRADE_ENGINE,
        UPGRADE_ARMOR,
        UPGRADE_TURRET,
        UPGRADE_COUNT
    };

    enum EnemyType : uint8_t {
        ENEMY_SCOUT,
        ENEMY_GUNNER,
        ENEMY_HEAVY,
        ENEMY_BOSS
    };

    enum MissionType : uint8_t {
        MISSION_NONE,
        MISSION_HOLD_BRIDGE,
        MISSION_BREAKTHROUGH,
        MISSION_DESTROY,
        MISSION_ESCORT,
        MISSION_INTERCEPT,
        MISSION_COUNT
    };

    enum ObstacleType : uint8_t {
        OBSTACLE_ROCK,
        OBSTACLE_RUIN
    };

    struct Enemy {
        PRUZEA::Vector2 position;
        float radius;
        float speed;
        float shootTimer;
        int hp;
        EnemyType type;
        bool active;
    };

    struct Bullet {
        PRUZEA::Vector2 position;
        PRUZEA::Vector2 velocity;
        float life;
        int damage;
        int pierce;
        bool active;
    };

    struct EnemyBullet {
        PRUZEA::Vector2 position;
        PRUZEA::Vector2 velocity;
        float life;
        uint8_t damage;
        bool active;
    };

    struct ExpOrb {
        PRUZEA::Vector2 position;
        uint8_t value;
        bool active;
    };

    struct RepairItem {
        PRUZEA::Vector2 position;
        bool active;
    };

    struct Particle {
        PRUZEA::Vector2 position;
        PRUZEA::Vector2 velocity;
        float life;
        float maxLife;
        uint8_t size;
        PRUZEA::Graphics::Color color;
        bool active;
    };

    struct TitleBullet {
        PRUZEA::Vector2 position;
        PRUZEA::Vector2 velocity;
        float life;
        bool active;
    };

    struct Bomber {
        PRUZEA::Vector2 position;
        PRUZEA::Vector2 direction;
        float speed;
        float warningTimer;
        float bombTimer;
        float flightTimer;
        bool active;
        bool flying;
        bool enteredView;
    };

    struct Bomb {
        PRUZEA::Vector2 position;
        float fuse;
        float explosionTimer;
        bool exploded;
        bool active;
    };

    struct Obstacle {
        float x;
        float y;
        float w;
        float h;
        ObstacleType type;
    };

    struct Mission {
        MissionType type;
        PRUZEA::Vector2 targetPosition;
        PRUZEA::Vector2 targetPosition2;
        int targetBridge;
        float progress;
        float timeLeft;
        uint8_t stage;
        bool active;
    };

    struct MissionTarget {
        PRUZEA::Vector2 position;
        float radius;
        int hp;
        int maxHp;
        bool active;
    };

    struct EscortVehicle {
        PRUZEA::Vector2 position;
        float speed;
        bool joined;
        bool active;
    };

    struct SupplyVehicle {
        PRUZEA::Vector2 position;
        PRUZEA::Vector2 escapeTarget;
        float speed;
        int hp;
        int maxHp;
        bool active;
    };

    static constexpr int SCREEN_W = 320;
    static constexpr int SCREEN_H = 240;

    static constexpr int WORLD_W = 1600;
    static constexpr int WORLD_H = 1200;

    static constexpr float RIVER_X = 760.0f;
    static constexpr float RIVER_W = 80.0f;
    static constexpr float BRIDGE_0_Y = 260.0f;
    static constexpr float BRIDGE_1_Y = 820.0f;
    static constexpr float BRIDGE_H = 80.0f;

    static constexpr int MAX_ENEMIES = 18;
    static constexpr int MAX_BULLETS = 48;
    static constexpr int MAX_ENEMY_BULLETS = 36;
    static constexpr int MAX_ORBS = 32;
    static constexpr int MAX_REPAIRS = 4;
    static constexpr int MAX_PARTICLES = 40;
    static constexpr int MAX_TITLE_BULLETS = 3;
    static constexpr int MAX_BOMBS = 10;
    static constexpr int RANKING_COUNT = 10;
    static constexpr int OBSTACLE_COUNT = 10;

    static constexpr float PLAYER_RADIUS = 9.0f;
    static constexpr float ENEMY_RADIUS = 7.0f;
    static constexpr float BULLET_RADIUS = 2.0f;

    static const Obstacle OBSTACLES[OBSTACLE_COUNT];

    Mode mode = MODE_TITLE;

    PRUZEA::Vector2 player;
    PRUZEA::Vector2 lastMoveDirection = PRUZEA::Vector2(1.0f, 0.0f);

    float bodyAngle = 0.0f;
    float turretAngle = 0.0f;
    float turretTurnSpeed = 2.8f;

    int hp = 5;
    int maxHp = 5;
    int level = 1;
    int experience = 0;
    int nextLevelExp = 8;
    uint32_t score = 0;
    uint32_t kills = 0;

    uint32_t rankings[RANKING_COUNT] = {};
    int lastRank = -1;
    bool rankingFromTitle = false;
    bool runFinalized = false;

    float moveSpeed = 72.0f;
    int bulletDamage = 1;
    float fireInterval = 0.48f;
    int multiShot = 1;
    int bulletPierce = 0;

    float fireTimer = 0.0f;
    float spawnTimer = 0.0f;
    float gameTime = 0.0f;
    float invincibleTimer = 0.0f;
    float hitFlashTimer = 0.0f;
    float bomberEventTimer = 18.0f;
    float bossTimer = 70.0f;
    float missionTimer = 22.0f;
    float missionNoticeTimer = 0.0f;
    float stateTimer = 0.0f;

    float cameraShakeTimer = 0.0f;
    float cameraShakeDuration = 0.0f;
    float cameraShakeStrength = 0.0f;

    PRUZEA::Vector2 destroyBlastPosition;
    float destroyBlastTimer = 0.0f;

    float titleTankX = 132.0f;
    float titleTankSpeed = 20.0f;
    float titleTurretAngle = 0.0f;
    float titleFireTimer = 0.25f;
    float titleAnimTime = 0.0f;

    bool specialTargetHeld = false;

    MissionType lastMissionType = MISSION_NONE;

    int selectedUpgrade = 0;
    Upgrade upgradeChoices[3] = {
        UPGRADE_POWER, UPGRADE_RAPID, UPGRADE_ENGINE
    };

    Enemy enemies[MAX_ENEMIES];
    Bullet bullets[MAX_BULLETS];
    EnemyBullet enemyBullets[MAX_ENEMY_BULLETS];
    ExpOrb orbs[MAX_ORBS];
    RepairItem repairs[MAX_REPAIRS];
    Particle particles[MAX_PARTICLES];
    TitleBullet titleBullets[MAX_TITLE_BULLETS];
    Bomb bombs[MAX_BOMBS];
    Bomber bomber{};

    Mission mission{MISSION_NONE, PRUZEA::Vector2(), PRUZEA::Vector2(), 0, 0.0f, 0.0f, 0, false};
    MissionTarget missionTarget{};
    EscortVehicle escortVehicle{};
    SupplyVehicle supplyVehicle{};

    void resetGame();
    void clearObjects();
    void finalizeRun(PRUZEA::Storage& storage);

    void updateTitle(float deltaSec);
    void updatePlayer(PRUZEA::Input& input, float deltaSec);
    void resolveTerrainCollision();
    void resolveActorTerrain(PRUZEA::Vector2& position, float radius);
    bool isActorPositionBlocked(const PRUZEA::Vector2& position, float radius) const;
    bool isBulletBlocked(const PRUZEA::Vector2& position) const;
    bool isWorldPointVisible(const PRUZEA::Vector2& position, float margin = 0.0f) const;

    void updateEnemies(PRUZEA::Audio& audio, float deltaSec);
    void updateEnemyShooting(float deltaSec);
    void updateBullets(PRUZEA::Audio& audio, float deltaSec);
    void updateEnemyBullets(PRUZEA::Audio& audio, float deltaSec);
    void updateOrbs(PRUZEA::Audio& audio, float deltaSec);
    void updateRepairs(PRUZEA::Audio& audio);
    void updateParticles(float deltaSec);
    void updateSpawning(float deltaSec);
    void updateAutoFire(PRUZEA::Audio& audio, float deltaSec);

    void spawnEnemy();
    void spawnBoss(PRUZEA::Audio& audio);
    void spawnEnemyBullet(const PRUZEA::Vector2& position, float angle, float speed, uint8_t damage);
    void spawnBullet(float angle);
    void spawnOrb(const PRUZEA::Vector2& position, uint8_t value);
    void spawnMissionReward(const PRUZEA::Vector2& position);
    void spawnRepair(const PRUZEA::Vector2& position);
    void spawnDeathParticles(const PRUZEA::Vector2& position,
                             PRUZEA::Graphics::Color color, int count);
    void triggerCameraShake(float strength, float duration);
    void triggerDestroyExplosion(const PRUZEA::Vector2& position);
    int findNearestEnemy() const;
    bool getSpecialTarget(PRUZEA::Vector2& outPosition) const;
    bool hasBoss() const;

    PRUZEA::Vector2 getEnemyMoveTarget(const Enemy& enemy) const;
    PRUZEA::Vector2 getBridgeCenter(int bridgeIndex) const;
    PRUZEA::Vector2 getMoveTargetAcrossRiver(const PRUZEA::Vector2& from,
                                             const PRUZEA::Vector2& destination) const;

    void updateBomber(PRUZEA::Audio& audio, float deltaSec);
    void startBomberRun(PRUZEA::Audio& audio);
    void dropBomb();
    void updateBombs(PRUZEA::Audio& audio, float deltaSec);
    void explodeBomb(Bomb& bomb, PRUZEA::Audio& audio);

    void updateMission(PRUZEA::Audio& audio, float deltaSec);
    void startMission(PRUZEA::Audio& audio);
    void failMission(PRUZEA::Audio& audio);
    void completeMission(const PRUZEA::Vector2& rewardPosition,
                         PRUZEA::Audio& audio, bool dropRepair);
    void clearMissionObjects();
    void startHoldBridgeMission();
    void startBreakthroughMission();
    void startDestroyMission();
    void startEscortMission();
    void startInterceptMission();
    void updateEscortVehicle(float deltaSec);
    void updateSupplyVehicle(float deltaSec);
    bool handleMissionBulletHit(Bullet& bullet, PRUZEA::Audio& audio);
    const char* getMissionName() const;

    void damagePlayer(int damage, PRUZEA::Audio& audio);

    void gainExperience(int amount, PRUZEA::Audio& audio);
    void beginLevelUp(PRUZEA::Audio& audio);
    void generateUpgradeChoices();
    void applyUpgrade(Upgrade upgrade, PRUZEA::Audio& audio);
    const char* getUpgradeName(Upgrade upgrade) const;
    const char* getUpgradeDescription(Upgrade upgrade) const;
    const char* getUpgradeValue(Upgrade upgrade) const;

    void loadRanking(PRUZEA::Storage& storage);
    void saveRanking(PRUZEA::Storage& storage);
    int insertRanking(uint32_t value);

    void drawWorld(PRUZEA::Graphics& graphics) const;
    void drawParticles(PRUZEA::Graphics& graphics) const;
    void drawTerrain(PRUZEA::Graphics& graphics) const;
    void drawObstacles(PRUZEA::Graphics& graphics) const;
    void drawTank(PRUZEA::Graphics& graphics) const;
    void drawEnemy(PRUZEA::Graphics& graphics, const Enemy& enemy) const;
    void drawMissionObjects(PRUZEA::Graphics& graphics) const;
    void drawBomber(PRUZEA::Graphics& graphics) const;
    void drawBombs(PRUZEA::Graphics& graphics) const;
    void drawHud(PRUZEA::Graphics& graphics) const;
    void drawRadar(PRUZEA::Graphics& graphics) const;
    void drawMission(PRUZEA::Graphics& graphics) const;
    void drawTitle(PRUZEA::Graphics& graphics) const;
    void drawReady(PRUZEA::Graphics& graphics) const;
    void drawLevelUp(PRUZEA::Graphics& graphics) const;
    void drawPause(PRUZEA::Graphics& graphics) const;
    void drawGameOver(PRUZEA::Graphics& graphics) const;
    void drawResult(PRUZEA::Graphics& graphics) const;
    void drawRanking(PRUZEA::Graphics& graphics) const;

    static int16_t sx(float value);
    static int16_t sy(float value);
};

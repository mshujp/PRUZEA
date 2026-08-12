#include "GroundFront.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

using namespace PRUZEA;

namespace {
constexpr int16_t SCREEN_W = 320;
constexpr int16_t SCREEN_H = 240;
constexpr int16_t FIELD_X = 60;
constexpr int16_t FIELD_W = 200;
constexpr int16_t FIELD_RIGHT = FIELD_X + FIELD_W - 1;
constexpr int16_t SAFE_UI_BOTTOM = 240;

constexpr float PLAYER_SPEED = 145.0f;
constexpr float PLAYER_FOCUS_SPEED = 78.0f;
constexpr float PLAYER_RADIUS = 4.0f;
constexpr int16_t PLAYER_START_HP = 4;
constexpr int16_t PLAYER_MAX_HP = 9;
constexpr uint8_t PLAYER_START_BOMBS = 2;
constexpr uint8_t PLAYER_MAX_BOMBS = 3;
constexpr uint64_t PLAYER_SHOT_INTERVAL = 105;
constexpr uint64_t PLAYER_FOCUS_SHOT_INTERVAL = 80;
constexpr uint64_t PLAYER_INVINCIBLE_MSEC = 1700;

constexpr uint64_t BOSS_INTERVAL_MSEC = 45000;
constexpr uint64_t ENEMY_SPAWN_BASE_MSEC = 1050;
constexpr uint64_t ENEMY_SPAWN_MIN_MSEC = 520;
constexpr uint32_t SCORE_SMALL_ENEMY = 120;
constexpr uint32_t SCORE_BOSS_BASE = 5000;
constexpr uint8_t BOSS_COUNT = 5;
constexpr float PI_F = 3.14159265358979323846f;

constexpr float PLAYER_FIRE_INTERVAL_RATE = 1.00f;
constexpr float ENEMY_SPEED_RATE = 1.00f;
constexpr float ENEMY_BULLET_SPEED_RATE = 1.00f;
constexpr float ENEMY_SPAWN_INTERVAL_RATE = 0.90f;
constexpr float ENEMY_FIRE_INTERVAL_RATE = 1.00f;
constexpr float BOSS_HP_RATE = 3.00f;
constexpr float BOSS_BULLET_SPEED_RATE = 0.80f;
constexpr float BOSS_FIRE_INTERVAL_RATE = 0.80f;

constexpr Graphics::Color COLOR_BG = Graphics::rgb565(5, 13, 18);
constexpr Graphics::Color COLOR_FIELD = Graphics::rgb565(17, 43, 45);
constexpr Graphics::Color COLOR_GRID = Graphics::rgb565(31, 71, 68);
constexpr Graphics::Color COLOR_PANEL = Graphics::rgb565(9, 24, 31);
constexpr Graphics::Color COLOR_PANEL_LINE = Graphics::rgb565(37, 122, 125);
constexpr Graphics::Color COLOR_TEXT = Graphics::rgb565(210, 240, 225);
constexpr Graphics::Color COLOR_ACCENT = Graphics::rgb565(62, 227, 188);
constexpr Graphics::Color COLOR_WARNING = Graphics::rgb565(255, 164, 55);
constexpr Graphics::Color COLOR_DANGER = Graphics::rgb565(255, 75, 74);
constexpr Graphics::Color COLOR_PLAYER = Graphics::rgb565(90, 225, 255);
constexpr Graphics::Color COLOR_PLAYER_SHOT = Graphics::rgb565(205, 255, 235);
constexpr Graphics::Color COLOR_ENEMY_SHOT = Graphics::rgb565(255, 107, 177);
constexpr Graphics::Color COLOR_BOSS = Graphics::rgb565(225, 80, 65);
constexpr Graphics::Color COLOR_GROUND = Graphics::rgb565(62, 91, 64);
constexpr Graphics::Color COLOR_OVERLAY = Graphics::rgb565(0, 0, 0);

static const Audio::ToneNote BGM_NOTES[] = {
    {Audio::ToneNote::E4, Audio::ToneNote::E}, {Audio::ToneNote::G4, Audio::ToneNote::E},
    {Audio::ToneNote::A4, Audio::ToneNote::E}, {Audio::ToneNote::G4, Audio::ToneNote::E},
    {Audio::ToneNote::D4, Audio::ToneNote::E}, {Audio::ToneNote::FS4, Audio::ToneNote::E},
    {Audio::ToneNote::A4, Audio::ToneNote::E}, {Audio::ToneNote::FS4, Audio::ToneNote::E},
    {Audio::ToneNote::C4, Audio::ToneNote::E}, {Audio::ToneNote::E4, Audio::ToneNote::E},
    {Audio::ToneNote::G4, Audio::ToneNote::E}, {Audio::ToneNote::B4, Audio::ToneNote::Q},
    {Audio::ToneNote::G4, Audio::ToneNote::E}, {Audio::ToneNote::E4, Audio::ToneNote::E},
    {Audio::ToneNote::D4, Audio::ToneNote::E}, {Audio::ToneNote::B3, Audio::ToneNote::Q}
};

static const Audio::Music BGM = {
    BGM_NOTES,
    231,
    static_cast<uint16_t>(sizeof(BGM_NOTES) / sizeof(BGM_NOTES[0])),
    0,
    0.28f
};

float clampf(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

float randUnit() {
    return static_cast<float>(rand() % 10000) / 10000.0f;
}

float normalizeAngle(float angle) {
    while (angle > PI_F) angle -= PI_F * 2.0f;
    while (angle < -PI_F) angle += PI_F * 2.0f;
    return angle;
}

const char* weaponLabel(uint8_t weapon) {
    switch (weapon) {
    case 1: return "SPRD";
    case 2: return "HOME";
    default: return "NORM";
    }
}

uint8_t cycleWeaponByTime(uint64_t now) {
    return static_cast<uint8_t>((now / 900u) % 3u);
}
}

GroundFront::GroundFront()
    : mode_(MODE_TITLE), score_(0), currentBossIndex_(0), defeatedEnemies_(0),
      escapedEnemies_(0), runStartMsec_(0), nextBossMsec_(0),
      lastEnemySpawnMsec_(0), resultStartMsec_(0), shakeUntilMsec_(0),
      flashUntilMsec_(0), pauseStartMsec_(0), backgroundSeed_(0x3412ABCDu),
      scoreSaved_(false), rankingLoaded_(false), weaponType_(WEAPON_NORMAL),
      weaponLevel_(1), bombCount_(PLAYER_START_BOMBS), homingFireRight_(false),
      shakeAmplitude_(0) {
    memset(&player_, 0, sizeof(player_));
    memset(playerBullets_, 0, sizeof(playerBullets_));
    memset(enemies_, 0, sizeof(enemies_));
    memset(enemyBullets_, 0, sizeof(enemyBullets_));
    memset(&boss_, 0, sizeof(boss_));
    memset(particles_, 0, sizeof(particles_));
    memset(items_, 0, sizeof(items_));
    memset(ranking_, 0, sizeof(ranking_));
}

const char* GroundFront::getId() const { return "groundfront"; }
const char* GroundFront::getName() const { return "GROUND FRONT"; }
const char* GroundFront::getMenuName() const { return "GROUND FRONT"; }
uint16_t GroundFront::getLogicalScreenWidth() const { return 320; }
uint16_t GroundFront::getLogicalScreenHeight() const { return 240; }
uint16_t GroundFront::getTargetScreenWidth() const { return 320; }
uint16_t GroundFront::getTargetScreenHeight() const { return 240; }

void GroundFront::onInit(Storage& storage) {
    loadRanking(storage);
    mode_ = MODE_TITLE;
    scoreSaved_ = false;
    dirty = true;
}

void GroundFront::resetRun() {
    memset(playerBullets_, 0, sizeof(playerBullets_));
    memset(enemies_, 0, sizeof(enemies_));
    memset(enemyBullets_, 0, sizeof(enemyBullets_));
    memset(&boss_, 0, sizeof(boss_));
    memset(particles_, 0, sizeof(particles_));
    memset(items_, 0, sizeof(items_));

    player_.x = 160.0f;
    player_.y = 205.0f;
    player_.hp = PLAYER_START_HP;
    player_.lastShotMsec = 0;
    player_.invincibleUntilMsec = 0;
    weaponType_ = WEAPON_NORMAL;
    weaponLevel_ = 1;
    bombCount_ = PLAYER_START_BOMBS;
    homingFireRight_ = false;
    shakeAmplitude_ = 0;

    score_ = 0;
    currentBossIndex_ = 0;
    defeatedEnemies_ = 0;
    escapedEnemies_ = 0;
    runStartMsec_ = Platform::getMsec();
    nextBossMsec_ = runStartMsec_ + BOSS_INTERVAL_MSEC;
    lastEnemySpawnMsec_ = runStartMsec_;
    resultStartMsec_ = 0;
    shakeUntilMsec_ = 0;
    flashUntilMsec_ = 0;
    pauseStartMsec_ = 0;
    scoreSaved_ = false;
}

void GroundFront::shiftGameTimers(uint64_t pausedMsec) {
    if (pausedMsec == 0) return;

    runStartMsec_ += pausedMsec;
    nextBossMsec_ += pausedMsec;
    lastEnemySpawnMsec_ += pausedMsec;
    if (resultStartMsec_ != 0) resultStartMsec_ += pausedMsec;
    if (shakeUntilMsec_ != 0) shakeUntilMsec_ += pausedMsec;
    if (flashUntilMsec_ != 0) flashUntilMsec_ += pausedMsec;

    if (player_.lastShotMsec != 0) player_.lastShotMsec += pausedMsec;
    if (player_.invincibleUntilMsec != 0) player_.invincibleUntilMsec += pausedMsec;

    for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemies_[i].active) continue;
        enemies_[i].bornMsec += pausedMsec;
        enemies_[i].lastShotMsec += pausedMsec;
    }

    if (boss_.active) {
        boss_.bornMsec += pausedMsec;
        boss_.lastShotMsec += pausedMsec;
        boss_.patternMsec += pausedMsec;
    }

    for (uint8_t i = 0; i < MAX_PARTICLES; ++i) {
        if (particles_[i].active) particles_[i].endMsec += pausedMsec;
    }
    for (uint8_t i = 0; i < MAX_ITEMS; ++i) {
        if (items_[i].active) items_[i].bornMsec += pausedMsec;
    }
}

void GroundFront::startRun(Audio& audio) {
    resetRun();
    mode_ = MODE_PLAYING;
    audio.playSE(&Audio::SE::NO_8, 0.75f);
    audio.playMusic(&BGM);
}

Game::GameState GroundFront::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) {
    (void)deltaSec;
    dirty = true;
    const uint64_t now = Platform::getMsec();

    switch (mode_) {
    case MODE_TITLE:
        if (input.justPressed(Input::START) || input.justPressed(Input::A)) {
            startRun(audio);
        } else if (input.justPressed(Input::X)) {
            mode_ = MODE_RANKING;
            audio.playSE(&Audio::SE::NO_1, 0.55f);
        }
        return GameState::RUNNING;

    case MODE_PLAYING:
        if (input.justPressed(Input::START)) {
            mode_ = MODE_PAUSED;
            pauseStartMsec_ = now;
            audio.stopMusic();
            audio.playSE(&Audio::SE::NO_1, 0.5f);
            return GameState::RUNNING;
        }
        updatePlaying(input, audio, storage, now);
        return GameState::RUNNING;

    case MODE_PAUSED:
        if (input.justPressed(Input::START)) {
            const uint64_t pausedMsec = now - pauseStartMsec_;
            shiftGameTimers(pausedMsec);
            pauseStartMsec_ = 0;
            mode_ = MODE_PLAYING;
            audio.playSE(&Audio::SE::NO_1, 0.5f);
            audio.playMusic(&BGM);
        }
        return GameState::RUNNING;

    case MODE_GAME_OVER:
    case MODE_STAGE_CLEAR:
        if (input.justPressed(Input::A) || input.justPressed(Input::START)) {
            mode_ = MODE_TITLE;
            audio.playSE(&Audio::SE::NO_1, 0.6f);
        } else if (input.justPressed(Input::X)) {
            mode_ = MODE_RANKING;
            audio.playSE(&Audio::SE::NO_1, 0.5f);
        }
        return GameState::RUNNING;

    case MODE_RANKING:
        if (input.justPressed(Input::B) || input.justPressed(Input::START) || input.justPressed(Input::A)) {
            mode_ = MODE_TITLE;
            audio.playSE(&Audio::SE::NO_2, 0.45f);
        }
        return GameState::RUNNING;
    }

    return GameState::RUNNING;
}

void GroundFront::updatePlaying(Input& input, Audio& audio, Storage& storage, uint64_t now) {
    if (input.justPressed(Input::B) && bombCount_ > 0) {
        useBomb(audio, now);
    }

    updatePlayer(input, audio, now);
    updatePlayerBullets(now);
    updateEnemies(audio, now);
    updateEnemyBullets(audio, now);
    updateBoss(audio, storage, now);
    updateParticles(now);
    updateItems(audio, now);

    if (!boss_.active && currentBossIndex_ < BOSS_COUNT && now >= nextBossMsec_) {
        spawnBoss(now, audio);
    }

    if (player_.hp <= 0) {
        finishRun(false, audio, storage, now);
    }
}

void GroundFront::updatePlayer(Input& input, Audio& audio, uint64_t now) {
    static uint64_t previousMsec = now;
    uint64_t deltaMsec = now - previousMsec;
    previousMsec = now;
    if (deltaMsec > 50) deltaMsec = 50;
    const float dt = static_cast<float>(deltaMsec) / 1000.0f;

    float dx = 0.0f;
    float dy = 0.0f;
    if (input.pressed(Input::LEFT)) dx -= 1.0f;
    if (input.pressed(Input::RIGHT)) dx += 1.0f;
    if (input.pressed(Input::UP)) dy -= 1.0f;
    if (input.pressed(Input::DOWN)) dy += 1.0f;

    if (dx != 0.0f && dy != 0.0f) {
        dx *= 0.7071f;
        dy *= 0.7071f;
    }

    player_.x = clampf(player_.x + dx * PLAYER_SPEED * dt, FIELD_X + 10.0f, FIELD_RIGHT - 10.0f);
    player_.y = clampf(player_.y + dy * PLAYER_SPEED * dt, 34.0f, 218.0f);

    const uint8_t level = weaponLevel_ < 1 ? 1 : (weaponLevel_ > 3 ? 3 : weaponLevel_);
    uint64_t interval = PLAYER_SHOT_INTERVAL;
    if (weaponType_ == WEAPON_NORMAL) {
        static const uint64_t NORMAL_INTERVAL[3] = {105, 90, 75};
        interval = NORMAL_INTERVAL[level - 1];
    } else if (weaponType_ == WEAPON_SPREAD) {
        // Keep the approximate number of bullets fired per second close to NORMAL.
        // L1: 2-way, L2/L3: 3-way.
        static const uint64_t SPREAD_INTERVAL[3] = {120, 135, 115};
        interval = SPREAD_INTERVAL[level - 1];
    } else {
        static const uint64_t HOMING_INTERVAL[3] = {380, 340, 300};
        interval = HOMING_INTERVAL[level - 1];
    }
    interval = static_cast<uint64_t>(static_cast<float>(interval) * PLAYER_FIRE_INTERVAL_RATE);

    if (input.pressed(Input::A) && now - player_.lastShotMsec >= interval) {
        firePlayerShot(audio);
        player_.lastShotMsec = now;
    }
}

void GroundFront::firePlayerShot(Audio& audio) {
    const uint8_t level = weaponLevel_ < 1 ? 1 : (weaponLevel_ > 3 ? 3 : weaponLevel_);

    switch (weaponType_) {
    case WEAPON_SPREAD:
        if (level == 1) {
            // Two-way shot with a deliberate gap directly ahead.
            addPlayerBullet(player_.x - 4.0f, player_.y - 8.0f, -36.0f, -255.0f, 1, PLAYER_BULLET_NORMAL);
            addPlayerBullet(player_.x + 4.0f, player_.y - 8.0f, 36.0f, -255.0f, 1, PLAYER_BULLET_NORMAL);
        } else if (level == 2) {
            addPlayerBullet(player_.x - 5.0f, player_.y - 8.0f, -58.0f, -252.0f, 1, PLAYER_BULLET_NORMAL);
            addPlayerBullet(player_.x, player_.y - 10.0f, 0.0f, -262.0f, 1, PLAYER_BULLET_NORMAL);
            addPlayerBullet(player_.x + 5.0f, player_.y - 8.0f, 58.0f, -252.0f, 1, PLAYER_BULLET_NORMAL);
        } else {
            addPlayerBullet(player_.x - 5.0f, player_.y - 7.0f, -92.0f, -242.0f, 1, PLAYER_BULLET_NORMAL);
            addPlayerBullet(player_.x, player_.y - 10.0f, 0.0f, -262.0f, 1, PLAYER_BULLET_NORMAL);
            addPlayerBullet(player_.x + 5.0f, player_.y - 7.0f, 92.0f, -242.0f, 1, PLAYER_BULLET_NORMAL);
        }
        break;

    case WEAPON_HOMING: {
        static const float HOMING_SPEED[3] = {185.0f, 195.0f, 205.0f};
        static const float HOMING_TURN[3] = {2.0f, 2.8f, 3.6f};
        const float speed = HOMING_SPEED[level - 1];
        const float turnRate = HOMING_TURN[level - 1];
        const float side = homingFireRight_ ? 7.0f : -7.0f;
        const float vx = homingFireRight_ ? 28.0f : -28.0f;
        const float vy = -Math::sqrt(speed * speed - vx * vx);
        addPlayerBullet(player_.x + side, player_.y - 5.0f, vx, vy, 2, PLAYER_BULLET_HOMING, turnRate);
        homingFireRight_ = !homingFireRight_;
        break;
    }

    default:
        addPlayerBullet(player_.x - 5.0f, player_.y - 9.0f, 0.0f, -260.0f, 1, PLAYER_BULLET_NORMAL);
        addPlayerBullet(player_.x + 5.0f, player_.y - 9.0f, 0.0f, -260.0f, 1, PLAYER_BULLET_NORMAL);
        break;
    }
    audio.playSE(&Audio::SE::NO_1, 0.16f);
}

void GroundFront::addPlayerBullet(float x, float y, float vx, float vy, uint8_t power, PlayerBulletType type, float turnRate) {
    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; ++i) {
        if (!playerBullets_[i].active) {
            playerBullets_[i].active = true;
            playerBullets_[i].x = x;
            playerBullets_[i].y = y;
            playerBullets_[i].vx = vx;
            playerBullets_[i].vy = vy;
            playerBullets_[i].power = power;
            playerBullets_[i].type = type;
            playerBullets_[i].turnRate = turnRate;
            playerBullets_[i].bornMsec = Platform::getMsec();
            return;
        }
    }
}

void GroundFront::updatePlayerBullets(uint64_t now) {
    static uint64_t previousMsec = now;
    uint64_t deltaMsec = now - previousMsec;
    previousMsec = now;
    if (deltaMsec > 50) deltaMsec = 50;
    const float dt = static_cast<float>(deltaMsec) / 1000.0f;

    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; ++i) {
        PlayerBullet& shot = playerBullets_[i];
        if (!shot.active) continue;

        if (shot.type == PLAYER_BULLET_HOMING && now - shot.bornMsec < 2000) {
            float targetX = 0.0f;
            float targetY = 0.0f;
            float bestDist2 = 1.0e12f;
            bool found = false;

            if (boss_.active) {
                const float dx = boss_.x - shot.x;
                const float dy = boss_.y - shot.y;
                bestDist2 = dx * dx + dy * dy;
                targetX = boss_.x;
                targetY = boss_.y;
                found = true;
            }
            for (uint8_t e = 0; e < MAX_ENEMIES; ++e) {
                const Enemy& enemy = enemies_[e];
                if (!enemy.active) continue;
                const float dx = enemy.x - shot.x;
                const float dy = enemy.y - shot.y;
                const float dist2 = dx * dx + dy * dy;
                if (!found || dist2 < bestDist2) {
                    bestDist2 = dist2;
                    targetX = enemy.x;
                    targetY = enemy.y;
                    found = true;
                }
            }
            if (found) {
                const float speed = Math::length(shot.vx, shot.vy);
                float currentAngle = Math::atan2(shot.vy, shot.vx);
                const float targetAngle = Math::atan2(targetY - shot.y, targetX - shot.x);
                const float maxTurn = shot.turnRate * dt;
                const float turn = clampf(normalizeAngle(targetAngle - currentAngle), -maxTurn, maxTurn);
                currentAngle += turn;
                shot.vx = Math::cos(currentAngle) * speed;
                shot.vy = Math::sin(currentAngle) * speed;
            }
        }

        shot.x += shot.vx * dt;
        shot.y += shot.vy * dt;
        if (shot.y < -10.0f || shot.x < FIELD_X - 16.0f || shot.x > FIELD_RIGHT + 16.0f) {
            shot.active = false;
            continue;
        }

        bool consumed = false;
        for (uint8_t e = 0; e < MAX_ENEMIES && !consumed; ++e) {
            Enemy& enemy = enemies_[e];
            if (!enemy.active) continue;
            const float enemyRadius = enemy.type == ENEMY_STAGE_SPECIAL ? 9.0f : 8.0f;
            if (hitCircle(shot.x, shot.y, 2.0f, enemy.x, enemy.y, enemyRadius)) {
                enemy.hp -= shot.power;
                shot.active = false;
                consumed = true;
                if (enemy.hp <= 0) {
                    const bool guaranteedWeapon = enemy.type == ENEMY_STAGE_SPECIAL;
                    const float defeatedX = enemy.x;
                    const float defeatedY = enemy.y;
                    enemy.active = false;
                    ++defeatedEnemies_;
                    score_ += SCORE_SMALL_ENEMY + static_cast<uint32_t>(enemy.type) * 40;
                    addExplosion(defeatedX, defeatedY, enemy.type == ENEMY_STAGE_SPECIAL ? 10 : 7, now);
                    maybeDropItem(defeatedX, defeatedY, guaranteedWeapon, now);
                }
            }
        }

        if (!consumed && boss_.active && hitCircle(shot.x, shot.y, 2.0f, boss_.x, boss_.y, 19.0f)) {
            boss_.hp -= shot.power;
            shot.active = false;
            flashUntilMsec_ = now + 35;
        }
    }
}

void GroundFront::spawnEnemy(uint64_t now) {
    for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
        if (enemies_[i].active) continue;
        Enemy& enemy = enemies_[i];
        enemy.active = true;
        const bool spawnSpecial = (currentBossIndex_ < BOSS_COUNT) && ((rand() % 100) < 15);
        enemy.type = spawnSpecial ? ENEMY_STAGE_SPECIAL : static_cast<EnemyType>(rand() % 4);
        enemy.x = static_cast<float>(FIELD_X + 14 + rand() % (FIELD_W - 28));
        enemy.y = -12.0f;
        enemy.vx = 0.0f;
        enemy.vy = 28.0f + static_cast<float>(currentBossIndex_) * 4.0f;
        enemy.hp = 2 + static_cast<int16_t>(currentBossIndex_ / 2);
        enemy.bornMsec = now;
        enemy.lastShotMsec = now + static_cast<uint64_t>(rand() % 500);
        enemy.phase = static_cast<uint8_t>(rand() % 8);

        if (enemy.type == ENEMY_BIRD) {
            enemy.hp = 1;
            enemy.vy += 15.0f;
        } else if (enemy.type == ENEMY_BALLOON) {
            enemy.hp += 1;
            enemy.vy -= 8.0f;
        } else if (enemy.type == ENEMY_TANK) {
            enemy.hp += 2;
            enemy.vy = 22.0f;
        } else if (enemy.type == ENEMY_STAGE_SPECIAL) {
            switch (currentBossIndex_) {
            case 0: // Scout
                enemy.hp = 1;
                enemy.vy = 74.0f;
                enemy.vx = (enemy.x < 160.0f) ? 46.0f : -46.0f;
                break;
            case 1: // Mortar
                enemy.hp = 3;
                enemy.vy = 30.0f;
                break;
            case 2: // Shielder
                enemy.hp = 5;
                enemy.vy = 24.0f;
                break;
            case 3: // Jammer
                enemy.hp = 3;
                enemy.vy = 38.0f;
                break;
            default: // Assault
                enemy.hp = 4;
                enemy.vy = 55.0f;
                break;
            }
        }
        enemy.vx *= ENEMY_SPEED_RATE;
        enemy.vy *= ENEMY_SPEED_RATE;
        return;
    }
}

void GroundFront::updateEnemies(Audio& audio, uint64_t now) {
    static uint64_t previousMsec = now;
    uint64_t deltaMsec = now - previousMsec;
    previousMsec = now;
    if (deltaMsec > 50) deltaMsec = 50;
    const float dt = static_cast<float>(deltaMsec) / 1000.0f;

    const uint64_t difficultyReduction = static_cast<uint64_t>(currentBossIndex_) * 75;
    uint64_t spawnInterval = ENEMY_SPAWN_BASE_MSEC > difficultyReduction
        ? ENEMY_SPAWN_BASE_MSEC - difficultyReduction : ENEMY_SPAWN_MIN_MSEC;
    if (spawnInterval < ENEMY_SPAWN_MIN_MSEC) spawnInterval = ENEMY_SPAWN_MIN_MSEC;
    spawnInterval = static_cast<uint64_t>(static_cast<float>(spawnInterval) * ENEMY_SPAWN_INTERVAL_RATE);

    if (!boss_.active && now - lastEnemySpawnMsec_ >= spawnInterval) {
        spawnEnemy(now);
        lastEnemySpawnMsec_ = now;
    }

    for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& enemy = enemies_[i];
        if (!enemy.active) continue;
        const float age = static_cast<float>(now - enemy.bornMsec) / 1000.0f;

        if (enemy.type == ENEMY_BIRD) {
            enemy.x += Math::sin(age * 4.0f + enemy.phase) * 38.0f * dt;
        } else if (enemy.type == ENEMY_BALLOON) {
            enemy.x += Math::sin(age * 2.0f + enemy.phase) * 20.0f * dt;
        } else if (enemy.type == ENEMY_DRONE) {
            enemy.x += Math::cos(age * 3.1f + enemy.phase) * 52.0f * dt;
        } else if (enemy.type == ENEMY_STAGE_SPECIAL) {
            switch (currentBossIndex_) {
            case 0:
                enemy.x += enemy.vx * dt;
                break;
            case 1:
                if (enemy.y > 60.0f) enemy.vy = 10.0f;
                break;
            case 2:
                enemy.x += Math::sin(age * 1.6f + enemy.phase) * 14.0f * dt;
                break;
            case 3:
                enemy.x += Math::sin(age * 5.5f + enemy.phase) * 65.0f * dt;
                break;
            default: {
                const float dirX = player_.x - enemy.x;
                const float dirY = player_.y - enemy.y;
                const float len = Math::length(dirX, dirY);
                if (len > 0.001f && age < 1.2f) {
                    enemy.x += (dirX / len) * 46.0f * dt;
                    enemy.y += (dirY / len) * 24.0f * dt;
                }
                break;
            }
            }
        }
        enemy.y += enemy.vy * dt;
        enemy.x = clampf(enemy.x, FIELD_X + 9.0f, FIELD_RIGHT - 9.0f);

        uint64_t baseShotInterval = 1650 - static_cast<uint64_t>(currentBossIndex_) * 110;
        if (enemy.type == ENEMY_STAGE_SPECIAL && currentBossIndex_ > 0) baseShotInterval -= 180;
        const uint64_t shotInterval = static_cast<uint64_t>(static_cast<float>(baseShotInterval) * ENEMY_FIRE_INTERVAL_RATE);
        if (enemy.y > 20.0f && enemy.y < 178.0f && now - enemy.lastShotMsec >= shotInterval) {
            if (enemy.type == ENEMY_TANK) {
                fireEnemyFan(enemy.x, enemy.y + 7.0f, 3, 60.0f * ENEMY_BULLET_SPEED_RATE, 0.35f, 2);
            } else if (enemy.type == ENEMY_STAGE_SPECIAL) {
                switch (currentBossIndex_) {
                case 0:
                    // Stage 1 Scout is movement pressure only; it does not shoot.
                    break;
                case 1:
                    fireEnemyAimed(enemy.x, enemy.y + 6.0f, 48.0f * ENEMY_BULLET_SPEED_RATE, 3);
                    break;
                case 2:
                    fireEnemyFan(enemy.x, enemy.y + 4.0f, 2, 58.0f * ENEMY_BULLET_SPEED_RATE, 0.22f, 2);
                    break;
                case 3:
                    fireEnemyRing(enemy.x, enemy.y + 4.0f, 4, 56.0f * ENEMY_BULLET_SPEED_RATE, enemy.phase * 0.35f, 2);
                    break;
                default:
                    fireEnemyFan(enemy.x, enemy.y + 5.0f, 3, 74.0f * ENEMY_BULLET_SPEED_RATE, 0.16f, 2);
                    break;
                }
            } else {
                fireEnemyAimed(enemy.x, enemy.y + 6.0f, (56.0f + currentBossIndex_ * 5.0f) * ENEMY_BULLET_SPEED_RATE, 2);
            }
            enemy.lastShotMsec = now;
        }

        if (enemy.y > 235.0f) {
            enemy.active = false;
            ++escapedEnemies_;
        } else if (hitCircle(player_.x, player_.y, PLAYER_RADIUS, enemy.x, enemy.y, enemy.type == ENEMY_STAGE_SPECIAL ? 9.0f : 8.0f)) {
            const float hitX = enemy.x;
            const float hitY = enemy.y;
            enemy.active = false;
            addExplosion(hitX, hitY, 5, now);
            damagePlayer(audio, now);
        }
    }
}

void GroundFront::addEnemyBullet(float x, float y, float vx, float vy, uint8_t radius) {
    for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        if (!enemyBullets_[i].active) {
            enemyBullets_[i].active = true;
            enemyBullets_[i].x = x;
            enemyBullets_[i].y = y;
            enemyBullets_[i].vx = vx;
            enemyBullets_[i].vy = vy;
            enemyBullets_[i].radius = radius;
            return;
        }
    }
}

void GroundFront::fireEnemyAimed(float x, float y, float speed, uint8_t radius) {
    const float dx = player_.x - x;
    const float dy = player_.y - y;
    const float length = Math::length(dx, dy);
    if (length < 0.001f) return;
    addEnemyBullet(x, y, dx / length * speed, dy / length * speed, radius);
}

void GroundFront::fireEnemyFan(float x, float y, uint8_t count, float speed, float spread, uint8_t radius) {
    const float base = Math::atan2(player_.y - y, player_.x - x);
    const float center = static_cast<float>(count - 1) * 0.5f;
    for (uint8_t i = 0; i < count; ++i) {
        const float angle = base + (static_cast<float>(i) - center) * spread;
        addEnemyBullet(x, y, Math::cos(angle) * speed, Math::sin(angle) * speed, radius);
    }
}

void GroundFront::fireEnemyRing(float x, float y, uint8_t count, float speed, float angleOffset, uint8_t radius) {
    for (uint8_t i = 0; i < count; ++i) {
        const float angle = angleOffset + (2.0f * PI_F * static_cast<float>(i) / static_cast<float>(count));
        addEnemyBullet(x, y, Math::cos(angle) * speed, Math::sin(angle) * speed, radius);
    }
}

void GroundFront::updateEnemyBullets(Audio& audio, uint64_t now) {
    static uint64_t previousMsec = now;
    uint64_t deltaMsec = now - previousMsec;
    previousMsec = now;
    if (deltaMsec > 50) deltaMsec = 50;
    const float dt = static_cast<float>(deltaMsec) / 1000.0f;

    for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        EnemyBullet& shot = enemyBullets_[i];
        if (!shot.active) continue;
        shot.x += shot.vx * dt;
        shot.y += shot.vy * dt;
        if (shot.x < FIELD_X - 8 || shot.x > FIELD_RIGHT + 8 || shot.y < -10 || shot.y > 242) {
            shot.active = false;
            continue;
        }
        if (hitCircle(player_.x, player_.y, PLAYER_RADIUS, shot.x, shot.y, static_cast<float>(shot.radius))) {
            shot.active = false;
            damagePlayer(audio, now);
        }
    }
}

void GroundFront::spawnBoss(uint64_t now, Audio& audio) {
    memset(&boss_, 0, sizeof(boss_));
    boss_.active = true;
    boss_.type = currentBossIndex_;
    boss_.x = 160.0f;
    boss_.y = -28.0f;
    boss_.targetY = 53.0f;
    boss_.maxHp = static_cast<int16_t>(static_cast<float>(50 + currentBossIndex_ * 28) * BOSS_HP_RATE);
    if (boss_.maxHp < 1) boss_.maxHp = 1;
    boss_.hp = boss_.maxHp;
    boss_.bornMsec = now;
    boss_.lastShotMsec = now;
    boss_.patternMsec = now;
    boss_.phase = 0;

    for (uint8_t i = 0; i < MAX_ENEMIES; ++i) enemies_[i].active = false;
    for (uint8_t i = 0; i < MAX_ITEMS; ++i) items_[i].active = false;
    audio.playSE(&Audio::SE::NO_10, 0.8f);
    shakeUntilMsec_ = now + 650;
    shakeAmplitude_ = 4;
}

void GroundFront::updateBoss(Audio& audio, Storage& storage, uint64_t now) {
    if (!boss_.active) return;

    static uint64_t previousMsec = now;
    uint64_t deltaMsec = now - previousMsec;
    previousMsec = now;
    if (deltaMsec > 50) deltaMsec = 50;
    const float dt = static_cast<float>(deltaMsec) / 1000.0f;
    const float age = static_cast<float>(now - boss_.bornMsec) / 1000.0f;

    if (boss_.y < boss_.targetY) {
        boss_.y += 45.0f * dt;
        if (boss_.y > boss_.targetY) boss_.y = boss_.targetY;
        return;
    }

    const float amplitude = 42.0f + static_cast<float>(boss_.type) * 6.0f;
    boss_.x = 160.0f + Math::sin(age * (0.75f + boss_.type * 0.12f)) * amplitude;

    uint64_t interval = 1150;
    if (boss_.type == 1) interval = 980;
    if (boss_.type == 2) interval = 860;
    if (boss_.type == 3) interval = 760;
    if (boss_.type == 4) interval = 660;
    interval = static_cast<uint64_t>(static_cast<float>(interval) * BOSS_FIRE_INTERVAL_RATE);

    if (now - boss_.lastShotMsec >= interval) {
        switch (boss_.type) {
        case 0:
            fireEnemyFan(boss_.x, boss_.y + 15.0f, 3, 66.0f * BOSS_BULLET_SPEED_RATE, 0.30f, 2);
            break;

        case 1: {
            const float side = (boss_.phase & 1u) ? 15.0f : -15.0f;
            fireEnemyFan(boss_.x + side, boss_.y + 10.0f, 2, 68.0f * BOSS_BULLET_SPEED_RATE, 0.22f, 2);
            fireEnemyFan(boss_.x - side, boss_.y + 10.0f, 2, 61.0f * BOSS_BULLET_SPEED_RATE, -0.22f, 2);
            ++boss_.phase;
            break;
        }

        case 2:
            fireEnemyRing(boss_.x, boss_.y + 8.0f, 5, 56.0f * BOSS_BULLET_SPEED_RATE, boss_.phase * 0.22f, 2);
            if ((boss_.phase & 1u) == 0) {
                fireEnemyAimed(boss_.x, boss_.y + 16.0f, 78.0f * BOSS_BULLET_SPEED_RATE, 2);
            }
            ++boss_.phase;
            break;

        case 3: {
            const uint8_t safeLane = static_cast<uint8_t>(boss_.phase % 7u);
            for (uint8_t lane = 0; lane < 7; ++lane) {
                if (lane == safeLane) continue;
                const float x = FIELD_X + 16.0f + lane * 28.0f;
                addEnemyBullet(x, boss_.y + 10.0f, 0.0f, 64.0f * BOSS_BULLET_SPEED_RATE, 2);
            }
            if ((boss_.phase & 1u) != 0) {
                fireEnemyFan(boss_.x, boss_.y + 15.0f, 3, 70.0f * BOSS_BULLET_SPEED_RATE, 0.18f, 2);
            }
            ++boss_.phase;
            break;
        }

        default:
            if ((boss_.phase & 1u) == 0) {
                fireEnemyRing(boss_.x, boss_.y + 10.0f, 10, 64.0f * BOSS_BULLET_SPEED_RATE, boss_.phase * 0.19f, 2);
            } else {
                fireEnemyFan(boss_.x, boss_.y + 15.0f, 5, 82.0f * BOSS_BULLET_SPEED_RATE, 0.16f, 2);
                fireEnemyAimed(boss_.x - 20.0f, boss_.y + 8.0f, 88.0f * BOSS_BULLET_SPEED_RATE, 2);
                fireEnemyAimed(boss_.x + 20.0f, boss_.y + 8.0f, 88.0f * BOSS_BULLET_SPEED_RATE, 2);
            }
            ++boss_.phase;
            break;
        }
        boss_.lastShotMsec = now;
    }

    if (boss_.hp <= 0) {
        const float defeatedX = boss_.x;
        const float defeatedY = boss_.y;
        boss_.active = false;
        score_ += SCORE_BOSS_BASE * static_cast<uint32_t>(currentBossIndex_ + 1);
        addExplosion(defeatedX, defeatedY, 24, now);
        shakeUntilMsec_ = now + 850;
        shakeAmplitude_ = 6;
        flashUntilMsec_ = now + 180;
        audio.playSE(&Audio::SE::NO_4, 0.9f);
        ++currentBossIndex_;
        for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; ++i) enemyBullets_[i].active = false;

        if (currentBossIndex_ >= BOSS_COUNT) {
            finishRun(true, audio, storage, now);
        } else {
            nextBossMsec_ = now + BOSS_INTERVAL_MSEC;
        }
    }
}

void GroundFront::updateParticles(uint64_t now) {
    static uint64_t previousMsec = now;
    uint64_t deltaMsec = now - previousMsec;
    previousMsec = now;
    if (deltaMsec > 50) deltaMsec = 50;
    const float dt = static_cast<float>(deltaMsec) / 1000.0f;

    for (uint8_t i = 0; i < MAX_PARTICLES; ++i) {
        Particle& p = particles_[i];
        if (!p.active) continue;
        if (now >= p.endMsec) {
            p.active = false;
            continue;
        }
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 18.0f * dt;
    }
}

void GroundFront::spawnItem(float x, float y, ItemType type, uint64_t now) {
    for (uint8_t i = 0; i < MAX_ITEMS; ++i) {
        if (items_[i].active) continue;
        items_[i].active = true;
        items_[i].type = type;
        items_[i].x = x;
        items_[i].y = y;
        items_[i].vy = 42.0f;
        items_[i].bornMsec = now;
        return;
    }
}

void GroundFront::maybeDropItem(float x, float y, bool guaranteedWeapon, uint64_t now) {
    if (guaranteedWeapon) {
        spawnItem(x, y, ITEM_WEAPON, now);
        return;
    }

    const int roll = rand() % 100;
    if (roll < 4) {
        spawnItem(x, y, ITEM_WEAPON, now);
    } else if (roll < 7) {
        spawnItem(x, y, ITEM_BOMB, now);
    } else if (roll < 9) {
        spawnItem(x, y, ITEM_LIFE, now);
    }
}

void GroundFront::updateItems(Audio& audio, uint64_t now) {
    static uint64_t previousMsec = now;
    uint64_t deltaMsec = now - previousMsec;
    previousMsec = now;
    if (deltaMsec > 50) deltaMsec = 50;
    const float dt = static_cast<float>(deltaMsec) / 1000.0f;

    for (uint8_t i = 0; i < MAX_ITEMS; ++i) {
        Item& item = items_[i];
        if (!item.active) continue;
        item.y += item.vy * dt;
        if (item.y > 240.0f) {
            item.active = false;
            continue;
        }

        if (hitCircle(player_.x, player_.y, PLAYER_RADIUS + 1.0f, item.x, item.y, 8.0f)) {
            if (item.type == ITEM_WEAPON) {
                const WeaponType collected = static_cast<WeaponType>(cycleWeaponByTime(now));
                if (collected == weaponType_) {
                    if (weaponLevel_ < 3) ++weaponLevel_;
                    else score_ += 500;
                } else {
                    weaponType_ = collected;
                    weaponLevel_ = 1;
                }
            } else if (item.type == ITEM_BOMB) {
                if (bombCount_ < PLAYER_MAX_BOMBS) ++bombCount_;
            } else if (item.type == ITEM_LIFE) {
                if (player_.hp < PLAYER_MAX_HP) ++player_.hp;
            }
            item.active = false;
            audio.playSE(&Audio::SE::NO_3, 0.65f);
        }
    }
}

void GroundFront::useBomb(Audio& audio, uint64_t now) {
    if (bombCount_ == 0) return;
    --bombCount_;
    player_.invincibleUntilMsec = now + 1100;
    shakeUntilMsec_ = now + 650;
    shakeAmplitude_ = 14;
    flashUntilMsec_ = now + 130;
    addExplosion(player_.x, player_.y - 20.0f, 16, now);

    for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        if (!enemyBullets_[i].active) continue;
        addExplosion(enemyBullets_[i].x, enemyBullets_[i].y, 1, now);
        enemyBullets_[i].active = false;
    }
    for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& enemy = enemies_[i];
        if (!enemy.active) continue;
        enemy.hp -= 3;
        addExplosion(enemy.x, enemy.y, 4, now);
        if (enemy.hp <= 0) {
            const bool guaranteedWeapon = enemy.type == ENEMY_STAGE_SPECIAL;
            const float defeatedX = enemy.x;
            const float defeatedY = enemy.y;
            enemy.active = false;
            ++defeatedEnemies_;
            score_ += SCORE_SMALL_ENEMY + static_cast<uint32_t>(enemy.type) * 40;
            maybeDropItem(defeatedX, defeatedY, guaranteedWeapon, now);
        }
    }
    if (boss_.active) {
        boss_.hp -= 8;
        addExplosion(boss_.x, boss_.y, 8, now);
    }
    audio.playSE(&Audio::SE::NO_11, 0.85f);
}

void GroundFront::addExplosion(float x, float y, uint8_t count, uint64_t now) {
    for (uint8_t c = 0; c < count; ++c) {
        for (uint8_t i = 0; i < MAX_PARTICLES; ++i) {
            if (particles_[i].active) continue;
            const float angle = randUnit() * PI_F * 2.0f;
            const float speed = 28.0f + randUnit() * 72.0f;
            particles_[i].active = true;
            particles_[i].x = x;
            particles_[i].y = y;
            particles_[i].vx = Math::cos(angle) * speed;
            particles_[i].vy = Math::sin(angle) * speed;
            particles_[i].endMsec = now + 350 + static_cast<uint64_t>(rand() % 500);
            break;
        }
    }
}

bool GroundFront::hitCircle(float ax, float ay, float ar, float bx, float by, float br) const {
    const float dx = ax - bx;
    const float dy = ay - by;
    const float radius = ar + br;
    return dx * dx + dy * dy <= radius * radius;
}

void GroundFront::damagePlayer(Audio& audio, uint64_t now) {
    if (now < player_.invincibleUntilMsec) return;
    --player_.hp;
    player_.invincibleUntilMsec = now + PLAYER_INVINCIBLE_MSEC;
    shakeUntilMsec_ = now + 280;
    shakeAmplitude_ = 4;
    flashUntilMsec_ = now + 90;
    addExplosion(player_.x, player_.y, 10, now);
    audio.playSE(&Audio::SE::NO_2, 0.75f);
}

void GroundFront::finishRun(bool cleared, Audio& audio, Storage& storage, uint64_t now) {
    audio.stopMusic();
    mode_ = cleared ? MODE_STAGE_CLEAR : MODE_GAME_OVER;
    resultStartMsec_ = now;
    registerScore();
    saveRanking(storage);
    scoreSaved_ = true;
    audio.playSE(cleared ? &Audio::SE::NO_11 : &Audio::SE::NO_12, 0.8f);
}

void GroundFront::loadRanking(Storage& storage) {
    rankingLoaded_ = true;
    if (!storage.isAvailable()) return;

    SaveData sd;
    sd.load(storage, getId(), savePath_);
    for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "RANK_%u", i);
        ranking_[i] = sd.getInt32(buffer, 0);
    }
}

void GroundFront::registerScore() {
    uint32_t value = score_;
    for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
        if (value > ranking_[i]) {
            const uint32_t displaced = ranking_[i];
            ranking_[i] = value;
            value = displaced;
        }
    }
}

void GroundFront::saveRanking(Storage& storage) {
    if (!storage.isAvailable()) return;

    SaveData sd;
    if (!sd.load(storage, getId(), savePath_)) return;

    for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "RANK_%u", i);
        sd.setInt32(buffer, ranking_[i]);
    }
    sd.save(storage, getId(), savePath_);
}

bool GroundFront::onDraw(Graphics& graphics, bool requestFullRedraw) {
    if (!requestFullRedraw && !dirty) {
        return false;
    }

    const uint64_t now = Platform::getMsec();
    int16_t shakeX = 0;
    int16_t shakeY = 0;
    if (now < shakeUntilMsec_) {
        const int amplitudeX = shakeAmplitude_ > 0 ? shakeAmplitude_ : 4;
        const int amplitudeY = amplitudeX >= 12 ? 9 : amplitudeX;
        shakeX = static_cast<int16_t>(rand() % (amplitudeX * 2 + 1) - amplitudeX);
        shakeY = static_cast<int16_t>(rand() % (amplitudeY * 2 + 1) - amplitudeY);
    } else {
        shakeAmplitude_ = 0;
    }
    graphics.setViewport(shakeX, shakeY);
    drawBackground(graphics, now);

    if (mode_ == MODE_TITLE) {
        drawTitle(graphics, now);
    } else if (mode_ == MODE_RANKING) {
        drawRanking(graphics, now);
    } else {
        graphics.setClipRect(FIELD_X, 0, FIELD_W, SAFE_UI_BOTTOM);
        drawPlayfield(graphics, now);
        graphics.resetClipRect();
        drawSidePanels(graphics, now);
        if (mode_ == MODE_PAUSED) drawPause(graphics);
        if (mode_ == MODE_GAME_OVER || mode_ == MODE_STAGE_CLEAR) drawResult(graphics, now);
    }

    if (now < flashUntilMsec_) {
        graphics.drawRect(FIELD_X, 0, FIELD_W, 240, COLOR_WARNING);
        graphics.drawRect(FIELD_X + 1, 1, FIELD_W - 2, 238, COLOR_WARNING);
    }
    // Viewport is applied later by Graphics::push().
    // Keep the shake offset until push() completes; the next frame updates
    // or clears it with setViewport(0, 0) at the start of this draw path.
    dirty = false;
    return true;
}

void GroundFront::drawBackground(Graphics& graphics, uint64_t now) {
    graphics.fillScreen(COLOR_BG);
    graphics.fillRect(FIELD_X, 0, FIELD_W, SCREEN_H, COLOR_FIELD);

    const int16_t scroll = static_cast<int16_t>((now / 45) % 24);
    for (int16_t y = -24 + scroll; y < SCREEN_H; y += 24) {
        graphics.drawLine(FIELD_X, y, FIELD_RIGHT, y, COLOR_GRID);
    }
    for (int16_t x = FIELD_X + 20; x < FIELD_RIGHT; x += 20) {
        graphics.drawLine(x, 0, x, SCREEN_H - 1, COLOR_GRID);
    }

    for (int16_t y = 8; y < SCREEN_H; y += 36) {
        const int16_t offset = static_cast<int16_t>((y * 7 + now / 70) % 31);
        graphics.fillRect(FIELD_X + 4 + offset, y, 8, 3, COLOR_GROUND);
        graphics.fillRect(FIELD_RIGHT - 20 - offset / 2, y + 12, 14, 3, COLOR_GROUND);
    }

    graphics.fillRect(0, 0, FIELD_X, SCREEN_H, COLOR_PANEL);
    graphics.fillRect(FIELD_RIGHT + 1, 0, SCREEN_W - FIELD_RIGHT - 1, SCREEN_H, COLOR_PANEL);
    graphics.drawLine(FIELD_X - 1, 0, FIELD_X - 1, SCREEN_H - 1, COLOR_PANEL_LINE);
    graphics.drawLine(FIELD_RIGHT + 1, 0, FIELD_RIGHT + 1, SCREEN_H - 1, COLOR_PANEL_LINE);
}

void GroundFront::drawTitle(Graphics& graphics, uint64_t now) {
    graphics.drawString("GROUND", 160, 44, COLOR_TEXT, Graphics::SIZE_32B,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    graphics.drawString("FRONT", 160, 79, COLOR_ACCENT, Graphics::SIZE_32B,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    graphics.drawString("VERTICAL BARRAGE", 160, 111, COLOR_TEXT, Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    graphics.drawRoundRect(82, 132, 156, 52, 8, COLOR_PANEL_LINE);
    graphics.drawString("A : START", 160, 148, COLOR_TEXT, Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    graphics.drawString("X : RANK", 160, 167, COLOR_ACCENT, Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    if (((now / 500) % 2) == 0) {
        graphics.drawString("PRESS START", 160, 205, COLOR_WARNING, Graphics::SIZE_22B,
            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }
}

void GroundFront::drawSidePanels(Graphics& graphics, uint64_t now) {
    char text[32];
    graphics.drawString("SCORE", 5, 10, COLOR_ACCENT, Graphics::SIZE_13);
    std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(score_));
    graphics.drawString(text, 5, 27, COLOR_TEXT, Graphics::SIZE_13);

    graphics.drawString("LIFE", 5, 51, COLOR_ACCENT, Graphics::SIZE_13);
    constexpr int16_t LIFE_COLS = 3;
    constexpr int16_t LIFE_STEP_X = 16;
    constexpr int16_t LIFE_STEP_Y = 15;
    for (int16_t i = 0; i < PLAYER_MAX_HP; ++i) {
        const int16_t col = i % LIFE_COLS;
        const int16_t row = i / LIFE_COLS;
        const int16_t cx = 12 + col * LIFE_STEP_X;
        const int16_t cy = 72 + row * LIFE_STEP_Y;
        const Graphics::Color color = i < player_.hp ? COLOR_PLAYER : COLOR_GRID;
        graphics.fillTriangle(cx, cy - 5, cx - 5, cy + 4, cx + 5, cy + 4, color);
    }

    graphics.drawString("BOMB", 5, 125, COLOR_ACCENT, Graphics::SIZE_13);
    for (uint8_t i = 0; i < PLAYER_MAX_BOMBS; ++i) {
        const Graphics::Color color = i < bombCount_ ? COLOR_WARNING : COLOR_GRID;
        graphics.fillCircle(12 + i * 16, 143, 5, color);
        graphics.drawCircle(12 + i * 16, 143, 7, COLOR_PANEL_LINE);
    }

    graphics.drawString("ARMS", 5, 163, COLOR_ACCENT, Graphics::SIZE_13);
    char weaponText[16];
    std::snprintf(weaponText, sizeof(weaponText), "%s L%u", weaponLabel(weaponType_), static_cast<unsigned int>(weaponLevel_));
    graphics.drawString(weaponText, 5, 180, COLOR_TEXT, Graphics::SIZE_13);
    if (mode_ == MODE_PLAYING) {
        graphics.drawString("B:BOMB", 5, 198, COLOR_WARNING, Graphics::SIZE_13);
    }

    graphics.drawString("BOSS", 266, 10, COLOR_ACCENT, Graphics::SIZE_13);
    std::snprintf(text, sizeof(text), "%u/5", static_cast<unsigned int>(currentBossIndex_ + (boss_.active ? 1 : 0)));
    graphics.drawString(text, 266, 27, COLOR_TEXT, Graphics::SIZE_13);

    if (!boss_.active && currentBossIndex_ < BOSS_COUNT) {
        const uint64_t displayNow = (mode_ == MODE_PAUSED && pauseStartMsec_ != 0) ? pauseStartMsec_ : now;
        const uint64_t remain = displayNow < nextBossMsec_ ? nextBossMsec_ - displayNow : 0;
        std::snprintf(text, sizeof(text), "%02lu", static_cast<unsigned long>((remain + 999) / 1000));
        graphics.drawString("NEXT", 266, 57, COLOR_ACCENT, Graphics::SIZE_13);
        graphics.drawString(text, 266, 74, remain < 10000 ? COLOR_WARNING : COLOR_TEXT, Graphics::SIZE_22B);
    }

    graphics.drawString("DOWN", 266, 117, COLOR_ACCENT, Graphics::SIZE_13);
    std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(defeatedEnemies_));
    graphics.drawString(text, 266, 134, COLOR_TEXT, Graphics::SIZE_13);

    graphics.drawString("MISS", 266, 164, COLOR_ACCENT, Graphics::SIZE_13);
    std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(escapedEnemies_));
    graphics.drawString(text, 266, 181, COLOR_TEXT, Graphics::SIZE_13);
}

void GroundFront::drawPlayfield(Graphics& graphics, uint64_t now) {
    drawBullets(graphics);
    drawEnemies(graphics, now);
    drawBoss(graphics, now);
    drawItems(graphics, now);
    drawPlayer(graphics, now);
    drawParticles(graphics, now);
}

void GroundFront::drawPlayer(Graphics& graphics, uint64_t now) {
    if (now < player_.invincibleUntilMsec && ((now / 80) % 2) == 0) return;
    const int16_t x = static_cast<int16_t>(player_.x);
    const int16_t y = static_cast<int16_t>(player_.y);

    // Compact double-delta silhouette. The visual size stays close to the
    // original craft so the small PLAYER_RADIUS remains easy to read.
//    graphics.fillTriangle(x, y - 2, x - 8, y + 8, x + 8, y + 8, COLOR_PLAYER);
//    graphics.fillTriangle(x, y - 10, x - 2, y + 7, x + 2, y + 7, COLOR_TEXT);
//    graphics.fillTriangle(x, y - 7, x - 5, y + 2, x + 5, y + 2, COLOR_ACCENT);
    graphics.fillTriangle(x, y - 2, x - 8, y + 7, x + 8, y + 7, COLOR_PLAYER);
    graphics.fillTriangle(x, y - 10, x - 2, y + 8, x + 2, y + 8, COLOR_PLAYER);
    graphics.drawTriangle(x, y - 11, x - 3, y + 9, x + 3, y + 9, Graphics::GRAY);
}

void GroundFront::drawEnemies(Graphics& graphics, uint64_t now) {
    (void)now;
    static const Graphics::Color ROUND_MAIN[BOSS_COUNT] = {
        Graphics::rgb565(245, 174, 82),
        Graphics::rgb565(80, 210, 220),
        Graphics::rgb565(235, 112, 155),
        Graphics::rgb565(170, 116, 230),
        Graphics::rgb565(240, 82, 72)
    };
    static const Graphics::Color ROUND_SUB[BOSS_COUNT] = {
        Graphics::rgb565(112, 175, 90),
        Graphics::rgb565(72, 145, 225),
        Graphics::rgb565(174, 105, 72),
        Graphics::rgb565(104, 212, 184),
        Graphics::rgb565(250, 174, 60)
    };
    const uint8_t round = currentBossIndex_ < BOSS_COUNT ? currentBossIndex_ : BOSS_COUNT - 1;
    const Graphics::Color mainColor = ROUND_MAIN[round];
    const Graphics::Color subColor = ROUND_SUB[round];

    for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
        const Enemy& enemy = enemies_[i];
        if (!enemy.active) continue;
        const int16_t x = static_cast<int16_t>(enemy.x);
        const int16_t y = static_cast<int16_t>(enemy.y);
        switch (enemy.type) {
        case ENEMY_BIRD:
            graphics.drawString("v", x, y, mainColor, Graphics::SIZE_18,
                Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
            graphics.drawLine(x - 8, y, x, y + 5, mainColor);
            graphics.drawLine(x, y + 5, x + 8, y, COLOR_WARNING);
            break;
        case ENEMY_BALLOON:
            graphics.fillCircle(x, y - 2, 7, mainColor);
            graphics.drawLine(x, y + 5, x, y + 11, COLOR_TEXT);
            break;
        case ENEMY_TANK:
            graphics.fillRect(x - 8, y - 5, 16, 10, subColor);
            graphics.fillRect(x - 2, y - 10, 4, 8, COLOR_TEXT);
            break;
        case ENEMY_DRONE:
            graphics.drawCircle(x, y, 8, mainColor);
            graphics.drawLine(x - 11, y, x + 11, y, COLOR_TEXT);
            graphics.fillCircle(x, y, 3, subColor);
            break;
        case ENEMY_STAGE_SPECIAL:
            switch (currentBossIndex_) {
            case 0: // Scout
                graphics.fillTriangle(x, y - 8, x - 8, y + 6, x + 8, y + 6, mainColor);
                graphics.drawLine(x - 11, y + 3, x + 11, y + 3, COLOR_TEXT);
                break;
            case 1: // Mortar
                graphics.fillRect(x - 7, y - 6, 14, 12, subColor);
                graphics.fillRect(x - 2, y - 13, 4, 8, COLOR_WARNING);
                break;
            case 2: // Shielder
                graphics.drawCircle(x, y, 9, mainColor);
                graphics.drawCircle(x, y, 6, COLOR_TEXT);
                graphics.fillRect(x - 3, y - 8, 6, 16, subColor);
                break;
            case 3: // Jammer
                graphics.fillCircle(x, y, 6, mainColor);
                graphics.drawLine(x - 12, y - 6, x + 12, y + 6, COLOR_TEXT);
                graphics.drawLine(x - 12, y + 6, x + 12, y - 6, COLOR_TEXT);
                break;
            default: // Assault
                graphics.fillTriangle(x, y - 10, x - 10, y + 8, x + 10, y + 8, mainColor);
                graphics.fillTriangle(x, y - 2, x - 16, y + 8, x + 16, y + 8, subColor);
                break;
            }
            break;
        }
    }
}

void GroundFront::drawBoss(Graphics& graphics, uint64_t now) {
    (void)now;
    if (!boss_.active) return;
    const int16_t x = static_cast<int16_t>(boss_.x);
    const int16_t y = static_cast<int16_t>(boss_.y);

    switch (boss_.type) {
    case 0:
        graphics.fillRect(x - 22, y - 8, 44, 16, Graphics::rgb565(190, 78, 55));
        graphics.fillTriangle(x - 31, y + 5, x - 13, y - 8, x - 12, y + 11, COLOR_BOSS);
        graphics.fillTriangle(x + 31, y + 5, x + 13, y - 8, x + 12, y + 11, COLOR_BOSS);
        graphics.fillCircle(x, y, 6, COLOR_WARNING);
        break;
    case 1:
        graphics.fillCircle(x, y, 14, Graphics::rgb565(54, 146, 126));
        graphics.fillRect(x - 5, y - 17, 10, 28, Graphics::rgb565(72, 186, 156));
        graphics.drawLine(x - 30, y - 15, x + 30, y - 15, COLOR_TEXT);
        graphics.drawLine(x, y - 21, x, y - 9, COLOR_TEXT);
        graphics.fillCircle(x, y, 4, COLOR_WARNING);
        break;
    case 2:
        graphics.fillRect(x - 29, y - 9, 58, 18, Graphics::rgb565(132, 91, 62));
        graphics.fillRect(x - 17, y - 17, 34, 11, Graphics::rgb565(170, 118, 72));
        graphics.fillRect(x - 3, y - 25, 6, 17, COLOR_TEXT);
        graphics.fillCircle(x - 20, y + 10, 5, COLOR_PANEL);
        graphics.fillCircle(x + 20, y + 10, 5, COLOR_PANEL);
        break;
    case 3:
        graphics.fillRect(x - 28, y - 13, 56, 26, Graphics::rgb565(92, 104, 128));
        graphics.fillRect(x - 36, y - 5, 72, 12, Graphics::rgb565(67, 79, 99));
        graphics.fillRect(x - 5, y - 22, 10, 18, COLOR_WARNING);
        graphics.fillCircle(x - 24, y + 13, 6, COLOR_PANEL);
        graphics.fillCircle(x + 24, y + 13, 6, COLOR_PANEL);
        break;
    default:
        graphics.fillTriangle(x, y - 24, x - 27, y + 13, x + 27, y + 13, Graphics::rgb565(166, 54, 178));
        graphics.fillTriangle(x, y - 13, x - 16, y + 9, x + 16, y + 9, Graphics::rgb565(232, 82, 105));
        graphics.fillCircle(x, y, 7, COLOR_PANEL);
        graphics.fillCircle(x, y, 3, COLOR_WARNING);
        graphics.drawLine(x - 34, y, x - 18, y, COLOR_TEXT);
        graphics.drawLine(x + 18, y, x + 34, y, COLOR_TEXT);
        break;
    }

    const int16_t barW = 170;
    const int16_t hpW = boss_.maxHp > 0 ? static_cast<int16_t>((barW * boss_.hp) / boss_.maxHp) : 0;
    graphics.fillRect(75, 6, barW, 7, COLOR_PANEL);
    graphics.fillRect(75, 6, hpW > 0 ? hpW : 0, 7, COLOR_DANGER);
    graphics.drawRect(75, 6, barW, 7, COLOR_TEXT);
}

void GroundFront::drawBullets(Graphics& graphics) {
    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; ++i) {
        const PlayerBullet& shot = playerBullets_[i];
        if (!shot.active) continue;
        if (shot.type == PLAYER_BULLET_HOMING) {
            graphics.fillCircle(static_cast<int16_t>(shot.x), static_cast<int16_t>(shot.y), 1, COLOR_WARNING);
            graphics.drawCircle(static_cast<int16_t>(shot.x), static_cast<int16_t>(shot.y), 2, COLOR_PLAYER_SHOT);
        } else {
            graphics.fillRect(static_cast<int16_t>(shot.x) - 1, static_cast<int16_t>(shot.y) - 4, 3, 8, COLOR_PLAYER_SHOT);
        }
    }
    for (uint8_t i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        const EnemyBullet& shot = enemyBullets_[i];
        if (!shot.active) continue;
        graphics.fillCircle(static_cast<int16_t>(shot.x), static_cast<int16_t>(shot.y), shot.radius, COLOR_ENEMY_SHOT);
        graphics.drawCircle(static_cast<int16_t>(shot.x), static_cast<int16_t>(shot.y), shot.radius + 1, COLOR_TEXT);
    }
}

void GroundFront::drawParticles(Graphics& graphics, uint64_t now) {
    for (uint8_t i = 0; i < MAX_PARTICLES; ++i) {
        const Particle& p = particles_[i];
        if (!p.active) continue;
        const Graphics::Color color = ((p.endMsec - now) > 250) ? COLOR_WARNING : COLOR_DANGER;
        graphics.fillRect(static_cast<int16_t>(p.x), static_cast<int16_t>(p.y), 2, 2, color);
    }
}

void GroundFront::drawItems(Graphics& graphics, uint64_t now) {
    for (uint8_t i = 0; i < MAX_ITEMS; ++i) {
        const Item& item = items_[i];
        if (!item.active) continue;
        const int16_t x = static_cast<int16_t>(item.x);
        const int16_t y = static_cast<int16_t>(item.y);
        graphics.fillRoundRect(x - 8, y - 8, 16, 16, 4, COLOR_PANEL);
        graphics.drawRoundRect(x - 8, y - 8, 16, 16, 4, COLOR_PANEL_LINE);
        switch (item.type) {
        case ITEM_WEAPON: {
            const WeaponType shown = static_cast<WeaponType>(cycleWeaponByTime(now));
            graphics.drawString(shown == WEAPON_NORMAL ? "N" : (shown == WEAPON_SPREAD ? "S" : "H"),
                x, y, shown == WEAPON_HOMING ? COLOR_WARNING : COLOR_ACCENT, Graphics::SIZE_13,
                Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
            break;
        }
        case ITEM_BOMB:
            graphics.fillCircle(x, y, 4, COLOR_WARNING);
            break;
        case ITEM_LIFE:
            graphics.fillTriangle(x, y - 5, x - 5, y + 4, x + 5, y + 4, COLOR_PLAYER);
            break;
        default:
            break;
        }
    }
}

void GroundFront::drawPause(Graphics& graphics) {
    graphics.fillRectAlpha(FIELD_X, 0, FIELD_W, SAFE_UI_BOTTOM, 120, COLOR_OVERLAY);
    graphics.fillRect(82, 88, 156, 60, COLOR_PANEL);
    graphics.drawRect(82, 88, 156, 60, 2, COLOR_ACCENT);
    graphics.drawString("PAUSED", 160, 108, COLOR_TEXT, Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    graphics.drawString("START : RESUME", 160, 136, COLOR_ACCENT, Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void GroundFront::drawResult(Graphics& graphics, uint64_t now) {
    graphics.fillRectAlpha(FIELD_X, 0, FIELD_W, SAFE_UI_BOTTOM, mode_ == MODE_STAGE_CLEAR ? 120 : 148, COLOR_OVERLAY);
    graphics.fillRect(68, 48, 184, 145, COLOR_PANEL);
    graphics.drawRect(68, 48, 184, 145, 2, mode_ == MODE_STAGE_CLEAR ? COLOR_ACCENT : COLOR_DANGER);

    graphics.drawString(mode_ == MODE_STAGE_CLEAR ? "MISSION CLEAR" : "MISSION FAILED",
        160, 72, mode_ == MODE_STAGE_CLEAR ? COLOR_ACCENT : COLOR_DANGER, Graphics::SIZE_22B,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    char text[32];
    std::snprintf(text, sizeof(text), "SCORE %lu", static_cast<unsigned long>(score_));
    graphics.drawString(text, 160, 108, COLOR_TEXT, Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    std::snprintf(text, sizeof(text), "BOSS %u / 5", static_cast<unsigned int>(currentBossIndex_));
    graphics.drawString(text, 160, 131, COLOR_TEXT, Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    if (((now / 500) % 2) == 0) {
        graphics.drawString("A : TITLE", 160, 160, COLOR_WARNING, Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        graphics.drawString("X : RANK", 160, 178, COLOR_ACCENT, Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }
}

void GroundFront::drawRanking(Graphics& graphics, uint64_t now) {
    graphics.drawString("TOP 5 RANKING", 160, 27, COLOR_ACCENT, Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    char text[32];
    for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
        std::snprintf(text, sizeof(text), "%u. %08lu", static_cast<unsigned int>(i + 1),
            static_cast<unsigned long>(ranking_[i]));
        graphics.drawString(text, 160, 67 + i * 27, i == 0 ? COLOR_WARNING : COLOR_TEXT, Graphics::SIZE_22B,
            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }

    if (((now / 500) % 2) == 0) {
        graphics.drawString("B / START : BACK", 160, 211, COLOR_ACCENT, Graphics::SIZE_18,
            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }
}

void GroundFront::onTerminate(Storage& storage) {
    if (!scoreSaved_ && score_ > 0) {
        registerScore();
        saveRanking(storage);
        scoreSaved_ = true;
    }
}

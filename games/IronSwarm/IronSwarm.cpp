#include "IronSwarm.h"
#include <cstdio>

using namespace PRUZEA;

namespace
{
    bool isAcrossRiver(float ax, float bx)
    {
        constexpr float riverX = 760.0f;
        constexpr float riverW = 80.0f;
        return (ax < riverX && bx > riverX + riverW) ||
               (bx < riverX && ax > riverX + riverW);
    }
}

const IronSwarm::Obstacle IronSwarm::OBSTACLES[IronSwarm::OBSTACLE_COUNT] = {
    {150.0f, 145.0f, 54.0f, 38.0f, OBSTACLE_ROCK},
    {365.0f, 355.0f, 66.0f, 34.0f, OBSTACLE_RUIN},
    {210.0f, 705.0f, 48.0f, 58.0f, OBSTACLE_ROCK},
    {500.0f, 955.0f, 68.0f, 38.0f, OBSTACLE_RUIN},
    {600.0f, 110.0f, 42.0f, 46.0f, OBSTACLE_ROCK},
    {1005.0f, 145.0f, 62.0f, 36.0f, OBSTACLE_RUIN},
    {1250.0f, 360.0f, 48.0f, 56.0f, OBSTACLE_ROCK},
    {1040.0f, 700.0f, 70.0f, 36.0f, OBSTACLE_RUIN},
    {1320.0f, 930.0f, 58.0f, 48.0f, OBSTACLE_ROCK},
    {930.0f, 1010.0f, 48.0f, 42.0f, OBSTACLE_RUIN}
};

int16_t IronSwarm::sx(float value)
{
    return static_cast<int16_t>(value);
}

int16_t IronSwarm::sy(float value)
{
    return static_cast<int16_t>(value);
}

void IronSwarm::onInit(Storage& storage)
{
    loadRanking(storage);
    mode = MODE_TITLE;
    clearObjects();
    dirty = true;
}

void IronSwarm::clearObjects()
{
    for (int i = 0; i < MAX_ENEMIES; ++i) enemies[i].active = false;
    for (int i = 0; i < MAX_BULLETS; ++i) bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i) enemyBullets[i].active = false;
    for (int i = 0; i < MAX_ORBS; ++i) orbs[i].active = false;
    for (int i = 0; i < MAX_REPAIRS; ++i) repairs[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; ++i) particles[i].active = false;
    for (int i = 0; i < MAX_TITLE_BULLETS; ++i) titleBullets[i].active = false;
    for (int i = 0; i < MAX_BOMBS; ++i) bombs[i].active = false;

    bomber.active = false;
    bomber.flying = false;
    clearMissionObjects();
    mission = {
        MISSION_NONE, Vector2(), Vector2(), 0,
        0.0f, 0.0f, 0, false
    };
}

void IronSwarm::resetGame()
{
    clearObjects();

    player = Vector2(360.0f, 600.0f);
    lastMoveDirection = Vector2(1.0f, 0.0f);
    bodyAngle = 0.0f;
    turretAngle = 0.0f;
    turretTurnSpeed = 2.8f;

    hp = 5;
    maxHp = 5;
    level = 1;
    experience = 0;
    nextLevelExp = 8;
    score = 0;
    kills = 0;

    moveSpeed = 72.0f;
    bulletDamage = 1;
    fireInterval = 0.48f;
    multiShot = 1;
    bulletPierce = 0;

    fireTimer = 0.15f;
    spawnTimer = 0.10f;
    gameTime = 0.0f;
    invincibleTimer = 0.0f;
    hitFlashTimer = 0.0f;

    bomberEventTimer = 18.0f;
    bossTimer = 70.0f;
    missionTimer = 20.0f;
    missionNoticeTimer = 0.0f;
    specialTargetHeld = false;

    cameraShakeTimer = 0.0f;
    cameraShakeDuration = 0.0f;
    cameraShakeStrength = 0.0f;
    destroyBlastTimer = 0.0f;

    selectedUpgrade = 0;
    lastMissionType = MISSION_NONE;
    lastRank = -1;
    runFinalized = false;

    mode = MODE_READY;
    stateTimer = 1.20f;
    dirty = true;
}

Game::GameState IronSwarm::onUpdate(
    Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    if (mode == MODE_TITLE) {
        updateTitle(deltaSec);
        if (input.justPressed(Input::START) || input.justPressed(Input::A)) {
            audio.playSE(&Audio::SE::NO_1, 0.7f);
            resetGame();
        } else if (input.justPressed(Input::SELECT)) {
            rankingFromTitle = true;
            mode = MODE_RANKING;
            dirty = true;
        }
        return GameState::RUNNING;
    }

    if (mode == MODE_READY) {
        stateTimer -= deltaSec;
        if (stateTimer <= 0.0f) {
            mode = MODE_GO;
            stateTimer = 0.60f;
            audio.playSE(&Audio::SE::NO_8, 0.65f);
            dirty = true;
        }
        return GameState::RUNNING;
    }

    if (mode == MODE_GO) {
        stateTimer -= deltaSec;
        if (stateTimer <= 0.0f) {
            mode = MODE_PLAYING;
            dirty = true;
        }
        return GameState::RUNNING;
    }

    if (mode == MODE_PAUSED) {
        if (input.justPressed(Input::START)) {
            mode = MODE_PLAYING;
            audio.playSE(&Audio::SE::NO_1, 0.6f);
            dirty = true;
        }
        return GameState::RUNNING;
    }

    if (mode == MODE_GAME_OVER) {
        if (!runFinalized) {
            finalizeRun(storage);
        }
        if (input.justPressed(Input::START) || input.justPressed(Input::A)) {
            mode = MODE_RESULT;
            audio.playSE(&Audio::SE::NO_1, 0.6f);
            dirty = true;
        }
        return GameState::RUNNING;
    }

    if (mode == MODE_RESULT) {
        if (input.justPressed(Input::START) || input.justPressed(Input::A)) {
            rankingFromTitle = false;
            mode = MODE_RANKING;
            audio.playSE(&Audio::SE::NO_1, 0.6f);
            dirty = true;
        }
        return GameState::RUNNING;
    }

    if (mode == MODE_RANKING) {
        if (rankingFromTitle) {
            if (input.justPressed(Input::START) ||
                input.justPressed(Input::A) ||
                input.justPressed(Input::B)) {
                mode = MODE_TITLE;
                rankingFromTitle = false;
                dirty = true;
            }
        } else if (input.justPressed(Input::START) || input.justPressed(Input::A)) {
            audio.playSE(&Audio::SE::NO_1, 0.7f);
            resetGame();
        }
        return GameState::RUNNING;
    }

    if (mode == MODE_LEVEL_UP) {
        if (input.justPressed(Input::LEFT) || input.repeat(Input::LEFT)) {
            selectedUpgrade = (selectedUpgrade + 2) % 3;
            audio.playSE(&Audio::SE::NO_5, 0.35f);
            dirty = true;
        }
        if (input.justPressed(Input::RIGHT) || input.repeat(Input::RIGHT)) {
            selectedUpgrade = (selectedUpgrade + 1) % 3;
            audio.playSE(&Audio::SE::NO_5, 0.35f);
            dirty = true;
        }
        if (input.justPressed(Input::A)) {
            applyUpgrade(upgradeChoices[selectedUpgrade], audio);
            mode = MODE_PLAYING;
            dirty = true;
        }
        return GameState::RUNNING;
    }

    if (input.justPressed(Input::START)) {
        mode = MODE_PAUSED;
        audio.stopMusic();
        audio.playSE(&Audio::SE::NO_2, 0.6f);
        dirty = true;
        return GameState::RUNNING;
    }

    gameTime += deltaSec;
    if (invincibleTimer > 0.0f) invincibleTimer -= deltaSec;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= deltaSec;
    if (missionNoticeTimer > 0.0f) missionNoticeTimer -= deltaSec;
    if (cameraShakeTimer > 0.0f) {
        cameraShakeTimer -= deltaSec;
        if (cameraShakeTimer <= 0.0f) {
            cameraShakeTimer = 0.0f;
            cameraShakeStrength = 0.0f;
        }
    }
    if (destroyBlastTimer > 0.0f) {
        destroyBlastTimer -= deltaSec;
        if (destroyBlastTimer < 0.0f) destroyBlastTimer = 0.0f;
    }

    updatePlayer(input, deltaSec);
    updateSpawning(deltaSec);
    updateEnemies(audio, deltaSec);
    updateEnemyShooting(deltaSec);
    updateAutoFire(audio, deltaSec);
    updateBullets(audio, deltaSec);
    updateEnemyBullets(audio, deltaSec);
    updateBomber(audio, deltaSec);
    updateBombs(audio, deltaSec);
    updateOrbs(audio, deltaSec);
    updateRepairs(audio);
    updateParticles(deltaSec);
    updateMission(audio, deltaSec);

    bossTimer -= deltaSec;
    if (bossTimer <= 0.0f && !hasBoss()) {
        spawnBoss(audio);
        bossTimer = 82.0f;
    }

    if (mode == MODE_GAME_OVER && !runFinalized) {
        finalizeRun(storage);
    }

    dirty = true;
    return GameState::RUNNING;
}

void IronSwarm::finalizeRun(Storage& storage)
{
    if (runFinalized) return;

    lastRank = insertRanking(score);
    saveRanking(storage);
    runFinalized = true;
}

void IronSwarm::updateTitle(float deltaSec)
{
    titleAnimTime += deltaSec;
    titleTankX += titleTankSpeed * deltaSec;

    if (titleTankX > 202.0f) {
        titleTankX = 202.0f;
        titleTankSpeed = -20.0f;
    } else if (titleTankX < 118.0f) {
        titleTankX = 118.0f;
        titleTankSpeed = 20.0f;
    }

    const float targetAngle = Math::sin(titleAnimTime * 1.15f) * Math::degToRad(15.0f);
    titleTurretAngle = Math::moveTowardsAngle(
        titleTurretAngle, targetAngle, 1.8f * deltaSec);

    titleFireTimer -= deltaSec;
    if (titleFireTimer <= 0.0f) {
        for (int i = 0; i < MAX_TITLE_BULLETS; ++i) {
            if (titleBullets[i].active) continue;

            const Vector2 dir(Math::cos(titleTurretAngle), Math::sin(titleTurretAngle));
            titleBullets[i].position = Vector2(titleTankX, 110.0f) + dir * 18.0f;
            titleBullets[i].velocity = dir * 92.0f;
            titleBullets[i].life = 1.45f;
            titleBullets[i].active = true;
            break;
        }

        titleFireTimer = 0.72f;
    }

    for (int i = 0; i < MAX_TITLE_BULLETS; ++i) {
        TitleBullet& bullet = titleBullets[i];
        if (!bullet.active) continue;

        bullet.position += bullet.velocity * deltaSec;
        bullet.life -= deltaSec;

        if (bullet.life <= 0.0f ||
            bullet.position.x < 0.0f || bullet.position.x >= SCREEN_W ||
            bullet.position.y < 0.0f || bullet.position.y >= SCREEN_H) {
            bullet.active = false;
        }
    }

    dirty = true;
}

void IronSwarm::updatePlayer(Input& input, float deltaSec)
{
    specialTargetHeld = input.pressed(Input::B);

    Vector2 direction;

    if (input.hasAnalogSticks()) {
        direction.x = static_cast<float>(input.axis(Input::LEFT_X)) / 1000.0f;
        direction.y = static_cast<float>(input.axis(Input::LEFT_Y)) / 1000.0f;
    }

    if (input.pressed(Input::LEFT))  direction.x -= 1.0f;
    if (input.pressed(Input::RIGHT)) direction.x += 1.0f;
    if (input.pressed(Input::UP))    direction.y -= 1.0f;
    if (input.pressed(Input::DOWN))  direction.y += 1.0f;

    if (direction.length() > 1.0f) {
        direction = direction.normalized();
    }

    if (direction.length() > 0.05f) {
        lastMoveDirection = direction.normalized();
        const float targetAngle = Math::angle(lastMoveDirection.x, lastMoveDirection.y);
        bodyAngle = Math::moveTowardsAngle(bodyAngle, targetAngle, 6.0f * deltaSec);

        player += direction * (moveSpeed * deltaSec);
        resolveTerrainCollision();

        player.x = Math::clamp(player.x, PLAYER_RADIUS, static_cast<float>(WORLD_W) - PLAYER_RADIUS);
        player.y = Math::clamp(player.y, PLAYER_RADIUS, static_cast<float>(WORLD_H) - PLAYER_RADIUS);
    }

    float targetAngle = bodyAngle;
    Vector2 specialTarget;

    if (specialTargetHeld && getSpecialTarget(specialTarget)) {
        const Vector2 toTarget = specialTarget - player;
        targetAngle = Math::angle(toTarget.x, toTarget.y);
    } else {
        const int nearest = findNearestEnemy();
        if (nearest >= 0) {
            const Vector2 toEnemy = enemies[nearest].position - player;
            targetAngle = Math::angle(toEnemy.x, toEnemy.y);
        }
    }

    turretAngle = Math::moveTowardsAngle(
        turretAngle, targetAngle, turretTurnSpeed * deltaSec);
}

void IronSwarm::resolveTerrainCollision()
{
    resolveActorTerrain(player, PLAYER_RADIUS);
}

void IronSwarm::resolveActorTerrain(Vector2& position, float radius)
{
    const float riverSegments[3][4] = {
        {RIVER_X, 0.0f, RIVER_W, BRIDGE_0_Y},
        {RIVER_X, BRIDGE_0_Y + BRIDGE_H,
         RIVER_W, BRIDGE_1_Y - (BRIDGE_0_Y + BRIDGE_H)},
        {RIVER_X, BRIDGE_1_Y + BRIDGE_H,
         RIVER_W, WORLD_H - (BRIDGE_1_Y + BRIDGE_H)}
    };

    for (int i = 0; i < 3; ++i) {
        Vector2 pushOut;
        if (Collision::circleRect(
                position.x, position.y, radius,
                riverSegments[i][0], riverSegments[i][1],
                riverSegments[i][2], riverSegments[i][3],
                pushOut)) {
            position += pushOut;
        }
    }

    for (int i = 0; i < OBSTACLE_COUNT; ++i) {
        const Obstacle& o = OBSTACLES[i];
        Vector2 pushOut;
        if (Collision::circleRect(
                position.x, position.y, radius,
                o.x, o.y, o.w, o.h, pushOut)) {
            position += pushOut;
        }
    }
}

bool IronSwarm::isActorPositionBlocked(const Vector2& position, float radius) const
{
    if (position.x < radius || position.y < radius ||
        position.x > static_cast<float>(WORLD_W) - radius ||
        position.y > static_cast<float>(WORLD_H) - radius) {
        return true;
    }

    const float riverSegments[3][4] = {
        {RIVER_X, 0.0f, RIVER_W, BRIDGE_0_Y},
        {RIVER_X, BRIDGE_0_Y + BRIDGE_H,
         RIVER_W, BRIDGE_1_Y - (BRIDGE_0_Y + BRIDGE_H)},
        {RIVER_X, BRIDGE_1_Y + BRIDGE_H,
         RIVER_W, WORLD_H - (BRIDGE_1_Y + BRIDGE_H)}
    };

    for (int i = 0; i < 3; ++i) {
        if (Collision::circleRect(
                position.x, position.y, radius,
                riverSegments[i][0], riverSegments[i][1],
                riverSegments[i][2], riverSegments[i][3])) {
            return true;
        }
    }

    for (int i = 0; i < OBSTACLE_COUNT; ++i) {
        const Obstacle& o = OBSTACLES[i];
        if (Collision::circleRect(
                position.x, position.y, radius,
                o.x, o.y, o.w, o.h)) {
            return true;
        }
    }

    return false;
}

bool IronSwarm::isWorldPointVisible(const Vector2& position, float margin) const
{
    const float cameraX = Math::clamp(
        player.x - 160.0f, 0.0f,
        static_cast<float>(WORLD_W - SCREEN_W));
    const float cameraY = Math::clamp(
        player.y - 120.0f, 0.0f,
        static_cast<float>(WORLD_H - SCREEN_H));

    return position.x >= cameraX - margin &&
           position.x <= cameraX + SCREEN_W + margin &&
           position.y >= cameraY - margin &&
           position.y <= cameraY + SCREEN_H + margin;
}

void IronSwarm::triggerCameraShake(float strength, float duration)
{
    if (strength >= cameraShakeStrength || cameraShakeTimer <= 0.0f) {
        cameraShakeStrength = strength;
        cameraShakeDuration = duration;
        cameraShakeTimer = duration;
    }
}

void IronSwarm::triggerDestroyExplosion(const Vector2& position)
{
    destroyBlastPosition = position;
    destroyBlastTimer = 0.48f;
    spawnDeathParticles(
        position, Graphics::rgb565(236, 118, 48), 16);
    triggerCameraShake(3.0f, 0.18f);
}

bool IronSwarm::isBulletBlocked(const Vector2& position) const
{
    for (int i = 0; i < OBSTACLE_COUNT; ++i) {
        const Obstacle& o = OBSTACLES[i];
        if (Collision::pointRect(position.x, position.y, o.x, o.y, o.w, o.h)) {
            return true;
        }
    }
    return false;
}

Vector2 IronSwarm::getBridgeCenter(int bridgeIndex) const
{
    const float y = bridgeIndex == 0
        ? BRIDGE_0_Y + BRIDGE_H * 0.5f
        : BRIDGE_1_Y + BRIDGE_H * 0.5f;
    return Vector2(RIVER_X + RIVER_W * 0.5f, y);
}

Vector2 IronSwarm::getMoveTargetAcrossRiver(
    const Vector2& from, const Vector2& destination) const
{
    if (!isAcrossRiver(from.x, destination.x)) {
        return destination;
    }

    const Vector2 b0 = getBridgeCenter(0);
    const Vector2 b1 = getBridgeCenter(1);
    return from.distance(b0) < from.distance(b1) ? b0 : b1;
}

Vector2 IronSwarm::getEnemyMoveTarget(const Enemy& enemy) const
{
    return getMoveTargetAcrossRiver(enemy.position, player);
}

void IronSwarm::updateSpawning(float deltaSec)
{
    spawnTimer -= deltaSec;
    if (spawnTimer > 0.0f) return;

    int activeCount = 0;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (enemies[i].active && enemies[i].type != ENEMY_BOSS) activeCount++;
    }

    int desired = 6 + static_cast<int>(gameTime / 34.0f);
    desired = Math::clamp(desired, 6, MAX_ENEMIES - 1);

    if (activeCount < desired) {
        spawnEnemy();
    }

    spawnTimer = Math::clamp(0.82f - gameTime * 0.0018f, 0.32f, 0.82f);
}

void IronSwarm::spawnEnemy()
{
    int slot = -1;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemies[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;

    const float angle = Math::randomFloat(0.0f, Math::TWO_PI);
    const float distance = Math::randomFloat(180.0f, 225.0f);

    Enemy& enemy = enemies[slot];
    enemy.position = player + Vector2(Math::cos(angle), Math::sin(angle)) * distance;
    enemy.position.x = Math::clamp(enemy.position.x, 20.0f, static_cast<float>(WORLD_W) - 20.0f);
    enemy.position.y = Math::clamp(enemy.position.y, 20.0f, static_cast<float>(WORLD_H) - 20.0f);

    const float roll = Math::randomFloat();
    const float gunnerChance = level >= 2
        ? Math::clamp(0.10f + static_cast<float>(level - 2) * 0.035f, 0.10f, 0.30f)
        : 0.0f;
    const float heavyChance = level >= 5
        ? Math::clamp(0.06f + static_cast<float>(level - 5) * 0.02f, 0.06f, 0.18f)
        : 0.0f;

    if (roll < heavyChance) {
        enemy.type = ENEMY_HEAVY;
        enemy.radius = 10.0f;
        enemy.speed = 20.0f;
        enemy.hp = 7 + level / 4;
        enemy.shootTimer = 2.4f;
    } else if (roll < heavyChance + gunnerChance) {
        enemy.type = ENEMY_GUNNER;
        enemy.radius = 7.0f;
        enemy.speed = 25.0f;
        enemy.hp = 2 + level / 6;
        enemy.shootTimer = Math::randomFloat(1.4f, 2.4f);
    } else {
        enemy.type = ENEMY_SCOUT;
        enemy.radius = ENEMY_RADIUS;
        enemy.speed = Math::randomFloat(29.0f, 39.0f);
        enemy.hp = level >= 6 && Math::chance(0.25f) ? 2 : 1;
        enemy.shootTimer = 0.0f;
    }

    resolveActorTerrain(enemy.position, enemy.radius);
    enemy.active = true;
}

void IronSwarm::spawnBoss(Audio& audio)
{
    int slot = -1;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemies[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;

    Enemy& enemy = enemies[slot];
    enemy.type = ENEMY_BOSS;
    enemy.radius = 16.0f;
    enemy.speed = 16.0f;
    enemy.hp = 28 + level * 3;
    enemy.shootTimer = 1.2f;

    const float side = player.x < WORLD_W * 0.5f ? 1.0f : -1.0f;
    enemy.position = player + Vector2(side * 210.0f, Math::randomFloat(-80.0f, 80.0f));
    enemy.position.x = Math::clamp(enemy.position.x, 25.0f, static_cast<float>(WORLD_W) - 25.0f);
    enemy.position.y = Math::clamp(enemy.position.y, 25.0f, static_cast<float>(WORLD_H) - 25.0f);
    resolveActorTerrain(enemy.position, enemy.radius);
    enemy.active = true;

    audio.playSE(&Audio::SE::NO_10, 0.9f);
}

bool IronSwarm::hasBoss() const
{
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (enemies[i].active && enemies[i].type == ENEMY_BOSS) return true;
    }
    return false;
}

void IronSwarm::updateEnemies(Audio& audio, float deltaSec)
{
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& enemy = enemies[i];
        if (!enemy.active) continue;

        const Vector2 target = getEnemyMoveTarget(enemy);
        Vector2 toTarget = target - enemy.position;
        Vector2 toPlayer = player - enemy.position;
        const float playerDistance = toPlayer.length();

        float moveSign = 1.0f;
        if (enemy.type == ENEMY_GUNNER && !isAcrossRiver(enemy.position.x, player.x)) {
            if (playerDistance < 82.0f) moveSign = -1.0f;
            else if (playerDistance < 116.0f) moveSign = 0.0f;
        }
        if (enemy.type == ENEMY_BOSS && !isAcrossRiver(enemy.position.x, player.x)) {
            if (playerDistance < 96.0f) moveSign = -0.5f;
            else if (playerDistance < 128.0f) moveSign = 0.0f;
        }

        if (toTarget.length() > 0.001f && moveSign != 0.0f) {
            enemy.position += toTarget.normalized() * (enemy.speed * moveSign * deltaSec);
            resolveActorTerrain(enemy.position, enemy.radius);
        }

        if (Collision::circleCircle(
                player.x, player.y, PLAYER_RADIUS,
                enemy.position.x, enemy.position.y, enemy.radius)) {
            Vector2 pushOut;
            if (Collision::circleCircle(
                    player.x, player.y, PLAYER_RADIUS,
                    enemy.position.x, enemy.position.y, enemy.radius,
                    pushOut)) {
                player += pushOut * 0.60f;
                enemy.position -= pushOut * 0.40f;
                resolveTerrainCollision();
                resolveActorTerrain(enemy.position, enemy.radius);
            }

            if (invincibleTimer <= 0.0f) {
                damagePlayer(enemy.type == ENEMY_BOSS ? 2 : 1, audio);
                if (mode == MODE_GAME_OVER) return;
            }
        }
    }
}

void IronSwarm::updateEnemyShooting(float deltaSec)
{
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& enemy = enemies[i];
        if (!enemy.active) continue;
        if (enemy.type != ENEMY_GUNNER &&
            enemy.type != ENEMY_HEAVY &&
            enemy.type != ENEMY_BOSS) {
            continue;
        }

        enemy.shootTimer -= deltaSec;
        if (enemy.shootTimer > 0.0f) continue;

        const Vector2 toPlayer = player - enemy.position;
        if (toPlayer.length() <= 0.001f) continue;

        const float angle = Math::angle(toPlayer.x, toPlayer.y);

        if (enemy.type == ENEMY_BOSS) {
            const float spread = Math::degToRad(12.0f);
            spawnEnemyBullet(enemy.position, angle - spread, 82.0f, 1);
            spawnEnemyBullet(enemy.position, angle,          88.0f, 1);
            spawnEnemyBullet(enemy.position, angle + spread, 82.0f, 1);
            enemy.shootTimer = Math::clamp(1.55f - level * 0.035f, 1.05f, 1.55f);
        } else if (enemy.type == ENEMY_HEAVY) {
            spawnEnemyBullet(enemy.position, angle, 68.0f, 2);
            enemy.shootTimer = Math::clamp(2.9f - level * 0.045f, 2.05f, 2.9f);
        } else {
            spawnEnemyBullet(enemy.position, angle, 76.0f, 1);
            enemy.shootTimer = Math::clamp(2.55f - level * 0.055f, 1.65f, 2.55f);
        }
    }
}

void IronSwarm::spawnEnemyBullet(
    const Vector2& position, float angle, float speed, uint8_t damage)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        if (enemyBullets[i].active) continue;

        const Vector2 direction(Math::cos(angle), Math::sin(angle));
        enemyBullets[i].position = position + direction * 9.0f;
        enemyBullets[i].velocity = direction * speed;
        enemyBullets[i].life = 3.0f;
        enemyBullets[i].damage = damage;
        enemyBullets[i].active = true;
        return;
    }
}

int IronSwarm::findNearestEnemy() const
{
    int best = -1;
    float bestDistance = 1000000000.0f;

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemies[i].active) continue;
        if (!isWorldPointVisible(enemies[i].position, 10.0f)) continue;

        const float d = player.distance(enemies[i].position);
        if (d < bestDistance) {
            bestDistance = d;
            best = i;
        }
    }
    return best;
}

bool IronSwarm::getSpecialTarget(Vector2& outPosition) const
{
    if (!mission.active) return false;

    if (mission.type == MISSION_DESTROY && missionTarget.active) {
        outPosition = missionTarget.position;
        return true;
    }

    if (mission.type == MISSION_INTERCEPT && supplyVehicle.active) {
        outPosition = supplyVehicle.position;
        return true;
    }

    return false;
}

void IronSwarm::updateAutoFire(Audio& audio, float deltaSec)
{
    fireTimer -= deltaSec;
    if (fireTimer > 0.0f) return;

    float targetAngle = turretAngle;
    bool targetAvailable = false;

    Vector2 specialTarget;
    if (specialTargetHeld && getSpecialTarget(specialTarget)) {
        const Vector2 toTarget = specialTarget - player;
        if (toTarget.length() > 0.001f) {
            targetAngle = Math::angle(toTarget.x, toTarget.y);
            targetAvailable = true;
        }
    } else {
        const int target = findNearestEnemy();
        if (target >= 0) {
            const Vector2 toEnemy = enemies[target].position - player;
            if (toEnemy.length() > 0.001f) {
                targetAngle = Math::angle(toEnemy.x, toEnemy.y);
                targetAvailable = true;
            }
        }
    }

    if (!targetAvailable) return;

    float angleError = Math::deltaAngle(turretAngle, targetAngle);
    if (angleError < 0.0f) angleError = -angleError;

    if (angleError > Math::degToRad(7.0f)) {
        return;
    }

    if (multiShot <= 1) {
        spawnBullet(turretAngle);
    } else {
        const float spread = Math::degToRad(10.0f);
        const float center = (static_cast<float>(multiShot) - 1.0f) * 0.5f;
        for (int i = 0; i < multiShot; ++i) {
            spawnBullet(turretAngle + (static_cast<float>(i) - center) * spread);
        }
    }

    audio.playSE(&Audio::SE::NO_1, 0.22f);
    fireTimer = fireInterval;
}

void IronSwarm::spawnBullet(float angle)
{
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].active) continue;

        const Vector2 direction(Math::cos(angle), Math::sin(angle));
        bullets[i].position = player + direction * 13.0f;
        bullets[i].velocity = direction * 175.0f;
        bullets[i].life = 1.7f;
        bullets[i].damage = bulletDamage;
        bullets[i].pierce = bulletPierce;
        bullets[i].active = true;
        return;
    }
}

void IronSwarm::updateBullets(Audio& audio, float deltaSec)
{
    for (int b = 0; b < MAX_BULLETS; ++b) {
        Bullet& bullet = bullets[b];
        if (!bullet.active) continue;

        bullet.position += bullet.velocity * deltaSec;
        bullet.life -= deltaSec;

        if (bullet.life <= 0.0f || isBulletBlocked(bullet.position)) {
            bullet.active = false;
            continue;
        }

        if (handleMissionBulletHit(bullet, audio)) {
            continue;
        }

        for (int e = 0; e < MAX_ENEMIES; ++e) {
            Enemy& enemy = enemies[e];
            if (!enemy.active) continue;

            if (!Collision::circleCircle(
                    bullet.position.x, bullet.position.y, BULLET_RADIUS,
                    enemy.position.x, enemy.position.y, enemy.radius)) {
                continue;
            }

            enemy.hp -= bullet.damage;

            if (enemy.hp <= 0) {
                const Vector2 deathPosition = enemy.position;
                const EnemyType type = enemy.type;
                enemy.active = false;
                kills++;

                if (type == ENEMY_BOSS) {
                    spawnDeathParticles(
                        deathPosition, Graphics::rgb565(220, 74, 52), 10);
                    score += 2500;
                    for (int i = 0; i < 3; ++i) {
                        const float a = Math::randomFloat(0.0f, Math::TWO_PI);
                        spawnOrb(
                            deathPosition +
                            Vector2(Math::cos(a), Math::sin(a)) * 14.0f,
                            4);
                    }
                    audio.playSE(&Audio::SE::NO_4, 1.0f);
                } else {
                    const Graphics::Color particleColor =
                        type == ENEMY_HEAVY
                        ? Graphics::rgb565(145, 82, 66)
                        : type == ENEMY_GUNNER
                        ? Graphics::rgb565(214, 143, 50)
                        : Graphics::rgb565(190, 76, 56);
                    spawnDeathParticles(
                        deathPosition, particleColor,
                        type == ENEMY_HEAVY ? 6 : 4);

                    score += type == ENEMY_HEAVY ? 250 :
                             type == ENEMY_GUNNER ? 160 : 100;
                    spawnOrb(deathPosition, type == ENEMY_HEAVY ? 2 : 1);
                    audio.playSE(&Audio::SE::NO_6, 0.32f);
                }
            }

            if (bullet.pierce > 0) {
                bullet.pierce--;
            } else {
                bullet.active = false;
                break;
            }
        }
    }
}

void IronSwarm::updateEnemyBullets(Audio& audio, float deltaSec)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        EnemyBullet& bullet = enemyBullets[i];
        if (!bullet.active) continue;

        bullet.position += bullet.velocity * deltaSec;
        bullet.life -= deltaSec;

        if (bullet.life <= 0.0f || isBulletBlocked(bullet.position)) {
            bullet.active = false;
            continue;
        }

        if (Collision::circleCircle(
                player.x, player.y, PLAYER_RADIUS,
                bullet.position.x, bullet.position.y, 3.0f)) {
            bullet.active = false;
            damagePlayer(bullet.damage, audio);
            if (mode == MODE_GAME_OVER) return;
        }
    }
}

void IronSwarm::spawnOrb(const Vector2& position, uint8_t value)
{
    for (int i = 0; i < MAX_ORBS; ++i) {
        if (!orbs[i].active) {
            orbs[i].position = position;
            orbs[i].value = value;
            orbs[i].active = true;
            return;
        }
    }

    for (int i = 0; i < MAX_ORBS; ++i) {
        if (orbs[i].active) {
            orbs[i].value = static_cast<uint8_t>(
                Math::clamp<int>(orbs[i].value + value, 1, 255));
            return;
        }
    }
}

void IronSwarm::spawnMissionReward(const Vector2& position)
{
    int freeCount = 0;
    for (int i = 0; i < MAX_ORBS; ++i) {
        if (!orbs[i].active) freeCount++;
    }

    // Mission rewards must always be visible. Reclaim ordinary orbs first
    // instead of silently merging the reward into an off-screen orb.
    for (int pass = 0; pass < 2 && freeCount < 3; ++pass) {
        for (int i = 0; i < MAX_ORBS && freeCount < 3; ++i) {
            if (!orbs[i].active) continue;

            const bool ordinary = orbs[i].value < 4;
            if ((pass == 0 && ordinary) || pass == 1) {
                orbs[i].active = false;
                freeCount++;
            }
        }
    }

    for (int i = 0; i < 3; ++i) {
        const float angle =
            -Math::PI * 0.5f +
            Math::TWO_PI * static_cast<float>(i) / 3.0f;
        spawnOrb(
            position +
            Vector2(Math::cos(angle), Math::sin(angle)) * 34.0f,
            4);
    }
}

void IronSwarm::spawnRepair(const Vector2& position)
{
    for (int i = 0; i < MAX_REPAIRS; ++i) {
        if (!repairs[i].active) {
            repairs[i].position = position;
            repairs[i].active = true;
            return;
        }
    }
}

void IronSwarm::updateOrbs(Audio& audio, float deltaSec)
{
    constexpr float PICKUP_RADIUS = 14.0f;
    constexpr float MAGNET_RADIUS = 48.0f;

    for (int i = 0; i < MAX_ORBS; ++i) {
        ExpOrb& orb = orbs[i];
        if (!orb.active) continue;

        Vector2 toPlayer = player - orb.position;
        const float distance = toPlayer.length();

        if (distance < MAGNET_RADIUS && distance > 0.001f) {
            const float pull = Math::map(
                distance, 0.0f, MAGNET_RADIUS, 150.0f, 45.0f);
            orb.position += toPlayer.normalized() * (pull * deltaSec);
        }

        if (distance <= PICKUP_RADIUS) {
            const int value = orb.value;
            orb.active = false;
            gainExperience(value, audio);
            if (mode == MODE_LEVEL_UP) return;
        }
    }
}

void IronSwarm::updateRepairs(Audio& audio)
{
    for (int i = 0; i < MAX_REPAIRS; ++i) {
        RepairItem& item = repairs[i];
        if (!item.active) continue;

        if (player.distance(item.position) <= 16.0f) {
            item.active = false;
            hp = Math::clamp(hp + 2, 0, maxHp);
            audio.playSE(&Audio::SE::NO_3, 0.8f);
        }
    }
}

void IronSwarm::spawnDeathParticles(
    const Vector2& position, Graphics::Color color, int count)
{
    for (int n = 0; n < count; ++n) {
        int slot = -1;
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            if (!particles[i].active) {
                slot = i;
                break;
            }
        }
        if (slot < 0) return;

        const float angle = Math::randomFloat(0.0f, Math::TWO_PI);
        const float speed = Math::randomFloat(20.0f, 52.0f);

        Particle& p = particles[slot];
        p.position = position;
        p.velocity = Vector2(Math::cos(angle), Math::sin(angle)) * speed;
        p.maxLife = Math::randomFloat(0.20f, 0.38f);
        p.life = p.maxLife;
        p.size = static_cast<uint8_t>(Math::random(1, 3));
        p.color = color;
        p.active = true;
    }
}

void IronSwarm::updateParticles(float deltaSec)
{
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        Particle& p = particles[i];
        if (!p.active) continue;

        p.position += p.velocity * deltaSec;
        p.velocity *= Math::clamp(1.0f - deltaSec * 3.5f, 0.0f, 1.0f);
        p.life -= deltaSec;

        if (p.life <= 0.0f) {
            p.active = false;
        }
    }
}

void IronSwarm::gainExperience(int amount, Audio& audio)
{
    experience += amount;

    if (experience >= nextLevelExp) {
        experience = 0;
        level++;
        nextLevelExp = 7 + level * 4;
        beginLevelUp(audio);
    }
}

void IronSwarm::beginLevelUp(Audio& audio)
{
    generateUpgradeChoices();
    selectedUpgrade = 0;
    mode = MODE_LEVEL_UP;
    audio.playSE(&Audio::SE::NO_11, 0.7f);
}

void IronSwarm::generateUpgradeChoices()
{
    upgradeChoices[0] = static_cast<Upgrade>(Math::random(UPGRADE_COUNT));

    do {
        upgradeChoices[1] = static_cast<Upgrade>(Math::random(UPGRADE_COUNT));
    } while (upgradeChoices[1] == upgradeChoices[0]);

    do {
        upgradeChoices[2] = static_cast<Upgrade>(Math::random(UPGRADE_COUNT));
    } while (upgradeChoices[2] == upgradeChoices[0] ||
             upgradeChoices[2] == upgradeChoices[1]);
}

void IronSwarm::applyUpgrade(Upgrade upgrade, Audio& audio)
{
    switch (upgrade) {
        case UPGRADE_POWER:
            bulletDamage++;
            break;
        case UPGRADE_RAPID:
            fireInterval *= 0.82f;
            fireInterval = Math::clamp(fireInterval, 0.12f, 1.0f);
            break;
        case UPGRADE_MULTI:
            multiShot = Math::clamp(multiShot + 1, 1, 5);
            break;
        case UPGRADE_PIERCE:
            bulletPierce = Math::clamp(bulletPierce + 1, 0, 4);
            break;
        case UPGRADE_ENGINE:
            moveSpeed += 8.0f;
            break;
        case UPGRADE_ARMOR:
            maxHp++;
            hp = Math::clamp(hp + 2, 0, maxHp);
            break;
        case UPGRADE_TURRET:
            turretTurnSpeed = Math::clamp(turretTurnSpeed + 0.75f, 2.8f, 8.0f);
            break;
        default:
            break;
    }

    audio.playSE(&Audio::SE::NO_3, 0.8f);
}

const char* IronSwarm::getUpgradeName(Upgrade upgrade) const
{
    switch (upgrade) {
        case UPGRADE_POWER:  return "POWER";
        case UPGRADE_RAPID:  return "RAPID";
        case UPGRADE_MULTI:  return "MULTI";
        case UPGRADE_PIERCE: return "PIERCE";
        case UPGRADE_ENGINE: return "ENGINE";
        case UPGRADE_ARMOR:  return "ARMOR";
        case UPGRADE_TURRET: return "TURRET";
        default:             return "---";
    }
}

const char* IronSwarm::getUpgradeDescription(Upgrade upgrade) const
{
    switch (upgrade) {
        case UPGRADE_POWER:  return "SHELL DAMAGE";
        case UPGRADE_RAPID:  return "RELOAD TIME";
        case UPGRADE_MULTI:  return "EXTRA SHELL";
        case UPGRADE_PIERCE: return "PENETRATION";
        case UPGRADE_ENGINE: return "MOVE SPEED";
        case UPGRADE_ARMOR:  return "MAX ARMOR";
        case UPGRADE_TURRET: return "TURN SPEED";
        default:             return "";
    }
}

const char* IronSwarm::getUpgradeValue(Upgrade upgrade) const
{
    switch (upgrade) {
        case UPGRADE_POWER:  return "+1";
        case UPGRADE_RAPID:  return "FASTER";
        case UPGRADE_MULTI:  return "+1";
        case UPGRADE_PIERCE: return "+1";
        case UPGRADE_ENGINE: return "+8";
        case UPGRADE_ARMOR:  return "+1";
        case UPGRADE_TURRET: return "+0.75";
        default:             return "";
    }
}

void IronSwarm::damagePlayer(int damage, Audio& audio)
{
    if (invincibleTimer > 0.0f || mode != MODE_PLAYING) return;

    hp -= damage;
    if (hp < 0) hp = 0;

    invincibleTimer = 0.75f;
    hitFlashTimer = 0.16f;
    audio.playSE(&Audio::SE::NO_2, 0.8f);

    if (hp <= 0) {
        mode = MODE_GAME_OVER;
        audio.playSE(&Audio::SE::NO_12, 0.9f);
    }
}

void IronSwarm::loadRanking(Storage& storage)
{
    for (int i = 0; i < RANKING_COUNT; ++i) rankings[i] = 0;

    SaveData save;
    if (!save.load(storage, getId(), "save.dat")) {
        return;
    }

    char key[8];
    for (int i = 0; i < RANKING_COUNT; ++i) {
        snprintf(key, sizeof(key), "s%d", i);
        rankings[i] = save.getUInt32(key, 0);
    }

    // Migrate the previous single best score when present.
    const uint32_t oldBest = save.getUInt32("best", 0);
    if (rankings[0] == 0 && oldBest > 0) {
        rankings[0] = oldBest;
    }
}

void IronSwarm::saveRanking(Storage& storage)
{
    SaveData save;
    save.load(storage, getId(), "save.dat");

    char key[8];
    for (int i = 0; i < RANKING_COUNT; ++i) {
        snprintf(key, sizeof(key), "s%d", i);
        save.setUInt32(key, rankings[i]);
    }

    save.save(storage, getId(), "save.dat");
}

int IronSwarm::insertRanking(uint32_t value)
{
    for (int i = 0; i < RANKING_COUNT; ++i) {
        if (value > rankings[i]) {
            for (int j = RANKING_COUNT - 1; j > i; --j) {
                rankings[j] = rankings[j - 1];
            }
            rankings[i] = value;
            return i;
        }
    }
    return -1;
}

void IronSwarm::onTerminate(Storage& storage)
{
    saveRanking(storage);
}

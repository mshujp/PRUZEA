#include "IronSwarm.h"

using namespace PRUZEA;

namespace
{
    static const Audio::SoundStep BOMB_EXPLOSION_STEPS[] = {
        {130, 75, 90, 1.0f, 0.9f},
        {75, 45, 150, 0.9f, 0.0f}
    };

    static const Audio::Sound BOMB_EXPLOSION_SOUND = {
        BOMB_EXPLOSION_STEPS,
        static_cast<uint16_t>(
            sizeof(BOMB_EXPLOSION_STEPS) / sizeof(BOMB_EXPLOSION_STEPS[0]))
    };
}

void IronSwarm::updateBomber(Audio& audio, float deltaSec)
{
    if (!bomber.active) {
        bomberEventTimer -= deltaSec;
        if (bomberEventTimer <= 0.0f) {
            startBomberRun(audio);
            bomberEventTimer = Math::randomFloat(24.0f, 34.0f);
        }
        return;
    }

    if (!bomber.flying) {
        bomber.warningTimer -= deltaSec;
        if (bomber.warningTimer <= 0.0f) {
            bomber.flying = true;
            audio.playSE(&Audio::SE::NO_7, 0.8f);
        }
        return;
    }

    const float travel = bomber.speed * deltaSec;
    bomber.position += bomber.direction * travel;
    bomber.flightTimer += deltaSec;

    if (isWorldPointVisible(bomber.position, 28.0f)) {
        bomber.enteredView = true;
    }

    bomber.bombTimer -= deltaSec;
    if (bomber.bombTimer <= 0.0f) {
        dropBomb();
        bomber.bombTimer = 0.43f;
    }

    // Do not use a distance fixed at spawn time. The player may scroll
    // in the same direction as the aircraft.
    if ((bomber.enteredView &&
         !isWorldPointVisible(bomber.position, 86.0f)) ||
        bomber.flightTimer >= 6.5f) {
        bomber.active = false;
        bomber.flying = false;
    }
}

void IronSwarm::startBomberRun(Audio& audio)
{
    const bool horizontal = Math::chance(0.5f);
    const bool positive = Math::chance(0.5f);

    bomber.active = true;
    bomber.flying = false;
    bomber.warningTimer = 1.35f;
    bomber.speed = 185.0f;
    bomber.bombTimer = 0.25f;
    bomber.flightTimer = 0.0f;
    bomber.enteredView = false;

    if (horizontal) {
        bomber.direction = Vector2(positive ? 1.0f : -1.0f, 0.0f);
        bomber.position = player + Vector2(
            positive ? -235.0f : 235.0f,
            Math::randomFloat(-85.0f, 85.0f));
    } else {
        bomber.direction = Vector2(0.0f, positive ? 1.0f : -1.0f);
        bomber.position = player + Vector2(
            Math::randomFloat(-115.0f, 115.0f),
            positive ? -235.0f : 235.0f);
    }

    audio.playSE(&Audio::SE::NO_7, 0.7f);
}

void IronSwarm::dropBomb()
{
    for (int i = 0; i < MAX_BOMBS; ++i) {
        if (bombs[i].active) continue;

        bombs[i].position = bomber.position;
        bombs[i].fuse = 0.82f;
        bombs[i].explosionTimer = 0.0f;
        bombs[i].exploded = false;
        bombs[i].active = true;
        return;
    }
}

void IronSwarm::updateBombs(Audio& audio, float deltaSec)
{
    for (int i = 0; i < MAX_BOMBS; ++i) {
        Bomb& bomb = bombs[i];
        if (!bomb.active) continue;

        if (!bomb.exploded) {
            bomb.fuse -= deltaSec;
            if (bomb.fuse <= 0.0f) {
                explodeBomb(bomb, audio);
            }
        } else {
            bomb.explosionTimer -= deltaSec;
            if (bomb.explosionTimer <= 0.0f) {
                bomb.active = false;
            }
        }
    }
}

void IronSwarm::explodeBomb(Bomb& bomb, Audio& audio)
{
    bomb.exploded = true;
    bomb.explosionTimer = 0.28f;
    audio.playSE(&BOMB_EXPLOSION_SOUND, 1.0f);

    if (isWorldPointVisible(bomb.position, 8.0f)) {
        const float distance = player.distance(bomb.position);
        const float strength = distance < 90.0f ? 2.0f : 1.2f;
        triggerCameraShake(strength, 0.12f);
    }

    constexpr float BLAST_RADIUS = 31.0f;

    if (player.distance(bomb.position) <= BLAST_RADIUS && invincibleTimer <= 0.0f) {
        damagePlayer(2, audio);
    }

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& enemy = enemies[i];
        if (!enemy.active) continue;
        if (enemy.position.distance(bomb.position) > BLAST_RADIUS + enemy.radius) continue;

        enemy.hp -= 6;
        if (enemy.hp <= 0) {
            const EnemyType type = enemy.type;
            const Vector2 p = enemy.position;
            enemy.active = false;
            kills++;
            spawnDeathParticles(
                p,
                type == ENEMY_BOSS
                    ? Graphics::rgb565(220, 74, 52)
                    : Graphics::rgb565(194, 92, 58),
                type == ENEMY_BOSS ? 10 : 4);
            score += type == ENEMY_BOSS ? 1800 : 120;
            spawnOrb(p, type == ENEMY_BOSS ? 4 : 1);
        }
    }

    if (mission.active && mission.type == MISSION_DESTROY && missionTarget.active &&
        missionTarget.position.distance(bomb.position) <= BLAST_RADIUS + missionTarget.radius) {
        missionTarget.hp -= 5;
        if (missionTarget.hp <= 0) {
            const Vector2 reward = missionTarget.position;
            triggerDestroyExplosion(reward);
            missionTarget.active = false;
            completeMission(reward, audio, false);
        }
    }

    if (mission.active && mission.type == MISSION_INTERCEPT && supplyVehicle.active &&
        supplyVehicle.position.distance(bomb.position) <= BLAST_RADIUS + 8.0f) {
        supplyVehicle.hp -= 5;
        if (supplyVehicle.hp <= 0) {
            const Vector2 reward = supplyVehicle.position;
            spawnDeathParticles(
                reward, Graphics::rgb565(220, 196, 80), 6);
            supplyVehicle.active = false;
            completeMission(reward, audio, true);
        }
    }
}

void IronSwarm::clearMissionObjects()
{
    missionTarget.active = false;
    escortVehicle.active = false;
    escortVehicle.joined = false;
    supplyVehicle.active = false;
}

void IronSwarm::startMission(Audio& audio)
{
    clearMissionObjects();

    MissionType nextType = MISSION_HOLD_BRIDGE;
    do {
        nextType = static_cast<MissionType>(Math::random(1, MISSION_COUNT));
    } while (nextType == lastMissionType);

    lastMissionType = nextType;
    mission.type = nextType;
    missionNoticeTimer = 1.45f;
    mission.progress = 0.0f;
    mission.stage = 0;
    mission.active = true;

    switch (nextType) {
        case MISSION_HOLD_BRIDGE:
            startHoldBridgeMission();
            break;
        case MISSION_BREAKTHROUGH:
            startBreakthroughMission();
            break;
        case MISSION_DESTROY:
            startDestroyMission();
            break;
        case MISSION_ESCORT:
            startEscortMission();
            break;
        case MISSION_INTERCEPT:
            startInterceptMission();
            break;
        default:
            break;
    }

    audio.playSE(&Audio::SE::NO_11, 0.65f);
}

void IronSwarm::startHoldBridgeMission()
{
    mission.targetBridge = Math::random(0, 2);
    mission.targetPosition = getBridgeCenter(mission.targetBridge);
    mission.timeLeft = 40.0f;
    mission.progress = 0.0f;
}

void IronSwarm::startBreakthroughMission()
{
    const bool playerWest = player.x < RIVER_X;
    mission.targetPosition = Vector2(
        playerWest ? 1260.0f : 340.0f,
        Math::randomFloat(170.0f, 1030.0f));
    mission.timeLeft = 42.0f;
}

void IronSwarm::startDestroyMission()
{
    const bool playerWest = player.x < RIVER_X;
    missionTarget.position = Vector2(
        playerWest ? Math::randomFloat(1080.0f, 1390.0f)
                   : Math::randomFloat(210.0f, 520.0f),
        Math::randomFloat(160.0f, 1040.0f));
    resolveActorTerrain(missionTarget.position, 22.0f);

    missionTarget.radius = 22.0f;
    missionTarget.maxHp = 20 + level * 4;
    missionTarget.hp = missionTarget.maxHp;
    missionTarget.active = true;

    mission.targetPosition = missionTarget.position;
    mission.timeLeft = 56.0f;

    // A small guard group makes DESTROY feel like an enemy position,
    // without increasing the normal battlefield population too much.
    for (int guard = 0; guard < 2; ++guard) {
        int slot = -1;
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (!enemies[i].active) {
                slot = i;
                break;
            }
        }
        if (slot < 0) break;

        Enemy& enemy = enemies[slot];

        const float angle =
            guard == 0 ? Math::degToRad(35.0f) : Math::degToRad(215.0f);
        enemy.position =
            missionTarget.position +
            Vector2(Math::cos(angle), Math::sin(angle)) * 48.0f;

        enemy.type =
            (guard == 1 && level >= 5) ? ENEMY_HEAVY : ENEMY_GUNNER;

        if (enemy.type == ENEMY_HEAVY) {
            enemy.radius = 10.0f;
            enemy.speed = 20.0f;
            enemy.hp = 7 + level / 4;
            enemy.shootTimer = 1.8f;
        } else {
            enemy.radius = 7.0f;
            enemy.speed = 24.0f;
            enemy.hp = 2 + level / 6;
            enemy.shootTimer = 1.4f + guard * 0.35f;
        }

        resolveActorTerrain(enemy.position, enemy.radius);
        enemy.active = true;
    }
}

void IronSwarm::startEscortMission()
{
    const bool playerWest = player.x < RIVER_X;
    escortVehicle.position = player + Vector2(
        playerWest ? 180.0f : -180.0f,
        Math::randomFloat(-120.0f, 120.0f));
    escortVehicle.position.x = Math::clamp(
        escortVehicle.position.x, 80.0f, static_cast<float>(WORLD_W) - 80.0f);
    escortVehicle.position.y = Math::clamp(
        escortVehicle.position.y, 100.0f, static_cast<float>(WORLD_H) - 100.0f);
    resolveActorTerrain(escortVehicle.position, 8.0f);

    escortVehicle.speed = 44.0f;
    escortVehicle.joined = false;
    escortVehicle.active = true;

    mission.targetPosition = Vector2(
        playerWest ? 1300.0f : 300.0f,
        Math::randomFloat(180.0f, 1020.0f));
    mission.targetPosition2 = escortVehicle.position;
    mission.timeLeft = 68.0f;
    mission.stage = 0;
}

void IronSwarm::startInterceptMission()
{
    const bool playerWest = player.x < RIVER_X;

    // Start inside the current view, but far enough away to make the chase readable.
    bool placed = false;
    for (int attempt = 0; attempt < 16 && !placed; ++attempt) {
        const float angle = Math::randomFloat(0.0f, Math::TWO_PI);
        const float distance = Math::randomFloat(92.0f, 132.0f);
        Vector2 candidate =
            player + Vector2(Math::cos(angle), Math::sin(angle)) * distance;

        // Keep the vehicle on the player's side of the river at mission start.
        if (playerWest) {
            candidate.x = Math::clamp(candidate.x, 70.0f, RIVER_X - 24.0f);
        } else {
            candidate.x = Math::clamp(
                candidate.x, RIVER_X + RIVER_W + 24.0f,
                static_cast<float>(WORLD_W) - 70.0f);
        }
        candidate.y = Math::clamp(candidate.y, 70.0f, static_cast<float>(WORLD_H) - 70.0f);

        if (!isActorPositionBlocked(candidate, 8.0f) &&
            isWorldPointVisible(candidate, -14.0f)) {
            supplyVehicle.position = candidate;
            placed = true;
        }
    }

    if (!placed) {
        supplyVehicle.position = player + Vector2(playerWest ? 100.0f : -100.0f, 0.0f);
        resolveActorTerrain(supplyVehicle.position, 8.0f);
    }

    // Escape across a long route on the same side of the river.
    supplyVehicle.escapeTarget = Vector2(
        playerWest ? 80.0f : static_cast<float>(WORLD_W) - 80.0f,
        supplyVehicle.position.y < WORLD_H * 0.5f
            ? static_cast<float>(WORLD_H) - 100.0f
            : 100.0f);

    supplyVehicle.speed = 50.0f;
    supplyVehicle.maxHp = 7 + level;
    supplyVehicle.hp = supplyVehicle.maxHp;
    supplyVehicle.active = true;

    mission.targetPosition = supplyVehicle.position;
    mission.timeLeft = 38.0f;
}

void IronSwarm::failMission(Audio& audio)
{
    clearMissionObjects();
    mission.active = false;
    mission.type = MISSION_NONE;
    mission.progress = 0.0f;
    mission.stage = 0;
    missionTimer = 22.0f;
    audio.playSE(&Audio::SE::NO_13, 0.6f);
}

void IronSwarm::completeMission(
    const Vector2& rewardPosition, Audio& audio, bool dropRepair)
{
    clearMissionObjects();

    score += 1200;

    spawnMissionReward(rewardPosition);

    if (dropRepair) {
        spawnRepair(rewardPosition + Vector2(0.0f, 18.0f));
    }

    mission.active = false;
    mission.type = MISSION_NONE;
    mission.progress = 0.0f;
    mission.stage = 0;
    missionTimer = 20.0f;
    audio.playSE(&Audio::SE::NO_8, 0.85f);
}

void IronSwarm::updateMission(Audio& audio, float deltaSec)
{
    if (!mission.active) {
        missionTimer -= deltaSec;
        if (missionTimer <= 0.0f) {
            startMission(audio);
        }
        return;
    }

    mission.timeLeft -= deltaSec;
    if (mission.timeLeft <= 0.0f) {
        failMission(audio);
        return;
    }

    switch (mission.type) {
        case MISSION_HOLD_BRIDGE:
            if (player.distance(getBridgeCenter(mission.targetBridge)) <= 58.0f) {
                mission.progress += deltaSec;
                if (mission.progress >= 12.0f) {
                    completeMission(getBridgeCenter(mission.targetBridge), audio, false);
                }
            }
            break;

        case MISSION_BREAKTHROUGH:
            if (player.distance(mission.targetPosition) <= 28.0f) {
                completeMission(mission.targetPosition, audio, false);
            }
            break;

        case MISSION_DESTROY:
            if (!missionTarget.active) {
                completeMission(mission.targetPosition, audio, false);
            }
            break;

        case MISSION_ESCORT:
            updateEscortVehicle(deltaSec);
            if (!escortVehicle.joined) {
                if (player.distance(escortVehicle.position) <= 18.0f) {
                    escortVehicle.joined = true;
                    mission.stage = 1;
                    audio.playSE(&Audio::SE::NO_3, 0.65f);
                }
            } else if (escortVehicle.position.distance(mission.targetPosition) <= 28.0f) {
                completeMission(mission.targetPosition, audio, false);
            }
            break;

        case MISSION_INTERCEPT:
            updateSupplyVehicle(deltaSec);
            if (supplyVehicle.active &&
                supplyVehicle.position.distance(supplyVehicle.escapeTarget) <= 18.0f) {
                failMission(audio);
            }
            break;

        default:
            break;
    }
}

void IronSwarm::updateEscortVehicle(float deltaSec)
{
    if (!escortVehicle.active || !escortVehicle.joined) return;

    constexpr float FOLLOW_DISTANCE = 24.0f;
    constexpr float MAX_ESCORT_DISTANCE = 105.0f;

    // If the player leaves the escort too far behind, it waits in place.
    // The player has to come back and resume the escort.
    const float playerDistance =
        escortVehicle.position.distance(player);
    if (playerDistance > MAX_ESCORT_DISTANCE) {
        return;
    }

    const Vector2 destination = player;
    const Vector2 target =
        getMoveTargetAcrossRiver(escortVehicle.position, destination);
    const Vector2 toTarget = target - escortVehicle.position;
    const float distance = toTarget.length();

    if (distance > FOLLOW_DISTANCE) {
        escortVehicle.position +=
            toTarget.normalized() * (escortVehicle.speed * deltaSec);
        resolveActorTerrain(escortVehicle.position, 8.0f);
    }
}

void IronSwarm::updateSupplyVehicle(float deltaSec)
{
    if (!supplyVehicle.active) return;

    const Vector2 toEscape =
        supplyVehicle.escapeTarget - supplyVehicle.position;

    if (toEscape.length() > 0.001f) {
        const float baseAngle = Math::angle(toEscape.x, toEscape.y);
        const float probeAngles[] = {
            0.0f,
            Math::degToRad(28.0f),
            -Math::degToRad(28.0f),
            Math::degToRad(55.0f),
            -Math::degToRad(55.0f),
            Math::degToRad(85.0f),
            -Math::degToRad(85.0f)
        };

        Vector2 moveDirection = toEscape.normalized();

        for (float offset : probeAngles) {
            const float angle = baseAngle + offset;
            const Vector2 candidateDirection(
                Math::cos(angle), Math::sin(angle));
            const Vector2 probe =
                supplyVehicle.position + candidateDirection * 24.0f;

            if (!isActorPositionBlocked(probe, 8.0f)) {
                moveDirection = candidateDirection;
                break;
            }
        }

        const Vector2 next =
            supplyVehicle.position +
            moveDirection * (supplyVehicle.speed * deltaSec);

        if (!isActorPositionBlocked(next, 8.0f)) {
            supplyVehicle.position = next;
        }
    }

    mission.targetPosition = supplyVehicle.position;
}

bool IronSwarm::handleMissionBulletHit(Bullet& bullet, Audio& audio)
{
    if (!bullet.active || !mission.active) return false;

    if (mission.type == MISSION_DESTROY && missionTarget.active &&
        Collision::circleCircle(
            bullet.position.x, bullet.position.y, BULLET_RADIUS,
            missionTarget.position.x, missionTarget.position.y,
            missionTarget.radius)) {
        missionTarget.hp -= bullet.damage;
        bullet.active = false;

        if (missionTarget.hp <= 0) {
            const Vector2 reward = missionTarget.position;
            triggerDestroyExplosion(reward);
            missionTarget.active = false;
            completeMission(reward, audio, false);
        }
        return true;
    }

    if (mission.type == MISSION_INTERCEPT && supplyVehicle.active &&
        Collision::circleCircle(
            bullet.position.x, bullet.position.y, BULLET_RADIUS,
            supplyVehicle.position.x, supplyVehicle.position.y, 8.0f)) {
        supplyVehicle.hp -= bullet.damage;
        bullet.active = false;

        if (supplyVehicle.hp <= 0) {
            const Vector2 reward = supplyVehicle.position;
            spawnDeathParticles(
                reward, Graphics::rgb565(220, 196, 80), 6);
            supplyVehicle.active = false;
            completeMission(reward, audio, true);
        }
        return true;
    }

    return false;
}

const char* IronSwarm::getMissionName() const
{
    switch (mission.type) {
        case MISSION_HOLD_BRIDGE:  return "HOLD THE BRIDGE";
        case MISSION_BREAKTHROUGH: return "BREAKTHROUGH";
        case MISSION_DESTROY:      return "DESTROY";
        case MISSION_ESCORT:       return "ESCORT";
        case MISSION_INTERCEPT:    return "INTERCEPT";
        default:                   return "";
    }
}

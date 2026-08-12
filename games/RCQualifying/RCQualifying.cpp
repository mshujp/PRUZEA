#include "RCQualifying.h"

#include <cstdio>

using namespace PRUZEA;

namespace
{
using P = RCQualifying::Point;

static const P SPEEDWAY_POINTS[] = {
    {160, 199}, {225, 199}, {275, 190}, {292, 154},
    {292, 88},  {275, 52},  {225, 43},  {95, 43},
    {45, 52},   {28, 88},   {28, 154},  {45, 190}, {95, 199}
};

// Triangle-shaped oval: long front straight, two long diagonal straights,
// and a rounded upper apex. The control points intentionally preserve
// visible straight sections instead of producing a generic smooth oval.
static const P TRI_OVAL_POINTS[] = {
    {160,198}, {220,198}, {254,197}, {276,188},
    {287,174}, {289,159}, {284,143}, {270,125},
    {239,99},  {207,73},  {187,58},  {174,52},
    {160,50},  {146,52},  {133,59},  {121,73},
    {101,101}, {85,124},  {72,144},  {64,161},
    {63,175},  {69,187},  {82,195},  {104,198}
};

// D-shaped oval: a nearly straight back stretch and a bowed front side.
static const P D_SHAPE_POINTS[] = {
    {160, 198}, {204, 188}, {244, 174}, {270, 159},
    {285, 137}, {289, 108}, {286, 82},  {274, 64},
    {258, 55},  {235, 52},  {198, 52},  {160, 52},
    {122, 52},  {84, 52},   {62, 56},   {47, 66},
    {37, 82},   {32, 104},  {34, 128},  {44, 148},
    {61, 164},  {89, 178},  {124, 190}
};

// Egg-shaped oval: a tight left end, long rising back stretch, and a large
// rounded right end. The front straight remains flat and easy to recognize.
static const P EGG_OVAL_POINTS[] = {
    {160, 198}, {208, 198}, {244, 197}, {266, 188},
    {281, 171}, {288, 148}, {289, 121}, {286, 94},
    {278, 73},  {266, 59},  {252, 53},  {238, 53},
    {219, 59},  {188, 71},  {151, 85},  {113, 99},
    {78, 112},  {52, 124},  {38, 138},  {32, 154},
    {34, 171},  {44, 187},  {62, 196},  {88, 198}
};

// Top-down car sprite, 16x12, nose pointing toward +X (matches carAngle == 0.0f).
// SpriteOptions::transparent uses the default transparentColor (MAGENTA), so every
// MAGENTA pixel below is drawn as transparent background.
static const uint16_t CAR_SPRITE_W = 16;
static const uint16_t CAR_SPRITE_H = 12;
static const uint16_t CAR_SPRITE_BITMAP[] = {
    0xF81F, 0xF81F, 0x0000, 0x0000, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0x0000, 0x0000, 0xF81F, 0xF81F, 0xF81F,
    0xF81F, 0xF81F, 0x0000, 0x0000, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0x0000, 0x0000, 0xF81F, 0xF81F, 0xF81F,
    0xF81F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xF81F,
    0xF81F, 0xFFFF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0xFFFF, 0xF81F,
    0xF81F, 0xFFFF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x4208, 0x4208, 0x4208, 0x4208, 0x07FF, 0x07FF, 0xFFFF,
    0xF81F, 0xFFFF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x4208, 0x4208, 0x4208, 0x4208, 0x07FF, 0x07FF, 0xFFE0,
    0xF81F, 0xFFFF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x4208, 0x4208, 0x4208, 0x4208, 0x07FF, 0x07FF, 0xFFE0,
    0xF81F, 0xFFFF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x4208, 0x4208, 0x4208, 0x4208, 0x07FF, 0x07FF, 0xFFFF,
    0xF81F, 0xFFFF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0x07FF, 0xFFFF, 0xF81F,
    0xF81F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xF81F,
    0xF81F, 0xF81F, 0x0000, 0x0000, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0x0000, 0x0000, 0xF81F, 0xF81F, 0xF81F,
    0xF81F, 0xF81F, 0x0000, 0x0000, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0xF81F, 0x0000, 0x0000, 0xF81F, 0xF81F, 0xF81F,
};
}

const char* RCQualifying::getId() const { return "rc_qualifying"; }
const char* RCQualifying::getName() const { return "RC OVAL"; }
const char* RCQualifying::getMenuName() const { return "RC OVAL"; }
uint16_t RCQualifying::getLogicalScreenWidth() const { return Display::ILI9341_SCREEN_W; }
uint16_t RCQualifying::getLogicalScreenHeight() const { return Display::ILI9341_SCREEN_H; }
uint16_t RCQualifying::getTargetScreenWidth() const { return Display::ILI9341_SCREEN_W; }
uint16_t RCQualifying::getTargetScreenHeight() const { return Display::ILI9341_SCREEN_H; }

void RCQualifying::onInit(Storage& storage)
{
    saveData.clear();
    if (storage.isAvailable()) saveData.load(storage, getId(), "save.ini");
    for (uint8_t i = 0; i < COURSE_COUNT; ++i)
    {
        bestTotalMsec[i] = saveData.getUInt32(getBestKey(static_cast<Course>(i)), 0);
    }
    saveDirty = false;
    resetTitle();
}

void RCQualifying::resetTitle()
{
    mode = MODE_TITLE;
    timingActive = false;
    completedLaps = 0;
    checkpoint = 0;
    finishTotalMsec = 0;
    for (uint8_t i = 0; i < QUALIFYING_LAPS; ++i) lapTimes[i] = 0;
    resetCar();
    modeStartMsec = Platform::getMsec();
    dirty = true;
}

void RCQualifying::resetCar()
{
    const float startProgress = 0.015f;
    const Point p = getCoursePoint(startProgress);
    const Point tangent = getCourseTangent(startProgress);
    carX = p.x;
    carY = p.y;
    carAngle = Math::angle(tangent.x, tangent.y);
    speed = 0.0f;
    steering = 0.0f;
    previousProgress = startProgress;
}

void RCQualifying::startRun()
{
    resetCar();
    timingActive = false;
    completedLaps = 0;
    checkpoint = 0;
    finishTotalMsec = 0;
    for (uint8_t i = 0; i < QUALIFYING_LAPS; ++i) lapTimes[i] = 0;
    mode = MODE_RUNNING;
    modeStartMsec = Platform::getMsec();
    dirty = true;
}

Game::GameState RCQualifying::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    analogAvailable = input.hasAnalogSticks();
    const uint32_t now = Platform::getMsec();

    if (mode == MODE_TITLE)
    {
        bool changed = false;
        if (input.justPressed(Input::LEFT))
        {
            selectedCourse = static_cast<Course>((selectedCourse + COURSE_COUNT - 1) % COURSE_COUNT);
            changed = true;
        }
        if (input.justPressed(Input::RIGHT))
        {
            selectedCourse = static_cast<Course>((selectedCourse + 1) % COURSE_COUNT);
            changed = true;
        }
        if (changed)
        {
            resetCar();
            audio.playSE(&Audio::SE::NO_1, 0.35f);
            dirty = true;
        }
        if (input.justPressed(Input::A) || input.justPressed(Input::START))
        {
            audio.playSE(&Audio::SE::NO_8, 0.60f);
            startRun();
        }
        if ((now / 500u) != ((now - static_cast<uint32_t>(deltaSec * 1000.0f)) / 500u)) dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_RESULT)
    {
        if (input.justPressed(Input::A) || input.justPressed(Input::START))
        {
            audio.playSE(&Audio::SE::NO_1, 0.45f);
            startRun();
        }
        else if (input.justPressed(Input::B)) resetTitle();

        if (!Platform::elapsed(now, modeStartMsec, 250u)) dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_PAUSED)
    {
        if (input.justPressed(Input::START))
        {
            const uint32_t pausedMsec = now - pauseStartMsec;
            if (timingActive)
            {
                runStartMsec += pausedMsec;
                lapStartMsec += pausedMsec;
            }
            mode = MODE_RUNNING;
            audio.playSE(&Audio::SE::NO_1, 0.4f);
            dirty = true;
        }
        else if (input.justPressed(Input::B)) resetTitle();
        return Game::GameState::RUNNING;
    }

    if (input.justPressed(Input::START))
    {
        pauseStartMsec = now;
        mode = MODE_PAUSED;
        audio.stopMusic();
        audio.playSE(&Audio::SE::NO_1, 0.4f);
        dirty = true;
        return Game::GameState::RUNNING;
    }

    updateDriving(input, audio, storage, deltaSec);
    dirty = true;
    return Game::GameState::RUNNING;
}

float RCQualifying::readSteering(const Input& input) const
{
    float value = 0.0f;
    if (input.hasAnalogSticks()) value = static_cast<float>(input.axis(Input::LEFT_X)) / 1000.0f;
    if (input.pressed(Input::LEFT)) value = -1.0f;
    if (input.pressed(Input::RIGHT)) value = 1.0f;
    return Math::clamp(value, -1.0f, 1.0f);
}

void RCQualifying::updateDriving(Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    float throttleAmount = 0.0f;
    float brakeAmount = 0.0f;
    if (input.hasAnalogSticks())
    {
        const float rightY = static_cast<float>(input.axis(Input::RIGHT_Y)) / 1000.0f;
        throttleAmount = Math::clamp(-rightY, 0.0f, 1.0f);
        brakeAmount = Math::clamp(rightY, 0.0f, 1.0f);
    }
    if (input.pressed(Input::Y) || input.pressed(Input::UP)) throttleAmount = 1.0f;
    if (input.pressed(Input::B) || input.pressed(Input::DOWN)) brakeAmount = 1.0f;

    const float targetSteering = readSteering(input);
    steering = Math::moveTowards(steering, targetSteering, STEER_RESPONSE * deltaSec);

    if (throttleAmount > 0.0f && brakeAmount <= 0.0f)
    {
        if (speed < 0.0f) speed = Math::moveTowards(speed, 0.0f, BRAKE_FORCE * throttleAmount * deltaSec);
        else speed = Math::moveTowards(speed, MAX_FORWARD_SPEED * throttleAmount, ACCELERATION * throttleAmount * deltaSec);
    }
    else if (brakeAmount > 0.0f && throttleAmount <= 0.0f)
    {
        if (speed > 4.0f) speed = Math::moveTowards(speed, 0.0f, BRAKE_FORCE * brakeAmount * deltaSec);
        else speed = Math::moveTowards(speed, -MAX_REVERSE_SPEED * brakeAmount, REVERSE_ACCELERATION * brakeAmount * deltaSec);
    }
    else speed = Math::moveTowards(speed, 0.0f, COAST_DRAG * deltaSec);

    const float speedRatio = Math::clamp(Math::abs(speed) / MAX_FORWARD_SPEED, 0.0f, 1.0f);
    const float steerAmount = Math::abs(steering);
    const float steeringLoss = steerAmount * steerAmount * speedRatio * STEERING_DRAG_RATE;
    speed *= Math::clamp(1.0f - steeringLoss * deltaSec, 0.0f, 1.0f);

    const float direction = speed >= 0.0f ? 1.0f : -1.0f;
    const float turnStrength = 0.18f + speedRatio * 0.82f;
    carAngle += steering * TURN_RATE * turnStrength * direction * deltaSec;

    const float moveX = Math::cos(carAngle) * speed * deltaSec;
    const float moveY = Math::sin(carAngle) * speed * deltaSec;
    const float nextX = carX + moveX;
    const float nextY = carY + moveY;

    if (isOnTrack(nextX, nextY))
    {
        carX = nextX;
        carY = nextY;
    }
    else
    {
        const bool canMoveX = isOnTrack(nextX, carY);
        const bool canMoveY = isOnTrack(carX, nextY);
        if (canMoveX) carX = nextX;
        if (canMoveY) carY = nextY;
        speed *= (canMoveX || canMoveY) ? 0.72f : 0.38f;
    }

    updateLapProgress(audio, storage);
}

void RCQualifying::updateLapProgress(Audio& audio, Storage& storage)
{
    const float progress = getTrackProgress(carX, carY);

    if (checkpoint == 0 && progress >= 0.18f && progress < 0.40f) checkpoint = 1;
    else if (checkpoint == 1 && progress >= 0.43f && progress < 0.65f) checkpoint = 2;
    else if (checkpoint == 2 && progress >= 0.68f && progress < 0.90f) checkpoint = 3;

    const bool crossedLine = checkpoint == 3 && previousProgress > 0.82f && progress < 0.18f;
    previousProgress = progress;
    if (!crossedLine) return;

    const uint32_t now = Platform::getMsec();
    checkpoint = 0;

    if (!timingActive)
    {
        timingActive = true;
        runStartMsec = now;
        lapStartMsec = now;
        audio.playSE(&Audio::SE::NO_11, 0.55f);
        return;
    }

    if (completedLaps < QUALIFYING_LAPS)
    {
        lapTimes[completedLaps] = now - lapStartMsec;
        ++completedLaps;
        lapStartMsec = now;
        audio.playSE(&Audio::SE::NO_3, 0.45f);
    }
    if (completedLaps >= QUALIFYING_LAPS) finishRun(audio, storage);
}

void RCQualifying::finishRun(Audio& audio, Storage& storage)
{
    finishTotalMsec = Platform::getMsec() - runStartMsec;
    uint32_t& best = bestTotalMsec[selectedCourse];
    if (best == 0 || finishTotalMsec < best)
    {
        best = finishTotalMsec;
        saveDirty = true;
        saveBests(storage);
    }
    speed = 0.0f;
    mode = MODE_RESULT;
    modeStartMsec = Platform::getMsec();
    audio.playSE(&Audio::SE::NO_8, 0.75f);
    dirty = true;
}

void RCQualifying::saveBests(Storage& storage)
{
    if (!saveDirty || !storage.isAvailable()) return;
    bool ok = true;
    for (uint8_t i = 0; i < COURSE_COUNT; ++i)
    {
        ok = saveData.setUInt32(getBestKey(static_cast<Course>(i)), bestTotalMsec[i]) && ok;
    }
    if (ok && saveData.save(storage, getId(), "save.ini")) saveDirty = false;
}

float RCQualifying::distanceToSegmentSquared(float px, float py, const Point& a, const Point& b)
{
    const float vx = b.x - a.x;
    const float vy = b.y - a.y;
    const float wx = px - a.x;
    const float wy = py - a.y;
    const float length2 = vx * vx + vy * vy;
    float t = length2 > 0.0001f ? (wx * vx + wy * vy) / length2 : 0.0f;
    t = Math::clamp(t, 0.0f, 1.0f);
    const float dx = px - (a.x + vx * t);
    const float dy = py - (a.y + vy * t);
    return dx * dx + dy * dy;
}

RCQualifying::Point RCQualifying::catmullRom(const Point& p0, const Point& p1, const Point& p2, const Point& p3, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    Point out;
    out.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t +
                    (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                    (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    out.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t +
                    (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                    (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
    return out;
}

const RCQualifying::Point* RCQualifying::getCoursePoints(uint8_t& count) const
{
    switch (selectedCourse)
    {
        case COURSE_TRI_OVAL: count = sizeof(TRI_OVAL_POINTS) / sizeof(TRI_OVAL_POINTS[0]); return TRI_OVAL_POINTS;
        case COURSE_D_SHAPE: count = sizeof(D_SHAPE_POINTS) / sizeof(D_SHAPE_POINTS[0]); return D_SHAPE_POINTS;
        case COURSE_EGG_OVAL: count = sizeof(EGG_OVAL_POINTS) / sizeof(EGG_OVAL_POINTS[0]); return EGG_OVAL_POINTS;
        default: count = sizeof(SPEEDWAY_POINTS) / sizeof(SPEEDWAY_POINTS[0]); return SPEEDWAY_POINTS;
    }
}

RCQualifying::Point RCQualifying::getCoursePoint(float progress) const
{
    uint8_t count = 0;
    const Point* points = getCoursePoints(count);
    progress = Math::wrap(progress, 0.0f, 1.0f);
    const float scaled = progress * count;
    const int i1 = static_cast<int>(scaled) % count;
    const float local = scaled - static_cast<float>(static_cast<int>(scaled));
    const int i0 = (i1 + count - 1) % count;
    const int i2 = (i1 + 1) % count;
    const int i3 = (i1 + 2) % count;
    return catmullRom(points[i0], points[i1], points[i2], points[i3], local);
}

RCQualifying::Point RCQualifying::getCourseTangent(float progress) const
{
    const Point a = getCoursePoint(progress - 0.0025f);
    const Point b = getCoursePoint(progress + 0.0025f);
    return { b.x - a.x, b.y - a.y };
}

float RCQualifying::getTrackProgress(float x, float y) const
{
    float bestDistance = 1000000000.0f;
    float bestProgress = 0.0f;
    Point a = getCoursePoint(0.0f);
    for (uint8_t i = 0; i < TRACK_SAMPLES; ++i)
    {
        const float nextProgress = static_cast<float>(i + 1) / TRACK_SAMPLES;
        const Point b = getCoursePoint(nextProgress);
        const float distance = distanceToSegmentSquared(x, y, a, b);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestProgress = (static_cast<float>(i) + 0.5f) / TRACK_SAMPLES;
        }
        a = b;
    }
    return Math::wrap(bestProgress, 0.0f, 1.0f);
}

bool RCQualifying::insideRoundedRect(float px, float py, float x, float y, float w, float h, float radius)
{
    const float innerLeft = x + radius;
    const float innerRight = x + w - radius;
    const float innerTop = y + radius;
    const float innerBottom = y + h - radius;
    if (px >= innerLeft && px <= innerRight && py >= y && py <= y + h) return true;
    if (py >= innerTop && py <= innerBottom && px >= x && px <= x + w) return true;
    const float cx = px < innerLeft ? innerLeft : innerRight;
    const float cy = py < innerTop ? innerTop : innerBottom;
    return Math::distanceSquared(px, py, cx, cy) <= radius * radius;
}

bool RCQualifying::isOnTrack(float x, float y) const
{
    // Keep the original v3 four-turn speedway geometry exactly as it was.
    if (selectedCourse == COURSE_SPEEDWAY)
    {
        const bool insideOuter = insideRoundedRect(x, y, 14.0f, 34.0f, 292.0f, 182.0f, 52.0f);
        const bool insideInfield = insideRoundedRect(x, y, 58.0f, 76.0f, 204.0f, 98.0f, 18.0f);
        return insideOuter && !insideInfield;
    }

    const float limit2 = TRACK_HALF_WIDTH * TRACK_HALF_WIDTH;
    Point a = getCoursePoint(0.0f);
    for (uint8_t i = 0; i < TRACK_SAMPLES; ++i)
    {
        const Point b = getCoursePoint(static_cast<float>(i + 1) / TRACK_SAMPLES);
        if (distanceToSegmentSquared(x, y, a, b) <= limit2) return true;
        a = b;
    }
    return false;
}

const char* RCQualifying::getCourseName() const
{
    switch (selectedCourse)
    {
        case COURSE_TRI_OVAL: return "TRI-OVAL";
        case COURSE_D_SHAPE: return "D-SHAPE";
        case COURSE_EGG_OVAL: return "EGG OVAL";
        default: return "SPEEDWAY";
    }
}

const char* RCQualifying::getBestKey(Course course) const
{
    static const char* const keys[COURSE_COUNT] = {
        "BEST_SPEEDWAY", "BEST_TRI_OVAL", "BEST_D_SHAPE", "BEST_EGG_OVAL"
    };
    return keys[course];
}

bool RCQualifying::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty) return false;
    graphics.fillScreen(Graphics::rgb565(18, 72, 30));
    drawTrack(graphics);
    const uint32_t now = Platform::getMsec();
    if (mode == MODE_TITLE) drawTitle(graphics, now);
    else
    {
        drawCar(graphics);
        drawHud(graphics, now);
        if (mode == MODE_PAUSED) drawPause(graphics);
        else if (mode == MODE_RESULT) drawResult(graphics);
    }
    dirty = false;
    return true;
}

void RCQualifying::drawTrack(Graphics& g) const
{
    const Graphics::Color asphalt = Graphics::rgb565(72, 76, 82);
    const Graphics::Color apron = Graphics::rgb565(145, 148, 150);

    if (selectedCourse == COURSE_SPEEDWAY)
    {
        const Graphics::Color infield = Graphics::rgb565(24, 92, 38);
        g.fillRoundRect(10, 30, 300, 190, 56, apron);
        g.fillRoundRect(14, 34, 292, 182, 52, asphalt);
        g.fillRoundRect(54, 72, 212, 106, 22, Graphics::WHITE);
        g.fillRoundRect(58, 76, 204, 98, 18, infield);

        for (int16_t y = 182; y < 211; y += 6)
        {
            g.fillRect(157, y, 3, 3, Graphics::WHITE);
            g.fillRect(160, y + 3, 3, 3, Graphics::WHITE);
        }
        return;
    }

    Point points[TRACK_SAMPLES];
    for (uint8_t i = 0; i < TRACK_SAMPLES; ++i)
    {
        points[i] = getCoursePoint(static_cast<float>(i) / TRACK_SAMPLES);
        g.fillCircle(static_cast<int16_t>(points[i].x), static_cast<int16_t>(points[i].y), static_cast<uint16_t>(APRON_HALF_WIDTH), apron);
    }
    for (uint8_t i = 0; i < TRACK_SAMPLES; ++i)
    {
        g.fillCircle(static_cast<int16_t>(points[i].x), static_cast<int16_t>(points[i].y), static_cast<uint16_t>(TRACK_HALF_WIDTH), asphalt);
    }

    const Point line = getCoursePoint(0.0f);
    Point tangent = getCourseTangent(0.0f);
    Math::normalize(tangent.x, tangent.y);
    const float nx = -tangent.y;
    const float ny = tangent.x;
    for (int i = -4; i <= 4; ++i)
    {
        const float offset = static_cast<float>(i) * 4.0f;
        const int16_t x = static_cast<int16_t>(line.x + nx * offset);
        const int16_t y = static_cast<int16_t>(line.y + ny * offset);
        g.fillRect(x - 2, y - 2, 4, 4, (i & 1) ? Graphics::BLACK : Graphics::WHITE);
    }

    g.drawString(getCourseName(), 160, 112, Graphics::WHITE, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("4-LAP QUALIFYING", 160, 134, Graphics::LIGHTGRAY, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void RCQualifying::drawCar(Graphics& g) const
{
    Graphics::SpriteOptions options;
    options.angle = carAngle; // carAngle == 0 already faces +X, matching the sprite's nose direction.
    options.transparent = true; // Drops every MAGENTA pixel in CAR_SPRITE_BITMAP.

    const int16_t x = static_cast<int16_t>(carX) - static_cast<int16_t>(CAR_SPRITE_W / 2);
    const int16_t y = static_cast<int16_t>(carY) - static_cast<int16_t>(CAR_SPRITE_H / 2);
    g.drawSprite(CAR_SPRITE_BITMAP, x, y, CAR_SPRITE_W, CAR_SPRITE_H, options);
}

void RCQualifying::drawHud(Graphics& g, uint32_t now) const
{
    g.fillRect(0, 0, SCREEN_W, 27, Graphics::BLACK);
    char text[32];
    if (!timingActive) g.drawString("WARM UP", 8, 5, Graphics::YELLOW, Graphics::SIZE_18);
    else if (completedLaps >= QUALIFYING_LAPS) g.drawString("FINISH", 8, 6, Graphics::YELLOW, Graphics::SIZE_13);
    else
    {
        std::snprintf(text, sizeof(text), "LAP %u/%u", static_cast<unsigned>(completedLaps + 1), static_cast<unsigned>(QUALIFYING_LAPS));
        g.drawString(text, 8, 6, Graphics::WHITE, Graphics::SIZE_13);
        drawTime(g, now - lapStartMsec, 222, 6, Graphics::YELLOW, Graphics::SIZE_13, Graphics::HorizontalAlign::RIGHT);
    }
    const int speedDisplay = static_cast<int>(Math::abs(speed) * 2.0f + 0.5f);
    std::snprintf(text, sizeof(text), "SPD %03d", speedDisplay);
    g.drawString(text, 312, 6, Graphics::CYAN, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);
}

void RCQualifying::drawTitle(Graphics& g, uint32_t now) const
{
    g.fillRoundRect(38, 62, 244, 122, 10, Graphics::BLACK);
    g.drawRoundRect(38, 62, 244, 122, 10, 2, Graphics::CYAN);
    g.drawString("RC OVAL", 160, 73, Graphics::WHITE, Graphics::SIZE_25B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    g.drawString("<", 54, 108, Graphics::YELLOW, Graphics::SIZE_25B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString(getCourseName(), 160, 108, Graphics::YELLOW, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString(">", 266, 108, Graphics::YELLOW, Graphics::SIZE_25B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("LEFT/RIGHT: COURSE", 160, 129, Graphics::LIGHTGRAY, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    if (((now / 500u) & 1u) == 0u)
    {
        g.drawString("PRESS A OR START", 160, 146, Graphics::CYAN, Graphics::SIZE_13,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    }
    const uint32_t best = bestTotalMsec[selectedCourse];
    if (best > 0)
    {
        g.drawString("BEST", 84, 165, Graphics::WHITE, Graphics::SIZE_10);
        drawTime(g, best, 238, 164, Graphics::YELLOW, Graphics::SIZE_13, Graphics::HorizontalAlign::RIGHT);
    }
}

void RCQualifying::drawResult(Graphics& g) const
{
    g.fillRectAlpha(0, 0, SCREEN_W, SCREEN_H, 96, Graphics::BLACK);

    const uint32_t now = Platform::getMsec();
    const float t = Math::clamp(static_cast<float>(now - modeStartMsec) / 250.0f, 0.0f, 1.0f);
    const float eased = Tween::apply(t, Tween::Ease::EASE_OUT_BACK);
    const int16_t panelY = static_cast<int16_t>(Tween::lerp(-180.0f, 38.0f, eased));
    const int16_t offsetY = static_cast<int16_t>(panelY - 38);

    g.fillRoundRect(49, panelY, 222, 180, 10, Graphics::BLACK);
    g.drawRoundRect(49, panelY, 222, 180, 10, 2, Graphics::YELLOW);
    g.drawString("QUALIFYING COMPLETE", 160, static_cast<int16_t>(48 + offsetY), Graphics::YELLOW, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    g.drawString(getCourseName(), 160, static_cast<int16_t>(63 + offsetY), Graphics::LIGHTGRAY, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    for (uint8_t i = 0; i < QUALIFYING_LAPS; ++i)
    {
        char label[12];
        std::snprintf(label, sizeof(label), "LAP %u", static_cast<unsigned>(i + 1));
        const int16_t y = static_cast<int16_t>(80 + i * 19 + offsetY);
        g.drawString(label, 76, y, Graphics::WHITE, Graphics::SIZE_13);
        drawTime(g, lapTimes[i], 244, y, Graphics::CYAN, Graphics::SIZE_13, Graphics::HorizontalAlign::RIGHT);
    }
    g.drawLine(70, static_cast<int16_t>(160 + offsetY), 250, static_cast<int16_t>(160 + offsetY), Graphics::DARKGRAY);
    g.drawString("TOTAL", 76, static_cast<int16_t>(169 + offsetY), Graphics::WHITE, Graphics::SIZE_13);
    drawTime(g, finishTotalMsec, 244, static_cast<int16_t>(169 + offsetY), Graphics::YELLOW, Graphics::SIZE_13, Graphics::HorizontalAlign::RIGHT);
    g.drawString("A: AGAIN   B: TITLE", 160, static_cast<int16_t>(198 + offsetY), Graphics::LIGHTGRAY, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
}

void RCQualifying::drawPause(Graphics& g) const
{
    g.fillRectAlpha(0, 0, SCREEN_W, SCREEN_H, 96, Graphics::BLACK);
    g.fillRoundRect(84, 91, 152, 58, 8, Graphics::BLACK);
    g.drawRoundRect(84, 91, 152, 58, 8, 2, Graphics::WHITE);
    g.drawString("PAUSED", 160, 101, Graphics::WHITE, Graphics::SIZE_25B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    g.drawString("START: RESUME  B: QUIT", 160, 132, Graphics::LIGHTGRAY, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
}

void RCQualifying::drawTime(Graphics& g, uint32_t msec, int16_t x, int16_t y,
                            Graphics::Color color, Graphics::Font font,
                            Graphics::HorizontalAlign align) const
{
    const uint32_t minutes = msec / 60000u;
    const uint32_t seconds = (msec / 1000u) % 60u;
    const uint32_t millis = msec % 1000u;
    char text[24];
    std::snprintf(text, sizeof(text), "%02lu:%02lu.%03lu",
                  static_cast<unsigned long>(minutes),
                  static_cast<unsigned long>(seconds),
                  static_cast<unsigned long>(millis));
    g.drawString(text, x, y, color, font, align, Graphics::VerticalAlign::TOP);
}

void RCQualifying::onTerminate(Storage& storage)
{
    saveBests(storage);
}

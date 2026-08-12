#include "CollisionLab.h"

#include <cstdio>
#include <cstring>

using namespace PRUZEA;

const char* CollisionLab::getId() const { return "collisionlab"; }
const char* CollisionLab::getName() const { return "Collision Lab"; }
const char* CollisionLab::getMenuName() const { return "02 COLLISION LAB"; }
const char* CollisionLab::getMenuGroup() const { return "SAMPLES"; }

uint16_t CollisionLab::getLogicalScreenWidth() const { return 384; }
uint16_t CollisionLab::getLogicalScreenHeight() const { return 288; }
uint16_t CollisionLab::getTargetScreenWidth() const { return 320; }
uint16_t CollisionLab::getTargetScreenHeight() const { return 240; }

void CollisionLab::onInit(Storage& storage)
{
    saveAvailable = storage.isAvailable();
    saveStatusOk = false;
    bestTimeMsec = 0;
    clearCount = 0;

    if (saveAvailable && saveData.load(storage, getId(), SAVE_FILE))
    {
        bestTimeMsec = saveData.getUInt32("best_ms", 0);
        clearCount = saveData.getUInt32("clear_count", 0);
        saveStatusOk = true;
    }

    mode = MODE_TITLE;
    resetRun();
    dirty = true;
}

void CollisionLab::resetRun()
{
    playerX = 38.0f;
    playerY = 120.0f;
    pointX = playerX + PLAYER_RADIUS;
    pointY = playerY;
    completedTests = 0;
    runStartMsec = Platform::getMsec();
    completedMsec = 0;
    transitionStartMsec = 0;
    messageActive = false;
    shakeActive = false;
    viewportWasReset = true;
    message[0] = '\0';
}

uint8_t CollisionLab::getCompletedCount() const
{
    uint8_t count = 0;
    for (uint8_t bit = 1; bit != 0; bit <<= 1)
    {
        if ((completedTests & bit) != 0) ++count;
    }
    return count;
}

void CollisionLab::beginTest(uint8_t bit, const char* text, Audio& audio)
{
    if ((completedTests & bit) != 0) return;

    completedTests |= bit;
    std::snprintf(message, sizeof(message), "%s PASS", text);
    messageStartMsec = Platform::getMsec();
    messageActive = true;
    startShake();
    audio.playSE(&Audio::SE::NO_3, 0.65f);

    if (mode == MODE_PHASE1 && (completedTests & TEST_PHASE1_ALL) == TEST_PHASE1_ALL)
    {
        transitionStartMsec = Platform::getMsec();
        mode = MODE_TRANSITION;
        messageActive = false;
        audio.playSE(&Audio::SE::NO_8, 0.65f);
    }
    else if (mode == MODE_PHASE2 && completedTests == TEST_ALL)
    {
        mode = MODE_COMPLETE;
        completedMsec = Platform::getMsec();
        const uint32_t result = completedMsec - runStartMsec;
        if (bestTimeMsec == 0 || result < bestTimeMsec) bestTimeMsec = result;
        ++clearCount;
    }
}

void CollisionLab::beginPhase2(Audio& audio)
{
    mode = MODE_PHASE2;
    playerX = 34.0f;
    playerY = 120.0f;
    pointX = playerX + PLAYER_RADIUS;
    pointY = playerY;
    messageActive = false;
    shakeActive = false;
    audio.playSE(&Audio::SE::NO_8, 0.6f);
    dirty = true;
}

void CollisionLab::startShake()
{
    shakeStartMsec = Platform::getMsec();
    shakeActive = true;
    viewportWasReset = false;
}

void CollisionLab::updatePhase1Tests(Audio& audio)
{
    if (Collision::pointRect(pointX, pointY, 76.0f, 43.0f, 50.0f, 34.0f))
        beginTest(TEST_POINT_RECT, "POINT-RECT", audio);

    const float playerRectX = playerX - 7.0f;
    const float playerRectY = playerY - 7.0f;
    if (Collision::rectRect(playerRectX, playerRectY, 14.0f, 14.0f,
                            178.0f, 42.0f, 44.0f, 36.0f))
        beginTest(TEST_RECT_RECT, "RECT-RECT", audio);

    if (Collision::circleCircle(playerX, playerY, PLAYER_RADIUS,
                                104.0f, 178.0f, 17.0f))
        beginTest(TEST_CIRCLE_CIRCLE, "CIRCLE-CIRCLE", audio);

    if (Collision::circleRect(playerX, playerY, PLAYER_RADIUS,
                              242.0f, 151.0f, 48.0f, 30.0f))
        beginTest(TEST_CIRCLE_RECT, "CIRCLE-RECT", audio);
}

void CollisionLab::updatePhase2Tests(Audio& audio)
{
    // Point-Circle: the red point at the player's nose must enter the sensor.
    if (Collision::pointCircle(pointX, pointY, 78.0f, 72.0f, 18.0f))
        beginTest(TEST_POINT_CIRCLE, "POINT-CIRCLE", audio);

    // The laser is the segment from the player's nose to the right side.
    const float x1 = pointX;
    const float y1 = pointY;
    const float x2 = 310.0f;
    const float y2 = pointY;

    if (Collision::lineLine(x1, y1, x2, y2,
                            132.0f, 156.0f, 132.0f, 192.0f))
        beginTest(TEST_LINE_LINE, "LINE-LINE", audio);

    if (Collision::lineRect(x1, y1, x2, y2,
                            181.0f, 55.0f, 46.0f, 30.0f))
        beginTest(TEST_LINE_RECT, "LINE-RECT", audio);

    if (Collision::lineCircle(x1, y1, x2, y2,
                              263.0f, 174.0f, 18.0f))
        beginTest(TEST_LINE_CIRCLE, "LINE-CIRCLE", audio);
}

Game::GameState CollisionLab::onUpdate(Input& input, Audio& audio,
                                        Storage& storage, float deltaSec)
{
    const uint32_t now = Platform::getMsec();

    if (messageActive && Platform::elapsed(now, messageStartMsec, MESSAGE_MSEC))
    {
        messageActive = false;
        dirty = true;
    }

    if (shakeActive && Platform::elapsed(now, shakeStartMsec, SHAKE_MSEC))
    {
        shakeActive = false;
        dirty = true;
    }

    if (mode == MODE_TITLE)
    {
        if (input.justPressed(Input::A) || input.justPressed(Input::START))
        {
            resetRun();
            mode = MODE_PHASE1;
            audio.playSE(&Audio::SE::NO_8, 0.6f);
            dirty = true;
        }
        return Game::RUNNING;
    }

    if (mode == MODE_TRANSITION)
    {
        if (Platform::elapsed(now, transitionStartMsec, TRANSITION_MSEC))
            beginPhase2(audio);
        else
            dirty = true;
        return Game::RUNNING;
    }

    if (mode == MODE_COMPLETE)
    {
        if (Platform::elapsed(now, completedMsec, COMPLETE_WAIT_MSEC) &&
            (input.justPressed(Input::A) || input.justPressed(Input::START)))
        {
            saveProgress(storage);
            resetRun();
            mode = MODE_PHASE1;
            audio.playSE(&Audio::SE::NO_8, 0.6f);
            dirty = true;
        }
        return Game::RUNNING;
    }

    float dx = 0.0f;
    float dy = 0.0f;
    if (input.pressed(Input::LEFT))  dx -= 1.0f;
    if (input.pressed(Input::RIGHT)) dx += 1.0f;
    if (input.pressed(Input::UP))    dy -= 1.0f;
    if (input.pressed(Input::DOWN))  dy += 1.0f;

    if (dx != 0.0f || dy != 0.0f)
    {
        Math::normalize(dx, dy);
        playerX += dx * PLAYER_SPEED * deltaSec;
        playerY += dy * PLAYER_SPEED * deltaSec;
        playerX = Math::clamp(playerX, PLAYER_RADIUS, SCREEN_W - PLAYER_RADIUS);
        playerY = Math::clamp(playerY, 30.0f + PLAYER_RADIUS,
                              SCREEN_H - 16.0f - PLAYER_RADIUS);
        pointX = playerX + PLAYER_RADIUS;
        pointY = playerY;

        if (mode == MODE_PHASE1) updatePhase1Tests(audio);
        else if (mode == MODE_PHASE2) updatePhase2Tests(audio);
        dirty = true;
    }

    if (input.justPressed(Input::SELECT))
    {
        resetRun();
        mode = MODE_PHASE1;
        audio.playSE(&Audio::SE::NO_2, 0.5f);
        dirty = true;
    }

    static uint32_t lastPulseMsec = 0;
    if (Platform::elapsed(now, lastPulseMsec, PULSE_MSEC))
    {
        lastPulseMsec = now;
        dirty = true;
    }

    return Game::RUNNING;
}

void CollisionLab::saveProgress(Storage& storage)
{
    if (!saveAvailable) return;

    saveData.setUInt32("best_ms", bestTimeMsec);
    saveData.setUInt32("clear_count", clearCount);
    saveData.setBool("all_collision_ok", completedTests == TEST_ALL);
    saveStatusOk = saveData.save(storage, getId(), SAVE_FILE);
}

void CollisionLab::drawBackground(Graphics& g)
{
    const Graphics::Color deep = Graphics::rgb565(8, 28, 45);
    const Graphics::Color grid = Graphics::rgb565(18, 55, 72);
    g.fillScreen(deep);

    for (int16_t x = 0; x < 384; x += 24) g.drawLine(x, 0, x, 287, grid);
    for (int16_t y = 0; y < 288; y += 24) g.drawLine(0, y, 383, y, grid);

    g.fillRect(0, 0, 320, 29, Graphics::rgb565(5, 18, 30));
    g.drawLine(0, 29, 319, 29, Graphics::CYAN);
}

void CollisionLab::drawPhase1Stations(Graphics& g)
{
    const Graphics::Color idle = Graphics::rgb565(45, 85, 95);
    const Graphics::Color pass = Graphics::rgb565(50, 210, 150);

    Graphics::Color c = (completedTests & TEST_POINT_RECT) ? pass : idle;
    g.drawRect(76, 43, 50, 34, 2, c);
    g.drawString("P-R", 101, 60, c, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    c = (completedTests & TEST_RECT_RECT) ? pass : idle;
    g.fillRect(178, 42, 44, 36, c);
    g.drawString("R-R", 200, 60, Graphics::BLACK, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    c = (completedTests & TEST_CIRCLE_CIRCLE) ? pass : idle;
    g.drawCircle(104, 178, 17, c);
    g.drawCircle(104, 178, 12, c);
    g.drawString("C-C", 104, 178, c, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    c = (completedTests & TEST_CIRCLE_RECT) ? pass : idle;
    g.drawRect(242, 151, 48, 30, 2, c);
    g.drawString("C-R", 266, 166, c, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void CollisionLab::drawPhase2Stations(Graphics& g)
{
    const Graphics::Color idle = Graphics::rgb565(48, 76, 96);
    const Graphics::Color pass = Graphics::rgb565(50, 210, 150);

    Graphics::Color c = (completedTests & TEST_POINT_CIRCLE) ? pass : idle;
    g.drawCircle(78, 72, 18, c);
    g.drawCircle(78, 72, 13, c);
    g.drawString("P-C", 78, 72, c, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    c = (completedTests & TEST_LINE_LINE) ? pass : idle;
    g.drawLine(132, 156, 132, 192, c);
    g.drawLine(128, 156, 136, 156, c);
    g.drawLine(128, 192, 136, 192, c);
    g.drawString("L-L", 132, 201, c, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    c = (completedTests & TEST_LINE_RECT) ? pass : idle;
    g.drawRect(181, 55, 46, 30, 2, c);
    g.drawString("L-R", 204, 70, c, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    c = (completedTests & TEST_LINE_CIRCLE) ? pass : idle;
    g.drawCircle(263, 174, 18, c);
    g.drawString("L-C", 263, 174, c, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void CollisionLab::drawPlayer(Graphics& g, bool drawLaser)
{
    g.fillCircle(static_cast<int16_t>(playerX), static_cast<int16_t>(playerY),
                 static_cast<uint16_t>(PLAYER_RADIUS), Graphics::YELLOW);
    g.drawPixel(static_cast<int16_t>(pointX), static_cast<int16_t>(pointY), Graphics::RED);

    if (drawLaser)
    {
        const Graphics::Color beam = Graphics::rgb565(255, 80, 80);
        g.drawLine(static_cast<int16_t>(pointX), static_cast<int16_t>(pointY),
                   310, static_cast<int16_t>(pointY), beam);
        g.fillCircle(310, static_cast<int16_t>(pointY), 2, beam);
    }
}

void CollisionLab::drawHud(Graphics& g)
{
    char buf[64];
    const uint32_t now = Platform::getMsec();
    const uint32_t runMsec = (mode == MODE_COMPLETE) ?
                             (completedMsec - runStartMsec) : (now - runStartMsec);

    std::snprintf(buf, sizeof(buf), "TEST %u/8  TIME %lu.%02lu",
                  static_cast<unsigned>(getCompletedCount()),
                  static_cast<unsigned long>(runMsec / 1000),
                  static_cast<unsigned long>((runMsec % 1000) / 10));
    g.drawString(buf, 8, 15, Graphics::WHITE, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::LEFT, Graphics::VerticalAlign::MIDDLE);

    const char* phase = (mode == MODE_PHASE2) ? "PHASE 2 : LINE" : "PHASE 1 : BASIC";
    g.drawString(phase, 312, 15, Graphics::LIGHTGRAY, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::MIDDLE);

    if (messageActive)
    {
        g.fillRoundRect(64, 97, 192, 35, 7, Graphics::rgb565(2, 15, 25));
        g.drawRoundRect(64, 97, 192, 35, 7, Graphics::CYAN);
        g.drawString(message, 160, 114, Graphics::WHITE, Graphics::SIZE_18,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }
}

void CollisionLab::drawTitle(Graphics& g)
{
    g.drawString("COLLISION LAB", 160, 55, Graphics::CYAN, Graphics::SIZE_32B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("8 COLLISION TESTS / 2 PHASES", 160, 88, Graphics::LIGHTGRAY,
                 Graphics::SIZE_18, Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);

    g.drawString("PHASE 1 : TOUCH", 160, 126, Graphics::WHITE, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("PHASE 2 : LASER", 160, 149, Graphics::WHITE, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("A / START", 160, 184, Graphics::YELLOW, Graphics::SIZE_22B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    char buf[64];
    if (bestTimeMsec > 0)
    {
        std::snprintf(buf, sizeof(buf), "BEST %lu.%02lu  CLEARS %lu",
                      static_cast<unsigned long>(bestTimeMsec / 1000),
                      static_cast<unsigned long>((bestTimeMsec % 1000) / 10),
                      static_cast<unsigned long>(clearCount));
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "SAVE %s",
                      saveAvailable ? (saveStatusOk ? "LOADED" : "READY") : "NO SD");
    }
    g.drawString(buf, 160, 216, Graphics::LIGHTGRAY, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void CollisionLab::drawTransition(Graphics& g)
{
    const uint32_t now = Platform::getMsec();
    const uint32_t elapsedMsec = now - transitionStartMsec;

    g.fillRectAlpha(0, 0, 320, 240, 210, Graphics::BLACK);
    g.drawString("PHASE 1 COMPLETE", 160, 91, Graphics::GREEN, Graphics::SIZE_22B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    if (elapsedMsec >= 450)
    {
        g.drawString("PHASE 2", 160, 122, Graphics::CYAN, Graphics::SIZE_32B,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        g.drawString("LINE TEST AREA", 160, 154, Graphics::WHITE, Graphics::SIZE_18,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }
}

void CollisionLab::drawComplete(Graphics& g)
{
    char buf[64];
    const uint32_t result = completedMsec - runStartMsec;

    g.fillRectAlpha(0, 0, 320, 240, 170, Graphics::BLACK);
    g.fillRoundRect(40, 62, 240, 122, 12, Graphics::rgb565(3, 18, 28));
    g.drawRoundRect(40, 62, 240, 122, 12, 2, Graphics::GREEN);
    g.drawString("8 / 8 COMPLETE", 160, 91, Graphics::GREEN, Graphics::SIZE_25B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    std::snprintf(buf, sizeof(buf), "TIME %lu.%02lu SEC",
                  static_cast<unsigned long>(result / 1000),
                  static_cast<unsigned long>((result % 1000) / 10));
    g.drawString(buf, 160, 127, Graphics::WHITE, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("A / START : SAVE & RETRY", 160, 158, Graphics::YELLOW,
                 Graphics::SIZE_13, Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
}

bool CollisionLab::onDraw(Graphics& g, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty) return false;

    if (shakeActive)
    {
        const int16_t shakeX = static_cast<int16_t>(Math::random(-6, 7));
        const int16_t shakeY = static_cast<int16_t>(Math::random(-6, 7));
        g.setViewport(shakeX, shakeY);
        viewportWasReset = false;
    }
    else
    {
        g.resetViewport();
        viewportWasReset = true;
    }

    drawBackground(g);

    if (mode == MODE_TITLE)
    {
        drawTitle(g);
    }
    else if (mode == MODE_PHASE1 || mode == MODE_TRANSITION)
    {
        drawPhase1Stations(g);
        drawPlayer(g, false);
        drawHud(g);
        if (mode == MODE_TRANSITION) drawTransition(g);
    }
    else
    {
        drawPhase2Stations(g);
        drawPlayer(g, true);
        drawHud(g);
        if (mode == MODE_COMPLETE) drawComplete(g);
    }

    dirty = false;
    return true;
}

void CollisionLab::onTerminate(Storage& storage)
{
    if (mode == MODE_COMPLETE || saveData.isDirty())
        saveProgress(storage);
}

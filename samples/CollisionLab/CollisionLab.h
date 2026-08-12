#pragma once
#include "PRUZEA.h"

// -----------------------------------------------------------------------------
// CollisionLab
//
// Demonstrates all basic Collision helpers in two playable phases.
//
// Phase 1: pointRect / rectRect / circleCircle / circleRect
// Phase 2: pointCircle / lineLine / lineRect / lineCircle
// -----------------------------------------------------------------------------

class CollisionLab : public PRUZEA::Game
{
public:
    const char* getId() const override;
    const char* getName() const override;
    const char* getMenuName() const override;
    const char* getMenuGroup() const override;

    uint16_t getLogicalScreenWidth() const override;
    uint16_t getLogicalScreenHeight() const override;
    uint16_t getTargetScreenWidth() const override;
    uint16_t getTargetScreenHeight() const override;

protected:
    void onInit(PRUZEA::Storage& storage) override;
    Game::GameState onUpdate(PRUZEA::Input& input, PRUZEA::Audio& audio, PRUZEA::Storage& storage, float deltaSec) override;
    bool onDraw(PRUZEA::Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(PRUZEA::Storage& storage) override;

private:
    enum Mode : uint8_t
    {
        MODE_TITLE,
        MODE_PHASE1,
        MODE_TRANSITION,
        MODE_PHASE2,
        MODE_COMPLETE
    };

    enum TestBit : uint8_t
    {
        TEST_POINT_RECT    = 1 << 0,
        TEST_RECT_RECT     = 1 << 1,
        TEST_CIRCLE_CIRCLE = 1 << 2,
        TEST_CIRCLE_RECT   = 1 << 3,
        TEST_POINT_CIRCLE  = 1 << 4,
        TEST_LINE_LINE     = 1 << 5,
        TEST_LINE_RECT     = 1 << 6,
        TEST_LINE_CIRCLE   = 1 << 7,

        TEST_PHASE1_ALL = TEST_POINT_RECT | TEST_RECT_RECT |
                          TEST_CIRCLE_CIRCLE | TEST_CIRCLE_RECT,
        TEST_ALL = TEST_PHASE1_ALL | TEST_POINT_CIRCLE | TEST_LINE_LINE |
                   TEST_LINE_RECT | TEST_LINE_CIRCLE
    };

    static constexpr float PLAYER_RADIUS = 8.0f;
    static constexpr float PLAYER_SPEED = 105.0f;
    static constexpr uint32_t MESSAGE_MSEC = 900;
    static constexpr uint32_t SHAKE_MSEC = 220;
    static constexpr uint32_t PULSE_MSEC = 500;
    static constexpr uint32_t TRANSITION_MSEC = 1500;
    static constexpr uint32_t COMPLETE_WAIT_MSEC = 1800;

    static constexpr int16_t SCREEN_W = 320;
    static constexpr int16_t SCREEN_H = 240;
    static constexpr const char* SAVE_FILE = "progress.sav";

    Mode mode = MODE_TITLE;
    PRUZEA::SaveData saveData;

    float playerX = 38.0f;
    float playerY = 120.0f;
    float pointX = 46.0f;
    float pointY = 120.0f;

    uint8_t completedTests = 0;
    uint32_t runStartMsec = 0;
    uint32_t completedMsec = 0;
    uint32_t transitionStartMsec = 0;
    uint32_t messageStartMsec = 0;
    uint32_t shakeStartMsec = 0;
    uint32_t bestTimeMsec = 0;
    uint32_t clearCount = 0;

    bool messageActive = false;
    bool shakeActive = false;
    bool saveAvailable = false;
    bool saveStatusOk = false;
    bool viewportWasReset = true;
    char message[32] = {};

    void resetRun();
    void beginTest(uint8_t bit, const char* text, PRUZEA::Audio& audio);
    void beginPhase2(PRUZEA::Audio& audio);
    void updatePhase1Tests(PRUZEA::Audio& audio);
    void updatePhase2Tests(PRUZEA::Audio& audio);
    void startShake();
    void saveProgress(PRUZEA::Storage& storage);
    uint8_t getCompletedCount() const;

    void drawBackground(PRUZEA::Graphics& g);
    void drawPhase1Stations(PRUZEA::Graphics& g);
    void drawPhase2Stations(PRUZEA::Graphics& g);
    void drawPlayer(PRUZEA::Graphics& g, bool drawLaser);
    void drawHud(PRUZEA::Graphics& g);
    void drawTitle(PRUZEA::Graphics& g);
    void drawTransition(PRUZEA::Graphics& g);
    void drawComplete(PRUZEA::Graphics& g);
};

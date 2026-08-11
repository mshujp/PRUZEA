#pragma once
#include "PRUZEA.h"

class MazeEscape : public PRUZEA::Game
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
        MODE_INTRO,
        MODE_PLAYING,
        MODE_CLEAR
    };

    static constexpr int16_t SCREEN_W = 320;
    static constexpr int16_t SCREEN_H = 240;
    static constexpr int16_t TILE_SIZE = 16;
    static constexpr int16_t MAP_W = 40;
    static constexpr int16_t MAP_H = 30;
    static constexpr int16_t WORLD_W = MAP_W * TILE_SIZE;
    static constexpr int16_t WORLD_H = MAP_H * TILE_SIZE;

    static constexpr float PLAYER_RADIUS = 6.0f;
    static constexpr float PLAYER_SPEED = 64.0f;
    static constexpr int16_t LIGHT_SIZE = 112;

    static constexpr uint32_t INTRO_MSEC = 1000;
    static constexpr uint32_t CLEAR_MSEC = 1200;
    static constexpr float FOCUS_ZOOM = 3.0f;

    Mode mode = MODE_TITLE;
    PRUZEA::Vector2 player;
    PRUZEA::Animation mouthAnimation{0.30f, 6, true};
    float facingAngle = 0.0f;
    bool playerMoving = false;

    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float cameraZoom = FOCUS_ZOOM;

    uint32_t transitionStartMsec = 0;
    float transitionCameraStartX = 0.0f;
    float transitionCameraStartY = 0.0f;
    float transitionProgress = 0.0f;
    bool clearAnimationDone = false;

    void resetGame();
    void startIntro();
    void updateIntro();
    void updatePlayer(PRUZEA::Input& input, float deltaSec);
    void updatePlayCamera();
    void startClear();
    void updateClear();
    void getPlayCameraTarget(float& x, float& y) const;
    void getFocusCameraTarget(float& x, float& y) const;
    void resolveWallCollisions();
    bool isGoalReached() const;

    void applyCamera(PRUZEA::Graphics& graphics) const;
    void applyLightClip(PRUZEA::Graphics& graphics, float width, float height) const;
    void drawMaze(PRUZEA::Graphics& graphics) const;
    void drawPlayer(PRUZEA::Graphics& graphics) const;
    void drawTitle(PRUZEA::Graphics& graphics) const;
    void drawClear(PRUZEA::Graphics& graphics) const;
};

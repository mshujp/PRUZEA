#include "PRUZEA.h"

#pragma once

// -----------------------------------------------------------------------------
// PRUZEA APIs - SSD1306 Test
//
// Compact API reference for SSD1306 systems.
// Graphics features that have a meaningful monochrome implementation are
// demonstrated here. Alpha blending, gradients, and decoded images are omitted.
// -----------------------------------------------------------------------------

class PruzeaAPIsSSD1306 : public PRUZEA::Game {
public:
    const char* getId() const override;
    const char* getName() const override;
    const char* getMenuName() const override;
    const char* getMenuGroup() const override { return "SAMPLES"; }

    uint16_t getLogicalScreenWidth() const override;
    uint16_t getLogicalScreenHeight() const override;
    uint16_t getTargetScreenWidth() const override;
    uint16_t getTargetScreenHeight() const override;

    void onInit(PRUZEA::Storage& storage) override;
    GameState onUpdate(PRUZEA::Input& input,
                       PRUZEA::Audio& audio,
                       PRUZEA::Storage& storage,
                       float deltaSec) override;
    bool onDraw(PRUZEA::Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(PRUZEA::Storage& storage) override;

private:
    enum class Mode : uint8_t {
        TITLE,
        GRAPHICS
    };

    void enterGraphics(PRUZEA::Audio& audio);
    void changeStep(int8_t amount, PRUZEA::Audio& audio);
    void playTestSE(PRUZEA::Audio& audio,
                    const PRUZEA::Audio::Sound* sound,
                    float gain,
                    uint16_t iconMsec);

    void updateInputMask(PRUZEA::Input& input);
    void updateTitle(PRUZEA::Input& input, PRUZEA::Audio& audio);
    void updateGraphics(PRUZEA::Input& input, PRUZEA::Audio& audio);

    void drawTitle(PRUZEA::Graphics& graphics);
    void drawGraphicsTest(PRUZEA::Graphics& graphics);
    void drawMovingShape(PRUZEA::Graphics& graphics,
                         uint8_t step,
                         int16_t x,
                         int16_t y);
    void drawArcTest(PRUZEA::Graphics& graphics);
    void drawFontTest(PRUZEA::Graphics& graphics);
    void drawAlignmentTest(PRUZEA::Graphics& graphics);
    void drawViewportTest(PRUZEA::Graphics& graphics);
    void drawCameraTest(PRUZEA::Graphics& graphics);
    void drawClipRectTest(PRUZEA::Graphics& graphics);
    void drawSpriteTest(PRUZEA::Graphics& graphics);
    void drawSpriteTransformTest(PRUZEA::Graphics& graphics);
    void drawSpriteSheetTest(PRUZEA::Graphics& graphics);
    void drawInputOverlay(PRUZEA::Graphics& graphics);
    void drawSpeakerIcon(PRUZEA::Graphics& graphics, int16_t x, int16_t y);

    int16_t getAnimX(uint32_t now) const;
    const char* getStepName() const;

    Mode mode = Mode::TITLE;
    uint8_t drawStep = 0;
    uint16_t inputMask = 0;
    uint32_t stepStartMsec = 0;
    uint32_t speakerIconStartMsec = 0;
    uint16_t speakerIconDurationMsec = 0;
};

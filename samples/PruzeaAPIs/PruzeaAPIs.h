#include "PRUZEA.h"

#pragma once

// -----------------------------------------------------------------------------
// PRUZEA Test
//
// A lightweight game framework with reference samples and tests.
//
// Features:
//   * Graphics
//   * Audio
//   * Input
//   * Storage
//   * Collision
//   * Math
//
// -----------------------------------------------------------------------------

class PruzeaAPIs : public PRUZEA::Game {
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
    GameState onUpdate(PRUZEA::Input& input, PRUZEA::Audio& audio, PRUZEA::Storage& storage, float deltaSec) override;
    bool onDraw(PRUZEA::Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(PRUZEA::Storage& storage) override;

private:
    enum class Mode : uint8_t {
        TITLE,
        DRAW_TEST,
        INPUT_TEST,
        AUDIO_TEST,
        STORAGE_TEST
    };

    enum class AudioRow : uint8_t {
        SE,
        MUSIC
    };

    struct StorageContext {
        PruzeaAPIs* self;
        bool wrote;
    };

    static bool writeStorageLine(std::string& line, void* arg);
    static bool readStorageLine(const char* line, void* arg);

    void resetToTitle(PRUZEA::Audio& audio);
    void enterDrawTest(PRUZEA::Audio& audio);
    void enterInputTest(PRUZEA::Audio& audio);
    void enterAudioTest(PRUZEA::Audio& audio);
    void enterStorageTest(PRUZEA::Audio& audio, PRUZEA::Storage& storage);
    void runStorageTest(PRUZEA::Storage& storage);
    void nextDrawStep(PRUZEA::Audio& audio);

    void updateTitle(PRUZEA::Input& input, PRUZEA::Audio& audio, PRUZEA::Storage& storage);
    void updateDrawTest(PRUZEA::Input& input, PRUZEA::Audio& audio);
    void updateInputTest(PRUZEA::Input& input, PRUZEA::Audio& audio);
    void updateAudioTest(PRUZEA::Input& input, PRUZEA::Audio& audio);
    void updateStorageTest(PRUZEA::Input& input, PRUZEA::Audio& audio, PRUZEA::Storage& storage);

    bool drawTitle(PRUZEA::Graphics& graphics);
    bool drawDrawTest(PRUZEA::Graphics& graphics, bool requestFullRedraw);
    bool drawInputTest(PRUZEA::Graphics& graphics);
    bool drawAudioTest(PRUZEA::Graphics& graphics);
    bool drawStorageTest(PRUZEA::Graphics& graphics);

    void drawBackground(PRUZEA::Graphics& graphics);
    void drawHeader(PRUZEA::Graphics& graphics, const char* label);
    void drawCenteredHint(PRUZEA::Graphics& graphics, const char* text, int16_t y);
    void drawMovingShape(PRUZEA::Graphics& graphics, uint8_t step, int16_t x, int16_t y);
    void drawArcTest(PRUZEA::Graphics& graphics);
    void drawGradientTest(PRUZEA::Graphics& graphics);
    void drawFontTest(PRUZEA::Graphics& graphics);
    void drawAlignmentTest(PRUZEA::Graphics& graphics);
    void drawTextMetricsTest(PRUZEA::Graphics& graphics);
    void drawViewportTest(PRUZEA::Graphics& graphics);
    void drawCameraTest(PRUZEA::Graphics& graphics);
    void drawClipRectTest(PRUZEA::Graphics& graphics);
    void drawSpriteTest(PRUZEA::Graphics& graphics);
    void drawSpriteTransformTest(PRUZEA::Graphics& graphics);
    void drawSpriteSheetTest(PRUZEA::Graphics& graphics);
    void drawButtonLamp(PRUZEA::Graphics& graphics, int16_t x, int16_t y, int16_t w, int16_t h, const char* label, bool on);

    int16_t getAnimX(uint32_t now) const;
    const char* getDrawStepName() const;
    const char* getSeName() const;
    const PRUZEA::Audio::Music* getSelectedMusic(bool loop) const;

    Mode mode = Mode::TITLE;
    uint8_t titleIndex = 0;
    uint8_t drawStep = 0;
    uint8_t lastDrawStep = 0;
    uint32_t modeStartMsec = 0;
    uint32_t stepStartMsec = 0;
    uint32_t lastVisualMsec = 0;
    uint16_t inputMask = 0;

    AudioRow audioRow = AudioRow::SE;
    uint8_t seIndex = 0;
    uint8_t musicIndex = 0;

    uint32_t storageWriteValue = 0;
    uint32_t storageReadValue = 0;
    bool storageAvailable = false;
    bool storageWriteOk = false;
    bool storageReadOk = false;
    bool storageMatch = false;

    bool saveDataSetOk = false;
    bool saveDataSaveOk = false;
    bool saveDataLoadOk = false;
    bool saveDataMatch = false;
};

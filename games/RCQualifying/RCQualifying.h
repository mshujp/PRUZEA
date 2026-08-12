#pragma once
#include "PRUZEA.h"

class RCQualifying : public PRUZEA::Game
{
public:
    struct Point { float x; float y; };
    const char* getId() const override;
    const char* getName() const override;
    const char* getMenuName() const override;
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
    enum Mode : uint8_t { MODE_TITLE, MODE_RUNNING, MODE_PAUSED, MODE_RESULT };
    enum Course : uint8_t { COURSE_SPEEDWAY, COURSE_TRI_OVAL, COURSE_D_SHAPE, COURSE_EGG_OVAL, COURSE_COUNT };

    static constexpr int16_t SCREEN_W = 320;
    static constexpr int16_t SCREEN_H = 240;
    static constexpr uint8_t QUALIFYING_LAPS = 4;
    static constexpr uint8_t TRACK_SAMPLES = 96;
    static constexpr float TRACK_HALF_WIDTH = 19.0f;
    static constexpr float APRON_HALF_WIDTH = 23.0f;

    static constexpr float MAX_FORWARD_SPEED = 116.0f;
    static constexpr float MAX_REVERSE_SPEED = 34.0f;
    static constexpr float ACCELERATION = 78.0f;
    static constexpr float BRAKE_FORCE = 145.0f;
    static constexpr float REVERSE_ACCELERATION = 46.0f;
    static constexpr float COAST_DRAG = 32.0f;
    static constexpr float TURN_RATE = 2.55f;
    static constexpr float STEER_RESPONSE = 4.8f;
    static constexpr float STEERING_DRAG_RATE = 0.62f;

    Mode mode = MODE_TITLE;
    Course selectedCourse = COURSE_SPEEDWAY;
    bool analogAvailable = false;
    bool timingActive = false;
    bool saveDirty = false;

    float carX = 176.0f;
    float carY = 196.0f;
    float carAngle = 0.0f;
    float speed = 0.0f;
    float steering = 0.0f;
    float previousProgress = 0.0f;

    uint8_t checkpoint = 0;
    uint8_t completedLaps = 0;
    uint32_t runStartMsec = 0;
    uint32_t lapStartMsec = 0;
    uint32_t finishTotalMsec = 0;
    uint32_t lapTimes[QUALIFYING_LAPS] = {};
    uint32_t bestTotalMsec[COURSE_COUNT] = {};
    uint32_t modeStartMsec = 0;
    uint32_t pauseStartMsec = 0;

    PRUZEA::SaveData saveData;

    void resetTitle();
    void startRun();
    void resetCar();
    void updateDriving(PRUZEA::Input& input, PRUZEA::Audio& audio, PRUZEA::Storage& storage, float deltaSec);
    float readSteering(const PRUZEA::Input& input) const;
    void updateLapProgress(PRUZEA::Audio& audio, PRUZEA::Storage& storage);
    void finishRun(PRUZEA::Audio& audio, PRUZEA::Storage& storage);
    void saveBests(PRUZEA::Storage& storage);

    static float distanceToSegmentSquared(float px, float py, const Point& a, const Point& b);
    static bool insideRoundedRect(float px, float py, float x, float y, float w, float h, float radius);
    static Point catmullRom(const Point& p0, const Point& p1, const Point& p2, const Point& p3, float t);
    const Point* getCoursePoints(uint8_t& count) const;
    Point getCoursePoint(float progress) const;
    Point getCourseTangent(float progress) const;
    float getTrackProgress(float x, float y) const;
    bool isOnTrack(float x, float y) const;
    const char* getCourseName() const;
    const char* getBestKey(Course course) const;

    void drawTrack(PRUZEA::Graphics& graphics) const;
    void drawCar(PRUZEA::Graphics& graphics) const;
    void drawHud(PRUZEA::Graphics& graphics, uint32_t now) const;
    void drawTitle(PRUZEA::Graphics& graphics, uint32_t now) const;
    void drawResult(PRUZEA::Graphics& graphics) const;
    void drawPause(PRUZEA::Graphics& graphics) const;
    void drawTime(PRUZEA::Graphics& graphics, uint32_t msec, int16_t x, int16_t y,
                  PRUZEA::Graphics::Color color, PRUZEA::Graphics::Font font,
                  PRUZEA::Graphics::HorizontalAlign align) const;
};

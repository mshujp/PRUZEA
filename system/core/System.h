#pragma once

#include "AudioBase.h"
#include "GameCatalog.h"
#include "GraphicsBase.h"
#include "InputBase.h"
#include "StorageBase.h"
#include "SystemUI.h"

#include <atomic>
#include <stdint.h>

namespace PRUZEA
{

class System
{
public:
    enum class ShutdownReason : uint8_t
    {
        USER_REQUEST,
        LOW_BATTERY,
        INITIALIZATION_FAILURE,
        RUNTIME_ERROR
    };

    struct BatteryConfig
    {
        float externalPowerThreshold = 0.0f;
        float warningThreshold = 0.0f;
        float criticalThreshold = 0.0f;
        float cutoffThreshold = 0.0f;
        uint16_t sampleIntervalFrames = 300;
        uint8_t initialSampleCount = 10;
        uint16_t initialSampleDelayMsec = 50;
    };

    struct Callbacks
    {
        void* context = nullptr;
        bool (*readBatteryVoltage)(void* context, float& voltage) = nullptr;
        bool (*launchAudioWorker)(void* context, System& system) = nullptr;
        void (*finishAudioWorker)(void* context, bool stoppedCleanly) = nullptr;
        void (*enterPowerOffState)(void* context, ShutdownReason reason) = nullptr;
    };

    System(GraphicsBase& graphics, InputBase& input, StorageBase& storage, AudioBase& audio, SystemUI& systemUI, const Callbacks& callbacks, const BatteryConfig& batteryConfig);
    System(const System&) = delete;
    System& operator=(const System&) = delete;

    bool start();
    void runAudioWorker();

private:
    enum class ExecState : uint8_t
    {
        SYSTEM_UI,
        IN_GAME,
        SHUT_DOWN,
        ERROR
    };

    static constexpr uint16_t BATTERY_HISTORY_SIZE = 10;
    static constexpr uint32_t MIN_SPLASH_DISPLAY_MSEC = 800;
    static constexpr uint32_t AUDIO_WORKER_START_TIMEOUT_MSEC = 500;
    static constexpr uint32_t AUDIO_WORKER_STOP_TIMEOUT_MSEC = 500;

    GraphicsBase& graphics;
    InputBase& input;
    StorageBase& storage;
    AudioBase& audio;
    SystemUI& systemUI;
    Callbacks callbacks;
    BatteryConfig batteryConfig;

    GameCatalog gameCatalog;
    Game* currentGame = nullptr;
    ExecState execState = ExecState::SYSTEM_UI;

    uint8_t volumeSteps;

    float batteryHistory[BATTERY_HISTORY_SIZE]{};
    uint8_t batteryHistoryIndex = 0;
    uint8_t batteryHistoryCount = 0;
    float currentDisplayedV = 0.0f;
    uint16_t batterySampleFrameCounter = 0;

    uint16_t fpsFrameCounter = 0;
    uint16_t measuredFps = 0;
    uint32_t fpsWindowStartMsec = 0;
    uint32_t lastFrameMsec = 0;

    bool graphicsAvailable = false;
    bool inputAvailable = false;
    bool storageStarted = false;
    bool debugMode = false;
    bool statusDirty = true;
    bool requestFullRedraw = true;
    bool shutdownPrepared = false;

    std::atomic<bool> audioWorkerStopRequested{false};
    std::atomic<bool> audioWorkerAlive{false};
    std::atomic<bool> audioWorkerReady{false};
    std::atomic<bool> audioAvailable{false};

    static void getSystemInfoHandler(SystemUI::SystemInfo& info, void* context);
    static bool deleteGameDataHandler(const char* gameId, void* context);

    bool initialize();
    bool launchAudioWorker();
    bool stopAudioWorker();
    bool loop();
    void waitFor30Fps();

    void updateSystem();
    void updateBattery();
    void updateCoreRingPowerState();
    void requestSystemStatusRedraw();
    void drawSystemStatus();

    void initGame(Game& game);
    void resetGraphicsFor(Game& game);
    void returnToSystemUI();

    void saveVolume(uint8_t volume);
    uint8_t loadVolume();

    void beginShutdown(ShutdownReason reason);
    void stopAllServices(bool keepDisplayActive);
};

} // namespace PRUZEA

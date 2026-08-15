#include "System.h"

#include "CoreRing.h"
#include "PRUZEAGeneratedGames.h"
#include "Platform.h"
#include "ScreenShot.h"

#include <stdio.h>
#include <stdlib.h>

using namespace PRUZEA;

namespace
{
constexpr const char* SYSTEM_ID = "_system";
constexpr const char* SYSTEM_CONFIG_FILE = "config.ini";
}

System::System(GraphicsBase& graphics, InputBase& input, StorageBase& storage, AudioBase& audio, SystemUI& systemUI, const Callbacks& callbacks, const BatteryConfig& batteryConfig)
    : graphics(graphics), input(input), storage(storage), audio(audio), systemUI(systemUI), callbacks(callbacks), batteryConfig(batteryConfig)
{
}

bool System::start()
{
    const bool initialized = initialize();
    const bool audioLaunched = launchAudioWorker();

    if (!initialized)
    {
        if (audioLaunched && audioAvailable.load())
        {
            audio.playSE(&Audio::SE::NO_13, 0.6f);
            Platform::sleepMsec(700);
        }
        beginShutdown(ShutdownReason::INITIALIZATION_FAILURE);
        return false;
    }

    while (loop())
    {
        waitFor30Fps();
    }

    return true;
}

bool System::initialize()
{
    graphicsAvailable = graphics.begin();
    if (graphicsAvailable)
    {
        if (!graphics.setLogicalScreenSize(systemUI.getLogicalScreenWidth(), systemUI.getLogicalScreenHeight()))
        {
            graphicsAvailable = false;
        }
    }

    inputAvailable = input.begin();
    storageStarted = storage.begin();

    volumeSteps = audio.getVolumeSteps();
    audio.setVolumeLevel(loadVolume());

    if (callbacks.readBatteryVoltage != nullptr)
    {
        const uint8_t sampleCount = batteryConfig.initialSampleCount > BATTERY_HISTORY_SIZE ? BATTERY_HISTORY_SIZE : batteryConfig.initialSampleCount;

        for (uint8_t i = 0; i < sampleCount; ++i)
        {
            float voltage = 0.0f;
            if (callbacks.readBatteryVoltage(callbacks.context, voltage))
            {
                batteryHistory[batteryHistoryIndex] = voltage;
                batteryHistoryIndex = (batteryHistoryIndex + 1) % BATTERY_HISTORY_SIZE;
                batteryHistoryCount++;
            }
            if (i + 1 < sampleCount)
            {
                Platform::sleepMsec(batteryConfig.initialSampleDelayMsec);
            }
        }

        if (batteryHistoryCount > 0)
        {
            float sum = 0.0f;
            for (uint8_t i = 0; i < batteryHistoryCount; ++i)
            {
                sum += batteryHistory[i];
            }
            currentDisplayedV = sum / batteryHistoryCount;
        }
    }

    updateCoreRingPowerState();
    srand(Platform::getMsec());

    registerGeneratedGames(gameCatalog, graphics);
    systemUI.setCatalog(&gameCatalog);
    systemUI.setSystemInfoHandler(&System::getSystemInfoHandler, this);
    systemUI.setDeleteGameDataHandler(&System::deleteGameDataHandler, this);

    if (graphicsAvailable && inputAvailable)
    {
        systemUI.init(storage);
    }

    CoreRing::setMode(CoreRing::MODE_SYSTEM_UI);

    requestSystemStatusRedraw();
    return graphicsAvailable && inputAvailable;
}

bool System::launchAudioWorker()
{
    if (callbacks.launchAudioWorker == nullptr) return false;

    audioWorkerStopRequested.store(false);
    audioWorkerReady.store(false);
    audioAvailable.store(false);

    if (!callbacks.launchAudioWorker(callbacks.context, *this)) return false;

    const uint32_t startMsec = Platform::getMsec();
    while (!audioWorkerReady.load())
    {
        if (Platform::elapsed(Platform::getMsec(), startMsec, AUDIO_WORKER_START_TIMEOUT_MSEC))
        {
            return false;
        }
        Platform::sleepMsec(1);
    }

    return audioAvailable.load();
}

void System::runAudioWorker()
{
    audioWorkerAlive.store(true);
    audioAvailable.store(audio.begin());
    audioWorkerReady.store(true);

    if (audioAvailable.load())
    {
        audio.playSE(&Audio::SE::NO_8, 1.0f);

        while (!audioWorkerStopRequested.load())
        {
            audio.update();
            Platform::sleepUsec(1000);
        }

        audio.end();
    }

    audioWorkerAlive.store(false);
}

bool System::stopAudioWorker()
{
    audioWorkerStopRequested.store(true);

    const uint32_t startMsec = Platform::getMsec();
    while (audioWorkerAlive.load())
    {
        if (Platform::elapsed(Platform::getMsec(), startMsec, AUDIO_WORKER_STOP_TIMEOUT_MSEC))
        {
            break;
        }
        Platform::sleepMsec(1);
    }

    const bool stoppedCleanly = !audioWorkerAlive.load();
    if (callbacks.finishAudioWorker != nullptr)
    {
        callbacks.finishAudioWorker(callbacks.context, stoppedCleanly);
    }
    return stoppedCleanly;
}

bool System::loop()
{
    const uint32_t frameNowMsec = Platform::getMsec();
    float deltaSec = 1.0f / 30.0f;
    if (lastFrameMsec != 0)
    {
        uint32_t deltaMsec = frameNowMsec - lastFrameMsec;
        if (deltaMsec > 100) deltaMsec = 100;
        deltaSec = static_cast<float>(deltaMsec) / 1000.0f;
    }
    lastFrameMsec = frameNowMsec;

    input.update();
    updateSystem();
    CoreRing::update();

    const bool statusDirtyAtFrameStart = statusDirty;
    const bool fullRedraw = requestFullRedraw || statusDirtyAtFrameStart;
    bool drew = false;

    if (execState == ExecState::SYSTEM_UI)
    {
        const Game::GameState state = systemUI.update(input, audio, storage, deltaSec);
        if (state == Game::GameState::TERMINATED)
        {
            execState = ExecState::SHUT_DOWN;
            beginShutdown(ShutdownReason::USER_REQUEST);
            return false;
        }

        currentGame = systemUI.takeSelectedGame();
        if (currentGame == nullptr)
        {
            drew = systemUI.draw(graphics, fullRedraw);
        }
        else
        {
            initGame(*currentGame);
            execState = ExecState::IN_GAME;
            requestFullRedraw = true;
            lastFrameMsec = 0;
        }

        if (input.pressed(Input::Button::X) && input.justPressed(Input::Button::SELECT)) debugMode = !debugMode;
    }
    else if (execState == ExecState::IN_GAME)
    {
        if (currentGame == nullptr)
        {
            returnToSystemUI();
        }
        else if (input.justPressed(Input::Button::HOME))
        {
            if (currentGame->requestTerminate() == Game::TerminateResponse::ACCEPT)
            {
                audio.stopSE();
                audio.stopMusic();
                currentGame->terminate(storage);
                currentGame = nullptr;
                returnToSystemUI();
                audio.playSE(&Audio::SE::NO_2, 1.0f);
            }
        }
        else
        {
            const Game::GameState gameState = currentGame->update(input, audio, storage, deltaSec);
            drew = currentGame->draw(graphics, fullRedraw);

            if (gameState == Game::GameState::TERMINATED)
            {
                audio.stopMusic();
                currentGame = nullptr;
                returnToSystemUI();
                audio.playSE(&Audio::SE::NO_2, 1.0f);
            }
        }
    }
    drew = CoreRing::drawOverlay(graphics, drew) || drew;

    if (drew)
    {
        drawSystemStatus();
        graphics.push();
        statusDirty = false;
        requestFullRedraw = false;
    }

    return execState != ExecState::SHUT_DOWN;
}

void System::waitFor30Fps()
{
    static uint32_t lastLoopMsec = 0;
    const uint32_t now = Platform::getMsec();

    if (lastLoopMsec == 0 || now - lastLoopMsec > 100)
    {
        lastLoopMsec = now;
    }

    while (Platform::getMsec() - lastLoopMsec < 33)
    {
        Platform::sleepMsec(1);
    }
    lastLoopMsec += 33;
}

void System::updateSystem()
{
    fpsFrameCounter++;

    if (debugMode)
    {
        const uint32_t now = Platform::getMsec();
        if (fpsWindowStartMsec == 0)
        {
            fpsWindowStartMsec = now;
        }
        else if (now - fpsWindowStartMsec >= 1000)
        {
            measuredFps = static_cast<uint16_t>((static_cast<uint32_t>(fpsFrameCounter) * 1000u) / (now - fpsWindowStartMsec));
            fpsFrameCounter = 0;
            fpsWindowStartMsec = now;
            requestSystemStatusRedraw();
        }
    }

    if (input.justPressed(Input::Button::VOL_UP) && input.pressed(Input::Button::VOL_DOWN))
    {
        ScreenShot shot;
        shot.save(graphics, storage);
        audio.playSE(&Audio::SE::NO_5, 1.0f);
    }
    else if (input.justPressed(Input::Button::MUTE))
    {
        audio.toggleMute();
        requestSystemStatusRedraw();
    }
    else if (input.justPressed(Input::Button::VOL_DOWN) || input.justPressed(Input::Button::VOL_UP))
    {
        const int8_t previous = audio.getVolumeLevel();
        if (input.justPressed(Input::Button::VOL_DOWN))
        {
            audio.downVolume();
        }
        else
        {
            audio.upVolume();
        }

        if (audio.getVolumeLevel() != previous)
        {
            requestSystemStatusRedraw();
            audio.playSE(&Audio::SE::NO_1, 1.0f);
            saveVolume(static_cast<uint8_t>(audio.getVolumeLevel()));
        }
    }

    updateBattery();
}

void System::updateBattery()
{
    if (callbacks.readBatteryVoltage == nullptr || batteryConfig.sampleIntervalFrames == 0) return;

    batterySampleFrameCounter++;
    if (batterySampleFrameCounter < batteryConfig.sampleIntervalFrames) return;
    batterySampleFrameCounter = 0;

    float voltage = 0.0f;
    if (!callbacks.readBatteryVoltage(callbacks.context, voltage)) return;

    const float previous = currentDisplayedV;
    batteryHistory[batteryHistoryIndex] = voltage;
    batteryHistoryIndex = (batteryHistoryIndex + 1) % BATTERY_HISTORY_SIZE;
    if (batteryHistoryCount < BATTERY_HISTORY_SIZE) batteryHistoryCount++;

    float sum = 0.0f;
    for (uint8_t i = 0; i < batteryHistoryCount; ++i)
    {
        sum += batteryHistory[i];
    }
    currentDisplayedV = sum / batteryHistoryCount;

    updateCoreRingPowerState();
    if (currentDisplayedV != previous)
    {
        requestSystemStatusRedraw();
    }

    if (currentDisplayedV > batteryConfig.externalPowerThreshold && currentDisplayedV <= batteryConfig.cutoffThreshold)
    {
        beginShutdown(ShutdownReason::LOW_BATTERY);
        execState = ExecState::SHUT_DOWN;
    }
}

void System::updateCoreRingPowerState()
{
    if (callbacks.readBatteryVoltage != nullptr && currentDisplayedV >= batteryConfig.externalPowerThreshold)
    {
        CoreRing::setPowerWarning(currentDisplayedV <= batteryConfig.warningThreshold, currentDisplayedV <= batteryConfig.criticalThreshold);
    }
    else
    {
        CoreRing::setPowerWarning(false, false);
    }
}

void System::requestSystemStatusRedraw()
{
    statusDirty = true;
    requestFullRedraw = true;
}

void System::drawSystemStatus()
{
    const int16_t viewportX = graphics.getViewportX();
    const int16_t viewportY = graphics.getViewportY();

    const char* volumeText = nullptr;
    const uint8_t volume = audio.getVolumeLevel();
    if (audio.isMuted())
    {
        volumeText = "VOL: MUTE";
    }
    else if (volumeSteps == 0)
    {
    }
    else if (volumeSteps == 2)
    {
        switch (volume)
        {
            case 1: volumeText = "VOL: ON"; break;
            default: volumeText = "VOL: ?"; break;
        }
    }
    else
    {
        switch (volume)
        {
            case 1: volumeText = "VOL: - _ _"; break;
            case 2: volumeText = "VOL: - = _"; break;
            case 3: volumeText = "VOL: - = #"; break;
            default: volumeText = "VOL: ?"; break;
        }
    }
    graphics.suspendCamera();
    graphics.suspendClipRect();
    if (volumeText != nullptr)
    {
        graphics.drawString(volumeText, 14 + viewportX, 225 + viewportY, Graphics::BLACK, Graphics::Font::SIZE_10);
        graphics.drawString(volumeText, 13 + viewportX, 226 + viewportY, Graphics::BLACK, Graphics::Font::SIZE_10);
        graphics.drawString(volumeText, 13 + viewportX, 225 + viewportY, Graphics::WHITE, Graphics::Font::SIZE_10);
    }

    if (debugMode)
    {
        char fpsText[16];
        snprintf(fpsText, sizeof(fpsText), "%u fps", measuredFps);
        graphics.drawString(fpsText, 143 + viewportX, 225 + viewportY, Graphics::BLACK, Graphics::Font::SIZE_10);
        graphics.drawString(fpsText, 142 + viewportX, 226 + viewportY, Graphics::BLACK, Graphics::Font::SIZE_10);
        graphics.drawString(fpsText, 142 + viewportX, 225 + viewportY, Graphics::WHITE, Graphics::Font::SIZE_10);
    }
    graphics.resumeClipRect();
    graphics.resumeCamera();

    requestFullRedraw = true;
}

void System::resetGraphicsFor(Game& game)
{
    graphics.resetViewport();
    graphics.resetCamera();
    graphics.resetClipRect();
    graphics.setLogicalScreenSize(game.getLogicalScreenWidth(), game.getLogicalScreenHeight());
    graphics.clearScreen();
}

void System::initGame(Game& game)
{
    resetGraphicsFor(game);
    game.init(storage);
    input.setRepeatSettings(Input::DAS_DELAY_MSEC_DEFAULT, Input::ARR_DELAY_MSEC_DEFAULT);
    CoreRing::setMode(CoreRing::MODE_IDLE);
}

void System::returnToSystemUI()
{
    resetGraphicsFor(systemUI);
    input.setRepeatSettings(Input::DAS_DELAY_MSEC_DEFAULT, Input::ARR_DELAY_MSEC_DEFAULT);
    execState = ExecState::SYSTEM_UI;
    CoreRing::setMode(CoreRing::MODE_SYSTEM_UI);
    requestFullRedraw = true;
}

void System::saveVolume(uint8_t volume)
{
    if (!storage.isAvailable()) return;

    SaveData data;
    if (data.load(storage, SYSTEM_ID, SYSTEM_CONFIG_FILE))
    {
        data.setUInt32("volume", volume);
        data.save(storage, SYSTEM_ID, SYSTEM_CONFIG_FILE);
    }
}

uint8_t System::loadVolume()
{
    if (!storage.isAvailable()) return 1;

    SaveData data;
    data.load(storage, SYSTEM_ID, SYSTEM_CONFIG_FILE);
    return static_cast<uint8_t>(data.getUInt32("volume", 1));
}

void System::beginShutdown(ShutdownReason reason)
{
    if (shutdownPrepared) return;

    const bool userRequested = reason == ShutdownReason::USER_REQUEST;

    if (graphicsAvailable)
    {
        if (userRequested)
        {
            systemUI.drawPowerOffReady(graphics);
        }
        else if (reason == ShutdownReason::LOW_BATTERY)
        {
            if (audioAvailable.load())
            {
                audio.playSE(&Audio::SE::NO_12, 0.6f);
            }
            systemUI.drawLowBttery(graphics);
        }
        else
        {
            graphics.fillScreen(Graphics::BLACK);
            graphics.drawString(
                "SYSTEM ERROR",
                graphics.getScreenWidth() / 2, graphics.getScreenHeight() / 2,
                Graphics::RED,
                Graphics::Font::SIZE_25B,
                Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }
        graphics.push();
    }

    if (!userRequested)
    {
        Platform::sleepMsec(reason == ShutdownReason::LOW_BATTERY ? 2000 : 1000);
    }

    stopAllServices(userRequested);

    if (callbacks.enterPowerOffState != nullptr)
    {
        callbacks.enterPowerOffState(callbacks.context, reason);
    }
}

void System::stopAllServices(bool keepDisplayActive)
{
    if (shutdownPrepared) return;
    shutdownPrepared = true;

    if (currentGame != nullptr)
    {
        currentGame->terminate(storage);
        currentGame = nullptr;
    }

    audio.stopMusic();
    audio.stopSE();

    if (inputAvailable)
    {
        input.end();
        inputAvailable = false;
    }

    if (storageStarted)
    {
        storage.end();
        storageStarted = false;
    }

    stopAudioWorker();

    if (graphicsAvailable && !keepDisplayActive)
    {
        graphics.end();
        graphicsAvailable = false;
    }
}

void System::getSystemInfoHandler(SystemUI::SystemInfo& info, void* context)
{
    System* system = static_cast<System*>(context);
    if (system == nullptr) return;

    snprintf(info.version, sizeof(info.version), "%s", PRUZEA_VERSION);
    snprintf(info.display, sizeof(info.display), "%s", system->graphicsAvailable ? system->graphics.getName() : "---");
    snprintf(info.input, sizeof(info.input), "%s", system->inputAvailable ? system->input.getName() : "---");
    snprintf(info.audio, sizeof(info.audio), "%s", system->audioAvailable.load() ? system->audio.getName() : "---");
    snprintf(info.storage, sizeof(info.storage), "%s", system->storage.isAvailable() ? system->storage.getName() : "---");

    if (system->callbacks.readBatteryVoltage != nullptr && system->currentDisplayedV > system->batteryConfig.externalPowerThreshold)
    {
        snprintf(info.battery, sizeof(info.battery), "%.1fV", system->currentDisplayedV);
    }
    else
    {
        snprintf(info.battery, sizeof(info.battery), "%s", "---");
    }
}

bool System::deleteGameDataHandler(const char* gameId, void* context)
{
    System* system = static_cast<System*>(context);
    return system != nullptr && system->storage.deleteGameData(gameId);
}

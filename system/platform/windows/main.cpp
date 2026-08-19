#include "System.h"

#include "graphics/GraphicsSDL.h"
#include "input/InputSDL.h"
#include "audio/AudioSDL.h"
#include "storage/StorageWinFS.h"
#include "SystemUI320x240.h"

#include <SDL3/SDL.h>
#include <thread>

using namespace PRUZEA;

namespace
{

struct WindowsPlatformContext
{
    std::thread audioThread;
};

bool launchAudioWorker(void* context, System& system)
{
    auto* platform = static_cast<WindowsPlatformContext*>(context);
    if (platform == nullptr || platform->audioThread.joinable())
    {
        return false;
    }

    try
    {
        platform->audioThread = std::thread([&system]()
        {
            system.runAudioWorker();
        });
    }
    catch (...)
    {
        return false;
    }

    return true;
}

void finishAudioWorker(void* context, bool)
{
    auto* platform = static_cast<WindowsPlatformContext*>(context);
    if (platform != nullptr && platform->audioThread.joinable())
    {
        platform->audioThread.join();
    }
}

void requestSystemTerminate(void* context)
{
    auto* system = static_cast<System*>(context);
    if (system != nullptr)
    {
        system->requestTerminate();
    }
}

} // namespace

int main()
{
    int exitCode = 1;

    {
        GraphicsSDL graphicsImpl;
        InputSDL inputImpl;
        StorageWinFS storageImpl;
        AudioSDL audioImpl;
        SystemUI320x240 systemUIImpl;

        WindowsPlatformContext platformContext;

        const System::Callbacks callbacks{
            &platformContext,
            nullptr,
            launchAudioWorker,
            finishAudioWorker,
            nullptr
        };

        const System::BatteryConfig batteryConfig{};

        System system(
            graphicsImpl,
            inputImpl,
            storageImpl,
            audioImpl,
            systemUIImpl,
            callbacks,
            batteryConfig);

        inputImpl.setTerminateHandler(requestSystemTerminate, &system);

        exitCode = system.start() ? 0 : 1;

        // Normally System::stopAudioWorker() joins the worker through
        // finishAudioWorker(). Keep this as a final safeguard if startup
        // stopped before the normal shutdown path was reached.
        if (platformContext.audioThread.joinable())
        {
            platformContext.audioThread.join();
        }
    }

    SDL_Quit();
    return exitCode;
}

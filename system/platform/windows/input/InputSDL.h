#pragma once

#include "InputBase.h"
#include <SDL3/SDL.h>

namespace PRUZEA {

class InputSDL : public InputBase
{
public:
    using TerminateHandler = void(*)(void* context);

    struct Config
    {
        int16_t analogDeadZone = 8000;
    };

    InputSDL();
    explicit InputSDL(const Config& config);
    ~InputSDL() override;

    const char* getName() const override { return "SDL"; }
    bool begin() override;
    void end() override;
    void update() override;

    bool hasAnalogSticks() const override;
    int16_t axis(Axis axis) const override;
    bool touched() const override { return currentTouched; }
    bool justTouched() const override { return currentTouched && !previousTouched; }
    bool justTouchReleased() const override { return !currentTouched && previousTouched; }
    int16_t touchX() const override { return currentTouchX; }
    int16_t touchY() const override { return currentTouchY; }

    void setTerminateHandler(TerminateHandler handler, void* context);

private:
    int16_t analogDeadZone;
    SDL_Gamepad* gamepad = nullptr;
    bool currentTouched = false;
    bool previousTouched = false;
    int16_t currentTouchX = -1;
    int16_t currentTouchY = -1;
    TerminateHandler terminateHandler = nullptr;
    void* terminateContext = nullptr;

    uint32_t readButtons() override;
    void processEvents();
    void openFirstGamepad();
    void closeGamepad();
    void updateMouseTouch();
    void resetTouch();
    int16_t normalizedAxis(SDL_GamepadAxis axis) const;
};

} // namespace PRUZEA

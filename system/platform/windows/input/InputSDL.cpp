#include "InputSDL.h"

#include <algorithm>

using namespace PRUZEA;

namespace
{
bool keyDown(const bool* keys, SDL_Scancode code)
{
    return keys != nullptr && keys[code];
}
}

InputSDL::InputSDL()
    : InputSDL(Config{})
{
}

InputSDL::InputSDL(const Config& config)
    : analogDeadZone(config.analogDeadZone)
{
}

InputSDL::~InputSDL()
{
    end();
}

bool InputSDL::begin()
{
    if (!SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
    {
        available = false;
        return false;
    }

    openFirstGamepad();
    available = true;
    reset();
    resetTouch();
    return true;
}

void InputSDL::end()
{
    closeGamepad();
    available = false;
    reset();
    resetTouch();
}

void InputSDL::update()
{
    InputBase::update();
    updateMouseTouch();
}

void InputSDL::setTerminateHandler(TerminateHandler handler, void* context)
{
    terminateHandler = handler;
    terminateContext = context;
}

void InputSDL::processEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (terminateHandler != nullptr)
            {
                terminateHandler(terminateContext);
            }
            break;

        case SDL_EVENT_GAMEPAD_ADDED:
            if (gamepad == nullptr)
            {
                gamepad = SDL_OpenGamepad(event.gdevice.which);
            }
            break;

        case SDL_EVENT_GAMEPAD_REMOVED:
            if (gamepad != nullptr &&
                SDL_GetGamepadID(gamepad) == event.gdevice.which)
            {
                closeGamepad();
                openFirstGamepad();
            }
            break;

        default:
            break;
        }
    }
}

void InputSDL::openFirstGamepad()
{
    if (gamepad != nullptr) return;

    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (ids == nullptr) return;

    for (int i = 0; i < count; ++i)
    {
        gamepad = SDL_OpenGamepad(ids[i]);
        if (gamepad != nullptr) break;
    }

    SDL_free(ids);
}

void InputSDL::closeGamepad()
{
    if (gamepad != nullptr)
    {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }
}

void InputSDL::resetTouch()
{
    currentTouched = false;
    previousTouched = false;
    currentTouchX = -1;
    currentTouchY = -1;
}

void InputSDL::updateMouseTouch()
{
    previousTouched = currentTouched;
    currentTouched = false;
    currentTouchX = -1;
    currentTouchY = -1;

    if (!available) return;

    SDL_Window* window = SDL_GetMouseFocus();
    if (window == nullptr) return;

    int windowWidth = 0;
    int windowHeight = 0;
    if (!SDL_GetWindowSize(window, &windowWidth, &windowHeight) || windowWidth <= 0 || windowHeight <= 0) return;

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouseX, &mouseY);
    if ((buttons & SDL_BUTTON_LMASK) == 0 || mouseX < 0.0f || mouseY < 0.0f || mouseX >= windowWidth || mouseY >= windowHeight)
    {
        return;
    }

    currentTouched = true;
    currentTouchX = static_cast<int16_t>(std::clamp(
        static_cast<int>(mouseX * Display::ILI9341_SCREEN_W / windowWidth),
        0,
        static_cast<int>(Display::ILI9341_SCREEN_W) - 1));
    currentTouchY = static_cast<int16_t>(std::clamp(
        static_cast<int>(mouseY * Display::ILI9341_SCREEN_H / windowHeight),
        0,
        static_cast<int>(Display::ILI9341_SCREEN_H) - 1));
}

uint32_t InputSDL::readButtons()
{
    processEvents();

    const bool* keys = SDL_GetKeyboardState(nullptr);
    uint32_t result = 0;

    auto set = [&](Button button, bool pressed)
    {
        if (pressed) result |= static_cast<uint32_t>(button);
    };

    set(Button::UP,    keyDown(keys, SDL_SCANCODE_UP));
    set(Button::DOWN,  keyDown(keys, SDL_SCANCODE_DOWN));
    set(Button::LEFT,  keyDown(keys, SDL_SCANCODE_LEFT));
    set(Button::RIGHT, keyDown(keys, SDL_SCANCODE_RIGHT));

    // Nintendo-style physical layout:
    //        X
    //    Y       A
    //        B
    set(Button::A, keyDown(keys, SDL_SCANCODE_X));
    set(Button::B, keyDown(keys, SDL_SCANCODE_Z));
    set(Button::X, keyDown(keys, SDL_SCANCODE_S));
    set(Button::Y, keyDown(keys, SDL_SCANCODE_A));

    set(Button::L,      keyDown(keys, SDL_SCANCODE_Q));
    set(Button::R,      keyDown(keys, SDL_SCANCODE_W));
    set(Button::L2,     keyDown(keys, SDL_SCANCODE_1));
    set(Button::R2,     keyDown(keys, SDL_SCANCODE_2));
    set(Button::L3,     keyDown(keys, SDL_SCANCODE_3));
    set(Button::R3,     keyDown(keys, SDL_SCANCODE_4));
    set(Button::START,  keyDown(keys, SDL_SCANCODE_RETURN));
    set(Button::SELECT, keyDown(keys, SDL_SCANCODE_BACKSPACE));
    set(Button::HOME,   keyDown(keys, SDL_SCANCODE_ESCAPE));

    set(Button::VOL_UP,   keyDown(keys, SDL_SCANCODE_EQUALS) ||
                          keyDown(keys, SDL_SCANCODE_KP_PLUS));
    set(Button::VOL_DOWN, keyDown(keys, SDL_SCANCODE_MINUS) ||
                          keyDown(keys, SDL_SCANCODE_KP_MINUS));
    set(Button::MUTE,     keyDown(keys, SDL_SCANCODE_M));

    if (gamepad != nullptr)
    {
        set(Button::UP,    SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP));
        set(Button::DOWN,  SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN));
        set(Button::LEFT,  SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT));
        set(Button::RIGHT, SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT));

        set(Button::A, SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST));
        set(Button::B, SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH));
        set(Button::X, SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH));
        set(Button::Y, SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST));

        set(Button::L,  SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
        set(Button::R,  SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
        set(Button::L2, SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > analogDeadZone);
        set(Button::R2, SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > analogDeadZone);
        set(Button::L3, SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK));
        set(Button::R3, SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK));

        set(Button::START,  SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START));
        set(Button::SELECT, SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK));
        set(Button::HOME,   SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_GUIDE));
    }

    return result;
}

bool InputSDL::hasAnalogSticks() const
{
    return gamepad != nullptr;
}

int16_t InputSDL::normalizedAxis(SDL_GamepadAxis targetAxis) const
{
    if (gamepad == nullptr) return 0;

    int value = SDL_GetGamepadAxis(gamepad, targetAxis);
    if (value > -analogDeadZone && value < analogDeadZone) return 0;

    if (value < 0)
    {
        value = ((value + analogDeadZone) * 1000) / (32768 - analogDeadZone);
    }
    else
    {
        value = ((value - analogDeadZone) * 1000) / (32767 - analogDeadZone);
    }

    return static_cast<int16_t>(std::clamp(value, -1000, 1000));
}

int16_t InputSDL::axis(Axis targetAxis) const
{
    switch (targetAxis)
    {
    case Axis::LEFT_X:  return normalizedAxis(SDL_GAMEPAD_AXIS_LEFTX);
    case Axis::LEFT_Y:  return normalizedAxis(SDL_GAMEPAD_AXIS_LEFTY);
    case Axis::RIGHT_X: return normalizedAxis(SDL_GAMEPAD_AXIS_RIGHTX);
    case Axis::RIGHT_Y: return normalizedAxis(SDL_GAMEPAD_AXIS_RIGHTY);
    default:            return 0;
    }
}

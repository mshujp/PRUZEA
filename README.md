# PRUZEA

> **AI-Friendly Game Framework**

A lightweight game framework designed for AI-assisted game development.


> [!TIP]
> Looking for an Arduino IDE version?
> [PRUZEA mini](https://github.com/mshujp/PRUZEAmini) is designed for single games and UI applications, while PRUZEA provides a complete multi-game system with Raspberry Pi Pico targets and a Windows development simulator.

------------------------------------------------------------------------

# Features

-   AI-friendly public API
-   Portable game code across supported platforms
-   Unified Graphics / Input / Audio / Storage APIs
-   Fixed 30 FPS game loop
-   Built-in SaveData helper
-   2D camera, zoom, clipping, viewport, and scrolling support
-   2D game utilities for vectors, animation, tweening, math, and collision detection
-   SpriteSheet rendering support
-   PWM / I2S audio support
-   ILI9341 (SPI / Parallel) and SSD1306 display support
-   AI-oriented documentation and API design
-   Embedded JPEG / PNG image support
-   RGB565 sprite scaling, rotation, flipping, and color-key transparency
-   ToneNote and embedded SMF Format 0 / 1 MIDI playback
-   Simultaneous background music and sound-effect mixing
-   GPIO buttons, gamepads, analog sticks, and touchscreen input
-   Windows development simulator using SDL3

## Supported Platforms

- Raspberry Pi Pico family
  - RP2040
  - RP2350
- Windows (SDL3) — development simulator
- ESP32 family — planned
  

| Hardware |  |
| :---: | :---: |
| ![](docs/images/01.jpg) | ![](docs/images/02.jpg) |
| ![](docs/images/03.jpg) | ![](docs/images/04.jpg) |

| Screenshots | |
| :---: | :---: |
| ![](docs/images/ss01.png) | ![](docs/images/ss02.png) |
| ![](docs/images/ss03.png) | ![](docs/images/ss04.png) |
| ![](docs/images/ss05.png) | ![](docs/images/ss06.png) |
| ![](docs/images/ss07.png) | ![](docs/images/ss08.png) |
| ![](docs/images/ss09.png) | ![](docs/images/ss10.png) |
| ![](docs/images/ss11.png) | ![](docs/images/ss12.png) |
| ![](docs/images/ss13.png) | ![](docs/images/ss14.png) |
| ![](docs/images/ss15.png) | ![](docs/images/ss16.png) | 
| ![](docs/images/ss17.png) | ![](docs/images/ss18.png) |
| ![](docs/images/ss19.png) | ![](docs/images/ss20.png) |
| ![](docs/images/ss21.png) | |

------------------------------------------------------------------------

# Philosophy

PRUZEA is designed so that both humans and AI can write games using the
same simple API.

Games implement only a small set of interfaces while the runtime manages
graphics, input, audio, storage, and the game loop.

This allows game logic to remain clean, portable, and easy to generate.

------------------------------------------------------------------------

# Sample Games

| Sample | Description |
|--------|-------------|
| [01 PRUZEA APIs](samples/PruzeaAPIs/) | Learn the basic APIs |
| [02 Collision Lab](samples/CollisionLab/) | Collision detection |
| [03 Sound Tile](samples/SoundTile/) | Audio and input |
| [04](samples/ParticleLab/) [05](samples/FireEffect/) [06](samples/WaterRipple/) Graphics Effects | Animation techniques |
| [07 Sprite Adventure](samples/SpriteAdventure/) | Sprite rendering |
| [08](samples/TinyStarfield/) [09](samples/WireframeTunnel/) [10](samples/Software3D/) 3D Samples | Advanced rendering |
| [11 SL](samples/SL/) | Bonus sample |
| [12 Touch Paint](samples/TouchPaint/) | Touchscreen |
| [13 Analog Stick](samples/AnalogStick/) | Analog Stick Input |
| [14 Image Gallery](samples/ImageGallery/) | JPEG and PNG image rendering |
| [15 Midi Music Box](samples/MidiMusicBox/) | Embedded SMF Format 0 / 1 MIDI playback |
| [16 Maze Escape](samples/MazeEscape/) | Advanced gameplay and visual effects |
| [Game Template](samples/GameTemplate/) | Empty project template |

Each sample is placed under the [`samples`](samples) directory.

## Learning Path

The samples are intended to be completed in numerical order.
Each sample introduces one or more new concepts while building on previous examples.

------------------------------------------------------------------------

# Build

## Raspberry Pi Pico

### Build

Build the project with CMake:

```sh
cmake -S . -B build
cmake --build build
```

### Deployment

After building, the generated firmware can be found at:

```text
build/system/pruzea.uf2
```

Copy the UF2 file to a board in BOOTSEL mode to install the firmware.

VS Code tasks or custom scripts can also be used to automate the deployment process.

## Windows Simulator

PRUZEA includes a Windows simulator based on SDL3.

It is intended for developing and testing PRUZEA games on a PC before running them on Raspberry Pi Pico hardware. The same game source and PRUZEA API can be used on both targets.

The simulator provides:

- Graphics rendering in an SDL3 window
- Keyboard input
- SDL-compatible game controller input
- Analog stick input
- Mouse input as touchscreen input
- Audio and MIDI playback
- Storage access

The simulator reproduces PRUZEA graphics, input, audio, storage, and game-loop behavior, but it does not emulate RP2040 or RP2350 performance characteristics.

### SDL3

Place the complete SDL3 release source under:

```text
PRUZEA/lib/SDL/
```

### VS Code

When using VS Code with CMake Tools:

1. Press `Ctrl + Shift + P`.
2. Select `CMake: Select Configure Preset`.
3. Select `PRUZEA Windows Simulator`.
4. Build the project using the **Build** button in VS Code.
5. Start PRUZEA using the **Run (▶)** button.

If CMake needs to be reconfigured, right-click the root `CMakeLists.txt` and use the CMake configure/reconfigure command.

### Command Line

The Windows simulator can also be configured and built from the command line:

```sh
cmake --preset windows-simulator
cmake --build --preset windows-simulator-build
```

### Keyboard Controls

| PRUZEA Input | Keyboard |
|---|---|
| D-Pad | Arrow keys |
| A | X |
| B | Z |
| X | S |
| Y | A |
| L | Q |
| R | W |
| L2 | 1 |
| R2 | 2 |
| L3 | 3 |
| R3 | 4 |
| START | Enter |
| SELECT | Backspace |
| HOME | Esc |
| Volume Up | `=` / Numpad `+` |
| Volume Down | `-` / Numpad `-` |
| Mute | M |

SDL-compatible game controllers can also be used. D-pad, face buttons, shoulder buttons, triggers, stick buttons, Start/Select, and analog sticks are supported.

The left mouse button is treated as touchscreen input, with the mouse position converted to PRUZEA screen coordinates.

------------------------------------------------------------------------

# Creating a Game

To create a game, simply create **one class** that inherits from the `PRUZEA::Game` class.

The PRUZEA system automatically manages the game loop, rendering, input, audio, and storage.

Your game only needs to implement its own game logic.

## Core API

PRUZEA provides the following hardware abstraction interfaces to every game.

Game code does not need to access platform-specific hardware or drivers directly.

| Class | Purpose |
|------|---------|
| `PRUZEA::Graphics` | Drawing API for text, shapes, images, and sprites. |
| `PRUZEA::Input` | Controller input, button state, repeat, and hold detection. |
| `PRUZEA::Audio` | Play sound effects and music. |
| `PRUZEA::Storage` | Read and write save data and configuration files. |

For the complete API reference, see:

- [`sdk/PRUZEA.h`](sdk/PRUZEA.h)

## `PRUZEA::Game` class

Your game class should inherit from the `PRUZEA::Game` class.

Most games implement their game logic in:

- `onInit()`
- `onUpdate()`
- `onDraw()`
- `onTerminate()`

Other required virtual functions provide game metadata, such as the game name and ID.

For the complete `PRUZEA::Game` class reference, see:

- [`sdk/PRUZEA.h`](sdk/PRUZEA.h)

## Project Structure

Each game is placed under the [`games`](games) directory.

The directory name and the game class name must match.

```text
games/
└── MyGame/
    ├── MyGame.h
    └── MyGame.cpp
```

After adding a new game, reconfigure CMake and build the project.

## Resource Files

### Image
- Embedded JPEG, PNG, and RGB565 images are supported.
- Sprites support scaling, rotation, flipping, and color-key transparency.

### Audio
- ToneNote sound effects, ToneNote music, and MIDI music are supported.
- ToneNote music and MIDI music can play simultaneously with sound effects.
- Embedded MIDI supports SMF Format 0 and Format 1.

## AI Workflow

PRUZEA is designed for AI-assisted game development.

Provide only the SDK files listed below.
Do not provide platform-specific source files.

1. Edit [`sdk/PRUZEA_GAME_DESIGN_TEMPLATE.md`](sdk/PRUZEA_GAME_DESIGN_TEMPLATE.md) to describe your game.
2. Upload the following SDK files to your AI chat:

  - [sdk/PRUZEA.h](sdk/PRUZEA.h)
  - [sdk/PRUZEA_AI_GUIDELINES.md](sdk/PRUZEA_AI_GUIDELINES.md)
  - [sdk/PRUZEA_GAME_DESIGN_TEMPLATE.md](sdk/PRUZEA_GAME_DESIGN_TEMPLATE.md)

    If your AI does not support file uploads, copy and paste the file contents into the chat instead.

3. Discuss the game design with the AI.
4. Let the AI generate the game source files.
5. Add the generated files to the [`games`](games) directory, reconfigure CMake, and build the project.

------------------------------------------------------------------------

# Recommended AI

PRUZEA is designed to work with modern AI coding assistants.

Based on current development experience:

| AI | Recommendation | Notes |
|----|---------------|-------|
| **ChatGPT** | **Highly Recommended** | Best overall experience with PRUZEA |
| **Claude** | **Recommended** | Strong at understanding the SDK and generating well-structured game code |
| **Gemini** | **Recommended** | Works well for most tasks |
| **Copilot** | **Best for code completion** | Less suitable for full game generation |
| **Google Search AI Mode** | **Not Recommended** | Does not currently support file uploads, making it difficult to provide the PRUZEA SDK. |

------------------------------------------------------------------------

# Hardware Notes

## Minimum Configuration

- **Main board:** RP2040  
  Examples: Waveshare RP2040 Zero
- **Input:** About 7 tactile switches  
  D-pad, A, B, and Home
- **Display:** SSD1306
- **Audio:** PWM  
  Add a potentiometer for volume adjustment if needed.

## Standard Configuration

- **Main board:** RP2350  
  Examples: Raspberry Pi Pico 2
- **Input:** About 7 tactile switches  
  D-pad, A, B, and Home
  (Optional: X, Y, Start, Select, L, R, VolumeUp, VolumeDown and Mute)
- **Display:** ILI9341
- **Audio:** I2S
- **Storage:** SD card reader

## Pin Assignment Advice

Ask an AI assistant to read this page, then describe your hardware configuration and request advice on suitable pin assignments.

## GPIO BUTTONS

Internal pull-up resistors are used.

## SD Card SPI

For SD card builds, the following configuration is recommended and has been verified on both RP2040 and RP2350.

| Peripheral | SPI |
|------------|-----|
| ILI9341 LCD | SPI1 |
| SD Card | SPI0 |

This configuration has been verified on both RP2040 and RP2350 and is recommended for best compatibility.

### SD Card

> [!IMPORTANT]
> PRUZEA supports **SDHC** and **SDXC** memory cards.
> Standard **SD cards (2GB and smaller)** are **not supported**.

> [!WARNING]
> Although PRUZEA provides a software shutdown option, embedded systems can still lose power unexpectedly (for example, due to battery removal or depletion).
> Do **not** store important or irreplaceable data on the SD card.

## PWM Audio

PWM audio supports only **MUTE** or **ON**.
If adjustable volume is required, use an external amplifier or a potentiometer.

## Touchscreen

Touchscreen input is an optional extension to the primary button input.
It is intended for secondary-screen-style interaction and does not replace system menu controls.

PRUZEA supports ILI9341 display modules with an integrated XPT2046 touchscreen controller.

For builds that use both a touchscreen and an SD card, the following configuration is recommended:

| Peripheral | SPI |
|------------|-----|
| ILI9341 LCD | SPI1 |
| XPT2046 Touchscreen | SPI1 |
| SD Card | SPI0 |

The touchscreen and ILI9341 card may share the SPI1 clock, MOSI, and MISO pins.
They must use separate CS pins.

The IRQ pin is optional.
Set `irqPin` to `-1` when it is not connected.
In that case, PRUZEA detects touch input by polling the XPT2046 controller.

## Analog Stick

PRUZEA supports the analog sticks of PlayStation 1 and PlayStation 2 controllers.

Analog stick values are normalized to the range `-1000` to `1000`.

```cpp
const int16_t moveX = input.axis(Input::Axis::LEFT_X);
const int16_t moveY = input.axis(Input::Axis::LEFT_Y);
```

------------------------------------------------------------------------

# Project Layout

``` text
PRUZEA/
├── sdk/
├── games/
├── system/
├── samples/
├── scripts/
├── lib/
│   ├── SDL/
│   ├── LovyanGFX/
│   ├── pico-extras/
│   └── no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/
└── ...
```

------------------------------------------------------------------------

# Supported Hardware

The following hardware configurations have been verified with PRUZEA.

| Platform | ILI9341 SPI | ILI9341 Parallel | SSD1306 | PWM Audio | I2S Audio | GPIO Buttons | SNES Pad | PS Pad | SD Card |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| RP2040 | ✅ | ✅ | ✅ | ✅ |  | ✅ |  |  | ✅ |
| RP2350 | ✅ |  |  |  | ✅ | ✅ | ✅ | ✅  | ✅ |

- ✅: Verified on actual hardware
- Blank: Not yet tested. A blank cell does **not** mean unsupported or incompatible.
  Some untested combinations may already be supported by the implementation, but they have not yet been verified on physical hardware.
- PS Pad: PlayStation 1/2 controller with digital buttons and analog stick support.

Additional hardware configurations can be supported by creating a new hardware profile under:

```text
system/platform/pico/boards/
```

Only the configurations listed above have been verified.

------------------------------------------------------------------------

# License

MIT License

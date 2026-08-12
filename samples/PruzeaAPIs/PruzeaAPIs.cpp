#include "PruzeaAPIs.h"

#include <cstdio>
#include <cstring>

using PRUZEA::Audio;
using PRUZEA::Graphics;
using PRUZEA::Input;
using PRUZEA::SaveData;
namespace Platform = PRUZEA::Platform;
using PRUZEA::Storage;

namespace {
constexpr uint16_t SCREEN_W = 320;
constexpr uint16_t SCREEN_H = 240;
constexpr uint32_t DRAW_STEP_MSEC = 5000;
constexpr uint32_t DRAW_HOLD_MSEC = 1000;
constexpr uint8_t DRAW_STEP_COUNT = 27;
constexpr uint8_t SE_COUNT = 12;
constexpr uint8_t MUSIC_COUNT = 3;
constexpr int16_t UI_SAFE_BOTTOM = 224;

constexpr Graphics::Color COL_BG = Graphics::rgb565(6, 12, 22);
constexpr Graphics::Color COL_PANEL = Graphics::rgb565(14, 28, 48);
constexpr Graphics::Color COL_PANEL_2 = Graphics::rgb565(22, 42, 68);
constexpr Graphics::Color COL_LINE = Graphics::rgb565(62, 124, 170);
constexpr Graphics::Color COL_DIM = Graphics::rgb565(86, 112, 136);
constexpr Graphics::Color COL_TEXT = Graphics::rgb565(220, 238, 248);
constexpr Graphics::Color COL_ACCENT = Graphics::rgb565(70, 220, 255);
constexpr Graphics::Color COL_GREEN = Graphics::rgb565(80, 240, 140);
constexpr Graphics::Color COL_WARN = Graphics::rgb565(255, 188, 68);
constexpr Graphics::Color COL_DANGER = Graphics::rgb565(255, 82, 94);
constexpr Graphics::Color COL_PURPLE = Graphics::rgb565(180, 110, 255);

const char* const DRAW_STEP_NAMES[DRAW_STEP_COUNT] = {
    "fillScreen",
    "drawPixel",
    "drawLine",
    "drawTriangle",
    "fillTriangle",
    "drawRect",
    "drawRoundRect",
    "drawRect thick",
    "drawRoundRect thick",
    "fillRect",
    "fillRectAlpha",
    "fillRoundRect",
    "drawCircle",
    "drawCircle ellipse",
    "fillCircle",
    "fillCircle ellipse",
    "Arc",
    "Gradient",
    "Font",
    "Alignment",
    "Text Metrics",
    "Viewport / Shake",
    "Camera",
    "Clip Rect",
    "Sprite",
    "Sprite Transform",
    "SpriteSheet"
};

const Audio::Sound* SE_LIST[SE_COUNT] = {
    &Audio::SE::NO_1, &Audio::SE::NO_2, &Audio::SE::NO_3, &Audio::SE::NO_4,
    &Audio::SE::NO_5, &Audio::SE::NO_6, &Audio::SE::NO_7, &Audio::SE::NO_8,
    &Audio::SE::NO_9, &Audio::SE::NO_10, &Audio::SE::NO_11, &Audio::SE::NO_12
};

const char* const SE_NAMES[SE_COUNT] = {
    "NO_1", "NO_2", "NO_3", "NO_4",
    "NO_5", "NO_6", "NO_7", "NO_8",
    "NO_9", "NO_10", "NO_11", "NO_12"
};

const Audio::ToneNote MUSIC_A_NOTES[] = {
    {Audio::ToneNote::A5,   Audio::ToneNote::S}, {Audio::ToneNote::G5,   Audio::ToneNote::S}, {Audio::ToneNote::F5,   Audio::ToneNote::S}, {Audio::ToneNote::E5,   Audio::ToneNote::S},
    {Audio::ToneNote::D5,   Audio::ToneNote::E}, {Audio::ToneNote::REST, Audio::ToneNote::E}, {Audio::ToneNote::CS4,  Audio::ToneNote::E}, {Audio::ToneNote::D4,   Audio::ToneNote::E},
    {Audio::ToneNote::E4,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::E}, {Audio::ToneNote::D5,   Audio::ToneNote::S}, {Audio::ToneNote::E5,   Audio::ToneNote::S},
    {Audio::ToneNote::F5,   Audio::ToneNote::S}, {Audio::ToneNote::G5,   Audio::ToneNote::S}, {Audio::ToneNote::A5,   Audio::ToneNote::E}, {Audio::ToneNote::REST, Audio::ToneNote::E},
    {Audio::ToneNote::F5,   Audio::ToneNote::E}, {Audio::ToneNote::E5,   Audio::ToneNote::E}, {Audio::ToneNote::D5,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::E}
};

const Audio::ToneNote MUSIC_B_NOTES[] = {
    {Audio::ToneNote::D3,   Audio::ToneNote::Q}, {Audio::ToneNote::D3,   Audio::ToneNote::E}, {Audio::ToneNote::D3,   Audio::ToneNote::E}, {Audio::ToneNote::D3,   Audio::ToneNote::Q},
    {Audio::ToneNote::D3,   Audio::ToneNote::E}, {Audio::ToneNote::D3,   Audio::ToneNote::E}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::D2,   Audio::ToneNote::Q}, {Audio::ToneNote::D2,   Audio::ToneNote::E}, {Audio::ToneNote::D2,   Audio::ToneNote::E}, {Audio::ToneNote::D2,   Audio::ToneNote::Q},
    {Audio::ToneNote::D2,   Audio::ToneNote::E}, {Audio::ToneNote::D2,   Audio::ToneNote::E}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::A3,   Audio::ToneNote::Q}, {Audio::ToneNote::A3,   Audio::ToneNote::E}, {Audio::ToneNote::A3,   Audio::ToneNote::E}, {Audio::ToneNote::A3,   Audio::ToneNote::Q},
    {Audio::ToneNote::A3,   Audio::ToneNote::E}, {Audio::ToneNote::A3,   Audio::ToneNote::E}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::A2,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::D5,   Audio::ToneNote::Q}, {Audio::ToneNote::F5,   Audio::ToneNote::Q}, {Audio::ToneNote::G5,   Audio::ToneNote::Q}, {Audio::ToneNote::F5,   Audio::ToneNote::E},
    {Audio::ToneNote::D5,   Audio::ToneNote::E}, {Audio::ToneNote::F5,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::F5,   Audio::ToneNote::Q}, {Audio::ToneNote::G5,   Audio::ToneNote::Q}, {Audio::ToneNote::A5,   Audio::ToneNote::Q}, {Audio::ToneNote::G5,   Audio::ToneNote::E},
    {Audio::ToneNote::F5,   Audio::ToneNote::E}, {Audio::ToneNote::G5,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::A5,   Audio::ToneNote::Q}, {Audio::ToneNote::C6,   Audio::ToneNote::Q}, {Audio::ToneNote::A5,   Audio::ToneNote::Q}, {Audio::ToneNote::G5,   Audio::ToneNote::E},
    {Audio::ToneNote::F5,   Audio::ToneNote::E}, {Audio::ToneNote::D5,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::F5,   Audio::ToneNote::Q}, {Audio::ToneNote::D5,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::C5,   Audio::ToneNote::Q}, {Audio::ToneNote::D5,   Audio::ToneNote::H}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::D3,   Audio::ToneNote::E}, {Audio::ToneNote::D5,   Audio::ToneNote::E}, {Audio::ToneNote::D3,   Audio::ToneNote::E}, {Audio::ToneNote::D5,   Audio::ToneNote::E},
    {Audio::ToneNote::D3,   Audio::ToneNote::Q}, {Audio::ToneNote::F5,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::D3,   Audio::ToneNote::E}, {Audio::ToneNote::F5,   Audio::ToneNote::E}, {Audio::ToneNote::D3,   Audio::ToneNote::E}, {Audio::ToneNote::G5,   Audio::ToneNote::E},
    {Audio::ToneNote::D3,   Audio::ToneNote::Q}, {Audio::ToneNote::A5,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::A5,   Audio::ToneNote::Q}, {Audio::ToneNote::G5,   Audio::ToneNote::Q}, {Audio::ToneNote::F5,   Audio::ToneNote::Q}, {Audio::ToneNote::D5,   Audio::ToneNote::Q},
    {Audio::ToneNote::F5,   Audio::ToneNote::Q}, {Audio::ToneNote::G5,   Audio::ToneNote::Q}, {Audio::ToneNote::A5,   Audio::ToneNote::H}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::A5,   Audio::ToneNote::Q}, {Audio::ToneNote::F5,   Audio::ToneNote::Q}, {Audio::ToneNote::D5,   Audio::ToneNote::Q}, {Audio::ToneNote::REST, Audio::ToneNote::Q},
    {Audio::ToneNote::REST, Audio::ToneNote::W}
};

const Audio::ToneNote MUSIC_C_NOTES[] = {
    {Audio::ToneNote::F4,  Audio::ToneNote::E}, {Audio::ToneNote::AS4, Audio::ToneNote::E}, {Audio::ToneNote::CS5, Audio::ToneNote::Q}, {Audio::ToneNote::F5,  Audio::ToneNote::Q},
    {Audio::ToneNote::DS5, Audio::ToneNote::E}, {Audio::ToneNote::CS5, Audio::ToneNote::E}, {Audio::ToneNote::AS4, Audio::ToneNote::Q}, {Audio::ToneNote::GS4, Audio::ToneNote::Q},
    {Audio::ToneNote::AS4, Audio::ToneNote::Q}, {Audio::ToneNote::CS5, Audio::ToneNote::E}, {Audio::ToneNote::DS5, Audio::ToneNote::E}, {Audio::ToneNote::F5,  Audio::ToneNote::Q},
    {Audio::ToneNote::FS5, Audio::ToneNote::E}, {Audio::ToneNote::F5,  Audio::ToneNote::E}, {Audio::ToneNote::DS5, Audio::ToneNote::Q}, {Audio::ToneNote::CS5, Audio::ToneNote::Q},
    {Audio::ToneNote::F5,  Audio::ToneNote::Q}, {Audio::ToneNote::GS5, Audio::ToneNote::E}, {Audio::ToneNote::FS5, Audio::ToneNote::E}, {Audio::ToneNote::F5,  Audio::ToneNote::Q},
    {Audio::ToneNote::DS5, Audio::ToneNote::E}, {Audio::ToneNote::CS5, Audio::ToneNote::E}, {Audio::ToneNote::AS4, Audio::ToneNote::Q}, {Audio::ToneNote::GS4, Audio::ToneNote::Q},
    {Audio::ToneNote::CS5, Audio::ToneNote::Q}, {Audio::ToneNote::DS5, Audio::ToneNote::E}, {Audio::ToneNote::CS5, Audio::ToneNote::E}, {Audio::ToneNote::AS4, Audio::ToneNote::Q}, 
    {Audio::ToneNote::GS4, Audio::ToneNote::E}, {Audio::ToneNote::F4,  Audio::ToneNote::E}, {Audio::ToneNote::AS4, Audio::ToneNote::H}
};

const Audio::Music MUSIC_LOOP[MUSIC_COUNT] = {
    {MUSIC_A_NOTES, 130, static_cast<uint16_t>(sizeof(MUSIC_A_NOTES) / sizeof(MUSIC_A_NOTES[0])), 0, 0.55f},
    {MUSIC_B_NOTES, 140, static_cast<uint16_t>(sizeof(MUSIC_B_NOTES) / sizeof(MUSIC_B_NOTES[0])), 0, 0.50f},
    {MUSIC_C_NOTES, 167, static_cast<uint16_t>(sizeof(MUSIC_C_NOTES) / sizeof(MUSIC_C_NOTES[0])), 0, 0.50f}
};

const Audio::Music MUSIC_ONCE[MUSIC_COUNT] = {
    {MUSIC_A_NOTES, 167, static_cast<uint16_t>(sizeof(MUSIC_A_NOTES) / sizeof(MUSIC_A_NOTES[0])), 1, 0.55f},
    {MUSIC_B_NOTES, 200, static_cast<uint16_t>(sizeof(MUSIC_B_NOTES) / sizeof(MUSIC_B_NOTES[0])), 1, 0.50f},
    {MUSIC_C_NOTES, 167, static_cast<uint16_t>(sizeof(MUSIC_C_NOTES) / sizeof(MUSIC_C_NOTES[0])), 1, 0.50f}
};

const char* const MUSIC_NAMES[MUSIC_COUNT] = {
    "Music A", "Music B", "Music C"
};

static const uint16_t DUMMY_SPRITE[16 * 16] = {
    0x0000,0x0000,0x0000,0x07FF,0x07FF,0x0000,0x0000,0x0000,0x0000,0x0000,0x07FF,0x07FF,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x07FF,0xFFFF,0xFFFF,0x07FF,0x0000,0x0000,0x0000,0x07FF,0xFFFF,0xFFFF,0x07FF,0x0000,0x0000,0x0000,
    0x0000,0x07FF,0xFFFF,0xFFE0,0xFFFF,0xFFFF,0x07FF,0x0000,0x07FF,0xFFFF,0xFFFF,0xFFE0,0xFFFF,0x07FF,0x0000,0x0000,
    0x07FF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x07FF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x07FF,0x0000,
    0x07FF,0xFFFF,0x001F,0xFFFF,0xFFFF,0xFFFF,0x001F,0xFFFF,0xFFFF,0x001F,0xFFFF,0xFFFF,0xFFFF,0x001F,0xFFFF,0x07FF,
    0x07FF,0xFFFF,0xFFFF,0xFFFF,0xF81F,0xF81F,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xF81F,0xF81F,0xFFFF,0xFFFF,0xFFFF,0x07FF,
    0x0000,0x07FF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x07FF,0x0000,
    0x0000,0x0000,0x07FF,0xFFFF,0xFFFF,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xFFFF,0xFFFF,0x07FF,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x07FF,0xFFFF,0xFFFF,0xF800,0xF800,0xF800,0xF800,0xFFFF,0xFFFF,0x07FF,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x07FF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x07FF,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x07FF,0xFFFF,0x07FF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x07FF,0xFFFF,0x07FF,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x07FF,0xFFFF,0x07FF,0x0000,0x07FF,0x07FF,0x07FF,0x07FF,0x0000,0x07FF,0xFFFF,0x07FF,0x0000,0x0000,
    0x0000,0x07FF,0xFFFF,0x07FF,0x0000,0x0000,0x0000,0x07FF,0x07FF,0x0000,0x0000,0x0000,0x07FF,0xFFFF,0x07FF,0x0000,
    0x07FF,0xFFFF,0x07FF,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x07FF,0xFFFF,0x07FF,
    0x07FF,0x07FF,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x07FF,0x07FF,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
};

constexpr Graphics::Color _BK = Graphics::rgb565(0, 0, 0);          // BLACK
constexpr Graphics::Color _WT = Graphics::rgb565(255, 255, 255);    // WHITE
constexpr Graphics::Color _GR = Graphics::rgb565(132, 132, 130);    // GRAY
constexpr Graphics::Color _DG = Graphics::rgb565(66, 66, 66);       // DARKGRAY
constexpr Graphics::Color _RD = Graphics::rgb565(248, 0, 0);        // RED
constexpr Graphics::Color _YL = Graphics::rgb565(255, 255, 0);      // YELLOW
constexpr Graphics::Color _BL = Graphics::rgb565(0, 0, 248);        // BLUE
constexpr Graphics::Color _GN = Graphics::rgb565(0, 248, 0);        // GREEN
constexpr Graphics::Color _CN = Graphics::rgb565(0, 255, 255);      // CYAN
constexpr Graphics::Color _MG = Graphics::rgb565(248, 0, 248);      // MAGENTA (透過色)
constexpr Graphics::Color _OR = Graphics::rgb565(250, 160, 0);      // ORANGE
constexpr Graphics::Color _BR = Graphics::rgb565(160, 80, 40);       // BROWN
constexpr Graphics::Color ___ = _MG;

static const uint16_t DUMMY_SPRITE_SHEET_DATA[64 * 64] = {
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_GR,_GR,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_GR,_MG,_MG,_MG,_GR,_GR,_MG,_MG,_MG,_GR,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_OR,_OR,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_MG,_MG,  _MG,_MG,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_MG,  _MG,_MG,_MG,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,  _RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,  _MG,_MG,_MG,_GR,_GR,_GR,_GR,_MG,_MG,_GR,_GR,_GR,_GR,_MG,_MG,_MG,  _MG,_MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,_MG,
    _MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,  _RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,  _MG,_MG,_MG,_GR,_GR,_GR,_MG,_MG,_MG,_MG,_GR,_GR,_GR,_MG,_MG,_MG,  _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,
    _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,  _MG,_GR,_GR,_GR,_GR,_MG,_MG,_MG,_MG,_MG,_MG,_GR,_GR,_GR,_GR,_MG,  _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,
    _MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,  _RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,  _MG,_GR,_GR,_GR,_GR,_MG,_MG,_MG,_MG,_MG,_MG,_GR,_GR,_GR,_GR,_MG,  _MG,_MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,  _MG,_MG,_MG,_GR,_GR,_GR,_MG,_MG,_MG,_MG,_GR,_GR,_GR,_MG,_MG,_MG,  _MG,_MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,  _MG,_MG,_MG,_GR,_GR,_GR,_GR,_MG,_MG,_GR,_GR,_GR,_GR,_MG,_MG,_MG,  _MG,_MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_YL,_YL,_YL,_MG,_MG,_YL,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,  _MG,_MG,_MG,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_MG,_MG,_MG,  _MG,_MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_GR,_MG,_MG,  _MG,_MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_GR,_MG,_MG,_MG,_GR,_GR,_MG,_MG,_MG,_GR,_MG,_MG,_MG,  _MG,_MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_OR,_OR,_OR,_OR,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_GR,_GR,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_OR,_OR,_OR,_MG,_MG,_MG,_MG,_OR,_OR,_OR,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,

    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_MG,_MG,  _MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,  _MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,  _MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_GN,_GN,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_GN,_GN,_GN,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_GN,_GN,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_RD,_RD,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_RD,_RD,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_GN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_RD,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_RD,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,

    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_CN,_CN,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,
    _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_MG,
    _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_CN,_CN,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,
    _MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_CN,_CN,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,
    _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,  _MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_CN,_CN,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,
    _MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_WT,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_CN,_CN,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_CN,_MG,
    _MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_WT,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,  _MG,_MG,_CN,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_CN,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_YL,_MG,_MG,_MG,  _MG,_MG,_MG,_CN,_CN,_CN,_CN,_WT,_WT,_CN,_CN,_CN,_CN,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_YL,_YL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_CN,_CN,_CN,_CN,_CN,_CN,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,

    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_GR,_GR,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_GR,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_BL,_BL,_BL,_BL,_BL,_BL,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_BL,_BL,_BL,_BL,_MG,_MG,_BL,_BL,_BL,_BL,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,  _MG,_MG,_BL,_BL,_BL,_MG,_MG,_BL,_BL,_MG,_MG,_BL,_BL,_BL,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_DG,_DG,_DG,_DG,_DG,_DG,_WT,_WT,_MG,_MG,_MG,  _MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,  _MG,_MG,_BL,_BL,_MG,_BL,_BL,_BL,_BL,_BL,_BL,_MG,_BL,_BL,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,  _MG,_MG,_BL,_MG,_BL,_BL,_MG,_MG,_MG,_MG,_BL,_BL,_MG,_BL,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_DG,_DG,_DG,_DG,_DG,_DG,_WT,_WT,_MG,_MG,_MG,  _MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,  _MG,_MG,_MG,_MG,_BL,_MG,_MG,_MG,_MG,_MG,_MG,_BL,_MG,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_DG,_DG,_DG,_DG,_DG,_DG,_WT,_WT,_MG,_MG,_MG,  _MG,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_GN,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_DG,_DG,_DG,_DG,_DG,_DG,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_OR,_MG,_MG,  _MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_BL,_BL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_WT,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_BL,_BL,_MG,_MG,_MG,_MG,_MG,_MG,_MG,
    _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,  _MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG,_MG
};

static const Graphics::SpriteSheet DUMMY_SPRITE_SHEET = {
    .bitmap = DUMMY_SPRITE_SHEET_DATA,
    .spriteWidth = 16,
    .spriteHeight = 16,
    .columns = 4,
    .rows = 4
};

void drawTextBox(Graphics& g, int16_t x, int16_t y, int16_t w, int16_t h, const char* text, bool active) {
    g.fillRoundRect(x, y, w, h, 8, active ? COL_PANEL_2 : COL_PANEL);
    g.drawRoundRect(x, y, w, h, 8, active ? 3 : 1, active ? COL_ACCENT : COL_LINE);
    g.drawString(text, x + w / 2, y + h / 2, active ? COL_ACCENT : COL_TEXT,
                 Graphics::SIZE_18, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void drawGrid(Graphics& g) {
    for (int16_t x = 0; x < SCREEN_W; x += 20) {
        g.drawLine(x, 0, x, SCREEN_H - 1, Graphics::rgb565(10, 24, 38));
    }
    for (int16_t y = 0; y < SCREEN_H; y += 20) {
        g.drawLine(0, y, SCREEN_W - 1, y, Graphics::rgb565(10, 24, 38));
    }
}
}

const char* PruzeaAPIs::getId() const { return "pruzeaapis_sample"; }
const char* PruzeaAPIs::getName() const { return "PRUZEA APIs"; }
const char* PruzeaAPIs::getMenuName() const{ return "01 PRUZEA APIs"; }
uint16_t PruzeaAPIs::getLogicalScreenWidth() const { return 320; }
uint16_t PruzeaAPIs::getLogicalScreenHeight() const { return 240; }
uint16_t PruzeaAPIs::getTargetScreenWidth() const { return SCREEN_W; }
uint16_t PruzeaAPIs::getTargetScreenHeight() const { return SCREEN_H; }

void PruzeaAPIs::onInit(Storage& storage) {
    (void)storage;
    mode = Mode::TITLE;
    titleIndex = 0;
    drawStep = 0;
    lastDrawStep = 0;
    modeStartMsec = Platform::getMsec();
    stepStartMsec = modeStartMsec;
    lastVisualMsec = 0;
    inputMask = 0;
    audioRow = AudioRow::SE;
    seIndex = 0;
    musicIndex = 0;
    storageWriteValue = 0;
    storageReadValue = 0;
    storageAvailable = false;
    storageWriteOk = false;
    storageReadOk = false;
    storageMatch = false;
    saveDataSetOk = false;
    saveDataSaveOk = false;
    saveDataLoadOk = false;
    saveDataMatch = false;
    saveDataEntryCount = 0;
    saveDataUsedBytes = 0;
}

PRUZEA::Game::GameState PruzeaAPIs::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) {
    switch (mode) {
        case Mode::TITLE: updateTitle(input, audio, storage); break;
        case Mode::DRAW_TEST: updateDrawTest(input, audio); break;
        case Mode::INPUT_TEST: updateInputTest(input, audio); break;
        case Mode::AUDIO_TEST: updateAudioTest(input, audio); break;
        case Mode::STORAGE_TEST: updateStorageTest(input, audio, storage); break;
    }
    return GameState::RUNNING;
}

bool PruzeaAPIs::onDraw(Graphics& graphics, bool requestFullRedraw) {
    if (!requestFullRedraw && !dirty) {
        return false;
    }

    graphics.resetViewport();
    graphics.resetCamera();
    graphics.resetClipRect();
    bool drew = false;
    switch (mode) {
        case Mode::TITLE: drew = drawTitle(graphics); break;
        case Mode::DRAW_TEST: drew = drawDrawTest(graphics, requestFullRedraw); break;
        case Mode::INPUT_TEST: drew = drawInputTest(graphics); break;
        case Mode::AUDIO_TEST: drew = drawAudioTest(graphics); break;
        case Mode::STORAGE_TEST: drew = drawStorageTest(graphics); break;
    }
    if (drew) dirty = false;
    return drew;
}

void PruzeaAPIs::onTerminate(Storage& storage) {
    (void)storage;
}

bool PruzeaAPIs::writeStorageLine(std::string& line, void* arg) {
    auto* context = static_cast<StorageContext*>(arg);
    if (context == nullptr || context->self == nullptr || context->wrote) {
        return false;
    }
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "VALUE=%lu", static_cast<unsigned long>(context->self->storageWriteValue));
    line.assign(buffer);
    context->wrote = true;
    return true;
}

bool PruzeaAPIs::readStorageLine(const char* line, void* arg) {
    auto* self = static_cast<PruzeaAPIs*>(arg);
    if (self == nullptr || line == nullptr) {
        return true;
    }
    if (strncmp(line, "VALUE=", 6) == 0) {
        self->storageReadValue = static_cast<uint32_t>(strtoul(line + 6, nullptr, 10));
    }
    return true;
}

void PruzeaAPIs::resetToTitle(Audio& audio) {
    audio.stopMusic();
    mode = Mode::TITLE;
    modeStartMsec = Platform::getMsec();
    stepStartMsec = modeStartMsec;
    audio.playSE(&Audio::SE::NO_2, 0.7f);
    dirty = true;
}

void PruzeaAPIs::enterDrawTest(Audio& audio) {
    audio.stopMusic();
    mode = Mode::DRAW_TEST;
    drawStep = 0;
    modeStartMsec = Platform::getMsec();
    stepStartMsec = modeStartMsec;
    audio.playSE(&Audio::SE::NO_8, 0.7f);
}

void PruzeaAPIs::enterInputTest(Audio& audio) {
    audio.stopMusic();
    mode = Mode::INPUT_TEST;
    modeStartMsec = Platform::getMsec();
    stepStartMsec = modeStartMsec;
    audio.playSE(&Audio::SE::NO_1, 0.7f);
}

void PruzeaAPIs::enterAudioTest(Audio& audio) {
    audio.stopMusic();
    mode = Mode::AUDIO_TEST;
    audioRow = AudioRow::SE;
    modeStartMsec = Platform::getMsec();
    stepStartMsec = modeStartMsec;
    audio.playSE(&Audio::SE::NO_1, 0.7f);
}

void PruzeaAPIs::enterStorageTest(Audio& audio, Storage& storage) {
    audio.stopMusic();
    mode = Mode::STORAGE_TEST;
    modeStartMsec = Platform::getMsec();
    stepStartMsec = modeStartMsec;
    runStorageTest(storage);

    const bool allOk = storageMatch && saveDataMatch;
    audio.playSE(allOk ? &Audio::SE::NO_3 : &Audio::SE::NO_12, 0.8f);
}

void PruzeaAPIs::runStorageTest(Storage& storage) {
    storageWriteValue = static_cast<uint32_t>(PRUZEA::Math::random(0, 0x7FFFFFFF)) ^ Platform::getMsec();
    storageReadValue = 0;
    storageAvailable = storage.isAvailable();
    storageWriteOk = false;
    storageReadOk = false;
    storageMatch = false;

    saveDataSetOk = false;
    saveDataSaveOk = false;
    saveDataLoadOk = false;
    saveDataMatch = false;
    saveDataEntryCount = 0;
    saveDataUsedBytes = 0;

    if (!storageAvailable) {
        dirty = true;
        return;
    }

    // Low-level UserFile callback API test.
    StorageContext context = {this, false};
    storageWriteOk = storage.writeUserFile(
        getId(), "userfile_test.txt", PruzeaAPIs::writeStorageLine, &context);
    storageReadOk = storage.readUserFile(
        getId(), "userfile_test.txt", PruzeaAPIs::readStorageLine, this);
    storageMatch = storageWriteOk && storageReadOk &&
                   storageWriteValue == storageReadValue;

    // Game-oriented SaveData helper test. Save, clear RAM, reload, and compare types.
    SaveData saveData;
    const int32_t signedValue = -12345;
    const bool flagValue = true;
    const char* const textValue = "PRUZEA";

    saveDataSetOk =
        saveData.setUInt32("random", storageWriteValue) &&
        saveData.setInt32("signed", signedValue) &&
        saveData.setBool("flag", flagValue) &&
        saveData.setString("text", textValue);

    if (saveDataSetOk) {
        saveDataSaveOk = saveData.save(storage, getId(), "savedata_test.txt");
    }

    saveData.clear();
    saveDataLoadOk = saveData.load(storage, getId(), "savedata_test.txt");

    char loadedText[16] = {};
    const bool stringOk = saveData.getString("text", loadedText, sizeof(loadedText));
    saveDataMatch =
        saveDataSetOk && saveDataSaveOk && saveDataLoadOk && stringOk &&
        saveData.getUInt32("random", 0) == storageWriteValue &&
        saveData.getInt32("signed", 0) == signedValue &&
        saveData.getBool("flag", false) == flagValue &&
        strcmp(loadedText, textValue) == 0;

    saveDataEntryCount = saveData.getEntryCount();
    saveDataUsedBytes = saveData.getUsedBytes();
    dirty = true;
}

void PruzeaAPIs::nextDrawStep(Audio& audio) {
    drawStep++;
    if (drawStep >= DRAW_STEP_COUNT) {
        resetToTitle(audio);
        return;
    }
    stepStartMsec = Platform::getMsec();
    audio.playSE(&Audio::SE::NO_1, 0.6f);
}

void PruzeaAPIs::updateTitle(Input& input, Audio& audio, Storage& storage) {
    if (input.justPressed(Input::LEFT) || input.justPressed(Input::RIGHT) ||
        input.justPressed(Input::UP) || input.justPressed(Input::DOWN)) {
        if (input.justPressed(Input::LEFT) && (titleIndex % 2) == 1) titleIndex--;
        if (input.justPressed(Input::RIGHT) && (titleIndex % 2) == 0) titleIndex++;
        if (input.justPressed(Input::UP) && titleIndex >= 2) titleIndex -= 2;
        if (input.justPressed(Input::DOWN) && titleIndex < 2) titleIndex += 2;
        audio.playSE(&Audio::SE::NO_1, 0.5f);
        dirty = true;
    }

    if (input.justPressed(Input::A) || input.justPressed(Input::START)) {
        if (titleIndex == 0) enterDrawTest(audio);
        else if (titleIndex == 1) enterInputTest(audio);
        else if (titleIndex == 2) enterAudioTest(audio);
        else enterStorageTest(audio, storage);
        dirty = true;
    }
}

void PruzeaAPIs::updateDrawTest(Input& input, Audio& audio) {
    dirty = true;

    if (input.justPressed(Input::B) || input.justPressed(Input::START)) {
        resetToTitle(audio);
        return;
    }
    if (input.justPressed(Input::A)) {
        nextDrawStep(audio);
        return;
    }
    if (Platform::elapsed(Platform::getMsec(), stepStartMsec, DRAW_STEP_MSEC)) {
        nextDrawStep(audio);
    }
}

void PruzeaAPIs::updateInputTest(Input& input, Audio& audio) {
    uint16_t prev = inputMask;
    inputMask = 0;
    if (input.pressed(Input::B)) inputMask |= Input::B;
    if (input.pressed(Input::Y)) inputMask |= Input::Y;
    if (input.pressed(Input::SELECT)) inputMask |= Input::SELECT;
    if (input.pressed(Input::START)) inputMask |= Input::START;
    if (input.pressed(Input::UP)) inputMask |= Input::UP;
    if (input.pressed(Input::DOWN)) inputMask |= Input::DOWN;
    if (input.pressed(Input::LEFT)) inputMask |= Input::LEFT;
    if (input.pressed(Input::RIGHT)) inputMask |= Input::RIGHT;
    if (input.pressed(Input::A)) inputMask |= Input::A;
    if (input.pressed(Input::X)) inputMask |= Input::X;
    if (input.pressed(Input::L)) inputMask |= Input::L;
    if (input.pressed(Input::R)) inputMask |= Input::R;
    if (prev != inputMask)
    {
        dirty = true;
    }

    if (input.pressed(Input::LEFT) && input.justPressed(Input::A)) {
        resetToTitle(audio);
        return;
    }
    if (input.justPressed(Input::A) || input.justPressed(Input::B) || input.justPressed(Input::X) || input.justPressed(Input::Y) ||
        input.justPressed(Input::L) || input.justPressed(Input::R) || input.justPressed(Input::SELECT) || input.justPressed(Input::START) ||
        input.justPressed(Input::UP) || input.justPressed(Input::DOWN) || input.justPressed(Input::LEFT) || input.justPressed(Input::RIGHT)) {
        audio.playSE(&Audio::SE::NO_1, 0.35f);
    }
}

void PruzeaAPIs::updateAudioTest(Input& input, Audio& audio) {
    if (input.justPressed(Input::START)) {
        resetToTitle(audio);
        dirty = true;
        return;
    }
    if (input.justPressed(Input::UP) || input.justPressed(Input::DOWN)) {
        audioRow = (audioRow == AudioRow::SE) ? AudioRow::MUSIC : AudioRow::SE;
        audio.playSE(&Audio::SE::NO_1, 0.4f);
        dirty = true;
    }
    if (input.justPressed(Input::LEFT)) {
        if (audioRow == AudioRow::SE) seIndex = (seIndex + SE_COUNT - 1) % SE_COUNT;
        else musicIndex = (musicIndex + MUSIC_COUNT - 1) % MUSIC_COUNT;
        audio.playSE(&Audio::SE::NO_1, 0.4f);
        dirty = true;
    }
    if (input.justPressed(Input::RIGHT)) {
        if (audioRow == AudioRow::SE) seIndex = (seIndex + 1) % SE_COUNT;
        else musicIndex = (musicIndex + 1) % MUSIC_COUNT;
        audio.playSE(&Audio::SE::NO_1, 0.4f);
        dirty = true;
    }
    if (input.justPressed(Input::A)) {
        if (audioRow == AudioRow::SE) audio.playSE(SE_LIST[seIndex], 0.8f);
        else audio.playMusic(getSelectedMusic(true));
        dirty = true;
    }
    if (input.justPressed(Input::B) && audioRow == AudioRow::MUSIC) {
        audio.playMusic(getSelectedMusic(false));
        dirty = true;
    }
    if (input.justPressed(Input::X)) {
        audio.stopMusic();
        audio.playSE(&Audio::SE::NO_2, 0.4f);
        dirty = true;
    }
}

void PruzeaAPIs::updateStorageTest(Input& input, Audio& audio, Storage& storage) {
    if (input.justPressed(Input::A)) {
        runStorageTest(storage);
        const bool allOk = storageMatch && saveDataMatch;
        audio.playSE(allOk ? &Audio::SE::NO_3 : &Audio::SE::NO_12, 0.8f);
    }
    if (input.justPressed(Input::B) || input.justPressed(Input::START)) {
        resetToTitle(audio);
    }
}

bool PruzeaAPIs::drawTitle(Graphics& g) {
    drawBackground(g);
    g.drawString("PRUZEA APIs", 160, 28, COL_ACCENT, Graphics::SIZE_32B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("Showcases of APIs", 160, 55, COL_DIM, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    drawTextBox(g, 20, 84, 134, 48, "Graphics", titleIndex == 0);
    drawTextBox(g, 166, 84, 134, 48, "Input", titleIndex == 1);
    drawTextBox(g, 20, 146, 134, 48, "Audio", titleIndex == 2);
    drawTextBox(g, 166, 146, 134, 48, "Storage", titleIndex == 3);

    drawCenteredHint(g, "D-PAD: Select   A/START: Open", 214);
    return true;
}

bool PruzeaAPIs::drawDrawTest(Graphics& g, bool requestFullRedraw) {
    const bool staticStep =
        drawStep == 17 || // Gradient
        drawStep == 18 || // Font
        drawStep == 19 || // Alignment
        drawStep == 20 || // Text Metrics
        drawStep == 23 || // Clip Rect
        drawStep == 24;   // Sprite

    if (staticStep && !requestFullRedraw && lastDrawStep == drawStep) {
        return false;
    }
    if (staticStep) {
        lastDrawStep = drawStep;
    }

    if (drawStep == 21) {
        drawViewportTest(g);
        return true;
    }

    drawBackground(g);
    drawHeader(g, getDrawStepName());

    const uint32_t now = Platform::getMsec();
    const int16_t x = getAnimX(now);
    drawMovingShape(g, drawStep, x, 130);

    switch (drawStep) {
        case 16: drawArcTest(g); break;
        case 17: drawGradientTest(g); break;
        case 18: drawFontTest(g); break;
        case 19: drawAlignmentTest(g); break;
        case 20: drawTextMetricsTest(g); break;
        case 22: drawCameraTest(g); break;
        case 23: drawClipRectTest(g); break;
        case 24: drawSpriteTest(g); break;
        case 25: drawSpriteTransformTest(g); break;
        case 26: drawSpriteSheetTest(g); break;
        default: break;
    }

    drawCenteredHint(g, "A: Next   B/START: Title", 214);
    return true;
}

bool PruzeaAPIs::drawInputTest(Graphics& g) {
    drawBackground(g);
    drawHeader(g, "Input");
    g.drawString("Input Button Monitor", 160, 52, COL_TEXT, Graphics::SIZE_22B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    drawButtonLamp(g, 64, 104, 38, 30, "UP", (inputMask & Input::UP) != 0);
    drawButtonLamp(g, 24, 138, 38, 30, "LEFT", (inputMask & Input::LEFT) != 0);
    drawButtonLamp(g, 64, 172, 38, 30, "DOWN", (inputMask & Input::DOWN) != 0);
    drawButtonLamp(g, 104, 138, 38, 30, "RIGHT", (inputMask & Input::RIGHT) != 0);

    drawButtonLamp(g, 222, 104, 38, 30, "X", (inputMask & Input::X) != 0);
    drawButtonLamp(g, 182, 138, 38, 30, "Y", (inputMask & Input::Y) != 0);
    drawButtonLamp(g, 262, 138, 38, 30, "A", (inputMask & Input::A) != 0);
    drawButtonLamp(g, 222, 172, 38, 30, "B", (inputMask & Input::B) != 0);

    drawButtonLamp(g, 22, 74, 54, 24, "L", (inputMask & Input::L) != 0);
    drawButtonLamp(g, 244, 74, 54, 24, "R", (inputMask & Input::R) != 0);
    drawButtonLamp(g, 104, 84, 54, 24, "SEL", (inputMask & Input::SELECT) != 0);
    drawButtonLamp(g, 164, 84, 54, 24, "START", (inputMask & Input::START) != 0);

    g.drawString("LEFT + A: Title", 160, 214, COL_DIM, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    return true;
}

bool PruzeaAPIs::drawAudioTest(Graphics& g) {
    drawBackground(g);
    drawHeader(g, "Audio");

    drawTextBox(g, 34, 74, 252, 46, getSeName(), audioRow == AudioRow::SE);
    drawTextBox(g, 34, 138, 252, 46, MUSIC_NAMES[musicIndex], audioRow == AudioRow::MUSIC);

    g.drawString("SE", 45, 96, audioRow == AudioRow::SE ? COL_ACCENT : COL_DIM, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::LEFT, Graphics::VerticalAlign::MIDDLE);
    g.drawString("Music", 45, 160, audioRow == AudioRow::MUSIC ? COL_ACCENT : COL_DIM, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::LEFT, Graphics::VerticalAlign::MIDDLE);
    g.drawString("LEFT/RIGHT: Select", 160, 47, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("A: Play/Loop  B: Once  X: Stop", 160, 63, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    drawCenteredHint(g, "START: Title", 214);
    return true;
}

bool PruzeaAPIs::drawStorageTest(Graphics& g) {
    drawBackground(g);
    drawHeader(g, "Storage");

    const bool allOk = storageAvailable && storageMatch && saveDataMatch;
    const Graphics::Color resultColor = allOk ? COL_GREEN : COL_DANGER;
    g.drawString(allOk ? "ALL OK" : "FAILED", 160, 55, resultColor, Graphics::SIZE_25B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    char line[72];
    snprintf(line, sizeof(line), "Storage: %s", storageAvailable ? "AVAILABLE" : "NOT AVAILABLE");
    g.drawString(line, 16, 79, storageAvailable ? COL_TEXT : COL_DANGER, Graphics::SIZE_18);

    g.drawString("UserFile", 16, 105, COL_ACCENT, Graphics::SIZE_18);
    snprintf(line, sizeof(line), "WRITE %s   READ %s   MATCH %s",
             storageWriteOk ? "OK" : "NG",
             storageReadOk ? "OK" : "NG",
             storageMatch ? "OK" : "NG");
    g.drawString(line, 16, 126, storageMatch ? COL_GREEN : COL_DANGER, Graphics::SIZE_13);
    snprintf(line, sizeof(line), "%lu -> %lu",
             static_cast<unsigned long>(storageWriteValue),
             static_cast<unsigned long>(storageReadValue));
    g.drawString(line, 315, 108, storageMatch ? COL_DIM : COL_WARN, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);

    g.drawString("SaveData", 16, 151, COL_ACCENT, Graphics::SIZE_18);
    snprintf(line, sizeof(line), "SET %s   SAVE %s   LOAD %s   TYPES %s",
             saveDataSetOk ? "OK" : "NG",
             saveDataSaveOk ? "OK" : "NG",
             saveDataLoadOk ? "OK" : "NG",
             saveDataMatch ? "OK" : "NG");
    g.drawString(line, 16, 172, saveDataMatch ? COL_GREEN : COL_DANGER, Graphics::SIZE_13);
    snprintf(line, sizeof(line), "ENTRIES %u   USED %u / 512 BYTES",
             static_cast<unsigned>(saveDataEntryCount),
             static_cast<unsigned>(saveDataUsedBytes));
    g.drawString(line, 315, 190, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);

    drawCenteredHint(g, "A: Retest   B/START: Title", 214);
    return true;
}

void PruzeaAPIs::drawBackground(Graphics& g) {
    g.fillScreen(COL_BG);
    drawGrid(g);
    g.drawRect(0, 0, SCREEN_W, UI_SAFE_BOTTOM, 1, COL_LINE);
}

void PruzeaAPIs::drawHeader(Graphics& g, const char* label) {
    g.fillRect(0, 0, SCREEN_W, 34, COL_PANEL);
    g.drawLine(0, 34, SCREEN_W - 1, 34, COL_LINE);
    g.drawString(label, 10, 13, COL_ACCENT, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::LEFT, Graphics::VerticalAlign::TOP);
}

void PruzeaAPIs::drawCenteredHint(Graphics& g, const char* text, int16_t y) {
    g.drawString(text, 160, y, COL_DIM, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void PruzeaAPIs::drawMovingShape(Graphics& g, uint8_t step, int16_t x, int16_t y) {
    switch (step) {
        case 0:
            g.fillScreen(Graphics::rgb565(8, 20, 34));
            drawHeader(g, getDrawStepName());
            g.fillCircle(x, y, 32, COL_ACCENT);
            break;
        case 1:
            for (int i = 0; i < 60; ++i) {
                int16_t px = x + static_cast<int16_t>((i * 17) % 80) - 40;
                int16_t py = y + static_cast<int16_t>((i * 31) % 60) - 30;
                g.drawPixel(px, py, (i % 3 == 0) ? COL_ACCENT : COL_GREEN);
            }
            break;
        case 2:
            g.drawLine(x - 42, y - 28, x + 42, y + 28, COL_ACCENT);
            g.drawLine(x - 42, y + 28, x + 42, y - 28, COL_PURPLE);
            break;
        case 3:
            g.drawTriangle(x, y - 38, x - 44, y + 34, x + 44, y + 34, COL_ACCENT);
            break;
        case 4:
            g.fillTriangle(x, y - 38, x - 44, y + 34, x + 44, y + 34, COL_PURPLE);
            break;
        case 5:
            g.drawRect(x - 44, y - 30, 88, 60, COL_ACCENT);
            break;
        case 6:
            g.drawRoundRect(x - 44, y - 30, 88, 60, 12, COL_ACCENT);
            break;
        case 7:
            g.drawRect(x - 44, y - 30, 88, 60, 5, COL_GREEN);
            break;
        case 8:
            g.drawRoundRect(x - 44, y - 30, 88, 60, 12, 5, COL_GREEN);
            break;
        case 9:
            g.fillRect(x - 44, y - 30, 88, 60, COL_ACCENT);
            break;
        case 10:
            g.fillRectAlpha(x - 44, y - 30, 88, 60, 100, COL_ACCENT);
            break;
         case 11:
            g.fillRoundRect(x - 44, y - 30, 88, 60, 12, COL_ACCENT);
            break;
        case 12:
            g.drawCircle(x, y, 38, COL_ACCENT);
            break;
        case 13:
            g.drawCircle(x, y, 58, 28, COL_ACCENT);
            break;
        case 14:
            g.fillCircle(x, y, 38, COL_PURPLE);
            break;
        case 15:
            g.fillCircle(x, y, 58, 28, COL_PURPLE);
            break;
        default:
            break;
    }
}

void PruzeaAPIs::drawArcTest(Graphics& g) {
    const uint32_t now = Platform::getMsec();
    const float phase = static_cast<float>(now - stepStartMsec) * 0.0025f;
    const float sweep = PRUZEA::Math::PI * (0.55f + 0.30f * (PRUZEA::Math::sin(phase) + 1.0f));

    g.drawArc(62, 112, 30, 0.0f, sweep, COL_ACCENT);
    g.drawArc(130, 112, static_cast<uint16_t>(34), static_cast<uint8_t>(7),
              PRUZEA::Math::HALF_PI, PRUZEA::Math::HALF_PI + sweep, COL_GREEN);
    g.drawArc(204, 112, static_cast<uint16_t>(38), static_cast<uint16_t>(24),
              -PRUZEA::Math::HALF_PI, -PRUZEA::Math::HALF_PI + sweep, COL_PURPLE);
    g.fillArc(278, 112, 30, PRUZEA::Math::PI, PRUZEA::Math::PI + sweep, COL_WARN);

    g.drawString("draw", 62, 162, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("thick", 130, 162, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("ellipse", 204, 162, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("fill", 278, 162, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void PruzeaAPIs::drawGradientTest(Graphics& g) {
    g.fillRectGradient(22, 62, 84, 112, COL_ACCENT, COL_PURPLE, Graphics::HORIZONRAL_LINEAR);
    g.fillRectGradient(118, 62, 84, 112, COL_GREEN, COL_DANGER, Graphics::VERTICAL_LINEAR);
    g.fillRectGradient(214, 62, 84, 112, COL_WARN, COL_BG, Graphics::RADIAL_CENTER);

    g.drawRect(22, 62, 84, 112, COL_LINE);
    g.drawRect(118, 62, 84, 112, COL_LINE);
    g.drawRect(214, 62, 84, 112, COL_LINE);

    g.drawString("H", 64, 188, COL_TEXT, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("V", 160, 188, COL_TEXT, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("RADIAL", 256, 188, COL_TEXT, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void PruzeaAPIs::drawFontTest(Graphics& g) {
    const int16_t x = 22;
    int16_t y = 54;
    g.drawString("SIZE_10", x, y, COL_TEXT, Graphics::SIZE_10); y += 13;
    g.drawString("SIZE_13", x, y, COL_TEXT, Graphics::SIZE_13); y += 16;
    g.drawString("SIZE_18", x, y, COL_TEXT, Graphics::SIZE_18); y += 22;
    g.drawString("SIZE_22B", x, y, COL_TEXT, Graphics::SIZE_22B); y += 25;
    g.drawString("SIZE_25", x, y, COL_TEXT, Graphics::SIZE_25); y += 28;
    g.drawString("SIZE_32B", x, y, COL_ACCENT, Graphics::SIZE_32B);
    g.drawString("Japanese: テスト", 184, 72, COL_GREEN, Graphics::SIZE_16J);
    g.drawString("日本語", 184, 104, COL_WARN, Graphics::SIZE_20J);
    g.drawString("表示", 184, 146, COL_PURPLE, Graphics::SIZE_32J);
}

void PruzeaAPIs::drawAlignmentTest(Graphics& g) {
    const int16_t xs[3] = {58, 160, 262};
    const int16_t ys[3] = {70, 118, 166};
    const Graphics::HorizontalAlign has[3] = {Graphics::HorizontalAlign::LEFT, Graphics::HorizontalAlign::CENTER, Graphics::HorizontalAlign::RIGHT};
    const Graphics::VerticalAlign vas[3] = {Graphics::VerticalAlign::TOP, Graphics::VerticalAlign::MIDDLE, Graphics::VerticalAlign::BOTTOM};
    const char* labels[3][3] = {
        {"LT", "CT", "RT"},
        {"LM", "CM", "RM"},
        {"LB", "CB", "RB"}
    };
    for (int yi = 0; yi < 3; ++yi) {
        for (int xi = 0; xi < 3; ++xi) {
            g.drawLine(xs[xi] - 7, ys[yi], xs[xi] + 7, ys[yi], COL_DIM);
            g.drawLine(xs[xi], ys[yi] - 7, xs[xi], ys[yi] + 7, COL_DIM);
            g.drawString(labels[yi][xi], xs[xi], ys[yi], COL_ACCENT, Graphics::SIZE_13, has[xi], vas[yi]);
        }
    }

    // Alignment is shared by text and shape overloads.
    g.drawRect(160, 190, 62, 18, COL_GREEN,
               Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("shape", 160, 190, COL_TEXT, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void PruzeaAPIs::drawTextMetricsTest(Graphics& g) {
    const char* text = "PRUZEA";
    const Graphics::Font font = Graphics::SIZE_25B;
    const uint16_t w = g.getTextWidth(text, font);
    const uint16_t h = g.getTextHeight(text, font);
    const int16_t cx = 160;
    const int16_t cy = 112;

    g.drawRect(cx, cy, w, h, COL_GREEN,
               Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString(text, cx, cy, COL_TEXT, font,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    char sizeText[40];
    snprintf(sizeText, sizeof(sizeText), "width=%u  height=%u",
             static_cast<unsigned>(w), static_cast<unsigned>(h));
    g.drawString(sizeText, 160, 166, COL_ACCENT, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("getTextWidth / getTextHeight", 160, 188, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}


void PruzeaAPIs::drawViewportTest(Graphics& g) {
    // Viewport is applied when the graphics buffer is presented, so keep the
    // offset active after this function returns. onDraw() resets it next frame.
    const int16_t shakeX = static_cast<int16_t>(PRUZEA::Math::random(-4, 5));
    const int16_t shakeY = static_cast<int16_t>(PRUZEA::Math::random(-3, 4));
    g.setViewport(shakeX, shakeY);

    g.fillScreen(COL_BG);
    for (int16_t x = -24; x <= 344; x += 24) {
        g.drawLine(x, -24, x, 264, COL_LINE);
    }
    for (int16_t y = -24; y <= 264; y += 24) {
        g.drawLine(-24, y, 344, y, COL_LINE);
    }
    g.fillCircle(160, 120, 36, COL_ACCENT);
    g.drawRoundRect(98, 58, 124, 124, 10, 3, COL_PURPLE);
    drawHeader(g, getDrawStepName());
    g.drawString("SCREEN SHAKE", 160, 120, COL_BG, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    drawCenteredHint(g, "Viewport moves the final screen", 214);
}

void PruzeaAPIs::drawCameraTest(Graphics& g) {
    const uint32_t now = Platform::getMsec();
    const float phase = static_cast<float>(now - stepStartMsec) * 0.0015f;

    Graphics::Camera camera;
    camera.x = static_cast<int16_t>(60.0f + PRUZEA::Math::sin(phase) * 54.0f);
    camera.y = static_cast<int16_t>(30.0f + PRUZEA::Math::cos(phase * 0.7f) * 24.0f);
    camera.zoom = 1.0f + (PRUZEA::Math::sin(phase * 0.8f) + 1.0f) * 0.25f;
    camera.zoomCenterX = 160;
    camera.zoomCenterY = 120;
    g.setCamera(camera);

    // World-space content.
    for (int16_t x = -80; x <= 480; x += 40) {
        g.drawLine(x, -80, x, 360, COL_LINE);
    }
    for (int16_t y = -80; y <= 360; y += 40) {
        g.drawLine(-80, y, 480, y, COL_LINE);
    }
    g.fillCircle(90, 80, 14, COL_GREEN);
    g.fillCircle(220, 130, 20, COL_PURPLE);
    g.fillCircle(340, 190, 16, COL_WARN);
    g.drawRect(60, 48, 300, 176, 2, COL_ACCENT);

    // HUD is screen-space: reset camera before drawing it.
    g.resetCamera();
    g.fillRectAlpha(8, 42, 142, 38, 180, Graphics::BLACK);
    g.drawString("WORLD MOVES", 16, 49, COL_TEXT, Graphics::SIZE_13);
    char zoomText[24];
    snprintf(zoomText, sizeof(zoomText), "zoom %.2f", static_cast<double>(camera.zoom));
    g.drawString(zoomText, 16, 64, COL_ACCENT, Graphics::SIZE_13);
    g.drawString("HUD FIXED", 304, 49, COL_GREEN, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);
}

void PruzeaAPIs::drawClipRectTest(Graphics& g) {
    constexpr int16_t clipX = 70;
    constexpr int16_t clipY = 62;
    constexpr uint16_t clipW = 180;
    constexpr uint16_t clipH = 112;

    g.setClipRect(clipX, clipY, clipW, clipH);
    for (int16_t x = 20; x < 310; x += 24) {
        g.drawLine(x, 40, 320 - x, 198, COL_PURPLE);
    }
    g.fillCircle(160, 118, 82, COL_ACCENT);
    g.drawCircle(92, 92, 56, COL_WARN);
    g.drawCircle(232, 150, 62, COL_GREEN);
    g.resetClipRect();

    g.drawRect(clipX, clipY, clipW, clipH, 2, Graphics::WHITE);
    g.drawString("CLIP AREA", 160, 184, COL_TEXT, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}


void PruzeaAPIs::drawSpriteTest(Graphics& g) {
    const int16_t baseY = 74;
    for (uint8_t scale = 1; scale <= 4; ++scale) {
        int16_t x = 34 + static_cast<int16_t>((scale - 1) * 68);
        g.drawSprite(DUMMY_SPRITE, x, baseY, 16, 16, {
            .scale = scale,
            .transparent = true,
            .transparentColor = Graphics::BLACK
        });
        char label[12];
        snprintf(label, sizeof(label), "x%u", static_cast<unsigned>(scale));
        g.drawString(label, x + 16, 178, COL_TEXT, Graphics::SIZE_18,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }
}

void PruzeaAPIs::drawSpriteTransformTest(Graphics& g) {
    const uint32_t now = Platform::getMsec();
    const float angle = static_cast<float>(now - stepStartMsec) * 0.0025f;

    struct Item {
        int16_t x;
        const char* label;
        Graphics::SpriteOptions options;
    };

    Item items[4] = {
        {48,  "ROTATE", {.scale = 3, .angle = angle, .transparent = true, .transparentColor = Graphics::BLACK}},
        {118, "FLIP X", {.scale = 3, .flipX = true, .transparent = true, .transparentColor = Graphics::BLACK}},
        {188, "FLIP Y", {.scale = 3, .flipY = true, .transparent = true, .transparentColor = Graphics::BLACK}},
        {258, "BOTH",   {.scale = 3, .angle = -angle * 0.6f, .flipX = true, .flipY = true, .transparent = true, .transparentColor = Graphics::BLACK}}
    };

    for (const Item& item : items) {
        g.drawSprite(DUMMY_SPRITE, item.x - 24, 82, 16, 16, item.options);
        g.drawString(item.label, item.x, 164, COL_TEXT, Graphics::SIZE_10,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }

    g.drawString("angle / flipX / flipY", 160, 190, COL_DIM, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void PruzeaAPIs::drawSpriteSheetTest(Graphics& g)
{
    const uint32_t index = ((Platform::getMsec() - stepStartMsec) / 1000) % (DUMMY_SPRITE_SHEET.columns * DUMMY_SPRITE_SHEET.rows);

    const uint16_t column = index % DUMMY_SPRITE_SHEET.columns;
    const uint16_t row    = index / DUMMY_SPRITE_SHEET.columns;

    g.drawSprite(DUMMY_SPRITE_SHEET, column, row, 166, 88,
        {
            .scale = 4,
            .transparent = true,
            .transparentColor = Graphics::MAGENTA
        }
    );
    g.drawRect(166, 88, 64, 64, 1, COL_ACCENT);

    char text[32];
    snprintf(text, sizeof(text), "Column:%u  Row:%u",column, row);

    g.drawString(text, 160, 200, COL_TEXT, Graphics::SIZE_13, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    const int startX = 90;
    const int startY = 88;

    g.drawSprite(DUMMY_SPRITE_SHEET_DATA, startX, startY, 64, 64,
        {
            .scale = 1,
            .transparent = true,
            .transparentColor = Graphics::MAGENTA
        }
    );
    g.drawRect(startX + column * 16, startY + row * 16, 16, 16, 1, COL_WARN);
}

void PruzeaAPIs::drawButtonLamp(Graphics& g, int16_t x, int16_t y, int16_t w, int16_t h, const char* label, bool on) {
    g.fillRoundRect(x, y, w, h, 8, on ? COL_ACCENT : COL_PANEL);
    g.drawRoundRect(x, y, w, h, 8, 2, on ? COL_TEXT : COL_LINE);
    g.drawString(label, x + w / 2, y + h / 2, on ? COL_BG : COL_TEXT, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

int16_t PruzeaAPIs::getAnimX(uint32_t now) const {
    const uint32_t elapsed = now - stepStartMsec;
    const uint32_t travelMsec = (DRAW_STEP_MSEC - DRAW_HOLD_MSEC) / 2;
    if (elapsed < travelMsec) {
        return static_cast<int16_t>(360 - (210L * static_cast<int32_t>(elapsed)) / static_cast<int32_t>(travelMsec));
    }
    if (elapsed < travelMsec + DRAW_HOLD_MSEC) {
        return 150;
    }
    uint32_t outElapsed = elapsed - travelMsec - DRAW_HOLD_MSEC;
    if (outElapsed > travelMsec) outElapsed = travelMsec;
    return static_cast<int16_t>(150 - (230L * static_cast<int32_t>(outElapsed)) / static_cast<int32_t>(travelMsec));
}

const char* PruzeaAPIs::getDrawStepName() const {
    if (drawStep < DRAW_STEP_COUNT) return DRAW_STEP_NAMES[drawStep];
    return "Draw Test";
}

const char* PruzeaAPIs::getSeName() const {
    if (seIndex < SE_COUNT) return SE_NAMES[seIndex];
    return "NO_1";
}

const Audio::Music* PruzeaAPIs::getSelectedMusic(bool loop) const {
    uint8_t index = musicIndex;
    if (index >= MUSIC_COUNT) index = 0;
    return loop ? &MUSIC_LOOP[index] : &MUSIC_ONCE[index];
}

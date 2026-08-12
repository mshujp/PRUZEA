#include "PruzeaAPIsSSD1306.h"

#include <cstdio>

using PRUZEA::Audio;
using PRUZEA::Graphics;
using PRUZEA::Input;
namespace Platform = PRUZEA::Platform;
namespace Math = PRUZEA::Math;
using PRUZEA::Storage;

namespace {
constexpr uint16_t TARGET_W = PRUZEA::Display::SSD1306_SCREEN_W;
constexpr uint16_t TARGET_H = PRUZEA::Display::SSD1306_SCREEN_H;
constexpr uint16_t LOGICAL_W = TARGET_W;
constexpr uint16_t LOGICAL_H = TARGET_H;

constexpr uint32_t STEP_MSEC = 3500;
constexpr uint32_t HOLD_MSEC = 1400;
constexpr uint8_t STEP_COUNT = 25;

constexpr Graphics::Color OFF = Graphics::SSD1306_OFF;
constexpr Graphics::Color ON = Graphics::SSD1306_ON;

const char* const STEP_NAMES[STEP_COUNT] = {
    "clearScreen",
    "fillScreen",
    "drawPixel",
    "drawLine",
    "drawTriangle",
    "fillTriangle",
    "drawRect",
    "drawRoundRect",
    "drawRect thick",
    "drawRound thick",
    "fillRect",
    "fillRoundRect",
    "drawCircle",
    "drawCircle XY",
    "fillCircle",
    "fillCircle XY",
    "Arc",
    "Font / measure",
    "Alignment",
    "Viewport / Shake",
    "Camera / World",
    "Clip Rect",
    "Sprite",
    "Sprite Transform",
    "SpriteSheet"
};

// 8x8 monochrome-friendly speaker-like sprite.
// OFF is transparent when drawn.
static const uint16_t TEST_SPRITE[8 * 8] = {
    OFF,OFF,ON, ON, OFF,OFF,OFF,OFF,
    OFF,ON, ON, ON, OFF,ON, OFF,OFF,
    ON, ON, ON, ON, OFF,OFF,ON, OFF,
    ON, ON, ON, ON, OFF,ON, ON, OFF,
    ON, ON, ON, ON, OFF,ON, ON, OFF,
    ON, ON, ON, ON, OFF,OFF,ON, OFF,
    OFF,ON, ON, ON, OFF,ON, OFF,OFF,
    OFF,OFF,ON, ON, OFF,OFF,OFF,OFF
};

// 2x2 sheet of 8x8 monochrome icons. The source is laid out as one 16x16 bitmap.
static const uint16_t TEST_SPRITE_SHEET[16 * 16] = {
    // Row 0
    OFF,OFF,ON,ON,OFF,OFF,OFF,OFF,  OFF,OFF,OFF,ON,ON,OFF,OFF,OFF,
    OFF,ON,ON,ON,OFF,ON,OFF,OFF,    OFF,OFF,ON,ON,ON,ON,OFF,OFF,
    ON,ON,ON,ON,OFF,OFF,ON,OFF,     OFF,ON,ON,OFF,OFF,ON,ON,OFF,
    ON,ON,ON,ON,OFF,ON,ON,OFF,      ON,ON,OFF,ON,ON,OFF,ON,ON,
    ON,ON,ON,ON,OFF,ON,ON,OFF,      ON,ON,OFF,ON,ON,OFF,ON,ON,
    ON,ON,ON,ON,OFF,OFF,ON,OFF,     OFF,ON,ON,OFF,OFF,ON,ON,OFF,
    OFF,ON,ON,ON,OFF,ON,OFF,OFF,    OFF,OFF,ON,ON,ON,ON,OFF,OFF,
    OFF,OFF,ON,ON,OFF,OFF,OFF,OFF,  OFF,OFF,OFF,ON,ON,OFF,OFF,OFF,
    // Row 1
    ON,OFF,OFF,OFF,OFF,OFF,OFF,ON,  OFF,OFF,ON,ON,ON,ON,OFF,OFF,
    OFF,ON,OFF,OFF,OFF,OFF,ON,OFF,  OFF,ON,ON,OFF,OFF,ON,ON,OFF,
    OFF,OFF,ON,OFF,OFF,ON,OFF,OFF,  ON,ON,OFF,OFF,OFF,OFF,ON,ON,
    OFF,OFF,OFF,ON,ON,OFF,OFF,OFF,  ON,OFF,OFF,ON,ON,OFF,OFF,ON,
    OFF,OFF,OFF,ON,ON,OFF,OFF,OFF,  ON,OFF,OFF,ON,ON,OFF,OFF,ON,
    OFF,OFF,ON,OFF,OFF,ON,OFF,OFF,  ON,ON,OFF,OFF,OFF,OFF,ON,ON,
    OFF,ON,OFF,OFF,OFF,OFF,ON,OFF,  OFF,ON,ON,OFF,OFF,ON,ON,OFF,
    ON,OFF,OFF,OFF,OFF,OFF,OFF,ON,  OFF,OFF,ON,ON,ON,ON,OFF,OFF
};

static const Graphics::SpriteSheet TEST_SHEET = {
    TEST_SPRITE_SHEET,
    8,
    8,
    2,
    2
};
}

const char* PruzeaAPIsSSD1306::getId() const { return "pruzeaapis1306sample"; }
const char* PruzeaAPIsSSD1306::getName() const { return "PRUZEA APIs SSD1306"; }
const char* PruzeaAPIsSSD1306::getMenuName() const { return "01 PRUZEA APIs for SSD1306"; }

uint16_t PruzeaAPIsSSD1306::getLogicalScreenWidth() const { return LOGICAL_W; }
uint16_t PruzeaAPIsSSD1306::getLogicalScreenHeight() const { return LOGICAL_H; }
uint16_t PruzeaAPIsSSD1306::getTargetScreenWidth() const { return TARGET_W; }
uint16_t PruzeaAPIsSSD1306::getTargetScreenHeight() const { return TARGET_H; }

void PruzeaAPIsSSD1306::onInit(Storage& storage) {
    (void)storage;
    mode = Mode::TITLE;
    drawStep = 0;
    inputMask = 0;
    stepStartMsec = Platform::getMsec();
    speakerIconStartMsec = 0;
    speakerIconDurationMsec = 0;
    dirty = true;
}

PRUZEA::Game::GameState PruzeaAPIsSSD1306::onUpdate(
    Input& input, Audio& audio, Storage& storage, float deltaSec) {
    (void)storage;
    (void)deltaSec;

    updateInputMask(input);

    switch (mode) {
        case Mode::TITLE:
            updateTitle(input, audio);
            break;
        case Mode::GRAPHICS:
            updateGraphics(input, audio);
            break;
    }

    // The speaker icon must disappear even when no button is pressed.
    if (speakerIconDurationMsec != 0) {
        const uint32_t now = Platform::getMsec();
        if (Platform::elapsed(now, speakerIconStartMsec, speakerIconDurationMsec)) {
            speakerIconDurationMsec = 0;
            dirty = true;
        }
    }

    return GameState::RUNNING;
}

bool PruzeaAPIsSSD1306::onDraw(Graphics& graphics, bool requestFullRedraw) {
    if (!requestFullRedraw && !dirty) {
        return false;
    }

    // Every API page starts from a known graphics state.
    graphics.resetViewport();
    graphics.resetCamera();
    graphics.resetClipRect();

    switch (mode) {
        case Mode::TITLE:
            drawTitle(graphics);
            break;
        case Mode::GRAPHICS:
            drawGraphicsTest(graphics);
            break;
    }

    // Do not reset Viewport here. The Viewport / Shake page must keep its
    // offset until the graphics buffer is transferred to the display.
    dirty = false;
    return true;
}

void PruzeaAPIsSSD1306::onTerminate(Storage& storage) {
    (void)storage;
}

void PruzeaAPIsSSD1306::enterGraphics(Audio& audio) {
    mode = Mode::GRAPHICS;
    drawStep = 0;
    stepStartMsec = Platform::getMsec();
    playTestSE(audio, &Audio::SE::NO_8, 0.55f, 300);
    dirty = true;
}

void PruzeaAPIsSSD1306::changeStep(int8_t amount, Audio& audio) {
    int16_t next = static_cast<int16_t>(drawStep) + amount;
    while (next < 0) next += STEP_COUNT;
    while (next >= STEP_COUNT) next -= STEP_COUNT;

    drawStep = static_cast<uint8_t>(next);
    stepStartMsec = Platform::getMsec();

    const Audio::Sound* sound = amount >= 0 ? &Audio::SE::NO_1 : &Audio::SE::NO_2;
    playTestSE(audio, sound, 0.40f, 180);
    dirty = true;
}

void PruzeaAPIsSSD1306::playTestSE(
    Audio& audio, const Audio::Sound* sound, float gain, uint16_t iconMsec) {
    audio.playSE(sound, gain);
    speakerIconStartMsec = Platform::getMsec();
    speakerIconDurationMsec = iconMsec;
    dirty = true;
}

void PruzeaAPIsSSD1306::updateInputMask(Input& input) {
    const uint16_t previous = inputMask;
    inputMask = 0;

    if (input.pressed(Input::UP)) inputMask |= Input::UP;
    if (input.pressed(Input::DOWN)) inputMask |= Input::DOWN;
    if (input.pressed(Input::LEFT)) inputMask |= Input::LEFT;
    if (input.pressed(Input::RIGHT)) inputMask |= Input::RIGHT;
    if (input.pressed(Input::A)) inputMask |= Input::A;
    if (input.pressed(Input::B)) inputMask |= Input::B;

    if (inputMask != previous) {
        dirty = true;
    }
}

void PruzeaAPIsSSD1306::updateTitle(Input& input, Audio& audio) {
    if (input.justPressed(Input::A)) {
        enterGraphics(audio);
        return;
    }

    if (input.justPressed(Input::UP) ||
        input.justPressed(Input::DOWN) ||
        input.justPressed(Input::LEFT) ||
        input.justPressed(Input::RIGHT) ||
        input.justPressed(Input::B)) {
        playTestSE(audio, &Audio::SE::NO_1, 0.30f, 150);
    }
}

void PruzeaAPIsSSD1306::updateGraphics(Input& input, Audio& audio) {
    // API pages contain moving examples, so redraw continuously while visible.
    dirty = true;

    if (input.justPressed(Input::A)) {
        changeStep(1, audio);
        return;
    }
    if (input.justPressed(Input::B)) {
        changeStep(-1, audio);
        return;
    }

    if (input.justPressed(Input::UP) ||
        input.justPressed(Input::DOWN) ||
        input.justPressed(Input::LEFT) ||
        input.justPressed(Input::RIGHT)) {
        playTestSE(audio, &Audio::SE::NO_1, 0.25f, 130);
    }

    const uint32_t now = Platform::getMsec();
    if (Platform::elapsed(now, stepStartMsec, STEP_MSEC)) {
        changeStep(1, audio);
    }
}

void PruzeaAPIsSSD1306::drawTitle(Graphics& g) {
    g.clearScreen();
    g.drawRoundRect(1, 1, 126, 62, 5, ON);
    g.drawString("PRUZEA", 64, 9, ON, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::TOP);
    g.drawString("API TEST", 64, 29, ON, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::TOP);
    g.drawString("SSD1306", 64, 43, ON, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::TOP);
    g.drawString("A:START", 64, 61, ON, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::BOTTOM);

    drawInputOverlay(g);
}

void PruzeaAPIsSSD1306::drawGraphicsTest(Graphics& g) {
    g.clearScreen();

    const uint32_t now = Platform::getMsec();
    const int16_t x = getAnimX(now);

    switch (drawStep) {
        case 16: drawArcTest(g); break;
        case 17: drawFontTest(g); break;
        case 18: drawAlignmentTest(g); break;
        case 19: drawViewportTest(g); break;
        case 20: drawCameraTest(g); break;
        case 21: drawClipRectTest(g); break;
        case 22: drawSpriteTest(g); break;
        case 23: drawSpriteTransformTest(g); break;
        case 24: drawSpriteSheetTest(g); break;
        default: drawMovingShape(g, drawStep, x, 36); break;
    }

    // Camera and Clip pages restore their own state before returning.
    // Viewport intentionally remains active so this header shakes with the screen.
    g.fillRect(0, 0, TARGET_W, 11, OFF);
    g.drawLine(0, 12, TARGET_W - 1, 12, ON);

    char header[32];
    snprintf(header, sizeof(header), "%02u/%02u %s",
             static_cast<unsigned>(drawStep + 1),
             static_cast<unsigned>(STEP_COUNT),
             getStepName());
    g.drawString(header, 2, 1, ON, Graphics::SIZE_10);

    drawInputOverlay(g);
}

void PruzeaAPIsSSD1306::drawMovingShape(
    Graphics& g, uint8_t step, int16_t x, int16_t y) {
    switch (step) {
        case 0:
            g.fillScreen(ON);
            g.clearScreen();
            g.drawCircle(x, y, 12, ON);
            break;
        case 1:
            g.fillScreen(ON);
            g.fillRect(x - 10, y - 10, 20, 20, OFF);
            break;
        case 2:
            for (int16_t i = 0; i < 30; ++i) {
                g.drawPixel(x + ((i * 11) % 35) - 17,
                            y + ((i * 7) % 25) - 12,
                            ON);
            }
            break;
        case 3:
            g.drawLine(x - 20, y - 13, x + 20, y + 13, ON);
            g.drawLine(x - 20, y + 13, x + 20, y - 13, ON);
            break;
        case 4:
            g.drawTriangle(x, y - 16, x - 19, y + 14, x + 19, y + 14, ON);
            break;
        case 5:
            g.fillTriangle(x, y - 16, x - 19, y + 14, x + 19, y + 14, ON);
            break;
        case 6:
            g.drawRect(x - 22, y - 14, 44, 28, ON);
            break;
        case 7:
            g.drawRoundRect(x - 22, y - 14, 44, 28, 6, ON);
            break;
        case 8:
            g.drawRect(x - 22, y - 14, 44, 28, 3, ON);
            break;
        case 9:
            g.drawRoundRect(x - 22, y - 14, 44, 28, 6, 3, ON);
            break;
        case 10:
            g.fillRect(x - 22, y - 14, 44, 28, ON);
            break;
        case 11:
            g.fillRoundRect(x - 22, y - 14, 44, 28, 6, ON);
            break;
        case 12:
            g.drawCircle(x, y, 15, ON);
            break;
        case 13:
            g.drawCircle(x, y, 22, 11, ON);
            break;
        case 14:
            g.fillCircle(x, y, 15, ON);
            break;
        case 15:
            g.fillCircle(x, y, 22, 11, ON);
            break;
        default:
            break;
    }
}

void PruzeaAPIsSSD1306::drawArcTest(Graphics& g) {
    const uint32_t elapsed = Platform::getMsec() - stepStartMsec;
    const float phase = static_cast<float>(elapsed) * 0.0022f;

    g.drawArc(24, 37, static_cast<uint16_t>(15), 0.0f, Math::PI + phase, ON);
    g.drawArc(61, 37, static_cast<uint16_t>(17), static_cast<uint8_t>(4),
              -Math::PI * 0.5f, Math::PI * 0.75f, ON);
    g.fillArc(101, 37, static_cast<uint16_t>(18), static_cast<uint16_t>(11),
              0.0f, Math::PI + phase * 0.5f, ON);
}

void PruzeaAPIsSSD1306::drawFontTest(Graphics& g) {
    g.drawString("SIZE_10", 3, 14, ON, Graphics::SIZE_10);
    g.drawString("SIZE_13", 3, 25, ON, Graphics::SIZE_13);
    g.drawString("SIZE_18", 3, 39, ON, Graphics::SIZE_18);

    const char* measured = "WIDTH";
    const uint16_t width = g.getTextWidth(measured, Graphics::SIZE_10);
    const uint16_t height = g.getTextHeight(measured, Graphics::SIZE_10);

    g.drawRect(66, 18, width, height, ON);
    g.drawString(measured, 66, 18, ON, Graphics::SIZE_10);

    char text[20];
    snprintf(text, sizeof(text), "W%u H%u",
             static_cast<unsigned>(width),
             static_cast<unsigned>(height));
    g.drawString(text, 126, 63, ON, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT,
                 Graphics::VerticalAlign::BOTTOM);
}

void PruzeaAPIsSSD1306::drawAlignmentTest(Graphics& g) {
    const int16_t xs[3] = {20, 64, 108};
    const int16_t ys[3] = {19, 36, 54};
    const Graphics::HorizontalAlign ha[3] = {
        Graphics::HorizontalAlign::LEFT,
        Graphics::HorizontalAlign::CENTER,
        Graphics::HorizontalAlign::RIGHT
    };
    const Graphics::VerticalAlign va[3] = {
        Graphics::VerticalAlign::TOP,
        Graphics::VerticalAlign::MIDDLE,
        Graphics::VerticalAlign::BOTTOM
    };
    const char* const labels[3][3] = {
        {"LT", "CT", "RT"},
        {"LM", "CM", "RM"},
        {"LB", "CB", "RB"}
    };

    for (uint8_t yi = 0; yi < 3; ++yi) {
        for (uint8_t xi = 0; xi < 3; ++xi) {
            g.drawPixel(xs[xi], ys[yi], ON);
            g.drawString(labels[yi][xi], xs[xi], ys[yi], ON,
                         Graphics::SIZE_10, ha[xi], va[yi]);
        }
    }
}

void PruzeaAPIsSSD1306::drawViewportTest(Graphics& g) {
    // Viewport is a final presentation offset. Use it here as a screen shake.
    const int16_t shakeX = static_cast<int16_t>(Math::random(-2, 3));
    const int16_t shakeY = static_cast<int16_t>(Math::random(-2, 3));
    g.setViewport(shakeX, shakeY);

    for (int16_t x = 0; x < static_cast<int16_t>(TARGET_W); x += 16) {
        g.drawLine(x, 13, x, TARGET_H - 1, ON);
    }
    for (int16_t y = 16; y < static_cast<int16_t>(TARGET_H); y += 12) {
        g.drawLine(0, y, TARGET_W - 1, y, ON);
    }

    g.drawRoundRect(37, 18, 54, 34, 5, 2, ON);
    g.drawString("SHAKE", 64, 35, ON, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
}

void PruzeaAPIsSSD1306::drawCameraTest(Graphics& g) {
    const uint32_t elapsed = Platform::getMsec() - stepStartMsec;
    const float phase = static_cast<float>(elapsed) * 0.002f;

    Graphics::Camera camera;
    camera.x = static_cast<int16_t>(Math::sin(phase) * 20.0f);
    camera.y = static_cast<int16_t>(Math::cos(phase * 0.7f) * 6.0f);
    camera.zoom = 1.0f + (Math::sin(phase * 0.5f) + 1.0f) * 0.20f;
    camera.zoomCenterX = 64;
    camera.zoomCenterY = 38;
    g.setCamera(camera);

    // World content moves and zooms.
    for (int16_t x = -32; x <= 160; x += 16) {
        g.drawLine(x, 13, x, 80, ON);
    }
    for (int16_t y = 16; y <= 80; y += 12) {
        g.drawLine(-32, y, 160, y, ON);
    }
    g.drawCircle(64, 38, 8, ON);
    g.drawRect(44, 25, 40, 26, ON);

    // Screen-space HUD is intentionally not affected by the camera.
    g.resetCamera();
    g.drawString("HUD", 124, 61, ON, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT,
                 Graphics::VerticalAlign::BOTTOM);
}

void PruzeaAPIsSSD1306::drawClipRectTest(Graphics& g) {
    constexpr int16_t clipX = 28;
    constexpr int16_t clipY = 20;
    constexpr uint16_t clipW = 72;
    constexpr uint16_t clipH = 32;

    g.setClipRect(clipX, clipY, clipW, clipH);

    const int16_t x = getAnimX(Platform::getMsec());
    g.fillCircle(x, 36, 18, ON);
    g.drawLine(0, 18, TARGET_W - 1, 58, ON);
    g.drawLine(0, 58, TARGET_W - 1, 18, ON);

    g.resetClipRect();
    g.drawRect(clipX, clipY, clipW, clipH, ON);
    g.drawString("CLIP", 64, 36, OFF, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
}

void PruzeaAPIsSSD1306::drawSpriteTest(Graphics& g) {
    const int16_t x = getAnimX(Platform::getMsec());

    g.drawSprite(TEST_SPRITE, x - 12, 20, 8, 8, {
        .transparent = true,
        .transparentColor = OFF
    });
    g.drawSprite(TEST_SPRITE, x, 28, 8, 8, {
        .scale = 2,
        .transparent = true,
        .transparentColor = OFF
    });
    g.drawSprite(TEST_SPRITE, x + 20, 36, 8, 8, {
        .scale = 3,
        .transparent = true,
        .transparentColor = OFF
    });
}

void PruzeaAPIsSSD1306::drawSpriteTransformTest(Graphics& g) {
    const uint32_t elapsed = Platform::getMsec() - stepStartMsec;
    const float angle = static_cast<float>(elapsed) * 0.003f;

    g.drawSprite(TEST_SPRITE, 21, 27, 8, 8, {
        .scale = 2,
        .angle = angle,
        .transparent = true,
        .transparentColor = OFF
    });
    g.drawSprite(TEST_SPRITE, 57, 27, 8, 8, {
        .scale = 2,
        .flipX = true,
        .transparent = true,
        .transparentColor = OFF
    });
    g.drawSprite(TEST_SPRITE, 93, 27, 8, 8, {
        .scale = 2,
        .flipY = true,
        .transparent = true,
        .transparentColor = OFF
    });

    g.drawString("ROT", 29, 58, ON, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::BOTTOM);
    g.drawString("FX", 65, 58, ON, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::BOTTOM);
    g.drawString("FY", 101, 58, ON, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::BOTTOM);
}

void PruzeaAPIsSSD1306::drawSpriteSheetTest(Graphics& g) {
    const uint32_t elapsed = Platform::getMsec() - stepStartMsec;
    const uint8_t frame = static_cast<uint8_t>((elapsed / 450u) % 4u);
    const uint16_t column = frame % 2u;
    const uint16_t row = frame / 2u;

    g.drawSprite(TEST_SHEET, column, row, 48, 23, {
        .scale = 4,
        .transparent = true,
        .transparentColor = OFF
    });

    char text[16];
    snprintf(text, sizeof(text), "C%u R%u",
             static_cast<unsigned>(column),
             static_cast<unsigned>(row));
    g.drawString(text, 64, 61, ON, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::BOTTOM);
}

void PruzeaAPIsSSD1306::drawInputOverlay(Graphics& g) {
    if ((inputMask & Input::UP) != 0) {
        g.drawLine(0, 0, TARGET_W - 1, 0, ON);
    }
    if ((inputMask & Input::DOWN) != 0) {
        g.drawLine(0, TARGET_H - 1, TARGET_W - 1, TARGET_H - 1, ON);
    }
    if ((inputMask & Input::LEFT) != 0) {
        g.drawLine(0, 0, 0, TARGET_H - 1, ON);
    }
    if ((inputMask & Input::RIGHT) != 0) {
        g.drawLine(TARGET_W - 1, 0, TARGET_W - 1, TARGET_H - 1, ON);
    }

    // A/B input lamps. Outline is always visible; fill means pressed.
    if ((inputMask & Input::A) != 0) g.fillCircle(121, 21, 4, ON);
    else g.drawCircle(121, 21, 4, ON);

    if ((inputMask & Input::B) != 0) g.fillCircle(121, 48, 4, ON);
    else g.drawCircle(121, 48, 4, ON);

    g.drawString("A", 121, 21, (inputMask & Input::A) != 0 ? OFF : ON,
                 Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
    g.drawString("B", 121, 48, (inputMask & Input::B) != 0 ? OFF : ON,
                 Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);

    if (speakerIconDurationMsec != 0) {
        drawSpeakerIcon(g, 108, 1);
    }
}

void PruzeaAPIsSSD1306::drawSpeakerIcon(Graphics& g, int16_t x, int16_t y) {
    g.fillRect(x, y + 3, 3, 4, ON);
    g.fillTriangle(x + 2, y + 3, x + 6, y, x + 6, y + 9, ON);
    g.drawLine(x + 8, y + 2, x + 9, y + 3, ON);
    g.drawLine(x + 9, y + 3, x + 9, y + 6, ON);
    g.drawLine(x + 9, y + 6, x + 8, y + 7, ON);
}

int16_t PruzeaAPIsSSD1306::getAnimX(uint32_t now) const {
    const uint32_t elapsed = now - stepStartMsec;
    const uint32_t travelMsec = (STEP_MSEC - HOLD_MSEC) / 2;

    if (elapsed < travelMsec) {
        return static_cast<int16_t>(150 -
            (86L * static_cast<int32_t>(elapsed)) /
            static_cast<int32_t>(travelMsec));
    }

    if (elapsed < travelMsec + HOLD_MSEC) {
        return 64;
    }

    uint32_t outElapsed = elapsed - travelMsec - HOLD_MSEC;
    if (outElapsed > travelMsec) outElapsed = travelMsec;

    return static_cast<int16_t>(64 -
        (96L * static_cast<int32_t>(outElapsed)) /
        static_cast<int32_t>(travelMsec));
}

const char* PruzeaAPIsSSD1306::getStepName() const {
    return drawStep < STEP_COUNT ? STEP_NAMES[drawStep] : "Graphics";
}

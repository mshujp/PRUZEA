#include "MazeEscape.h"

using namespace PRUZEA;

namespace
{
static const char MAZE[30][41] =
{
    "########################################",
    "#S....#.........#.............#.......##",
    "#####.#####.###.#########.###.#####.#.##",
    "#...#.......#.#...........#.#...#...#.##",
    "#.#.#.##.####.#############.###.#.###.##",
    "#...........#.......#.#.......#.#.#...##",
    "#.######.##.#.#####.#.#.#.###.#.#.###.##",
    "#.........#...#.....#.#.#.......#...#.##",
    "#.#######.#######...#.#.###########...##",
    "#...#.....#.....#...........#.......#.##",
    "###.#.#####.###.#####.#####.#.###..##.##",
    "#.....#.....#.#...#.#.#...#.....#...#.##",
    "#.###.#.#####.###.#.#.###.#####.###.####",
    "#.....#.........#.#...#...#...#...#...##",
    "###.#.###.###.###...###.#.###.###.#.#.##",
    "#...#.........#...#.#...#.....#...#.#.##",
    "#.###########.#.###.#.#.#####.#.#####.##",
    "#.#...#.#...#.#.#.#.....#...#.#...#...##",
    "#.#.#.#.#.#.###.#.###.#####.#.###.#.####",
    "#...#...#.#.#...#...#.#...#.....#.#...##",
    "#####.###.#.#.###.###.#G#.#####.#.###.##",
    "#...#.#...#...#.......#.#.....#.#...#.##",
    "#.#.#.#.###########.###.#####.#.###.#.##",
    "#.#.#.#...........#...#...#...#...#...##",
    "###.#.#########.#.#######.#.#.######..##",
    "#...#.#.........#.......#...#.....#...##",
    "#.###.#########.#######.###.#####.#.####",
    "#.....................#.........#.....##",
    "########################################",
    "########################################"
};

constexpr Graphics::Color FLOOR_COLOR = Graphics::rgb565(18, 20, 28);
constexpr Graphics::Color WALL_COLOR = Graphics::rgb565(40, 74, 118);
constexpr Graphics::Color WALL_EDGE_COLOR = Graphics::rgb565(82, 132, 186);
constexpr Graphics::Color PLAYER_COLOR = Graphics::rgb565(255, 132, 20);
constexpr Graphics::Color GOAL_COLOR = Graphics::rgb565(64, 220, 120);
constexpr Graphics::Color PANEL_COLOR = Graphics::BLACK;
constexpr uint8_t PANEL_ALPHA = 168;
}

const char* MazeEscape::getId() const
{
    return "maze_escape";
}

const char* MazeEscape::getName() const
{
    return "MAZE ESCAPE";
}

const char* MazeEscape::getMenuName() const
{
    return "16 Maze Escape";
}

const char* MazeEscape::getMenuGroup() const
{
    return "SAMPLES";
}

uint16_t MazeEscape::getLogicalScreenWidth() const
{
    return SCREEN_W;
}

uint16_t MazeEscape::getLogicalScreenHeight() const
{
    return SCREEN_H;
}

uint16_t MazeEscape::getTargetScreenWidth() const
{
    return Display::ILI9341_SCREEN_W;
}

uint16_t MazeEscape::getTargetScreenHeight() const
{
    return Display::ILI9341_SCREEN_H;
}

void MazeEscape::onInit(Storage& storage)
{
    (void)storage;
    resetGame();
    mode = MODE_TITLE;
}

Game::GameState MazeEscape::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    (void)storage;

    switch (mode)
    {
    case MODE_TITLE:
        if (input.justPressed(Input::A) || input.justPressed(Input::START))
        {
            startIntro();
            audio.playSE(&Audio::SE::NO_1, 0.6f);
        }
        else
        {
            // Keep redrawing only while the title message is scrolling.
            dirty = true;
        }
        break;

    case MODE_INTRO:
        updateIntro();
        break;

    case MODE_PLAYING:
        updatePlayer(input, deltaSec);
        updatePlayCamera();

        if (isGoalReached())
        {
            startClear();
            audio.playSE(&Audio::SE::NO_8, 0.8f);
        }
        break;

    case MODE_CLEAR:
        updateClear();

        if (clearAnimationDone && (input.justPressed(Input::A) || input.justPressed(Input::START)))
        {
            resetGame();
            startIntro();
            audio.playSE(&Audio::SE::NO_1, 0.6f);
        }
        break;
    }

    return Game::GameState::RUNNING;
}

bool MazeEscape::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty)
    {
        return false;
    }

    graphics.resetCamera();
    graphics.resetClipRect();
    graphics.fillScreen(Graphics::BLACK);

    applyCamera(graphics);

    if (mode == MODE_PLAYING)
    {
        applyLightClip(graphics, LIGHT_SIZE, LIGHT_SIZE);
    }
    else if (mode == MODE_INTRO)
    {
        const float clipW = Math::lerp(static_cast<float>(SCREEN_W), static_cast<float>(LIGHT_SIZE), transitionProgress);
        const float clipH = Math::lerp(static_cast<float>(SCREEN_H), static_cast<float>(LIGHT_SIZE), transitionProgress);
        applyLightClip(graphics, clipW, clipH);
    }
    else if (mode == MODE_CLEAR && !clearAnimationDone)
    {
        const float clipW = Math::lerp(static_cast<float>(LIGHT_SIZE), static_cast<float>(SCREEN_W), transitionProgress);
        const float clipH = Math::lerp(static_cast<float>(LIGHT_SIZE), static_cast<float>(SCREEN_H), transitionProgress);
        applyLightClip(graphics, clipW, clipH);
    }

    drawMaze(graphics);
    drawPlayer(graphics);

    graphics.resetCamera();
    graphics.resetClipRect();

    if (mode == MODE_TITLE)
    {
        drawTitle(graphics);
    }
    else if (mode == MODE_CLEAR && clearAnimationDone)
    {
        drawClear(graphics);
    }

    dirty = false;
    return true;
}

void MazeEscape::onTerminate(Storage& storage)
{
    (void)storage;
}

void MazeEscape::resetGame()
{
    player.x = TILE_SIZE * 1.5f;
    player.y = TILE_SIZE * 1.5f;
    facingAngle = 0.0f;
    mouthAnimation.reset();
    playerMoving = false;

    getFocusCameraTarget(cameraX, cameraY);
    cameraZoom = FOCUS_ZOOM;

    transitionStartMsec = 0;
    transitionCameraStartX = cameraX;
    transitionCameraStartY = cameraY;
    transitionProgress = 0.0f;
    clearAnimationDone = false;

    dirty = true;
}

void MazeEscape::startIntro()
{
    mode = MODE_INTRO;
    transitionStartMsec = Platform::getMsec();
    transitionCameraStartX = cameraX;
    transitionCameraStartY = cameraY;
    transitionProgress = 0.0f;
    playerMoving = false;
    mouthAnimation.reset();
    dirty = true;
}

void MazeEscape::updateIntro()
{
    const uint32_t now = Platform::getMsec();
    float t = static_cast<float>(now - transitionStartMsec) / static_cast<float>(INTRO_MSEC);
    t = Math::clamp(t, 0.0f, 1.0f);

    const float eased = Tween::apply(t, Tween::EASE_IN_OUT);
    float targetX;
    float targetY;
    getPlayCameraTarget(targetX, targetY);

    cameraX = Math::lerp(transitionCameraStartX, targetX, eased);
    cameraY = Math::lerp(transitionCameraStartY, targetY, eased);
    cameraZoom = Math::lerp(FOCUS_ZOOM, 1.0f, eased);
    transitionProgress = eased;

    if (t >= 1.0f)
    {
        mode = MODE_PLAYING;
        updatePlayCamera();
        transitionProgress = 1.0f;
    }

    dirty = true;
}

void MazeEscape::updatePlayer(Input& input, float deltaSec)
{
    Vector2 direction;

    if (input.pressed(Input::LEFT))
    {
        direction.x -= 1.0f;
    }
    if (input.pressed(Input::RIGHT))
    {
        direction.x += 1.0f;
    }
    if (input.pressed(Input::UP))
    {
        direction.y -= 1.0f;
    }
    if (input.pressed(Input::DOWN))
    {
        direction.y += 1.0f;
    }

    const bool wasMoving = playerMoving;
    playerMoving = direction.x != 0.0f || direction.y != 0.0f;
    if (!playerMoving)
    {
        if (wasMoving)
        {
            mouthAnimation.reset();
            dirty = true;
        }
        return;
    }

    if (!mouthAnimation.isPlaying())
    {
        mouthAnimation.start();
    }
    mouthAnimation.update(deltaSec);

    direction = direction.normalized();
    facingAngle = Math::angle(direction.x, direction.y);

    player += direction * (PLAYER_SPEED * deltaSec);
    resolveWallCollisions();

    dirty = true;
}

void MazeEscape::getPlayCameraTarget(float& x, float& y) const
{
    x = Math::clamp(
        player.x - SCREEN_W * 0.5f,
        0.0f,
        static_cast<float>(WORLD_W - SCREEN_W));
    y = Math::clamp(
        player.y - SCREEN_H * 0.5f,
        0.0f,
        static_cast<float>(WORLD_H - SCREEN_H));
}

void MazeEscape::getFocusCameraTarget(float& x, float& y) const
{
    x = player.x - SCREEN_W * 0.5f;
    y = player.y - SCREEN_H * 0.5f;
}

void MazeEscape::updatePlayCamera()
{
    getPlayCameraTarget(cameraX, cameraY);
    cameraZoom = 1.0f;
}

void MazeEscape::startClear()
{
    mode = MODE_CLEAR;
    transitionStartMsec = Platform::getMsec();
    transitionCameraStartX = cameraX;
    transitionCameraStartY = cameraY;
    transitionProgress = 0.0f;
    clearAnimationDone = false;
    playerMoving = false;
    mouthAnimation.reset();
    dirty = true;
}

void MazeEscape::updateClear()
{
    if (clearAnimationDone)
    {
        return;
    }

    const uint32_t now = Platform::getMsec();
    float t = static_cast<float>(now - transitionStartMsec) / static_cast<float>(CLEAR_MSEC);
    t = Math::clamp(t, 0.0f, 1.0f);

    const float eased = Tween::apply(t, Tween::EASE_IN_OUT);
    float targetX;
    float targetY;
    getFocusCameraTarget(targetX, targetY);

    cameraX = Math::lerp(transitionCameraStartX, targetX, eased);
    cameraY = Math::lerp(transitionCameraStartY, targetY, eased);
    cameraZoom = Math::lerp(1.0f, FOCUS_ZOOM, eased);
    transitionProgress = eased;

    if (t >= 1.0f)
    {
        cameraX = targetX;
        cameraY = targetY;
        cameraZoom = FOCUS_ZOOM;
        transitionProgress = 1.0f;
        clearAnimationDone = true;
    }

    dirty = true;
}

void MazeEscape::resolveWallCollisions()
{
    const int16_t tileX = static_cast<int16_t>(player.x) / TILE_SIZE;
    const int16_t tileY = static_cast<int16_t>(player.y) / TILE_SIZE;

    const int16_t minX = Math::clamp<int16_t>(tileX - 1, 0, MAP_W - 1);
    const int16_t maxX = Math::clamp<int16_t>(tileX + 1, 0, MAP_W - 1);
    const int16_t minY = Math::clamp<int16_t>(tileY - 1, 0, MAP_H - 1);
    const int16_t maxY = Math::clamp<int16_t>(tileY + 1, 0, MAP_H - 1);

    for (int16_t y = minY; y <= maxY; ++y)
    {
        for (int16_t x = minX; x <= maxX; ++x)
        {
            if (MAZE[y][x] != '#')
            {
                continue;
            }

            Vector2 pushOut;
            if (Collision::circleRect(
                    player.x,
                    player.y,
                    PLAYER_RADIUS,
                    static_cast<float>(x * TILE_SIZE),
                    static_cast<float>(y * TILE_SIZE),
                    static_cast<float>(TILE_SIZE),
                    static_cast<float>(TILE_SIZE),
                    pushOut))
            {
                player += pushOut;
            }
        }
    }
}

bool MazeEscape::isGoalReached() const
{
    const int16_t tileX = static_cast<int16_t>(player.x) / TILE_SIZE;
    const int16_t tileY = static_cast<int16_t>(player.y) / TILE_SIZE;

    if (tileX < 0 || tileX >= MAP_W || tileY < 0 || tileY >= MAP_H)
    {
        return false;
    }

    if (MAZE[tileY][tileX] != 'G')
    {
        return false;
    }

    return Collision::circleRect(
        player.x,
        player.y,
        PLAYER_RADIUS,
        static_cast<float>(tileX * TILE_SIZE + 2),
        static_cast<float>(tileY * TILE_SIZE + 2),
        static_cast<float>(TILE_SIZE - 4),
        static_cast<float>(TILE_SIZE - 4));
}

void MazeEscape::applyCamera(Graphics& graphics) const
{
    Graphics::Camera camera;
    camera.x = static_cast<int16_t>(cameraX);
    camera.y = static_cast<int16_t>(cameraY);
    camera.zoom = cameraZoom;
    camera.zoomCenterX = SCREEN_W / 2;
    camera.zoomCenterY = SCREEN_H / 2;
    graphics.setCamera(camera);
}

void MazeEscape::applyLightClip(Graphics& graphics, float width, float height) const
{
    const int16_t clipW = Math::clamp<int16_t>(static_cast<int16_t>(Math::round(width)), 1, SCREEN_W);
    const int16_t clipH = Math::clamp<int16_t>(static_cast<int16_t>(Math::round(height)), 1, SCREEN_H);

    const float playerScreenX = SCREEN_W * 0.5f +
        (player.x - cameraX - SCREEN_W * 0.5f) * cameraZoom;
    const float playerScreenY = SCREEN_H * 0.5f +
        (player.y - cameraY - SCREEN_H * 0.5f) * cameraZoom;

    const int16_t clipX = Math::clamp<int16_t>(
        static_cast<int16_t>(Math::round(playerScreenX - clipW * 0.5f)),
        0,
        SCREEN_W - clipW);
    const int16_t clipY = Math::clamp<int16_t>(
        static_cast<int16_t>(Math::round(playerScreenY - clipH * 0.5f)),
        0,
        SCREEN_H - clipH);

    graphics.resetCamera();
    graphics.setClipRect(clipX, clipY, static_cast<uint16_t>(clipW), static_cast<uint16_t>(clipH));
    applyCamera(graphics);
}

void MazeEscape::drawMaze(Graphics& graphics) const
{
    const float zoom = cameraZoom > 0.01f ? cameraZoom : 1.0f;
    const float centerX = SCREEN_W * 0.5f;
    const float centerY = SCREEN_H * 0.5f;

    const float worldLeft = cameraX + centerX + (0.0f - centerX) / zoom;
    const float worldRight = cameraX + centerX + (SCREEN_W - centerX) / zoom;
    const float worldTop = cameraY + centerY + (0.0f - centerY) / zoom;
    const float worldBottom = cameraY + centerY + (SCREEN_H - centerY) / zoom;

    const int16_t minX = Math::clamp<int16_t>(static_cast<int16_t>(Math::floor(worldLeft / TILE_SIZE)) - 1, 0, MAP_W - 1);
    const int16_t maxX = Math::clamp<int16_t>(static_cast<int16_t>(Math::floor(worldRight / TILE_SIZE)) + 1, 0, MAP_W - 1);
    const int16_t minY = Math::clamp<int16_t>(static_cast<int16_t>(Math::floor(worldTop / TILE_SIZE)) - 1, 0, MAP_H - 1);
    const int16_t maxY = Math::clamp<int16_t>(static_cast<int16_t>(Math::floor(worldBottom / TILE_SIZE)) + 1, 0, MAP_H - 1);

    const int16_t floorX = minX * TILE_SIZE;
    const int16_t floorY = minY * TILE_SIZE;
    const uint16_t floorW = static_cast<uint16_t>((maxX - minX + 1) * TILE_SIZE);
    const uint16_t floorH = static_cast<uint16_t>((maxY - minY + 1) * TILE_SIZE);
    graphics.fillRect(floorX, floorY, floorW, floorH, FLOOR_COLOR);

    for (int16_t y = minY; y <= maxY; ++y)
    {
        for (int16_t x = minX; x <= maxX; ++x)
        {
            const int16_t px = x * TILE_SIZE;
            const int16_t py = y * TILE_SIZE;
            const char tile = MAZE[y][x];

            if (tile == '#')
            {
                graphics.fillRect(px, py, TILE_SIZE, TILE_SIZE, WALL_COLOR);
                graphics.drawRect(px, py, TILE_SIZE, TILE_SIZE, WALL_EDGE_COLOR);
            }
            else if (tile == 'G')
            {
                graphics.fillRect(px + 3, py + 3, TILE_SIZE - 6, TILE_SIZE - 6, GOAL_COLOR);
                graphics.drawRect(px + 2, py + 2, TILE_SIZE - 4, TILE_SIZE - 4, Graphics::WHITE);
            }
        }
    }
}

void MazeEscape::drawPlayer(Graphics& graphics) const
{
    static constexpr float MOUTH_ANGLES[6] =
    {
        0.10f,
        0.30f,
        0.55f,
        0.90f,
        0.55f,
        0.30f
    };

    const float dirX = Math::cos(facingAngle);
    const float dirY = Math::sin(facingAngle);
    const float sideX = -dirY;
    const float sideY = dirX;

    const int16_t tailX = static_cast<int16_t>(Math::round(player.x - dirX * (PLAYER_RADIUS + 2.0f)));
    const int16_t tailY = static_cast<int16_t>(Math::round(player.y - dirY * (PLAYER_RADIUS + 2.0f)));
    graphics.fillCircle(tailX, tailY, 3, PLAYER_COLOR);

    const int frame = playerMoving ? mouthAnimation.frame() : 0;
    const float mouthHalfAngle = MOUTH_ANGLES[frame];
    const float startAngle = facingAngle + mouthHalfAngle;
    const float endAngle = facingAngle + Math::TWO_PI - mouthHalfAngle;

    graphics.fillArc(
        static_cast<int16_t>(player.x),
        static_cast<int16_t>(player.y),
        static_cast<uint16_t>(PLAYER_RADIUS),
        startAngle,
        endAngle,
        PLAYER_COLOR);

    const int16_t eyeX = static_cast<int16_t>(Math::round(player.x + dirX * 2.0f - sideX * 3.0f));
    const int16_t eyeY = static_cast<int16_t>(Math::round(player.y + dirY * 2.0f - sideY * 3.0f));
    graphics.fillCircle(eyeX, eyeY, 2, Graphics::WHITE);
    graphics.fillCircle(eyeX, eyeY, 1, Graphics::BLACK);
}

void MazeEscape::drawTitle(Graphics& graphics) const
{
    constexpr int16_t PANEL_W = 240;
    constexpr int16_t PANEL_H = 112;
    constexpr int16_t PANEL_X = (SCREEN_W - PANEL_W) / 2;
    constexpr int16_t PANEL_Y = 64;

    constexpr int16_t MESSAGE_X = PANEL_X + 10;
    constexpr int16_t MESSAGE_Y = PANEL_Y + 82;
    constexpr uint16_t MESSAGE_W = PANEL_W - 20;
    constexpr uint16_t MESSAGE_H = 18;
    constexpr float MESSAGE_SPEED = 36.0f;
    constexpr int16_t MESSAGE_GAP = 48;

    static const char* MESSAGE =
        "LONG AGO, A TINY ORANGE WANDERER ENTERED THE MAZE OF SHADOWS... "
        "FIND THE LIGHT AND ESCAPE.";

    graphics.fillRectAlpha(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_ALPHA, PANEL_COLOR);
    graphics.drawRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, Graphics::WHITE);
    graphics.drawString(
        "MAZE ESCAPE",
        SCREEN_W / 2,
        PANEL_Y + 24,
        PLAYER_COLOR,
        Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
    graphics.drawString(
        "A / START : BEGIN",
        SCREEN_W / 2,
        PANEL_Y + 58,
        Graphics::WHITE,
        Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
    
    graphics.fillRectGradient(
        MESSAGE_X,
        MESSAGE_Y - 6,
        MESSAGE_W,
        2,
        WALL_EDGE_COLOR,
        PLAYER_COLOR,
        Graphics::HORIZONRAL_LINEAR);

    // Typical ClipRect use: the text keeps moving outside this narrow window,
    // but only the portion inside the message area is visible.
    const int16_t messageWidth = static_cast<int16_t>(graphics.getTextWidth(MESSAGE, Graphics::SIZE_13));
    const float cycleWidth = static_cast<float>(MESSAGE_W + messageWidth + MESSAGE_GAP);
    const float travel = Math::wrap(Platform::getMsec() * (MESSAGE_SPEED / 1000.0f), 0.0f, cycleWidth);
    const int16_t messageX = static_cast<int16_t>(Math::round(MESSAGE_X + MESSAGE_W - travel));

    graphics.setClipRect(MESSAGE_X, MESSAGE_Y, MESSAGE_W, MESSAGE_H);
    graphics.drawString(
        MESSAGE,
        messageX,
        MESSAGE_Y + MESSAGE_H / 2,
        Graphics::WHITE,
        Graphics::SIZE_13,
        Graphics::HorizontalAlign::LEFT,
        Graphics::VerticalAlign::MIDDLE);
    graphics.resetClipRect();
}

void MazeEscape::drawClear(Graphics& graphics) const
{
    constexpr int16_t PANEL_W = 220;
    constexpr int16_t PANEL_H = 82;
    constexpr int16_t PANEL_X = (SCREEN_W - PANEL_W) / 2;
    constexpr int16_t PANEL_Y = 72;

    graphics.fillRectAlpha(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_ALPHA, PANEL_COLOR);
    graphics.drawRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, Graphics::WHITE);
    graphics.drawString(
        "ESCAPED!",
        SCREEN_W / 2,
        PANEL_Y + 25,
        GOAL_COLOR,
        Graphics::SIZE_25B,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
    graphics.drawString(
        "A / START : AGAIN",
        SCREEN_W / 2,
        PANEL_Y + 58,
        Graphics::WHITE,
        Graphics::SIZE_18,
        Graphics::HorizontalAlign::CENTER,
        Graphics::VerticalAlign::MIDDLE);
}

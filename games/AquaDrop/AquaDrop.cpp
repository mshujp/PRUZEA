#include "AquaDrop.h"

#include <cstdio>
#include <cstring>

using namespace PRUZEA;

constexpr uint8_t AquaDrop::STAGE_COLOR_COUNT[AquaDrop::STAGE_COUNT];
constexpr uint16_t AquaDrop::STAGE_TARGET[AquaDrop::STAGE_COUNT];
constexpr uint8_t AquaDrop::STAGE_INITIAL_ROWS[AquaDrop::STAGE_COUNT];
constexpr float AquaDrop::STAGE_FALL_INTERVAL_SEC[AquaDrop::STAGE_COUNT];

const char* AquaDrop::getId() const
{
    return "aqua_drop";
}

const char* AquaDrop::getName() const
{
    return "AQUA DROP";
}

const char* AquaDrop::getMenuName() const
{
    return "AQUA DROP";
}

uint16_t AquaDrop::getLogicalScreenWidth() const
{
    return Display::ILI9341_SCREEN_W;
}

uint16_t AquaDrop::getLogicalScreenHeight() const
{
    return Display::ILI9341_SCREEN_H;
}

uint16_t AquaDrop::getTargetScreenWidth() const
{
    return Display::ILI9341_SCREEN_W;
}

uint16_t AquaDrop::getTargetScreenHeight() const
{
    return Display::ILI9341_SCREEN_H;
}

void AquaDrop::onInit(Storage& storage)
{
    saveData.clear();
    if (storage.isAvailable())
    {
        saveData.load(storage, getId(), "save.ini");
    }
    highScore = saveData.getInt32("HIGH_SCORE", 0);
    saveDirty = false;
    resetToTitle();
    dirty = true;
}

Game::GameState AquaDrop::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    const uint32_t now = Platform::getMsec();

    switch (mode)
    {
    case MODE_TITLE:
        if (input.justPressed(Input::A) || input.justPressed(Input::START))
        {
            audio.playSE(&Audio::SE::NO_8, 0.65f);
            startNewGame();
        }
        if ((now / TITLE_BLINK_MSEC) != ((now - static_cast<uint32_t>(deltaSec * 1000.0f)) / TITLE_BLINK_MSEC))
        {
            dirty = true;
        }
        break;

    case MODE_STAGE_INTRO:
        if (Platform::elapsed(now, modeStartMsec, STAGE_INTRO_MSEC))
        {
            enterMode(MODE_PLAYING);
        }
        else
        {
            dirty = true;
        }
        break;

    case MODE_PLAYING:
    {
        if (input.justPressed(Input::START))
        {
            modeBeforePause = MODE_PLAYING;
            enterMode(MODE_PAUSED);
            audio.stopMusic();
            audio.playSE(&Audio::SE::NO_1, 0.45f);
            break;
        }

        bool moved = false;
        if (input.repeat(Input::LEFT))
        {
            moved = tryMove(-1, 0);
        }
        else if (input.repeat(Input::RIGHT))
        {
            moved = tryMove(1, 0);
        }
        if (moved)
        {
            audio.playSE(&Audio::SE::NO_1, 0.28f);
        }

        if (input.justPressed(Input::A) || input.justPressed(Input::UP))
        {
            if (tryRotate(1))
            {
                audio.playSE(&Audio::SE::NO_1, 0.35f);
            }
        }
        else if (input.justPressed(Input::B))
        {
            if (tryRotate(-1))
            {
                audio.playSE(&Audio::SE::NO_1, 0.35f);
            }
        }

        float speed = 1.0f;
        if (input.pressed(Input::DOWN))
        {
            speed = SOFT_DROP_MULTIPLIER;
        }
        fallAccumulatorSec += deltaSec * speed;
        const float interval = STAGE_FALL_INTERVAL_SEC[stageIndex];
        while (fallAccumulatorSec >= interval)
        {
            fallAccumulatorSec -= interval;
            if (!tryMove(0, 1))
            {
                lockPair();
                audio.playSE(&Audio::SE::NO_5, 0.32f);
                chainCount = 0;
                if (findClearGroups())
                {
                    enterMode(MODE_CLEARING);
                }
                else
                {
                    spawnPair();
                    if (isBoardOverflowed() || !canPlacePair(pivotX, pivotY, orientation))
                    {
                        audio.playSE(&Audio::SE::NO_12, 0.70f);
                        enterMode(MODE_GAME_OVER);
                    }
                }
                break;
            }
        }
        dirty = true;
        break;
    }

    case MODE_CLEARING:
        if (Platform::elapsed(now, modeStartMsec, CLEAR_FLASH_MSEC))
        {
            removeMarkedBubbles();
            chainCount++;
            displayedChain = chainCount;
            chainDisplayStartMsec = now;
            const int32_t chainMultiplier = static_cast<int32_t>(chainCount);
            score += static_cast<int32_t>(pendingClearCount) * SCORE_PER_BUBBLE * chainMultiplier;
            if (pendingClearCount > 4)
            {
                score += static_cast<int32_t>(pendingClearCount - 4) * GROUP_BONUS_PER_EXTRA;
            }
            stageClearedCount += pendingClearCount;
            updateHighScore();
            audio.playSE(chainCount >= 2 ? &Audio::SE::NO_3 : &Audio::SE::NO_6, chainCount >= 2 ? 0.72f : 0.52f);
            if (chainCount >= 3)
            {
                shakeStartMsec = now;
                shakeDurationMsec = 220;
            }
            applyGravity();
            enterMode(MODE_FALLING);
        }
        else
        {
            dirty = true;
        }
        break;

    case MODE_FALLING:
        if (Platform::elapsed(now, modeStartMsec, FALL_SETTLE_MSEC))
        {
            if (findClearGroups())
            {
                enterMode(MODE_CLEARING);
            }
            else if (stageClearedCount >= STAGE_TARGET[stageIndex])
            {
                finishStage(audio, storage);
            }
            else
            {
                spawnPair();
                if (isBoardOverflowed() || !canPlacePair(pivotX, pivotY, orientation))
                {
                    audio.playSE(&Audio::SE::NO_12, 0.70f);
                    enterMode(MODE_GAME_OVER);
                }
                else
                {
                    enterMode(MODE_PLAYING);
                }
            }
        }
        else
        {
            dirty = true;
        }
        break;

    case MODE_STAGE_CLEAR:
        if (Platform::elapsed(now, modeStartMsec, STAGE_CLEAR_MSEC))
        {
            if (stageIndex + 1 >= STAGE_COUNT)
            {
                enterMode(MODE_ALL_CLEAR);
            }
            else
            {
                stageIndex++;
                prepareStage();
            }
        }
        else
        {
            dirty = true;
        }
        break;

    case MODE_PAUSED:
        if (input.justPressed(Input::START))
        {
            mode = modeBeforePause;
            modeStartMsec = now;
            audio.playSE(&Audio::SE::NO_1, 0.45f);
            dirty = true;
        }
        else if (input.justPressed(Input::SELECT))
        {
            saveIfNeeded(storage);
            resetToTitle();
            audio.playSE(&Audio::SE::NO_2, 0.45f);
        }
        break;

    case MODE_GAME_OVER:
        if (Platform::elapsed(now, modeStartMsec, GAME_OVER_MSEC))
        {
            finishGameOver(storage);
        }
        else
        {
            dirty = true;
        }
        break;

    case MODE_ALL_CLEAR:
        if (Platform::elapsed(now, modeStartMsec, ALL_CLEAR_MSEC) || input.justPressed(Input::A) || input.justPressed(Input::START))
        {
            saveIfNeeded(storage);
            resetToTitle();
        }
        else
        {
            dirty = true;
        }
        break;
    }

    if (displayedChain > 0 && !Platform::elapsed(now, chainDisplayStartMsec, CHAIN_DISPLAY_MSEC))
    {
        dirty = true;
    }
    if (shakeDurationMsec > 0)
    {
        if (Platform::elapsed(now, shakeStartMsec, shakeDurationMsec))
        {
            shakeDurationMsec = 0;
            dirty = true;
        }
        else
        {
            dirty = true;
        }
    }

    return Game::GameState::RUNNING;
}

bool AquaDrop::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty)
    {
        return false;
    }

    const uint32_t now = Platform::getMsec();
    graphics.resetViewport();

    if (shakeDurationMsec > 0 && !Platform::elapsed(now, shakeStartMsec, shakeDurationMsec))
    {
        const int16_t offsetX = static_cast<int16_t>(Math::random(-3, 4));
        const int16_t offsetY = static_cast<int16_t>(Math::random(-2, 3));
        graphics.setViewport(offsetX, offsetY);
    }

    drawBackground(graphics);
    if (mode == MODE_TITLE)
    {
        drawTitle(graphics, now);
    }
    else
    {
        drawGame(graphics, now);
    }

    dirty = false;
    return true;
}

void AquaDrop::onTerminate(Storage& storage)
{
    saveIfNeeded(storage);
}

void AquaDrop::resetToTitle()
{
    mode = MODE_TITLE;
    modeStartMsec = Platform::getMsec();
    displayedChain = 0;
    shakeDurationMsec = 0;
    dirty = true;
}

void AquaDrop::startNewGame()
{
    stageIndex = 0;
    score = 0;
    stageClearedCount = 0;
    displayedChain = 0;
    chainCount = 0;
    prepareStage();
}

void AquaDrop::prepareStage()
{
    memset(board, 0, sizeof(board));
    memset(clearMask, 0, sizeof(clearMask));
    stageClearedCount = 0;
    chainCount = 0;
    displayedChain = 0;
    fallAccumulatorSec = 0.0f;
    generateNextPair();
    fillInitialBoard();
    spawnPair();
    enterMode(MODE_STAGE_INTRO);
}

void AquaDrop::fillInitialBoard()
{
    const uint8_t rows = STAGE_INITIAL_ROWS[stageIndex];
    for (uint8_t rowOffset = 0; rowOffset < rows; rowOffset++)
    {
        const int16_t y = BOARD_ROWS - 1 - rowOffset;
        for (int16_t x = 0; x < BOARD_COLS; x++)
        {
            if (Math::chance(0.72f))
            {
                uint8_t color = randomColor();
                if (x >= 3 && board[y][x - 1] == color && board[y][x - 2] == color && board[y][x - 3] == color)
                {
                    color = static_cast<uint8_t>((color % STAGE_COLOR_COUNT[stageIndex]) + 1);
                }
                board[y][x] = color;
            }
        }
    }
}

void AquaDrop::spawnPair()
{
    pivotX = 2;
    pivotY = 0;
    orientation = 0;
    pivotColor = nextPivotColor;
    childColor = nextChildColor;
    generateNextPair();
    fallAccumulatorSec = 0.0f;
}

void AquaDrop::generateNextPair()
{
    nextPivotColor = randomColor();
    nextChildColor = randomColor();
}

uint8_t AquaDrop::randomColor() const
{
    return static_cast<uint8_t>(Math::random(1, static_cast<int>(STAGE_COLOR_COUNT[stageIndex]) + 1));
}

bool AquaDrop::isCellFree(int16_t x, int16_t y) const
{
    if (x < 0 || x >= BOARD_COLS || y >= BOARD_ROWS)
    {
        return false;
    }
    if (y < 0)
    {
        return true;
    }
    return board[y][x] == EMPTY;
}

void AquaDrop::getChildPosition(int16_t x, int16_t y, uint8_t dir, int16_t& outX, int16_t& outY) const
{
    static constexpr int8_t DX[4] = {0, 1, 0, -1};
    static constexpr int8_t DY[4] = {-1, 0, 1, 0};
    outX = static_cast<int16_t>(x + DX[dir & 3]);
    outY = static_cast<int16_t>(y + DY[dir & 3]);
}

bool AquaDrop::canPlacePair(int16_t x, int16_t y, uint8_t dir) const
{
    int16_t childX;
    int16_t childY;
    getChildPosition(x, y, dir, childX, childY);
    return isCellFree(x, y) && isCellFree(childX, childY);
}

bool AquaDrop::tryMove(int16_t dx, int16_t dy)
{
    const int16_t nextX = static_cast<int16_t>(pivotX + dx);
    const int16_t nextY = static_cast<int16_t>(pivotY + dy);
    if (!canPlacePair(nextX, nextY, orientation))
    {
        return false;
    }
    pivotX = nextX;
    pivotY = nextY;
    dirty = true;
    return true;
}

bool AquaDrop::tryRotate(int8_t direction)
{
    const uint8_t newOrientation = static_cast<uint8_t>((orientation + (direction > 0 ? 1 : 3)) & 3);
    static constexpr int8_t KICK_X[5] = {0, -1, 1, -2, 2};
    for (uint8_t i = 0; i < 5; i++)
    {
        const int16_t testX = static_cast<int16_t>(pivotX + KICK_X[i]);
        if (canPlacePair(testX, pivotY, newOrientation))
        {
            pivotX = testX;
            orientation = newOrientation;
            dirty = true;
            return true;
        }
    }
    return false;
}

void AquaDrop::lockPair()
{
    int16_t childX;
    int16_t childY;
    getChildPosition(pivotX, pivotY, orientation, childX, childY);
    if (pivotY >= 0 && pivotY < BOARD_ROWS)
    {
        board[pivotY][pivotX] = pivotColor;
    }
    if (childY >= 0 && childY < BOARD_ROWS)
    {
        board[childY][childX] = childColor;
    }
    dirty = true;
}

bool AquaDrop::findClearGroups()
{
    bool visited[BOARD_ROWS][BOARD_COLS] = {};
    memset(clearMask, 0, sizeof(clearMask));
    pendingClearCount = 0;

    for (int16_t y = 0; y < BOARD_ROWS; y++)
    {
        for (int16_t x = 0; x < BOARD_COLS; x++)
        {
            if (board[y][x] != EMPTY && !visited[y][x])
            {
                const uint16_t groupSize = markGroup(x, y, visited);
                if (groupSize >= 4)
                {
                    pendingClearCount = static_cast<uint16_t>(pendingClearCount + groupSize);
                }
            }
        }
    }
    return pendingClearCount > 0;
}

uint16_t AquaDrop::markGroup(int16_t startX, int16_t startY, bool visited[BOARD_ROWS][BOARD_COLS])
{
    const uint8_t targetColor = board[startY][startX];
    int16_t queueX[BOARD_ROWS * BOARD_COLS];
    int16_t queueY[BOARD_ROWS * BOARD_COLS];
    uint16_t head = 0;
    uint16_t tail = 0;

    queueX[tail] = startX;
    queueY[tail] = startY;
    tail++;
    visited[startY][startX] = true;

    while (head < tail)
    {
        const int16_t x = queueX[head];
        const int16_t y = queueY[head];
        head++;
        static constexpr int8_t DX[4] = {1, -1, 0, 0};
        static constexpr int8_t DY[4] = {0, 0, 1, -1};
        for (uint8_t i = 0; i < 4; i++)
        {
            const int16_t nx = static_cast<int16_t>(x + DX[i]);
            const int16_t ny = static_cast<int16_t>(y + DY[i]);
            if (nx >= 0 && nx < BOARD_COLS && ny >= 0 && ny < BOARD_ROWS && !visited[ny][nx] && board[ny][nx] == targetColor)
            {
                visited[ny][nx] = true;
                queueX[tail] = nx;
                queueY[tail] = ny;
                tail++;
            }
        }
    }

    if (tail >= 4)
    {
        for (uint16_t i = 0; i < tail; i++)
        {
            clearMask[queueY[i]][queueX[i]] = true;
        }
    }
    return tail;
}

void AquaDrop::removeMarkedBubbles()
{
    for (int16_t y = 0; y < BOARD_ROWS; y++)
    {
        for (int16_t x = 0; x < BOARD_COLS; x++)
        {
            if (clearMask[y][x])
            {
                board[y][x] = EMPTY;
            }
        }
    }
    memset(clearMask, 0, sizeof(clearMask));
    dirty = true;
}

bool AquaDrop::applyGravity()
{
    bool moved = false;
    for (int16_t x = 0; x < BOARD_COLS; x++)
    {
        int16_t writeY = BOARD_ROWS - 1;
        for (int16_t y = BOARD_ROWS - 1; y >= 0; y--)
        {
            if (board[y][x] != EMPTY)
            {
                if (writeY != y)
                {
                    board[writeY][x] = board[y][x];
                    board[y][x] = EMPTY;
                    moved = true;
                }
                writeY--;
            }
        }
        while (writeY >= 0)
        {
            board[writeY][x] = EMPTY;
            writeY--;
        }
    }
    dirty = true;
    return moved;
}

bool AquaDrop::isBoardOverflowed() const
{
    return board[0][2] != EMPTY || board[0][3] != EMPTY;
}

void AquaDrop::enterMode(Mode newMode)
{
    mode = newMode;
    modeStartMsec = Platform::getMsec();
    dirty = true;
}

void AquaDrop::finishStage(Audio& audio, Storage& storage)
{
    score += STAGE_CLEAR_BONUS * static_cast<int32_t>(stageIndex + 1);
    updateHighScore();
    saveIfNeeded(storage);
    audio.playSE(&Audio::SE::NO_8, 0.78f);
    enterMode(MODE_STAGE_CLEAR);
}

void AquaDrop::finishGameOver(Storage& storage)
{
    updateHighScore();
    saveIfNeeded(storage);
    resetToTitle();
}

void AquaDrop::updateHighScore()
{
    if (score > highScore)
    {
        highScore = score;
        saveDirty = true;
    }
}

void AquaDrop::saveIfNeeded(Storage& storage)
{
    if (!saveDirty || !storage.isAvailable())
    {
        return;
    }
    saveData.setInt32("HIGH_SCORE", highScore);
    if (saveData.save(storage, getId(), "save.ini"))
    {
        saveDirty = false;
    }
}

void AquaDrop::drawBackground(Graphics& graphics) const
{
    const Graphics::Color deep = Graphics::rgb565(12, 77, 112);
    const Graphics::Color mid = Graphics::rgb565(22, 137, 164);
    const Graphics::Color light = Graphics::rgb565(83, 196, 201);
    const Graphics::Color sand = Graphics::rgb565(213, 204, 157);
    const Graphics::Color weed = Graphics::rgb565(28, 123, 104);

    graphics.fillScreen(deep);
    graphics.fillRect(0, 0, SCREEN_W, 76, light);
    graphics.fillRect(0, 76, SCREEN_W, 82, mid);
    graphics.fillRect(0, 158, SCREEN_W, 82, deep);
    graphics.fillRect(0, 228, SCREEN_W, 12, sand);

    for (int16_t x = 4; x < SCREEN_W; x += 24)
    {
        graphics.drawLine(x, 239, static_cast<int16_t>(x + ((x / 24) % 2 == 0 ? 5 : -4)), 216, weed);
        graphics.fillCircle(static_cast<int16_t>(x + 2), 215, 3, weed);
    }

    for (int16_t i = 0; i < 11; i++)
    {
        const int16_t x = static_cast<int16_t>(12 + i * 29);
        const int16_t y = static_cast<int16_t>(18 + (i * 37) % 188);
        const uint16_t r = static_cast<uint16_t>(2 + (i % 3));
        graphics.drawCircle(x, y, r, Graphics::rgb565(176, 236, 232));
    }
}

void AquaDrop::drawTitle(Graphics& graphics, uint32_t now) const
{
    const Graphics::Color panel = Graphics::rgb565(8, 61, 88);
    const Graphics::Color accent = Graphics::rgb565(122, 236, 219);
    const Graphics::Color soft = Graphics::rgb565(213, 247, 239);

    graphics.fillRoundRect(34, 40, 252, 148, 14, panel);
    graphics.drawRoundRect(34, 40, 252, 148, 14, 2, accent);
    graphics.drawString("AQUA", SCREEN_W / 2, 68, soft, Graphics::SIZE_42B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    graphics.drawString("DROP", SCREEN_W / 2, 108, accent, Graphics::SIZE_42B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    drawBubble(graphics, 72, 146, 1, false);
    drawBubble(graphics, 100, 150, 2, false);
    drawBubble(graphics, 220, 150, 3, false);
    drawBubble(graphics, 248, 146, 4, false);

    if (((now / TITLE_BLINK_MSEC) & 1u) == 0u)
    {
        graphics.drawString("PRESS A OR START", SCREEN_W / 2, 205, Graphics::WHITE, Graphics::SIZE_18, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }

    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "HI %ld", static_cast<long>(highScore));
    graphics.drawString(scoreText, SCREEN_W - 10, 16, Graphics::WHITE, Graphics::SIZE_18, Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);
}

void AquaDrop::drawGame(Graphics& graphics, uint32_t now) const
{
    drawBoard(graphics, now);
    if (mode == MODE_PLAYING || mode == MODE_PAUSED)
    {
        drawCurrentPair(graphics);
    }
    drawSidePanel(graphics, now);
    drawOverlay(graphics, now);
}

void AquaDrop::drawBoard(Graphics& graphics, uint32_t now) const
{
    const Graphics::Color boardBg = Graphics::rgb565(6, 45, 68);
    const Graphics::Color boardLine = Graphics::rgb565(69, 163, 176);
    graphics.fillRoundRect(BOARD_X - 5, BOARD_Y - 5, BOARD_W + 10, BOARD_H + 10, 8, boardBg);
    graphics.drawRoundRect(BOARD_X - 5, BOARD_Y - 5, BOARD_W + 10, BOARD_H + 10, 8, 2, boardLine);

    for (int16_t x = 1; x < BOARD_COLS; x++)
    {
        graphics.drawLine(BOARD_X + x * CELL_SIZE, BOARD_Y, BOARD_X + x * CELL_SIZE, BOARD_Y + BOARD_H, Graphics::rgb565(13, 65, 88));
    }
    for (int16_t y = 1; y < BOARD_ROWS; y++)
    {
        graphics.drawLine(BOARD_X, BOARD_Y + y * CELL_SIZE, BOARD_X + BOARD_W, BOARD_Y + y * CELL_SIZE, Graphics::rgb565(13, 65, 88));
    }

    const bool flashOn = ((now / 70u) & 1u) == 0u;
    for (int16_t y = 0; y < BOARD_ROWS; y++)
    {
        for (int16_t x = 0; x < BOARD_COLS; x++)
        {
            if (board[y][x] != EMPTY)
            {
                const bool flashing = mode == MODE_CLEARING && clearMask[y][x] && flashOn;
                drawBubble(graphics,
                           static_cast<int16_t>(BOARD_X + x * CELL_SIZE + CELL_SIZE / 2),
                           static_cast<int16_t>(BOARD_Y + y * CELL_SIZE + CELL_SIZE / 2),
                           board[y][x], flashing);
            }
        }
    }
}

void AquaDrop::drawBubble(Graphics& graphics, int16_t cx, int16_t cy, uint8_t colorIndex, bool flashing) const
{
    const Graphics::Color body = flashing ? Graphics::WHITE : bubbleColor(colorIndex);
    const Graphics::Color outline = Graphics::rgb565(7, 48, 67);
    const Graphics::Color shine = Graphics::rgb565(225, 255, 248);
    graphics.fillCircle(cx, cy, 7, outline);
    graphics.fillCircle(cx, cy, 6, body);
    graphics.fillCircle(static_cast<int16_t>(cx - 2), static_cast<int16_t>(cy - 2), 2, shine);
}

void AquaDrop::drawCurrentPair(Graphics& graphics) const
{
    int16_t childX;
    int16_t childY;
    getChildPosition(pivotX, pivotY, orientation, childX, childY);

    if (pivotY >= 0)
    {
        drawBubble(graphics,
                   static_cast<int16_t>(BOARD_X + pivotX * CELL_SIZE + CELL_SIZE / 2),
                   static_cast<int16_t>(BOARD_Y + pivotY * CELL_SIZE + CELL_SIZE / 2),
                   pivotColor, false);
    }
    if (childY >= 0)
    {
        drawBubble(graphics,
                   static_cast<int16_t>(BOARD_X + childX * CELL_SIZE + CELL_SIZE / 2),
                   static_cast<int16_t>(BOARD_Y + childY * CELL_SIZE + CELL_SIZE / 2),
                   childColor, false);
    }
}

void AquaDrop::drawSidePanel(Graphics& graphics, uint32_t now) const
{
    const int16_t panelX = 132;
    const Graphics::Color panel = Graphics::rgb565(7, 56, 80);
    const Graphics::Color accent = Graphics::rgb565(130, 238, 221);
    const Graphics::Color pale = Graphics::rgb565(220, 249, 241);

    graphics.fillRoundRect(panelX, 18, 176, 204, 10, panel);
    graphics.drawRoundRect(panelX, 18, 176, 204, 10, 2, accent);

    char text[40];
    snprintf(text, sizeof(text), "STAGE %u", static_cast<unsigned>(stageIndex + 1));
    graphics.drawString(text, panelX + 14, 32, pale, Graphics::SIZE_22B);

    graphics.drawString("NEXT", panelX + 14, 65, accent, Graphics::SIZE_18);
    drawBubble(graphics, panelX + 104, 73, nextPivotColor, false);
    drawBubble(graphics, panelX + 126, 73, nextChildColor, false);

    snprintf(text, sizeof(text), "SCORE %ld", static_cast<long>(score));
    graphics.drawString(text, panelX + 14, 96, Graphics::WHITE, Graphics::SIZE_18);

    snprintf(text, sizeof(text), "HI %ld", static_cast<long>(highScore));
    graphics.drawString(text, panelX + 14, 120, pale, Graphics::SIZE_18);

    const uint16_t target = STAGE_TARGET[stageIndex];
    const uint16_t remaining = stageClearedCount >= target ? 0 : static_cast<uint16_t>(target - stageClearedCount);
    snprintf(text, sizeof(text), "TARGET %u", static_cast<unsigned>(remaining));
    graphics.drawString(text, panelX + 14, 151, accent, Graphics::SIZE_18);

    graphics.fillRoundRect(panelX + 14, 179, 144, 12, 6, Graphics::rgb565(13, 78, 99));
    const uint16_t progressW = static_cast<uint16_t>((static_cast<uint32_t>(stageClearedCount > target ? target : stageClearedCount) * 144u) / target);
    if (progressW > 0)
    {
        graphics.fillRoundRect(panelX + 14, 179, progressW, 12, 6, Graphics::rgb565(95, 226, 190));
    }

    if (displayedChain >= 2 && !Platform::elapsed(now, chainDisplayStartMsec, CHAIN_DISPLAY_MSEC))
    {
        snprintf(text, sizeof(text), "%u CHAIN!", static_cast<unsigned>(displayedChain));
        graphics.drawString(text, panelX + 86, 208, Graphics::YELLOW, Graphics::SIZE_22B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }
}

void AquaDrop::drawOverlay(Graphics& graphics, uint32_t now) const
{
    const Graphics::Color overlay = Graphics::rgb565(5, 36, 55);
    const Graphics::Color accent = Graphics::rgb565(130, 238, 221);

    const char* title = nullptr;
    const char* subtitle = nullptr;

    switch (mode)
    {
    case MODE_STAGE_INTRO:
        title = "READY";
        subtitle = "CONNECT 4 BUBBLES";
        break;
    case MODE_PAUSED:
        title = "PAUSED";
        subtitle = "START: RESUME  SELECT: TITLE";
        break;
    case MODE_STAGE_CLEAR:
        title = "STAGE CLEAR";
        subtitle = "THE CURRENT IS CALM";
        break;
    case MODE_GAME_OVER:
        title = "GAME OVER";
        subtitle = "THE REEF IS FULL";
        break;
    case MODE_ALL_CLEAR:
        title = "ALL CLEAR";
        subtitle = "THE OCEAN SPARKLES";
        break;
    default:
        break;
    }

    if (title == nullptr)
    {
        return;
    }

    const int16_t panelW = 270;
    const int16_t panelH = 82;
    const int16_t panelX = (SCREEN_W - panelW) / 2;
    const int16_t panelY = 78;
    graphics.fillRoundRect(panelX, panelY, panelW, panelH, 12, overlay);
    graphics.drawRoundRect(panelX, panelY, panelW, panelH, 12, 2, accent);
    graphics.drawString(title, SCREEN_W / 2, panelY + 27, Graphics::WHITE, Graphics::SIZE_32B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    graphics.drawString(subtitle, SCREEN_W / 2, panelY + 61, accent, Graphics::SIZE_18, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    if (mode == MODE_STAGE_CLEAR || mode == MODE_ALL_CLEAR)
    {
        for (int16_t i = 0; i < 8; i++)
        {
            const uint32_t elapsed = static_cast<uint32_t>(now - modeStartMsec);
            const int16_t bx = static_cast<int16_t>(24 + i * 39);
            const int16_t by = static_cast<int16_t>(218 - ((elapsed / 8u + i * 27u) % 170u));
            graphics.drawCircle(bx, by, static_cast<uint16_t>(2 + i % 3), Graphics::WHITE);
        }
    }
}

Graphics::Color AquaDrop::bubbleColor(uint8_t colorIndex) const
{
    switch (colorIndex)
    {
    case 1: return Graphics::rgb565(243, 104, 112);
    case 2: return Graphics::rgb565(250, 201, 82);
    case 3: return Graphics::rgb565(91, 210, 174);
    case 4: return Graphics::rgb565(103, 167, 238);
    case 5: return Graphics::rgb565(190, 121, 221);
    default: return Graphics::LIGHTGRAY;
    }
}

#include "NeonReversi.h"
#include <cstring>

namespace PRUZEA
{

static Graphics::Color PLAYER_COLOR = Graphics::rgb565(255, 0, 255);
static Graphics::Color CPU_COLOR = Graphics::CYAN;

NeonReversi::NeonReversi()
    : m_mode(MODE_TITLE)
    , m_nextModeAfterPass(MODE_PLAYING)
    , m_cursorX(3)
    , m_cursorY(3)
    , m_playerScore(2)
    , m_cpuScore(2)
    , m_highScore(0)
    , m_playerTurn(true)
    , m_stateStartTime(0)
    , m_lastFlipTime(0)
    , m_titleAnimTime(0)
    , m_shakeIntensity(0)
    , m_flipTargetCount(0)
    , m_flipCurrentIndex(0)
{
    memset(m_board, 0, sizeof(m_board));
    memset(m_validMoves, 0, sizeof(m_validMoves));
    memset(m_particles, 0, sizeof(m_particles));
    memset(m_shockwaves, 0, sizeof(m_shockwaves));
}

void NeonReversi::onInit(Storage& storage)
{
    loadHighScore(storage);
    resetGame();
    m_mode = MODE_TITLE;
}

void NeonReversi::resetGame()
{
    // 盤面のクリア
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            m_board[y][x] = CELL_EMPTY;
        }
    }

    // 初期配置 (中央の4枚)
    m_board[3][3] = CELL_CPU;
    m_board[3][4] = CELL_PLAYER;
    m_board[4][3] = CELL_PLAYER;
    m_board[4][4] = CELL_CPU;

    m_playerScore = 2;
    m_cpuScore = 2;
    
    m_playerTurn = Math::chance(0.5f);

    m_cursorX = 3;
    m_cursorY = 3;
    m_flipTargetCount = 0;
    m_flipCurrentIndex = 0;
    m_shakeIntensity = 0;

    memset(m_particles, 0, sizeof(m_particles));
    memset(m_shockwaves, 0, sizeof(m_shockwaves));

    updateValidMoves();
}

void NeonReversi::updateValidMoves()
{
    CellState currentTurnColor = m_playerTurn ? CELL_PLAYER : CELL_CPU;
    
    m_playerScore = 0;
    m_cpuScore = 0;

    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            if (m_board[y][x] == CELL_PLAYER) m_playerScore++;
            else if (m_board[y][x] == CELL_CPU) m_cpuScore++;

            m_validMoves[y][x] = isValidMove(x, y, currentTurnColor);
        }
    }
}

bool NeonReversi::isValidMove(int8_t startX, int8_t startY, CellState color) const
{
    if (m_board[startY][startX] != CELL_EMPTY) return false;

    CellState opponent = (color == CELL_PLAYER) ? CELL_CPU : CELL_PLAYER;

    static const int8_t dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    static const int8_t dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };

    for (int i = 0; i < 8; ++i) {
        int8_t x = startX + dx[i];
        int8_t y = startY + dy[i];
        bool hasOpponentBetween = false;

        while (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
            if (m_board[y][x] == opponent) {
                hasOpponentBetween = true;
            } else if (m_board[y][x] == color) {
                if (hasOpponentBetween) return true;
                break;
            } else {
                break;
            }
            x += dx[i];
            y += dy[i];
        }
    }
    return false;
}

void NeonReversi::getFlipCells(int8_t startX, int8_t startY, CellState color)
{
    m_flipTargetCount = 0;
    CellState opponent = (color == CELL_PLAYER) ? CELL_CPU : CELL_PLAYER;

    static const int8_t dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    static const int8_t dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };

    for (int i = 0; i < 8; ++i) {
        int8_t x = startX + dx[i];
        int8_t y = startY + dy[i];
        
        struct { int8_t x, y; } tempTargets[BOARD_SIZE];
        uint8_t tempCount = 0;

        while (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
            if (m_board[y][x] == opponent) {
                tempTargets[tempCount].x = x;
                tempTargets[tempCount].y = y;
                tempCount++;
            } else if (m_board[y][x] == color) {
                for (uint8_t t = 0; t < tempCount; ++t) {
                    if (m_flipTargetCount < 64) {
                        m_flipTargets[m_flipTargetCount].x = tempTargets[t].x;
                        m_flipTargets[m_flipTargetCount].y = tempTargets[t].y;
                        m_flipTargetCount++;
                    }
                }
                break;
            } else {
                break;
            }
            x += dx[i];
            y += dy[i];
        }
    }
}

bool NeonReversi::hasAnyValidMove(CellState color)
{
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            if (isValidMove(x, y, color)) return true;
        }
    }
    return false;
}

void NeonReversi::executeMove(int8_t x, int8_t y, CellState color)
{
    m_board[y][x] = color;
    getFlipCells(x, y, color);

    m_flipCurrentIndex = 0;
    m_mode = MODE_FLIPPING;
    m_lastFlipTime = Platform::getMsec();
}

void NeonReversi::triggerPassSequence(InternalMode nextMode)
{
    m_mode = MODE_PASS;
    m_nextModeAfterPass = nextMode;
    m_stateStartTime = Platform::getMsec();
}

void NeonReversi::checkTurnTransitions()
{
    m_playerTurn = !m_playerTurn;
    updateValidMoves();

    CellState nextColor = m_playerTurn ? CELL_PLAYER : CELL_CPU;

    if (!hasAnyValidMove(nextColor)) {
        m_playerTurn = !m_playerTurn;
        updateValidMoves();

        CellState currentColor = m_playerTurn ? CELL_PLAYER : CELL_CPU;
        if (!hasAnyValidMove(currentColor)) {
            m_mode = MODE_GAME_OVER;
            m_stateStartTime = Platform::getMsec();
        } else {
            InternalMode nextMode = m_playerTurn ? MODE_PLAYING : MODE_CPU_THINKING;
            triggerPassSequence(nextMode);
        }
    } else {
        if (!m_playerTurn) {
            m_mode = MODE_CPU_THINKING;
            m_stateStartTime = Platform::getMsec();
        } else {
            m_mode = MODE_PLAYING;
        }
    }
}

void NeonReversi::doCPUMove()
{
    int8_t bestX = -1;
    int8_t bestY = -1;
    int16_t bestScore = -9999;

    static const int16_t weights[BOARD_SIZE][BOARD_SIZE] = {
        {  120, -20,  20,   5,   5,  20, -20,  120 },
        {  -20, -40,  -5,  -5,  -5,  -5, -40,  -20 },
        {   20,  -5,  15,   3,   3,  15,  -5,   20 },
        {    5,  -5,   3,   3,   3,   3,  -5,    5 },
        {    5,  -5,   3,   3,   3,   3,  -5,    5 },
        {   20,  -5,  15,   3,   3,  15,  -5,   20 },
        {  -20, -40,  -5,  -5,  -5,  -5, -40,  -20 },
        {  120, -20,  20,   5,   5,  20, -20,  120 }
    };

    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            if (m_validMoves[y][x]) {
                int16_t currentScore = weights[y][x] + Math::random(0, 5);
                if (currentScore > bestScore) {
                    bestScore = currentScore;
                    bestX = x;
                    bestY = y;
                }
            }
        }
    }

    if (bestX != -1 && bestY != -1) {
        executeMove(bestX, bestY, CELL_CPU);
    } else {
        checkTurnTransitions();
    }
}

void NeonReversi::spawnParticles(int16_t x, int16_t y, Graphics::Color color, uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i) {
        for (uint8_t p = 0; p < MAX_PARTICLES; ++p) {
            if (m_particles[p].life == 0) {
                m_particles[p].x = static_cast<float>(x);
                m_particles[p].y = static_cast<float>(y);
                
                float angle = Math::randomFloat(0.0f, Math::TWO_PI);
                float speed = Math::randomFloat(1.0f, 5.0f);
                
                m_particles[p].vx = Math::cos(angle) * speed;
                m_particles[p].vy = Math::sin(angle) * speed;
                m_particles[p].color = color;
                m_particles[p].life = static_cast<uint8_t>(Math::random(10, 25));
                break;
            }
        }
    }
}

void NeonReversi::spawnShockwave(int16_t x, int16_t y, Graphics::Color color)
{
    for (uint8_t i = 0; i < MAX_SHOCKWAVES; ++i) {
        if (m_shockwaves[i].life == 0) {
            m_shockwaves[i].x = x;
            m_shockwaves[i].y = y;
            m_shockwaves[i].radius = 2;
            m_shockwaves[i].maxRadius = 50;
            m_shockwaves[i].color = color;
            m_shockwaves[i].life = 1;
            break;
        }
    }
}

void NeonReversi::updateEffects()
{
    if (m_shakeIntensity > 0) m_shakeIntensity--;

    for (uint8_t i = 0; i < MAX_PARTICLES; ++i) {
        if (m_particles[i].life > 0) {
            m_particles[i].x += m_particles[i].vx;
            m_particles[i].y += m_particles[i].vy;
            m_particles[i].life--;
        }
    }

    for (uint8_t i = 0; i < MAX_SHOCKWAVES; ++i) {
        if (m_shockwaves[i].life > 0) {
            m_shockwaves[i].radius += 3;
            if (m_shockwaves[i].radius >= m_shockwaves[i].maxRadius) {
                m_shockwaves[i].life = 0;
            }
        }
    }
}

Game::GameState NeonReversi::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    uint32_t now = Platform::getMsec();
    updateEffects();
    bool hasInput = false;

    // ------------------------------------------
    // MODE: TITLE
    // ------------------------------------------
    if (m_mode == MODE_TITLE) {
        m_titleAnimTime++;
        if (input.justPressed(Input::Button::START)) {
            audio.playSE(&Audio::SE::NO_8, 1.0f);
            resetGame();
            
            m_mode = m_playerTurn ? MODE_PLAYING : MODE_CPU_THINKING;
            m_stateStartTime = now;
        }
        dirty = true;
        return GameState::RUNNING;
    }

    if (m_mode == MODE_PLAYING) {
        if (m_playerTurn) {
            if (input.repeat(Input::Button::UP))    { m_cursorY = (m_cursorY - 1 + 8) % 8; audio.playSE(&Audio::SE::NO_1, 0.4f); hasInput = true; }
            if (input.repeat(Input::Button::DOWN))  { m_cursorY = (m_cursorY + 1) % 8;     audio.playSE(&Audio::SE::NO_1, 0.4f); hasInput = true; }
            if (input.repeat(Input::Button::LEFT))   { m_cursorX = (m_cursorX - 1 + 8) % 8; audio.playSE(&Audio::SE::NO_1, 0.4f); hasInput = true; }
            if (input.repeat(Input::Button::RIGHT))  { m_cursorX = (m_cursorX + 1) % 8;     audio.playSE(&Audio::SE::NO_1, 0.4f); hasInput = true; }

            if (input.justPressed(Input::Button::A)) {
                if (m_validMoves[m_cursorY][m_cursorX]) {
                    audio.playSE(&Audio::SE::NO_3, 0.8f);
                    int16_t screenX = BOARD_X + m_cursorX * CELL_SIZE + CELL_SIZE / 2;
                    int16_t screenY = BOARD_Y + m_cursorY * CELL_SIZE + CELL_SIZE / 2;
                    spawnShockwave(screenX, screenY, PLAYER_COLOR);
                    
                    executeMove(m_cursorX, m_cursorY, CELL_PLAYER);
                } else {
                    audio.playSE(&Audio::SE::NO_2, 0.6f);
                }
                hasInput = true;
            }
        }
    }

    else if (m_mode == MODE_PASS) {
        if (Platform::elapsed(now, m_stateStartTime, PASS_DISPLAY_TIME_MS)) {
            m_mode = m_nextModeAfterPass;
            if (m_mode == MODE_CPU_THINKING) {
                m_stateStartTime = now;
            }
        }
    }

    else if (m_mode == MODE_CPU_THINKING) {
        if (Platform::elapsed(now, m_stateStartTime, CPU_THINK_TIME_MS)) {
            doCPUMove();
        }
    }

    else if (m_mode == MODE_FLIPPING) {
        if (m_flipCurrentIndex < m_flipTargetCount) {
            if (Platform::elapsed(now, m_lastFlipTime, FLIP_DELAY_MS)) {
                int8_t fx = m_flipTargets[m_flipCurrentIndex].x;
                int8_t fy = m_flipTargets[m_flipCurrentIndex].y;
                
                m_board[fy][fx] = m_playerTurn ? CELL_PLAYER : CELL_CPU;
                
                audio.playSE(&Audio::SE::NO_5, 0.5f);
                int16_t pX = BOARD_X + fx * CELL_SIZE + CELL_SIZE / 2;
                int16_t pY = BOARD_Y + fy * CELL_SIZE + CELL_SIZE / 2;
                Graphics::Color pColor = m_playerTurn ? PLAYER_COLOR : CPU_COLOR;
                spawnParticles(pX, pY, pColor, 6);
                
                m_shakeIntensity = 3;
                
                m_flipCurrentIndex++;
                m_lastFlipTime = now;
            }
        } else {
            checkTurnTransitions();
        }
    }

    else if (m_mode == MODE_GAME_OVER) {
        int16_t finalScore = m_playerScore;
        if (finalScore > m_highScore) {
            m_highScore = finalScore;
        }

        if (input.justPressed(Input::Button::START) || input.justPressed(Input::Button::A)) {
            audio.playSE(&Audio::SE::NO_1, 0.8f);
            m_mode = MODE_TITLE;
            hasInput = true;
        }
    }

    bool isEffectActive = false;

    if (m_shakeIntensity > 0) {
        isEffectActive = true;
    }

    for (uint8_t i = 0; i < MAX_PARTICLES; ++i) {
        if (m_particles[i].life > 0) {
            isEffectActive = true;
            break;
        }
    }
    for (uint8_t i = 0; i < MAX_SHOCKWAVES; ++i) {
        if (m_shockwaves[i].life > 0) {
            isEffectActive = true;
            break;
        }
    }
    bool isStateChanging = (m_mode == MODE_CPU_THINKING) ||
                           (m_mode == MODE_FLIPPING)     ||
                           (m_mode == MODE_PASS)         ||
                           (m_mode == MODE_GAME_OVER &&
                            !Platform::elapsed(now, m_stateStartTime, GAME_OVER_TWEEN_MS));

    if (hasInput || isEffectActive || isStateChanging) {
        dirty = true;
    }

    return GameState::RUNNING;
}

bool NeonReversi::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty) {
        return false;
    }

    if (m_shakeIntensity > 0) {
        const int16_t sx = static_cast<int16_t>(Math::random(-m_shakeIntensity, m_shakeIntensity + 1));
        const int16_t sy = static_cast<int16_t>(Math::random(-m_shakeIntensity, m_shakeIntensity + 1));
        graphics.setViewport(sx, sy);
    } else {
        graphics.resetViewport();
    }

    graphics.fillScreen(Graphics::BLACK);

    if (m_mode == MODE_TITLE) {
        graphics.drawString("NEON", 160, 50, Graphics::CYAN, Graphics::Font::SIZE_42B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        graphics.drawString("REVERSI", 160, 95, PLAYER_COLOR, Graphics::Font::SIZE_42B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        
        graphics.drawRect(50, 130, 220, 4, Graphics::rgb565(40, 40, 60));

        if ((m_titleAnimTime % 30) < 18) {
            graphics.drawString("PRESS START BUTTON", 160, 170, Graphics::Color::WHITE, Graphics::Font::SIZE_18, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }
        
        char hsBuf[32];
        snprintf(hsBuf, sizeof(hsBuf), "HI-SCORE: %02d", m_highScore);
        graphics.drawString(hsBuf, 160, 210, Graphics::YELLOW, Graphics::Font::SIZE_13, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    } else {
        drawBoard(graphics);
        drawUI(graphics);
        drawEffects(graphics);

        if (m_mode == MODE_PASS) {
            graphics.fillRectAlpha(0, 0, 320, 240, 80, Graphics::BLACK);
            graphics.fillRect(50, 100, 140, 40, Graphics::rgb565(20, 10, 10));
            graphics.drawRect(50, 100, 140, 40, Graphics::Color::WHITE);
            graphics.drawString("PASS", 120, 120, Graphics::Color::WHITE, Graphics::Font::SIZE_32B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }

        if (m_mode == MODE_GAME_OVER) {
            const uint32_t now = Platform::getMsec();
            const float t = Math::clamp(
                static_cast<float>(now - m_stateStartTime) / static_cast<float>(GAME_OVER_TWEEN_MS),
                0.0f, 1.0f);
            const float eased = Tween::apply(t, Tween::Ease::EASE_OUT_BACK);
            const int16_t panelY = static_cast<int16_t>(Tween::lerp(-80.0f, 80.0f, eased));

            graphics.fillRectAlpha(0, 0, 320, 240, 96, Graphics::BLACK);
            graphics.fillRect(20, panelY, 280, 80, Graphics::rgb565(10, 10, 20));
            graphics.drawRect(20, panelY, 280, 80, Graphics::YELLOW);

            char hsBuf[32];
            snprintf(hsBuf, sizeof(hsBuf), "%02d", m_playerScore);
            graphics.drawString(hsBuf, 40, panelY + 40, PLAYER_COLOR, Graphics::Font::SIZE_32B, Graphics::HorizontalAlign::LEFT, Graphics::VerticalAlign::MIDDLE);
            snprintf(hsBuf, sizeof(hsBuf), "%02d", m_cpuScore);
            graphics.drawString(hsBuf, 280, panelY + 40, CPU_COLOR, Graphics::Font::SIZE_32B, Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::MIDDLE);

            if (m_playerScore > m_cpuScore) {
                graphics.drawString("YOU WIN", 160, panelY + 25, Graphics::YELLOW, Graphics::Font::SIZE_25B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
            } else if (m_playerScore < m_cpuScore) {
                graphics.drawString("YOU LOSE", 160, panelY + 25, Graphics::BLUE, Graphics::Font::SIZE_25B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
            } else {
                graphics.drawString("DRAW GAME", 160, panelY + 25, Graphics::Color::WHITE, Graphics::Font::SIZE_25B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
            }
            graphics.drawString("A / START to TITLE", 160, panelY + 60, Graphics::Color::WHITE, Graphics::Font::SIZE_13, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }
    }

    dirty = false;
    return true;
}

void NeonReversi::drawBoard(Graphics& graphics)
{
    graphics.drawRect(BOARD_X - 2, BOARD_Y - 2, BOARD_SIZE * CELL_SIZE + 4, BOARD_SIZE * CELL_SIZE + 4, Graphics::rgb565(30, 40, 80));

    for (int i = 0; i <= BOARD_SIZE; ++i) {
        int coord = i * CELL_SIZE;
        graphics.drawLine(BOARD_X + coord, BOARD_Y, BOARD_X + coord, BOARD_Y + BOARD_SIZE * CELL_SIZE, Graphics::rgb565(20, 30, 60));
        graphics.drawLine(BOARD_X, BOARD_Y + coord, BOARD_X + BOARD_SIZE * CELL_SIZE, BOARD_Y + coord, Graphics::rgb565(20, 30, 60));
    }

    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            int16_t cx = BOARD_X + x * CELL_SIZE + CELL_SIZE / 2;
            int16_t cy = BOARD_Y + y * CELL_SIZE + CELL_SIZE / 2;

            if (m_board[y][x] == CELL_PLAYER) {
                graphics.fillCircle(cx, cy, 10, Graphics::rgb565(60, 0, 0));
                graphics.drawCircle(cx, cy, 10, PLAYER_COLOR);
                graphics.drawCircle(cx, cy, 7, Graphics::Color::WHITE);
            } else if (m_board[y][x] == CELL_CPU) {
                graphics.fillCircle(cx, cy, 10, Graphics::rgb565(0, 40, 70));
                graphics.drawCircle(cx, cy, 10, CPU_COLOR);
                graphics.drawCircle(cx, cy, 7, Graphics::Color::WHITE);
            } else {
                if (m_mode == MODE_PLAYING && m_playerTurn && m_validMoves[y][x]) {
                    graphics.fillCircle(cx, cy, 3, Graphics::rgb565(100, 30, 30));
                }
            }
        }
    }

    if (m_mode == MODE_PLAYING && m_playerTurn) {
        int16_t curX = BOARD_X + m_cursorX * CELL_SIZE;
        int16_t curY = BOARD_Y + m_cursorY * CELL_SIZE;
        graphics.drawRect(curX, curY, CELL_SIZE, CELL_SIZE, Graphics::YELLOW);
        graphics.drawRect(curX + 1, curY + 1, CELL_SIZE - 2, CELL_SIZE - 2, Graphics::YELLOW);
    }
}

void NeonReversi::drawUI(Graphics& graphics)
{
    int16_t uiX = BOARD_X + BOARD_SIZE * CELL_SIZE + 15;

    if (m_mode == MODE_PLAYING) {
        if (m_playerTurn) {
            graphics.drawString("YOUR", uiX, 15, PLAYER_COLOR, Graphics::Font::SIZE_18);
            graphics.drawString("TURN", uiX, 33, PLAYER_COLOR, Graphics::Font::SIZE_18);
        } else {
            graphics.drawString("CPU", uiX, 15, CPU_COLOR, Graphics::Font::SIZE_18);
            graphics.drawString("TURN", uiX, 33, CPU_COLOR, Graphics::Font::SIZE_18);
        }
    } else if (m_mode == MODE_CPU_THINKING) {
        graphics.drawString("CPU", uiX, 15, CPU_COLOR, Graphics::Font::SIZE_18);
        graphics.drawString("THINK..", uiX, 33, CPU_COLOR, Graphics::Font::SIZE_18);
    } else if (m_mode == MODE_FLIPPING) {
        graphics.drawString("SYNC..", uiX, 15, Graphics::YELLOW, Graphics::Font::SIZE_18);
    } else if (m_mode == MODE_PASS) {
        graphics.drawString("PASS", uiX, 15, Graphics::MAGENTA, Graphics::Font::SIZE_18);
    }

    graphics.drawRect(uiX, 60, 95, 140, Graphics::rgb565(40, 40, 50));

    graphics.drawString("YOU", uiX + 8, 72, PLAYER_COLOR, Graphics::Font::SIZE_18);
    char pBuf[16];
    snprintf(pBuf, sizeof(pBuf), "%02d", m_playerScore);
    graphics.drawString(pBuf, uiX + 12, 94, Graphics::Color::WHITE, Graphics::Font::SIZE_25B);

    graphics.drawLine(uiX + 5, 128, uiX + 90, 128, Graphics::rgb565(40, 40, 50));

    graphics.drawString("CPU", uiX + 8, 142, Graphics::CYAN, Graphics::Font::SIZE_18);
    char cBuf[16];
    snprintf(cBuf, sizeof(cBuf), "%02d", m_cpuScore);
    graphics.drawString(cBuf, uiX + 12, 164, Graphics::Color::WHITE, Graphics::Font::SIZE_25B);
}

void NeonReversi::drawEffects(Graphics& graphics)
{
    for (uint8_t i = 0; i < MAX_SHOCKWAVES; ++i) {
        if (m_shockwaves[i].life > 0) {
            graphics.drawCircle(m_shockwaves[i].x, m_shockwaves[i].y, m_shockwaves[i].radius, m_shockwaves[i].color);
            if (m_shockwaves[i].radius > 4) {
                graphics.drawCircle(m_shockwaves[i].x, m_shockwaves[i].y, m_shockwaves[i].radius - 2, m_shockwaves[i].color);
            }
        }
    }

    for (uint8_t i = 0; i < MAX_PARTICLES; ++i) {
        if (m_particles[i].life > 0) {
            int16_t px = static_cast<int16_t>(m_particles[i].x);
            int16_t py = static_cast<int16_t>(m_particles[i].y);
            graphics.drawPixel(px, py, m_particles[i].color);
            graphics.drawPixel(px + 1, py, m_particles[i].color);
            graphics.drawPixel(px - 1, py, m_particles[i].color);
            graphics.drawPixel(px, py + 1, m_particles[i].color);
            graphics.drawPixel(px, py - 1, m_particles[i].color);
        }
    }
}

void NeonReversi::loadHighScore(Storage& storage)
{
    SaveData sd;
    sd.load(storage, getId(), "score.dat");
    m_highScore = sd.getInt32("high_score", 0);

}

void NeonReversi::saveHighScore(Storage& storage)
{
    SaveData sd;
    sd.load(storage, getId(), "score.dat");
    sd.setInt32("high_score", m_highScore);
    sd.save(storage, getId(), "score.dat");
}

void NeonReversi::onTerminate(Storage& storage)
{
    saveHighScore(storage);
}

} // namespace PRUZEA

#pragma once
#include "PRUZEA.h"

namespace PRUZEA
{

class NeonReversi : public Game
{
private:
    enum InternalMode : uint8_t {
        MODE_TITLE,
        MODE_PLAYING,
        MODE_PASS,
        MODE_CPU_THINKING,
        MODE_FLIPPING,
        MODE_GAME_OVER
    };

    enum CellState : int8_t {
        CELL_EMPTY = 0,
        CELL_PLAYER = 1,
        CELL_CPU = -1
    };

    struct Particle {
        float x, y;
        float vx, vy;
        Graphics::Color color;
        uint8_t life;
    };

    struct Shockwave {
        int16_t x, y;
        uint16_t radius;
        uint16_t maxRadius;
        Graphics::Color color;
        uint8_t life;
    };

    static constexpr int BOARD_SIZE = 8;
    static constexpr int CELL_SIZE = 26;
    static constexpr int BOARD_X = 15;
    static constexpr int BOARD_Y = 15;
    
    static constexpr uint8_t MAX_PARTICLES = 64;
    static constexpr uint8_t MAX_SHOCKWAVES = 4;
    static constexpr uint32_t CPU_THINK_TIME_MS = 600;
    static constexpr uint32_t FLIP_DELAY_MS = 150;
    static constexpr uint32_t PASS_DISPLAY_TIME_MS = 800;
    static constexpr uint32_t GAME_OVER_TWEEN_MS = 250;

    InternalMode m_mode;
    InternalMode m_nextModeAfterPass;
    CellState m_board[BOARD_SIZE][BOARD_SIZE];
    bool m_validMoves[BOARD_SIZE][BOARD_SIZE];
    
    int8_t m_cursorX;
    int8_t m_cursorY;
    
    int16_t m_playerScore;
    int16_t m_cpuScore;
    int16_t m_highScore;
    bool m_playerTurn;
    
    uint32_t m_stateStartTime;
    uint32_t m_lastFlipTime;
    uint32_t m_titleAnimTime;
    int8_t m_shakeIntensity;

    struct FlipTarget {
        int8_t x, y;
    } m_flipTargets[64];
    uint8_t m_flipTargetCount;
    uint8_t m_flipCurrentIndex;

    Particle m_particles[MAX_PARTICLES];
    Shockwave m_shockwaves[MAX_SHOCKWAVES];

    void resetGame();
    void updateValidMoves();
    bool isValidMove(int8_t startX, int8_t startY, CellState color) const;
    void getFlipCells(int8_t startX, int8_t startY, CellState color);
    bool hasAnyValidMove(CellState color);
    void executeMove(int8_t x, int8_t y, CellState color);
    void checkTurnTransitions();
    void triggerPassSequence(InternalMode nextMode);
    
    void doCPUMove();

    void spawnParticles(int16_t x, int16_t y, Graphics::Color color, uint8_t count);
    void spawnShockwave(int16_t x, int16_t y, Graphics::Color color);
    void updateEffects();
    
    void loadHighScore(Storage& storage);
    void saveHighScore(Storage& storage);

    void drawBoard(Graphics& graphics);
    void drawUI(Graphics& graphics);
    void drawEffects(Graphics& graphics);

public:
    NeonReversi();
    virtual ~NeonReversi() override = default;

    virtual const char* getId() const override { return "neonreversi"; }
    virtual const char* getName() const override { return "NEON REVERSI"; }
    virtual const char* getMenuName() const override { return "NEON REVERSI"; }

    virtual uint16_t getLogicalScreenWidth() const override { return 320; }
    virtual uint16_t getLogicalScreenHeight() const override { return 240; }
    virtual uint16_t getTargetScreenWidth() const override { return 320; }
    virtual uint16_t getTargetScreenHeight() const override { return 240; }

    void onInit(Storage& storage) override;
    GameState onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override;
    bool onDraw(Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(Storage& storage) override;
};

} // namespace PRUZEA

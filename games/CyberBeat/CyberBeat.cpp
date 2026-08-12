// CyberBeat.cpp
#include "CyberBeat.h"
#include <cstdio>

namespace PRUZEA
{

// =========================================================================
// # Sound Tracks Data (RODATA - Flash Memory)
// =========================================================================
static const Audio::ToneNote GAME_BGM_NOTES[] = {
    {Audio::ToneNote::A4, Audio::ToneNote::E}, {Audio::ToneNote::A4, Audio::ToneNote::E}, {Audio::ToneNote::C5, Audio::ToneNote::Q},
    {Audio::ToneNote::A4, Audio::ToneNote::E}, {Audio::ToneNote::A4, Audio::ToneNote::E}, {Audio::ToneNote::D5, Audio::ToneNote::Q},
    {Audio::ToneNote::A4, Audio::ToneNote::E}, {Audio::ToneNote::A4, Audio::ToneNote::E}, {Audio::ToneNote::E5, Audio::ToneNote::Q},
    {Audio::ToneNote::D5, Audio::ToneNote::E}, {Audio::ToneNote::C5, Audio::ToneNote::E}, {Audio::ToneNote::A4, Audio::ToneNote::Q},
    {Audio::ToneNote::G4, Audio::ToneNote::E}, {Audio::ToneNote::G4, Audio::ToneNote::E}, {Audio::ToneNote::B4, Audio::ToneNote::Q},
    {Audio::ToneNote::G4, Audio::ToneNote::E}, {Audio::ToneNote::G4, Audio::ToneNote::E}, {Audio::ToneNote::C5, Audio::ToneNote::Q},
    {Audio::ToneNote::G4, Audio::ToneNote::E}, {Audio::ToneNote::G4, Audio::ToneNote::E}, {Audio::ToneNote::D5, Audio::ToneNote::Q},
    {Audio::ToneNote::C5, Audio::ToneNote::E}, {Audio::ToneNote::B4, Audio::ToneNote::E}, {Audio::ToneNote::G4, Audio::ToneNote::Q}
};
static const Audio::Music GAME_BGM = {
    GAME_BGM_NOTES,
    200,
    sizeof(GAME_BGM_NOTES) / sizeof(Audio::ToneNote),
    0, // Infinite loop
    0.6f
};

// =========================================================================
// # Core Game Logic Methods
// =========================================================================
void CyberBeat::onInit(Storage& storage)
{
    m_mode = MODE_TITLE;
    m_score = 0;
    m_highScore = 0;
    m_combo = 0;
    m_maxCombo = 0;
    m_life = MAX_LIFE;
    m_lastPerfectTime = 0;
    m_lastMissTime = 0;
    m_judgeText = "";
    m_judgeTextTime = 0;

    for (uint16_t i = 0; i < MAX_NOTES; ++i) {
        m_notes[i].active = false;
    }

    SaveData sd;
    if (sd.load(storage, getId(), "highscore.dat"))
    {
        m_highScore = sd.getUInt32("highscore");
    }
}

void CyberBeat::resetGame(uint32_t currentTime)
{
    m_score = 0;
    m_combo = 0;
    m_maxCombo = 0;
    m_life = MAX_LIFE;
    m_startTime = currentTime;
    m_lastSpawnTime = currentTime;
    m_lastTime = currentTime;
    m_lastPerfectTime = 0;
    m_lastMissTime = 0;
    m_judgeText = "";
    m_judgeTextTime = 0;

    for (uint16_t i = 0; i < MAX_NOTES; ++i) {
        m_notes[i].active = false;
    }
}

void CyberBeat::spawnNote(uint32_t currentTime)
{
    for (uint16_t i = 0; i < MAX_NOTES; ++i) {
        if (!m_notes[i].active) {
            m_notes[i].active = true;
            m_notes[i].targetTime = currentTime + NOTE_TRAVEL_TIME;
            m_notes[i].lane = Math::random(4); // 4 tactical lanes
            break;
        }
    }
}

void CyberBeat::checkInput(Input& input, Audio& audio, uint32_t currentTime)
{
    // Ergonomic layout mapping: [LEFT] [RIGHT] [Y] [A]
    bool lanePressed[4] = {
        input.justPressed(Input::Button::LEFT),
        input.justPressed(Input::Button::RIGHT),
        input.justPressed(Input::Button::Y),
        input.justPressed(Input::Button::A)
    };

    for (int l = 0; l < 4; ++l) {
        if (!lanePressed[l]) continue;

        int16_t closestIdx = -1;
        uint32_t minDiff = 0xFFFFFFFF;

        // Find the nearest active note in the pressed lane
        for (uint16_t i = 0; i < MAX_NOTES; ++i) {
            if (m_notes[i].active && m_notes[i].lane == l) {
                uint32_t diff = (currentTime > m_notes[i].targetTime) ? 
                                (currentTime - m_notes[i].targetTime) : 
                                (m_notes[i].targetTime - currentTime);
                if (diff < minDiff) {
                    minDiff = diff;
                    closestIdx = i;
                }
            }
        }

        // Evaluate timing judgment windows
        if (closestIdx != -1 && minDiff <= JUDGE_GOOD) {
            m_notes[closestIdx].active = false;

            if (minDiff <= JUDGE_PERFECT) {
                m_score += 150 + (m_combo * 2);
                m_combo++;
                m_judgeText = "PERFECT!";
                m_lastPerfectTime = currentTime; // Trigger juice screen shake effect
                audio.playSE(&Audio::SE::NO_9, 0.7f); // Sparkling hit SFX
            } else if (minDiff <= JUDGE_GREAT) {
                m_score += 100 + m_combo;
                m_combo++;
                m_judgeText = "GREAT";
                audio.playSE(&Audio::SE::NO_1, 0.6f); // Laser style hit SFX
            } else {
                m_score += 50;
                m_combo++;
                m_judgeText = "GOOD";
                audio.playSE(&Audio::SE::NO_5, 0.5f); // Message style hit SFX
            }
            m_judgeTextTime = currentTime;

            if (m_combo > m_maxCombo) m_maxCombo = m_combo;
        }
    }
}

void CyberBeat::updateJuice(uint32_t currentTime)
{
    m_juiceTime = currentTime;
}

Game::GameState CyberBeat::onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec)
{
    uint32_t currentTime = Platform::getMsec();
    updateJuice(currentTime);

    switch (m_mode) {
        case MODE_TITLE:
            if (input.justPressed(Input::Button::START)) {
                resetGame(currentTime);
                m_mode = MODE_PLAYING;
                audio.playMusic(&GAME_BGM); // Stream music session once
            }
            break;

        case MODE_PLAYING:
            if (input.justPressed(Input::Button::START)) {
                audio.stopMusic();
                m_mode = MODE_PAUSED;
                break;
            }

            // Procedural Note Generation
            if (Platform::elapsed(currentTime, m_lastSpawnTime, NOTE_SPAWN_INTERVAL)) {
                spawnNote(currentTime);
                m_lastSpawnTime = currentTime;
            }

            checkInput(input, audio, currentTime);

            // Process Dropout Notes (Handle MISS tracking)
            for (uint16_t i = 0; i < MAX_NOTES; ++i) {
                if (m_notes[i].active) {
                    if (currentTime > m_notes[i].targetTime && (currentTime - m_notes[i].targetTime) > JUDGE_GOOD) {
                        m_notes[i].active = false;
                        m_combo = 0; // Break combo counter
                        m_judgeText = "MISS";
                        m_judgeTextTime = currentTime;
                        m_lastMissTime = currentTime; // Trigger red flash juice overlay
                        audio.playSE(&Audio::SE::NO_2, 0.5f); // Fail sound
                        
                        m_life--; // Apply life penalty
                        if (m_life <= 0) {
                            m_life = 0;
                            audio.stopMusic();
                            m_mode = MODE_GAME_OVER;
                            audio.playSE(&Audio::SE::NO_4, 0.8f); // Game Over explosion SFX
                            break;
                        }
                    }
                }
            }

            // Check Win Condition
            if (m_score >= TARGET_SCORE) {
                audio.stopMusic();
                m_mode = MODE_STAGE_CLEAR;
                audio.playSE(&Audio::SE::NO_8, 0.8f);

                if (m_score > m_highScore) {
                    m_highScore = m_score;
                }
            }
            break;

        case MODE_PAUSED:
            if (input.justPressed(Input::Button::START)) {
                audio.playMusic(&GAME_BGM);
                m_mode = MODE_PLAYING;
            }
            break;

        case MODE_STAGE_CLEAR:
        case MODE_GAME_OVER:
            if (input.justPressed(Input::Button::START)) {
                m_mode = MODE_TITLE;
            }
            break;
    }

    m_lastTime = currentTime;
    return GameState::RUNNING;
}

bool CyberBeat::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    uint32_t currentTime = Platform::getMsec();
    
    // Intense Viewport Shake on PERFECT Hits
    if (m_mode == MODE_PLAYING && !Platform::elapsed(currentTime, m_lastPerfectTime, 60)) {
        int16_t shakeX = Math::random(-6, 7);
        int16_t shakeY = Math::random(-6, 7);
        graphics.setViewport(shakeX, shakeY);
    } else {
        graphics.setViewport(0, 0);
    }

    Graphics::Color bgColor = Graphics::rgb565(10, 14, 22);
    if (m_mode == MODE_PLAYING && !Platform::elapsed(currentTime, m_lastMissTime, 100)) {
        bgColor = Graphics::rgb565(60, 10, 10); // Red flash overlay
    }
    graphics.fillScreen(bgColor);

    // Render Sci-Fi Pulse Grid
    Graphics::Color gridColor = Graphics::rgb565(20, 40, 60);
    float beatPhase = Math::sin((currentTime * 0.001f) * (BEAT_BPM / 60.0f) * Math::TWO_PI);
    if (beatPhase > 0.0f) {
        gridColor = Graphics::rgb565(30, 60, 90);
    }

    for (int16_t x = 0; x <= 320; x += 40) graphics.drawLine(x, 0, x, 240, gridColor);
    for (int16_t y = 0; y <= 240; y += 30) graphics.drawLine(0, y, 320, y, gridColor);

    Graphics::Color cyanColor = Graphics::rgb565(0, 220, 255);
    Graphics::Color yellowColor = Graphics::rgb565(255, 220, 0);

    if (m_mode == MODE_TITLE) {
        graphics.drawString("CYBER BEAT", 160, 70, cyanColor, Graphics::Font::SIZE_42B, 
                            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        graphics.drawString("HIGH-TECH RHYTHM SYSTEM", 160, 110, yellowColor, Graphics::Font::SIZE_13, 
                            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

        char hiScoreBuf[32];
        snprintf(hiScoreBuf, sizeof(hiScoreBuf), "HI-SCORE: %u", m_highScore);
        graphics.drawString(hiScoreBuf, 160, 150, Graphics::Color::WHITE, Graphics::Font::SIZE_18, 
                            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

        if ((currentTime / 400) % 2 == 0) {
            graphics.drawString("PRESS START", 160, 190, Graphics::Color::WHITE, Graphics::Font::SIZE_18, 
                                Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }
    }
    else if (m_mode == MODE_PLAYING || m_mode == MODE_PAUSED) {
        // Optimized Lane Positions matching ergonomic layout [LEFT] [RIGHT] [Y] [A]
        int16_t laneX[4] = { 60, 120, 200, 260 };
        const char* laneLabels[4] = { "<", ">", "Y", "A" };

        for (int l = 0; l < 4; ++l) {
            graphics.drawCircle(laneX[l], JUDGE_LINE_Y, NOTE_RADIUS, Graphics::Color::WHITE);
            graphics.drawCircle(laneX[l], JUDGE_LINE_Y, NOTE_RADIUS + 2, gridColor);
            graphics.drawString(laneLabels[l], laneX[l], JUDGE_LINE_Y, Graphics::Color::WHITE, Graphics::Font::SIZE_13,
                                Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }

        // Render Active Flying Notes
        for (uint16_t i = 0; i < MAX_NOTES; ++i) {
            if (m_notes[i].active) {
                int16_t l = m_notes[i].lane;
                int32_t timeLeft = static_cast<int32_t>(m_notes[i].targetTime - currentTime);
                float progress = 1.0f - (static_cast<float>(timeLeft) / NOTE_TRAVEL_TIME);

                if (progress >= 0.0f && progress <= 1.0f) {
                    int16_t currentY = static_cast<int16_t>(progress * JUDGE_LINE_Y);
                    graphics.fillCircle(laneX[l], currentY, 6, cyanColor);
                    graphics.drawCircle(laneX[l], currentY, NOTE_RADIUS, Graphics::Color::WHITE);
                }
            }
        }

        // HUD Dashboard Rendering
        char scoreBuf[32];
        snprintf(scoreBuf, sizeof(scoreBuf), "SCORE: %u", m_score);
        graphics.drawString(scoreBuf, 15, 15, Graphics::Color::WHITE, Graphics::Font::SIZE_18);

        // Render Life Bar (Sci-Fi Segmented Blocks)
        graphics.drawString("LIFE:", 15, 38, Graphics::Color::WHITE, Graphics::Font::SIZE_13);
        for (int8_t i = 0; i < MAX_LIFE; ++i) {
            Graphics::Color barColor = (i < m_life) ? cyanColor : Graphics::rgb565(40, 40, 40);
            graphics.fillRect(55 + (i * 8), 39, 6, 10, barColor);
        }

        if (m_combo > 0) {
            char comboBuf[32];
            snprintf(comboBuf, sizeof(comboBuf), "%u COMBO", m_combo);
            graphics.drawString(comboBuf, 305, 15, yellowColor, Graphics::Font::SIZE_18, 
                                Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);
        }

        // Tweened Judgment Pop-up Text Animation
        if (m_judgeText && !Platform::elapsed(currentTime, m_judgeTextTime, 400)) {
            float elapsed = (currentTime - m_judgeTextTime) / 400.0f;
            float scaleY = Tween::value(0.0f, 1.0f, elapsed, Tween::Ease::EASE_OUT_BACK);
            int16_t textY = 110 - static_cast<int16_t>(scaleY * 15.0f);

            Graphics::Color judgeColor = Graphics::Color::WHITE;
            if (m_judgeText[0] == 'P') judgeColor = yellowColor;
            if (m_judgeText[0] == 'M') judgeColor = Graphics::rgb565(255, 60, 60);

            graphics.drawString(m_judgeText, 160, textY, judgeColor, Graphics::Font::SIZE_22B, 
                                Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }

        if (m_mode == MODE_PAUSED) {
            graphics.fillRect(0, 0, 320, 240, Graphics::rgb565(0, 0, 0));
            graphics.drawString("PAUSE", 160, 120, Graphics::Color::WHITE, Graphics::Font::SIZE_25B, 
                                Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
        }
    }
    else if (m_mode == MODE_STAGE_CLEAR || m_mode == MODE_GAME_OVER) {
        bool isClear = (m_mode == MODE_STAGE_CLEAR);
        
        graphics.drawString(isClear ? "STAGE CLEAR" : "GAME OVER", 160, 60, 
                            isClear ? yellowColor : Graphics::rgb565(255, 60, 60), 
                            Graphics::Font::SIZE_32B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

        char resBuf[64];
        snprintf(resBuf, sizeof(resBuf), "FINAL SCORE: %u", m_score);
        graphics.drawString(resBuf, 160, 110, Graphics::Color::WHITE, Graphics::Font::SIZE_18, 
                            // ... rendering safe results inside 228Y bounds ...
                            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

        snprintf(resBuf, sizeof(resBuf), "MAX COMBO: %u", m_maxCombo);
        graphics.drawString(resBuf, 160, 140, cyanColor, Graphics::Font::SIZE_18, 
                            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

        graphics.drawString("PRESS START TO RETURN", 160, 200, Graphics::Color::WHITE, Graphics::Font::SIZE_13, 
                            Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }

    return true;
}

void CyberBeat::onTerminate(Storage& storage)
{
    SaveData sd;
    if (sd.load(storage, getId(), "highscore.dat"))
    {
        sd.getUInt32("highscore", m_highScore);
        sd.save(storage, getId(), "highscore.dat");
    }
}

} // namespace PRUZEA

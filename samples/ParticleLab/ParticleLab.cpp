#include "ParticleLab.h"

#include <cstdio>
#include <cstring>

using namespace PRUZEA;

namespace
{
constexpr uint32_t TITLE_BLINK_MSEC = 500;

constexpr Graphics::Color COL_BG = Graphics::rgb565(5, 12, 22);
constexpr Graphics::Color COL_PANEL = Graphics::rgb565(11, 25, 39);
constexpr Graphics::Color COL_LINE = Graphics::rgb565(38, 74, 94);
constexpr Graphics::Color COL_TEXT = Graphics::rgb565(222, 238, 239);
constexpr Graphics::Color COL_DIM = Graphics::rgb565(118, 151, 160);
constexpr Graphics::Color COL_ACCENT = Graphics::rgb565(77, 218, 190);
constexpr Graphics::Color COL_HOT = Graphics::rgb565(255, 181, 61);

const char* effectName(ParticleLab::Effect effect)
{
    switch (effect)
    {
        case ParticleLab::Effect::EXPLOSION: return "EXPLOSION";
        case ParticleLab::Effect::SMOKE:     return "SMOKE";
        case ParticleLab::Effect::FOUNTAIN:  return "FOUNTAIN";
        case ParticleLab::Effect::SNOW:      return "SNOW";
        default:                             return "UNKNOWN";
    }
}
}

const char* ParticleLab::getId() const { return "particlelab"; }
const char* ParticleLab::getName() const { return "Particle Lab"; }
const char* ParticleLab::getMenuName() const { return "04 PARTICLE LAB"; }

uint16_t ParticleLab::getLogicalScreenWidth() const { return SCREEN_W; }
uint16_t ParticleLab::getLogicalScreenHeight() const { return SCREEN_H; }
uint16_t ParticleLab::getTargetScreenWidth() const { return SCREEN_W; }
uint16_t ParticleLab::getTargetScreenHeight() const { return SCREEN_H; }

void ParticleLab::onInit(Storage& storage)
{
    (void)storage;
    mode = Mode::TITLE;
    effect = Effect::EXPLOSION;
    autoEmit = true;
    titleBlinkMsec = Platform::getMsec();
    showPressStart = true;
    clearParticles();
    dirty = true;
}

Game::GameState ParticleLab::onUpdate(Input& input, Audio& audio,
                                      Storage& storage, float deltaSec)
{
    (void)storage;
    const uint32_t now = Platform::getMsec();

    if (mode == Mode::TITLE)
    {
        if (Platform::elapsed(now, titleBlinkMsec, TITLE_BLINK_MSEC))
        {
            titleBlinkMsec = now;
            showPressStart = !showPressStart;
            dirty = true;
        }

        if (input.justPressed(Input::A) || input.justPressed(Input::START))
        {
            mode = Mode::LAB;
            lastEmitMsec = now;
            emitSelected();
            audio.playSE(&Audio::SE::NO_8, 0.7f);
            dirty = true;
        }
        return Game::RUNNING;
    }

    if (input.justPressed(Input::LEFT)) selectEffect(-1, audio);
    if (input.justPressed(Input::RIGHT)) selectEffect(1, audio);

    if (input.justPressed(Input::A))
    {
        emitSelected();
        audio.playSE(&Audio::SE::NO_3, 0.45f);
    }

    if (input.justPressed(Input::X))
    {
        autoEmit = !autoEmit;
        audio.playSE(&Audio::SE::NO_1, 0.45f);
        dirty = true;
    }

    if (input.justPressed(Input::B))
    {
        clearParticles();
        audio.playSE(&Audio::SE::NO_2, 0.45f);
        dirty = true;
    }

    if (input.justPressed(Input::START))
    {
        clearParticles();
        mode = Mode::TITLE;
        titleBlinkMsec = now;
        showPressStart = true;
        audio.playSE(&Audio::SE::NO_2, 0.55f);
        dirty = true;
        return Game::RUNNING;
    }

    if (autoEmit)
    {
        uint32_t intervalMsec = 900;
        if (effect == Effect::SMOKE) intervalMsec = 120;
        if (effect == Effect::FOUNTAIN) intervalMsec = 90;
        if (effect == Effect::SNOW) intervalMsec = 75;

        if (Platform::elapsed(now, lastEmitMsec, intervalMsec))
        {
            lastEmitMsec = now;
            if (effect == Effect::EXPLOSION) spawnExplosion();
            else if (effect == Effect::SMOKE) spawnSmoke(2);
            else if (effect == Effect::FOUNTAIN) spawnFountain(2);
            else if (effect == Effect::SNOW) spawnSnow(2);
        }
    }

    updateParticles(deltaSec);
    if (activeParticleCount() > 0 || autoEmit) dirty = true;
    return Game::RUNNING;
}

void ParticleLab::clearParticles()
{
    std::memset(particles, 0, sizeof(particles));
}

ParticleLab::Particle* ParticleLab::acquireParticle()
{
    for (Particle& particle : particles)
    {
        if (!particle.active)
        {
            particle = {};
            particle.active = true;
            return &particle;
        }
    }
    return nullptr;
}

uint8_t ParticleLab::activeParticleCount() const
{
    uint8_t count = 0;
    for (const Particle& particle : particles)
        if (particle.active) ++count;
    return count;
}

void ParticleLab::selectEffect(int8_t direction, Audio& audio)
{
    int value = static_cast<int>(effect) + direction;
    const int count = static_cast<int>(Effect::COUNT);
    value = (value + count) % count;
    effect = static_cast<Effect>(value);
    clearParticles();
    lastEmitMsec = Platform::getMsec();
    emitSelected();
    audio.playSE(&Audio::SE::NO_1, 0.4f);
    dirty = true;
}

void ParticleLab::emitSelected()
{
    switch (effect)
    {
        case Effect::EXPLOSION: spawnExplosion(); break;
        case Effect::SMOKE:     spawnSmoke(10); break;
        case Effect::FOUNTAIN:  spawnFountain(12); break;
        case Effect::SNOW:      spawnSnow(14); break;
        default: break;
    }
    dirty = true;
}

void ParticleLab::updateParticles(float deltaSec)
{
    if (deltaSec > 0.05f) deltaSec = 0.05f;

    for (Particle& particle : particles)
    {
        if (!particle.active) continue;

        particle.life -= deltaSec;
        if (particle.life <= 0.0f)
        {
            particle.active = false;
            continue;
        }

        switch (effect)
        {
            case Effect::EXPLOSION: updateExplosion(particle, deltaSec); break;
            case Effect::SMOKE:     updateSmoke(particle, deltaSec); break;
            case Effect::FOUNTAIN:  updateFountain(particle, deltaSec); break;
            case Effect::SNOW:      updateSnow(particle, deltaSec); break;
            default: break;
        }
    }
}

void ParticleLab::spawnExplosion()
{
    for (uint8_t i = 0; i < 28; ++i)
    {
        Particle* particle = acquireParticle();
        if (!particle) return;

        const float angle = Math::randomFloat(0.0f, Math::TWO_PI);
        const float speed = Math::randomFloat(45.0f, 145.0f);
        particle->x = 160.0f;
        particle->y = 126.0f;
        particle->vx = Math::cos(angle) * speed;
        particle->vy = Math::sin(angle) * speed;
        particle->life = particle->maxLife = Math::randomFloat(0.35f, 0.9f);
        particle->size = Math::randomFloat(1.0f, 3.0f);
        particle->color = (i & 1) ? Graphics::YELLOW : Graphics::ORANGE;
    }
}

void ParticleLab::spawnSmoke(uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        Particle* particle = acquireParticle();
        if (!particle) return;

        particle->x = 160.0f + Math::randomFloat(-8.0f, 8.0f);
        particle->y = 185.0f + Math::randomFloat(-3.0f, 3.0f);
        particle->vx = Math::randomFloat(-10.0f, 10.0f);
        particle->vy = Math::randomFloat(-34.0f, -18.0f);
        particle->life = particle->maxLife = Math::randomFloat(1.2f, 2.2f);
        particle->size = Math::randomFloat(2.0f, 4.0f);
        particle->phase = Math::randomFloat(0.0f, Math::TWO_PI);
        particle->color = Graphics::rgb565(140, 155, 164);
    }
}

void ParticleLab::spawnFountain(uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        Particle* particle = acquireParticle();
        if (!particle) return;

        particle->x = 160.0f + Math::randomFloat(-3.0f, 3.0f);
        particle->y = 190.0f;
        particle->vx = Math::randomFloat(-48.0f, 48.0f);
        particle->vy = Math::randomFloat(-155.0f, -95.0f);
        particle->life = particle->maxLife = Math::randomFloat(1.1f, 1.7f);
        particle->size = Math::randomFloat(1.0f, 2.5f);
        particle->color = Graphics::rgb565(91, 209, 255);
    }
}

void ParticleLab::spawnSnow(uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        Particle* particle = acquireParticle();
        if (!particle) return;

        particle->x = Math::randomFloat(8.0f, 312.0f);
        particle->y = Math::randomFloat(36.0f, 72.0f);
        particle->vx = Math::randomFloat(-5.0f, 5.0f);
        particle->vy = Math::randomFloat(18.0f, 38.0f);
        particle->life = particle->maxLife = Math::randomFloat(3.8f, 6.5f);
        particle->size = Math::randomFloat(1.0f, 2.5f);
        particle->phase = Math::randomFloat(0.0f, Math::TWO_PI);
        particle->color = Graphics::WHITE;
    }
}

void ParticleLab::updateExplosion(Particle& particle, float deltaSec)
{
    particle.x += particle.vx * deltaSec;
    particle.y += particle.vy * deltaSec;
    particle.vx *= 1.0f - 2.2f * deltaSec;
    particle.vy *= 1.0f - 2.2f * deltaSec;
}

void ParticleLab::updateSmoke(Particle& particle, float deltaSec)
{
    particle.phase += deltaSec * 2.5f;
    particle.x += (particle.vx + Math::sin(particle.phase) * 8.0f) * deltaSec;
    particle.y += particle.vy * deltaSec;
    particle.size += deltaSec * 2.0f;
}

void ParticleLab::updateFountain(Particle& particle, float deltaSec)
{
    particle.vy += 150.0f * deltaSec;
    particle.x += particle.vx * deltaSec;
    particle.y += particle.vy * deltaSec;
    if (particle.y > 205.0f) particle.active = false;
}

void ParticleLab::updateSnow(Particle& particle, float deltaSec)
{
    particle.phase += deltaSec * 2.0f;
    particle.x += (particle.vx + Math::sin(particle.phase) * 11.0f) * deltaSec;
    particle.y += particle.vy * deltaSec;
    if (particle.y > 218.0f) particle.active = false;
}

bool ParticleLab::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty) return false;

    graphics.fillScreen(COL_BG);
    if (mode == Mode::TITLE) drawTitle(graphics);
    else drawLab(graphics);

    dirty = false;
    return true;
}

void ParticleLab::drawTitle(Graphics& graphics)
{
    graphics.drawString("PARTICLE LAB", 160, 72, COL_ACCENT, Graphics::SIZE_32B,
                        Graphics::HorizontalAlign::CENTER,
                        Graphics::VerticalAlign::MIDDLE);
    graphics.drawString("SMALL PATTERNS FOR GAME EFFECTS", 160, 106, COL_DIM,
                        Graphics::SIZE_13,
                        Graphics::HorizontalAlign::CENTER,
                        Graphics::VerticalAlign::MIDDLE);

    graphics.drawString("PRESS A / START", 160, 160,
                        showPressStart ? COL_HOT : COL_BG,
                        Graphics::SIZE_22B,
                        Graphics::HorizontalAlign::CENTER,
                        Graphics::VerticalAlign::MIDDLE);
}

void ParticleLab::drawLab(Graphics& graphics)
{
    graphics.fillRect(0, 0, SCREEN_W, 31, COL_PANEL);
    graphics.drawLine(0, 31, SCREEN_W - 1, 31, COL_LINE);

    graphics.drawString("<", 13, 15, COL_DIM, Graphics::SIZE_18,
                        Graphics::HorizontalAlign::CENTER,
                        Graphics::VerticalAlign::MIDDLE);
    graphics.drawString(effectName(effect), 160, 15, COL_ACCENT, Graphics::SIZE_18,
                        Graphics::HorizontalAlign::CENTER,
                        Graphics::VerticalAlign::MIDDLE);
    graphics.drawString(">", 307, 15, COL_DIM, Graphics::SIZE_18,
                        Graphics::HorizontalAlign::CENTER,
                        Graphics::VerticalAlign::MIDDLE);

    for (const Particle& particle : particles)
        if (particle.active) drawParticle(graphics, particle);

    char status[48];
    std::snprintf(status, sizeof(status), "PARTICLES %u   AUTO %s",
                  static_cast<unsigned>(activeParticleCount()),
                  autoEmit ? "ON" : "OFF");
    graphics.drawString(status, 8, 204, COL_DIM, Graphics::SIZE_13);

    graphics.drawString("L/R SELECT  A EMIT  X AUTO  B CLEAR", 160, 222,
                        COL_TEXT, Graphics::SIZE_13,
                        Graphics::HorizontalAlign::CENTER,
                        Graphics::VerticalAlign::MIDDLE);
}

void ParticleLab::drawParticle(Graphics& graphics, const Particle& particle)
{
    const int16_t x = static_cast<int16_t>(particle.x);
    const int16_t y = static_cast<int16_t>(particle.y);
    const uint16_t radius = static_cast<uint16_t>(particle.size < 1.0f ? 1.0f : particle.size);
    const float lifeRatio = particle.life / particle.maxLife;

    switch (effect)
    {
        case Effect::EXPLOSION:
            if (lifeRatio > 0.35f) graphics.fillCircle(x, y, radius, particle.color);
            else graphics.drawPixel(x, y, particle.color);
            break;

        case Effect::SMOKE:
            graphics.drawCircle(x, y, radius, particle.color);
            if (radius > 2) graphics.drawCircle(x, y, radius - 2, particle.color);
            break;

        case Effect::FOUNTAIN:
            graphics.drawLine(x, y, x - static_cast<int16_t>(particle.vx * 0.025f),
                              y - static_cast<int16_t>(particle.vy * 0.025f),
                              particle.color);
            break;

        case Effect::SNOW:
            graphics.drawPixel(x, y, particle.color);
            if (radius >= 2)
            {
                graphics.drawPixel(x - 1, y, particle.color);
                graphics.drawPixel(x + 1, y, particle.color);
                graphics.drawPixel(x, y - 1, particle.color);
                graphics.drawPixel(x, y + 1, particle.color);
            }
            break;

        default:
            break;
    }
}

void ParticleLab::onTerminate(Storage& storage)
{
    (void)storage;
}

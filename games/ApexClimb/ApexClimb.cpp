#include "ApexClimb.h"
#include <cstdio>
#include <cmath>

using namespace PRUZEA;

namespace
{
using P = ApexClimb::Point;

enum CourseTheme : uint8_t {
    THEME_APEX,
    THEME_SKYLINE,
    THEME_TOUGE,
    THEME_HAIRPINS
};

struct CourseDef {
    const char* id;
    const char* name;
    const char* subtitle;
    const P* points;
    uint8_t pointCount;
    CourseTheme theme;
    float startT;
    float sector1T;
    float sector2T;
    float finishT;
};

struct RailRange {
    float from;
    float to;
    int8_t side; // -1 right side, +1 left side relative to course tangent
};

static const Audio::SoundStep START_BEEP_STEPS[] = {
    {740, 740, 130, 0.75f, 0.75f}
};
static const Audio::Sound START_BEEP = {
    START_BEEP_STEPS, 1
};
static const Audio::SoundStep START_GO_STEPS[] = {
    {988, 988, 620, 0.85f, 0.85f}
};
static const Audio::Sound START_GO = {
    START_GO_STEPS, 1
};

// An original mountain road: fast lower section, switchback-heavy middle,
// then a narrower-feeling summit run. START is at the bottom.
static const Audio::SoundStep ENGINE_STEP_0[] = {
    {67, 67, 90, 0.80f, 0.72f},
    {67, 134, 28, 0.72f, 0.46f},
    {134, 67, 28, 0.46f, 0.72f},
    {67, 67, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_0 = {
    ENGINE_STEP_0, 4
};
static const Audio::SoundStep ENGINE_STEP_1[] = {
    {91, 91, 90, 0.80f, 0.72f},
    {91, 182, 28, 0.72f, 0.46f},
    {182, 91, 28, 0.46f, 0.72f},
    {91, 91, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_1 = {
    ENGINE_STEP_1, 4
};
static const Audio::SoundStep ENGINE_STEP_2[] = {
    {116, 116, 90, 0.80f, 0.72f},
    {116, 232, 28, 0.72f, 0.46f},
    {232, 116, 28, 0.46f, 0.72f},
    {116, 116, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_2 = {
    ENGINE_STEP_2, 4
};
static const Audio::SoundStep ENGINE_STEP_3[] = {
    {140, 140, 90, 0.80f, 0.72f},
    {140, 280, 28, 0.72f, 0.46f},
    {280, 140, 28, 0.46f, 0.72f},
    {140, 140, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_3 = {
    ENGINE_STEP_3, 4
};
static const Audio::SoundStep ENGINE_STEP_4[] = {
    {165, 165, 90, 0.80f, 0.72f},
    {165, 330, 28, 0.72f, 0.46f},
    {330, 165, 28, 0.46f, 0.72f},
    {165, 165, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_4 = {
    ENGINE_STEP_4, 4
};
static const Audio::SoundStep ENGINE_STEP_5[] = {
    {189, 189, 90, 0.80f, 0.72f},
    {189, 378, 28, 0.72f, 0.46f},
    {378, 189, 28, 0.46f, 0.72f},
    {189, 189, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_5 = {
    ENGINE_STEP_5, 4
};
static const Audio::SoundStep ENGINE_STEP_6[] = {
    {213, 213, 90, 0.80f, 0.72f},
    {213, 426, 28, 0.72f, 0.46f},
    {426, 213, 28, 0.46f, 0.72f},
    {213, 213, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_6 = {
    ENGINE_STEP_6, 4
};
static const Audio::SoundStep ENGINE_STEP_7[] = {
    {238, 238, 90, 0.80f, 0.72f},
    {238, 476, 28, 0.72f, 0.46f},
    {476, 238, 28, 0.46f, 0.72f},
    {238, 238, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_7 = {
    ENGINE_STEP_7, 4
};
static const Audio::SoundStep ENGINE_STEP_8[] = {
    {262, 262, 90, 0.80f, 0.72f},
    {262, 524, 28, 0.72f, 0.46f},
    {524, 262, 28, 0.46f, 0.72f},
    {262, 262, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_8 = {
    ENGINE_STEP_8, 4
};
static const Audio::SoundStep ENGINE_STEP_9[] = {
    {287, 287, 90, 0.80f, 0.72f},
    {287, 574, 28, 0.72f, 0.46f},
    {574, 287, 28, 0.46f, 0.72f},
    {287, 287, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_9 = {
    ENGINE_STEP_9, 4
};
static const Audio::SoundStep ENGINE_STEP_10[] = {
    {311, 311, 90, 0.80f, 0.72f},
    {311, 622, 28, 0.72f, 0.46f},
    {622, 311, 28, 0.46f, 0.72f},
    {311, 311, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_10 = {
    ENGINE_STEP_10, 4
};
static const Audio::SoundStep ENGINE_STEP_11[] = {
    {335, 335, 90, 0.80f, 0.72f},
    {335, 670, 28, 0.72f, 0.46f},
    {670, 335, 28, 0.46f, 0.72f},
    {335, 335, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_11 = {
    ENGINE_STEP_11, 4
};
static const Audio::SoundStep ENGINE_STEP_12[] = {
    {360, 360, 90, 0.80f, 0.72f},
    {360, 720, 28, 0.72f, 0.46f},
    {720, 360, 28, 0.46f, 0.72f},
    {360, 360, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_12 = {
    ENGINE_STEP_12, 4
};
static const Audio::SoundStep ENGINE_STEP_13[] = {
    {384, 384, 90, 0.80f, 0.72f},
    {384, 768, 28, 0.72f, 0.46f},
    {768, 384, 28, 0.46f, 0.72f},
    {384, 384, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_13 = {
    ENGINE_STEP_13, 4
};
static const Audio::SoundStep ENGINE_STEP_14[] = {
    {409, 409, 90, 0.80f, 0.72f},
    {409, 818, 28, 0.72f, 0.46f},
    {818, 409, 28, 0.46f, 0.72f},
    {409, 409, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_14 = {
    ENGINE_STEP_14, 4
};
static const Audio::SoundStep ENGINE_STEP_15[] = {
    {433, 433, 90, 0.80f, 0.72f},
    {433, 866, 28, 0.72f, 0.46f},
    {866, 433, 28, 0.46f, 0.72f},
    {433, 433, 94, 0.72f, 0.80f}
};
static const Audio::Sound ENGINE_SOUND_15 = {
    ENGINE_STEP_15, 4
};
static const Audio::Sound* ENGINE_SOUNDS[] = {
    &ENGINE_SOUND_0, &ENGINE_SOUND_1, &ENGINE_SOUND_2, &ENGINE_SOUND_3, &ENGINE_SOUND_4, &ENGINE_SOUND_5, &ENGINE_SOUND_6, &ENGINE_SOUND_7, &ENGINE_SOUND_8, &ENGINE_SOUND_9, &ENGINE_SOUND_10, &ENGINE_SOUND_11, &ENGINE_SOUND_12, &ENGINE_SOUND_13, &ENGINE_SOUND_14, &ENGINE_SOUND_15
};

static const P COURSE_APEX[] = {
    // Natural pre-start runoff: extend the original start road backwards along the same direction.
    {105, 2710}, {155, 2685}, {205, 2660}, {255, 2635}, {305, 2610}, {355, 2585}, {405, 2560}, {455, 2535}, {535, 2495},

    // Original all-round section.
    {690, 2405}, {815, 2260}, {790, 2105}, {625, 1990}, {365, 1940}, {155, 1810}, {190, 1645}, {455, 1575},
    {760, 1490}, {825, 1360}, {650, 1285}, {300, 1275}, {120, 1190}, {195, 1085}, {500, 1065}, {825, 1015},
    {835, 900}, {565, 850}, {185, 875}, {120, 755}, {420, 690}, {790, 610}, {730, 475},

    // Straighten before FINISH, then continue on exactly the same heading for the automatic-braking runoff.
    {590, 375}, {500, 310}, {410, 245}, {320, 180}, {230, 115}, {140, 50}, {100, 21}
};

// Open plateau high-speed course. It keeps the bottom-to-top flow, but now has enough length for a real long run.
// Two deliberate low-speed interruptions break the rhythm: a crank near the first third and one deep hairpin later on.
static const P COURSE_SKYLINE[] = {
    // RC27 layout stretched another 30 percent. Keep the fast-flow character and existing S-chicane.
    {500, 6510}, {500, 6354}, {505, 6198}, {520, 6026}, {610, 5854}, {760, 5636},
    {835, 5386}, {810, 5137}, {690, 4950}, {500, 4809}, {325, 4622}, {250, 4404},
    {300, 4201}, {470, 4060}, {585, 3943}, {690, 3826}, {730, 3709}, {680, 3592},
    {585, 3475}, {535, 3335}, {500, 3218}, {350, 3093}, {220, 2906}, {245, 2688},
    {420, 2532}, {650, 2391}, {815, 2204}, {835, 1986}, {735, 1814}, {555, 1689},
    {360, 1580}, {220, 1424}, {245, 1252}, {430, 1190}, {650, 1252}, {790, 1135},
    {825, 940}, {735, 769}, {565, 660}, {390, 535}, {280, 387}, {335, 246},
    {520, 168}, {675, 114}, {785, 59}, {820, 20},
};

// Low-speed technical mountain road. Irregular corner clusters are separated by short breathing spaces.
static const P COURSE_TOUGE[] = {
    {520, 3880}, {520, 3760}, {515, 3640}, {500, 3520},
    {430, 3435}, {270, 3380}, {170, 3280}, {205, 3180}, {390, 3135}, {550, 3070},
    {625, 2975}, {540, 2900}, {355, 2880}, {220, 2805}, {245, 2710}, {430, 2675},
    {650, 2630}, {775, 2545}, {730, 2450}, {545, 2420}, {365, 2360}, {250, 2265},
    {300, 2175}, {510, 2150}, {720, 2105}, {785, 2010}, {665, 1935}, {435, 1920},
    {245, 1850}, {205, 1750}, {350, 1685}, {590, 1645}, {745, 1560}, {700, 1450},
    {510, 1390}, {380, 1300}, {420, 1185}, {610, 1125}, {755, 1030}, {720, 925},
    {535, 875}, {330, 820}, {235, 730}, {285, 635}, {500, 605}, {710, 560},
    {785, 470}, {700, 390}, {610, 335},
    {540, 270}, {470, 205}, {400, 140}, {330, 75}, {270, 20}
};

// Night hairpin course. The first four linked hairpins retain the RC23 geometry (shifted only in world Y), because they already
// allow one continuous drift. The upper half adds more switchbacks, a short faster release, then another compact hairpin group.
static const P COURSE_HAIRPINS[] = {
    {500, 4180}, {500, 4070}, {500, 3960}, {500, 3860},

    // Keep the linked opening section intact; all four can be connected in one drift.
    {500, 3785}, {500, 3725}, {515, 3660}, {590, 3595}, {710, 3530}, {775, 3440}, {735, 3370}, {560, 3350},
    {370, 3310}, {300, 3220}, {345, 3140}, {510, 3115}, {670, 3140}, {740, 3070}, {705, 2990}, {555, 2960},
    {400, 2985}, {315, 2915}, {340, 2825}, {500, 2785}, {660, 2805}, {730, 2725}, {685, 2645}, {520, 2610},
    {360, 2635}, {285, 2550}, {335, 2465}, {515, 2435}, {700, 2460}, {770, 2370}, {720, 2280}, {540, 2245},
    {365, 2270}, {290, 2180}, {345, 2085}, {535, 2050}, {720, 2065}, {790, 1970}, {720, 1875}, {520, 1850},
    {345, 1800}, {295, 1705}, {365, 1620}, {560, 1585}, {735, 1515}, {775, 1415}, {650, 1345}, {505, 1305},

    // Upper half.
    {390, 1225}, {330, 1135}, {390, 1050}, {570, 1020}, {735, 1045}, {800, 955}, {735, 865}, {555, 840},
    {385, 865}, {300, 785}, {350, 690}, {535, 655}, {720, 680}, {790, 585}, {710, 495}, {520, 470},
    {355, 425}, {300, 335}, {390, 260},

    // Long final straight. FINISH is farther along the straight, with runoff still remaining after the line.
    {500, 225}, {610, 190}, {720, 155}, {820, 123}, {885, 102}, {925, 89}, {950, 81}, {965, 76}
};

static const CourseDef COURSES[] = {
    {"APEX", "APEX PASS", "ALL ROUND", COURSE_APEX, static_cast<uint8_t>(sizeof(COURSE_APEX) / sizeof(COURSE_APEX[0])),
        THEME_APEX, 0.145f, 0.420f, 0.705f, 0.885f},
    {"SKY", "SKYLINE", "HIGH SPEED", COURSE_SKYLINE, static_cast<uint8_t>(sizeof(COURSE_SKYLINE) / sizeof(COURSE_SKYLINE[0])),
        THEME_SKYLINE, 0.075f, 0.405f, 0.720f, 0.930f},
    {"TOUGE", "TOUGE", "LOW SPEED", COURSE_TOUGE, static_cast<uint8_t>(sizeof(COURSE_TOUGE) / sizeof(COURSE_TOUGE[0])),
        THEME_TOUGE, 0.065f, 0.365f, 0.690f, 0.900f},
    {"HAIR", "HAIRPINS", "ULTRA LOW", COURSE_HAIRPINS, static_cast<uint8_t>(sizeof(COURSE_HAIRPINS) / sizeof(COURSE_HAIRPINS[0])),
        THEME_HAIRPINS, 0.050f, 0.355f, 0.690f, 0.915f}
};

// Guardrails are deliberately concentrated at dangerous/tight sections.
// side +1/-1 selects one side of the road relative to the tangent.
static const RailRange RAILS_APEX[] = {
    {0.40f, 0.47f, +1}, {0.51f, 0.58f, +1}, {0.62f, 0.70f, +1}
};
static const RailRange RAILS_SKYLINE[] = {
    {0.33f, 0.44f, -1}, {0.66f, 0.78f, +1}
};
static const RailRange RAILS_TOUGE[] = {
    {0.39f, 0.45f, -1}, {0.78f, 0.84f, -1}
};
static const RailRange RAILS_HAIRPINS[] = {
    {0.18f, 0.24f, -1}, {0.50f, 0.56f, -1}
};

static const RailRange* getRailRanges(uint8_t course, uint8_t& count)
{
    switch (course) {
    case 0:
        count = static_cast<uint8_t>(sizeof(RAILS_APEX) / sizeof(RAILS_APEX[0]));
        return RAILS_APEX;
    case 1:
        count = static_cast<uint8_t>(sizeof(RAILS_SKYLINE) / sizeof(RAILS_SKYLINE[0]));
        return RAILS_SKYLINE;
    case 2:
        count = static_cast<uint8_t>(sizeof(RAILS_TOUGE) / sizeof(RAILS_TOUGE[0]));
        return RAILS_TOUGE;
    default:
        count = static_cast<uint8_t>(sizeof(RAILS_HAIRPINS) / sizeof(RAILS_HAIRPINS[0]));
        return RAILS_HAIRPINS;
    }
}

static constexpr uint16_t CAR_W = 16;
static constexpr uint16_t CAR_H = 26;
static const uint16_t CAR_BITMAP[] = {
    0xF81F,0xF81F,0xF81F,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xF81F,0xFD20,0xFFE0,0xFD20,0xFD20,0xFD20,0xFD20,0xFFE0,0xFD20,0xF81F,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0x4208,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFFE0,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFFE0,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xFD20,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xF81F,0xF81F,0xF81F,
    0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,0xF81F,
};

constexpr Graphics::Color GRASS_LOW  = Graphics::rgb565(33, 78, 37);
constexpr Graphics::Color GRASS_HIGH = Graphics::rgb565(58, 72, 48);
constexpr Graphics::Color ROCK       = Graphics::rgb565(92, 91, 82);
constexpr Graphics::Color ROCK_LIGHT = Graphics::rgb565(126, 122, 108);
constexpr Graphics::Color SHOULDER   = Graphics::rgb565(171, 166, 145);
constexpr Graphics::Color ROAD       = Graphics::rgb565(62, 64, 67);
constexpr Graphics::Color ROAD_EDGE  = Graphics::rgb565(218, 218, 202);
constexpr Graphics::Color PANEL      = Graphics::rgb565(15, 19, 22);
constexpr Graphics::Color PANEL_LINE = Graphics::rgb565(56, 70, 76);
constexpr Graphics::Color ACCENT     = Graphics::rgb565(255, 174, 45);
}

const char* ApexClimb::getId() const { return "apex_climb"; }
const char* ApexClimb::getName() const { return "APEX CLIMB"; }
const char* ApexClimb::getMenuName() const { return "APEX CLIMB"; }
uint16_t ApexClimb::getLogicalScreenWidth() const { return Display::ILI9341_SCREEN_W; }
uint16_t ApexClimb::getLogicalScreenHeight() const { return Display::ILI9341_SCREEN_H; }
uint16_t ApexClimb::getTargetScreenWidth() const { return Display::ILI9341_SCREEN_W; }
uint16_t ApexClimb::getTargetScreenHeight() const { return Display::ILI9341_SCREEN_H; }

void ApexClimb::onInit(Storage& storage)
{
    saveData.clear();
    if (storage.isAvailable()) saveData.load(storage, getId(), "save.ini");
    loadRanking(storage);
    saveDirty = false;
    mode = MODE_TITLE;
    prepareScenery();
    resetToStart();
}

void ApexClimb::onTerminate(Storage& storage)
{
    if (saveDirty) saveRanking(storage);
}

void ApexClimb::resetToStart()
{
    const float lineT = COURSES[currentCourse].startT;
    const Point linePoint = getCoursePoint(lineT);
    float startT = lineT;
    Point p = linePoint;
    Point previous = linePoint;
    float distanceBack = 0.0f;

    while (startT > 0.001f && distanceBack < 36.0f) {
        startT = startT - 0.001f > 0.0f ? startT - 0.001f : 0.0f;
        p = getCoursePoint(startT);
        const float dx = p.x - previous.x;
        const float dy = p.y - previous.y;
        distanceBack += std::sqrt(dx * dx + dy * dy);
        previous = p;
    }

    const Point tangent = getCourseTangent(startT);
    carPosition = Vector2(p.x, p.y);
    carAngle = Math::angle(tangent.x, tangent.y);
    velocity = Vector2();
    steering = 0.0f;
    digitalSteering = 0.0f;
    throttleInput = 0.0f;
    brakeInput = 0.0f;
    slipAngle = 0.0f;
    engineRpm = 1800.0f;
    displayRpm = 1800.0f;
    virtualGear = 1;
    engineSoundIndex = -1;
    engineSoundElapsed = 0.0f;
    drifting = false;
    driftBlend = 0.0f;
    frontGripUsage = 0.0f;
    rearGripFactor = 1.0f;
    handlingState = HANDLING_NEUTRAL;
    progress = startT;
    previousProgress = startT;
    sector = 0;
    sectorTimes[0] = sectorTimes[1] = sectorTimes[2] = 0;
    finishStopMsec = 0;
    finishEffectStartMsec = 0;
    finishStopped = false;
    finishRank = 0;
    finishDriftRank = 0;
    currentDriftScore = 0;
    bestDriftScore = 0;
    confirmedDriftScore = 0;
    driftSampleElapsedMsec = 0;
    driftScoreFlashStartMsec = 0;
    runDisqualified = false;
    skidLife = 0.0f;
    skidWriteIndex = 0;
    hasLastSkidSample = false;
    for (auto& skid : skidPoints) skid.active = false;
}

void ApexClimb::resetPractice()
{
    carPosition = Vector2(490.0f, 3350.0f);
    carAngle = -Math::PI * 0.5f;
    velocity = Vector2();
    steering = 0.0f;
    digitalSteering = 0.0f;
    throttleInput = 0.0f;
    brakeInput = 0.0f;
    slipAngle = 0.0f;
    engineRpm = 1800.0f;
    displayRpm = 1800.0f;
    virtualGear = 1;
    engineSoundIndex = -1;
    engineSoundElapsed = 0.0f;
    drifting = false;
    driftBlend = 0.0f;
    frontGripUsage = 0.0f;
    rearGripFactor = 1.0f;
    handlingState = HANDLING_NEUTRAL;
    currentDriftScore = 0;
    bestDriftScore = 0;
    confirmedDriftScore = 0;
    driftSampleElapsedMsec = 0;
    driftScoreFlashStartMsec = 0;
    skidLife = 0.0f;
    skidWriteIndex = 0;
    hasLastSkidSample = false;
    for (auto& skid : skidPoints) skid.active = false;
}

void ApexClimb::startReady()
{
    resetToStart();
    mode = MODE_READY;
    readyStartMsec = Platform::getMsec();
    readySignalStep = 0;
    dirty = true;
}

void ApexClimb::startRun()
{
    runStartMsec = Platform::getMsec();
    sectorStartMsec = runStartMsec;
    finishMsec = 0;
    finishStopMsec = 0;
    finishEffectStartMsec = 0;
    finishStopped = false;
    runDisqualified = false;
    mode = MODE_RUNNING;
}

Game::GameState ApexClimb::onUpdate(Input& input, Audio& audio,
                                       Storage& storage, float deltaSec)
{
    if (mode == MODE_TITLE) {
        if (input.justPressed(Input::A) || input.justPressed(Input::START)) {
            selectedCourse = currentCourse;
            mode = MODE_COURSE_SELECT;
        }
        dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_COURSE_SELECT) {
        if (input.justPressed(Input::UP)) {
            selectedCourse = selectedCourse == 0 ? COURSE_SELECT_COUNT - 1 : selectedCourse - 1;
        } else if (input.justPressed(Input::DOWN)) {
            selectedCourse = (selectedCourse + 1) % COURSE_SELECT_COUNT;
        }

        if (input.justPressed(Input::A) || input.justPressed(Input::START)) {
            if (selectedCourse == COURSE_COUNT) {
                freePractice = true;
                resetPractice();
                startRun();
            } else {
                freePractice = false;
                currentCourse = selectedCourse;
                bestMsec = rankings[currentCourse][0];
                prepareScenery();
                resetToStart();
                startReady();
            }
        } else if (input.justPressed(Input::B)) {
            mode = MODE_TITLE;
        }
        dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_READY) {
        const uint32_t elapsed = Platform::getMsec() - readyStartMsec;

        if (readySignalStep == 0) {
            audio.playSE(&START_BEEP, 0.75f);
            readySignalStep = 1;
        } else if (readySignalStep == 1 && elapsed >= 600) {
            audio.playSE(&START_BEEP, 0.75f);
            readySignalStep = 2;
        } else if (readySignalStep == 2 && elapsed >= 1200) {
            audio.playSE(&START_BEEP, 0.75f);
            readySignalStep = 3;
        }

        if (elapsed >= 1800) {
            audio.playSE(&START_GO, 0.85f);
            engineSoundMuteUntilMsec = Platform::getMsec() + 650;
            engineSoundIndex = -1;
            engineSoundElapsed = 0.0f;
            startRun();
        }
        dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_FINISHING) {
        updateFinishing(audio, deltaSec);
        dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_RESULT) {
        if (input.justPressed(Input::A) || input.justPressed(Input::START)) {
            mode = MODE_RANKING;
        } else if (input.justPressed(Input::B)) {
            mode = MODE_COURSE_SELECT;
        }
        dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_RANKING) {
        if (input.justPressed(Input::A) || input.justPressed(Input::START)) {
            startReady();
        } else if (input.justPressed(Input::B)) {
            mode = MODE_COURSE_SELECT;
        }
        dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_PAUSED) {
        if (input.justPressed(Input::START)) {
            mode = MODE_RUNNING;
            runStartMsec += Platform::getMsec() - readyStartMsec;
            sectorStartMsec += Platform::getMsec() - readyStartMsec;
        } else if (input.justPressed(Input::A)) {
            startReady();
        } else if (input.justPressed(Input::B)) {
            mode = MODE_COURSE_SELECT;
            if (freePractice) {
                freePractice = false;
            } else {
                resetToStart();
            }
        }
        dirty = true;
        return Game::GameState::RUNNING;
    }

    if (mode == MODE_TUNING) {
        updateTuning(input);
        if (input.justPressed(Input::START)) {
            const uint32_t paused = Platform::getMsec() - tuningPauseStartMsec;
            if (tuningReturnMode == MODE_RUNNING ||
                tuningReturnMode == MODE_PAUSED) {
                runStartMsec += paused;
                sectorStartMsec += paused;
            }
            mode = tuningReturnMode;
        }
        dirty = true;
        return Game::GameState::RUNNING;
    }


    if (input.justPressed(Input::START)) {
        mode = MODE_PAUSED;
        readyStartMsec = Platform::getMsec();
        dirty = true;
        return Game::GameState::RUNNING;
    }

    if (!freePractice && Platform::getMsec() - runStartMsec >= RUN_TIMEOUT_MSEC) {
        const uint32_t now = Platform::getMsec();
        if (drifting || currentDriftScore > 0) finishDriftScore(now);
        runDisqualified = true;
        finishMsec = RUN_TIMEOUT_MSEC;
        finishRank = 0;
        finishDriftRank = 0;
        velocity = Vector2();
        throttleInput = 0.0f;
        brakeInput = 0.0f;
        drifting = false;
        mode = MODE_RESULT;
        dirty = true;
        return Game::GameState::RUNNING;
    }

    updateDriving(input, audio, storage, deltaSec);
    dirty = true;
    return Game::GameState::RUNNING;
}

float ApexClimb::readSteering(const Input& input) const
{
    float value = 0.0f;
    if (input.hasAnalogSticks()) {
        value = static_cast<float>(input.axis(Input::LEFT_X)) / 1000.0f;
    }
    if (input.pressed(Input::LEFT)) value = -1.0f;
    if (input.pressed(Input::RIGHT)) value = 1.0f;
    return Math::clamp(value, -1.0f, 1.0f);
}

float ApexClimb::getDisplaySpeedKmh() const
{
    // Keep the original physical-to-km/h scale independent of the selected
    // top speed so changing top speed also changes the real velocity cap.
    static constexpr float KMH_PER_SPEED =
        195.0f / 148.0f;
    return velocity.length() * KMH_PER_SPEED;
}

float ApexClimb::getFrontGripLimit(float speedRatio) const
{
    return Math::lerp(
        tuning.frontGripLowSpeed,
        tuning.frontGripHighSpeed,
        speedRatio);
}

void ApexClimb::updateTuning(Input& input)
{
    static constexpr uint8_t ITEM_COUNT = 13;

    if (input.justPressed(Input::UP)) {
        tuningItem = tuningItem == 0 ? ITEM_COUNT - 1 : tuningItem - 1;
    }
    if (input.justPressed(Input::DOWN)) {
        tuningItem = (tuningItem + 1) % ITEM_COUNT;
    }

    float direction = 0.0f;
    if (input.justPressed(Input::LEFT)) direction = -1.0f;
    if (input.justPressed(Input::RIGHT)) direction = 1.0f;
    if (direction == 0.0f) return;

    switch (tuningItem) {
    case 0:
        presentation.engineGain =
            Math::clamp(presentation.engineGain + direction * 0.05f,
                        0.20f, 1.00f);
        break;
    case 1:
        presentation.tachRise =
            Math::clamp(presentation.tachRise + direction * 200.0f,
                        400.0f, 7000.0f);
        break;
    case 2:
        presentation.tachFall =
            Math::clamp(presentation.tachFall + direction * 200.0f,
                        400.0f, 7000.0f);
        break;
    case 3:
        presentation.gear2Kmh =
            Math::clamp(presentation.gear2Kmh + direction * 5.0f,
                        35.0f, presentation.gear3Kmh - 10.0f);
        break;
    case 4:
        presentation.gear3Kmh =
            Math::clamp(presentation.gear3Kmh + direction * 5.0f,
                        presentation.gear2Kmh + 10.0f,
                        presentation.gear4Kmh - 10.0f);
        break;
    case 5:
        presentation.gear4Kmh =
            Math::clamp(presentation.gear4Kmh + direction * 5.0f,
                        presentation.gear3Kmh + 10.0f,
                        presentation.gear5Kmh - 10.0f);
        break;
    case 6:
        presentation.gear5Kmh =
            Math::clamp(presentation.gear5Kmh + direction * 5.0f,
                        presentation.gear4Kmh + 10.0f,
                        presentation.topSpeedKmh - 5.0f);
        break;
    case 7:
        presentation.acceleration =
            Math::clamp(presentation.acceleration + direction * 2.0f,
                        20.0f, 100.0f);
        break;
    case 8:
        presentation.brakeForce =
            Math::clamp(presentation.brakeForce + direction * 5.0f,
                        40.0f, 220.0f);
        break;
    case 9:
        presentation.movementScale =
            Math::clamp(presentation.movementScale + direction * 0.05f,
                        0.70f, 1.80f);
        break;
    case 10:
        presentation.topSpeedKmh =
            Math::clamp(presentation.topSpeedKmh + direction * 5.0f,
                        presentation.gear5Kmh + 5.0f, 280.0f);
        break;
    case 11:
        presentation.highAccelStartKmh =
            Math::clamp(presentation.highAccelStartKmh + direction * 5.0f,
                        80.0f, presentation.topSpeedKmh - 20.0f);
        break;
    case 12:
        presentation.highAccelScale =
            Math::clamp(presentation.highAccelScale + direction * 0.05f,
                        0.10f, 1.00f);
        break;
    }
}

void ApexClimb::updateDriving(Input& input, Audio& audio,
                              Storage& storage, float deltaSec)
{
    const bool wasDrifting = drifting;
    float targetThrottle = 0.0f, targetBrake = 0.0f, targetSteering = 0.0f;
    if (input.hasAnalogSticks()) {
        const float rightY = static_cast<float>(input.axis(Input::RIGHT_Y)) / 1000.0f;
        targetThrottle = Math::clamp(-rightY, 0.0f, 1.0f);
        targetBrake = Math::clamp(rightY, 0.0f, 1.0f);
        targetSteering = readSteering(input);
        steering = Math::moveTowards(steering, targetSteering,
                                     tuning.steeringResponse * deltaSec);
        throttleInput = targetThrottle;
    } else {
        if (input.pressed(Input::LEFT)) targetSteering -= 1.0f;
        if (input.pressed(Input::RIGHT)) targetSteering += 1.0f;
        if (input.pressed(Input::Y) || input.pressed(Input::UP)) targetThrottle = 1.0f;

        const float steerRate = 1.0f / (tuning.digitalFullSteerTime > 0.01f ? tuning.digitalFullSteerTime : 0.01f);
        digitalSteering = Math::moveTowards(digitalSteering, targetSteering,
                                            steerRate * deltaSec);
        steering = digitalSteering;

        const float response = targetThrottle > throttleInput
            ? tuning.digitalFullThrottleTime : tuning.digitalThrottleReleaseTime;
        throttleInput = Math::moveTowards(throttleInput, targetThrottle,
                                          deltaSec / (response > 0.01f ? response : 0.01f));
    }
    if (input.pressed(Input::B) || input.pressed(Input::DOWN))
        targetBrake = 1.0f;

    if (input.hasAnalogSticks()) {
        brakeInput = targetBrake;
    } else {
        const float response =
            targetBrake > brakeInput
                ? tuning.digitalFullBrakeTime
                : tuning.digitalBrakeReleaseTime;
        const float safeResponse = response > 0.01f ? response : 0.01f;
        brakeInput = Math::moveTowards(
            brakeInput, targetBrake, deltaSec / safeResponse);
    }

    const float throttle = throttleInput;
    const float brake = brakeInput;

    Vector2 forward(Math::cos(carAngle), Math::sin(carAngle));
    Vector2 side(-forward.y, forward.x);
    float forwardSpeed = velocity.dot(forward);
    float speed = velocity.length();
    static constexpr float KMH_PER_SPEED_RATIO =
        195.0f / 148.0f;
    const float currentMaxSpeed =
        presentation.topSpeedKmh / KMH_PER_SPEED_RATIO;
    float speedRatio =
        Math::clamp(speed / currentMaxSpeed, 0.0f, 1.0f);
    const float speedKmh = getDisplaySpeedKmh();

    const float gripLimit = getFrontGripLimit(speedRatio);
    const float steeringDemand = std::fabs(steering);
    frontGripUsage =
        gripLimit > 0.001f ? steeringDemand / gripLimit : 99.0f;

    // There are two ways to exceed the rear-tire limit:
    // 1) intentional brake + steering,
    // 2) steering far beyond what the current speed can support.
    const bool brakeDrift =
        speedKmh >= tuning.driftMinSpeedKmh &&
        steeringDemand >= tuning.brakeDriftSteer &&
        brake >= tuning.brakeDriftAmount;

    const bool steeringOverLimit =
        speedKmh >= tuning.driftMinSpeedKmh &&
        steeringDemand > gripLimit + tuning.steerDriftMargin;

    if (brakeDrift || steeringOverLimit) {
        drifting = true;
    } else if (drifting &&
               (speedKmh < tuning.driftExitSpeedKmh ||
                (steeringDemand < tuning.driftExitSteer &&
                 slipAngle < tuning.driftExitSlipRad))) {
        drifting = false;
    }

    if (drifting && throttleInput < 0.08f) {
        driftBlend = Math::moveTowards(driftBlend, 0.0f,
                                       tuning.liftDriftRecovery * deltaSec);
        if (driftBlend <= 0.12f) drifting = false;
    } else {
        driftBlend = Math::moveTowards(driftBlend, drifting ? 1.0f : 0.0f,
                                       (drifting ? 5.5f : 3.0f) * deltaSec);
    }

    const float accelerationScale =
        Math::lerp(1.0f, tuning.driftAccelerationScale, driftBlend);

    if (throttle > 0.0f) {
        if (forwardSpeed < -2.0f) {
            velocity += forward *
                (presentation.brakeForce * throttle * deltaSec);
        } else {
            float highSpeedScale = 1.0f;
            const float speedKmhNow = getDisplaySpeedKmh();
            if (speedKmhNow > presentation.highAccelStartKmh) {
                const float denom =
                    presentation.topSpeedKmh - presentation.highAccelStartKmh;
                const float t = denom > 1.0f
                    ? Math::clamp(
                        (speedKmhNow - presentation.highAccelStartKmh) / denom,
                        0.0f, 1.0f)
                    : 1.0f;
                highSpeedScale =
                    Math::lerp(1.0f, presentation.highAccelScale, t);
            }

            velocity += forward *
                (presentation.acceleration *
                 accelerationScale * highSpeedScale *
                 throttle * deltaSec);
        }
    }

    const bool reverseRequest = brake > 0.0f && (forwardSpeed < -1.0f || speedKmh < 3.0f);
    if (reverseRequest) {
        drifting = false;
        driftBlend = Math::moveTowards(driftBlend, 0.0f, 8.0f * deltaSec);

        const float reverseSpeed = -velocity.dot(forward);
        if (reverseSpeed < REVERSE_SPEED) velocity -= forward * (presentation.acceleration * 0.65f * brake * deltaSec);
    }

    if (brake > 0.0f && !reverseRequest) {
        speed = velocity.length();

        // Full braking at medium/high speed, softer near walking pace.
        const float speedKmhNow = getDisplaySpeedKmh();
        const float speedBrakeT =
            Math::clamp(speedKmhNow / tuning.brakeFullEffectKmh, 0.0f, 1.0f);
        const float lowSpeedScale =
            Math::lerp(tuning.brakeLowSpeedScale, 1.0f, speedBrakeT);

        // A sideways car should scrub speed rather than stop instantly.
        const float cappedSlip =
            Math::clamp(slipAngle, 0.0f, Math::PI * 0.5f);
        const float forwardness = Math::cos(cappedSlip);
        const float slipBrakeScale =
            tuning.slideBrakeMinScale +
            (1.0f - tuning.slideBrakeMinScale) * forwardness;

        const float effectiveBrake =
            presentation.brakeForce *
            brake *
            lowSpeedScale *
            slipBrakeScale;

        if (forwardSpeed > 5.0f && speed > 0.01f) {
            const float denom = speed > 1.0f ? speed : 1.0f;
            const float brakeScale = Math::clamp(
                effectiveBrake * deltaSec / denom,
                0.0f, 0.72f);
            velocity *= 1.0f - brakeScale;
        } else if (forwardSpeed < -5.0f && speed > 0.01f) {
            const float denom = speed > 1.0f ? speed : 1.0f;
            const float brakeScale = Math::clamp(
                effectiveBrake * deltaSec / denom,
                0.0f, 0.72f);
            velocity *= 1.0f - brakeScale;
        }
    }

    if (throttle <= 0.0f && brake <= 0.0f && speed > 0.01f) {
        const float denom = speed > 1.0f ? speed : 1.0f;
        const float dragScale = Math::clamp(
            tuning.coastDrag * deltaSec / denom,
            0.0f, 0.55f);
        velocity *= 1.0f - dragScale;
    }

    forwardSpeed = velocity.dot(forward);
    speed = velocity.length();
    speedRatio = Math::clamp(speed / currentMaxSpeed, 0.0f, 1.0f);

    // Zero speed means zero yaw. A small amount of motion quickly enables
    // the tight low-speed RC-like turning radius.
    const float lowSpeedEnable =
        Math::clamp(speed / tuning.steeringEnableSpeed, 0.0f, 1.0f);

    // Front tire saturation produces understeer before drift.
    float frontYawScale = 1.0f;
    if (!drifting && frontGripUsage > 1.0f) {
        frontYawScale = Math::clamp(
            1.0f / frontGripUsage,
            tuning.understeerMinYaw, 1.0f);
    }

    float turnRate =
        tuning.lowSpeedTurnRate *
        lowSpeedEnable *
        frontYawScale;

    turnRate *= Math::lerp(
        1.0f, tuning.driftYawScale, driftBlend);

    const float direction = forwardSpeed >= 0.0f ? 1.0f : -1.0f;
    carAngle += steering * turnRate * direction * deltaSec;

    forward = Vector2(Math::cos(carAngle), Math::sin(carAngle));
    side = Vector2(-forward.y, forward.x);

    // Separate longitudinal and lateral motion. Normal grip removes lateral
    // velocity quickly; drift keeps it, making counter-steer meaningful.
    const float longitudinal = velocity.dot(forward);
    float lateral = velocity.dot(side);

    float align = Math::lerp(tuning.gripAlign, tuning.driftGripAlign, driftBlend);
    if (throttleInput < 0.08f && slipAngle > 0.04f)
        align = (align > tuning.driftGripAlign * tuning.liftGripScale ? align : tuning.driftGripAlign * tuning.liftGripScale);
    lateral *= Math::clamp(1.0f - align * deltaSec, 0.0f, 1.0f);

    velocity = forward * longitudinal + side * lateral;

    // DRIFT FORWARD controls how much entry speed is retained while sliding.
    // 1.00 preserves it almost completely; lower values make long slides
    // progressively more expensive without affecting drift acceleration.
    if (drifting && slipAngle > 0.05f) {
        const float inefficiency =
            1.0f - tuning.driftForwardEfficiency;
        const float loss = Math::clamp(
            inefficiency * slipAngle * 1.35f * deltaSec,
            0.0f, 0.18f);
        velocity *= 1.0f - loss;
    }

    speed = velocity.length();
    static constexpr float KMH_PER_SPEED =
        195.0f / 148.0f;
    const float maxSpeed =
        presentation.topSpeedKmh / KMH_PER_SPEED;
    if (speed > maxSpeed) {
        velocity *= maxSpeed / speed;
        speed = maxSpeed;
    }

    const float reverseComponent = -velocity.dot(forward);
    if (reverseComponent > REVERSE_SPEED) velocity += forward * (reverseComponent - REVERSE_SPEED);

    const float roadDistance =
        freePractice ? 0.0f : distanceToTrack(carPosition.x, carPosition.y);

    if (!freePractice && roadDistance > ROAD_HALF_WIDTH) {
        float drag = tuning.offroadDrag;
        if (reverseRequest && speed < 24.0f) drag *= 0.03f;
        else if (speed < 26.0f) drag *= 0.18f;
        else if (speed < 52.0f) drag *= 0.48f;

        if (speed > 0.01f) {
            const float denom = speed > 1.0f ? speed : 1.0f;
            const float scale = Math::clamp(drag * deltaSec / denom, 0.0f, 0.78f);
            velocity *= 1.0f - scale;
        }

        if (throttle > 0.0f && velocity.length() < 28.0f) velocity += forward * (46.0f * throttle * deltaSec);

        if (reverseRequest && -velocity.dot(forward) < 10.0f) {
            velocity -= forward * (presentation.acceleration * 0.45f * brake * deltaSec);
        }
    }

    carPosition += velocity * deltaSec * presentation.movementScale;

    if (freePractice) {
        static constexpr float PRACTICE_LEFT = 58.0f;
        static constexpr float PRACTICE_RIGHT = 922.0f;
        static constexpr float PRACTICE_TOP = 2768.0f;
        static constexpr float PRACTICE_BOTTOM = 3932.0f;

        if (carPosition.x < PRACTICE_LEFT) {
            carPosition.x = PRACTICE_LEFT;
            if (velocity.x < 0.0f) velocity.x = 0.0f;
        } else if (carPosition.x > PRACTICE_RIGHT) {
            carPosition.x = PRACTICE_RIGHT;
            if (velocity.x > 0.0f) velocity.x = 0.0f;
        }

        if (carPosition.y < PRACTICE_TOP) {
            carPosition.y = PRACTICE_TOP;
            if (velocity.y < 0.0f) velocity.y = 0.0f;
        } else if (carPosition.y > PRACTICE_BOTTOM) {
            carPosition.y = PRACTICE_BOTTOM;
            if (velocity.y > 0.0f) velocity.y = 0.0f;
        }
    } else {
        carPosition.x = Math::clamp(carPosition.x, 8.0f, static_cast<float>(WORLD_W - 8));
        carPosition.y = Math::clamp(carPosition.y, 8.0f, static_cast<float>(WORLD_H - 8));
        resolveGuardrailCollision(deltaSec);
    }

    const float velocityAngle =
        velocity.length() > 3.0f
            ? Math::angle(velocity.x, velocity.y)
            : carAngle;

    float angleDiff = velocityAngle - carAngle;
    while (angleDiff > Math::PI) angleDiff -= Math::TWO_PI;
    while (angleDiff < -Math::PI) angleDiff += Math::TWO_PI;
    slipAngle = std::fabs(angleDiff);

    rearGripFactor =
        Math::lerp(1.0f, tuning.driftGripAlign / tuning.gripAlign, driftBlend);

    if (drifting) {
        // Once a drift begins, keep the HUD in DRIFT through counter-steer
        // until the actual drift state returns to grip.
        handlingState = HANDLING_DRIFT;
    } else if (frontGripUsage > 1.0f) {
        handlingState = HANDLING_UNDER;
    } else {
        handlingState = HANDLING_NEUTRAL;
    }

    const uint32_t nowForDrift = Platform::getMsec();
    if (drifting) {
        driftSampleElapsedMsec += static_cast<uint32_t>(deltaSec * 1000.0f + 0.5f);
        while (driftSampleElapsedMsec >= DRIFT_SAMPLE_MSEC) {
            currentDriftScore += static_cast<uint32_t>(getDisplaySpeedKmh() + 0.5f);
            driftSampleElapsedMsec -= DRIFT_SAMPLE_MSEC;
        }
    } else {
        driftSampleElapsedMsec = 0;
        if (wasDrifting || currentDriftScore > 0) finishDriftScore(nowForDrift);
    }

    const Vector2 rear = carPosition - forward * 7.0f;
    previousSkidLeft = skidLeft;
    previousSkidRight = skidRight;
    skidLeft = rear + side * 5.2f;
    skidRight = rear - side * 5.2f;

    if (drifting) {
        const Vector2 c = (skidLeft + skidRight) * 0.5f;
        const Vector2 d = c - lastSkidSample;
        if (!hasLastSkidSample || d.x*d.x + d.y*d.y >= 16.0f) {
            skidPoints[skidWriteIndex] = {skidLeft, skidRight, true};
            skidWriteIndex = (skidWriteIndex + 1) % SKID_POINT_COUNT;
            lastSkidSample = c; hasLastSkidSample = true;
        }
    } else hasLastSkidSample = false;

    if ((drifting || slipAngle > 0.15f) &&
        roadDistance <= ROAD_HALF_WIDTH + 3.0f) {
        skidLife = 0.18f;
    } else {
        skidLife = skidLife - deltaSec > 0.0f
            ? skidLife - deltaSec : 0.0f;
    }

    if (freePractice) {
        updateEngineAudio(audio, deltaSec);
        return;
    }

    previousProgress = progress;
    progress = getTrackProgress(carPosition.x, carPosition.y);

    const uint32_t now = Platform::getMsec();
    if (sector == 0 &&
        previousProgress < COURSES[currentCourse].sector1T && progress >= COURSES[currentCourse].sector1T) {
        sectorTimes[0] = now - sectorStartMsec;
        sectorStartMsec = now;
        sector = 1;
        audio.playSE(&Audio::SE::NO_11, 0.35f);
        engineSoundMuteUntilMsec = now + 140;
        engineSoundIndex = -1;
        engineSoundElapsed = 0.0f;
    } else if (sector == 1 &&
               previousProgress < COURSES[currentCourse].sector2T && progress >= COURSES[currentCourse].sector2T) {
        sectorTimes[1] = now - sectorStartMsec;
        sectorStartMsec = now;
        sector = 2;
        audio.playSE(&Audio::SE::NO_11, 0.35f);
        engineSoundMuteUntilMsec = now + 140;
        engineSoundIndex = -1;
        engineSoundElapsed = 0.0f;
    }

    if (sector >= 2 &&
        previousProgress < COURSES[currentCourse].finishT && progress >= COURSES[currentCourse].finishT) {
        sectorTimes[2] = now - sectorStartMsec;
        finishRun(audio, storage);
        return;
    }

    updateEngineAudio(audio, deltaSec);
}

void ApexClimb::resolveGuardrailCollision(float deltaSec)
{
    uint8_t rangeCount = 0;
    const RailRange* ranges = getRailRanges(currentCourse, rangeCount);

    static constexpr float RAIL_OFFSET = SHOULDER_HALF_WIDTH + 6.0f;
    static constexpr float CAR_RAIL_RADIUS = 8.5f;

    for (uint8_t r = 0; r < rangeCount; ++r) {
        const RailRange& range = ranges[r];
        static constexpr int SEGMENTS = 18;

        for (int i = 0; i < SEGMENTS; ++i) {
            const float t0 = Math::lerp(
                range.from, range.to,
                static_cast<float>(i) / SEGMENTS);
            const float t1 = Math::lerp(
                range.from, range.to,
                static_cast<float>(i + 1) / SEGMENTS);

            auto railPoint = [&](float t) -> Vector2 {
                const Point p = getCoursePoint(t);
                Point tangent = getCourseTangent(t);
                const float len =
                    std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
                if (len <= 0.001f) return Vector2(p.x, p.y);
                tangent.x /= len;
                tangent.y /= len;
                const float nx = -tangent.y;
                const float ny = tangent.x;
                return Vector2(
                    p.x + nx * RAIL_OFFSET * range.side,
                    p.y + ny * RAIL_OFFSET * range.side);
            };

            const Vector2 a = railPoint(t0);
            const Vector2 b = railPoint(t1);
            const Vector2 ab = b - a;
            const float len2 = ab.dot(ab);
            if (len2 <= 0.001f) continue;

            const Vector2 ap = carPosition - a;
            const float u = Math::clamp(ap.dot(ab) / len2, 0.0f, 1.0f);
            const Vector2 closest = a + ab * u;
            Vector2 away = carPosition - closest;
            float distance = away.length();

            if (distance >= CAR_RAIL_RADIUS) continue;

            if (distance <= 0.001f) {
                Point tangent = getCourseTangent((t0 + t1) * 0.5f);
                const float len =
                    std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
                if (len > 0.001f) {
                    tangent.x /= len;
                    tangent.y /= len;
                    // Point toward the road center.
                    away = Vector2(
                        tangent.y * range.side,
                        -tangent.x * range.side);
                    distance = 1.0f;
                } else {
                    away = Vector2(1.0f, 0.0f);
                    distance = 1.0f;
                }
            }

            away *= 1.0f / distance;

            // Keep the car on the road side of the rail.
            carPosition = closest + away * CAR_RAIL_RADIUS;

            // Remove motion into the rail. A clear impact penalty plus
            // continuous tangential friction makes wall-riding slower than
            // braking correctly for the corner.
            const float intoRail = velocity.dot(away);
            if (intoRail < 0.0f) {
                velocity -= away * intoRail;
                velocity *= 0.68f;
            }

            const float scrapeScale =
                Math::clamp(1.0f - 3.8f * deltaSec, 0.0f, 1.0f);
            velocity *= scrapeScale;
        }
    }
}

void ApexClimb::updateEngineAudio(Audio& audio, float deltaSec)
{
    const float kmh = getDisplaySpeedKmh();

    // Virtual 5-speed gearbox. It exists only to give the engine sound
    // believable rises and drops; vehicle physics remain unchanged.
    const float SHIFT_SPEEDS[] = {
        0.0f,
        presentation.gear2Kmh,
        presentation.gear3Kmh,
        presentation.gear4Kmh,
        presentation.gear5Kmh,
        presentation.topSpeedKmh
    };

    uint8_t gear = 1;
    for (uint8_t g = 1; g < 5; ++g) {
        if (kmh >= SHIFT_SPEEDS[g]) gear = g + 1;
    }
    virtualGear = gear;

    const float low = SHIFT_SPEEDS[gear - 1];
    const float high = SHIFT_SPEEDS[gear];
    const float gearSpan = high - low > 1.0f ? high - low : 1.0f;
    const float gearT = Math::clamp((kmh - low) / gearSpan, 0.0f, 1.0f);

    float targetRpm = Math::lerp(2600.0f, 7800.0f, gearT);

    // Wheelspin / slip raises RPM without requiring more road speed.
    if (drifting || slipAngle > 0.10f) {
        const float slipT =
            Math::clamp(slipAngle / 0.65f, 0.0f, 1.0f);
        const float throttleEffect =
            0.30f + 0.70f * throttleInput;
        targetRpm += 1900.0f * slipT * throttleEffect;
    }

    if (throttleInput < 0.05f) {
        targetRpm *= 0.82f;
    }

    targetRpm = Math::clamp(targetRpm, 1600.0f, 9000.0f);
    engineRpm = Math::moveTowards(
        engineRpm, targetRpm,
        (targetRpm > engineRpm ? 9000.0f : 12000.0f) * deltaSec);

    displayRpm = Math::moveTowards(
        displayRpm, engineRpm,
        (engineRpm > displayRpm
            ? presentation.tachRise
            : presentation.tachFall) * deltaSec);

    if (Platform::getMsec() < engineSoundMuteUntilMsec) {
        engineSoundIndex = -1;
        engineSoundElapsed = 0.0f;
        return;
    }

    const float rpmT =
        Math::clamp((displayRpm - 1600.0f) / (9000.0f - 1600.0f), 0.0f, 1.0f);
    const int count =
        static_cast<int>(sizeof(ENGINE_SOUNDS) / sizeof(ENGINE_SOUNDS[0]));
    const float bandPosition =
        rpmT * static_cast<float>(count - 1);

    int index;
    if (engineSoundIndex < 0) {
        index = Math::clamp(
            static_cast<int>(bandPosition + 0.5f), 0, count - 1);
    } else {
        // Hysteresis keeps the engine from chattering between adjacent
        // RPM bands near a boundary.
        index = engineSoundIndex;
        while (index < count - 1 &&
               bandPosition > static_cast<float>(index) + 0.65f) {
            ++index;
        }
        while (index > 0 &&
               bandPosition < static_cast<float>(index) - 0.65f) {
            --index;
        }
    }

    engineSoundElapsed += deltaSec;

    // Each band is a short four-step engine texture (~240 ms).
    // AudioBase preserves oscillator phase when replacing an active SE,
    // so renewing it before the end keeps the sound continuous.
    static constexpr float ENGINE_SOUND_RENEW_SEC = 0.16f;
    if (index == engineSoundIndex &&
        engineSoundElapsed < ENGINE_SOUND_RENEW_SEC) {
        return;
    }

    audio.playSE(ENGINE_SOUNDS[index], presentation.engineGain);
    engineSoundIndex = static_cast<int8_t>(index);
    engineSoundElapsed = 0.0f;
}

bool ApexClimb::crossedGate(float gateT) const
{
    const Point gate = getCoursePoint(gateT);
    Point tangent = getCourseTangent(gateT);
    const float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    if (len <= 0.001f) return false;
    tangent.x /= len;
    tangent.y /= len;

    const Vector2 previousPosition = carPosition - velocity * (1.0f / 60.0f);
    const float before =
        (previousPosition.x - gate.x) * tangent.x +
        (previousPosition.y - gate.y) * tangent.y;
    const float after =
        (carPosition.x - gate.x) * tangent.x +
        (carPosition.y - gate.y) * tangent.y;

    const float lateral =
        std::fabs((carPosition.x - gate.x) * (-tangent.y) +
                  (carPosition.y - gate.y) * tangent.x);

    return before < 0.0f && after >= 0.0f &&
           lateral <= ROAD_HALF_WIDTH + 5.0f;
}

bool ApexClimb::reachedFinishZone() const
{
    if (progress < COURSES[currentCourse].finishT - 0.018f) return false;
    const Point f = getCoursePoint(COURSES[currentCourse].finishT);
    const float dx = carPosition.x-f.x, dy = carPosition.y-f.y;
    const float r = ROAD_HALF_WIDTH + 24.0f;
    return dx*dx + dy*dy <= r*r;
}

void ApexClimb::finishDriftScore(uint32_t now)
{
    if (currentDriftScore == 0) return;

    confirmedDriftScore = currentDriftScore;
    if (currentDriftScore > bestDriftScore) bestDriftScore = currentDriftScore;
    if (currentDriftScore >= DRIFT_DISPLAY_THRESHOLD) driftScoreFlashStartMsec = now;

    currentDriftScore = 0;
    driftSampleElapsedMsec = 0;
}

void ApexClimb::finishRun(Audio& audio, Storage& storage)
{
    const uint32_t now = Platform::getMsec();
    if (drifting || currentDriftScore > 0) finishDriftScore(now);

    finishMsec = now - runStartMsec;
    finishEffectStartMsec = now;
    finishRank = insertRanking(finishMsec);
    finishDriftRank = bestDriftScore > 0 ? insertDriftRanking(bestDriftScore) : 0;
    bestMsec = rankings[currentCourse][0];

    if (finishRank > 0 || finishDriftRank > 0) {
        saveDirty = true;
        saveRanking(storage);
    }

    // Keep the car on screen after crossing the line. The result is shown
    // only after the automatic braking sequence has brought it to a stop.
    throttleInput = 0.0f;
    brakeInput = 1.0f;
    finishStopped = false;
    finishStopMsec = 0;
    mode = MODE_FINISHING;
}

void ApexClimb::updateFinishing(Audio& audio, float deltaSec)
{
    throttleInput = 0.0f;
    brakeInput = 1.0f;
    steering = Math::moveTowards(steering, 0.0f, 4.0f * deltaSec);

    float speed = velocity.length();
    if (speed > 0.01f) {
        // Smooth post-finish braking instead of an abrupt stop.
        const float newSpeed = Math::moveTowards(speed, 0.0f, 72.0f * deltaSec);
        if (newSpeed <= 0.01f) {
            velocity = Vector2();
        } else {
            velocity *= newSpeed / speed;
        }
    }

    carPosition += velocity * deltaSec * presentation.movementScale;
    carPosition.x = Math::clamp(carPosition.x, 8.0f, static_cast<float>(WORLD_W - 8));
    carPosition.y = Math::clamp(carPosition.y, 8.0f, static_cast<float>(WORLD_H - 8));

    progress = getTrackProgress(carPosition.x, carPosition.y);
    updateEngineAudio(audio, deltaSec);

    if (velocity.length() <= 0.8f) {
        velocity = Vector2();
        brakeInput = 0.0f;

        if (!finishStopped) {
            finishStopped = true;
            finishStopMsec = Platform::getMsec();
        }

        if (Platform::elapsed(Platform::getMsec(), finishStopMsec, 650)) {
            audio.playSE(&Audio::SE::NO_4, 0.65f);
            engineSoundMuteUntilMsec = Platform::getMsec() + 800;
            engineSoundIndex = -1;
            engineSoundElapsed = 0.0f;
            mode = MODE_RESULT;
        }
    }
}

void ApexClimb::loadRanking(Storage& storage)
{
    (void)storage;

    for (uint8_t c = 0; c < COURSE_COUNT; ++c) {
        for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
            char key[20];
            std::snprintf(key, sizeof(key), "%s_%u", COURSES[c].id, static_cast<unsigned>(i + 1));
            rankings[c][i] = saveData.getUInt32(key, 0);

            std::snprintf(key, sizeof(key), "%s_D%u", COURSES[c].id, static_cast<unsigned>(i + 1));
            driftRankings[c][i] = saveData.getUInt32(key, 0);
        }
    }

    // Migrate the old one-course BEST into APEX once.
    if (rankings[0][0] == 0) {
        const uint32_t legacyBest = saveData.getUInt32("BEST", 0);
        if (legacyBest > 0) rankings[0][0] = legacyBest;
    }

    bestMsec = rankings[currentCourse][0];
}

void ApexClimb::saveRanking(Storage& storage)
{
    if (!storage.isAvailable()) return;

    for (uint8_t c = 0; c < COURSE_COUNT; ++c) {
        for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
            char key[20];
            std::snprintf(key, sizeof(key), "%s_%u", COURSES[c].id, static_cast<unsigned>(i + 1));
            saveData.setUInt32(key, rankings[c][i]);

            std::snprintf(key, sizeof(key), "%s_D%u", COURSES[c].id, static_cast<unsigned>(i + 1));
            saveData.setUInt32(key, driftRankings[c][i]);
        }
    }

    // Keep legacy BEST mapped to APEX for compatibility.
    saveData.setUInt32("BEST", rankings[0][0]);
    saveData.save(storage, getId(), "save.ini");
    saveDirty = false;
}

uint8_t ApexClimb::insertRanking(uint32_t msec)
{
    uint32_t* ranking = rankings[currentCourse];

    for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
        if (ranking[i] == 0 || msec < ranking[i]) {
            for (int j = RANKING_COUNT - 1; j > i; --j) {
                ranking[j] = ranking[j - 1];
            }
            ranking[i] = msec;
            return static_cast<uint8_t>(i + 1);
        }
    }
    return 0;
}

uint8_t ApexClimb::insertDriftRanking(uint32_t score)
{
    uint32_t* ranking = driftRankings[currentCourse];

    for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
        if (ranking[i] == 0 || score > ranking[i]) {
            for (int j = RANKING_COUNT - 1; j > i; --j) ranking[j] = ranking[j - 1];
            ranking[i] = score;
            return static_cast<uint8_t>(i + 1);
        }
    }
    return 0;
}

float ApexClimb::distanceToSegmentSquared(float px, float py,
                                             const Point& a, const Point& b)
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len2 = dx * dx + dy * dy;
    float t = len2 > 0.0f
        ? ((px - a.x) * dx + (py - a.y) * dy) / len2
        : 0.0f;
    t = Math::clamp(t, 0.0f, 1.0f);
    const float x = a.x + dx * t;
    const float y = a.y + dy * t;
    const float ex = px - x;
    const float ey = py - y;
    return ex * ex + ey * ey;
}

ApexClimb::Point ApexClimb::getCoursePoint(float t) const
{
    const CourseDef& course = COURSES[currentCourse];
    const P* points = course.points;
    const int count = course.pointCount;

    t = Math::clamp(t, 0.0f, 1.0f);
    const float scaled = t * static_cast<float>(count - 1);
    const int i1 = Math::clamp(
        static_cast<int>(scaled), 0, count - 2);
    const int i2 = i1 + 1;
    const int i0 = i1 > 0 ? i1 - 1 : i1;
    const int i3 = i2 + 1 < count ? i2 + 1 : i2;
    const float localT = scaled - static_cast<float>(i1);

    auto catmull = [&](float p0, float p1, float p2, float p3) {
        const float t2 = localT * localT;
        const float t3 = t2 * localT;
        return 0.5f * (
            2.0f * p1 +
            (-p0 + p2) * localT +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    };

    return {
        catmull(points[i0].x, points[i1].x, points[i2].x, points[i3].x),
        catmull(points[i0].y, points[i1].y, points[i2].y, points[i3].y)
    };
}

ApexClimb::Point ApexClimb::getCourseTangent(float t) const
{
    const Point a = getCoursePoint((t - 0.004f > 0.0f ? t - 0.004f : 0.0f));
    const Point b = getCoursePoint((t + 0.004f < 1.0f ? t + 0.004f : 1.0f));
    return { b.x - a.x, b.y - a.y };
}

float ApexClimb::getTrackProgress(float x, float y) const
{
    float bestDistance = 1.0e30f;
    float bestT = 0.0f;

    Point previous = getCoursePoint(0.0f);
    for (uint16_t i = 1; i <= TRACK_SAMPLES; ++i) {
        const float t = static_cast<float>(i) / TRACK_SAMPLES;
        const Point current = getCoursePoint(t);

        const float dx = current.x - previous.x;
        const float dy = current.y - previous.y;
        const float len2 = dx * dx + dy * dy;
        float u = len2 > 0.0f
            ? ((x - previous.x) * dx + (y - previous.y) * dy) / len2
            : 0.0f;
        u = Math::clamp(u, 0.0f, 1.0f);

        const float px = previous.x + dx * u;
        const float py = previous.y + dy * u;
        const float ex = x - px;
        const float ey = y - py;
        const float d = ex * ex + ey * ey;

        if (d < bestDistance) {
            bestDistance = d;
            bestT = (static_cast<float>(i - 1) + u) / TRACK_SAMPLES;
        }
        previous = current;
    }
    return bestT;
}

float ApexClimb::distanceToTrack(float x, float y) const
{
    float best = 1.0e30f;
    Point previous = getCoursePoint(0.0f);

    for (uint16_t i = 1; i <= TRACK_SAMPLES; ++i) {
        const Point current =
            getCoursePoint(static_cast<float>(i) / TRACK_SAMPLES);
        best = std::fmin(best, distanceToSegmentSquared(x, y, previous, current));
        previous = current;
    }
    return std::sqrt(best);
}

bool ApexClimb::onDraw(Graphics& graphics, bool requestFullRedraw)
{
    if (!requestFullRedraw && !dirty) return false;

    graphics.resetCamera();
    graphics.resetClipRect();

    if (mode == MODE_TITLE) {
        drawTitle(graphics);
        dirty = false;
        return true;
    }
    if (mode == MODE_COURSE_SELECT) {
        drawCourseSelect(graphics);
        dirty = false;
        return true;
    }
    if (mode == MODE_RANKING) {
        drawRanking(graphics);
        dirty = false;
        return true;
    }
    if (mode == MODE_TUNING) {
        drawTuning(graphics);
        dirty = false;
        return true;
    }

    Graphics::Camera camera;
    camera.x = static_cast<int16_t>(Math::clamp(
        carPosition.x - VIEW_W * 0.5f,
        0.0f, static_cast<float>(WORLD_W - VIEW_W)));
    camera.y = static_cast<int16_t>(Math::clamp(
        carPosition.y - VIEW_H * 0.5f,
        0.0f, static_cast<float>(WORLD_H - VIEW_H)));
    camera.zoom = 1.0f;
    camera.zoomCenterX = VIEW_W / 2;
    camera.zoomCenterY = VIEW_H / 2;
    graphics.setCamera(camera);

    if (freePractice) drawPracticeWorld(graphics);
    else drawWorld(graphics);
    drawCar(graphics);

    graphics.resetCamera();
    graphics.resetClipRect();

    const uint32_t now = Platform::getMsec();
    if (freePractice) {
        drawPracticeHud(graphics);
    } else {
        drawMiniMap(graphics);
        drawTelemetry(graphics, now);
    }
    drawDriftScore(graphics, now);

    if (mode == MODE_READY) drawReady(graphics, now);
    else if (mode == MODE_RESULT) drawResult(graphics);
    else if (mode == MODE_PAUSED) drawPause(graphics);

    dirty = false;
    return true;
}

void ApexClimb::drawPracticeWorld(Graphics& g) const
{
    const Graphics::Color asphalt = Graphics::rgb565(61, 63, 65);
    const Graphics::Color asphaltDark = Graphics::rgb565(51, 53, 55);
    const Graphics::Color line = Graphics::rgb565(112, 114, 112);
    const Graphics::Color cone = Graphics::rgb565(245, 112, 18);

    g.fillRect(40, 2750, 900, 1200, asphalt);
    g.drawRect(40, 2750, 900, 1200, asphaltDark);

    // Sparse, symmetric markings make the open space easier to read around the centered spawn.
    g.drawLine(120, 2880, 120, 3820, line);
    g.drawLine(860, 2880, 860, 3820, line);
    g.drawLine(280, 2820, 700, 2820, line);
    g.drawLine(280, 3880, 700, 3880, line);

    static const Point cones[] = {
        {420, 3260}, {560, 3260},                 // Figure-eight pair.
        {490, 3060}, {490, 3160}, {490, 3360},   // Short slalom.
        {350, 3560}, {490, 3600}, {630, 3560}    // Open practice markers.
    };

    for (const Point& p : cones) {
        g.fillTriangle(static_cast<int16_t>(p.x), static_cast<int16_t>(p.y - 7),
                       static_cast<int16_t>(p.x - 5), static_cast<int16_t>(p.y + 5),
                       static_cast<int16_t>(p.x + 5), static_cast<int16_t>(p.y + 5), cone);
        g.drawLine(static_cast<int16_t>(p.x - 4), static_cast<int16_t>(p.y + 1),
                   static_cast<int16_t>(p.x + 4), static_cast<int16_t>(p.y + 1), Graphics::WHITE);
    }

    // Reuse the normal skid marks so repeated practice leaves visible traces.
    for (uint8_t i = 0; i < SKID_POINT_COUNT; ++i) {
        const SkidPoint& skid = skidPoints[i];
        if (!skid.active) continue;
        g.fillCircle(static_cast<int16_t>(skid.left.x), static_cast<int16_t>(skid.left.y), 1, Graphics::DARKGRAY);
        g.fillCircle(static_cast<int16_t>(skid.right.x), static_cast<int16_t>(skid.right.y), 1, Graphics::DARKGRAY);
    }
}

void ApexClimb::drawPracticeHud(Graphics& g) const
{
    // Practice keeps the useful driving information, but omits time, sectors, minimap and saved ranking.
    g.fillRect(VIEW_W, 0, SCREEN_W - VIEW_W, SCREEN_H, PANEL);
    g.drawLine(VIEW_W, 0, VIEW_W, SCREEN_H, PANEL_LINE);

    g.drawString("FREE", 246, 8, ACCENT, Graphics::SIZE_10);
    g.drawString("PRACTICE", 312, 20, Graphics::WHITE, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);

    char text[24];
    g.drawString("BEST DRIFT", 246, 47, Graphics::LIGHTGRAY, Graphics::SIZE_10);
    if (bestDriftScore >= DRIFT_DISPLAY_THRESHOLD) {
        std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(bestDriftScore));
        g.drawString(text, 312, 60, Graphics::CYAN, Graphics::SIZE_13,
                     Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);
    } else {
        g.drawString("----", 312, 60, Graphics::DARKGRAY, Graphics::SIZE_13,
                     Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);
    }

    g.drawLine(246, 82, 312, 82, Graphics::DARKGRAY);

    const char* handling = handlingState == HANDLING_DRIFT ? "DRIFT" :
                           handlingState == HANDLING_UNDER ? "UNDER" : "GRIP";
    const Graphics::Color handlingColor = handlingState == HANDLING_DRIFT ? Graphics::YELLOW :
                                          handlingState == HANDLING_UNDER ? Graphics::CYAN : Graphics::LIGHTGRAY;

    g.drawString("STATE", 246, 91, Graphics::LIGHTGRAY, Graphics::SIZE_10);
    g.drawString(handling, 312, 91, handlingColor, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);

    auto drawInputBar = [&](const char* label, int16_t y, float value, Graphics::Color color, bool centered) {
        g.drawString(label, 246, y, Graphics::LIGHTGRAY, Graphics::SIZE_10);
        g.drawRect(274, y + 2, 38, 5, Graphics::DARKGRAY);

        if (centered) {
            const int16_t px = static_cast<int16_t>(Math::clamp(value, -1.0f, 1.0f) * 18.0f);
            g.drawLine(293, y + 2, 293, y + 6, Graphics::GRAY);
            if (px < 0) g.fillRect(293 + px, y + 3, -px, 3, color);
            else if (px > 0) g.fillRect(294, y + 3, px, 3, color);
        } else {
            const int16_t px = static_cast<int16_t>(Math::clamp(value, 0.0f, 1.0f) * 36.0f);
            if (px > 0) g.fillRect(275, y + 3, px, 3, color);
        }
    };

    drawInputBar("THR", 116, throttleInput, Graphics::GREEN, false);
    drawInputBar("BRK", 133, brakeInput, Graphics::RED, false);
    drawInputBar("STR", 150, steering, Graphics::CYAN, true);

    static constexpr int16_t TACH_X = 207;
    static constexpr int16_t TACH_Y = 204;
    static constexpr uint16_t TACH_R = 30;

    g.fillCircle(TACH_X, TACH_Y, TACH_R, Graphics::BLACK);
    g.drawCircle(TACH_X, TACH_Y, TACH_R, Graphics::WHITE);

    const float rpmT = Math::clamp((displayRpm - 1600.0f) / (9000.0f - 1600.0f), 0.0f, 1.0f);
    const float needleAngle = Math::degToRad(135.0f + rpmT * 270.0f);
    const int16_t needleX = static_cast<int16_t>(TACH_X + Math::cos(needleAngle) * (TACH_R - 5));
    const int16_t needleY = static_cast<int16_t>(TACH_Y + Math::sin(needleAngle) * (TACH_R - 5));
    g.drawLine(TACH_X, TACH_Y, needleX, needleY, Graphics::RED);
    g.fillCircle(TACH_X, TACH_Y, 18, Graphics::BLACK);

    std::snprintf(text, sizeof(text), "%d", static_cast<int>(getDisplaySpeedKmh() + 0.5f));
    g.drawString(text, TACH_X, TACH_Y - 8, Graphics::WHITE, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(virtualGear));
    g.drawString(text, TACH_X, TACH_Y + 10, Graphics::YELLOW, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void ApexClimb::drawWorld(Graphics& g) const
{
    const CourseTheme theme = COURSES[currentCourse].theme;

    if (theme == THEME_SKYLINE) {
        const Graphics::Color GRASS_LOW = Graphics::rgb565(119, 126, 64);
        const Graphics::Color GRASS_HIGH = Graphics::rgb565(151, 143, 76);
        g.fillRectGradient(0, 0, WORLD_W, WORLD_H, GRASS_HIGH, GRASS_LOW, Graphics::VERTICAL_LINEAR);
    } else if (theme == THEME_HAIRPINS) {
        const Graphics::Color NIGHT_TOP = Graphics::rgb565(7, 11, 24);
        const Graphics::Color NIGHT_LOW = Graphics::rgb565(18, 25, 34);
        g.fillRectGradient(0, 0, WORLD_W, WORLD_H, NIGHT_TOP, NIGHT_LOW, Graphics::VERTICAL_LINEAR);
    } else if (theme == THEME_TOUGE) {
        const Graphics::Color AUTUMN_TOP = Graphics::rgb565(83, 69, 39);
        const Graphics::Color AUTUMN_LOW = Graphics::rgb565(51, 73, 38);
        g.fillRectGradient(0, 0, WORLD_W, WORLD_H, AUTUMN_TOP, AUTUMN_LOW, Graphics::VERTICAL_LINEAR);
    } else {
        const Graphics::Color LOW =
            Graphics::rgb565(33, 78, 37);
        const Graphics::Color MID =
            Graphics::rgb565(45, 82, 42);
        const Graphics::Color HIGH =
            Graphics::rgb565(82, 80, 68);

        g.fillRect(0, 1640, WORLD_W, WORLD_H - 1640, LOW);
        g.fillRectGradient(
            0, 1560, WORLD_W, 80, MID, LOW,
            Graphics::VERTICAL_LINEAR);
        g.fillRect(0, 760, WORLD_W, 800, MID);
        g.fillRectGradient(
            0, 680, WORLD_W, 80, HIGH, MID,
            Graphics::VERTICAL_LINEAR);
        g.fillRect(0, 0, WORLD_W, 680, HIGH);
    }

    drawScenery(g);
    drawRoad(g);

    if (theme == THEME_HAIRPINS) drawStreetLights(g);

    drawSectorGate(g, COURSES[currentCourse].sector1T, Graphics::CYAN);
    drawSectorGate(g, COURSES[currentCourse].sector2T, Graphics::YELLOW);
    drawSectorGate(g, COURSES[currentCourse].startT, Graphics::WHITE);
    drawSectorGate(g, COURSES[currentCourse].finishT, Graphics::WHITE);

    const Point s = getCoursePoint(COURSES[currentCourse].startT);
    g.drawString("START", static_cast<int16_t>(s.x),
                 static_cast<int16_t>(s.y + 30),
                 Graphics::WHITE, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);

    drawFinishVenue(g);
    drawFinishConfetti(g);
}

void ApexClimb::drawStreetLights(Graphics& g) const
{
    if (COURSES[currentCourse].theme != THEME_HAIRPINS) return;

    static const float LIGHT_T[] = {
        0.085f, 0.155f, 0.245f, 0.335f, 0.430f, 0.535f, 0.635f, 0.745f, 0.845f, 0.905f
    };

    const Graphics::Color poleColor = Graphics::rgb565(88, 91, 94);
    const Graphics::Color lampColor = Graphics::rgb565(255, 230, 150);
    const Graphics::Color roadGlow = Graphics::rgb565(96, 91, 68);

    for (uint8_t i = 0; i < sizeof(LIGHT_T) / sizeof(LIGHT_T[0]); ++i) {
        const float t = LIGHT_T[i];
        const Point p = getCoursePoint(t);
        Point tangent = getCourseTangent(t);
        const float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
        if (len <= 0.001f) continue;

        tangent.x /= len;
        tangent.y /= len;
        const float nx = -tangent.y;
        const float ny = tangent.x;
        const float side = (i & 1U) ? 1.0f : -1.0f;
        const float offset = SHOULDER_HALF_WIDTH + 11.0f;

        const int16_t x = static_cast<int16_t>(p.x + nx * offset * side);
        const int16_t y = static_cast<int16_t>(p.y + ny * offset * side);
        const int16_t roadX = static_cast<int16_t>(p.x + nx * (ROAD_HALF_WIDTH - 7.0f) * side);
        const int16_t roadY = static_cast<int16_t>(p.y + ny * (ROAD_HALF_WIDTH - 7.0f) * side);

        // A small warm patch suggests light on the asphalt without drawing an opaque cone.
        g.fillCircle(roadX, roadY, 8, roadGlow);
        g.drawLine(x, y + 8, x, y - 7, poleColor);
        g.drawLine(x, y - 7, x - static_cast<int16_t>(nx * side * 5.0f), y - 7, poleColor);
        g.fillCircle(x - static_cast<int16_t>(nx * side * 5.0f), y - 7, 2, lampColor);
    }
}

void ApexClimb::drawFinishVenue(Graphics& g) const
{
    const float t = COURSES[currentCourse].finishT;
    const Point p = getCoursePoint(t);
    Point tangent = getCourseTangent(t);
    const float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    if (len <= 0.001f) return;

    tangent.x /= len;
    tangent.y /= len;
    const float nx = -tangent.y;
    const float ny = tangent.x;
    const Graphics::Color accent = COURSES[currentCourse].theme == THEME_HAIRPINS ? Graphics::CYAN : Graphics::ORANGE;
    const float gateHalf = ROAD_HALF_WIDTH + 12.0f;

    static constexpr int CHECKS = 10;
    for (int i = 0; i < CHECKS; ++i) {
        const float a = -ROAD_HALF_WIDTH + ROAD_HALF_WIDTH * 2.0f * i / CHECKS;
        const float b = -ROAD_HALF_WIDTH + ROAD_HALF_WIDTH * 2.0f * (i + 1) / CHECKS;
        const Graphics::Color c = (i & 1) ? Graphics::BLACK : Graphics::WHITE;
        g.drawWideLine(static_cast<int16_t>(p.x + nx * a), static_cast<int16_t>(p.y + ny * a),
                       static_cast<int16_t>(p.x + nx * b), static_cast<int16_t>(p.y + ny * b), 4, c);
    }

    const Point left = {p.x + nx * gateHalf, p.y + ny * gateHalf};
    const Point right = {p.x - nx * gateHalf, p.y - ny * gateHalf};
    g.fillCircle(static_cast<int16_t>(left.x), static_cast<int16_t>(left.y), 6, accent);
    g.fillCircle(static_cast<int16_t>(right.x), static_cast<int16_t>(right.y), 6, accent);
    g.drawWideLine(static_cast<int16_t>(left.x), static_cast<int16_t>(left.y), static_cast<int16_t>(right.x),
                   static_cast<int16_t>(right.y), 3, Graphics::WHITE);

    const Point banner = {p.x - tangent.x * 18.0f, p.y - tangent.y * 18.0f};
    g.fillRect(static_cast<int16_t>(banner.x) - 25, static_cast<int16_t>(banner.y) - 7, 50, 14, Graphics::BLACK);
    g.drawRect(static_cast<int16_t>(banner.x) - 25, static_cast<int16_t>(banner.y) - 7, 50, 14, accent);
    g.drawString("FINISH", static_cast<int16_t>(banner.x), static_cast<int16_t>(banner.y), Graphics::WHITE, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    for (int sideSign = -1; sideSign <= 1; sideSign += 2) {
        for (int i = 0; i < 4; ++i) {
            const float along = 18.0f + i * 14.0f;
            const float across = (ROAD_HALF_WIDTH + 25.0f + (i & 1) * 6.0f) * sideSign;
            const float x = p.x - tangent.x * along + nx * across;
            const float y = p.y - tangent.y * along + ny * across;
            g.fillCircle(static_cast<int16_t>(x), static_cast<int16_t>(y), 3, i & 1 ? Graphics::YELLOW : Graphics::WHITE);
        }

        const float bx = p.x + tangent.x * 24.0f + nx * (ROAD_HALF_WIDTH + 28.0f) * sideSign;
        const float by = p.y + tangent.y * 24.0f + ny * (ROAD_HALF_WIDTH + 28.0f) * sideSign;
        g.fillRect(static_cast<int16_t>(bx) - 7, static_cast<int16_t>(by) - 3, 14, 6, accent);
    }
}

void ApexClimb::drawFinishConfetti(Graphics& g) const
{
    if (finishEffectStartMsec == 0) return;

    const uint32_t elapsed = Platform::getMsec() - finishEffectStartMsec;
    if (elapsed > 1800) return;

    const float phase = static_cast<float>(elapsed) / 1800.0f;
    const float t = COURSES[currentCourse].finishT;
    const Point p = getCoursePoint(t);
    Point tangent = getCourseTangent(t);
    const float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    if (len <= 0.001f) return;

    tangent.x /= len;
    tangent.y /= len;
    const float nx = -tangent.y;
    const float ny = tangent.x;
    static const Graphics::Color colors[] = {Graphics::WHITE, Graphics::YELLOW, Graphics::CYAN, Graphics::ORANGE};

    for (int i = 0; i < 24; ++i) {
        const float side = (i & 1) ? 1.0f : -1.0f;
        const float spread = 18.0f + static_cast<float>((i * 17) % 42);
        const float forward = static_cast<float>((i * 23) % 55) - 20.0f;
        const float drift = phase * 24.0f + static_cast<float>((i * 7) % 11);
        const float x = p.x + nx * side * spread + tangent.x * (forward + drift);
        const float y = p.y + ny * side * spread + tangent.y * (forward + drift) + phase * phase * 22.0f;
        g.fillRect(static_cast<int16_t>(x), static_cast<int16_t>(y), 2, 2, colors[i & 3]);
    }
}

void ApexClimb::drawRoad(Graphics& g) const
{
    // Use the same world-space bounds as the active camera.
    // Road segments completely outside this rectangle are not submitted
    // to Graphics::fillTriangle().
    const float viewLeft = Math::clamp(
        carPosition.x - VIEW_W * 0.5f,
        0.0f, static_cast<float>(WORLD_W - VIEW_W));
    const float viewTop = Math::clamp(
        carPosition.y - VIEW_H * 0.5f,
        0.0f, static_cast<float>(WORLD_H - VIEW_H));
    const float viewRight = viewLeft + VIEW_W;
    const float viewBottom = viewTop + VIEW_H;

    Point samples[TRACK_SAMPLES + 1];
    Point miters[TRACK_SAMPLES + 1];
    float miterScale[TRACK_SAMPLES + 1];

    for (uint16_t i = 0; i <= TRACK_SAMPLES; ++i) {
        samples[i] = getCoursePoint(
            static_cast<float>(i) / TRACK_SAMPLES);
    }

    for (uint16_t i = 0; i <= TRACK_SAMPLES; ++i) {
        const uint16_t prev = i > 0 ? i - 1 : i;
        const uint16_t next = i < TRACK_SAMPLES ? i + 1 : i;

        float inX = samples[i].x - samples[prev].x;
        float inY = samples[i].y - samples[prev].y;
        float outX = samples[next].x - samples[i].x;
        float outY = samples[next].y - samples[i].y;

        float inLen = std::sqrt(inX * inX + inY * inY);
        float outLen = std::sqrt(outX * outX + outY * outY);

        if (inLen <= 0.001f && outLen > 0.001f) {
            inX = outX; inY = outY; inLen = outLen;
        }
        if (outLen <= 0.001f && inLen > 0.001f) {
            outX = inX; outY = inY; outLen = inLen;
        }

        if (inLen <= 0.001f || outLen <= 0.001f) {
            miters[i] = {0.0f, 1.0f};
            miterScale[i] = 1.0f;
            continue;
        }

        inX /= inLen; inY /= inLen;
        outX /= outLen; outY /= outLen;

        const float n0x = -inY;
        const float n0y =  inX;
        const float n1x = -outY;
        const float n1y =  outX;

        float mx = n0x + n1x;
        float my = n0y + n1y;
        const float mlen = std::sqrt(mx * mx + my * my);

        if (mlen > 0.001f) {
            mx /= mlen;
            my /= mlen;
        } else {
            mx = n1x;
            my = n1y;
        }

        float denom = mx * n1x + my * n1y;
        if (std::fabs(denom) < 0.30f) {
            denom = denom < 0.0f ? -0.30f : 0.30f;
        }

        float scale = 1.0f / denom;
        if (scale < 0.0f) scale = -scale;
        if (scale > 1.40f) scale = 1.40f;

        miters[i] = {mx, my};
        miterScale[i] = scale;
    }

    auto drawRibbon = [&](float halfWidth, Graphics::Color color)
    {
        for (uint16_t i = 0; i < TRACK_SAMPLES; ++i) {
            const Point& a = samples[i];
            const Point& b = samples[i + 1];

            const float aw = halfWidth * miterScale[i];
            const float bw = halfWidth * miterScale[i + 1];

            const int16_t aLx = static_cast<int16_t>(a.x + miters[i].x * aw);
            const int16_t aLy = static_cast<int16_t>(a.y + miters[i].y * aw);
            const int16_t aRx = static_cast<int16_t>(a.x - miters[i].x * aw);
            const int16_t aRy = static_cast<int16_t>(a.y - miters[i].y * aw);

            const int16_t bLx = static_cast<int16_t>(b.x + miters[i + 1].x * bw);
            const int16_t bLy = static_cast<int16_t>(b.y + miters[i + 1].y * bw);
            const int16_t bRx = static_cast<int16_t>(b.x - miters[i + 1].x * bw);
            const int16_t bRy = static_cast<int16_t>(b.y - miters[i + 1].y * bw);

            // Cull the whole quad (two triangles) when all four vertices
            // are on the same outside side of the camera rectangle.
            const int16_t minX = std::min(std::min(aLx, aRx), std::min(bLx, bRx));
            const int16_t maxX = std::max(std::max(aLx, aRx), std::max(bLx, bRx));
            const int16_t minY = std::min(std::min(aLy, aRy), std::min(bLy, bRy));
            const int16_t maxY = std::max(std::max(aLy, aRy), std::max(bLy, bRy));

            if (maxX < viewLeft || minX > viewRight ||
                maxY < viewTop || minY > viewBottom) {
                continue;
            }

            g.fillTriangle(aLx, aLy, aRx, aRy, bLx, bLy, color);
            g.fillTriangle(aRx, aRy, bRx, bRy, bLx, bLy, color);
        }
    };

    drawRibbon(SHOULDER_HALF_WIDTH, SHOULDER);
    drawRibbon(ROAD_HALF_WIDTH, ROAD);

    for (uint16_t i = 8; i < 50 && i <= TRACK_SAMPLES; i += 7) {
        const Point& p = samples[i];
        if (p.x < viewLeft || p.x > viewRight ||
            p.y < viewTop || p.y > viewBottom) {
            continue;
        }
        g.fillCircle(
            static_cast<int16_t>(p.x),
            static_cast<int16_t>(p.y),
            1, ROAD_EDGE);
    }
}

void ApexClimb::prepareScenery()
{
    const CourseTheme theme = COURSES[currentCourse].theme;

    for (uint16_t i = 0; i < SUSUKI_COUNT; ++i) susukiEnabled[i] = false;
    for (uint16_t i = 0; i < TREE_COUNT_MAX; ++i) treeEnabled[i] = false;
    for (uint16_t i = 0; i < ROCK_COUNT; ++i) rockEnabled[i] = false;

    if (theme == THEME_SKYLINE) {
        for (uint16_t i = 0; i < SUSUKI_COUNT; ++i) {
            const int16_t x = static_cast<int16_t>(18 + (i * 89) % 945);
            const int16_t y = static_cast<int16_t>(35 + (i * 137) % (WORLD_H - 80));
            susukiEnabled[i] = distanceToTrack(x, y) >= SHOULDER_HALF_WIDTH + 10.0f;
        }
    } else {
        const uint16_t treeCount = theme == THEME_TOUGE ? 82 : theme == THEME_HAIRPINS ? 54 : 64;
        for (uint16_t i = 0; i < treeCount; ++i) {
            const int16_t x = static_cast<int16_t>(25 + (i * 113) % 925);
            const int16_t y = static_cast<int16_t>(60 + (i * 149) % 4060);
            treeEnabled[i] = distanceToTrack(x, y) >= SHOULDER_HALF_WIDTH + 13.0f;
        }

        for (uint16_t i = 0; i < ROCK_COUNT; ++i) {
            const int16_t x = static_cast<int16_t>(30 + (i * 157) % 920);
            const int16_t y = static_cast<int16_t>(40 + (i * 97) % 4100);
            rockEnabled[i] = distanceToTrack(x, y) >= SHOULDER_HALF_WIDTH + 16.0f;
        }
    }

    float minX = 1.0e30f, minY = 1.0e30f, maxX = -1.0e30f, maxY = -1.0e30f;
    Point miniPoints[MINIMAP_SAMPLES + 1];

    for (uint8_t i = 0; i <= MINIMAP_SAMPLES; ++i) {
        miniPoints[i] = getCoursePoint(static_cast<float>(i) / MINIMAP_SAMPLES);
        minX = std::fmin(minX, miniPoints[i].x);
        minY = std::fmin(minY, miniPoints[i].y);
        maxX = std::fmax(maxX, miniPoints[i].x);
        maxY = std::fmax(maxY, miniPoints[i].y);
    }

    const float w = maxX - minX > 1.0f ? maxX - minX : 1.0f;
    const float h = maxY - minY > 1.0f ? maxY - minY : 1.0f;
    const float scale = std::fmin(56.0f / w, 56.0f / h);

    for (uint8_t i = 0; i <= MINIMAP_SAMPLES; ++i) {
        miniMapX[i] = static_cast<int16_t>((miniPoints[i].x - minX) * scale + 0.5f);
        miniMapY[i] = static_cast<int16_t>((miniPoints[i].y - minY) * scale + 0.5f);
    }

    sceneryPrepared = true;
}

void ApexClimb::drawScenery(Graphics& g) const
{
    const CourseTheme theme = COURSES[currentCourse].theme;
    const float viewLeft = Math::clamp(carPosition.x - VIEW_W * 0.5f, 0.0f, static_cast<float>(WORLD_W - VIEW_W));
    const float viewTop = Math::clamp(carPosition.y - VIEW_H * 0.5f, 0.0f, static_cast<float>(WORLD_H - VIEW_H));
    const float viewRight = viewLeft + VIEW_W;
    const float viewBottom = viewTop + VIEW_H;

    auto visible = [&](float x, float y, float margin) {
        return x >= viewLeft - margin && x <= viewRight + margin && y >= viewTop - margin && y <= viewBottom + margin;
    };

    if (theme == THEME_SKYLINE) {
        for (uint16_t i = 0; i < SUSUKI_COUNT; ++i) {
            if (!sceneryPrepared || !susukiEnabled[i]) continue;

            const int16_t x = static_cast<int16_t>(18 + (i * 89) % 945);
            const int16_t y = static_cast<int16_t>(35 + (i * 137) % (WORLD_H - 80));
            if (!visible(x, y, 10.0f)) continue;

            const int16_t lean = static_cast<int16_t>((i % 5) - 2);
            const Graphics::Color stem = Graphics::rgb565(178, 165, 104);
            const Graphics::Color head = Graphics::rgb565(220, 210, 160);
            g.drawLine(x, y + 5, x + lean, y - 5, stem);
            g.drawLine(x + lean - 2, y - 5, x + lean + 2, y - 7, head);
        }
    } else {
        const uint16_t treeCount = theme == THEME_TOUGE ? 82 : theme == THEME_HAIRPINS ? 54 : 64;

        for (uint16_t i = 0; i < treeCount; ++i) {
            if (!sceneryPrepared || !treeEnabled[i]) continue;

            const int16_t x = static_cast<int16_t>(25 + (i * 113) % 925);
            const int16_t y = static_cast<int16_t>(60 + (i * 149) % 4060);
            if (!visible(x, y, 10.0f)) continue;

            const uint16_t crown = theme == THEME_HAIRPINS ? 4 : 5;
            Graphics::Color treeColor = Graphics::rgb565(24, 66, 32);
            if (theme == THEME_HAIRPINS) treeColor = Graphics::rgb565(12, 25, 22);
            else if (theme == THEME_TOUGE) {
                static const Graphics::Color AUTUMN[] = {
                    Graphics::rgb565(190, 72, 35), Graphics::rgb565(218, 123, 32), Graphics::rgb565(177, 43, 28),
                    Graphics::rgb565(204, 160, 45), Graphics::rgb565(91, 106, 42)
                };
                treeColor = AUTUMN[i % 5];
            }
            g.fillCircle(x, y, crown, treeColor);
            g.fillRect(x - 1, y + crown - 1, 3, 5, Graphics::rgb565(90, 65, 42));
        }

        for (uint16_t i = 0; i < ROCK_COUNT; ++i) {
            if (!sceneryPrepared || !rockEnabled[i]) continue;

            const int16_t x = static_cast<int16_t>(30 + (i * 157) % 920);
            const int16_t y = static_cast<int16_t>(40 + (i * 97) % 4100);
            if (!visible(x, y, 10.0f)) continue;

            g.drawLine(x - 6, y + 4, x, y, ROCK);
            g.drawLine(x, y, x + 7, y + 3, ROCK_LIGHT);
        }
    }

    const uint16_t delimiterStep = theme == THEME_SKYLINE ? 12 : 16;
    for (uint16_t i = 18; i < TRACK_SAMPLES - 12; i += delimiterStep) {
        const float t = static_cast<float>(i) / TRACK_SAMPLES;
        const Point p = getCoursePoint(t);
        if (!visible(p.x, p.y, SHOULDER_HALF_WIDTH + 18.0f)) continue;

        Point tangent = getCourseTangent(t);
        const float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
        if (len <= 0.001f) continue;
        tangent.x /= len;
        tangent.y /= len;

        const float nx = -tangent.y;
        const float ny = tangent.x;
        const float side = ((i / delimiterStep) & 1) ? 1.0f : -1.0f;
        const float d = SHOULDER_HALF_WIDTH + 8.0f;
        const int16_t x = static_cast<int16_t>(p.x + nx * d * side);
        const int16_t y = static_cast<int16_t>(p.y + ny * d * side);

        if (!visible(x, y, 10.0f)) continue;
        const Graphics::Color postColor = theme == THEME_HAIRPINS ? Graphics::LIGHTGRAY : Graphics::WHITE;
        const Graphics::Color reflectorColor = theme == THEME_HAIRPINS ? Graphics::YELLOW : Graphics::RED;
        g.drawLine(x, y, x, y - 8, postColor);
        g.drawLine(x, y - 6, x, y - 4, reflectorColor);
    }

    uint8_t rangeCount = 0;
    const RailRange* ranges = getRailRanges(currentCourse, rangeCount);

    for (uint8_t r = 0; r < rangeCount; ++r) {
        const RailRange& range = ranges[r];
        Point previous{};
        bool havePrevious = false;

        static constexpr int SEGMENTS = 20;
        for (int j = 0; j <= SEGMENTS; ++j) {
            const float t = Math::lerp(range.from, range.to, static_cast<float>(j) / SEGMENTS);
            const Point p = getCoursePoint(t);
            Point tangent = getCourseTangent(t);
            const float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
            if (len <= 0.001f) {
                havePrevious = false;
                continue;
            }

            tangent.x /= len;
            tangent.y /= len;
            const float nx = -tangent.y;
            const float ny = tangent.x;
            const float d = SHOULDER_HALF_WIDTH + 6.0f;
            const Point rail = {p.x + nx * d * range.side, p.y + ny * d * range.side};

            if (havePrevious) {
                const float minX = std::fmin(previous.x, rail.x);
                const float maxX = std::fmax(previous.x, rail.x);
                const float minY = std::fmin(previous.y, rail.y);
                const float maxY = std::fmax(previous.y, rail.y);

                if (maxX >= viewLeft - 4.0f && minX <= viewRight + 4.0f && maxY >= viewTop - 4.0f && minY <= viewBottom + 4.0f) {
                    g.drawWideLine(static_cast<int16_t>(previous.x), static_cast<int16_t>(previous.y),
                                   static_cast<int16_t>(rail.x), static_cast<int16_t>(rail.y), 3, Graphics::LIGHTGRAY);
                }
            }

            if ((j % 4) == 0) {
                const int16_t sx = static_cast<int16_t>(rail.x + nx * 4.0f * range.side);
                const int16_t sy = static_cast<int16_t>(rail.y + ny * 4.0f * range.side);
                if (visible(sx, sy, 4.0f)) g.fillCircle(sx, sy, 2, Graphics::DARKGRAY);
            }

            previous = rail;
            havePrevious = true;
        }
    }

    static const float signT[] = {0.12f, 0.23f, 0.36f, 0.49f, 0.62f, 0.75f, 0.87f};
    for (float t : signT) {
        const Point p = getCoursePoint(t);
        if (!visible(p.x, p.y, SHOULDER_HALF_WIDTH + 30.0f)) continue;

        Point ta = getCourseTangent(t + 0.010f);
        Point tb = getCourseTangent(t + 0.050f);
        const float len = std::sqrt(ta.x * ta.x + ta.y * ta.y);
        if (len <= 0.001f) continue;

        ta.x /= len;
        ta.y /= len;
        const float nx = -ta.y;
        const float ny = ta.x;
        const int16_t x = static_cast<int16_t>(p.x + nx * (SHOULDER_HALF_WIDTH + 18.0f));
        const int16_t y = static_cast<int16_t>(p.y + ny * (SHOULDER_HALF_WIDTH + 18.0f));
        if (!visible(x, y, 12.0f)) continue;

        const bool right = ta.x * tb.y - ta.y * tb.x > 0.0f;
        g.fillRect(x - 7, y - 9, 14, 12, Graphics::WHITE);
        g.drawRect(x - 7, y - 9, 14, 12, Graphics::DARKGRAY);
        g.drawString(right ? ">" : "<", x, y - 8, Graphics::BLACK, Graphics::SIZE_10,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
        g.drawLine(x, y + 3, x, y + 9, Graphics::LIGHTGRAY);
    }
}

void ApexClimb::drawSectorGate(Graphics& g, float t,
                                  Graphics::Color color) const
{
    const Point p = getCoursePoint(t);
    Point tangent = getCourseTangent(t);
    const float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    if (len <= 0.001f) return;
    tangent.x /= len;
    tangent.y /= len;
    const float nx = -tangent.y;
    const float ny = tangent.x;

    g.drawLine(
        static_cast<int16_t>(p.x - nx * ROAD_HALF_WIDTH),
        static_cast<int16_t>(p.y - ny * ROAD_HALF_WIDTH),
        static_cast<int16_t>(p.x + nx * ROAD_HALF_WIDTH),
        static_cast<int16_t>(p.y + ny * ROAD_HALF_WIDTH),
        color);
}

void ApexClimb::drawCar(Graphics& g) const
{
    const Graphics::Color skidColor = Graphics::rgb565(42, 42, 42);
    for (const auto& p : skidPoints) {
        if (!p.active) continue;
        g.fillCircle((int16_t)p.left.x, (int16_t)p.left.y, 3, skidColor);
        g.fillCircle((int16_t)p.right.x, (int16_t)p.right.y, 3, skidColor);
    }

    if (skidLife > 0.0f) {
        g.drawLine(static_cast<int16_t>(previousSkidLeft.x),
                   static_cast<int16_t>(previousSkidLeft.y),
                   static_cast<int16_t>(skidLeft.x),
                   static_cast<int16_t>(skidLeft.y), Graphics::DARKGRAY);
        g.drawLine(static_cast<int16_t>(previousSkidRight.x),
                   static_cast<int16_t>(previousSkidRight.y),
                   static_cast<int16_t>(skidRight.x),
                   static_cast<int16_t>(skidRight.y), Graphics::DARKGRAY);
    }

    const Vector2 forward(Math::cos(carAngle), Math::sin(carAngle));
    const Vector2 side(-Math::sin(carAngle), Math::cos(carAngle));
    const Vector2 frontAxle = carPosition + forward * 7.6f;
    const Vector2 rearAxle = carPosition - forward * 7.4f;

    const float frontWheelAngle = carAngle + steering * 0.78f;
    const Vector2 frontWheelDir(Math::cos(frontWheelAngle),
                                Math::sin(frontWheelAngle));
    const Vector2 rearWheelDir = forward;

    auto drawWheel = [&](const Vector2& center,
                         const Vector2& wheelDir,
                         uint8_t tireHalfWidth)
    {
        const Vector2 wheelSide(-wheelDir.y, wheelDir.x);

        // Bright 1px outline separates black rubber from dark asphalt.
        for (int i = -static_cast<int>(tireHalfWidth) - 1;
             i <= static_cast<int>(tireHalfWidth) + 1; ++i) {
            const Vector2 o = wheelSide * static_cast<float>(i);
            g.drawLine(
                static_cast<int16_t>(center.x - wheelDir.x * 4.8f + o.x),
                static_cast<int16_t>(center.y - wheelDir.y * 4.8f + o.y),
                static_cast<int16_t>(center.x + wheelDir.x * 4.8f + o.x),
                static_cast<int16_t>(center.y + wheelDir.y * 4.8f + o.y),
                Graphics::GRAY);
        }
        for (int i = -tireHalfWidth; i <= tireHalfWidth; ++i) {
            const Vector2 o = wheelSide * static_cast<float>(i);
            g.drawLine(
                static_cast<int16_t>(center.x - wheelDir.x * 4.5f + o.x),
                static_cast<int16_t>(center.y - wheelDir.y * 4.5f + o.y),
                static_cast<int16_t>(center.x + wheelDir.x * 4.5f + o.x),
                static_cast<int16_t>(center.y + wheelDir.y * 4.5f + o.y),
                Graphics::BLACK);
        }
        g.fillCircle(static_cast<int16_t>(center.x),
                     static_cast<int16_t>(center.y), 1, Graphics::LIGHTGRAY);
    };

    drawWheel(frontAxle + side * 6.0f, frontWheelDir, 2);
    drawWheel(frontAxle - side * 6.0f, frontWheelDir, 2);
    drawWheel(rearAxle + side * 6.0f, rearWheelDir, 3);
    drawWheel(rearAxle - side * 6.0f, rearWheelDir, 3);

    Graphics::SpriteOptions options;
    options.angle = carAngle + Math::PI * 0.5f;
    options.transparent = true;
    g.drawSprite(
        CAR_BITMAP,
        static_cast<int16_t>(carPosition.x) - CAR_W / 2,
        static_cast<int16_t>(carPosition.y) - CAR_H / 2,
        CAR_W, CAR_H, options);

    // Thick hill-climb rear wing.
    const Vector2 wingCenter = carPosition - forward * 12.0f;
    for (int i = -1; i <= 1; ++i) {
        const Vector2 o = forward * static_cast<float>(i);
        g.drawLine(
            static_cast<int16_t>(wingCenter.x + side.x * 8.0f + o.x),
            static_cast<int16_t>(wingCenter.y + side.y * 8.0f + o.y),
            static_cast<int16_t>(wingCenter.x - side.x * 8.0f + o.x),
            static_cast<int16_t>(wingCenter.y - side.y * 8.0f + o.y),
            Graphics::WHITE);
    }
}

void ApexClimb::drawMiniMap(Graphics& g) const
{
    static constexpr int16_t MAP_X = 23;
    static constexpr int16_t MAP_Y = 159;

    for (uint8_t i = 0; i < MINIMAP_SAMPLES; ++i) {
        g.drawLine(MAP_X + miniMapX[i], MAP_Y + miniMapY[i], MAP_X + miniMapX[i + 1], MAP_Y + miniMapY[i + 1],
                   Graphics::LIGHTGRAY);
    }

    const int index = Math::clamp(static_cast<int>(progress * MINIMAP_SAMPLES + 0.5f), 0, static_cast<int>(MINIMAP_SAMPLES));
    g.fillCircle(MAP_X + miniMapX[index], MAP_Y + miniMapY[index], 2, Graphics::YELLOW);
    g.fillCircle(MAP_X + miniMapX[MINIMAP_SAMPLES], MAP_Y + miniMapY[MINIMAP_SAMPLES], 1, Graphics::WHITE);
}

void ApexClimb::drawDriftScore(Graphics& g, uint32_t now) const
{
    uint32_t score = 0;
    bool visible = false;

    if (drifting && currentDriftScore >= DRIFT_DISPLAY_THRESHOLD) {
        score = currentDriftScore;
        visible = true;
    } else if (confirmedDriftScore >= DRIFT_DISPLAY_THRESHOLD && driftScoreFlashStartMsec > 0) {
        const uint32_t elapsed = now - driftScoreFlashStartMsec;
        if (elapsed < 1000) {
            score = confirmedDriftScore;
            visible = ((elapsed / 100) & 1U) == 0;
        } else if (elapsed < 3000) {
            score = confirmedDriftScore;
            visible = true;
        }
    }

    if (!visible) return;

    char text[20];
    std::snprintf(text, sizeof(text), "DRIFT %lu", static_cast<unsigned long>(score));
    g.drawString(text, VIEW_W - 6, 162, Graphics::YELLOW, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::MIDDLE);
}

void ApexClimb::drawTelemetry(Graphics& g, uint32_t now) const
{
    // Fixed information panel on the right.
    g.fillRect(VIEW_W, 0, SCREEN_W - VIEW_W, SCREEN_H, PANEL);
    g.drawLine(VIEW_W, 0, VIEW_W, SCREEN_H, PANEL_LINE);

    const uint32_t elapsed =
        (mode == MODE_RUNNING || mode == MODE_FINISHING)
            ? (mode == MODE_RUNNING ? now - runStartMsec : finishMsec)
            : finishMsec;

    g.drawString("TIME", 246, 7, Graphics::LIGHTGRAY, Graphics::SIZE_10);
    drawTime(g, elapsed, 312, 19, Graphics::WHITE, Graphics::SIZE_13,
             Graphics::HorizontalAlign::RIGHT);

    g.drawString("BEST", 246, 42, Graphics::LIGHTGRAY, Graphics::SIZE_10);
    if (bestMsec > 0) {
        drawTime(g, bestMsec, 312, 54, Graphics::CYAN, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT);
    } else {
        g.drawString("--:--.--", 312, 54, Graphics::DARKGRAY, Graphics::SIZE_10,
                     Graphics::HorizontalAlign::RIGHT,
                     Graphics::VerticalAlign::TOP);
    }

    g.drawLine(246, 75, 312, 75, Graphics::DARKGRAY);
    g.drawString("SECTOR", 246, 80, ACCENT, Graphics::SIZE_10);

    auto drawSectorTime = [&](uint8_t index, int16_t y,
                              Graphics::Color color) {
        char label[4];
        std::snprintf(label, sizeof(label), "S%u",
                      static_cast<unsigned>(index + 1));
        g.drawString(label, 246, y, color, Graphics::SIZE_10);

        if (sectorTimes[index] > 0) {
            drawTime(g, sectorTimes[index], 312, y, Graphics::WHITE,
                     Graphics::SIZE_10,
                     Graphics::HorizontalAlign::RIGHT);
        } else {
            g.drawString("--:--.--", 312, y, Graphics::DARKGRAY,
                         Graphics::SIZE_10,
                         Graphics::HorizontalAlign::RIGHT,
                         Graphics::VerticalAlign::TOP);
        }
    };

    drawSectorTime(0, 96, Graphics::CYAN);
    drawSectorTime(1, 113, Graphics::YELLOW);
    drawSectorTime(2, 130, Graphics::RED);

    const char* handling =
        handlingState == HANDLING_DRIFT ? "DRIFT" :
        handlingState == HANDLING_UNDER ? "UNDER" : "GRIP";
    const Graphics::Color handlingColor =
        handlingState == HANDLING_DRIFT ? Graphics::YELLOW :
        handlingState == HANDLING_UNDER ? Graphics::CYAN :
                                          Graphics::LIGHTGRAY;
    g.drawString(handling, 312, 150, handlingColor, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT,
                 Graphics::VerticalAlign::TOP);

    // Input levels remain useful on both digital and analog controllers.
    auto drawInputBar = [&](const char* label, int16_t y,
                            float value, Graphics::Color color,
                            bool centered) {
        g.drawString(label, 246, y, Graphics::LIGHTGRAY, Graphics::SIZE_10);
        g.drawRect(274, y + 2, 38, 5, Graphics::DARKGRAY);

        if (centered) {
            const int16_t px = static_cast<int16_t>(
                Math::clamp(value, -1.0f, 1.0f) * 18.0f);
            g.drawLine(293, y + 2, 293, y + 6, Graphics::GRAY);
            if (px < 0)
                g.fillRect(293 + px, y + 3, -px, 3, color);
            else if (px > 0)
                g.fillRect(294, y + 3, px, 3, color);
        } else {
            const int16_t px = static_cast<int16_t>(
                Math::clamp(value, 0.0f, 1.0f) * 36.0f);
            if (px > 0) g.fillRect(275, y + 3, px, 3, color);
        }
    };

    drawInputBar("THR", 169, throttleInput, Graphics::GREEN, false);
    drawInputBar("BRK", 184, brakeInput, Graphics::RED, false);
    drawInputBar("STR", 199, steering, Graphics::CYAN, true);

    // Compact circular tachometer in the lower-right corner of the play area.
    static constexpr int16_t TACH_X = 207;
    static constexpr int16_t TACH_Y = 204;
    static constexpr uint16_t TACH_R = 30;

    g.fillCircle(TACH_X, TACH_Y, TACH_R, Graphics::BLACK);
    g.drawCircle(TACH_X, TACH_Y, TACH_R, Graphics::WHITE);

    const float rpmT =
        Math::clamp((displayRpm - 1600.0f) / (9000.0f - 1600.0f),
                    0.0f, 1.0f);
    // Sweep from lower-left, across the top, to lower-right.
    const float needleAngle =
        Math::degToRad(135.0f + rpmT * 270.0f);
    const int16_t needleX = static_cast<int16_t>(
        TACH_X + Math::cos(needleAngle) * (TACH_R - 5));
    const int16_t needleY = static_cast<int16_t>(
        TACH_Y + Math::sin(needleAngle) * (TACH_R - 5));
    g.drawLine(TACH_X, TACH_Y, needleX, needleY, Graphics::RED);

    // Inner disc keeps the speed and gear readable over the needle.
    g.fillCircle(TACH_X, TACH_Y, 18, Graphics::BLACK);

    char text[12];
    std::snprintf(text, sizeof(text), "%d",
                  static_cast<int>(getDisplaySpeedKmh() + 0.5f));
    g.drawString(text, TACH_X, TACH_Y - 8,
                 Graphics::WHITE, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);

    std::snprintf(text, sizeof(text), "%u",
                  static_cast<unsigned>(virtualGear));
    g.drawString(text, TACH_X, TACH_Y + 10,
                 Graphics::YELLOW, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
}

void ApexClimb::drawTitle(Graphics& g) const
{
    g.fillRectGradient(0, 0, SCREEN_W, SCREEN_H,
                       Graphics::rgb565(17, 35, 27),
                       Graphics::rgb565(72, 74, 66),
                       Graphics::VERTICAL_LINEAR);

    // Mountain silhouette.
    g.fillTriangle(0, 190, 92, 72, 170, 190, Graphics::rgb565(45, 55, 48));
    g.fillTriangle(90, 190, 205, 48, 320, 190, Graphics::rgb565(56, 61, 55));
    g.fillTriangle(170, 190, 258, 92, 320, 190, Graphics::rgb565(68, 67, 60));

    g.drawString("APEX", 160, 58, Graphics::WHITE, Graphics::SIZE_42B,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
    g.drawString("CLIMB", 160, 99, ACCENT, Graphics::SIZE_32B,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);

    g.fillRectGradient(48, 124, 112, 2,
                       Graphics::rgb565(25, 45, 40), ACCENT,
                       Graphics::HORIZONRAL_LINEAR);
    g.fillRectGradient(160, 124, 112, 2,
                       ACCENT, Graphics::rgb565(25, 45, 40),
                       Graphics::HORIZONRAL_LINEAR);

    g.drawString("TOUGE DRIFT TIME ATTACK", 160, 148,
                 Graphics::LIGHTGRAY, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
    g.drawString("Y/UP ACCEL   B/DOWN BRAKE", 160, 180,
                 Graphics::WHITE, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
    g.drawString("BRAKE + STEER: DRIFT", 160, 194,
                 Graphics::WHITE, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
    g.drawString("A / START", 160, 220, Graphics::YELLOW, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
}

void ApexClimb::drawCourseSelect(Graphics& g) const
{
    g.fillScreen(Graphics::BLACK);
    g.fillRectGradient(0, 0, SCREEN_W, 34, Graphics::rgb565(32, 46, 43), Graphics::rgb565(10, 13, 15),
                       Graphics::VERTICAL_LINEAR);

    g.drawString("COURSE SELECT", 160, 9, Graphics::WHITE, Graphics::SIZE_18,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);

    for (uint8_t i = 0; i < COURSE_COUNT; ++i) {
        const int16_t y = static_cast<int16_t>(46 + i * 36);
        const bool selected = i == selectedCourse;

        if (selected) {
            g.fillRectAlpha(24, y - 3, 272, 32, 100, Graphics::rgb565(55, 74, 68));
            g.drawRect(24, y - 3, 272, 32, ACCENT);
        }

        char label[24];
        std::snprintf(label, sizeof(label), "%c %s", selected ? '>' : ' ', COURSES[i].name);
        g.drawString(label, 36, y, selected ? Graphics::YELLOW : Graphics::WHITE, Graphics::SIZE_13);
        g.drawString(COURSES[i].subtitle, 284, y + 1, selected ? ACCENT : Graphics::DARKGRAY, Graphics::SIZE_10,
                     Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);

        const uint32_t best = rankings[i][0];
        if (best > 0) {
            drawTime(g, best, 284, y + 16, selected ? Graphics::CYAN : Graphics::LIGHTGRAY, Graphics::SIZE_10,
                     Graphics::HorizontalAlign::RIGHT);
        } else {
            g.drawString("--:--.--", 284, y + 16, Graphics::DARKGRAY, Graphics::SIZE_10,
                         Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);
        }
    }

    const bool practiceSelected = selectedCourse == COURSE_COUNT;
    if (practiceSelected) {
        g.fillRectAlpha(101, 197, 118, 24, 90, Graphics::rgb565(55, 74, 68));
        g.drawRect(101, 197, 118, 24, ACCENT);
    }

    g.drawString("FREE PRACTICE", 160, 209, practiceSelected ? Graphics::YELLOW : Graphics::LIGHTGRAY,
                 Graphics::SIZE_10, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void ApexClimb::drawReady(Graphics& g, uint32_t now) const
{
    const uint32_t elapsed = now - readyStartMsec;
    const char* text = elapsed < 600 ? "3" : (elapsed < 1200 ? "2" : ("1"));

    // Keep both the car and the road ahead visible during the countdown.
    g.fillRectAlpha(62, 146, 116, 58, 150, Graphics::BLACK);
    g.drawString(text, 120, 175, Graphics::WHITE, Graphics::SIZE_42B,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
}

void ApexClimb::drawResult(Graphics& g) const
{
    g.fillRectAlpha(26, 30, 188, 178, 220, Graphics::BLACK);
    g.drawRect(26, 30, 188, 178, runDisqualified ? Graphics::RED : ACCENT);

    g.drawString(runDisqualified ? "TIME OUT" : "FINISH", 120, 49, runDisqualified ? Graphics::RED : Graphics::WHITE,
                 Graphics::SIZE_22B, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    if (runDisqualified) {
        g.drawString("RUN DISQUALIFIED", 120, 76, Graphics::WHITE, Graphics::SIZE_13,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    } else {
        drawTime(g, finishMsec, 120, 76, Graphics::YELLOW, Graphics::SIZE_22B, Graphics::HorizontalAlign::CENTER);
    }

    char text[28];
    std::snprintf(text, sizeof(text), "BEST DRIFT %lu", static_cast<unsigned long>(bestDriftScore));
    g.drawString(text, 120, 108, bestDriftScore >= DRIFT_DISPLAY_THRESHOLD ? Graphics::CYAN : Graphics::LIGHTGRAY,
                 Graphics::SIZE_13, Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

    if (!runDisqualified) {
        if (finishRank > 0) std::snprintf(text, sizeof(text), "TIME RANK %u", static_cast<unsigned>(finishRank));
        else std::snprintf(text, sizeof(text), "TIME OUT OF TOP 5");
        g.drawString(text, 120, 132, finishRank == 1 ? Graphics::CYAN : Graphics::WHITE, Graphics::SIZE_10,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);

        if (finishDriftRank > 0) std::snprintf(text, sizeof(text), "DRIFT RANK %u", static_cast<unsigned>(finishDriftRank));
        else std::snprintf(text, sizeof(text), "DRIFT OUT OF TOP 5");
        g.drawString(text, 120, 149, finishDriftRank == 1 ? Graphics::YELLOW : Graphics::LIGHTGRAY, Graphics::SIZE_10,
                     Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    }

    g.drawString("A/START  RANKING", 120, 177, Graphics::WHITE, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
    g.drawString("B  COURSE", 120, 195, Graphics::LIGHTGRAY, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::MIDDLE);
}

void ApexClimb::drawRanking(Graphics& g) const
{
    g.fillScreen(Graphics::BLACK);
    g.fillRectGradient(0, 0, SCREEN_W, 34, Graphics::rgb565(31, 43, 41), Graphics::rgb565(9, 12, 14),
                       Graphics::VERTICAL_LINEAR);

    g.drawString("RANKING", 160, 8, Graphics::WHITE, Graphics::SIZE_22B,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    g.drawString(COURSES[currentCourse].name, 160, 37, ACCENT, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);

    g.drawString("TIME ATTACK", 78, 55, Graphics::CYAN, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
    g.drawString("DRIFT", 242, 55, Graphics::YELLOW, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);

    g.drawLine(160, 70, 160, 200, Graphics::DARKGRAY);

    for (uint8_t i = 0; i < RANKING_COUNT; ++i) {
        const int16_t y = static_cast<int16_t>(76 + i * 25);
        char place[6];
        std::snprintf(place, sizeof(place), "%u", static_cast<unsigned>(i + 1));

        g.drawString(place, 26, y, i == 0 ? Graphics::YELLOW : Graphics::LIGHTGRAY, Graphics::SIZE_13);
        if (rankings[currentCourse][i] > 0) {
            drawTime(g, rankings[currentCourse][i], 132, y, i == 0 ? Graphics::CYAN : Graphics::WHITE, Graphics::SIZE_13,
                     Graphics::HorizontalAlign::RIGHT);
        } else {
            g.drawString("--:--.--", 132, y, Graphics::DARKGRAY, Graphics::SIZE_13, Graphics::HorizontalAlign::RIGHT,
                         Graphics::VerticalAlign::TOP);
        }

        g.drawString(place, 206, y, i == 0 ? Graphics::YELLOW : Graphics::LIGHTGRAY, Graphics::SIZE_13);

        char score[16];
        if (driftRankings[currentCourse][i] > 0) {
            std::snprintf(score, sizeof(score), "%lu", static_cast<unsigned long>(driftRankings[currentCourse][i]));
            g.drawString(score, 282, y, i == 0 ? Graphics::YELLOW : Graphics::WHITE, Graphics::SIZE_13,
                         Graphics::HorizontalAlign::RIGHT, Graphics::VerticalAlign::TOP);
        } else {
            g.drawString("----", 282, y, Graphics::DARKGRAY, Graphics::SIZE_13, Graphics::HorizontalAlign::RIGHT,
                         Graphics::VerticalAlign::TOP);
        }
    }

    g.drawString("A/START RETRY   B COURSE", 160, 218, Graphics::WHITE, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER, Graphics::VerticalAlign::TOP);
}

void ApexClimb::drawPause(Graphics& g) const
{
    g.fillRectAlpha(38, 55, 164, 132, 215, Graphics::BLACK);
    g.drawRect(38, 55, 164, 132, ACCENT);

    g.drawString("PAUSE", 120, 72, Graphics::WHITE, Graphics::SIZE_25B,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);

    g.drawString("START  RESUME", 120, 108,
                 Graphics::WHITE, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
    g.drawString("A  RESTART", 120, 130,
                 Graphics::YELLOW, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
    g.drawString("B  COURSE", 120, 152,
                 Graphics::LIGHTGRAY, Graphics::SIZE_13,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
    g.drawString("START  RESUME", 120, 173,
                 Graphics::LIGHTGRAY, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::CENTER,
                 Graphics::VerticalAlign::MIDDLE);
}

void ApexClimb::drawTuning(Graphics& g) const
{
    g.fillRect(0, 0, SCREEN_W, SCREEN_H, Graphics::BLACK);
    g.fillRectGradient(
        0, 0, SCREEN_W, 26,
        Graphics::rgb565(28, 45, 48),
        Graphics::rgb565(10, 13, 15),
        Graphics::VERTICAL_LINEAR);

    g.drawString("PRESENTATION TUNING", 8, 6,
                 Graphics::WHITE, Graphics::SIZE_18);

    static const char* NAMES[] = {
        "ENGINE VOL",
        "TACH RISE",
        "TACH FALL",
        "GEAR 1>2",
        "GEAR 2>3",
        "GEAR 3>4",
        "GEAR 4>5",
        "ACCEL",
        "BRAKE",
        "MOVE SCALE",
        "TOP SPEED",
        "ACCEL TAPER",
        "TOP ACCEL"
    };

    const float VALUES[] = {
        presentation.engineGain,
        presentation.tachRise,
        presentation.tachFall,
        presentation.gear2Kmh,
        presentation.gear3Kmh,
        presentation.gear4Kmh,
        presentation.gear5Kmh,
        presentation.acceleration,
        presentation.brakeForce,
        presentation.movementScale,
        presentation.topSpeedKmh,
        presentation.highAccelStartKmh,
        presentation.highAccelScale
    };

    static constexpr uint8_t ITEM_COUNT = 13;
    static constexpr uint8_t VISIBLE = 11;

    int first = 0;
    if (tuningItem >= VISIBLE) {
        first = tuningItem - VISIBLE + 1;
    }

    char buf[32];
    for (uint8_t row = 0; row < VISIBLE; ++row) {
        const uint8_t i = static_cast<uint8_t>(first + row);
        if (i >= ITEM_COUNT) break;

        const int16_t y = static_cast<int16_t>(31 + row * 16);
        const bool selected = i == tuningItem;

        if (selected) {
            g.fillRectAlpha(4, y - 2, 204, 14, 110,
                            Graphics::rgb565(60, 84, 90));
        }

        g.drawString(NAMES[i], 8, y,
                     selected ? Graphics::YELLOW : Graphics::LIGHTGRAY,
                     Graphics::SIZE_10);

        if (i == 0 || i == 9 || i == 12) {
            std::snprintf(buf, sizeof(buf), "%.2f", VALUES[i]);
        } else {
            std::snprintf(buf, sizeof(buf), "%.0f", VALUES[i]);
        }

        g.drawString(buf, 204, y,
                     selected ? Graphics::YELLOW : Graphics::WHITE,
                     Graphics::SIZE_10,
                     Graphics::HorizontalAlign::RIGHT,
                     Graphics::VerticalAlign::TOP);
    }

    g.drawLine(215, 30, 215, 224, Graphics::DARKGRAY);

    g.drawString("LIVE", 224, 34, Graphics::CYAN, Graphics::SIZE_10);
    std::snprintf(buf, sizeof(buf), "SPD %3.0f", getDisplaySpeedKmh());
    g.drawString(buf, 312, 52, Graphics::WHITE, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT,
                 Graphics::VerticalAlign::TOP);
    std::snprintf(buf, sizeof(buf), "RPM %4.0f", displayRpm);
    g.drawString(buf, 312, 68, Graphics::YELLOW, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT,
                 Graphics::VerticalAlign::TOP);
    std::snprintf(buf, sizeof(buf), "GEAR %u",
                  static_cast<unsigned>(virtualGear));
    g.drawString(buf, 312, 84, Graphics::WHITE, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT,
                 Graphics::VerticalAlign::TOP);

    g.drawString("HANDLING", 224, 116, Graphics::DARKGRAY, Graphics::SIZE_10);
    g.drawString("FIXED", 312, 132, Graphics::LIGHTGRAY, Graphics::SIZE_10,
                 Graphics::HorizontalAlign::RIGHT,
                 Graphics::VerticalAlign::TOP);

    g.drawString("UP/DOWN ITEM", 224, 180,
                 Graphics::LIGHTGRAY, Graphics::SIZE_10);
    g.drawString("L/R VALUE", 224, 198,
                 Graphics::LIGHTGRAY, Graphics::SIZE_10);
    g.drawString("SELECT CLOSE", 224, 218,
                 Graphics::LIGHTGRAY, Graphics::SIZE_10);
}

void ApexClimb::drawTime(Graphics& g, uint32_t msec,
                            int16_t x, int16_t y,
                            Graphics::Color color,
                            Graphics::Font font,
                            Graphics::HorizontalAlign align) const
{
    char text[24];
    const uint32_t minutes = msec / 60000;
    const uint32_t seconds = (msec / 1000) % 60;
    const uint32_t centis = (msec / 10) % 100;
    std::snprintf(text, sizeof(text), "%02lu:%02lu.%02lu",
                  static_cast<unsigned long>(minutes),
                  static_cast<unsigned long>(seconds),
                  static_cast<unsigned long>(centis));
    g.drawString(text, x, y, color, font, align, Graphics::VerticalAlign::TOP);
}

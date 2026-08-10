// ============================================================================
// PRUZEA
// AI-Friendly Game Framework
//
// PRUZEA is a lightweight game framework designed for AI-assisted game development.
//
// This header defines the public API available to games.
// The runtime implementation is provided by the PRUZEA system.
//
// Execution model
//   System (30 FPS)
//       ├─ Input update
//       ├─ Game::onUpdate()
//       ├─ Game::onDraw()
//       └─ Graphics::push()
//   Audio
//       └─ Audio playback runs independently from the game loop.
// ============================================================================

#pragma once
#include <stdint.h>
#include <string>

namespace PRUZEA
{

constexpr char PRUZEA_VERSION[] = "1.2.1";

// =========================================================================
// [PROVIDED BY SYSTEM]
// All APIs declared in this header are provided by the PRUZEA system.
// They are already implemented by the runtime and do not need to be
// implemented or redefined in your application.
// =========================================================================
namespace Platform {
    uint32_t getMsec(); // Returns the number of milliseconds since system startup.
    uint64_t getUsec(); // Returns the number of microseconds since system startup.
    bool elapsed(uint32_t now, uint32_t startMsec, uint32_t durationMsec);
}
namespace Math {
    template<typename T>
    T clamp(T value, T min, T max); // All arguments must have the same type.
    float lerp(float a, float b, float t);
    float moveTowards(float current, float target, float maxDelta);
    float moveTowardsAngle(float current, float target, float maxDelta);
    float length(float x, float y);
    float lengthSquared(float x, float y);
    float distance(float ax, float ay, float bx, float by);
    float distanceSquared(float ax, float ay, float bx, float by);
    float dot(float ax, float ay, float bx, float by);
    float wrap(float value, float min, float max);
    float sin(float radians);
    float cos(float radians);
    float tan(float radians);
    float asin(float value);
    float acos(float value);
    float atan(float value);
    float atan2(float y, float x);
    void rotate(float x, float y, float radians, float& outX, float& outY);
    void normalize(float& x, float& y); // Normalize the vector. If the vector is zero, it is left unchanged.
    float angle(float x, float y); /// Returns the absolute angle of vector (x, y), in radians. Equivalent to atan2f(y, x). Return range: -PI to PI.
    float deltaAngle(float current, float target);
    float lerpAngle(float current, float target, float t);
    constexpr float PI = 3.14159265358979323846f;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float TWO_PI  = PI * 2.0f;
    float degToRad(float degrees);
    float radToDeg(float radians);
    int random(int max);          // [0, max)
    int random(int min, int max); // [min, max)
    float randomFloat();          // [0.0f, 1.0f)
    float randomFloat(float max); // [0.0f, max)
    float randomFloat(float min, float max); // [min, max)
    bool chance(float probability); // 0.0: 0%, 1.0: 100% (Returns true with the given probability (0.0f to 1.0f).)
    float map(float value, float inMin, float inMax, float outMin, float outMax);
    void reflect(float inX, float inY, float normalX, float normalY, float& outX, float& outY);
    float smoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaSec);
}
class Tween {
public:
    enum Ease : uint8_t {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,
        EASE_OUT_BACK,
        EASE_OUT_BOUNCE
    };
    // t is automatically clamped to 0.0f - 1.0f.
    static float apply(float t, Ease ease);
    static float lerp(float from, float to, float t);
    static float value(float from, float to, float t,Ease ease);
};
class Animation {
public:
    Animation(float duration, int totalFrames, bool loop = false);
    void start();
    void stop();
    void reset();
    void update(float deltaSec);
    bool isPlaying() const;
    bool isFinished() const;
    int frame() const;
private:
    float currentTime = 0.0f;
    float duration = 1.0f;
    int totalFrames = 1;
    bool playing = false;
    bool loop = false;
};
class Vector2 {
public:
    float x;
    float y;
    Vector2(float x = 0.0f, float y = 0.0f);
    float length() const;
    Vector2 normalized() const;
    float dot(const Vector2& other) const;
    float cross(const Vector2& other) const;
    float distance(const Vector2& other) const;

    Vector2 operator+(const Vector2& other) const;
    Vector2 operator-(const Vector2& other) const;
    Vector2& operator+=(const Vector2& other);
    Vector2& operator-=(const Vector2& other);
    Vector2 operator*(float scalar) const;
    Vector2 operator/(float scalar) const;
    Vector2& operator*=(float scalar);
    Vector2& operator/=(float scalar);
};
Vector2 operator*(float scalar, const Vector2& vector);
namespace Collision {
    bool pointRect(float px, float py, float rx, float ry, float rw, float rh);
    bool rectRect(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh);
    bool rectRect(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh, Vector2& pushOut);
    bool circleCircle(float ax, float ay, float ar, float bx, float by, float br);
    bool circleCircle(float ax, float ay, float ar, float bx, float by, float br, Vector2& pushOut);
    bool circleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh);
    bool circleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh, Vector2& pushOut);
    bool pointCircle(float px, float py, float cx, float cy, float radius);
    bool lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    bool lineRect(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh);
    bool lineCircle(float x1, float y1, float x2, float y2, float cx, float cy, float radius);
}
namespace Display {
    // Physical display resolutions.
    static constexpr uint16_t SSD1306_SCREEN_W = 128;
    static constexpr uint16_t SSD1306_SCREEN_H = 64;
    static constexpr uint16_t ILI9341_SCREEN_W = 320;
    static constexpr uint16_t ILI9341_SCREEN_H = 240;
}

// --- ======================
// # Input
// [!IMPORTANT] CRITICAL AI RULE FOR BUTTON INPUT:
//  - NEVER combine multiple buttons in a single function call.
//  - Functions like pressed() or justPressed() accept EXACTLY ONE button.
//  - Bad:  input.pressed(Input::LEFT | Input::A);
//  - Good: input.pressed(Input::LEFT) && input.pressed(Input::A);
// =========================
class Input {
public:
   // ## Input Style button layout:
   // - "Nintendo Style"
   //
   //        X
   //    Y       A
   //        B
   //
   //   A = Confirm, B = Cancel
   //
   // - "XInput Style"
   //
   //        Y
   //    X       B
   //        A
   //
   //   A = Confirm, B = Cancel
   //
   // - "Simple Style"
   //
   //    B       A
   //
   //   A = Confirm, B = Cancel

    enum Button : uint32_t {
        UP       = 1u << 0,
        DOWN     = 1u << 1,
        LEFT     = 1u << 2,
        RIGHT    = 1u << 3,
        A        = 1u << 4,
        B        = 1u << 5,
        X        = 1u << 6,
        Y        = 1u << 7,
        L        = 1u << 8,
        R        = 1u << 9,
        START    = 1u << 10,
        SELECT   = 1u << 11,
        L2       = 1u << 12,
        R2       = 1u << 13,
        L3       = 1u << 14,
        R3       = 1u << 15,
    
        // System-reserved virtual buttons. Hardware implementations may map
        // physical buttons or shortcuts to these.
        HOME     = 1u << 25,
        VOL_UP   = 1u << 26,
        VOL_DOWN = 1u << 27,
        MUTE     = 1u << 28
    };
    virtual bool pressed(Button b) const = 0;
    virtual bool justPressed(Button b) const = 0;
    virtual bool released(Button b) const = 0;
    virtual bool justReleased(Button b) const = 0;
    virtual bool held(Button b) const = 0;
    virtual uint64_t holdMillis(Button b) const = 0;

    // ## Button repeat settings
    // Default values are applied before Game::onInit() is called.
    static constexpr uint16_t DAS_DELAY_MSEC_DEFAULT = 150;
    static constexpr uint16_t ARR_DELAY_MSEC_DEFAULT = 50;

    // ## Button repeat settings
    // Default values are applied before Game::onInit().
    // Call this only when changing the default repeat timing.
    virtual void setRepeatSettings(uint16_t dasDelayMsec, uint16_t arrDelayMsec) = 0;
    virtual bool repeat(Button b) const = 0;

    // ## Analog stick input
    enum Axis : uint8_t
    {
        LEFT_X,
        LEFT_Y,
        RIGHT_X,
        RIGHT_Y
    };
    virtual bool hasAnalogSticks() const { return false; }
    // Returns a normalized value from -1000 to 1000.
    // Returns 0 when the axis is unavailable or inside the dead zone.
    virtual int16_t axis(Axis axis) const { return 0; }

    // ## Touchscreen input
    // Coordinates are already converted to the visible screen coordinate system.
    // Returns -1 for coordinates when no valid touch position has been received.
    virtual bool touched() const { return false; }
    virtual bool justTouched() const { return false; }
    virtual bool justTouchReleased() const { return false; }
    virtual int16_t touchX() const { return -1; }
    virtual int16_t touchY() const { return -1; }

protected:
    virtual ~Input() {};
};

// --- =================================================================
// # Graphics
//   - All graphics operations must be performed through this class.
//   - Supported displays:
//     - ILI9341
//       - RP2040:
//         - Graphics buffer size: 320x240 (same as the physical display)
//       - RP2350:
//         - Graphics buffer size: up to 384x288.
//         - The physical display shows a 320x240 area from this buffer.
//         - The extra buffer area may be used for camera movement and screen effects.
//     - SSD1306
//       - Graphics buffer size: up to 192x96.
//       - The physical display shows a 128x64 area from this buffer.
//       - The extra buffer area may be used for camera movement and screen effects.
//   - Drawing outside the visible screen is always safe. Graphics automatically clips all primitives internally.
//     The game does NOT need to perform screen boundary checks before drawing.
//     Prefer simple drawing code over manual visibility checks.
// ====================================================================
class Graphics {
public:
    static constexpr uint16_t ILI9341_SCREEN_BUF_W_MAX_RP2040 = 320;
    static constexpr uint16_t ILI9341_SCREEN_BUF_H_MAX_RP2040 = 240;
    static constexpr uint16_t ILI9341_SCREEN_VIEWPORT_X_MAX_RP2040 = 0;
    static constexpr uint16_t ILI9341_SCREEN_VIEWPORT_Y_MAX_RP2040 = 0;

    static constexpr uint16_t ILI9341_SCREEN_BUF_W_MAX_RP2350 = 384;
    static constexpr uint16_t ILI9341_SCREEN_BUF_H_MAX_RP2350 = 288;
    static constexpr uint16_t ILI9341_SCREEN_VIEWPORT_X_MAX_RP2350 = 64;
    static constexpr uint16_t ILI9341_SCREEN_VIEWPORT_Y_MAX_RP2350 = 48;

    static constexpr uint16_t SSD1306_SCREEN_BUF_W = 192;
    static constexpr uint16_t SSD1306_SCREEN_BUF_H = 96;
    static constexpr uint16_t SSD1306_SCREEN_VIEWPORT_X_MAX= 64;
    static constexpr uint16_t SSD1306_SCREEN_VIEWPORT_Y_MAX= 32;

    // -------------------------------------------------------------------------
    // RGB565 color value.
    // Use these constants for common colors, or use Graphics::rgb565(r, g, b)
    // to create any 24-bit RGB color converted to RGB565.
    // -------------------------------------------------------------------------
    enum Color : uint16_t
    {
        BLACK     = 0x0000,
        DARKGRAY  = 0x4208,
        GRAY      = 0x8410,
        LIGHTGRAY = 0xC618,
        WHITE     = 0xFFFF,
        RED       = 0xF800,
        GREEN     = 0x07E0,
        BLUE      = 0x001F,
        YELLOW    = 0xFFE0,
        CYAN      = 0x07FF,
        MAGENTA   = 0xF81F,
        ORANGE    = 0xFD20,
        PURPLE    = 0x8010,
        PINK      = 0xFC18,
        SSD1306_OFF = BLACK,
        SSD1306_ON  = WHITE
    };
    constexpr static Color rgb565(uint8_t r, uint8_t g, uint8_t b)
    {
        return static_cast<Color>( ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3) );
    }

    enum class HorizontalAlign : uint8_t {
        LEFT,
        CENTER,
        RIGHT
    };
    enum class VerticalAlign : uint8_t {
        TOP,
        MIDDLE,
        BOTTOM
    };

    virtual void clearScreen() = 0;
    virtual void fillScreen(Color color) = 0;
    virtual void drawPixel(int16_t x, int16_t y, Color color) = 0;
    virtual void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color)= 0;
    virtual void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) = 0;
    virtual void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color) = 0;
    virtual void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color) = 0;
    virtual void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t thickness, Color color) = 0;
    virtual void drawRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t thickness, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Color color) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t thickness, Color color) = 0;
    virtual void drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t radius, uint16_t thickness, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color) = 0;
    virtual void fillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Color color) = 0; // alpha: 255 = fully visible
    virtual void fillRectAlpha(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t alpha, Color color, HorizontalAlign ha, VerticalAlign va) = 0; // alpha: 255 = fully visible
    virtual void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Color color) = 0;
    virtual void fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t r, Color color, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void drawCircle(int16_t x, int16_t y, uint16_t r, Color color) = 0;
    virtual void drawCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Color color) = 0;
    virtual void fillCircle(int16_t x, int16_t y, uint16_t r, Color color) = 0;
    virtual void fillCircle(int16_t x, int16_t y, uint16_t rx, uint16_t ry, Color color) = 0;

    // [!IMPORTANT] AI WARNING:
    //   Use only the enum values defined below.
    //   Never invent or guess font names.
    //   e.g., Do not use SIZE_14, SIZE_18J, SIZE_25J, or SIZE_42J.
    //   Undefined font names will not compile.
    enum Font : uint8_t {
        // When the text width is required, always use `getTextWidth()` to obtain it.
        SIZE_10,  // very small / debug                   (height: 10px) English only
        SIZE_13,  // HUD                                  (height: 13px) English only
        SIZE_18,  // normal menu text                     (height: 18px) English only
        SIZE_22,  // normal menu text                     (height: 22px) English only
        SIZE_22B, // Bold normal menu text                (height: 22px) English only
        SIZE_25,  // large text / pause / game over       (height: 25px) English only
        SIZE_25B, // Bold  large text / pause / game over (height: 25px) English only
        SIZE_32,  // title                                (height: 32px) English only
        SIZE_32B, // Bold title                           (height: 32px) English only
        SIZE_42,  // big title                            (height: 40px) English only
        SIZE_42B, // Bold big title                       (height: 42px) English only

        // --- Fonts with Japanese text support ---
        SIZE_16J, // Japanese text  (height: 16px)
        SIZE_20J, // Japanese text  (height: 20px scaling)
        SIZE_32J  // Japanese text  (height: 32px scaling)
    };
    virtual uint16_t getTextWidth(const char* text, Font font) = 0;
    virtual uint16_t getTextHeight(const char* text, Font font) = 0;
    virtual void drawString(const char* str, int16_t x, int16_t y, Color color, Font font) = 0;
    virtual void drawString(const char* str, int16_t x, int16_t y, Color color, Font font, HorizontalAlign ha, VerticalAlign va) = 0;

    // ## Sprites (bitmap images)
    //   - `bitmap` must point to RGB565 (16-bit) pixel data.
    //   - [!IMPORTANT] CRITICAL AI RULE FOR BITMAP DATA
    //     - Define all sprite bitmap arrays as `static const uint16_t` at global or class scope.
    //     - Never declare bitmap arrays inside functions.
    //     - Mutable bitmap arrays waste SRAM and may cause Stack Overflow or Out of Memory.
    //       SRAM is limited.Large bitmap arrays must be defined as `static const uint16_t`
    //       so they are stored in flash memory instead of SRAM.
    //     Bad:
    //       uint16_t player[] = { ... };
    //     Good:
    //       static const uint16_t player_sprite[] = { ... };
    //   - Recommended sprite size: 32x32 pixels or smaller.
    //   - Enlarge sprites using `scale` instead of larger bitmap data.
    virtual void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h) = 0;
    struct SpriteOptions
    {
        uint8_t scale = 1;
        float angle = 0.0f;  /// Rotation angle in radians.
        bool flipX = false;
        bool flipY = false;

        bool transparent = false;
        Color transparentColor = MAGENTA;
    };
    virtual void drawSprite(const uint16_t* bitmap, int16_t x, int16_t y, uint16_t w, uint16_t h, const SpriteOptions& options) = 0;
    struct SpriteSheet
    {
        const uint16_t* bitmap = nullptr;
        uint16_t spriteWidth = 0;
        uint16_t spriteHeight = 0;
        uint16_t columns = 0;
        uint16_t rows = 0;
    };
    virtual void drawSprite(const SpriteSheet& sheet, uint16_t column, uint16_t row, int16_t x, int16_t y, const SpriteOptions& options) = 0;

    // Draws an image created by this Graphics backend.
    // Passing an image created by another backend is not supported.
    // Image loading may be unsupported by some display backends.
    // drawImage() uses the transparency information stored in the Image.
    // PNG alpha transparency is not supported; PNG images are drawn as opaque.
    // To specify a different transparent color, scale, or flip direction, use drawSprite() with Image::getBitmap(), getWidth(), and getHeight().
    // Images are created by the graphics backend. Never construct or subclass Image in game code.
    // Games only ever obtain instances via loadJpeg()/loadPng(); never construct or subclass Image directly.
    // Always call Image::close() when finished with the image. Forgetting to call close() will cause a memory leak.
    class Image
    {
    public:
        // Returns a decoded reusable image.
        // Returns nullptr when:
        // - the active graphics backend does not support this image format,
        // - the image data is invalid,
        // - the output size is invalid, or
        // - memory allocation or decoding fails.
        // The returned image must be explicitly closed by calling Image::close().
        enum class Fit : uint8_t
        {
            STRETCH,
            CONTAIN,
            COVER
        };
        static Image* loadJpeg(const uint8_t* jpegData, uint32_t jpegSize, uint16_t outputWidth, uint16_t outputHeight, Fit fit);
        static Image* loadPng(const uint8_t* pngData, uint32_t pngSize, uint16_t outputWidth, uint16_t outputHeight, Fit fit);

        virtual const uint16_t* getBitmap() const = 0;
        virtual uint16_t getWidth() const = 0;
        virtual uint16_t getHeight() const = 0;
        virtual void close();

    protected:
        Image() = default;
        virtual ~Image() = default;
     private:
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
    };
    virtual void drawImage(const Image& image, int16_t x, int16_t y) {}

    // ## Viewport control
    //   - The default viewport position is (0, 0).
    //   - Use setViewport() to move the visible area within the graphics buffer.
    //   - This may be used for camera movement or temporary screen effects.
    //   - When moving the viewport, draw enough background around the visible area to avoid exposing undrawn regions.
    //   - [!IMPORTANT] AI WARNING FOR TEMPORARY VIEWPORT OFFSETS:
    //     - Always restore the viewport with resetViewport() when the effect ends.
    //     - Otherwise, the screen will remain offset.
    //   - setViewport() is always memory-safe.
    //     If no extra buffer margin is available, moving the viewport may expose
    //     undrawn or clipped areas.
    //     The available margin depends on the active display backend.
    virtual void setViewport(int16_t viewportX, int16_t viewportY) = 0;
    virtual void resetViewport() = 0;

    struct Camera
    {
        int16_t x = 0;
        int16_t y = 0;
        float zoom = 1.0f;
        int16_t zoomCenterX = 0;
        int16_t zoomCenterY = 0;
    };
    virtual void setCamera(const Camera& camera) = 0;
    virtual void resetCamera() = 0;

    virtual void setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h) = 0;
    virtual void setClipRect(int16_t x, int16_t y, uint16_t w, uint16_t h, HorizontalAlign ha, VerticalAlign va) = 0;
    virtual void resetClipRect() = 0;

protected:
    virtual ~Graphics() {};
};

// --- =================================================================
// # Audio
// =====================================================================
class Audio {
public:
    // ## Sound Effect
    struct SoundStep
    {
        uint16_t startFrequency;
        uint16_t endFrequency;
        uint16_t durationMsec;
        float startVolume;
        float endVolume;
    };
    struct Sound
    {
        const SoundStep* steps;
        uint16_t stepCount;
    };
    // ### Sound Effect Presets
    struct SE
    {
        static const Sound NO_1;  // 1200hz 30msec - 1800hz 20msec (e.g., Menu confirm, Cursor move, Laser shot)
        static const Sound NO_2;  // 300hz 40msec - 150hz 30msec (e.g., Cancel, Damage taken)
        static const Sound NO_3;  // 800hz 60msec - 1200hz 60msec - 1600hz 100msec (e.g., Item pickup, Power-up)
        static const Sound NO_4;  // 1000hz 50msec - 1300hz 50msec - 1500hz 50msec - 2000hz 150msec (e.g., Large explosion, Game over)
        static const Sound NO_5;  // 600hz 30msec - 400hz 30msec - 600hz 30msec (e.g., Jump, Message advance)
        static const Sound NO_6;  // 1500hz 40msec - 1000hz 40msec (e.g., Enemy defeat, Attack blocked)
        static const Sound NO_7;  // 2500hz 20msec - 2500hz 20msec (10msec silence) (e.g., Low health alert, Time warning)
        static const Sound NO_8;  // 400hz 50msec - 600hz 50msec - 800hz 50msec - 1000hz 50msec - 1200hz 50msec (e.g., Stage start, Victory fanfare)
        static const Sound NO_9;  // 2000hz 20msec -> 1500hz 20msec - 2000hz 20msec - 1500hz 20msec (e.g., Invincibility, Star effect)
        static const Sound NO_10; // 100hz 80msec - 80hz 120msec (e.g., Earthquake, Heavy footsteps, Boss entrance)
        static const Sound NO_11; // 500hz 20msec - 1000hz 20msec - 1500hz 20msec - 2000hz 20msec - 2500hz 40msec (e.g., Charge attack, Secret discovered)
        static const Sound NO_12; // 1800hz 100msec - 1400hz 100msec - 1100hz 100msec - 900hz 200msec (e.g., Stage failed, Sad game over jingle)
        static const Sound NO_13; // 200hz 80msec - 0hz 50msec - 200hz 120msec (e.g., Error, Invalid operation)
    };
    // ### Sound playback
    //   - Playback runs on a separate core and returns immediately.
    //   - Valid gain range: 0.1-1.0.
    //     Use different gain values to match the intended effect.
    //   - Sound and SoundStep data passed to playSE() must remain valid until playback finishes.
    //     Define custom sound data as `static const`. Do not pass pointers to local variables.
    // [!IMPORTANT] AI WARNING:
    //   - The `sound` parameter requires a pointer (`const Sound*`).
    //   - Always pass the address of a preset using the `&` operator.
    //   - Bad:  audio.playSE(Audio::SE::NO_1, 1.0f);
    //   - Good: audio.playSE(&Audio::SE::NO_1, 1.0f);
    virtual void playSE(const Sound* sound, float gain) = 0;

    // ## Music
    struct ToneNote
    {
        static constexpr uint16_t REST = 0; // Silence.
        static constexpr uint16_t C2 = 65,  CS2 = 69,  D2 = 73,  DS2 = 78,  E2 = 82,  F2 = 87,  FS2 = 92,  G2 = 98,  GS2 = 104, A2 = 110, AS2 = 117, B2 = 123;
        static constexpr uint16_t C3 = 131, CS3 = 139, D3 = 147, DS3 = 156, E3 = 165, F3 = 175, FS3 = 185, G3 = 196, GS3 = 208, A3 = 220, AS3 = 233, B3 = 247;
        static constexpr uint16_t C4 = 262, CS4 = 277, D4 = 294, DS4 = 311, E4 = 330, F4 = 349, FS4 = 370, G4 = 392, GS4 = 415, A4 = 440, AS4 = 466, B4 = 494;
        static constexpr uint16_t C5 = 523, CS5 = 554, D5 = 587, DS5 = 622, E5 = 659, F5 = 698, FS5 = 740, G5 = 784, GS5 = 831, A5 = 880, AS5 = 932, B5 = 988;
        static constexpr uint16_t C6 = 1047,CS6 = 1109,D6 = 1175,DS6 = 1245,E6 = 1319,F6 = 1397,FS6 = 1480,G6 = 1568,GS6 = 1661,A6 = 1760,AS6 = 1865,B6 = 1976;
        uint16_t frequency;

        enum Duration : uint8_t {
        	// Note durations:
        	//   S  = sixteenth note, SD = dotted sixteenth note, E  = eighth note,    ED = dotted eighth note,
        	//   Q  = quarter note,   QD = dotted quarter note,   H  = half note,      HD = dotted half note, W  = whole note
            S  = 2,
            SD = 3,
            E  = 4,
            ED = 6,
            Q  = 8,
            QD = 12,
            H  = 16,
            HD = 24,
            W  = 32
        };
        Duration duration;
        bool tie;  // If true, continue into the next note without retriggering the tone.

        constexpr ToneNote() : frequency(REST), duration(Q), tie(false) {}
        constexpr ToneNote(uint16_t _frequency, Duration _duration, bool _tie = false) : frequency(_frequency), duration(_duration), tie(_tie) {}
    };
    struct Music
    {
        const ToneNote* notes;
        uint16_t bpm;
        uint16_t noteCount; // element count
        uint8_t playCount;  // 0 = infinite, 1 = play once
        float gain;         // Valid range: 0.1-1.0.
    };
    // ### Music playback
    //   - Music and ToneNote data passed to playMusic() must remain valid until playback stops.
    //   - Define custom music data as `static const`. Do not pass pointers to local variables.
    virtual void playMusic(const Music* music) = 0;

    // ## Embedded MIDI music
    //   - Supports lightweight playback of embedded Standard MIDI File data.
    //   - MIDI data must remain valid until playback stops.
    //   - Define MIDI byte arrays and Midi objects as `static const`.
    struct Midi
    {
        const uint8_t* data;
        uint32_t size;
        uint8_t playCount; // 0 = infinite, 1 = play once
        float gain;        // Valid range: 0.0-1.0.
    };
    virtual void playMidi(const Midi* midi) = 0;

    // Stops either ToneNote music or MIDI music.
    virtual void stopMusic() = 0;

protected:
    virtual ~Audio() {};
};


// --- =================================================================
// # Storage
// PRUZEA provides three storage APIs:
// 1. SaveData
//    Key-value persistent storage for ordinary game save data.
//    No file paths or file-system operations are required.
//    Use it for: high scores, unlocked content, game progress, game settings
// 2. UserFile
//    Writable per-game file storage for custom data formats.
//    The game supplies only its gameId and a relative file name.
//    The system determines the actual storage path.
//    Games must not construct save-data paths manually.
// 3. File
//    Read-only access to packaged game resources.
//
//    File cannot be used to create, write, or modify save data.
// =====================================================================
class Storage {
public:
    virtual bool isAvailable() const = 0;

    // ## Read-only resource file
    // File provides read-only access to packaged game assets.
    // It is not a writable save-data API.
    // Files opened with openRead() must be explicitly closed by the caller.
    // Resource files should normally be opened and read only inside Game::init().
    class File {
    public:
        virtual bool isOpen() const = 0;
        virtual uint32_t size() const = 0;
        virtual uint32_t read(void* buffer, uint32_t bytes) = 0;
        virtual void close() = 0;
    protected:
       virtual ~File() {}
    };

    // ## UserFile
    // ### Read UserFile
    //   - The system automatically opens and closes the file.
    //   - The reader callback receives one null-terminated line at a time.
    //   - Return true from the callback to continue reading the next line.
    //   - Return false from the callback to stop reading early.
    //   - Returning false is a normal early termination and does not mean that readUserFile() failed.
    //   - readUserFile() returns false when the file could not be opened or read.
    //   - The `arg` parameter can be used to pass `this` to a non-capturing lambda or static callback.
    using UserFileLineReaderHandler = bool(*)(const char* line, void* arg);
    virtual bool readUserFile(const char* gameId, const char* fileName, UserFileLineReaderHandler reader, void* arg) = 0;
    // ### Write UserFile
    //   - The callback uses std::string to prevent buffer overflows.
    //   - Inside this callback, ALWAYS use direct assignment (e.g., line = "value";)
    //      or std::string::assign(). Never use string concatenation ('+') to prevent dynamic memory allocation.
    //   - A single write operation may produce at most 256 lines.
    //   - The callback is called once for each output line.
    //     - If the callback continues beyond this limit, writing fails and writeUserFile() returns false.
    //     - The callback must return false after the final line.
    //   - [!IMPORTANT] CRITICAL AI RULE:
    //     - Never use string concatenation ('+').
    //     - Use direct assignment or std::string::assign().
    //     - Use snprintf() with a local buffer instead of std::to_string().
     static constexpr uint16_t USER_FILE_MAX_LINES = 256;
    using UserFileLineWriterHandler = bool(*)(std::string& line, void* arg);
    virtual bool writeUserFile(const char* gameId, const char* fileName, UserFileLineWriterHandler writer, void* arg) = 0;
    // ### Lightweight UserFile writing
    //   - For writes of 200 bytes or less, use this function instead of UserFileLineWriterHandler.
    //   - Maximum size: 200 bytes.
    //   - Larger writes must use UserFileLineWriterHandler.
    virtual bool writeUserFile(const char* gameId, const char* fileName, const char* data) = 0;

    // ## Read-only resource files
    // Opens a packaged read-only resource.
    //   - Do not use openRead() for save data or user settings.
    //   - Use UserFile for writable persistent data.
    //   - Always call File::close() when finished.
    virtual File* openRead(const char* path) = 0;

    virtual bool directoryExists(const char* path) = 0;
    virtual bool fileExists(const char* path) = 0;

protected:
    virtual ~Storage() {};
};

// --- =================================================================
// # SaveData: Storage helper
//   SaveData provides small key-value persistent storage.
//   It handles file formatting and UserFile access internally.
//   Use SaveData for ordinary save data instead of implementing a custom key-value file format.
// =====================================================================
class SaveData {
public:
    static constexpr uint8_t MAX_ENTRIES = 16;
    static constexpr uint16_t BUFFER_SIZE = 512;

    SaveData();

    void clear();
    bool load(Storage& storage, const char* gameId, const char* fileName);
    bool save(Storage& storage, const char* gameId, const char* fileName);
    bool contains(const char* key) const;
    bool remove(const char* key);
    bool getString(const char* key, char* outValue, size_t outSize, const char* defaultValue = "") const;
    int32_t getInt32(const char* key, int32_t defaultValue = 0) const;
    uint32_t getUInt32(const char* key, uint32_t defaultValue = 0) const;
    bool getBool(const char* key, bool defaultValue = false) const;
    bool setString(const char* key, const char* value);
    bool setInt32(const char* key, int32_t value);
    bool setUInt32(const char* key, uint32_t value);
    bool setBool(const char* key, bool value);
    bool isDirty() const;
    uint8_t getEntryCount() const;
    uint16_t getUsedBytes() const;
    uint16_t getFreeBytes() const;

private:
    struct Entry {
        uint16_t keyOffset;
        uint16_t valueOffset;
    };
    Entry entries[MAX_ENTRIES];
    char buffer[BUFFER_SIZE];
    uint8_t entryCount = 0;
    uint16_t usedBytes = 0;
    bool dirty = false;
    uint8_t saveCursor = 0;

    int16_t findEntry(const char* key) const;
    bool appendString(const char* text, uint16_t& offset);
    static bool writeLineHandler(std::string& line, void* arg);
};

// --- =================================================================
// # Game
//   Derive your game class from this class to run it on PRUZEA.
//   - Target update rate: 30 FPS
//   - deltaSec is provided every frame.
//   [!IMPORTANT] Never call init(), update(), draw(), or terminate() directly.
//   The PRUZEA runtime calls them automatically.
//   [!IMPORTANT] Override all pure virtual functions.
// ==========================================================&===========
class Game {
public:
    // [!IMPORTANT] GameState is a lifecycle status returned by onUpdate().
    //   Never use GameState to store gameplay states.
    //   Use a private enum for detailed internal game states.
    enum GameState : uint8_t {
        RUNNING,    // Running state.
        TERMINATE_REQUEST, // Requests termination. Game::update() calls onTerminate() exactly once, then returns TERMINATED to the system.
        TERMINATED  // Internal completion state returned to the system.
    };
    enum class TerminateResponse : uint8_t {
        ACCEPT,
        REJECT
    };

private:
    bool terminatedFlag = false;
    // ## Typical game mode example:
    // enum InternalMode {
    //     MODE_TITLE,
    //     MODE_PLAYING,
    //     MODE_PAUSED,
    //     MODE_GAME_OVER
    // };
    // Real-time games are encouraged to implement MODE_PAUSED and toggle it with the START button.
    // When entering the pause state, you MUST explicitly call `audio.stopMusic()`.
    // When resuming the game to `MODE_RUNNING` or `MODE_PLAYING`, you MUST call `audio.playMusic()` exactly once to restart the BGM.

protected:
    // ## Redraw flag
    // `dirty` indicates whether the screen needs to be redrawn.
    // - The initial value is true so the first frame is always drawn.
    // - Set dirty = true in update() whenever any visible state changes.
    // - Examples:
    //     player movement
    //     score changes
    //     animation changes
    //     state transitions
    //     viewport or camera changes
    // - Continuously animated action games may set dirty every update while running.
    bool dirty = true;

    // ## Called once before the game starts.
    //   - Load save data, initialize variables, and prepare resources.
    //   - Normally, rendering should be performed in draw().
    //   - After onInit() returns, the system calls draw() with requestFullRedraw = true.
    // onInit() has no call-order guard because it is called on every replay.
    // Therefore, it keeps its direct name.
    // update(), draw(), and terminate() are guarded and use the on* naming convention.
    virtual void onInit(Storage& storage) = 0;

    // ## Frame-by-frame game state updating
    // Called once per frame to update game logic, physics, and animations.
    //   - Input has already been updated before onUpdate() is called.
    //   - [!IMPORTANT] Request sound playback only from onUpdate().
    //   - [!IMPORTANT] Return GameState::TERMINATED to end the game.
    //     After that, onUpdate(), draw(), and onTerminate() will never be called again.
    // deltaSec:
    //   Elapsed time since the previous Game::update(), in seconds.
    //   The system guarantees a value in the range 0.0f to 0.1f.
    //   The first update after init(), resume, or a system-side pause receives 0.0f.
    virtual Game::GameState onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) = 0;

    // ## Frame-by-frame game screen drawing
    // Called once per frame to draw the game screen.
     // - If requestFullRedraw is true, the game must draw regardless of dirty.
    // - If requestFullRedraw is false and dirty is false, return false without drawing.
    // - Return true only when drawing was performed.
    // - After drawing, set dirty = false before returning true.
    // - The system uses the return value to decide whether the graphics buffer must be transferred to the physical display.
    // - [!IMPORTANT] CRITICAL AI RULE FOR DIRTY RENDERING:
    //   - Check `dirty` at the very beginning of onDraw().
    //   - Perform ALL drawing operations FIRST.
    //   - Set `dirty = false` ONLY AT THE VERY END of the function, right before `return true;`.
    virtual bool onDraw(Graphics& graphics, bool requestFullRedraw) = 0;

    // ## Termination request
    //   - Override this function only if the game may reject a termination request
    //     (e.g., when unsaved changes require user confirmation).
    //   - This is not a pure virtual function. If not needed, simply do not override it.
    //   - If the game always saves automatically (e.g., high scores), do not override this function.
    //     Perform the save operation in onTerminate() instead.
    //   - [!IMPORTANT]
    //     Called by the runtime when the user requests to close the game.
    //     Return ACCEPT to terminate immediately.
    //     Return REJECT to keep the game running.
    //     The game may present its own confirmation dialog before returning.
    virtual TerminateResponse onRequestTerminate() { return TerminateResponse::ACCEPT; }

    // ## Termination
    //   - Called by the runtime when the game is about to terminate.
    //   - Save persistent data or perform cleanup here if needed.
    //   - [!IMPORTANT] After onTerminate() is called, onUpdate() and onDraw() will never be called again.
    virtual void onTerminate(Storage& storage) = 0;

    virtual ~Game() {};

public:
    // Returns the game ID.
    // The ID may contain only lowercase letters (a-z), digits (0-9), underscores (_), and hyphens (-), up to 32 characters.
    // The game ID is used for folder names and other internal paths.
    virtual const char* getId() const = 0;

    // ## Returns the game title.
    // Japanese characters may be used when a Japanese font is available.
    // Use this title when displaying the game name.
    virtual const char* getName() const = 0;

    // ## Game selection name
    // Returns the game name displayed on the game selection screen. English characters only.
    virtual const char* getMenuName() const = 0;

    // Optional menu group name.
    //   Return an empty string to display this game in the root menu.
    //   Games returning exactly the same group name are displayed in the same folder.
    //   The system does not normalize, rename, or translate this value.
    //   Use a stable spelling and capitalization when grouping multiple games.
    //   Example: return "SAMPLES";
    virtual const char* getMenuGroup() const { return ""; }

    // ## Returns the drawing buffer size.
    // Normally, return the physical display size.
    // Return a larger size only when extra off-screen drawing space is needed,
    // e.g., for screen shake or camera movement.
    virtual uint16_t getLogicalScreenWidth() const = 0;
    virtual uint16_t getLogicalScreenHeight() const = 0;

    // ## Returns the target display resolution.
    // e.g., 128x64 for SSD1306, 320x240 for ILI9341.
    virtual uint16_t getTargetScreenWidth() const = 0;
    virtual uint16_t getTargetScreenHeight() const = 0;

    // ## System entry points
    // [!IMPORTANT]
    // The following functions are called by the PRUZEA runtime.
    // Do not modify, hide, or redefine them.
    // Implement the corresponding on*() functions instead.
    void init(Storage& storage)
    {
        terminatedFlag = false;
        dirty = true;
        onInit(storage);
    }
    Game::GameState update(Input& input, Audio& audio, Storage& storage, float deltaSec)
    {
        if (terminatedFlag) return Game::GameState::TERMINATED;
        const Game::GameState state = onUpdate(input, audio, storage, deltaSec);
        if (state == Game::GameState::TERMINATE_REQUEST || state == Game::GameState::TERMINATED)
        {
            terminate(storage);
            return Game::GameState::TERMINATED;
        }
        return state;
    }
    bool draw(Graphics& graphics, bool requestFullRedraw)
    {
        if (terminatedFlag) return false;
        return onDraw(graphics, requestFullRedraw);
    }
    TerminateResponse requestTerminate()
    {
        return onRequestTerminate();
    }
    void terminate(Storage& storage)
    {
        if (terminatedFlag) return;
        onTerminate(storage);
        terminatedFlag = true;
    }
};

} // namespace PRUZEA

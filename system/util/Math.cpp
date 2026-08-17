#include "PRUZEA.h"
#include "Platform.h"
#include <cmath>

namespace PRUZEA
{
namespace Math
{

// Template implementation of clamp
template<typename T>
T clamp(T value, T min, T max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Explicit instantiations for common types to avoid linking errors
template int clamp<int>(int value, int min, int max);
template long clamp<long>(long value, long min, long max);
template float clamp<float>(float value, float min, float max);
template double clamp<double>(double value, double min, double max);
template int8_t clamp<int8_t>(int8_t value, int8_t min, int8_t max);
template uint8_t clamp<uint8_t>(uint8_t value, uint8_t min, uint8_t max);
template int16_t clamp<int16_t>(int16_t value, int16_t min, int16_t max);
template uint16_t clamp<uint16_t>(uint16_t value, uint16_t min, uint16_t max);
// template int32_t clamp<int32_t>(int32_t value, int32_t min, int32_t max);
template uint32_t clamp<uint32_t>(uint32_t value, uint32_t min, uint32_t max);
template int64_t clamp<int64_t>(int64_t value, int64_t min, int64_t max);
template uint64_t clamp<uint64_t>(uint64_t value, uint64_t min, uint64_t max);

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float moveTowards(float current, float target, float maxDelta)
{
    if (fabsf(target - current) <= maxDelta)
    {
        return target;
    }
    return current + ((target > current) ? maxDelta : -maxDelta);
}

float moveTowardsAngle(float current, float target, float maxDelta)
{
    float delta = target - current;
    // Wrap to -PI to PI
    delta = wrap(delta, -3.14159265f, 3.14159265f);
    
    if (fabsf(delta) <= maxDelta)
    {
        return target;
    }
    
    float result = current + ((delta > 0.0f) ? maxDelta : -maxDelta);
    return wrap(result, -3.14159265f, 3.14159265f);
}

float length(float x, float y)
{
    return sqrtf(x * x + y * y);
}

float lengthSquared(float x, float y)
{
    return x * x + y * y;
}

float distance(float ax, float ay, float bx, float by)
{
    return length(bx - ax, by - ay);
}

float distanceSquared(float ax, float ay, float bx, float by)
{
    return lengthSquared(bx - ax, by - ay);
}

void normalize(float& x, float& y)
{
    float len = length(x, y);
    if (len > 0.0f)
    {
        x /= len;
        y /= len;
    }
}

float dot(float ax, float ay, float bx, float by)
{
    return ax * bx + ay * by;
}

float sqrt(float value)
{
    return sqrtf(value);
}

float abs(float value)
{
    return fabsf(value);
}

float wrap(float value, float min, float max)
{
    float range = max - min;
    if (range <= 0.0f) return min;
    
    float val = fmodf(value - min, range);
    if (val < 0.0f)
    {
        val += range;
    }
    return val + min;
}

float round(float value)
{
    return roundf(value);
}

float floor(float value)
{
    return floorf(value);
}

float ceil(float value)
{
    return ceilf(value);
}

float sin(float radians)
{
    return sinf(radians);
}

float cos(float radians)
{
    return cosf(radians);
}

float tan(float radians)
{
    return tanf(radians);
}

float asin(float value)
{
    return asinf(value);
}

float acos(float value)
{
    return acosf(value);
}

float atan(float value)
{
    return atanf(value);
}

float atan2(float y, float x)
{
    return atan2f(y, x);
}

void rotate(float x, float y, float radians, float& outX, float& outY)
{
    float s = sinf(radians);
    float c = cosf(radians);
    outX = x * c - y * s;
    outY = x * s + y * c;
}

float angle(float x, float y)
{
    return atan2f(y, x);
}

float deltaAngle(float current, float target)
{
    float delta = wrap(target - current, -Math::PI, Math::PI);
    return delta;
}

float lerpAngle(float current, float target, float t)
{
    float delta = deltaAngle(current, target);
    return current + delta * t;
}

float degToRad(float degrees)
{
    return degrees * (Math::PI / 180.0f);
}

float radToDeg(float radians)
{
    return radians * (180.0f / Math::PI);
}

int random(int max)
{
    if (max <= 0) return 0;
    uint64_t random_raw = Platform::random32();
    return static_cast<int>((random_raw * static_cast<uint32_t>(max)) >> 32);
}

int random(int min, int max) {
    if (min >= max) return min;
    return min + random(max - min);
}

float randomFloat() {
    return static_cast<float>(Platform::random32()) / 0x1.0p32f;
}

float randomFloat(float max) {
    if (max <= 0.0f) return 0.0f;
    return randomFloat() * max;
}

float randomFloat(float min, float max) {
    if (min >= max) return min;
    return min + randomFloat() * (max - min);
}

bool chance(float probability)
{
    if (probability <= 0.0f) return false;
    if (probability >= 1.0f) return true;
    return randomFloat() < probability;
}

float map(float value, float inMin, float inMax, float outMin, float outMax)
{
    float inRange = inMax - inMin;
    if (inRange == 0.0f) return outMin;
    float t = (value - inMin) / inRange;
    return outMin + t * (outMax - outMin);
}

void reflect(float inX, float inY, float normalX, float normalY, float& outX, float& outY)
{
    // Assumes (normalX, normalY) is a unit vector.
    float d = dot(inX, inY, normalX, normalY);
    outX = inX - 2.0f * d * normalX;
    outY = inY - 2.0f * d * normalY;
}

float smoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaSec)
{
    // Based on Game Programming Gems 4, Chapter 1.10 (critically damped spring).
    if (deltaSec <= 0.0f) return current;
    smoothTime = (smoothTime < 0.0001f) ? 0.0001f : smoothTime;
    float omega = 2.0f / smoothTime;

    float x = omega * deltaSec;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change = current - target;
    float originalTarget = target;

    float maxChange = maxSpeed * smoothTime;
    change = clamp(change, -maxChange, maxChange);
    target = current - change;

    float temp = (currentVelocity + omega * change) * deltaSec;
    currentVelocity = (currentVelocity - omega * temp) * exp;

    float output = target + (change + temp) * exp;

    // Prevent overshooting past the original target.
    if ((originalTarget - current > 0.0f) == (output > originalTarget))
    {
        output = originalTarget;
        currentVelocity = (output - originalTarget) / deltaSec;
    }

    return output;
}

} // namespace Math
} // namespace PRUZEA

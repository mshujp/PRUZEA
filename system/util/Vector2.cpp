#include "PRUZEA.h"
#include <cassert>

namespace PRUZEA
{

Vector2::Vector2(float x, float y) : x(x), y(y)
{
}

float Vector2::length() const
{
    return Math::length(x, y);
}

Vector2 Vector2::normalized() const
{
    float nx = x;
    float ny = y;
    Math::normalize(nx, ny);
    return Vector2(nx, ny);
}

float Vector2::dot(const Vector2& other) const
{
    return Math::dot(x, y, other.x, other.y);
}

float Vector2::cross(const Vector2& other) const
{
    return x * other.y - y * other.x;
}

float Vector2::distance(const Vector2& other) const
{
    return Math::distance(x, y, other.x, other.y);
}

Vector2 Vector2::operator+(const Vector2& other) const
{
    return Vector2(x + other.x, y + other.y);
}

Vector2 Vector2::operator-(const Vector2& other) const
{
    return Vector2(x - other.x, y - other.y);
}

Vector2& Vector2::operator+=(const Vector2& other)
{
    x += other.x;
    y += other.y;
    return *this;
}

Vector2& Vector2::operator-=(const Vector2& other)
{
    x -= other.x;
    y -= other.y;
    return *this;
}

Vector2 Vector2::operator*(float scalar) const
{
    return Vector2(x * scalar, y * scalar);
}

Vector2 Vector2::operator/(float scalar) const
{
    assert(scalar != 0.0f);
    return Vector2(x / scalar, y / scalar);
}

Vector2& Vector2::operator*=(float scalar)
{
    x *= scalar;
    y *= scalar;
    return *this;
}

Vector2& Vector2::operator/=(float scalar)
{
    assert(scalar != 0.0f);
    x /= scalar;
    y /= scalar;
    return *this;
}

Vector2 operator*(float scalar, const Vector2& vector)
{
    return Vector2(vector.x * scalar, vector.y * scalar);
}

} // namespace PRUZEA

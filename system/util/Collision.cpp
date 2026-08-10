#include "PRUZEA.h"
#include <cmath>

using namespace PRUZEA;

bool Collision::pointRect(float px, float py, float rx, float ry, float rw, float rh)
{
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

bool Collision::rectRect(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
{
    return (ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by);
}

bool Collision::rectRect(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh, Vector2& pushOut)
{
    if (!rectRect(ax, ay, aw, ah, bx, by, bw, bh))
    {
        return false;
    }

    float overlapX = (ax + aw < bx + bw ? ax + aw : bx + bw) - (ax > bx ? ax : bx);
    float overlapY = (ay + ah < by + bh ? ay + ah : by + bh) - (ay > by ? ay : by);

    float aCenterX = ax + aw * 0.5f;
    float aCenterY = ay + ah * 0.5f;
    float bCenterX = bx + bw * 0.5f;
    float bCenterY = by + bh * 0.5f;

    if (overlapX < overlapY)
    {
        float sign = (aCenterX < bCenterX) ? -1.0f : 1.0f;
        pushOut.x = overlapX * sign;
        pushOut.y = 0.0f;
    }
    else
    {
        float sign = (aCenterY < bCenterY) ? -1.0f : 1.0f;
        pushOut.x = 0.0f;
        pushOut.y = overlapY * sign;
    }

    return true;
}

bool Collision::circleCircle(float ax, float ay, float ar, float bx, float by, float br)
{
    float dx = ax - bx;
    float dy = ay - by;
    float distanceSq = (dx * dx) + (dy * dy);
    float radiusSum = ar + br;
    
    return distanceSq <= (radiusSum * radiusSum);
}

bool Collision::circleCircle(float ax, float ay, float ar, float bx, float by, float br, Vector2& pushOut)
{
    if (!circleCircle(ax, ay, ar, bx, by, br))
    {
        return false;
    }

    float dx = ax - bx;
    float dy = ay - by;
    float dist = std::sqrt(dx * dx + dy * dy);
    float radiusSum = ar + br;
    float overlap = radiusSum - dist;

    if (dist > 0.0f)
    {
        pushOut.x = (dx / dist) * overlap;
        pushOut.y = (dy / dist) * overlap;
    }
    else
    {
        // Circles share the exact same center: push along an arbitrary axis.
        pushOut.x = overlap;
        pushOut.y = 0.0f;
    }

    return true;
}

bool Collision::circleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh)
{
    float closestX = cx;
    if (closestX < rx)         closestX = rx;
    else if (closestX > rx + rw) closestX = rx + rw;

    float closestY = cy;
    if (closestY < ry)         closestY = ry;
    else if (closestY > ry + rh) closestY = ry + rh;

    float dx = cx - closestX;
    float dy = cy - closestY;
    float distanceSq = (dx * dx) + (dy * dy);

    return distanceSq <= (radius * radius);
}

bool Collision::circleRect(float cx, float cy, float radius, float rx, float ry, float rw, float rh, Vector2& pushOut)
{
    float closestX = cx;
    if (closestX < rx)           closestX = rx;
    else if (closestX > rx + rw) closestX = rx + rw;

    float closestY = cy;
    if (closestY < ry)           closestY = ry;
    else if (closestY > ry + rh) closestY = ry + rh;

    float dx = cx - closestX;
    float dy = cy - closestY;
    float distanceSq = (dx * dx) + (dy * dy);

    if (distanceSq > (radius * radius))
    {
        return false;
    }

    if (distanceSq > 0.0f)
    {
        // Circle center is outside the rect: push directly away from the closest edge point.
        float dist = std::sqrt(distanceSq);
        float overlap = radius - dist;
        pushOut.x = (dx / dist) * overlap;
        pushOut.y = (dy / dist) * overlap;
    }
    else
    {
        // Circle center is inside the rect: push out toward the nearest side.
        float leftDist   = cx - rx;
        float rightDist  = (rx + rw) - cx;
        float topDist    = cy - ry;
        float bottomDist = (ry + rh) - cy;

        float minDist = leftDist;
        pushOut.x = -(leftDist + radius);
        pushOut.y = 0.0f;

        if (rightDist < minDist)
        {
            minDist = rightDist;
            pushOut.x = rightDist + radius;
            pushOut.y = 0.0f;
        }
        if (topDist < minDist)
        {
            minDist = topDist;
            pushOut.x = 0.0f;
            pushOut.y = -(topDist + radius);
        }
        if (bottomDist < minDist)
        {
            minDist = bottomDist;
            pushOut.x = 0.0f;
            pushOut.y = bottomDist + radius;
        }
    }

    return true;
}

bool Collision::pointCircle(float px, float py, float cx, float cy, float radius)
{
    float dx = px - cx;
    float dy = py - cy;
    return (dx * dx + dy * dy) <= (radius * radius);
}

bool Collision::lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    float denominator = ((x2 - x1) * (y4 - y3)) - ((y2 - y1) * (x4 - x3));
    if (denominator == 0.0f)
    {
        return false;
    }

    float uA = (((x4 - x3) * (y1 - y3)) - ((y4 - y3) * (x1 - x3))) / denominator;
    float uB = (((x2 - x1) * (y1 - y3)) - ((y2 - y1) * (x1 - x3))) / denominator;

    return (uA >= 0.0f && uA <= 1.0f && uB >= 0.0f && uB <= 1.0f);
}

bool Collision::lineRect(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh)
{
    bool left   = lineLine(x1, y1, x2, y2, rx,      ry,      rx,      ry + rh);
    bool right  = lineLine(x1, y1, x2, y2, rx + rw, ry,      rx + rw, ry + rh);
    bool top    = lineLine(x1, y1, x2, y2, rx,      ry,      rx + rw, ry);
    bool bottom = lineLine(x1, y1, x2, y2, rx,      ry + rh, rx + rw, ry + rh);

    if (left || right || top || bottom)  return true;
    if (pointRect(x1, y1, rx, ry, rw, rh) || pointRect(x2, y2, rx, ry, rw, rh)) return true;

    return false;
}

bool Collision::lineCircle(float x1, float y1, float x2, float y2, float cx, float cy, float radius)
{
    float dx = x2 - x1, dy = y2 - y1;
    float len2 = dx * dx + dy * dy;
    float t = len2 > 0.0f ? ((cx - x1) * dx + (cy - y1) * dy) / len2 : 0.0f;
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    float closestX = x1 + t * dx;
    float closestY = y1 + t * dy;
    float ddx = cx - closestX, ddy = cy - closestY;
    return (ddx * ddx + ddy * ddy) <= (radius * radius);
}

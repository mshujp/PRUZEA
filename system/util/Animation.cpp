#include "PRUZEA.h"
#include <cmath>

namespace PRUZEA
{

Animation::Animation(float duration, int totalFrames, bool loop) : 
    currentTime(0.0f), duration(duration), totalFrames(totalFrames), playing(false), loop(loop)
{
}

void Animation::start()
{
    currentTime = 0.0f;
    playing = true;
}

void Animation::stop()
{
    playing = false;
}

void Animation::update(float deltaSec)
{
    if (!playing) return;

    currentTime += deltaSec;

    if (currentTime >= duration)
    {
        if (loop)
        {
            // Wrap around, preserving overshoot so timing stays accurate
            // even if a frame takes longer than `duration`.
            currentTime = (duration > 0.0f) ? fmodf(currentTime, duration) : 0.0f;
        }
        else
        {
            currentTime = duration;
            playing = false;
        }
    }
}

void Animation::reset()
{
    currentTime = 0.0f;
}

bool Animation::isPlaying() const
{
    return playing;
}

float Animation::progress() const
{
    if (duration <= 0.0f) return 0.0f;

    float p = currentTime / duration;
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;

    return p;
}

bool Animation::isFinished() const
{
    return !loop && currentTime >= duration;
}

int Animation::frame() const
{
    if (duration <= 0.0f || totalFrames <= 0) return 0;

    int f = static_cast<int>((currentTime / duration) * static_cast<float>(totalFrames));

    if (f >= totalFrames)
    {
        f = loop ? (f % totalFrames) : (totalFrames - 1);
    }
    if (f < 0) f = 0;

    return f;
}

} // namespace PRUZEA
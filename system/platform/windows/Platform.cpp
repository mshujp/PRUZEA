#include "Platform.h"
#include "PRUZEA.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <random>
#include <thread>

using namespace PRUZEA;

namespace
{
const auto START_TIME = std::chrono::steady_clock::now();
}

void Debug::log(const char* format, ...)
{
    if (format == nullptr) return;

    va_list arguments;
    va_start(arguments, format);
    std::vprintf(format, arguments);
    va_end(arguments);
    std::fflush(stdout);
}

uint32_t Platform::random32()
{
    static thread_local std::mt19937 generator(std::random_device{}());
    return generator();
}

uint32_t Platform::getMsec()
{
    const auto elapsed = std::chrono::steady_clock::now() - START_TIME;
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

uint64_t Platform::getUsec()
{
    const auto elapsed = std::chrono::steady_clock::now() - START_TIME;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
}

void Platform::sleepMsec(uint32_t msec)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

void Platform::sleepUsec(uint32_t usec)
{
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
}

bool Platform::elapsed(uint32_t now, uint32_t startMsec, uint32_t durationMsec)
{
    return static_cast<uint32_t>(now - startMsec) >= durationMsec;
}

#include "DebugTimer.h"

#include "External/pow2assert.h"

#include <windows.h>

#include <map>
#include <stdio.h>
#include <string>

static std::map<std::string, LARGE_INTEGER> g_timers;

void DebugTimer_Tic(const char* name)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    g_timers[std::string(name)] = counter;
}

double DebugTimer_Toc(const char* name)
{
    auto timer = g_timers.find(name);
    POW2_ASSERT(timer != g_timers.end());

    LARGE_INTEGER before = timer->second;
    LARGE_INTEGER after;
    QueryPerformanceCounter(&after);

    // TODO(manuel): This value can be cached when the app starts. Is it worth it?
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LONGLONG elapsed = after.QuadPart - before.QuadPart;
    double elapsedMs = double(elapsed) / double(freq.QuadPart) * 1000;

    return elapsedMs;
}

void DebugTimer_TocAndPrint(const char* name)
{
    char str[128];
    _snprintf(
        str, sizeof(str), "DebugTimer: %s elapsed = %.02fms\n", name, DebugTimer_Toc(name));
    OutputDebugStringA(str);
}

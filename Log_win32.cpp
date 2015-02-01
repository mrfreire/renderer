#include "Log.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

static void Print(const char* prefix, const char* suffix, const char* fmt, va_list* args)
{
	OutputDebugStringA(prefix);

	char tmp[512];  // TODO(manuel): Good enough for now, but handle this better
	vsnprintf(tmp, sizeof(tmp), fmt, *args);
	OutputDebugStringA(tmp);

	OutputDebugStringA(suffix);
}

void Log::Debug(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Print("", "\n", fmt, &args);
	va_end(args);
}

void Log::Warning(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Print("WARNING: ", "\n", fmt, &args);
	va_end(args);
}

void Log::Error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Print("ERROR: ", "\n", fmt, &args);
	va_end(args);
	exit(1);
}

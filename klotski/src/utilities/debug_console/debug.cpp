#include "debug.hpp"

#include <Windows.h>
#include <cstdarg>
#include <cstdio>


void debug::initialize(const wchar_t* console_title)
{
	AllocConsole();

	freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(0)), "conin$", "r", static_cast<_iobuf*>(__acrt_iob_func(0)));
	freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(1)), "conout$", "w", static_cast<_iobuf*>(__acrt_iob_func(1)));
	freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(2)), "conout$", "w", static_cast<_iobuf*>(__acrt_iob_func(2)));

	SetConsoleTitle(console_title);
}

void debug::release()
{
	fclose(static_cast<_iobuf*>(__acrt_iob_func(0)));
	fclose(static_cast<_iobuf*>(__acrt_iob_func(1)));
	fclose(static_cast<_iobuf*>(__acrt_iob_func(2)));

	FreeConsole();
}

void debug::log(const wchar_t* message...)
{
	va_list arguments;
	va_start(arguments, message);
	vwprintf_s(message, arguments);
	va_end(arguments);
}

void debug::log(const char* message...)
{
	va_list arguments;
	va_start(arguments, message);
	vprintf_s(message, arguments);
	va_end(arguments);
}

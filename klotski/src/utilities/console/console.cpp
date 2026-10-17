#include "console.hpp"

#include <Windows.h>

#include <cstdarg>
#include <cstdio>

namespace
{
	bool g_console_allocated = false;

	using color_message_t = void(__cdecl*)(const color_t&, const char*, ...);

	color_message_t color_message()
	{
		static const color_message_t function = []() -> color_message_t {
			HMODULE tier0 = GetModuleHandleA("tier0.dll");

			if (!tier0)
				return nullptr;

			return reinterpret_cast<color_message_t>(GetProcAddress(tier0, "?ConColorMsg@@YAXAEBVColor@@PEBDZZ"));
		}();

		return function;
	}

	void write(color_t color, const char* message, va_list arguments)
	{
		if (!message)
			return;

		char buffer[2048] = { };

		if (_vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, message, arguments) <= 0 && buffer[0] == '\0')
			return;

		if (const color_message_t function = color_message())
			function(color, "%s", buffer);

		OutputDebugStringA(buffer);

		if (g_console_allocated)
		{
			if (color.r == 255 && color.g == 255 && color.b == 255)
				std::fputs(buffer, stdout);
			else
				std::fprintf(stdout, "\x1b[38;2;%u;%u;%um%s\x1b[0m", color.r, color.g, color.b, buffer);

			std::fflush(stdout);
		}
	}
}

void console::initialize()
{
	if (g_console_allocated)
		return;

	if (GetConsoleWindow())
	{
		g_console_allocated = true;
		return;
	}

	if (!AllocConsole())
		return;

	SetConsoleTitleA("klotski | debug console");

	FILE* stream = nullptr;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);
	freopen_s(&stream, "CONIN$", "r", stdin);

	if (const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE))
	{
		DWORD mode = 0;

		if (GetConsoleMode(output, &mode))
			SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
	}

	if (const HANDLE input = GetStdHandle(STD_INPUT_HANDLE))
	{
		DWORD mode = 0;

		if (GetConsoleMode(input, &mode))
			SetConsoleMode(input, mode | ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE);
	}

	g_console_allocated = true;
}

void console::log(const char* message...)
{
	va_list arguments;
	va_start(arguments, message);

	write(color_t(255, 255, 255), message, arguments);

	va_end(arguments);
}

void console::console_color_msg(color_t color, const char* message...)
{
	va_list arguments;
	va_start(arguments, message);

	write(color, message, arguments);

	va_end(arguments);
}

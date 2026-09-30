#include "sdl.hpp"

#include <Windows.h>

static void* get_sdl_proc(const char* name)
{
	static HMODULE module = GetModuleHandleA("SDL3.dll");

	if (!module)
	{
		return nullptr;
	}

	return reinterpret_cast<void*>(GetProcAddress(module, name));
}

void sdl::set_relative_mouse_mode(bool mode)
{
	using function_t = std::int32_t(__stdcall*)(bool);
	static function_t fn = reinterpret_cast<function_t>(get_sdl_proc("SDL_SetRelativeMouseMode"));

	if (!fn)
	{
		return;
	}

	fn(mode);
}

void sdl::set_window_polling_mode(void* window, bool mode)
{
	using function_t = void(__stdcall*)(void*, bool);
	static function_t fn = reinterpret_cast<function_t>(get_sdl_proc("SDL_SetWindowMouseGrab"));

	if (!fn)
	{
		fn = reinterpret_cast<function_t>(get_sdl_proc("SDL_SetWindowGrab"));
	}

	if (!fn || !window)
	{
		return;
	}

	fn(window, mode);
}

void sdl::set_mouse_warp_position(void* window, std::int32_t x, std::int32_t y)
{
	using function_t = std::int32_t(__stdcall*)(void*, float, float);
	static function_t fn = reinterpret_cast<function_t>(get_sdl_proc("SDL_WarpMouseInWindow"));

	if (!fn || !window)
	{
		return;
	}

	fn(window, static_cast<float>(x), static_cast<float>(y));
}

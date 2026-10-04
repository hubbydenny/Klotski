#pragma once

#include <cstdint>
#include <utility>
#include <dxgi.h>

class c_view_setup;
namespace hooks
{
	bool initialize();
	void release();

	namespace frame_stage_notify
	{
		using function_t = std::int64_t(__fastcall*)(std::int64_t, std::int32_t);
		std::int64_t __fastcall hook(std::int64_t a1, std::int32_t a2);
	}

	namespace create_move
	{
		using function_t = bool(__fastcall*)(void*, std::uint32_t, std::uint8_t);
		bool __fastcall hook(void* a1, std::uint32_t a2, std::uint8_t a3);
	}

	namespace swap_chain_present
	{
		using function_t = HRESULT(__fastcall*)(IDXGISwapChain*, std::uint32_t, std::uint32_t);
		HRESULT __fastcall hook(IDXGISwapChain* swap_chain, std::uint32_t sync_interval, std::uint32_t flags);
	}

	namespace swap_chain_resize_buffers
	{
		using function_t = HRESULT(__fastcall*)(IDXGISwapChain*, std::uint32_t, std::uint32_t, std::uint32_t, DXGI_FORMAT, std::uint32_t);
		HRESULT __fastcall hook(IDXGISwapChain* swap_chain, std::uint32_t buffer_count, std::uint32_t width, std::uint32_t height, DXGI_FORMAT new_format, std::uint32_t swap_chain_flags);
	}

	namespace override_view
	{
		using function_t = void(__fastcall*)(void*, c_view_setup*);
		void __fastcall hook(void* client_mode, c_view_setup* view_setup);
	}
	namespace chat {
		enum color_t : std::uint32_t
		{
			color_white = 1,
			color_red = 2,
			color_light_purple = 3,
			color_green = 4,
			color_light_green = 5,
			color_light_green_2 = 6,
			color_light_red = 7,
			color_gray = 8,
			color_light_yellow = 9,
			color_gray_blue = 10,
			color_light_blue = 11,
			color_light_blue_2 = 12,
			color_light_purple_2 = 13,
			color_light_red_2 = 14,
			color_orange = 15,
			color_orange_2 = 16,
		};

		void chatprintf(const char* text);
		void chatprintf_color(color_t color, const char* text);
	}
	namespace mouse_input
	{
		using function_t = bool(__fastcall*)(std::int64_t);
		bool __fastcall hook(std::int64_t a1);
	}
	namespace skyboxcolors {
		using function_t = void(__fastcall*)(__int64, __int64, __int64, int, int, __int64, __int64);
		inline function_t oskybox = nullptr;
		void __fastcall hook(__int64 this_ptr, __int64 render_ctx, __int64 primitive, int count, int render_flags, __int64 view_info, __int64 render_stats);
	}
	namespace draw_glow
	{
		using function_t = void*(__fastcall*)(void*);
		void* __fastcall hook(void* glow_property);
	}
	namespace window_procedure
	{
		using function_t = LRESULT(__stdcall*)(HWND, std::uint32_t, WPARAM, LPARAM);
		LRESULT __stdcall hook(HWND hwnd, std::uint32_t message, WPARAM wparam, LPARAM lparam);
	}
}
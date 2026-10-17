#pragma once

#include <cstdint>
#include <utility>
#include <dxgi.h>

class c_view_setup;
class i_csgo_input;
class i_input_system;
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
	namespace mouse_input
	{
		using function_t = bool(__fastcall*)(i_csgo_input*);
		bool __fastcall hook(i_csgo_input* input);
	}
	namespace relative_mouse
	{
		using function_t = void*(__fastcall*)(i_input_system*, bool);
		void* __fastcall hook(i_input_system* input, bool enabled);
	}
	namespace skyboxcolors {
		bool initialize();
		void run();
		void release();
	}
	namespace draw_glow
	{
		using function_t = void*(__fastcall*)(void*);
		void* __fastcall hook(void* glow_property);
	}
	namespace generate_primitives
	{
		using function_t = std::int64_t(__fastcall*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
		std::int64_t __fastcall hook(std::uintptr_t thisptr, std::uintptr_t scene_object, std::uintptr_t scene_view, std::uintptr_t primitive_buffer);
	}
	namespace viewmodel_offsets
	{
		using function_t = void(__fastcall*)(void*, float*, float*);
		void __fastcall hook(void* viewmodel, float* offsets, float* fov);
	}
	namespace window_procedure
	{
		using function_t = LRESULT(__stdcall*)(HWND, std::uint32_t, WPARAM, LPARAM);
		LRESULT __stdcall hook(HWND hwnd, std::uint32_t message, WPARAM wparam, LPARAM lparam);
	}
}


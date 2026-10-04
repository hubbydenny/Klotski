#include "hooks.hpp"

#include <cstdio>
#include <cstring>
#include <cstddef>
#include <algorithm>
#include "../signatures.hpp"

#include "../utilities/minhook/MinHook.h"
#include "../utilities/utilities.hpp"
#include "../source2-sdk/sdk.hpp"
#include "../features/visuals/visuals.hpp"

#include "../features/misc/misc.hpp"
#include "../features/combat/combat.hpp"
#include "../features/movement/movement.hpp"
#include "../features/movement/prediction/prediction.hpp"
#include "../features/menu/menu.hpp"
#include "../features/config/config.hpp"
#include "../features/config/binds.hpp"
#include "../source2-sdk/classes/viewsetup.hpp"

#include "../utilities/imgui/imgui.h"
#include "../utilities/imgui/imgui_impl_win32.h"
#include "../utilities/imgui/imgui_impl_dx11.h"


#include <d3d11.h>
#include <dxgi.h>
#include <chrono>
#include <thread>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static hooks::frame_stage_notify::function_t frame_stage_notify_original = nullptr;
static hooks::create_move::function_t create_move_original = nullptr;
static hooks::swap_chain_present::function_t swap_chain_present_original = nullptr;
static hooks::swap_chain_resize_buffers::function_t swap_chain_resize_buffers_original = nullptr;
static hooks::override_view::function_t override_view_original = nullptr;
static hooks::mouse_input::function_t mouse_input_original = nullptr;
static hooks::draw_glow::function_t draw_glow_original = nullptr;


static hooks::window_procedure::function_t window_procedure_original = nullptr;

static ID3D11Device* device = nullptr;
static ID3D11DeviceContext* context = nullptr;
static HWND window = nullptr;
static ID3D11RenderTargetView* render_view = nullptr;

bool hooks::initialize()
{
	if (MH_Initialize() != MH_OK)
	{
		debug::log(L"[-] failed to initialize minhook\n");
		return false;
	}

	void* frame_stage_notify_target = utilities::pattern_scan(L"client.dll", FRAME_STAGE_NOTIFY);
	void* create_move_target = utilities::pattern_scan(L"client.dll", CREATE_MOVE);
	void* override_view_target = utilities::scan_function(L"client.dll", OVERRIDE_VIEW);
	void* mouse_input_target = utilities::pattern_scan(L"client.dll", MOUSE_INPUT_PATTERN);
	void* draw_glow_target = utilities::pattern_scan(L"client.dll", DRAW_GLOW_PATTERN);
	void* skybox_target = utilities::pattern_scan(L"scenesystem.dll", SKYBOXPAT);
	

	
	
	

	for (std::int32_t i = 0; i < 3000 && !GetModuleHandle(L"gameoverlayrenderer64.dll"); i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	void* swap_chain_present_target = utilities::pattern_scan(L"gameoverlayrenderer64.dll", PRESENT_PATTERN);
	void* swap_chain_resize_buffers_target = utilities::pattern_scan(L"gameoverlayrenderer64.dll", RESIZE_BUFFERS_PATTERN);

	if (!frame_stage_notify_target) debug::log(L"[-] failed: FRAME_STAGE_NOTIFY\n");
	if (!create_move_target) debug::log(L"[-] failed: CREATE_MOVE\n");
	if (!override_view_target) debug::log(L"[-] failed: OVERRIDE_VIEW\n");
	if (!mouse_input_target) debug::log(L"[-] failed: MOUSE_INPUT\n");
	

	if (!draw_glow_target) debug::log(L"[-] failed: DRAW_GLOW\n");
	if (!skybox_target) debug::log(L"[-] failed: SKYBOXPAT\n");
	
	if (!swap_chain_present_target) debug::log(L"[-] failed: PRESENT\n");
	if (!swap_chain_resize_buffers_target) debug::log(L"[-] failed: RESIZE_BUFFERS\n");

	if (!frame_stage_notify_target || !create_move_target || !swap_chain_present_target || !swap_chain_resize_buffers_target)
	{
		debug::log(L"[-] failed to initialize hooks\n");
		return false;
	}

	if (MH_CreateHook(frame_stage_notify_target, &hooks::frame_stage_notify::hook, reinterpret_cast<void**>(&frame_stage_notify_original)) != MH_OK)
	{
		debug::log(L"[-] failed to hook frame stage notify\n");
		return false;
	}

	if (MH_CreateHook(create_move_target, &hooks::create_move::hook, reinterpret_cast<void**>(&create_move_original)) != MH_OK)
	{
		debug::log(L"[-] failed to hook create move\n");
		return false;
	}

	if (MH_CreateHook(swap_chain_present_target, &hooks::swap_chain_present::hook, reinterpret_cast<void**>(&swap_chain_present_original)) != MH_OK)
	{
		debug::log(L"[-] failed to hook present\n");
		return false;
	}

	if (MH_CreateHook(swap_chain_resize_buffers_target, &hooks::swap_chain_resize_buffers::hook, reinterpret_cast<void**>(&swap_chain_resize_buffers_original)) != MH_OK)
	{
		debug::log(L"[-] failed to hook resize buffers\n");
		return false;
	}

	if (override_view_target && MH_CreateHook(override_view_target, &hooks::override_view::hook, reinterpret_cast<void**>(&override_view_original)) != MH_OK)
	{
		debug::log(L"[-] failed to hook override view\n");
		return false;
	}

	if (mouse_input_target && MH_CreateHook(mouse_input_target, &hooks::mouse_input::hook, reinterpret_cast<void**>(&mouse_input_original)) != MH_OK)
	{
		debug::log(L"[-] failed to hook mouse input\n");
		return false;
	}

	if (!draw_glow_target)
	{
		debug::log(L"[-] failed: DRAW_GLOW\n");
	}
	else
	{
		std::uint8_t* scan = reinterpret_cast<std::uint8_t*>(draw_glow_target);
		std::int32_t matches = 0;

		while (scan && matches < 4)
		{
			if (MH_CreateHook(scan, &hooks::draw_glow::hook, reinterpret_cast<void**>(&draw_glow_original)) == MH_OK)
			{
				matches++;
			}

			scan = utilities::pattern_scan_next(scan);
		}
	}

	if (skybox_target && MH_CreateHook(skybox_target, &hooks::skyboxcolors::hook, reinterpret_cast<void**>(&hooks::skyboxcolors::oskybox)) != MH_OK)
	{
		debug::log(L"[-] failed to hook skybox\n");
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		debug::log(L"[-] failed to enable hooks\n");
		return false;
	}

	debug::log(L"[+] hooks initialized\n");
	return true;
}


void hooks::release()
{
	if (window && window_procedure_original)
	{
		SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(window_procedure_original));
	}

	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

std::int64_t __fastcall hooks::frame_stage_notify::hook(std::int64_t a1, std::int32_t a2)
{
	const std::int64_t result = frame_stage_notify_original(a1, a2);

	return result;
}

bool __fastcall hooks::create_move::hook(void* a1, std::uint32_t a2, std::uint8_t a3)
{
	interfaces::csgo_input = reinterpret_cast<i_csgo_input*>(a1);

	const bool result = create_move_original(a1, a2, a3);

	sdk::update_local_controller();
	sdk::update_local_player();

	binds::run(menu::open);

	if (sdk::local_controller)
	{
		user_cmd_t* user_cmd = interfaces::csgo_input->get_user_cmd(sdk::local_controller);

		if (config::context.bhop)
		{
			movement::run_bhop(user_cmd);
		}

		if (config::context.pixelsurf)
		{
			movement::pixelsurf(user_cmd);
		}

		prediction::start(user_cmd);
		{
			if (config::context.standalone_rcs)
			{
				combat::run_rcs();
			}

			if (config::context.aimbot && sdk::local_controller)
			{
				combat::run_aimbot(sdk::local_controller);
			}
		}
		prediction::end(user_cmd);
	}

	return result;
}

HRESULT __fastcall hooks::swap_chain_present::hook(IDXGISwapChain* swap_chain, std::uint32_t sync_interval, std::uint32_t flags)
{
	if (!device)
	{
		ID3D11Texture2D* buffer = nullptr;

		swap_chain->GetBuffer(0, IID_PPV_ARGS(&buffer));

		if (buffer)
		{
			swap_chain->GetDevice(IID_PPV_ARGS(&device));
			device->CreateRenderTargetView(buffer, nullptr, &render_view);
			device->GetImmediateContext(&context);

			DXGI_SWAP_CHAIN_DESC desc = { };
			swap_chain->GetDesc(&desc);
			window = desc.OutputWindow;

			buffer->Release();
		}

		if (window_procedure_original && window)
		{
			SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(window_procedure_original));
			window_procedure_original = nullptr;
		}

		if (window)
		{
			window_procedure_original = reinterpret_cast<decltype(window_procedure_original)>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hooks::window_procedure::hook)));
		}

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX11_Init(device, context);

		ImFontConfig config;
		config.PixelSnapH = true;

		io.Fonts->AddFontDefault(&config);
		io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdanab.ttf", 13.f, &config);

		ImGui_ImplDX11_CreateDeviceObjects();
	}

	sdk::update_screen_size();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	visuals::run_player_esp();
	misc::run();
	menu::render();

	ImGui::Render();

	context->OMSetRenderTargets(1, &render_view, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return swap_chain_present_original(swap_chain, sync_interval, flags);
}

HRESULT __fastcall hooks::swap_chain_resize_buffers::hook(IDXGISwapChain* swap_chain, std::uint32_t buffer_count, std::uint32_t width, std::uint32_t height, DXGI_FORMAT new_format, std::uint32_t swap_chain_flags)
{
	if (render_view)
	{
		render_view->Release();
		render_view = nullptr;
	}

	if (context)
	{
		context->Release();
		context = nullptr;
	}

	if (device)
	{
		device->Release();
		device = nullptr;
	}

	ImGui_ImplDX11_Shutdown();
	ImGui::DestroyContext();

	return swap_chain_resize_buffers_original(swap_chain, buffer_count, width, height, new_format, swap_chain_flags);
}

namespace {
	constexpr std::uint32_t k_color_slot_min = 1;
	constexpr std::uint32_t k_color_slot_max = 16;

	const char* color_escape(std::uint32_t slot)
	{
		static const char* escapes[] = {
			"\x01", "\x01", "\x02", "\x03", "\x04", "\x05", "\x06", "\x07", "\x08",
			"\x09", "\x0a", "\x0b", "\x0c", "\x0d", "\x0e", "\x0f", "\x10"
		};

		if (slot < k_color_slot_min) slot = k_color_slot_min;
		if (slot > k_color_slot_max) slot = k_color_slot_max;

		return escapes[slot];
	}
}

// won't working i'll recode it

void hooks::chat::chatprintf_color(color_t color, const char* text)
{
	if (!text || !text[0]) return;

	using get_client_t = std::int64_t(__fastcall*)();
	using print_t = void(__fastcall*)(std::int64_t, std::uint32_t, const char*);

	static get_client_t get_client = reinterpret_cast<get_client_t>(utilities::scan_function(L"client.dll", LOCAL_QWORD_CHAT));
	static print_t print_to_chat = reinterpret_cast<print_t>(utilities::scan_function(L"client.dll", LOCAL_PRINT_CHAT));

	if (!get_client || !print_to_chat) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;

	const std::int64_t client = get_client();
	if (!utilities::is_valid_pointer(reinterpret_cast<void*>(client))) return;

	char buffer[1024];
	std::snprintf(buffer, sizeof(buffer), "%s%s", color_escape(static_cast<std::uint32_t>(color)), text);

	print_to_chat(client, 0xFFFFFFFFu, buffer);
}

void hooks::chat::chatprintf(const char* text)
{
	chatprintf_color(color_red, text);
}

void __fastcall hooks::override_view::hook(void* client_mode, c_view_setup* view_setup)
{
	override_view_original(client_mode, view_setup);

	if (!view_setup || !config::context.fov_changer) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;
	if (!config::context.scopefov) { if (sdk::local_player->is_scoped()) return; }
		
	view_setup->fov = config::context.fov;
}

void* __fastcall hooks::draw_glow::hook(void* glow_property)
{
	void* result = draw_glow_original(glow_property);

	if (interfaces::engine->is_in_game())
	{
		visuals::on_draw_glow(glow_property);
	}

	return result;
}

bool __fastcall hooks::mouse_input::hook(std::int64_t a1) {
	if (menu::open) return false;
	return mouse_input_original(a1);
}

void __fastcall hooks::skyboxcolors::hook(__int64 this_ptr, __int64 render_ctx, __int64 primitive, int count, int render_flags, __int64 view_info, __int64 render_stats) {
	if (config::context.skybox && count > 0)
	{
		const std::uintptr_t skybox_data_add = 0x68 * static_cast<unsigned long long>(count) + static_cast<unsigned long long>(primitive) - 0x50;

		auto* skybox_data = reinterpret_cast<float*>(*reinterpret_cast<std::uintptr_t*>(skybox_data_add));

		if (skybox_data)
		{
			skybox_data[0] = config::context.skybox_color[0];
			skybox_data[1] = config::context.skybox_color[1];
			skybox_data[2] = config::context.skybox_color[2];
		}
	}

	oskybox(this_ptr, render_ctx, primitive, count, render_flags, view_info, render_stats);
}

LRESULT __stdcall hooks::window_procedure::hook(HWND hwnd, std::uint32_t message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_KEYDOWN && LOWORD(wparam) == VK_INSERT) { menu::open = !menu::open; }
	ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam);
	if (menu::open) {
		switch (message) {
			case WM_MOUSEMOVE:
			case WM_NCMOUSEMOVE:
			case WM_MOUSELEAVE:
			case WM_NCMOUSELEAVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONDBLCLK:
			case WM_XBUTTONDOWN:
			case WM_XBUTTONDBLCLK:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_XBUTTONUP:
			case WM_MOUSEWHEEL:
			case WM_MOUSEHWHEEL:
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_SETFOCUS:
			case WM_KILLFOCUS:
			case WM_CHAR:
			case WM_DEVICECHANGE:
				return 1;
		}
	}
	return CallWindowProc(window_procedure_original, hwnd, message, wparam, lparam);
}


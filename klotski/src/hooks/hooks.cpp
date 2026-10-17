#include "hooks.hpp"

#include <cstdio>
#include <cstring>
#include <cstddef>
#include <algorithm>
#include <array>
#include <d3d11.h>
#include <dxgi.h>
#include <chrono>
#include <thread>


#include "../utilities/signatures.hpp"
#include "../utilities/minhook/MinHook.h"
#include "../utilities/utilities.hpp"
#include "../source2-sdk/sdk.hpp"
#include "../utilities/imgui/imgui.h"
#include "../utilities/imgui/imgui_impl_win32.h"
#include "../utilities/imgui/imgui_impl_dx11.h"

#include "../features/visuals/visuals.hpp"
#include "../features/misc/misc.hpp"
#include "../features/combat/combat.hpp"
#include "../features/movement/movement.hpp"
#include "../features/movement/prediction/prediction.hpp"
#include "../features/menu/menu.hpp"
#include "../features/config/config.hpp"
#include "../features/config/binds.hpp"

#include "../source2-sdk/interfaces/i_input_system.hpp"
#include "../source2-sdk/classes/viewsetup.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static hooks::frame_stage_notify::function_t frame_stage_notify_original = nullptr;
static hooks::create_move::function_t create_move_original = nullptr;
static hooks::swap_chain_present::function_t swap_chain_present_original = nullptr;
static hooks::swap_chain_resize_buffers::function_t swap_chain_resize_buffers_original = nullptr;
static hooks::override_view::function_t override_view_original = nullptr;
static hooks::mouse_input::function_t mouse_input_original = nullptr;
static hooks::relative_mouse::function_t relative_mouse_original = nullptr;
static hooks::draw_glow::function_t draw_glow_original = nullptr;
static bool(__fastcall* is_glowing_original)(void*) = nullptr;
static void(__fastcall* get_glow_color_original)(void*, float*) = nullptr;
static bool glow_queries_ready = false;

static bool __fastcall is_glowing_hook(void* property)
{
	if (visuals::glow_override(property, nullptr)) return true;
	return is_glowing_original(property);
}

static void __fastcall get_glow_color_hook(void* property, float* color)
{
	if (color && visuals::glow_override(property, color)) return;
	get_glow_color_original(property, color);
}
static hooks::generate_primitives::function_t generate_primitives_original = nullptr;
static hooks::viewmodel_offsets::function_t viewmodel_offsets_original = nullptr;
static hooks::window_procedure::function_t window_procedure_original = nullptr;

static ID3D11Device* device = nullptr;
static ID3D11DeviceContext* context = nullptr;
static HWND window = nullptr;
static ID3D11RenderTargetView* render_view = nullptr;

struct primitive_output_buffer_t
{
	std::uintptr_t fixed_data;
	std::int32_t fixed_capacity;
	std::int32_t fixed_count;
	std::int32_t overflow_count;
	std::uint32_t reserved;
	std::uintptr_t overflow_data;
	std::int32_t overflow_capacity;
	std::uint32_t overflow_flags;

	int count() const
	{
		if (fixed_count < 0 || overflow_count < 0 || fixed_capacity < 0 || overflow_capacity < 0) return -1;
		if (fixed_count > fixed_capacity || overflow_count > overflow_capacity) return -1;
		return fixed_count + overflow_count;
	}
};
bool hooks::initialize()
{
	if (MH_Initialize() != MH_OK)
	{
		console::log("[-] failed to initialize minhook\n");
		return false;
	}

	void* frame_stage_notify_target = utilities::pattern_scan(L"client.dll", FRAME_STAGE_NOTIFY);
	void* create_move_target = utilities::pattern_scan(L"client.dll", CREATE_MOVE);
	void* override_view_target = utilities::scan_function(L"client.dll", OVERRIDE_VIEW);
	void* mouse_input_target = interfaces::csgo_input ? (*reinterpret_cast<void***>(interfaces::csgo_input))[15] : nullptr;
	void* relative_mouse_target = interfaces::input_system ? (*reinterpret_cast<void***>(interfaces::input_system))[76] : nullptr;
	void* draw_glow_target = utilities::pattern_scan(L"client.dll", DRAW_GLOW_PATTERN);
	const char* generate_primitives_name = "animatable";
	void* generate_primitives_target = utilities::pattern_scan(L"scenesystem.dll", SCENE_ANIMATABLE_GENERATE_PRIMITIVES);
	if (!generate_primitives_target)
	{
		generate_primitives_name = "generic";
		generate_primitives_target = utilities::pattern_scan(L"scenesystem.dll", GENERATE_PRIMITIVES);
	}
	//if (generate_primitives_target) console::log("[?] chams: %s GeneratePrimitives target %p\n", generate_primitives_name, generate_primitives_target);
	void* viewmodel_offsets_target = utilities::pattern_scan(L"client.dll", GET_VIEW_MODEL_OFFSETS);
	for (std::int32_t i = 0; i < 3000 && !GetModuleHandle(L"gameoverlayrenderer64.dll"); i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	void* swap_chain_present_target = utilities::pattern_scan(L"gameoverlayrenderer64.dll", PRESENT_PATTERN);
	void* swap_chain_resize_buffers_target = utilities::pattern_scan(L"gameoverlayrenderer64.dll", RESIZE_BUFFERS_PATTERN);

	if (!frame_stage_notify_target) console::log("[-] failed: FRAME_STAGE_NOTIFY\n");
	if (!create_move_target) console::log("[-] failed: CREATE_MOVE\n");
	if (!override_view_target) console::log("[-] failed: OVERRIDE_VIEW\n");
	//if (!mouse_input_target) console::log("[-] failed: MOUSE_INPUT\n"); IT WORKS BUT SHOW ERROR!?
	if (!relative_mouse_target) console::log("[-] failed: RELATIVE_MOUSE\n");
	if (!viewmodel_offsets_target) console::log("[-] failed: GET_VIEW_MODEL_OFFSETS\n");
	if (!draw_glow_target) console::log("[-] failed: DRAW_GLOW\n");

if (!hooks::skyboxcolors::initialize()) console::log("[-] failed: SKYBOX_COLOR_PATTERN\n");
	
	if (!swap_chain_present_target) console::log("[-] failed: PRESENT\n");
	if (!swap_chain_resize_buffers_target) console::log("[-] failed: RESIZE_BUFFERS\n");
	if (!frame_stage_notify_target || !create_move_target || !swap_chain_present_target || !swap_chain_resize_buffers_target)
	{
		console::log("[-] failed to initialize hooks\n");
		return false;
	}

	if (MH_CreateHook(frame_stage_notify_target, &hooks::frame_stage_notify::hook, reinterpret_cast<void**>(&frame_stage_notify_original)) != MH_OK)
	{
		console::log("[-] failed to hook frame stage notify\n");
		return false;
	}

	if (MH_CreateHook(create_move_target, &hooks::create_move::hook, reinterpret_cast<void**>(&create_move_original)) != MH_OK)
	{
		console::log("[-] failed to hook create move\n");
		return false;
	}

	if (MH_CreateHook(swap_chain_present_target, &hooks::swap_chain_present::hook, reinterpret_cast<void**>(&swap_chain_present_original)) != MH_OK)
	{
		console::log("[-] failed to hook present\n");
		return false;
	}

	if (MH_CreateHook(swap_chain_resize_buffers_target, &hooks::swap_chain_resize_buffers::hook, reinterpret_cast<void**>(&swap_chain_resize_buffers_original)) != MH_OK)
	{
		console::log("[-] failed to hook resize buffers\n");
		return false;
	}

	if (override_view_target && MH_CreateHook(override_view_target, &hooks::override_view::hook, reinterpret_cast<void**>(&override_view_original)) != MH_OK)
	{
		console::log("[-] failed to hook override view\n");
		return false;
	}

	if (mouse_input_target && MH_CreateHook(mouse_input_target, &hooks::mouse_input::hook, reinterpret_cast<void**>(&mouse_input_original)) != MH_OK)
	{
		console::log("[-] failed to hook mouse input\n");
		return false;
	}
	if (relative_mouse_target)
	{
		menu::relative_mouse = interfaces::input_system->is_relative_mouse_mode();
		if (MH_CreateHook(relative_mouse_target, &hooks::relative_mouse::hook, reinterpret_cast<void**>(&relative_mouse_original)) != MH_OK)
		{
			console::log("[-] failed to hook relative mouse\n");
			return false;
		}
	}

	if (viewmodel_offsets_target && MH_CreateHook(viewmodel_offsets_target, &hooks::viewmodel_offsets::hook, reinterpret_cast<void**>(&viewmodel_offsets_original)) != MH_OK)
	{
		console::log("[-] failed to hook viewmodel offsets\n");
	}
	if (generate_primitives_target)
	{
		if (MH_CreateHook(generate_primitives_target, &hooks::generate_primitives::hook, reinterpret_cast<void**>(&generate_primitives_original)) != MH_OK)
		{
			console::log("[-] failed to hook generate primitives\n");
		}
	}

	if (draw_glow_target)
	{
		if (MH_CreateHook(draw_glow_target, &hooks::draw_glow::hook, reinterpret_cast<void**>(&draw_glow_original)) != MH_OK)
		{
			console::log("[-] failed to hook draw glow\n");
		}
	}

	auto glow_test_call = utilities::pattern_scan(L"client.dll", "00 00 48 8B EA 48 8B F9 E8 ? ? ? ? 45 33 F6 84 C0");
	auto glow_color_call = utilities::pattern_scan(L"client.dll", "E8 ? ? ? ? F3 0F 10 BE ? ? ? ? 48 8B CF");
	if (glow_test_call && glow_color_call)
	{
		auto test_target = utilities::resolve_rip(glow_test_call + 8, 1, 5);
		auto color_target = utilities::resolve_rip(glow_color_call, 1, 5);
		const bool test_created = MH_CreateHook(test_target, &is_glowing_hook, reinterpret_cast<void**>(&is_glowing_original)) == MH_OK;
		const bool color_created = MH_CreateHook(color_target, &get_glow_color_hook, reinterpret_cast<void**>(&get_glow_color_original)) == MH_OK;
		glow_queries_ready = test_created && color_created;
		if (!glow_queries_ready)
		{
			if (test_created) MH_RemoveHook(test_target);
			if (color_created) MH_RemoveHook(color_target);
		}
	}
	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		console::log("[-] failed to enable hooks\n");
		return false;
	}

	console::log("[+] hooks initialized\n");
	if (relative_mouse_original && interfaces::input_system && menu::open) relative_mouse_original(interfaces::input_system, false);
	return true;
}


void hooks::release()
{
	if (relative_mouse_original && interfaces::input_system && menu::open) relative_mouse_original(interfaces::input_system, menu::relative_mouse);
	hooks::skyboxcolors::release();

	if (window && window_procedure_original)
	{
		SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(window_procedure_original));
	}

	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

static bool g_frame_stage_logged[FRAME_RENDER_END + 1] = { };

std::int64_t __fastcall hooks::frame_stage_notify::hook(std::int64_t a1, std::int32_t a2)
{
	if (a2 >= 0 && a2 <= FRAME_RENDER_END && !g_frame_stage_logged[a2]) g_frame_stage_logged[a2] = true;
	const std::int64_t result = frame_stage_notify_original(a1, a2);
	if (a2 == FRAME_RENDER_START && !glow_queries_ready) visuals::run_glow();
	return result;
}

bool __fastcall hooks::create_move::hook(void* a1, std::uint32_t a2, std::uint8_t a3)
{
	interfaces::csgo_input = reinterpret_cast<i_csgo_input*>(a1);

	const bool result = create_move_original(a1, a2, a3);

	sdk::update_local_controller();
	sdk::update_local_player();


	if (sdk::local_controller)
	{
		if (!interfaces::engine || !interfaces::engine->is_in_game())
		{
			misc::practice_move(nullptr);
			return result;
		}

		user_cmd_t* user_cmd = interfaces::csgo_input->get_user_cmd(sdk::local_controller);
		if (!user_cmd) return result;
		const auto original_buttons = user_cmd->buttons;
		const auto original_buttons2 = user_cmd->buttons2;
		const auto original_buttons3 = user_cmd->buttons3;
		auto original_base = user_cmd->get_base();
		const float original_forward = original_base ? original_base->forward_move : 0.f;
		const float original_side = original_base ? original_base->side_move : 0.f;
		const float original_yaw = original_base && original_base->view_angles ? original_base->view_angles->angles.y : 0.f;
		misc::update_velocity();
		misc::practice_move(user_cmd);
		if (config::context.ej) movement::ej(user_cmd);

		movement::ladder_bug(user_cmd);
		movement::edge_bug(user_cmd);
		movement::jump_bug(user_cmd);
		if (config::context.bhop) movement::run_bhop(user_cmd);
		movement::ladder_ej(user_cmd);
		if (config::context.standalone_rcs || config::context.aimbot)
		{
			prediction::start(user_cmd);

			if (config::context.standalone_rcs) combat::run_rcs();
			if (config::context.aimbot && sdk::local_controller) combat::run_aimbot(sdk::local_controller);
			prediction::end(user_cmd);
		}
		if (config::context.pixelsurf) movement::pixelsurf(user_cmd);
		auto final_base = user_cmd->get_base();
		const bool move_changed = final_base && (final_base->forward_move != original_forward || final_base->side_move != original_side ||
			(final_base->view_angles && final_base->view_angles->angles.y != original_yaw));
		if (move_changed || user_cmd->buttons != original_buttons || user_cmd->buttons2 != original_buttons2 || user_cmd->buttons3 != original_buttons3) {
			user_cmd->sync_buttons();
			if (!user_cmd->update_move_crc()) {
				static bool reported = false;
				if (!reported) console::log("[-] movement: command CRC update failed\n");
				reported = true;
			}
		}
	}
	return result;
}

HRESULT __fastcall hooks::swap_chain_present::hook(IDXGISwapChain* swap_chain, std::uint32_t sync_interval, std::uint32_t flags) {
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
		ImFontConfig visualsconfig;
		config.PixelSnapH = true;

		io.Fonts->AddFontDefault(&config);
		if (GetFileAttributesA("C:\\Windows\\Fonts\\verdanab.ttf") != INVALID_FILE_ATTRIBUTES)
		{
			io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdanab.ttf", 13.f, &config);
			io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdanab.ttf", 9.f, &visualsconfig);
		}
		static const ImWchar velocity_ranges[] = { 0x20, 0x20, 0x28, 0x29, 0x30, 0x39, 0 };
		for (int size = 8; size <= 48; size++)
		{
			ImFontConfig velocity_config{};
			velocity_config.PixelSnapH = true;
			velocity_config.OversampleH = 2;
			velocity_config.OversampleV = 2;
			velocity_config.SizePixels = static_cast<float>(size);
			if (GetFileAttributesA("C:\\Windows\\Fonts\\verdanab.ttf") != INVALID_FILE_ATTRIBUTES)
				renderer::velocity_fonts[size] = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdanab.ttf", static_cast<float>(size), &velocity_config, velocity_ranges);
			if (!renderer::velocity_fonts[size]) renderer::velocity_fonts[size] = io.Fonts->AddFontDefault(&velocity_config);
		}
		ImFontConfig menu_font_config{};
		menu_font_config.PixelSnapH = true;
		menu_font_config.OversampleH = 1;
		menu_font_config.OversampleV = 1;
		if (GetFileAttributesA("C:\\Windows\\Fonts\\tahoma.ttf") != INVALID_FILE_ATTRIBUTES)
			menu::font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", 12.f, &menu_font_config);
		else menu::font = io.Fonts->Fonts[0];
		ImGui_ImplDX11_CreateDeviceObjects();
	}

	sdk::update_screen_size();
	hooks::skyboxcolors::run();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	binds::run(menu::open);

	visuals::run_player_esp();
	movement::visualize_eb();
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

	if (ImGui::GetCurrentContext())
		ImGui_ImplDX11_InvalidateDeviceObjects();

	const HRESULT result = swap_chain_resize_buffers_original(swap_chain, buffer_count, width, height, new_format, swap_chain_flags);
	if (FAILED(result) || !device)
		return result;

	ID3D11Texture2D* buffer = nullptr;
	if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&buffer))))
	{
		device->CreateRenderTargetView(buffer, nullptr, &render_view);
		buffer->Release();
	}

	if (ImGui::GetCurrentContext())
		ImGui_ImplDX11_CreateDeviceObjects();

	return result;
}
void __fastcall hooks::override_view::hook(void* client_mode, c_view_setup* view_setup)
{
	override_view_original(client_mode, view_setup);

	if (!view_setup || !config::context.fov_changer) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;
	if (!config::context.scopefov) { if (sdk::local_player->is_scoped()) return; }
		
	view_setup->fov = config::context.fov;
}


std::int64_t __fastcall hooks::generate_primitives::hook(std::uintptr_t thisptr, std::uintptr_t scene_object, std::uintptr_t scene_view, std::uintptr_t primitive_buffer)
{
	static bool logged = false;
	if (!logged)
	{
		logged = true;
	//	console::log("[?] chams: generate primitives reached\n");
	}

	std::int64_t result = 0;

	if (thisptr && scene_object && primitive_buffer)
	{
		if (visuals::should_chams(scene_object) && (config::context.chams_visible || config::context.chams_invisible))
		{
			if (config::context.chams_invisible)
			{
				primitive_output_buffer_t before = *reinterpret_cast<primitive_output_buffer_t*>(primitive_buffer);
				const int first = before.count();
				result = generate_primitives_original(thisptr, scene_object, scene_view, primitive_buffer);
				visuals::apply_chams(primitive_buffer, first, config::context.chams_invisible_material, true, config::context.chams_invisible_color);
			}

			if (config::context.chams_visible)
			{
				primitive_output_buffer_t before = *reinterpret_cast<primitive_output_buffer_t*>(primitive_buffer);
				const int first = before.count();
				result = generate_primitives_original(thisptr, scene_object, scene_view, primitive_buffer);
				visuals::apply_chams(primitive_buffer, first, config::context.chams_material, false, config::context.chams_visible_color);
			}

			return result;
		}


	}

	return generate_primitives_original(thisptr, scene_object, scene_view, primitive_buffer);
}
void* __fastcall hooks::draw_glow::hook(void* glow_property)
{
	static bool logged = false;
	if (!logged)
	{
		logged = true;
		console::log("[?] glow: draw glow reached\n");
	}
	return draw_glow_original(glow_property);
}

bool __fastcall hooks::mouse_input::hook(i_csgo_input* input) {
	if (menu::open) return false;
	return mouse_input_original(input);
}

void* __fastcall hooks::relative_mouse::hook(i_input_system* input, bool enabled)
{
	menu::relative_mouse = enabled;
	return relative_mouse_original(input, menu::open ? false : enabled);
}

void __fastcall hooks::viewmodel_offsets::hook(void* viewmodel, float* offsets, float* fov)
{
	viewmodel_offsets_original(viewmodel, offsets, fov);

	if (!config::context.viewmodel) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;

	if (offsets)
	{
		offsets[0] = config::context.viewmodel_x;
		offsets[1] = config::context.viewmodel_y;
		offsets[2] = config::context.viewmodel_z;
	}

	if (fov)
	{
		*fov = config::context.viewmodel_fov;
	}
}


namespace {
	constexpr std::size_t skybox_patch_size = 10;

	struct skybox_control_t
	{
		std::uint8_t enabled;
		std::uint8_t padding[3];
		float color[3];
	};

	std::uint8_t* skybox_destination = nullptr;
	std::uint8_t* skybox_block = nullptr;
	skybox_control_t* skybox_control = nullptr;
	bool skybox_installed = false;

	const std::array<std::uint8_t, skybox_patch_size> skybox_original = {
		0x48, 0x8D, 0x4D, 0xD0, 0xF2, 0x0F, 0x11, 0x4C, 0x24, 0x50
	};

	std::array<std::uint8_t, skybox_patch_size> skybox_patch = {
		0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90, 0x90
	};

	bool write_protected(void* destination, const void* source, std::size_t size)
	{
		DWORD previous = 0;
		if (!VirtualProtect(destination, size, PAGE_EXECUTE_READWRITE, &previous)) 	return false;
		std::memcpy(destination, source, size);
		DWORD restore = 0;
		VirtualProtect(destination, size, previous, &restore);
		FlushInstructionCache(GetCurrentProcess(), destination, size);
		return true;
	}

	bool replace_skybox_bytes(const std::array<std::uint8_t, skybox_patch_size>& expected, const std::array<std::uint8_t, skybox_patch_size>& replacement)
	{
		if (!skybox_destination || std::memcmp(skybox_destination, expected.data(), skybox_patch_size) != 0) return false;
		return write_protected(skybox_destination, replacement.data(), replacement.size());
	}

	std::uint8_t* allocate_near(std::uint8_t* target)
	{
		const std::uintptr_t target_address = reinterpret_cast<std::uintptr_t>(target);
		std::uintptr_t cursor = target_address > 0x7FFF0000 ? (target_address - 0x7FFF0000) : 0x10000;
		cursor = (cursor + 0xFFFF) & ~static_cast<std::uintptr_t>(0xFFFF);
		const std::uintptr_t limit = target_address + 0x7FFF0000;

		MEMORY_BASIC_INFORMATION info = { };

		while (cursor < limit)
		{
			if (!VirtualQuery(reinterpret_cast<void*>(cursor), &info, sizeof(info))) break;
			const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
			const std::uintptr_t end = base + info.RegionSize;
			if (end <= cursor) break;
			if (info.State == MEM_FREE)
			{
				const std::uintptr_t candidate = (std::max)(cursor, (base + 0xFFFF) & ~static_cast<std::uintptr_t>(0xFFFF));

				if (candidate + 0x2000 <= (std::min)(end, limit))
				{
					if (void* memory = VirtualAlloc(reinterpret_cast<void*>(candidate), 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) return static_cast<std::uint8_t*>(memory);
					
				}
			}

			cursor = end;
		}

		return nullptr;
	}

	bool prepare_skybox_patch()
	{
		skybox_block = allocate_near(skybox_destination);
		if (!skybox_block) return false;
		skybox_control = reinterpret_cast<skybox_control_t*>(skybox_block + 0x1000);
		std::memset(skybox_control, 0, sizeof(skybox_control_t));

		const std::uint8_t code[] = {
			0x9C,
			0x80, 0x3D, 0, 0, 0, 0, 0,
			0x74, 0x10,
			0xF2, 0x0F, 0x10, 0x0D, 0, 0, 0, 0,
			0xF3, 0x0F, 0x10, 0x1D, 0, 0, 0, 0,
			0x9D
		};

		std::memcpy(skybox_block, code, sizeof(code));

		*reinterpret_cast<std::int32_t*>(skybox_block + 3) = static_cast<std::int32_t>(reinterpret_cast<std::uint8_t*>(&skybox_control->enabled) - (skybox_block + 8));
		*reinterpret_cast<std::int32_t*>(skybox_block + 14) = static_cast<std::int32_t>(reinterpret_cast<std::uint8_t*>(&skybox_control->color[0]) - (skybox_block + 18));
		*reinterpret_cast<std::int32_t*>(skybox_block + 22) = static_cast<std::int32_t>(reinterpret_cast<std::uint8_t*>(&skybox_control->color[2]) - (skybox_block + 26));

		std::memcpy(skybox_block + 27, skybox_original.data(), skybox_original.size());

		skybox_block[37] = 0xFF;
		skybox_block[38] = 0x25;
		*reinterpret_cast<std::uint32_t*>(skybox_block + 39) = 0;
		*reinterpret_cast<std::uintptr_t*>(skybox_block + 43) = reinterpret_cast<std::uintptr_t>(skybox_destination + skybox_patch_size);

		*reinterpret_cast<std::int32_t*>(skybox_patch.data() + 1) = static_cast<std::int32_t>(skybox_block - (skybox_destination + 5));

		DWORD previous = 0;

		if (!VirtualProtect(skybox_block, 0x1000, PAGE_EXECUTE_READ, &previous))
		{
			VirtualFree(skybox_block, 0, MEM_RELEASE);
			skybox_block = nullptr;
			skybox_control = nullptr;
			return false;
		}

		FlushInstructionCache(GetCurrentProcess(), skybox_block, 51);

		return true;
	}
}

bool hooks::skyboxcolors::initialize()
{
	std::uint8_t* selection = utilities::pattern_scan(L"scenesystem.dll", SKYBOX_COLOR_PATTERN);

	if (!selection) return false;
	const std::uint32_t first_offset = *reinterpret_cast<std::uint32_t*>(selection + 4);
	const std::uint32_t second_offset = *reinterpret_cast<std::uint32_t*>(selection + 17);

	if (first_offset == 0 || first_offset >= 0x1000 || second_offset != first_offset + 8) return false;
	skybox_destination = selection + 21;

	if (std::memcmp(skybox_destination, skybox_original.data(), skybox_original.size()) != 0)
	{
		if (skybox_destination[0] != 0xE9 || skybox_destination[5] != 0x90 || skybox_destination[9] != 0x90) return false;
		skybox_installed = true;
	}

	return true;
}

void hooks::skyboxcolors::run()
{
	if (!skybox_destination) return;
	if (!config::context.skybox)
	{
		if (skybox_control) skybox_control->enabled = 0;
		if (skybox_installed && replace_skybox_bytes(skybox_patch, skybox_original)) skybox_installed = false;
		return;
	}

	if (!skybox_block && !prepare_skybox_patch()) return;
	const float multiplier = config::context.skybox_intensity;
	skybox_control->color[0] = config::context.skybox_color[0] * multiplier;
	skybox_control->color[1] = config::context.skybox_color[1] * multiplier;
	skybox_control->color[2] = config::context.skybox_color[2] * multiplier;
	skybox_control->enabled = 1;

	if (!skybox_installed && replace_skybox_bytes(skybox_original, skybox_patch)) skybox_installed = true;
}

void hooks::skyboxcolors::release()
{
	if (skybox_control) skybox_control->enabled = 0;
	if (skybox_installed)
	{
		replace_skybox_bytes(skybox_patch, skybox_original);
		skybox_installed = false;
	}

	if (skybox_block)
	{
		VirtualFree(skybox_block, 0, MEM_RELEASE);
		skybox_block = nullptr;
		skybox_control = nullptr;
	}
}

LRESULT __stdcall hooks::window_procedure::hook(HWND hwnd, std::uint32_t message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_KEYDOWN && LOWORD(wparam) == VK_INSERT && !(lparam & (1LL << 30)))
	{
		menu::open = !menu::open;
		if (relative_mouse_original && interfaces::input_system) relative_mouse_original(interfaces::input_system, menu::open ? false : menu::relative_mouse);
	}
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















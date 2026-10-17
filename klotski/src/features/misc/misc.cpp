#include "misc.hpp"

#include "../config/config.hpp"
#include "../../source2-sdk/sdk.hpp"
#include "../../utilities/utilities.hpp"
#include "../../utilities/renderer/renderer.hpp"
#include "../../utilities/imgui/imgui.h"
#include "../../source2-sdk/classes/players.hpp"
#include "../../source2-sdk/interfaces/i_engine_client.hpp"
#include "../../source2-sdk/classes/user_cmd.hpp"

#include <cstdio>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <chrono>
#include <atomic>
#include <cctype>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <d3d11.h>
#include <wrl/client.h>
#include <Windows.h>
#include "../menu/menu.hpp"

static std::atomic<unsigned> practice_pending{ 0 };

void misc::practice_request(bool load)
{
	practice_pending.fetch_or(load ? 2u : 1u, std::memory_order_relaxed);
}

static void practice_input()
{
	static bool previous_save = false;
	static bool previous_load = false;
	const int save_key = config::context.practice_save_key;
	const int load_key = config::context.practice_load_key;
	const bool save_down = save_key > 0 && save_key < 256 && (GetAsyncKeyState(save_key) & 0x8000);
	const bool load_down = load_key > 0 && load_key < 256 && (GetAsyncKeyState(load_key) & 0x8000);
	const bool in_game = interfaces::engine && interfaces::engine->is_in_game();
	if (!in_game)
		practice_pending.store(4u, std::memory_order_relaxed);
	else if (!config::context.practice)
		practice_pending.fetch_and(4u, std::memory_order_relaxed);
	else if (!menu::open)
	{
		if (save_down && !previous_save) misc::practice_request(false);
		if (load_down && !previous_load) misc::practice_request(true);
	}
	previous_save = save_down;
	previous_load = load_down;
}

void misc::practice_move(user_cmd_t* cmd) {
	static bool saved = false;
	static vec3_t origin{};
	static vec3_t angles{};
	auto local = sdk::local_player;
	if (!interfaces::engine || !interfaces::engine->is_in_game())
	{
		saved = false;
		practice_pending.store(0u, std::memory_order_relaxed);
		return;
	}
	if (!cmd || !config::context.practice) return;
	if (!local || !local->is_alive())
	{
		practice_pending.fetch_and(4u, std::memory_order_relaxed);
		return;
	}
	const auto actions = practice_pending.exchange(0u, std::memory_order_relaxed);
	if (actions & 4u) saved = false;
	if (actions & 1u)
	{
		auto view = cmd->get_view_angles();
		vec3_t position{};
		if (view && local->try_abs_origin(position))
		{
			origin = position;
			angles = view->angles;
			saved = true;
			utilities::chat_print("checkpoint saved");
		}
		else utilities::chat_print("unable to save checkpoint: position or angles missing");
	}
	if (!(actions & 2u)) return;
	if (!saved)
	{
		utilities::chat_print("no checkpoint saved");
		return;
	}
	auto cheats = interfaces::cvar ? interfaces::cvar->FindVar("sv_cheats") : nullptr;
	if (!cheats->get_bool()) {
		utilities::chat_print("unable due to sv_cheats 0");
		return;
	}
	char command[256]{};
	std::snprintf(command, sizeof(command), "setpos %.6f %.6f %.6f; setang %.6f %.6f %.6f", origin.x, origin.y, origin.z, angles.x, angles.y, angles.z);
	if (!interfaces::engine->execute_cmd(command)) utilities::chat_print("unable send cmd");
}

static void draw_watermark()
{
	if (!config::context.watermark) return;

	char buffer[64]{};
	sprintf_s(buffer, sizeof(buffer), "klotski | %.0f fps", ImGui::GetIO().Framerate);
	renderer::text(config::context.watermark_x, config::context.watermark_y, 16.f, color_t::white(), buffer);
}

struct velocity_sample_t {
	player_t* player = nullptr;
	float speed = 0.f;
	float time = 0.f;
	float takeoff_until = 0.f;
	int takeoff_velocity = 0;
	float last_velocity = 0.f;
	int color_index = 2;
	int last_tick = 0;
	std::chrono::steady_clock::time_point color_sample_time{};
	bool grounded = false;
	bool has_takeoff = false;
};
static velocity_sample_t velocity_sample{};
static std::mutex velocity_mutex;

void misc::update_velocity()
{
	std::lock_guard<std::mutex> guard(velocity_mutex);
	auto local = sdk::local_player;
	if (!local || !local->is_alive() || !interfaces::globals)
	{
		velocity_sample = {};
		return;
	}
	auto& sample = velocity_sample;
	const int tick = interfaces::globals->tick_count;
	const auto now = std::chrono::steady_clock::now();
	const float speed = vel2d(local->velocity());
	const int velocity = static_cast<int>(std::round(speed));
	const bool grounded = local->flags() & fl_onground;
	if (sample.player != local || tick < sample.last_tick || interfaces::globals->curtime < sample.time)
	{
		sample = {};
		sample.player = local;
		sample.grounded = grounded;
		sample.last_velocity = speed;
		sample.last_tick = tick;
		sample.color_sample_time = now;
	}
	sample.time = interfaces::globals->curtime;
	if (sample.grounded && !grounded)
	{
		sample.takeoff_velocity = velocity;
		sample.takeoff_until = sample.time + 2.f;
		sample.has_takeoff = true;
	}
	sample.speed = speed;
	sample.grounded = grounded;
	if (now - sample.color_sample_time >= std::chrono::milliseconds(10))
	{
		const float delta = speed - sample.last_velocity;
		sample.color_index = delta > 0.5f ? 4 : delta < -0.5f ? 3 : 2;
		sample.last_velocity = speed;
		sample.color_sample_time = now;
	}
	sample.last_tick = tick;
}

static void velocityind()
{
	if (!config::context.velocity || !interfaces::engine || !interfaces::engine->is_in_game()) return;
	velocity_sample_t sample;
	{
		std::lock_guard<std::mutex> guard(velocity_mutex);
		sample = velocity_sample;
	}
	if (!sample.player || sample.player != sdk::local_player) return;
	const int velocity = static_cast<int>(std::round(sample.speed));
	const float blend = (std::clamp)(sample.speed / 310.f, 0.f, 1.f);
	const int index = sample.color_index;
	float rgb[3]{};
	for (int i = 0; i < 3; i++)
		rgb[i] = config::context.velo_gradient ? config::context.velo_colors[0][i] + (config::context.velo_colors[1][i] - config::context.velo_colors[0][i]) * blend : config::context.velo_colors[index][i];
	const color_t color(static_cast<std::uint8_t>((std::clamp)(rgb[0], 0.f, 1.f) * 255.f), static_cast<std::uint8_t>((std::clamp)(rgb[1], 0.f, 1.f) * 255.f), static_cast<std::uint8_t>((std::clamp)(rgb[2], 0.f, 1.f) * 255.f), static_cast<std::uint8_t>((config::context.velo_fade ? blend : 1.f) * 255.f));
	char buffer[64]{};
	if (config::context.velo_pre_speed && sample.has_takeoff && (!sample.grounded || sample.time < sample.takeoff_until))
		sprintf_s(buffer, sizeof(buffer), "%d (%d)", velocity, sample.takeoff_velocity);
	else sprintf_s(buffer, sizeof(buffer), "%d", velocity);
	const int size = (std::clamp)(static_cast<int>(std::round(config::context.velo_size)), 8, 48);
	auto font = renderer::velocity_fonts[size];
	if (!font) font = ImGui::GetFont();
	renderer::text_options_t options{};
	options.font = font;
	options.centered = true;
	options.drop_shadow = true;
	const float x = std::floor(static_cast<float>(sdk::screen_width) * 0.5f);
	const float y = std::floor((std::clamp)(static_cast<float>(sdk::screen_height) - config::context.velo_y, 0.f, (std::max)(0.f, static_cast<float>(sdk::screen_height - size))));
	renderer::text(x, y, static_cast<float>(size), color, buffer, options);
}

bool duck_held = false;
bool jump_held = false;

static auto cmdhelper(user_cmd_t* cmd) {
	duck_held = cmd->is_button_down(in_duck);
	jump_held = cmd->is_button_down(in_jump);
}
static void keystrokes() {
	if (!config::context.keystrokes)  return;
	const float x = config::context.keystroke_pos.x;
	const float y = config::context.keystroke_pos.y;
	const float size = config::context.keystrokes_size;

	const bool w = ImGui::IsKeyDown(ImGuiKey_W);
	const bool s = ImGui::IsKeyDown(ImGuiKey_S);
	const bool a = ImGui::IsKeyDown(ImGuiKey_A);
	const bool d = ImGui::IsKeyDown(ImGuiKey_D);
	const bool crouch = duck_held;

	const bool show_crouch = config::context.show_crouch && crouch;
	const bool show_jump = config::context.show_in_jump && jump_held;

	const bool keep_visible = config::context.show_in_menu && menu::open;

	if (!keep_visible && !(w || s || a || d || show_crouch || show_jump)) return;
	
	//auto color{ 255,255,255,255 };
//	renderer::line(x, y, x, y,color);
	if (w) renderer::text(x, y, size, color_t::white(), "W");
	if (s) renderer::text(x, y + 20.f, size, color_t::white(), "S");
	if (a) renderer::text(x - 20.f, y + 20.f, size, color_t::white(), "A");
	if (d) renderer::text(x + 20.f, y + 20.f, size, color_t::white(), "D");
	if (show_crouch) renderer::text(x, y + 40.f, size, color_t::white(), "C");
	if (show_jump) renderer::text(x + 20.f, y - 20.f, size, color_t::white(), "J");
}
struct indicators {

} inline indicators_data_t;
static void indicators() {
};

//pasted
static ID3D11ShaderResourceView* spectator_avatar(std::uint64_t steam_id) {
    using Microsoft::WRL::ComPtr;
    static std::unordered_map<std::uint64_t, ComPtr<ID3D11ShaderResourceView>> entries;
    static ComPtr<ID3D11Device> cached_device;
    auto font_texture = reinterpret_cast<ID3D11ShaderResourceView*>(ImGui::GetIO().Fonts->TexID);
    if (!font_texture || !steam_id) return nullptr;
    ComPtr<ID3D11Device> device;
    font_texture->GetDevice(device.GetAddressOf());
    if (device.Get() != cached_device.Get()) { entries.clear(); cached_device = device; }
    auto it = entries.find(steam_id);
    if (it != entries.end()) return it->second.Get();
    auto module = GetModuleHandleA("steam_api64.dll");
    if (!module) return nullptr;
    const auto friends_fn = reinterpret_cast<void* (*)()>(GetProcAddress(module, "SteamAPI_SteamFriends_v018"));
    const auto utils_fn = reinterpret_cast<void* (*)()>(GetProcAddress(module, "SteamAPI_SteamUtils_v010"));
    const auto avatar_fn = reinterpret_cast<int (*)(void*, std::uint64_t)>(GetProcAddress(module, "SteamAPI_ISteamFriends_GetMediumFriendAvatar"));
    const auto size_fn = reinterpret_cast<bool (*)(void*, int, std::uint32_t*, std::uint32_t*)>(GetProcAddress(module, "SteamAPI_ISteamUtils_GetImageSize"));
    const auto rgba_fn = reinterpret_cast<bool (*)(void*, int, std::uint8_t*, int)>(GetProcAddress(module, "SteamAPI_ISteamUtils_GetImageRGBA"));
    if (!friends_fn || !utils_fn || !avatar_fn || !size_fn || !rgba_fn) return nullptr;
    auto friends = friends_fn();
    auto utils = utils_fn();
    if (!friends || !utils) return nullptr;
    const int image = avatar_fn(friends, steam_id);
    std::uint32_t width{}, height{};
    if (image <= 0 || !size_fn(utils, image, &width, &height) || !width || !height || width > 1024 || height > 1024) return nullptr;
    std::vector<std::uint8_t> rgba(width * height * 4);
    if (!rgba_fn(utils, image, rgba.data(), static_cast<int>(rgba.size()))) return nullptr;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = rgba.data();
    data.SysMemPitch = width * 4;
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&desc, &data, texture.GetAddressOf()))) return nullptr;
    auto& entry = entries[steam_id];
    if (FAILED(device->CreateShaderResourceView(texture.Get(), nullptr, entry.GetAddressOf()))) return nullptr;
    return entry.Get();
}

//pasted 
static void spectator_list()
{
	if (!config::context.spec_list) return;
	if (!interfaces::engine || !interfaces::engine->is_in_game()) return;
	if (!sdk::local_controller || !interfaces::entity_list) return;

	auto local = sdk::local_controller;
    auto schema_name = [](entity_instance_t* entity) -> const char* {
        void* binding = nullptr;
        utilities::call_virtual<47U, void>(entity, &binding);
        return binding ? *reinterpret_cast<const char**>(reinterpret_cast<std::uint8_t*>(binding) + 8) : nullptr;
    };

    auto observer_services = [&](controller_t* controller) -> player_observer_services_t* {
        const entity_handle_t handles[] = { controller->observer_pawn(), controller->pawn_handle() };
        for (const auto handle : handles) {
            if (!handle || handle == 0xffffffffu) continue;
            auto pawn = reinterpret_cast<base_player_pawn_t*>(interfaces::entity_list->get_entity_from_handle(handle));
            if (!pawn) continue;
            auto identity = pawn->entity_identity();
            const auto type = schema_name(pawn);
            const bool valid_handle = identity && identity->handle == handle;
            const bool valid_type = type && (!std::strcmp(type, "C_CSPlayerPawn") || !std::strcmp(type, "C_CSObserverPawn"));
            if (!valid_handle || !valid_type) continue;
            auto services = pawn->observer_services();
            if (!services) continue;
            if (!interfaces::entity_list->get_entity_from_handle(services->observer_target())) continue;
            return services;
        }
        return nullptr;
    };
    auto view_pawn = sdk::local_player;
    if (!view_pawn || !view_pawn->is_alive()) {
        auto services = observer_services(local);
        view_pawn = services ? reinterpret_cast<player_t*>(interfaces::entity_list->get_entity_from_handle(services->observer_target())) : nullptr;
    }
    if (!view_pawn) return;
    auto view_controller = interfaces::entity_list->get_entity_from_handle(view_pawn->controller_handle());
    struct spectator_entry { std::string name; std::uint64_t steam_id{}; };
    spectator_entry entries[32]{};
    int count = 0;
    for (int i = 1; i <= 64 && count < 32; ++i) {
        auto entity = reinterpret_cast<entity_t*>(interfaces::entity_list->get_base_entity(i));
        if (!entity) continue;
        const auto type = schema_name(entity);
        if (!type || std::strcmp(type, "CCSPlayerController")) continue;
        auto controller = reinterpret_cast<controller_t*>(entity);
        if (!controller || controller == local || controller == view_controller) continue;
        if (controller->is_alive()) continue;
        auto services = observer_services(controller);
        if (!services || interfaces::entity_list->get_entity_from_handle(services->observer_target()) != view_pawn) continue;
        const char* name = controller->name();
        if (!name) continue;
        auto& entry = entries[count++];
        entry.name.assign(name, strnlen_s(name, 127));
        std::transform(entry.name.begin(), entry.name.end(), entry.name.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        entry.steam_id = controller->steam_id();
    }
    if (!count && config::context.spec_list_hide_empty) return;
    if (menu::font) ImGui::PushFont(menu::font);
    const auto title = ImGui::CalcTextSize("spectators");
    const int visible = (std::max)(count, 1);
    constexpr float width = 200.f;
    constexpr float header_height = 31.f;
    constexpr float row_height = 23.f;
    const float height = header_height + visible * row_height + 8.f;
    const auto display = ImGui::GetIO().DisplaySize;
    auto& pos = config::context.speclist_pos;
    if (pos.x < 0.f || pos.y < 0.f) pos = vec2_t((display.x - width) * 0.5f, (display.y - height) * 0.5f);
    ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove;
    if (!menu::open) flags |= ImGuiWindowFlags_NoInputs;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    if (ImGui::Begin("##spectators", nullptr, flags)) {
        ImGui::InvisibleButton("##drag", ImVec2(width, height));
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            pos.x += ImGui::GetIO().MouseDelta.x;
            pos.y += ImGui::GetIO().MouseDelta.y;
        }
        const auto origin = ImGui::GetWindowPos();
        const float x = std::floor(origin.x);
        float y = std::floor(origin.y);
        auto draw = ImGui::GetWindowDrawList();
        auto packed_color = [](float (&rgba)[4]) {
            const bool legacy = rgba[0] > 1.f || rgba[1] > 1.f || rgba[2] > 1.f || rgba[3] > 1.f;
            if (legacy) for (auto& channel : rgba) channel /= 255.f;
            return ImGui::ColorConvertFloat4ToU32(ImVec4(rgba[0], rgba[1], rgba[2], rgba[3]));
        };
        const ImU32 accent = packed_color(config::context.spec_name_color);
        const ImU32 pl_accent = packed_color(config::context.spec_player_name_color);
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), IM_COL32(1, 1, 1, 245));
        draw->AddRect(ImVec2(x + 1.f, y + 1.f), ImVec2(x + width - 1.f, y + height - 1.f), IM_COL32(57, 57, 57, 255));
        draw->AddRectFilled(ImVec2(x + 2.f, y + 2.f), ImVec2(x + width - 2.f, y + height - 2.f), IM_COL32(35, 35, 35, 245));
        draw->AddRectFilled(ImVec2(x + 5.f, y + 5.f), ImVec2(x + width - 5.f, y + height - 5.f), IM_COL32(57, 57, 57, 245));
        draw->AddRectFilled(ImVec2(x + 6.f, y + 6.f), ImVec2(x + width - 6.f, y + height - 6.f), IM_COL32(4, 4, 4, 245));
        draw->AddText(ImVec2(x + 12.f, y + 8.f), accent, "spectators");
        char count_text[8]{};
        std::snprintf(count_text, sizeof(count_text), "%d", count);
        const auto count_size = ImGui::CalcTextSize(count_text);
        draw->AddText(ImVec2(x + width - 12.f - count_size.x, y + 8.f), IM_COL32(153, 153, 153, 255), count_text);
        draw->AddRectFilled(ImVec2(x + 8.f, y + 25.f), ImVec2(x + width - 8.f, y + 29.f), IM_COL32(36, 36, 36, 255));
        draw->AddRectFilled(ImVec2(x + 8.f, y + 25.f), ImVec2(x + 63.f, y + 29.f), IM_COL32(181, 0, 41, 255));
        draw->AddRectFilled(ImVec2(x + 8.f, y + 27.f), ImVec2(x + width - 8.f, y + 29.f), IM_COL32(36, 36, 36, 127));
        draw->AddRect(ImVec2(x + 8.f, y + header_height), ImVec2(x + width - 8.f, y + height - 7.f), IM_COL32(44, 44, 44, 255));
        y += header_height + 1.f;
        if (!count) draw->AddText(ImVec2(x + 14.f, y + (row_height - ImGui::GetFontSize()) * 0.5f), IM_COL32(153, 153, 153, 255), "no spectators");
        for (int i = 0; i < count; ++i) {
            auto avatar = spectator_avatar(entries[i].steam_id);
            const float offset = avatar ? 22.f : 0.f;
            auto name = entries[i].name;
            const float max_width = width - 28.f - offset;
            if (ImGui::CalcTextSize(name.c_str()).x > max_width) {
                while (!name.empty() && ImGui::CalcTextSize((name + "...").c_str()).x > max_width) {
                    auto last = name.size() - 1;
                    while (last > 0 && (static_cast<unsigned char>(name[last]) & 0xc0u) == 0x80u) --last;
                    name.resize(last);
                }
                name += "...";
            }
            if (i & 1) draw->AddRectFilled(ImVec2(x + 9.f, y), ImVec2(x + width - 9.f, y + row_height), IM_COL32(12, 12, 12, 180));
            if (avatar) {
                draw->AddRect(ImVec2(x + 13.f, y + 3.f), ImVec2(x + 30.f, y + 20.f), IM_COL32(44, 44, 44, 255));
                draw->AddImage(reinterpret_cast<ImTextureID>(avatar), ImVec2(x + 14.f, y + 4.f), ImVec2(x + 29.f, y + 19.f));
            }
            draw->AddText(ImVec2(x + 14.f + offset, y + (row_height - ImGui::GetFontSize()) * 0.5f), pl_accent, name.c_str());
            y += row_height;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    if (menu::font) ImGui::PopFont();
}

void misc::grenade_trail() {
	if (!interfaces::cvar) return;
	if (!interfaces::engine || !interfaces::engine->is_in_game()) return;
	auto cvar = interfaces::cvar->FindVar("sv_grenade_trajectory_prac_trailtime");
	if (!cvar) return;
	const bool want = config::context.grenade_trail;

	if (want && cvar->get_float() == 0.f)
		cvar->set_float(1.f);
	else if (!want && cvar->get_float() != 0.f)
		cvar->set_float(0.f);
}
void misc::run()
{
	practice_input();
	grenade_trail();
	draw_watermark();
	velocityind();
	keystrokes();
	indicators();
	spectator_list();
}

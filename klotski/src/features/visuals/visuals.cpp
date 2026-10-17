#include "visuals.hpp"

#include "../../hooks/hooks.hpp"

#include "../../utilities/utilities.hpp"
#include "../../source2-sdk/sdk.hpp"
#include "../config/config.hpp"
#include "../../source2-sdk/classes/players.hpp"
#include "../../source2-sdk/classes/weapons.hpp"
#include "../../source2-sdk/interfaces/visible.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"
#include "../../utilities/imgui/imgui.h"

#include <array>
#include <algorithm>
#include <limits>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>
#include <Windows.h>

// !CHAMS ARE VELO PASTED! 

typedef struct _box_t {
	float x, y, w, h;
} box_t;

static constexpr float k_pi{ 3.14159265358979323846f };
static double hitmarker_until = 0.0;
struct esp_bar_state_t
{
	player_t* player{};
	cs_weapon_base_t* weapon{};
	float health{};
	float ammo{};
};
static esp_bar_state_t esp_bars[65]{};
struct chams_primitive_buffer_t
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
		if (fixed_count && !fixed_data) return -1;
		if (overflow_count && !overflow_data) return -1;
		if (fixed_count > (1 << 20) - overflow_count) return -1;
		return fixed_count + overflow_count;
	}

	std::uintptr_t at(int index) const
	{
		const int total = count();
		if (index < 0 || total < 0 || index >= total) return 0;
		if (index < fixed_count) return fixed_data + static_cast<std::uintptr_t>(index) * 0x70;
		return overflow_data + static_cast<std::uintptr_t>(index - fixed_count) * 0x70;
	}
};

struct chams_key_values3_t
{
	std::uint64_t metadata;
	std::uint64_t payload;
};

struct chams_strong_handle_t
{
	const void* binding;
};

struct chams_kv3_id_t
{
	const char* format;
	std::uintptr_t guid_low;
	std::uintptr_t guid_high;
};

static constexpr int chams_material_count = 8;
static std::array<chams_strong_handle_t, chams_material_count * 2> chams_handles{};
static std::mutex chams_material_mutex;
static std::uintptr_t chams_materials[chams_material_count * 2] = { };
static bool chams_hook_seen = false;
static bool chams_player_seen = false;
static bool chams_material_failed = false;

static std::uint32_t color_dump(color_t color)
{
	return static_cast<std::uint32_t>(color.r) | (static_cast<std::uint32_t>(color.g) << 8) | (static_cast<std::uint32_t>(color.b) << 16) | (static_cast<std::uint32_t>(color.a) << 24);
}

static void chams_debug(bool* flag, const char* text)
{
	if (*flag) return;
	*flag = true;
	console::log("[?] chams: %s\n", text);
}

static const char* chams_vmat(int material, bool ignore_z)
{
	if (material >= 4) {
		static thread_local std::string definition;
		definition = R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
shader = "csgo_complex.vfx"
F_PAINT_VERTEX_COLORS = 1
F_RENDER_BACKFACES = 0
g_vColorTint = [1.0, 1.0, 1.0, 1.0]
g_flModelTintAmount = 1.0
g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
g_tAmbientOcclusion = resource:"materials/default/default_mask_tga_fde710a5.vtex"
g_tTintMask = resource:"materials/default/default_mask_tga_fde710a5.vtex"
)KV3";
		if (ignore_z) definition += "\nF_IGNOREZ = 1\nF_DISABLE_Z_BUFFERING = 1\nF_DISABLE_Z_WRITE = 1\n";
		else definition += "\nF_IGNOREZ = 0\nF_DISABLE_Z_BUFFERING = 0\nF_DISABLE_Z_WRITE = 0\n";
		switch (material) {
		case 4:
			definition += "\ng_flRoughness = 0.65\ng_flMetalness = 0.0\n";
			break;
		case 5:
			definition += "\nF_SELF_ILLUM = 1\ng_flSelfIllumBrightness = 3.0\ng_flSelfIllumScale = 1.0\ng_vSelfIllumTint = [1.0, 1.0, 1.0]\ng_flSelfIllumAlbedoFactor = 1.0\ng_tSelfIllumMask = resource:\"materials/dev/primary_white_color_tga_21186c76.vtex\"\n";
			break;
		case 6:
			definition += "\nF_METALNESS_TEXTURE = 0\ng_flMetalness = 1.0\ng_flRoughness = 0.12\n";
			break;
		case 7:
			definition += "\nF_METALNESS_TEXTURE = 0\nF_ANISOTROPIC_GLOSS = 1\ng_flMetalness = 0.8\ng_flRoughness = 0.2\ng_flPearlescent = 6.0\n";
			break;
		}
		definition += "\n}";
		return definition.c_str();
	}

	switch (material)
	{
	case 1:
		return ignore_z ? R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "csgo_effects.vfx"
	F_ADDITIVE_BLEND = 1
	F_BLEND_MODE = 1
	F_TRANSLUCENT = 1
	F_IGNOREZ = 1
	F_DISABLE_Z_BUFFERING = 1
	F_DISABLE_Z_WRITE = 1
	F_RENDER_BACKFACES = 0
	g_vColorTint = [1.0, 1.0, 1.0, 1.0]
	g_flFresnelExponent = 1.5
	g_flFresnelFalloff = 5.0
	g_flFresnelMax = 0.0
	g_flFresnelMin = 1.0
	g_flColorBoost = 20.0
	g_flOpacityScale = 0.6
	g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
	g_tMask1 = resource:"materials/default/default_mask_tga_344101f8.vtex"
	g_tMask2 = resource:"materials/default/default_mask_tga_344101f8.vtex"
	g_tMask3 = resource:"materials/default/default_mask_tga_344101f8.vtex"
})KV3" : R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "csgo_effects.vfx"
	F_ADDITIVE_BLEND = 1
	F_BLEND_MODE = 1
	F_TRANSLUCENT = 1
	F_IGNOREZ = 0
	F_DISABLE_Z_BUFFERING = 0
	F_RENDER_BACKFACES = 0
	g_vColorTint = [1.0, 1.0, 1.0, 1.0]
	g_flFresnelExponent = 1.5
	g_flFresnelFalloff = 5.0
	g_flFresnelMax = 0.0
	g_flFresnelMin = 1.0
	g_flColorBoost = 20.0
	g_flOpacityScale = 0.6
	g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
	g_tMask1 = resource:"materials/default/default_mask_tga_344101f8.vtex"
	g_tMask2 = resource:"materials/default/default_mask_tga_344101f8.vtex"
	g_tMask3 = resource:"materials/default/default_mask_tga_344101f8.vtex"
})KV3";
	case 2:
		return ignore_z ? R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "solidcolor.vfx"
	F_IGNOREZ = 1
	F_DISABLE_Z_BUFFERING = 1
	F_DISABLE_Z_WRITE = 1
	g_vColorTint = [5.0, 5.0, 5.0]
})KV3" : R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "solidcolor.vfx"
	F_DISABLE_Z_WRITE = 0
	g_vColorTint = [8.0, 8.0, 8.0]
})KV3";
	case 3:
		return ignore_z ? R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "csgo_complex.vfx"
	F_SELF_ILLUM = 1
	F_RENDER_BACKFACES = 1
	F_IGNOREZ = 1
	F_DISABLE_Z_BUFFERING = 1
	F_DISABLE_Z_WRITE = 1
	g_vColorTint = [0.0, 0.0, 0.0]
	g_flModelTintAmount = 1.0
	g_flOpacityScale = 0.6
	g_flSelfIllumBrightness = 4.5
	g_flSelfIllumScale = 2.0
	g_vSelfIllumTint = [0.45, 0.85, 1.0]
	g_flSelfIllumAlbedoFactor = 0.55
	g_vSelfIllumScrollSpeed = [0.0, 0.35]
	g_vTexCoordScale = [0.75, 9.0]
	g_bFogEnabled = 0
	g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
	g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
	g_tSelfIllumMask = resource:"materials/dev/water_waves.vtex"
	g_tAmbientOcclusion = resource:"materials/default/default_mask_tga_fde710a5.vtex"
	g_tTintMask = resource:"materials/default/default_mask_tga_fde710a5.vtex"
})KV3" : R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "csgo_complex.vfx"
	F_SELF_ILLUM = 1
	F_RENDER_BACKFACES = 1
	F_TRANSLUCENT = 1
	g_vColorTint = [0.0, 0.0, 0.0]
	g_flModelTintAmount = 1.0
	g_flOpacityScale = 0.6
	g_flSelfIllumBrightness = 4.5
	g_flSelfIllumScale = 2.0
	g_vSelfIllumTint = [0.45, 0.85, 1.0]
	g_flSelfIllumAlbedoFactor = 0.55
	g_vSelfIllumScrollSpeed = [0.0, 0.35]
	g_vTexCoordScale = [0.75, 9.0]
	g_bFogEnabled = 0
	g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
	g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
	g_tSelfIllumMask = resource:"materials/dev/water_waves.vtex"
	g_tAmbientOcclusion = resource:"materials/default/default_mask_tga_fde710a5.vtex"
	g_tTintMask = resource:"materials/default/default_mask_tga_fde710a5.vtex"
})KV3";
	default:
		return ignore_z ? R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "csgo_unlitgeneric.vfx"
	F_RENDER_BACKFACES = 0
	F_IGNOREZ = 1
	F_DISABLE_Z_BUFFERING = 1
	F_DISABLE_Z_WRITE = 1
	F_PAINT_VERTEX_COLORS = 1
	g_vColorTint = [1.0, 1.0, 1.0, 1.0]
	g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
	g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
})KV3" : R"KV3(<!-- kv3 encoding:text:version{e21c7f3c-8a33-41c5-9977-a76d3a32aa0d} format:generic:version{7412167c-06e9-4698-aff2-e63eb59037e7} -->
{
	shader = "csgo_unlitgeneric.vfx"
	F_RENDER_BACKFACES = 0
	F_DISABLE_Z_BUFFERING = 0
	F_PAINT_VERTEX_COLORS = 1
	F_TRANSLUCENT = 1
	F_BLEND_MODE = 1
	g_vColorTint = [1.0, 1.0, 1.0, 1.0]
	g_tColor = resource:"materials/dev/primary_white_color_tga_21186c76.vtex"
	g_tNormal = resource:"materials/default/default_normal_tga_7652cb.vtex"
})KV3";
	}
}
static std::uintptr_t load_chams_material(int material, bool ignore_z)
{
	std::lock_guard<std::mutex> guard(chams_material_mutex);
	material = std::clamp(material, 0, chams_material_count - 1);
	const int index = material + (ignore_z ? chams_material_count : 0);
	if (chams_materials[index]) return chams_materials[index];

	using kv3_alloc_t = chams_key_values3_t*(__fastcall*)(chams_key_values3_t*, std::uint32_t, std::uint32_t);
	using kv3_destroy_t = void(__fastcall*)(chams_key_values3_t*, std::uint32_t);
	using kv3_load_t = bool(__fastcall*)(chams_key_values3_t*, void*, const char*, const chams_kv3_id_t*, void*, std::uint32_t);
	using material_create_t = void*(__fastcall*)(void*, chams_strong_handle_t*, const char*, chams_key_values3_t*, int, bool);

	static kv3_alloc_t kv3_alloc = reinterpret_cast<kv3_alloc_t>(utilities::pattern_scan(L"tier0.dll", KV3_ALLOC));
	static kv3_destroy_t kv3_destroy = reinterpret_cast<kv3_destroy_t>(utilities::pattern_scan(L"tier0.dll", KV3_DESTROY));
	static kv3_load_t kv3_load = reinterpret_cast<kv3_load_t>(GetProcAddress(GetModuleHandleA("tier0.dll"), "?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2I@Z"));
	static material_create_t material_create = reinterpret_cast<material_create_t>(utilities::pattern_scan(L"materialsystem2.dll", MATERIAL_CREATE));

	if (!kv3_alloc || !kv3_destroy || !kv3_load || !material_create)
	{
		chams_debug(&chams_material_failed, "material functions missing");
		return 0;
	}

	chams_key_values3_t kv3{};
	chams_kv3_id_t kv3_id{ "generic", 0x41B818518343427E, 0xB5F447C23C0CDF8C };

	if (kv3_alloc(&kv3, 1u, 6u) != &kv3) return 0;

	chams_strong_handle_t handle{};
	char name[64]{};
	std::snprintf(name, sizeof(name), "klotski_chams_%d_%d", material, ignore_z ? 1 : 0);

	const bool loaded = kv3_load(&kv3, nullptr, chams_vmat(material, ignore_z), &kv3_id, nullptr, 0u);
	if (loaded) material_create(nullptr, &handle, name, &kv3, 0, true);
	kv3_destroy(&kv3, 0u);

	if (!loaded || !handle.binding)
	{
		chams_debug(&chams_material_failed, "material create failed");
		return 0;
	}

	const std::uintptr_t result = *reinterpret_cast<const std::uintptr_t*>(handle.binding);
	if (result) chams_handles[index] = handle;
	chams_materials[index] = result;
	return result;
}

static void replace_chams_primitives(const chams_primitive_buffer_t& buffer, int first, std::uintptr_t material, color_t color)
{
	const int total = buffer.count();
	const std::uint32_t packed = color_dump(color);

	for (int i = first; i < total; i++)
	{
		const std::uintptr_t primitive = buffer.at(i);
		if (!primitive) continue;
		if (material)
		{
			*reinterpret_cast<std::uintptr_t*>(primitive + 0x20) = material;
			*reinterpret_cast<std::uintptr_t*>(primitive + 0x28) = material;
		}
		*reinterpret_cast<std::uint32_t*>(primitive + 0x50) = packed;
	}
}

static bool apply_chams_layer(std::uintptr_t primitive_buffer, int first, int material_id, bool ignore_z, color_t color)
{
	if (!primitive_buffer) return false;
	if (first < 0) return false;
	const std::uintptr_t material = load_chams_material(material_id, ignore_z);
	if (!material) return false;
	const auto buffer = *reinterpret_cast<const chams_primitive_buffer_t*>(primitive_buffer);
	if (buffer.count() <= first) return false;
	replace_chams_primitives(buffer, first, material, color);
	return true;
}

static player_t* player_from_scene_object(std::uintptr_t scene_object)
{
	if (!scene_object || !interfaces::entity_list) return nullptr;
	const entity_handle_t owner_handle = *reinterpret_cast<entity_handle_t*>(scene_object + 0xc0);
	if (!handle_is_valid(owner_handle)) return nullptr;

	void* entity = interfaces::entity_list->get_entity_from_handle(owner_handle);
	if (!entity) return nullptr;

	for (std::int32_t i = 1; i <= 64; i++)
	{
		controller_t* controller = interfaces::entity_list->get_controller_by_index(i);
		if (!controller) continue;
		player_t* player = interfaces::entity_list->get_player_from_controller(controller);
		if (player == entity) return player;
	}

	return nullptr;
}

static color_t color_from_float(const float* value, float alpha = 1.f)
{
	return color_t(
		static_cast<std::uint8_t>((std::clamp)(value[0], 0.f, 1.f) * 255.f),
		static_cast<std::uint8_t>((std::clamp)(value[1], 0.f, 1.f) * 255.f),
		static_cast<std::uint8_t>((std::clamp)(value[2], 0.f, 1.f) * 255.f),
		static_cast<std::uint8_t>((std::clamp)(alpha, 0.f, 1.f) * 255.f));
}

static color_t with_alpha(color_t color, float alpha)
{
	color.a = static_cast<std::uint8_t>((std::clamp)(alpha, 0.f, 1.f) * 255.f);
	return color;
}

static bool is_screen_point_visible(const vec3_t& point)
{
	return point.x >= 0.f && point.x <= static_cast<float>(sdk::screen_width)
		&& point.y >= 0.f && point.y <= static_cast<float>(sdk::screen_height);
}

static void draw_out_of_fov_arrow(const vec3_t& local_eye_position, const vec3_t& target_origin, color_t color)
{
	if (!config::context.out_of_fov_arrows) return;
	if (!interfaces::csgo_input) return;
	if (sdk::screen_width <= 0 || sdk::screen_height <= 0) return;

	vec3_t* view_angles = interfaces::csgo_input->get_view_angles(0);
	if (!view_angles) return;

	const float dx = target_origin.x - local_eye_position.x;
	const float dy = target_origin.y - local_eye_position.y;
	const float target_yaw = std::atan2f(dy, dx) * 180.f / k_pi;
	const float yaw_delta = normalize_yaw(target_yaw - view_angles->y);
	const float angle = (-yaw_delta - 90.f) * k_pi / 180.f;

	const float center_x = static_cast<float>(sdk::screen_width) * 0.5f;
	const float center_y = static_cast<float>(sdk::screen_height) * 0.5f;
	const float max_radius = (std::min)(center_x, center_y) - config::context.out_of_fov_arrow_size * 2.f;
	const float radius = (std::clamp)(config::context.out_of_fov_arrow_radius, 20.f, (std::max)(20.f, max_radius));
	const float size = (std::clamp)(config::context.out_of_fov_arrow_size, 4.f, 48.f);
	const float dir_x = std::cosf(angle);
	const float dir_y = std::sinf(angle);
	const float arrow_x = center_x + dir_x * radius;
	const float arrow_y = center_y + dir_y * radius;
	const float base_x = arrow_x + dir_x * 3.f;
	const float base_y = arrow_y + dir_y * 3.f;
	const float perp_x = -dir_y;
	const float perp_y = dir_x;

	const float tip_x = base_x + dir_x * size;
	const float tip_y = base_y + dir_y * size;
	const float width = size * config::context.out_of_fov_arrow_width;
	const float left_x = base_x + perp_x * width;
	const float left_y = base_y + perp_y * width;
	const float right_x = base_x - perp_x * width;
	const float right_y = base_y - perp_y * width;
	const float pulse = config::context.out_of_fov_arrow_pulse ? 0.65f + 0.35f * std::sinf(static_cast<float>(ImGui::GetTime()) * 3.f) : 1.f;
	color.a = static_cast<std::uint8_t>(color.a * config::context.out_of_fov_arrow_opacity * pulse);
	renderer::filled_triangle(tip_x, tip_y, left_x, left_y, right_x, right_y, color);
	if (config::context.out_of_fov_arrow_outline)
	{
		auto draw = ImGui::GetBackgroundDrawList();
		draw->AddTriangle(ImVec2(tip_x, tip_y), ImVec2(left_x, left_y), ImVec2(right_x, right_y), IM_COL32(0, 0, 0, color.a), 1.f);
	}
}

static cs_weapon_base_t* get_active_weapon(player_t* player)
{
	cs_player_weapon_services_t* services = player->weapons();
	if (!services) return nullptr;
	const entity_handle_t handle = services->active_weapon();
	if (handle == invalid_entity_handle) return nullptr;

	cs_weapon_base_t* weapon = reinterpret_cast<cs_weapon_base_t*>(interfaces::entity_list->get_base_entity(handle_index(handle)));

	return weapon;
}

const auto get_bounding_box(player_t* player, box_t& box) {
	vec3_t origin{};
	vec3_t bounds_min{};
	vec3_t bounds_max{};

	if (!player->try_abs_origin(origin)) return false;
	if (!player->try_collision_bounds(bounds_min, bounds_max)) return false;

	vec3_t flb, brt, blb, frt, frb, brb, blt, flt = { };
	float left, top, right, bottom = 0.f;

	vec3_t min = bounds_min + origin;
	vec3_t max = bounds_max + origin;

	std::array<vec3_t, 8> points = {
		vec3_t(min.x, min.y, min.z),
		vec3_t(min.x, min.y, max.z),
		vec3_t(min.x, max.y, min.z),
		vec3_t(min.x, max.y, max.z),
		vec3_t(max.x, min.y, min.z),
		vec3_t(max.x, min.y, max.z),
		vec3_t(max.x, max.y, min.z),
		vec3_t(max.x, max.y, max.z) };

	if (!math::world_to_screen(points[3], flb)
		|| !math::world_to_screen(points[5], brt)
		|| !math::world_to_screen(points[0], blb)
		|| !math::world_to_screen(points[4], frt)
		|| !math::world_to_screen(points[2], frb)
		|| !math::world_to_screen(points[1], brb)
		|| !math::world_to_screen(points[6], blt)
		|| !math::world_to_screen(points[7], flt)) return false;
	
	std::array<vec3_t, 8> arr = { flb, brt, blb, frt, frb, brb, blt, flt };

	left = flb.x;
	top = flb.y;
	right = flb.x;
	bottom = flb.y;

	for (std::int32_t i = 1; i < 8; i++)
	{
		if (left > arr[i].x) left = arr[i].x;

		if (bottom < arr[i].y) 	bottom = arr[i].y;
		if (right < arr[i].x) right = arr[i].x;
		if (top > arr[i].y) top = arr[i].y;
		
	}

	box.x = left;
	box.y = top;
	box.w = right - left;
	box.h = bottom - top;

	return box.w > 1.f && box.h > 1.f;
}

static void draw_axis_line(float x1, float y1, float x2, float y2, color_t color, float thickness)
{
	if (thickness <= 1.01f)
	{
		renderer::line(x1, y1, x2, y2, color);
		return;
	}

	const float top = y1 < y2 ? y1 : y2;
	const float left = x1 < x2 ? x1 : x2;

	if (std::abs(x2 - x1) < 0.01f)
	{
		renderer::filled_rect(x1 - thickness * 0.5f, top, thickness, std::abs(y2 - y1), color);
	}
	else
	{
		renderer::filled_rect(left, y1 - thickness * 0.5f, std::abs(x2 - x1), thickness, color);
	}
}

static void draw_full_box(box_t& box, color_t color) {
	const float t = config::context.box_thickness;
	draw_axis_line(box.x, box.y, box.x + box.w, box.y, color, t);
	draw_axis_line(box.x, box.y + box.h, box.x + box.w, box.y + box.h, color, t);
	draw_axis_line(box.x, box.y, box.x, box.y + box.h, color, t);
	draw_axis_line(box.x + box.w, box.y, box.x + box.w, box.y + box.h, color, t);
}

static void draw_corner_box(box_t& box, color_t color)
{
	float len_x = box.w * config::context.box_corner_size / 100.f;
	float len_y = box.h * config::context.box_corner_size / 100.f;

	if (len_x < 1.f) len_x = 1.f;
	if (len_y < 1.f) len_y = 1.f;

	const float t = config::context.box_thickness;
	const float left = box.x;
	const float right = box.x + box.w;
	const float top = box.y;
	const float bottom = box.y + box.h;

	draw_axis_line(left, top, left + len_x, top, color, t);
	draw_axis_line(left, top, left, top + len_y, color, t);

	draw_axis_line(right - len_x, top, right, top, color, t);
	draw_axis_line(right, top, right, top + len_y, color, t);

	draw_axis_line(left, bottom, left + len_x, bottom, color, t);
	draw_axis_line(left, bottom - len_y, left, bottom, color, t);

	draw_axis_line(right - len_x, bottom, right, bottom, color, t);
	draw_axis_line(right, bottom - len_y, right, bottom, color, t);
}

static void draw_box(box_t& box, color_t color)
{
	if (!config::context.draw_box) return;

	if (config::context.box_fill)
	{
		color_t fill_color = color;
		fill_color.a = static_cast<std::uint8_t>(config::context.box_fill_alpha * 255.f);
		renderer::filled_rect(box.x, box.y, box.w, box.h, fill_color);
	}

	if (config::context.box_outline)
	{
		const color_t outline(0, 0, 0, color.a);
		draw_full_box(box, outline);
	}

	if (config::context.box_corner)
	{
		draw_corner_box(box, color);
	}
	else
	{
		draw_full_box(box, color);
	}
}

static void draw_health(box_t& box, std::int32_t player_health, float animated_health, color_t health_color, float alpha)
{
	if (!config::context.draw_health) return;

	const float ratio = (std::clamp)(animated_health / 100.f, 0.f, 1.f);
	float bar_height = ratio * box.h;
	if (!config::context.health_custom_color)
		health_color = color_t(static_cast<std::uint8_t>(255.f * (1.f - ratio)), static_cast<std::uint8_t>(255.f * ratio), 0, 255);

	box_t bar_box = { box.x - 5.f, box.y, 3.f, box.h };
	box_t health_bar = { box.x - 4.f, box.y + box.h - bar_height, 1.f, bar_height };

	const auto a = static_cast<std::uint8_t>(alpha * 255.f);
	health_color.a = a;

	renderer::filled_rect(bar_box.x, bar_box.y, bar_box.w, bar_box.h, color_t(0, 0, 0, a));
	if (config::context.health_gradient) {
		auto bottom = config::context.health_custom_color
			? color_t(health_color.r / 3, health_color.g / 3, health_color.b / 3, a)
			: color_t(255, 50, 35, a);
		ImGui::GetBackgroundDrawList()->AddRectFilledMultiColor(
			ImVec2(health_bar.x, health_bar.y), ImVec2(health_bar.x + health_bar.w, health_bar.y + health_bar.h),
			health_color.dump(), health_color.dump(), bottom.dump(), bottom.dump());
	} else renderer::filled_rect(health_bar.x, health_bar.y, health_bar.w, health_bar.h, health_color);
	if (config::context.health_text)
	{
		char text[16]{};
		std::snprintf(text, sizeof(text), "%d%s", player_health, config::context.health_suffix ? "hp" : "");
		const float width = ImGui::GetFont()->CalcTextSizeA(11.f, FLT_MAX, 0.f, text).x;
		const bool beside = config::context.health_text_position == 1;
		const float x = beside ? bar_box.x - width - 3.f : box.x + (box.w - width) * 0.5f;
		const float y = beside ? health_bar.y - 5.f : box.y + box.h + (config::context.draw_ammo_bar ? 8.f : 2.f);
		renderer::text(x, y, 11.f, health_color, text);
	}

}


static void draw_name(box_t& box, const char* player_name, color_t color, float alpha)
{
	if (!config::context.draw_name) return;
	if (!player_name) return;

	std::string name(player_name);

	if (name.empty()) return;
	if (name.size() > 15)name = name.substr(0, 15) + "...";
	renderer::text_centered(box.x, box.y, box.w, box.h, with_alpha(color, alpha), name.c_str());
}

static void draw_skeleton(player_t* player, color_t color, float alpha)
{
	if (!config::context.draw_skeleton) return;
	color = with_alpha(color, alpha);

	std::int32_t bone_count = 0;
	bone_data_t* bones = player->get_bone_cache(&bone_count);

	if (!bones || bone_count <= 0) return;

	for (const bone_chain_t& chain : bone_chains)
	{
		for (std::size_t i = 0; i + 1 < chain.bones.size(); i++)
		{
			const bone_ids from = chain.bones[i];
			const bone_ids to = chain.bones[i + 1];

			if (from == bone_none || to == bone_none) break;
			if (static_cast<std::int32_t>(from) >= bone_count || static_cast<std::int32_t>(to) >= bone_count) break;

			vec3_t from_screen = { };
			vec3_t to_screen = { };

			if (!math::world_to_screen(bones[from].position, from_screen)) break;
			if (!math::world_to_screen(bones[to].position, to_screen)) break;

			renderer::line(from_screen.x, from_screen.y, to_screen.x, to_screen.y, color);
		}
	}
}

static const char* get_weapon_display_name(const char* designer_name)
{
	if (!designer_name) return nullptr;
	for (const weapon_name_entry_t& entry : weapon_names) {if (std::strcmp(entry.designer_name, designer_name) == 0) return entry.display_name;}
	return designer_name;
}
static void draw_weapon(box_t& box, player_t* player, color_t color, float alpha) {
	cs_weapon_base_t* weapon = get_active_weapon(player);
	if (!config::context.draw_weapon) return;
	if (!weapon) return;

	const char* weapon_name = get_weapon_display_name(weapon->get_designer_name());
	if (!weapon_name) return;

	const float padding = config::context.draw_health && config::context.health_text && config::context.health_text_position == 0 ? 12.f : 0.f;
	renderer::text_centered(box.x, box.y + box.h + 9 + padding, box.w, box.h, with_alpha(color, alpha), weapon_name);
}

static void draw_ammo_bar(box_t& box, player_t* player, esp_bar_state_t& state, color_t color, float alpha)
{
	if (!config::context.draw_ammo_bar) return;

	cs_weapon_base_t* weapon = get_active_weapon(player);
	if (!weapon) return;

	cs_weapon_base_vdata_t* vdata = weapon->get_vdata();
	if (!vdata) return;

	const std::int32_t max_clip = vdata->max_clip();
	const std::int32_t clip = weapon->clip();

	if (max_clip <= 0 || clip < 0) return;

	if (state.weapon != weapon)
	{
		state.weapon = weapon;
		state.ammo = static_cast<float>(clip);
	}
	if (!config::context.ammo_smooth || state.ammo <= clip) state.ammo = static_cast<float>(clip);
	else state.ammo = (std::max)(static_cast<float>(clip), state.ammo - 2.f * ImGui::GetIO().DeltaTime);
	const float ratio = (std::clamp)(state.ammo / static_cast<float>(max_clip), 0.f, 1.f);
	const auto a = static_cast<std::uint8_t>(alpha * 255.f);
	color.a = a;
	const float y = box.y + box.h + 3.f;

	renderer::filled_rect(box.x, y, box.w, 3.f, color_t(0, 0, 0, a));
	renderer::filled_rect(box.x + 1.f, y + 1.f, (box.w - 2.f) * ratio, 1.f, color);

	if (clip < max_clip)
	{
		char ammo_text[8] = { };
		std::snprintf(ammo_text, sizeof(ammo_text), "%d", clip);
		renderer::text(box.x + box.w + 4.f, y - 6.f, 11.f, color, ammo_text);
	}
}

static void draw_flags(box_t& box, player_t* player, const vec3_t& eye_position, color_t color, float alpha)
{
	if (!config::context.draw_flags) return;

	char buffer[96] = { };
	char lines[8][24] = { };
	std::size_t length = 0;
	std::size_t line_count = 0;

	const auto append = [&](const char* text)
	{
		if (!text || !text[0]) return;
		if (line_count < 8)
		{
			std::snprintf(lines[line_count], sizeof(lines[line_count]), "%s", text);
			line_count++;
		}

		const std::size_t text_length = std::strlen(text);
		const std::size_t separator = length != 0 ? 2 : 0;

		if (length + separator + text_length >= sizeof(buffer)) return;

		if (separator != 0)
		{
			buffer[length++] = ',';
			buffer[length++] = ' ';
		}

		std::memcpy(buffer + length, text, text_length);
		length += text_length;
		buffer[length] = '\0';
	};

	for (std::uint32_t i = 0; i < esp_flag_count; i++)
	{
		if (!config::context.esp_flags[i]) continue;

		switch (esp_flags_table[i].flag)
		{
		case esp_flag_armor:
			if (player->armor_value() > 0) append("kevlar");
			break;

		case esp_flag_helmet:
			if (player->has_helmet()) append("helmet");
			break;

		case esp_flag_kit:
			if (player->has_defuser()) append("kit");
			break;

		case esp_flag_defusing:
			if (player->is_defusing()) append("defusing");
			break;

		case esp_flag_distance:
		{
			game_scene_node_t* scene_node = player->game_scene_node();

			if (!scene_node) break;

			const vec3_t origin = scene_node->abs_origin();
			const float delta_x = origin.x - eye_position.x;
			const float delta_y = origin.y - eye_position.y;
			const float delta_z = origin.z - eye_position.z;

			char distance_text[16] = { };
			std::snprintf(distance_text, sizeof(distance_text), "%.0fm", std::sqrtf(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z));

			append(distance_text);
			break;
		}
		default: break;
		}
	}

	if (length == 0) return;

	color = with_alpha(color, alpha);

	if (config::context.esp_flags_position == 1)
	{
		for (std::size_t i = 0; i < line_count; i++)
		{
			renderer::text(box.x + box.w + 5.f, box.y + static_cast<float>(i) * 12.f, 11.f, color, lines[i]);
		}

		return;
	}

	renderer::text_centered(box.x, box.y + box.h + 18.f, box.w, box.h, color, buffer);
}

static void trigger_hitmarker()
{
	if (!config::context.hitmarker) return;

	hitmarker_until = ImGui::GetTime() + static_cast<double>((std::max)(0.05f, config::context.hitmarker_duration));
}

namespace {
    std::atomic<unsigned> hitmarker_pending{0};
    struct hitmarker_listener_t {
        virtual ~hitmarker_listener_t() = default;
        virtual void fire(void* event) {
            if (!event || !sdk::local_controller) return;
            struct event_key_t { std::uintptr_t hash; const char* name; };
            event_key_t attacker_key{0, "attacker"};
            event_key_t victim_key{0, "userid"};
            auto attacker = utilities::call_virtual<16U, controller_t*>(event, &attacker_key);
            auto victim = utilities::call_virtual<16U, controller_t*>(event, &victim_key);
            if (attacker == sdk::local_controller && victim != sdk::local_controller)
                hitmarker_pending.fetch_add(1, std::memory_order_relaxed);
        }
        virtual int debug_id() { return 42; }
    } hitmarker_listener;
    void* hitmarker_manager = nullptr;
}

void visuals::initialize_hitmarker_events()
{
    if (hitmarker_manager) return;
    auto match = utilities::pattern_scan(L"client.dll", "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? FF C3");
    if (!match) {
        console::log("[-] hitmarker: event manager signature missing\n");
        return;
    }
    auto address = utilities::resolve_rip(match, 3, 7);
    auto manager = address ? *reinterpret_cast<void**>(address) : nullptr;
    if (!manager || !utilities::call_virtual<3U, bool>(manager, &hitmarker_listener, "player_hurt", false)) {
        console::log("[-] hitmarker: player_hurt listener registration failed\n");
        return;
    }
    hitmarker_manager = manager;
    console::log("[+] hitmarker: player_hurt listener registered\n");
}

void visuals::shutdown_hitmarker_events()
{
    if (hitmarker_manager) utilities::call_virtual<5U, void>(hitmarker_manager, &hitmarker_listener);
    hitmarker_manager = nullptr;
    hitmarker_pending.store(0, std::memory_order_relaxed);
}

static void update_hitmarker()
{
    const auto hits = hitmarker_pending.exchange(0, std::memory_order_relaxed);
    if (!config::context.hitmarker || !interfaces::engine || !interfaces::engine->is_in_game()) {
        hitmarker_until = 0.0;
        return;
    }
    if (hits) trigger_hitmarker();
}

static void draw_hitmarker()
{
	if (!config::context.hitmarker) return;
	const double now = ImGui::GetTime();
	if (now > hitmarker_until) return;

	const float remaining = static_cast<float>(hitmarker_until - now);
	const float alpha = (std::clamp)(remaining / (std::max)(0.05f, config::context.hitmarker_duration), 0.f, 1.f);
	const auto a = static_cast<std::uint8_t>(alpha * 255.f);
	const float center_x = static_cast<float>(sdk::screen_width) * 0.5f;
	const float center_y = static_cast<float>(sdk::screen_height) * 0.5f;
	const float gap = (std::clamp)(config::context.hitmarker_gap, 0.f, 24.f);
	const float size = (std::clamp)(config::context.hitmarker_size, 2.f, 32.f);
	const float thickness = (std::clamp)(config::context.hitmarker_thickness, 1.f, 6.f);
	const color_t outline(0, 0, 0, a);
	color_t color = color_from_float(config::context.hitmarker_color, alpha);

	renderer::line(center_x - gap - size, center_y - gap - size, center_x - gap, center_y - gap, outline, 3.f);
	renderer::line(center_x + gap, center_y - gap, center_x + gap + size, center_y - gap - size, outline, 3.f);
	renderer::line(center_x - gap - size, center_y + gap + size, center_x - gap, center_y + gap, outline, 3.f);
	renderer::line(center_x + gap, center_y + gap, center_x + gap + size, center_y + gap + size, outline, 3.f);

	renderer::line(center_x - gap - size, center_y - gap - size, center_x - gap, center_y - gap, color, thickness);
	renderer::line(center_x + gap, center_y - gap, center_x + gap + size, center_y - gap - size, color, thickness);
	renderer::line(center_x - gap - size, center_y + gap + size, center_x - gap, center_y + gap, color, thickness);
	renderer::line(center_x + gap, center_y + gap, center_x + gap + size, center_y + gap + size, color, thickness);
}

static color_t resolve_esp_color(bool teammate, std::int32_t health, float distance) {
	const float* base = teammate ? config::context.esp_colors_teammate : config::context.esp_colors;

	float red = base[0];
	float green = base[1];
	float blue = base[2];

	if (config::context.esp_color_by_health && !teammate)
	{
		const float scale = std::clamp(health / 100.f, 0.f, 1.f);
		red = scale;
		green = scale;
		blue = 0.f;
	}

	if (config::context.esp_color_by_distance && config::context.esp_max_distance > 0.f)
	{
		const float t = std::clamp(distance / config::context.esp_max_distance, 0.f, 1.f);
		red = std::lerp(red, 1.f, t);
		green = std::lerp(green, 1.f, t);
		blue = std::lerp(blue, 1.f, t);
	}

	const auto alpha = static_cast<std::uint8_t>(std::clamp(config::context.esp_opacity, 0.f, 1.f) * 255.f);

	return color_t(static_cast<std::uint8_t>(red * 255.f), static_cast<std::uint8_t>(green * 255.f), static_cast<std::uint8_t>(blue * 255.f), alpha);
}

static float resolve_esp_opacity(float distance)
{
	if (!config::context.esp_distance_fade || config::context.esp_max_distance <= 0.f) return std::clamp(config::context.esp_opacity, 0.f, 1.f);
	const float t = std::clamp(distance / config::context.esp_max_distance, 0.f, 1.f);
	return std::clamp(config::context.esp_opacity * (1.f - t), 0.f, 1.f);
}


void visuals::run_player_esp()
{
	update_hitmarker();
	if (!config::context.player_esp && !config::context.hitmarker) return;
	if (!interfaces::engine || !interfaces::engine->is_in_game()) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) { draw_hitmarker(); return; }

	const vec3_t local_eye_position = sdk::local_player->eye_position();
	box_t box = { };

	for (std::uint32_t i = 1; i <= 64; i++)
	{
		controller_t* controller = interfaces::entity_list->get_controller_by_index(i);
		if (!controller) continue;
		player_t* player = interfaces::entity_list->get_player_from_controller(controller);
		if (!player || !player->is_alive())
		{
			esp_bars[i] = {};
			continue;
		}

		const bool teammate = player->team() == sdk::local_player->team();
		if (teammate && !config::context.teammate_esp) continue;

		vec3_t origin{};
		if (!player->try_abs_origin(origin)) continue;
		const float delta_x = origin.x - local_eye_position.x;
		const float delta_y = origin.y - local_eye_position.y;
		const float delta_z = origin.z - local_eye_position.z;
		const float distance = std::sqrtf(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

		if (config::context.esp_max_distance > 0.f && distance > config::context.esp_max_distance) continue;

		const std::int32_t health = player->health();
		auto& bars = esp_bars[i];
		if (bars.player != player)
		{
			bars = {};
			bars.player = player;
			bars.health = static_cast<float>(health);
		}
		if (!config::context.health_smooth || bars.health <= health) bars.health = static_cast<float>(health);
		else bars.health = (std::max)(static_cast<float>(health), bars.health - 100.f * ImGui::GetIO().DeltaTime);

		if (!config::context.player_esp) continue;

		const float alpha = resolve_esp_opacity(distance);
		const color_t box_color = with_alpha(color_from_float(config::context.esp_box_color), alpha);
		const color_t name_color = color_from_float(config::context.esp_name_color);
		const color_t health_color = color_from_float(config::context.esp_health_color);
		const color_t weapon_color = color_from_float(config::context.esp_weapon_color);
		const color_t ammo_color = color_from_float(config::context.esp_ammo_color);
		const color_t skeleton_color = color_from_float(config::context.esp_skeleton_color);
		const color_t flags_color = color_from_float(config::context.esp_flags_color);
		const color_t arrow_color = with_alpha(color_from_float(config::context.out_of_fov_arrow_color), alpha);

		vec3_t screen_origin{};
		const bool origin_on_screen = math::world_to_screen(origin, screen_origin) && is_screen_point_visible(screen_origin);

		if (!get_bounding_box(player, box))
		{
			draw_out_of_fov_arrow(local_eye_position, origin, arrow_color);
			continue;
		}

		if (!origin_on_screen)
		{
			draw_out_of_fov_arrow(local_eye_position, origin, arrow_color);
			continue;
		}

		draw_box(box, box_color);
		draw_skeleton(player, skeleton_color, alpha);
		draw_health(box, health, bars.health, health_color, alpha);
		draw_ammo_bar(box, player, bars, ammo_color, alpha);
		draw_weapon(box, player, weapon_color, alpha);
		draw_flags(box, player, local_eye_position, flags_color, alpha);
		draw_name(box, controller->name(), name_color, alpha);
	}

	draw_hitmarker();
}

bool visuals::glow_override(void* glow_property, float* color)
{
	if (!glow_property || !config::context.glow || !sdk::local_player || !interfaces::engine->is_in_game()) return false;
	auto player = reinterpret_cast<player_t*>(reinterpret_cast<glow_property_t*>(glow_property)->owner());
	auto local = sdk::local_player;
	if (!player || !player->is_player_pawn() || player == local || !player->is_alive()) return false;
	if (!config::context.teammate_esp && player->team() == local->team()) return false;
	const bool visible = is_visible(local, player, player->eye_position());
	if (!visible && !config::context.glow_invincible) return false;
	if (color)
	{
		const auto rgb = visible ? config::context.glow_color : config::context.glow_color_invincible;
		for (int i = 0; i < 3; i++) color[i] = std::clamp(rgb[i] * config::context.glow_brightness, 0.f, 1.f);
		color[3] = std::clamp(config::context.glow_alpha, 0.f, 1.f);
	}
	return true;
}

static void apply_player_glow(player_t* player)
{
	auto local = sdk::local_player;
	if (!player || !local || player == local) return;
	auto glow = &player->glow();
	if (!player->is_alive())
	{
		glow->glowing() = false;
		return;
	}
	bool enabled = true;
	const bool visible = is_visible(local, player, player->eye_position());
	if (!config::context.teammate_esp && player->team() == local->team()) enabled = false;
	if (!config::context.glow_invincible && !visible) enabled = false;
	if (!enabled || !config::context.glow)
	{
		glow->glowing() = false;
		return;
	}
	const float* rgb = visible ? config::context.glow_color : config::context.glow_color_invincible;
	const color_t color(
		static_cast<std::uint8_t>((std::clamp)(rgb[0] * config::context.glow_brightness, 0.f, 1.f) * 255.f),
		static_cast<std::uint8_t>((std::clamp)(rgb[1] * config::context.glow_brightness, 0.f, 1.f) * 255.f),
		static_cast<std::uint8_t>((std::clamp)(rgb[2] * config::context.glow_brightness, 0.f, 1.f) * 255.f),
		static_cast<std::uint8_t>((std::clamp)(config::context.glow_alpha, 0.f, 1.f) * 255.f));
	if (config::context.glow)
	{
		glow->glow_color() = color;
		glow->glow_type() = 3;
		glow->glow_range() = 0;
		glow->glow_range_min() = 0;
		glow->eligible_for_screen_highlight() = true;
		glow->glowing() = true;
	}
	else glow->glowing() = false;
}

void visuals::run_glow()
{
	if (!interfaces::engine || !interfaces::engine->is_in_game() || !interfaces::entity_list || !sdk::local_player) return;
	for (std::uint32_t i = 1; i <= 64; i++)
	{
		auto controller = interfaces::entity_list->get_controller_by_index(i);
		if (!controller) continue;
		apply_player_glow(interfaces::entity_list->get_player_from_controller(controller));
	}
}


bool visuals::should_chams(std::uintptr_t scene_object)
{
	if (!config::context.chams) return false;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return false;

	player_t* player = player_from_scene_object(scene_object);
	if (!player) return false;
	if (player == sdk::local_player) return false;
	//if (!player->is_alive()) return false;
	const bool teammate = player->team() == sdk::local_player->team();
	if (teammate && !config::context.teammate_esp) return false;
	return true;
}

/*
bool visuals::should_local_chams(std::uintptr_t scene_objct) {
	if (!config::context.chams_local) return false;
	player_t* player;
	const bool teammate = player->team() == sdk::local_player->team();
	if (sdk::local_player || sdk::local_player->is_alive() || !player || !teammate) {
		return true;
	}
	return false;
}
*/
bool visuals::apply_chams(std::uintptr_t primitive_buffer, int first, int material_id, bool ignore_z, const float* color)
{
	if (!color) return false;
	return apply_chams_layer(primitive_buffer, first, material_id, ignore_z, color_from_float(color, config::context.esp_opacity));
}

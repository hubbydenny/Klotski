#include "visuals.hpp"

#include "../hooks/hooks.hpp"

#include "../utilities/utilities.hpp"
#include "../source2-sdk/sdk.hpp"
#include "../config/config.hpp"
#include "../source2-sdk/classes/players.hpp"
#include "../source2-sdk/classes/weapons.hpp"
#include "../source2-sdk/interfaces/visible.hpp"

#include <array>
#include <algorithm>
#include <limits>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

typedef struct _box_t
{
	float x, y, w, h;
} box_t;

static cs_weapon_base_t* get_active_weapon(player_t* player)
{
	cs_player_weapon_services_t* services = player->weapon_services();
	if (!utilities::is_valid_pointer(services)) return nullptr;
	const entity_handle_t handle = services->active_weapon();
	if (handle == invalid_entity_handle) return nullptr;
	return reinterpret_cast<cs_weapon_base_t*>(interfaces::entity_list->get_base_entity(handle_index(handle)));
}

static bool get_bounding_box(player_t* player, box_t& box)
{
	vec3_t origin = player->game_scene_node()->abs_origin();

	vec3_t flb, brt, blb, frt, frb, brb, blt, flt = { };
	float left, top, right, bottom = 0.f;

	vec3_t min = player->collision_property()->mins() + origin;
	vec3_t max = player->collision_property()->maxs() + origin;

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

	return true;
}

static void draw_box(box_t& box)
{
	if (!config::context.draw_box) return;

	renderer::rect(box.x - 1, box.y - 1, box.w + 2, box.h + 2, color_t::black());
	renderer::rect(box.x + 1, box.y + 1, box.w - 2, box.h - 2, color_t::black());
	renderer::rect(box.x, box.y, box.w, box.h, color_t::white());
}

static void draw_health(box_t& box, std::int32_t player_health)
{
	if (!config::context.draw_health) return;
	
	float bar_height = player_health * box.h / 100.f;
	std::uint8_t color_scale = static_cast<std::uint8_t>(player_health * 2.55);

	box_t bar_box = { box.x - 5.f, box.y, 3.f, box.h };
	box_t health_bar = { box.x - 4.f, box.y + box.h - bar_height, 1.f, bar_height };

	renderer::filled_rect(bar_box.x, bar_box.y, bar_box.w, bar_box.h, color_t::black());
	renderer::filled_rect(health_bar.x, health_bar.y, health_bar.w, health_bar.h, color_t(255 - color_scale, color_scale, 0));
}

static void draw_name(box_t& box, const char* player_name)
{
	if (!config::context.draw_name) return;
	std::string name(player_name);

	if (name.empty()) return;
	if (name.size() > 15)name = name.substr(0, 15) + "...";
	renderer::text_centered(box.x, box.y, box.w, box.h, color_t::white(), name.c_str());
}

static void draw_armor(box_t& box, std::int32_t player_armor, player_t* player) {
	if (!config::context.draw_armor) return;

	//if (!player->has_armor(player)) return;
	
	float bar_width = player_armor * box.w / 100.f;
	box_t bar_box = { box.x, box.y + box.h + 3.f, box.w, 3.f };
	box_t armor_bar = { box.x, box.y + box.h + 3.f, bar_width, 3.f };

	renderer::filled_rect(bar_box.x, bar_box.y, bar_box.w, bar_box.h, color_t::black());
	renderer::filled_rect(armor_bar.x, armor_bar.y, armor_bar.w, armor_bar.h, color_t(0, 0, 255, 255));
}

static void draw_skeleton(player_t* player)
{
	if (!config::context.draw_skeleton) return;

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

			renderer::line(from_screen.x, from_screen.y, to_screen.x, to_screen.y, color_t::white());
		}
	}
}

static const char* get_weapon_display_name(const char* designer_name)
{
	if (!designer_name) return nullptr;
	for (const weapon_name_entry_t& entry : weapon_names) {if (std::strcmp(entry.designer_name, designer_name) == 0) return entry.display_name;}
	return designer_name;
}
static void draw_weapon(box_t& box, player_t* player) {
	cs_weapon_base_t* weapon = get_active_weapon(player);
	if (!config::context.draw_weapon) return;
	if (!weapon) return;

	const char* weapon_name = get_weapon_display_name(weapon->get_designer_name());
	if (!weapon_name) return;

	renderer::text_centered(box.x, box.y + box.h + 6, box.w, box.h, color_t::white(), weapon_name);
}

static void apply_glow(player_t* player, bool teammate)
{
	// todo add invisible
	glow_property_t& glow = player->glow();
	glow.glow_type() = 3;
	glow.glow_team() = player->team();
	glow.glow_range() = 5000;
	glow.glow_range_min() = 0;
	glow.glow_color() = teammate ? color_t(0, 255, 255, 255) : color_t(255, 64, 64, 255);

	glow.glow_time() = 0.f;
	glow.glow_start_time() = 0.f;

	glow.eligible_for_screen_highlight() = true;
	glow.glowing() = true;
}
static void draw_flags(box_t& box, player_t* player, const vec3_t& eye_position)
{
	if (!config::context.draw_flags) return;

	char buffer[96] = { };
	std::size_t length = 0;

	const auto append = [&](const char* text)
	{
		if (!text || !text[0]) return;

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

			if (!utilities::is_valid_pointer(scene_node)) break;

			const vec3_t origin = scene_node->abs_origin();
			const float delta_x = origin.x - eye_position.x;
			const float delta_y = origin.y - eye_position.y;
			const float delta_z = origin.z - eye_position.z;

			char distance_text[16] = { };
			std::snprintf(distance_text, sizeof(distance_text), "%.0fm", std::sqrtf(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z));

			append(distance_text);
			break;
		}

		default:
			break;
		}
	}

	if (length == 0) return;

	renderer::text_centered(box.x, box.y + box.h + 18.f, box.w, box.h, color_t::white(), buffer);
}

void visuals::run_player_esp()
{
	if (!config::context.player_esp) return;
	if (!interfaces::engine->is_in_game()) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;

	const vec3_t local_eye_position = sdk::local_player->get_eye_position();
	box_t box = { };

	for (std::uint32_t i = 1; i <= 64; i++)
	{
		controller_t* controller = interfaces::entity_list->get_controller_by_index(i);
		if (!controller) continue;
		player_t* player = interfaces::entity_list->get_player_from_controller(controller);
		if (!player || !player->is_alive()) continue;
		if (player->team() == sdk::local_player->team() && !config::context.teammate_esp) continue;

		if (!get_bounding_box(player, box)) continue;
	
		draw_box(box);
		draw_skeleton(player);
		draw_health(box, player->health());
		draw_armor(box, player->armor_value(), player);
		draw_weapon(box, player);
		draw_flags(box, player, local_eye_position);
		draw_name(box, controller->name());
	}
}

void visuals::on_draw_glow(void* glow_property)
{
	if (!glow_property) return;
	if (!sdk::local_player || !utilities::is_valid_pointer(sdk::local_player)) return;

	glow_property_t* glow = reinterpret_cast<glow_property_t*>(glow_property);

	player_t* player = *reinterpret_cast<player_t**>(reinterpret_cast<std::uintptr_t>(glow_property) + 0x18);

	if (!utilities::is_valid_pointer(player)) return;

	bool is_player = false;

	for (std::int32_t i = 1; i <= 64; i++)
	{
		controller_t* controller = interfaces::entity_list->get_controller_by_index(i);

		if (!controller) continue;

		if (interfaces::entity_list->get_player_from_controller(controller) == player)
		{
			is_player = true;
			break;
		}
	}

	if (!is_player) return;
	if (player == sdk::local_player) return;
	if (!player->is_alive()) return;

	const bool invincible = !is_visible(sdk::local_player, player, player->get_eye_position());

	bool set_glow = config::context.glow;

	if (invincible && !config::context.glow_invincible)
	{
		set_glow = false;
	}

	if (set_glow)
	{
		const float* color = invincible ? config::context.glow_color_invincible : config::context.glow_color;

		const std::uint8_t red = static_cast<std::uint8_t>(std::clamp(color[0] * config::context.glow_brightness, 0.f, 1.f) * 255.f);
		const std::uint8_t green = static_cast<std::uint8_t>(std::clamp(color[1] * config::context.glow_brightness, 0.f, 1.f) * 255.f);
		const std::uint8_t blue = static_cast<std::uint8_t>(std::clamp(color[2] * config::context.glow_brightness, 0.f, 1.f) * 255.f);
		const std::uint8_t alpha = static_cast<std::uint8_t>(std::clamp(config::context.glow_alpha, 0.f, 1.f) * 255.f);

		glow->glow_color() = color_t(red, green, blue, alpha);
		glow->glowing() = true;
	}
	else
	{
		glow->glowing() = false;
	}
}


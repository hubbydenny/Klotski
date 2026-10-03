#include "visuals.hpp"

#include "../../hooks/hooks.hpp"

#include "../../utilities/utilities.hpp"
#include "../../source2-sdk/sdk.hpp"
#include "../config/config.hpp"
#include "../../source2-sdk/classes/players.hpp"
#include "../../source2-sdk/classes/weapons.hpp"
#include "../../source2-sdk/interfaces/visible.hpp"

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

	cs_weapon_base_t* weapon = reinterpret_cast<cs_weapon_base_t*>(interfaces::entity_list->get_base_entity(handle_index(handle)));

	return utilities::is_valid_pointer(weapon) ? weapon : nullptr;
}

static bool get_bounding_box(player_t* player, box_t& box)
{
	game_scene_node_t* scene_node = player->game_scene_node();

	if (!utilities::is_valid_pointer(scene_node)) return false;

	collision_property_t* collision = player->collision_property();

	if (!utilities::is_valid_pointer(collision)) return false;

	vec3_t origin = scene_node->abs_origin();

	vec3_t flb, brt, blb, frt, frb, brb, blt, flt = { };
	float left, top, right, bottom = 0.f;

	vec3_t min = collision->mins() + origin;
	vec3_t max = collision->maxs() + origin;

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

static void draw_full_box(box_t& box, color_t color)
{
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

static void draw_health(box_t& box, std::int32_t player_health, color_t tint, float alpha)
{
	if (!config::context.draw_health) return;

	float bar_height = player_health * box.h / 100.f;
	std::uint8_t color_scale = static_cast<std::uint8_t>(player_health * 2.55);

	box_t bar_box = { box.x - 5.f, box.y, 3.f, box.h };
	box_t health_bar = { box.x - 4.f, box.y + box.h - bar_height, 1.f, bar_height };

	const auto a = static_cast<std::uint8_t>(alpha * 255.f);

	renderer::filled_rect(bar_box.x, bar_box.y, bar_box.w, bar_box.h, color_t(0, 0, 0, a));
	renderer::filled_rect(health_bar.x, health_bar.y, health_bar.w, health_bar.h,
		color_t(static_cast<std::uint8_t>(tint.r * color_scale), static_cast<std::uint8_t>(tint.g * color_scale), 0, a));
}

static void draw_name(box_t& box, const char* player_name, color_t tint, float alpha)
{
	if (!config::context.draw_name) return;
	if (!player_name) return;

	std::string name(player_name);

	if (name.empty()) return;
	if (name.size() > 15)name = name.substr(0, 15) + "...";
	renderer::text_centered(box.x, box.y, box.w, box.h,
		color_t(tint.r, tint.g, tint.b, static_cast<std::uint8_t>(alpha * 255.f)), name.c_str());
}

static void draw_armor(box_t& box, std::int32_t player_armor, color_t tint, float alpha) {
	if (!config::context.draw_armor) return;

	float bar_width = player_armor * box.w / 100.f;
	box_t bar_box = { box.x, box.y + box.h + 3.f, box.w, 3.f };
	box_t armor_bar = { box.x, box.y + box.h + 3.f, bar_width, 3.f };

	const auto a = static_cast<std::uint8_t>(alpha * 255.f);

	renderer::filled_rect(bar_box.x, bar_box.y, bar_box.w, bar_box.h, color_t(0, 0, 0, a));
	renderer::filled_rect(armor_bar.x, armor_bar.y, armor_bar.w, armor_bar.h,
		color_t(static_cast<std::uint8_t>(tint.b * 255.f), static_cast<std::uint8_t>(tint.b * 255.f), static_cast<std::uint8_t>(tint.b * 255.f), a));
}

static void draw_skeleton(player_t* player, color_t tint, float alpha)
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

			renderer::line(from_screen.x, from_screen.y, to_screen.x, to_screen.y, tint);
		}
	}
}

static const char* get_weapon_display_name(const char* designer_name)
{
	if (!designer_name) return nullptr;
	for (const weapon_name_entry_t& entry : weapon_names) {if (std::strcmp(entry.designer_name, designer_name) == 0) return entry.display_name;}
	return designer_name;
}
static void draw_weapon(box_t& box, player_t* player, color_t tint, float alpha) {
	cs_weapon_base_t* weapon = get_active_weapon(player);
	if (!config::context.draw_weapon) return;
	if (!weapon) return;

	const char* weapon_name = get_weapon_display_name(weapon->get_designer_name());
	if (!weapon_name) return;

	renderer::text_centered(box.x, box.y + box.h + 6, box.w, box.h,
		color_t(tint.r, tint.g, tint.b, static_cast<std::uint8_t>(alpha * 255.f)), weapon_name);
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
static void draw_flags(box_t& box, player_t* player, const vec3_t& eye_position, color_t tint, float alpha)
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

	renderer::text_centered(box.x, box.y + box.h + 18.f, box.w, box.h,
		color_t(tint.r, tint.g, tint.b, static_cast<std::uint8_t>(alpha * 255.f)), buffer);
}static color_t resolve_esp_color(bool teammate, std::int32_t health, float distance)
{
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
	if (!config::context.esp_distance_fade || config::context.esp_max_distance <= 0.f)
	{
		return std::clamp(config::context.esp_opacity, 0.f, 1.f);
	}

	const float t = std::clamp(distance / config::context.esp_max_distance, 0.f, 1.f);

	return std::clamp(config::context.esp_opacity * (1.f - t), 0.f, 1.f);
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

		const bool teammate = player->team() == sdk::local_player->team();
		if (teammate && !config::context.teammate_esp) continue;

		game_scene_node_t* scene_node = player->game_scene_node();

		if (!utilities::is_valid_pointer(scene_node)) continue;

		const vec3_t origin = scene_node->abs_origin();
		const float delta_x = origin.x - local_eye_position.x;
		const float delta_y = origin.y - local_eye_position.y;
		const float delta_z = origin.z - local_eye_position.z;
		const float distance = std::sqrtf(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

		if (config::context.esp_max_distance > 0.f && distance > config::context.esp_max_distance) continue;

		if (!get_bounding_box(player, box)) continue;

		const std::int32_t health = player->health();
		const color_t tint = resolve_esp_color(teammate, health, distance);
		const float alpha = static_cast<float>(tint.a) / 255.f;

		draw_box(box, tint);
		draw_skeleton(player, tint, alpha);
		draw_health(box, health, tint, alpha);
		draw_armor(box, player->armor_value(), tint, alpha);
		draw_weapon(box, player, tint, alpha);
		draw_flags(box, player, local_eye_position, tint, alpha);
		draw_name(box, controller->name(), tint, alpha);
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


#include "combat.hpp"

#include "../config/config.hpp"
#include "../../source2-sdk/sdk.hpp"
#include "../../utilities/utilities.hpp"
#include "../../source2-sdk/interfaces/i_trace.hpp"
#include "../../source2-sdk/interfaces/visible.hpp"

static vec3_t get_target_position(player_t* player, const vec3_t& eye_position, const vec3_t& view_angles, float& best_fov)
{
	player->update_bones();

	std::int32_t bone_count = 0;
	bone_data_t* bones = player->get_bone_cache(&bone_count);

	if (bones && bone_count > 0)
	{
		vec3_t best_position = vec3_t();
		float lowest_fov = best_fov;
		bool selected = false;

		for (std::uint32_t i = 0; i < aim_bone_count; i++)
		{
			if (!config::context.aim_bones[i]) continue;

			selected = true;

			const std::int32_t bone = static_cast<std::int32_t>(aim_bones_table[i].bone);

			if (bone >= bone_count) continue;

			const vec3_t position = bones[bone].position;

			if (position.is_zero()) continue;

			const float fov = math::angle_distance(eye_position, position, view_angles);

			if (fov < lowest_fov)
			{
				lowest_fov = fov;
				best_position = position;
			}
		}

		if (!selected)
		{
			const std::int32_t head = static_cast<std::int32_t>(bone_ids::head);

			if (head >= bone_count) return vec3_t();

			const vec3_t position = bones[head].position;

			if (position.is_zero()) return vec3_t();

			const float fov = math::angle_distance(eye_position, position, view_angles);

			if (fov >= lowest_fov) return vec3_t();

			lowest_fov = fov;
			best_position = position;
		}

		if (best_position.is_zero()) return vec3_t();

		best_fov = lowest_fov;

		return best_position;
	}

	game_scene_node_t* scene_node = player->game_scene_node();

	if (!utilities::is_valid_pointer(scene_node))
	{
		return vec3_t();
	}

	vec3_t fallback = scene_node->abs_origin();
	fallback.z += player->view_offset().z;

	const float fov = math::angle_distance(eye_position, fallback, view_angles);

	if (fov >= best_fov) return vec3_t();

	best_fov = fov;

	return fallback;
}

void combat::run_rcs()
{
	if (!interfaces::csgo_input) return;
	if (!interfaces::engine->is_in_game()) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;

	vec3_t* view_angles = interfaces::csgo_input->get_view_angles(0);

	if (!view_angles) return;

	const vec3_t aim_punch = sdk::local_player->get_aim_punch();

	if (config::context.standalone_rcs)
	{
		combat::update_standalone_rcs(
			*view_angles,
			aim_punch,
			config::context.standalone_rcs_strength,
			config::context.standalone_rcs_min,
			config::context.standalone_rcs_max,
			!config::context.aimbot
		);
	}

	}

void combat::run_aimbot(controller_t* local_controller) {
	if (!local_controller || !interfaces::csgo_input) return;
	if (!interfaces::engine->is_in_game()) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;

	vec3_t* view_angles = interfaces::csgo_input->get_view_angles(0);

	if (!view_angles) return;
	const vec3_t local_eye_position = sdk::local_player->get_eye_position();

	float best_fov = config::context.aimbot_fov;
	vec3_t best_position = vec3_t();
	bool seen_enemy = false;

	for (std::int32_t i = 1; i <= 64; i++)
	{
		controller_t* controller = interfaces::entity_list->get_controller_by_index(i);
		player_t* player = interfaces::entity_list->get_player_from_controller(controller);

		if (!controller) continue;
		if (!player || player == sdk::local_player) continue;
		if (!player->is_alive() || player->has_gun_immunity()) continue;
		if (player->team() == sdk::local_player->team()) continue;

		game_scene_node_t* scene_node = player->game_scene_node();

		if (!utilities::is_valid_pointer(scene_node) || scene_node->dormant()) continue;

		float target_fov = best_fov;
		const vec3_t target_position = get_target_position(player, local_eye_position, *view_angles, target_fov);

		if (target_position.is_zero()) continue;
		if (config::context.onlyvisible && !is_visible(sdk::local_player, player, target_position)) continue;

		seen_enemy = true;

		if (target_fov < best_fov)
		{
			best_fov = target_fov;
			best_position = target_position;
		}
	}

	if (best_position.is_zero())
	{
		static bool logged_seen = false;
		static bool logged_missing = false;

		if (seen_enemy && !logged_seen)
		{
			logged_seen = true;
			debug::log("[-] aimbot: enemy found but outside fov\n");
		}

		if (!seen_enemy && !logged_missing)
		{
			logged_missing = true;
			debug::log("[-] aimbot: no enemy found\n");
		}

		return;
	}

	vec3_t aim_delta = math::calculate_angle(local_eye_position, best_position, *view_angles);

	if (config::context.aimbot_smooth)
	{
		const float speed = config::context.aimbot_speed / 100.0f;

		aim_delta.x *= speed;
		aim_delta.y *= speed;
		aim_delta.z = 0.0f;
	}

	vec3_t new_angles = *view_angles + aim_delta;

	new_angles.clamp();

	interfaces::csgo_input->set_view_angles(new_angles);

	static bool logged_aim = false;

	if (!logged_aim)
	{
		logged_aim = true;
		debug::log("[+] aimbot: aiming\n");
	}
}
static float compute_rcs_factor(int rand_min, int rand_max)
{
	const float current_time = interfaces::globals ? interfaces::globals->curtime : 0.0f;

	const auto seed = static_cast<std::uint32_t>(current_time * 1000.0f);
	const auto t = static_cast<float>(seed % 1000) / 1000.0f;

	const auto min_scale = static_cast<float>(rand_min) / 100.0f;
	const auto max_scale = static_cast<float>(rand_max) / 100.0f;

	return min_scale + (max_scale - min_scale) * t;
}

static vec3_t g_old_punch{};

void combat::update_standalone_rcs(const vec3_t& view_angles, const vec3_t& aim_punch, int amount, int rand_min, int rand_max, bool apply)
{
	if (!sdk::local_player || !interfaces::csgo_input) return;

	const std::int32_t shots_fired = sdk::local_player->shots_fired();

	if (shots_fired > 1)
	{
		const auto factor = compute_rcs_factor(rand_min, rand_max);
		const auto scale = static_cast<float>(amount) / 100.0f;

		vec3_t punch_scaled
		{
			aim_punch.x * scale * factor,
			aim_punch.y * scale * factor,
			0.0f
		};

		if (apply)
		{
			vec3_t new_angles = view_angles;
			new_angles.x += g_old_punch.x - punch_scaled.x;
			new_angles.y += g_old_punch.y - punch_scaled.y;
			new_angles.clamp();

			interfaces::csgo_input->set_view_angles(new_angles);
		}

		g_old_punch = punch_scaled;
	}
	else
	{
		g_old_punch = vec3_t();
	}
}




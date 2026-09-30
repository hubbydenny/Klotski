#include "movement.hpp"

#include "../../config/config.hpp"
#include "../../config/binds.hpp"
#include "../../menu/menu.hpp"
#include "../../source2-sdk/classes/entity.hpp"
#include "../../source2-sdk/classes/trace.hpp"
#include "../../source2-sdk/classes/types.hpp"
#include "../../source2-sdk/classes/user_cmd.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"
#include "../../source2-sdk/math/math.hpp"
#include "../../source2-sdk/classes/players.hpp"
#include "../../source2-sdk/sdk.hpp"
#include "../../utilities/utilities.hpp"
#include "../../hooks/hooks.hpp"
#include "../../features/prediction/prediction.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include <Windows.h>

static bool logged_missing_base = false;

namespace
{
	constexpr float k_rad_to_deg{ 180.0f / std::numbers::pi_v<float> };
	constexpr float k_scan_inset{ 1.0f };
	constexpr float k_scan_reach{ 4.0f };
	constexpr float k_max_surface_normal_z{ 0.7f };
	constexpr float k_ground_trace_length{ 2048.0f };
	constexpr float k_ground_offset{ 8.0f };
	constexpr float k_retry_offset{ 32.0f };
	constexpr float k_min_speed{ 5.0f };
	constexpr float k_min_turn_angle{ 10.0f };
	constexpr float k_air_slip_min_z{ -8.293333f };
	constexpr float k_air_slip_max_z{ -5.628950f };

	constexpr std::uint64_t k_movement_buttons{ in_forward | in_back | in_moveleft | in_moveright };
	constexpr std::uint64_t k_trace_mask{ MASK_PLAYERSOLID_BRUSHONLY | CONTENTS_HITBOX };
	const vec3_t k_default_hull_mins(-16.f, -16.f, 0.f);
	const vec3_t k_default_hull_maxs(16.f, 16.f, 72.f);

	struct surface_candidate
	{
		float target_yaw{};
		float distance{ (std::numeric_limits<float>::max)() };
		bool move_right{};
		bool found{};
	};

	constexpr int k_edgebug_max_path = 128;

	struct edgebug_state_t
	{
		bool active{};
		std::int32_t end_tick{};
		bool ducking{};
	};

	edgebug_state_t edgebug_state = { };

	struct edgebug_visual_t
	{
		int path_count{};
		vec3_t path[k_edgebug_max_path];
		vec3_t pos{};
		std::int32_t ticks_left{};
		bool valid{};
	};

	edgebug_visual_t edgebug_visual = { };

	void edgebug_zero_move(user_cmd_t* cmd, bool ducked)
	{
		cbase_user_cmd_pb_t* base = cmd->get_base();

		if (base)
		{
			base->has_bits |= cbase_user_cmd_pb_t::has_forwardmove | cbase_user_cmd_pb_t::has_leftmove | cbase_user_cmd_pb_t::has_mousedx | cbase_user_cmd_pb_t::has_mousedy;
			base->forwardmove = 0.f;
			base->leftmove = 0.f;
			base->mousedx = 0;
			base->mousedy = 0;
		}

		cmd->buttonstate1 &= ~k_movement_buttons;
		cmd->set_button(in_duck, ducked);
		cmd->sync_buttons();
	}

	bool edgebug_in_bad_movetype(player_t* local_player)
	{
		const movetype_t move_type = local_player->move_type();

		return move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer;
	}

	struct wall_alignment
	{
		float parallel_yaw{};
		bool surface_side{};
	};

	wall_alignment wall_parallel_yaw(const vec3_t& normal, float current_yaw)
	{
		const float normal_yaw = std::atan2f(normal.y, normal.x) * k_rad_to_deg;
		const float clockwise = normalize_yaw(normal_yaw - 90.0f);
		const float counter_clockwise = normalize_yaw(normal_yaw + 90.0f);
		const float clockwise_delta = std::fabsf(normalize_yaw(clockwise - current_yaw));
		const float counter_clockwise_delta = std::fabsf(normalize_yaw(counter_clockwise - current_yaw));
		const bool clockwise_closer = clockwise_delta <= counter_clockwise_delta;

		return { clockwise_closer ? clockwise : counter_clockwise, clockwise_closer };
	}

	float turn_angle_for_speed(float speed, bool slipping)
	{
		if (speed < 60.0f) return k_min_turn_angle;
		const float ratio = (slipping ? 30.006001f : 29.933001f) / speed;
		return std::asinf((std::clamp)(ratio, -1.0f, 1.0f)) * k_rad_to_deg;
	}

	bool get_player_hull(player_t* local_player, vec3_t* mins, vec3_t* maxs)
	{
		collision_property_t* collision = local_player->collision_property();

		if (utilities::is_valid_pointer(collision))
		{
			*mins = collision->mins();
			*maxs = collision->maxs();

			const bool sane = maxs->z > mins->z && maxs->z <= 128.f && mins->z >= -128.f && std::fabsf(maxs->x) <= 128.f && std::fabsf(maxs->y) <= 128.f;

			if (sane) return true;
		}
		*mins = k_default_hull_mins;
		*maxs = k_default_hull_maxs;
		return true;
	}

	bool should_log()
	{
		static std::uint32_t calls = 0;
		return (++calls % 32) == 0;
	}
}

void movement::run_bhop(user_cmd_t* cmd)
{
	if (!config::context.bhop || !utilities::is_valid_pointer(cmd)) return;
	player_t* local_player = sdk::local_player;

	if (!utilities::is_valid_pointer(local_player) || !local_player->is_alive()) return;
	if ((GetAsyncKeyState(VK_SPACE) & 0x8000) == 0) return;

	if (!cmd->get_base())
	{
		if (!logged_missing_base)
		{
			logged_missing_base = true;
			debug::log("[-] bhop: user cmd base unavailable\n");
		}
		return;
	}

	const bool on_ground = local_player->has_flag(fl_onground);
	cmd->set_button(in_jump, on_ground);
}

void movement::pixelsurf(user_cmd_t* cmd)
{
	if (!utilities::is_valid_pointer(cmd)) return;

	player_t* local_player = sdk::local_player;

	if (!utilities::is_valid_pointer(local_player) || !local_player->is_alive()) return;
	if (local_player->has_flag(fl_onground)) return;

	const movetype_t move_type = local_player->move_type();

	if (move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer) return;

	game_scene_node_t* scene_node = local_player->game_scene_node();
	player_movement_services_t* movement_services = local_player->movement_services();

	if (!utilities::is_valid_pointer(scene_node) || !utilities::is_valid_pointer(movement_services)) return;
	if (!utilities::is_valid_pointer(interfaces::trace)) return;

	cbase_user_cmd_pb_t* base = cmd->get_base();
	cmd_qangle_t* view = cmd->get_view_angles();

	if (!base || !view) return;

	const bool debug_now = should_log();

	const vec3_t origin = scene_node->abs_origin();
	const vec3_t velocity = local_player->velocity();
	const float pitch = view->angles.x;
	const float current_yaw = normalize_yaw(view->angles.y);
	const float speed = vel2d(velocity);
	const bool slipping = velocity.z >= k_air_slip_min_z && velocity.z <= k_air_slip_max_z;
	const bool velocity_reversed = speed >= k_min_speed && std::fabsf(normalize_yaw(std::atan2f(velocity.y, velocity.x) * k_rad_to_deg - current_yaw)) > 90.0f;
	const float turn_angle = turn_angle_for_speed(speed, slipping);

	vec3_t mins = k_default_hull_mins;
	vec3_t maxs = k_default_hull_maxs;
	get_player_hull(local_player, &mins, &maxs);

	trace_filter_t filter = { };

	if (!i_trace::make_movement_filter(&filter, local_player, k_trace_mask, COLLISION_GROUP_PLAYER_MOVEMENT)) return;

	const vec3_t directions[4] = { vec3_t(1.f, 0.f, 0.f), vec3_t(0.f, 1.f, 0.f), vec3_t(-1.f, 0.f, 0.f), vec3_t(0.f, -1.f, 0.f) };

	const auto consider_surface = [&](const vec3_t& scan_origin, const game_trace_t& trace, surface_candidate* candidate)
	{
		if (trace.fraction >= 1.0f || trace.start_solid) return;
		if (std::fabsf(trace.normal.z) >= k_max_surface_normal_z) return;

		const wall_alignment alignment = wall_parallel_yaw(trace.normal, current_yaw);
		const bool add_turn = slipping ^ velocity_reversed ^ alignment.surface_side;
		const float target_yaw = normalize_yaw(alignment.parallel_yaw + (add_turn ? turn_angle : -turn_angle));
		const float delta_x = trace.endpos.x - scan_origin.x;
		const float delta_y = trace.endpos.y - scan_origin.y;
		const float distance = std::sqrtf(delta_x * delta_x + delta_y * delta_y);

		if (distance >= candidate->distance) return;

		candidate->target_yaw = target_yaw;
		candidate->distance = distance;
		candidate->move_right = alignment.surface_side ^ slipping;
		candidate->found = true;
	};

	auto scan_surfaces = [&](const vec3_t& scan_origin, surface_candidate* candidate)
	{
		*candidate = surface_candidate();

		for (const vec3_t& direction : directions)
		{
			const float hull_extent = direction.x != 0.f ? (direction.x > 0.f ? maxs.x : -mins.x) : (direction.y > 0.f ? maxs.y : -mins.y);
			const float start_distance = hull_extent - k_scan_inset;
			const float end_distance = hull_extent + k_scan_reach;
			const vec3_t trace_start(scan_origin.x + direction.x * start_distance, scan_origin.y + direction.y * start_distance, scan_origin.z);
			const vec3_t trace_end(scan_origin.x + direction.x * end_distance, scan_origin.y + direction.y * end_distance, scan_origin.z);
			game_trace_t trace = { };

			if (!interfaces::trace->trace_line(trace_start, trace_end, &filter, &trace)) continue;
			consider_surface(scan_origin, trace, candidate);
		}
	};

	if (debug_now)
	{
		debug::log("[?] pixelsurf: cmd=%d last=%u origin=(%.1f %.1f %.1f) velocity=(%.1f %.1f %.1f) yaw=%.1f slot=%d\n",cmd->command_number(), movement_services->last_command_number_processed(), origin.x, origin.y, origin.z, velocity.x, velocity.y, velocity.z, current_yaw, base->subtick_moves.current_size);
	}

	surface_candidate best = { };
	scan_surfaces(origin, &best);

	if (!best.found)
	{
		trace_filter_t ground_filter = { };
		const vec3_t ground_end(origin.x, origin.y, origin.z - k_ground_trace_length);
		game_trace_t ground_trace = { };

		if (!i_trace::make_filter(&ground_filter, local_player, k_trace_mask, TRACE_LAYER_DEFAULT)) return;
		if (!interfaces::trace->trace_line(origin, ground_end, &ground_filter, &ground_trace)) return;
		if (ground_trace.fraction >= 1.0f || ground_trace.start_solid) return;

		const float ground_distance = origin.z - ground_trace.endpos.z;

		scan_surfaces(vec3_t(origin.x - k_retry_offset, origin.y - k_retry_offset, origin.z - k_retry_offset), &best);

		if (!best.found)
		{
			scan_surfaces(vec3_t(origin.x, origin.y, origin.z - ground_distance + k_ground_offset), &best);
		}
	}

	if (!best.found)
	{
		if (debug_now) debug::log("[-] pixelsurf: no surface found\n");		return;
	}

	cmd->set_view_angles(vec3_t(pitch, best.target_yaw, 0.f));
	cmd->set_move(0.f, best.move_right ? -1.f : 1.f);
	cmd->buttonstate1 = (cmd->buttonstate1 & ~k_movement_buttons) | (best.move_right ? in_moveright : in_moveleft);
	cmd->buttonstate2 |= k_movement_buttons;
	cmd->sync_buttons();
	cmd->clear_subticks();

	const bool crc_updated = cmd->update_move_crc();

	if (debug_now)
	{
		debug::log("[+] pixelsurf: yaw=%.1f move_right=%d distance=%.2f speed=%.1f slipping=%d reversed=%d turn=%.1f crc=%d\n",
			best.target_yaw, best.move_right ? 1 : 0, best.distance, speed, slipping ? 1 : 0,
			velocity_reversed ? 1 : 0, turn_angle, crc_updated ? 1 : 0);
	}

	if (!config::context.pixelsurf_silent && utilities::is_valid_pointer(interfaces::csgo_input))
	{
		vec3_t live_angles(pitch, best.target_yaw, 0.f);

		interfaces::csgo_input->set_view_angles(live_angles);

		if (debug_now) debug::log("[?] pixelsurf: live angles set to (%.1f %.1f)\n", pitch, best.target_yaw);
	}
}

void movement::edgebug(user_cmd_t* cmd)
{
	if (!config::context.edgebug || !utilities::is_valid_pointer(cmd)) return;

	const bool debug_now = config::context.edgebug_debug;

	if (debug_now) debug::log("[?] edgebug: tick enter\n");

	player_t* local_player = sdk::local_player;
	controller_t* local_controller = sdk::local_controller;

	if (!utilities::is_valid_pointer(local_player) || !utilities::is_valid_pointer(local_controller)) return;
	if (!local_player->is_alive())
	{
		if (debug_now) debug::log("[-] edgebug: not alive\n");
		return;
	}
	if (local_player->has_flag(fl_onground) || edgebug_in_bad_movetype(local_player))
	{
		if (debug_now)
		{
			if (local_player->has_flag(fl_onground)) debug::log("[-] edgebug: on ground\n");
			else debug::log("[-] edgebug: bad movetype %d\n", static_cast<int>(local_player->move_type()));
		}

		edgebug_state.active = false;
		edgebug_state.end_tick = 0;
		return;
	}

	if (!utilities::is_valid_pointer(interfaces::globals))
	{
		if (debug_now) debug::log("[-] edgebug: no globals\n");
		return;
	}

	const std::int32_t tick_count = interfaces::globals->tick_count;

	if (edgebug_state.active)
	{
		if (tick_count > edgebug_state.end_tick)
		{
			const bool was_active = edgebug_state.active;
			const bool was_ducking = edgebug_state.ducking;

			edgebug_state.active = false;
			edgebug_state.end_tick = 0;

			if (was_active) hooks::chat::chatprintf("klotski | edgebugged");

			if (debug_now) debug::log("[+] edgebug: done mode=%d\n", was_ducking ? 1 : 0);
			return;
		}

		edgebug_zero_move(cmd, edgebug_state.ducking);
		cmd->clear_subticks();
		cmd->update_move_crc();

		if (debug_now) debug::log("[?] edgebug: lock mode=%d left=%d\n", edgebug_state.ducking ? 1 : 0, edgebug_state.end_tick - tick_count);
		return;
	}

	if (!config::context.edgebug_key_state)
	{
		bool key_forced = false;

		for (const binds::bind_t& bind : binds::list)
		{
			if (bind.value == &config::context.edgebug_key_state && bind.key && binds::key_down(bind.key))
			{
				key_forced = true;
				break;
			}
		}

		if (!key_forced)
		{
			static std::int32_t last_logged_tick = -1;

			if (tick_count != last_logged_tick)
			{
				last_logged_tick = tick_count;

				if (debug_now)
				{
					bool bind_found = false;

					for (const binds::bind_t& bind : binds::list)
					{
						if (bind.value == &config::context.edgebug_key_state)
						{
							bind_found = true;
							debug::log("[-] edgebug: key not held key=0x%02X mode=%d menu_open=%d\n",
								bind.key, bind.mode, menu::open ? 1 : 0);
							break;
						}
					}

					if (!bind_found) debug::log("[-] edgebug: no bind, key not held\n");
				}
			}

			return;
		}
	}

	if (local_player->velocity().z >= 0.f)
	{
		if (debug_now) debug::log("[-] edgebug: vel.z %.2f not falling\n", local_player->velocity().z);
		return;
	}

	user_cmd_t sim_cmds[2] = { };

	if (!prediction::sim_setup())
	{
		if (debug_now) debug::log("[-] edgebug: sim_setup failed\n");
		return;
	}

	const float start_z_vel = local_player->velocity().z;

	edgebug_visual.valid = false;
	edgebug_visual.ticks_left = 0;
	edgebug_visual.path_count = 0;

	game_scene_node_t* scene_node = local_player->game_scene_node();

	for (int mode = 0; mode < 2; ++mode)
	{
		const bool ducked = mode == 0;

		prediction::sim_restore();

		if (debug_now) debug::log("[?] edgebug: sim start mode=%d start_z=%.3f ticks=%d\n", ducked ? 1 : 0, start_z_vel, config::context.edgebug_ticks);


		float prev_z_vel = start_z_vel;

		for (int i = 0; i < config::context.edgebug_ticks; ++i)
		{
			sim_cmds[mode] = *cmd;
			user_cmd_t* sim_cmd = &sim_cmds[mode];

			edgebug_zero_move(sim_cmd, ducked);
			sim_cmd->clear_subticks();
			sim_cmd->command_number_value = cmd->command_number_value + i + 1;

			prediction::sim_run(sim_cmd);

			game_scene_node_t* sim_node = local_player->game_scene_node();

			if (config::context.edgebug_visual && utilities::is_valid_pointer(sim_node) && edgebug_visual.path_count < k_edgebug_max_path)
			{
				edgebug_visual.path[edgebug_visual.path_count++] = sim_node->abs_origin();
			}

			const float new_z_vel = local_player->velocity().z;
			const bool new_on_ground = local_player->has_flag(fl_onground);

			if (prev_z_vel < -7.f && std::floorf(new_z_vel) == -6.f && !new_on_ground)
			{
				if (debug_now) debug::log("[+] edgebug: DETECT mode=%d tick_i=%d prev_z=%.3f z=%.3f\n", ducked ? 1 : 0, i, prev_z_vel, new_z_vel);

				edgebug_state.active = true;
				edgebug_state.ducking = ducked;
				edgebug_state.end_tick = tick_count + i + 1;

				if (config::context.edgebug_visual && utilities::is_valid_pointer(sim_node))
				{
					edgebug_visual.pos = sim_node->abs_origin();
					edgebug_visual.ticks_left = i + 1;
					edgebug_visual.valid = true;
				}

				edgebug_zero_move(cmd, ducked);
				cmd->clear_subticks();
				cmd->update_move_crc();

				prediction::sim_restore();
				return;
			}

			prev_z_vel = new_z_vel;

			if (debug_now) debug::log("[?] edgebug: sim mode=%d i=%d z=%.3f ground=%d\n", ducked ? 1 : 0, i, new_z_vel, new_on_ground ? 1 : 0);
		}
	}
	prediction::sim_restore();
}

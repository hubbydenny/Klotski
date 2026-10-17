#include "movement.hpp"

#include "../config/config.hpp"
#include "../config/binds.hpp"
#include "../menu/menu.hpp"
#include "../../source2-sdk/classes/entity.hpp"
#include "../../source2-sdk/classes/trace.hpp"
#include "../../source2-sdk/classes/types.hpp"
#include "../../source2-sdk/classes/user_cmd.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"
#include "../../source2-sdk/math/math.hpp"
#include "../../source2-sdk/classes/players.hpp"
#include "../../source2-sdk/sdk.hpp"
#include "../../utilities/utilities.hpp"
#include "../../utilities/renderer/w2r/w2r.hpp"
#include "../../hooks/hooks.hpp"
#include "prediction/prediction.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <mutex>
#include <vector>
#include <limits>
#include <numbers>
#include <memory>

#include <Windows.h>

namespace {
	constexpr float k_rad_to_deg{ 180.0f / std::numbers::pi_v<float> };
	constexpr float k_scan_step{ 1.f };
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

	struct wall_alignment
	{
		float parallel_yaw{};
		bool surface_side{};
	};

	wall_alignment wall_parallel_yaw(const vec3_t& normal, float current_yaw)
	{
		const float normal_yaw = std::atan2f(normal.y, normal.x) * k_rad_to_deg;
		const float radians = std::numbers::pi_v<float> / 180.f;
		const bool surface_side = std::sinf(normal_yaw * radians) * std::cosf(current_yaw * radians)
			- std::cosf(normal_yaw * radians) * std::sinf(current_yaw * radians) > 0.f;
		return { normalize_yaw(normal_yaw + (surface_side ? -90.f : 90.f)), surface_side };
	}

	float turn_angle_for_speed(float speed, bool slipping)
	{
		if (speed < 60.0f) return k_min_turn_angle;
		const float ratio = (slipping ? 30.006001f : 29.933001f) / speed;
		return std::asinf((std::clamp)(ratio, -1.0f, 1.0f)) * k_rad_to_deg;
	}

	bool get_player_hull(player_t* local_player, vec3_t* mins, vec3_t* maxs)
	{
		if (local_player->try_collision_bounds(*mins, *maxs))
		{
			const bool sane = maxs->z > mins->z && maxs->z <= 128.f && mins->z >= -128.f && std::fabsf(maxs->x) <= 128.f && std::fabsf(maxs->y) <= 128.f;

			if (sane) return true;
		}
		*mins = k_default_hull_mins;
		*maxs = k_default_hull_maxs;
		return true;
	}

	void pixelsurf_debug(const char* reason) {
		static const char* logged[32] = { };
		static std::uint32_t count = 0;

		for (std::uint32_t i = 0; i < count; i++)  if (logged[i] == reason) return;
		if (count < 32) logged[count++] = reason;
		console::log("[-] pixelsurf: %s\n", reason);
	}
}


static bool jumpbug_active = false;
static bool edgebug_locked();

void movement::run_bhop(user_cmd_t* cmd)
{
	if (!config::context.bhop || !cmd) return;
	if (jumpbug_active || edgebug_locked()) return;
	if (!((cmd->buttons | cmd->buttons3) & in_jump)) return;
	auto local = sdk::local_player;
	if (!local || !local->is_alive() || !interfaces::cvar) return;
	auto auto_bhop = interfaces::cvar->FindVar("sv_autobunnyhopping");
	if (auto_bhop && auto_bhop->get_bool()) return;
	const auto type = local->actual_move_type();
	if (type == movetype_ladder || type == movetype_noclip || type == movetype_observer) return;
	if (local->flags() & fl_onground) return;
	auto base = cmd->get_base();
	if (!base) return;
	cmd->buttons &= ~in_jump;
	cmd->buttons2 &= ~in_jump;
	cmd->buttons3 &= ~in_jump;
	for (int i = 0; i < cmd->subtick_count(); i++) {
		auto step = cmd->get_subtick(i);
		if (!step || !(step->has_bits & 0x1u) || !(step->button & in_jump)) continue;
		step->button &= ~in_jump;
		if (!step->button) step->has_bits &= ~0x3u;
	}
	cmd->sync_buttons();
}

void movement::pixelsurf(user_cmd_t* cmd)
{
	if (!config::context.pixelsurf) return;
	if (jumpbug_active) return;
	if (edgebug_locked()) return;
	if (!cmd)
	{
		pixelsurf_debug("cmd missing");
		return;
	}

	player_t* local_player = sdk::local_player;

	if (!local_player)
	{
		pixelsurf_debug("local player missing");
		return;
	}

	if (!local_player->is_alive()) return;
	if (local_player->flags() & fl_onground) return;

	const auto move_type = local_player->actual_move_type();

	if (move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer) return;
	if (!interfaces::trace)
	{
		pixelsurf_debug("trace interface missing");
		return;
	}
	auto services = local_player->movement_services();
	if (!services || !i_trace::trace_hull_target())
	{
		pixelsurf_debug("Classico hull target or movement services missing");
		return;
	}

	cbase_user_cmd_pb_t* base = cmd->get_base();
	cmd_qangle_t* view = cmd->get_view_angles();

	if (!base)
	{
		pixelsurf_debug("cmd base missing");
		return;
	}

	if (!view)
	{
		pixelsurf_debug("view angles missing");
		return;
	}

	vec3_t origin{};
	if (!local_player->try_abs_origin(origin))
	{
		pixelsurf_debug("origin missing");
		return;
	}
	const vec3_t velocity = local_player->velocity();
	const float pitch = view->angles.x;
	const float current_yaw = normalize_yaw(view->angles.y);
	const float speed = vel2d(velocity);
	const bool slipping = velocity.z >= k_air_slip_min_z && velocity.z <= k_air_slip_max_z;
	const bool velocity_reversed = speed >= k_min_speed && std::fabsf(normalize_yaw(std::atan2f(velocity.y, velocity.x) * k_rad_to_deg - current_yaw)) > 90.0f;
	const float turn_angle = turn_angle_for_speed(speed, slipping);

	vec3_t mins = k_default_hull_mins;
	vec3_t maxs = k_default_hull_maxs;
	if (!get_player_hull(local_player, &mins, &maxs))
	{
		pixelsurf_debug("hull missing");
		return;
	}

	trace_filter_t filter = {};
	if (!i_trace::make_player_movement_filter(&filter, local_player))
	{
		pixelsurf_debug("movement filter failed");
		return;
	}

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
		const float distance = std::sqrtf(delta_x * delta_x + delta_y * delta_y) - 24.f;

		if (distance >= candidate->distance) return;

		candidate->target_yaw = target_yaw;
		candidate->distance = distance;
		candidate->move_right = alignment.surface_side ^ slipping;
		candidate->found = true;
	};

	const auto scan_surfaces = [&](const vec3_t& scan_origin, surface_candidate* candidate)
	{
		*candidate = surface_candidate();

		for (const vec3_t& direction : directions)
		{
			const vec3_t trace_end(scan_origin.x + direction.x * k_scan_step, scan_origin.y + direction.y * k_scan_step, scan_origin.z);
			game_trace_t trace = {};

			if (!interfaces::trace->trace_hull_player(scan_origin, trace_end, mins, maxs, &filter, services, &trace)) continue;
			consider_surface(scan_origin, trace, candidate);
		}
	};

	surface_candidate best = {};
	scan_surfaces(origin, &best);

	if (!best.found)
	{
		trace_filter_t ground_filter = {};
		const vec3_t ground_end(origin.x, origin.y, origin.z - k_ground_trace_length);
		game_trace_t ground_trace = {};

		if (!i_trace::make_filter(&ground_filter, local_player, 0x1c3003, 4))
		{
			pixelsurf_debug("ground filter failed");
			return;
		}
		if (!interfaces::trace->trace_line(origin, ground_end, &ground_filter, &ground_trace))
		{
			pixelsurf_debug("ground trace failed");
			return;
		}
		if (ground_trace.fraction >= 1.0f || ground_trace.start_solid)
		{
			pixelsurf_debug("ground trace empty");
			return;
		}

		const float ground_distance = origin.z - ground_trace.endpos.z;

		scan_surfaces(vec3_t(origin.x - k_retry_offset, origin.y - k_retry_offset, origin.z - k_retry_offset), &best);

		if (!best.found)
			scan_surfaces(vec3_t(origin.x, origin.y, origin.z - ground_distance + k_ground_offset), &best);
	}

	if (!best.found)
	{
		pixelsurf_debug("surface not found");
		return;
	}

	const float side = best.move_right ? -1.f : 1.f;
	cmd->set_view_angles(vec3_t(pitch, best.target_yaw, 0.f));
	cmd->set_move(0.f, side);
	const auto previous_buttons = services->button_down_mask_prev();
	cmd->buttons &= ~k_movement_buttons;
	cmd->buttons |= best.move_right ? in_moveright : in_moveleft;
	cmd->buttons2 = (cmd->buttons2 & ~k_movement_buttons) | ((previous_buttons ^ cmd->buttons) & k_movement_buttons);
	cmd->buttons3 &= ~k_movement_buttons;
	cmd->sync_buttons();
	cmd->update_move_crc();

	if (!config::context.pixelsurf_silent && interfaces::csgo_input)
	{
		vec3_t live_angles(pitch, best.target_yaw, 0.f);
		interfaces::csgo_input->set_view_angles(live_angles);
	}
}

//works pgood

void movement::ej(user_cmd_t* cmd) {
	auto local = sdk::local_player;
	auto controller = sdk::local_controller;

	if (!cmd) return;
	if (!local) return;
	if (!controller) return;
	if (!local->is_alive()) return;
	const float velocity2d = vel2d(local->velocity());
	const movetype_t move_type = local->move_type();

	if (move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer) return;
	if (velocity2d < 5.0f) return;

	bool prev_grounded = local->flags() & fl_onground;
	bool will_fall = false;

	const bool simulated = prediction::simulate_ticks_until(cmd, 2, [&](std::size_t) {
		const bool grounded = local->flags() & fl_onground;
		will_fall = !grounded && prev_grounded;
		prev_grounded = grounded;
		return will_fall;
	});

	if (!simulated || !will_fall) return;
	cmd->buttons |= in_jump;
	cmd->buttons2 |= in_jump;
	cmd->sync_buttons();
}

void movement::ladder_ej(user_cmd_t* cmd) {
	static player_t* last_player = nullptr;
	static int saved_tick = -1;
	auto local = sdk::local_player;
	if (last_player != local) { saved_tick = -1; last_player = local; }
	if (!cmd || !config::context.ladder_ej || !local || !local->is_alive() || !interfaces::globals) {
		saved_tick = -1;
		return;
	}
	if (!cmd->get_base() || edgebug_locked()) return;
	const int tick = interfaces::globals->tick_count;
	const auto type = local->actual_move_type();
	if (type == movetype_noclip || type == movetype_observer || (local->flags() & fl_onground) || tick < saved_tick) saved_tick = -1;
	bool leaving = false;
	if (type == movetype_ladder) {
		saved_tick = -1;
		prediction::simulate_ticks_until(cmd, 1, [&](std::size_t) {
			const auto predicted_type = local->actual_move_type();
			leaving = predicted_type != movetype_ladder && predicted_type != movetype_noclip && predicted_type != movetype_observer;
			return true;
		});
		if (leaving) saved_tick = tick;
	}
	if (saved_tick < 0) return;
	const int elapsed = tick - saved_tick;
	if (elapsed >= 15) { saved_tick = -1; return; }
	if (!leaving && elapsed <= 1) return;
	const auto previous = cmd->buttons;
	cmd->set_move(0.f, 0.f);
	cmd->buttons &= ~k_movement_buttons;
	cmd->buttons |= leaving ? in_jump : in_duck;
	const auto changed = previous ^ cmd->buttons;
	cmd->buttons2 |= changed;
	cmd->buttons3 &= ~(k_movement_buttons | (leaving ? in_jump : in_duck));
	cmd->sync_buttons();
}
// TODO: add lock + predict
void movement::ladder_bug(user_cmd_t* cmd) {
	static player_t* last_player = nullptr;
	static bool notified = false;
	auto local = sdk::local_player;
	if (last_player != local)
	{
		last_player = local;
		notified = false;
	}
	if (!cmd || !config::context.ladder_bug || !local || !local->is_alive())
	{
		notified = false;
		return;
	}
	if (local->actual_move_type() != movetype_ladder)
	{
		notified = false;
		return;
	}
	auto base = cmd->get_base();
	if (!base) return;
	
	auto services = local->movement_services();
	const std::uint64_t previous_buttons = services ? services->button_down_mask_prev() : cmd->buttons;

	base->has_bits |= cbase_user_cmd_pb_t::has_upmove;
	base->up_move = 1.f;
	
	cmd->buttons |= in_jump;
	cmd->buttons2 = (cmd->buttons2 & ~in_jump) | ((previous_buttons ^ cmd->buttons) & in_jump);
	cmd->buttons3 &= ~in_jump;
	cmd->sync_buttons();
	if (!notified)
	{
		notified = true;
		utilities::chat_print("ladderbugged");
	}
	return;
}

void movement::long_jump(user_cmd_t* cmd) {
	auto local = sdk::local_player;
	auto base = cmd->get_base();

};
void movement::mini_jump(user_cmd_t* cmd) {};

struct edgebug_data_t
{
	bool contact_notified = false;
	bool edgebugging = false;
	bool edgebugged = false;
	bool ducking = false;

	int target_tick = 0;
	int target_command = 0;
	std::chrono::steady_clock::time_point lock_until{};
	int last_tick = 0;
	int last_search_command = 0;
	player_t* player = nullptr;
}; static edgebug_data_t edgebug_data{};

struct edgebug_visual_t
{
	player_t* player = nullptr;
	std::vector<vec3_t> path;
	vec3_t position{};
	std::chrono::steady_clock::time_point path_until{};
}; static edgebug_visual_t edgebug_visual{};
   static std::mutex edgebug_mutex;

static bool edgebug_locked()
{
	std::lock_guard<std::mutex> guard(edgebug_mutex);
	return config::context.edgebug && config::context.eb_lock && edgebug_data.edgebugging;
}

struct edgebug_result_t
{
    bool found = false;
    bool ducking = false;
    std::size_t contact_tick = 0;
    std::vector<vec3_t> path;
};
static auto detect_eb = [](user_cmd_t* cmd, const std::function<void(bool)>& prepare_simulation) -> edgebug_result_t {
    edgebug_result_t result{};
    auto local = sdk::local_player;
	auto base = cmd->get_base();
	if (!local || !base) return result;
	const auto ticks = static_cast<std::size_t>((std::clamp)(config::context.edgebug_ticks, 1, static_cast<int>(prediction::max_simulation_ticks)));
	for (int mode = 0; mode < 2; mode++) {
		const bool ducking = mode == 0;
		float previous_z = local->velocity().z;
		vec3_t previous_origin{};
		if (!local->try_abs_origin(previous_origin)) return result;
		bool found = false;
		std::size_t contact_tick = 0;
		std::vector<vec3_t> path;
		path.reserve(ticks + 1);
		path.push_back(previous_origin);
		const bool simulated = prediction::simulate_ticks_until(cmd, ticks, [&](std::size_t index) {
				vec3_t origin{};
				if (!local->try_abs_origin(origin)) return true;
				path.push_back(origin);
				
				const float new_z = local->velocity().z;
				const bool grounded = local->flags() & fl_onground;

				found = previous_z < -7.f && std::floor(new_z) == -7.f && !grounded;
				if (found) contact_tick = index + 1; 
				previous_z = new_z; 

				const auto new_type = local->actual_move_type();
				return found || grounded || new_type == movetype_ladder || new_type == movetype_noclip || new_type == movetype_observer;
			}, nullptr, [&](std::size_t) { 
				prepare_simulation(ducking);
			});
			if (!simulated || !found) continue; 

        result.found = true;
        result.ducking = ducking;
        result.contact_tick = contact_tick;
        if (path.size() > contact_tick + 1) path.resize(contact_tick + 1);
        result.path = std::move(path);
        return result;
    }
    return result;
};

void movement::edge_bug(user_cmd_t* cmd) {
	std::lock_guard<std::mutex> guard(edgebug_mutex);
	auto local = sdk::local_player;
	auto globals = interfaces::globals;

	if (!cmd || !config::context.edgebug || !local || !local->is_alive() || !globals || !interfaces::engine || !interfaces::engine->is_in_game()) {
		edgebug_data = {};
		return;
	}

	auto base = cmd->get_base();
	if (!base) { edgebug_data = {}; return; }

		auto apply_input = [&](bool ducking) { 
		cmd->set_move(0.f, 0.f);
		base->has_bits |= cbase_user_cmd_pb_t::has_mousedx | cbase_user_cmd_pb_t::has_mousedy;
		base->mousedx = 0;
		base->mousedy = 0;

		auto services = local->movement_services();
		const std::uint64_t previous = services ? services->button_down_mask_prev() : cmd->buttons; // Берём прошлую маску кнопок; без services используем текущую как fallback.
		const std::uint64_t mask = in_duck | k_movement_buttons;
		cmd->buttons &= ~k_movement_buttons;
		cmd->buttons &= ~in_duck;
		if (ducking) cmd->buttons |= in_duck; // В duck-варианте включаем приседание; в stand оставляем выключенным.
		cmd->buttons2 = (cmd->buttons2 & ~mask) | ((previous ^ cmd->buttons) & mask); // Обновляем changed-маску только для кнопок, которыми управляет EB.
		cmd->buttons3 &= ~mask; // Удаляем scroll-события управляемых кнопок.
		cmd->sync_buttons(); // Синхронизируем три маски кнопок с protobuf команды.

		for (int i = 0; i < cmd->subtick_count(); i++) {
			auto step = cmd->get_subtick(i);
			if (!step) continue;
			if (step->has_bits & 0x8u) step->set_analog_forward_delta(0.f); // Убираем уже заданное изменение forwardmove внутри тика.
			if (step->has_bits & 0x10u) step->set_analog_left_delta(0.f); // Убираем уже заданное изменение sidemove внутри тика.
			if (step->has_bits & 0x20u) step->set_pitch_delta(0.f); // Обнуляем существующую сабтиковую поправку pitch.
			if (step->has_bits & 0x40u) step->set_yaw_delta(0.f); // Обнуляем существующую сабтиковую поправку yaw.
			if ((step->has_bits & 0x1u) && (step->button & mask)) // Меняем pressed только у событий управляемых кнопок.
			step->set_pressed(ducking && (step->button & in_duck)); // Оставляем нажатым только duck в duck-варианте; остальные события отпускаем.
		}
	};

	const int tick = globals->tick_count;
	if (edgebug_data.player != local || tick < edgebug_data.last_tick) edgebug_data = {};

	edgebug_data.player = local;
	edgebug_data.last_tick = tick;
	const auto type = local->actual_move_type();
	if ((local->flags() & fl_onground) || type == movetype_ladder || type == movetype_noclip || type == movetype_observer) {
		edgebug_data = {};
		return;
	}


	const float actual_z = std::floor(local->velocity().z);
	if (actual_z != -7.f && actual_z != -6.f) edgebug_data.contact_notified = false;
	if (edgebug_data.edgebugging) {
		const float z = std::floor(local->velocity().z);
		const bool caught = z == -7.f || z == -6.f;
		if (caught) {
			if (!edgebug_data.contact_notified) {
				edgebug_data.contact_notified = true;
				utilities::chat_print("edgebugged");
			}
			edgebug_data.edgebugging = false;
			edgebug_data.target_tick = 0;
			edgebug_data.edgebugged = true;
			edgebug_visual.path_until = std::chrono::steady_clock::now() + std::chrono::seconds(1);
		} else if (tick > edgebug_data.target_tick ||
			cmd->command_number() > edgebug_data.target_command ||
			std::chrono::steady_clock::now() >= edgebug_data.lock_until)
		{
			edgebug_data.edgebugging = false;
			edgebug_data.target_tick = 0;
			edgebug_data.edgebugged = std::floor(local->velocity().z) == -7.f || std::floor(local->velocity().z) == -6.f;
			return;
		} 
			if (!caught) {
			if (config::context.eb_lock) {
				apply_input(edgebug_data.ducking);
			}
			edgebug_visual.path_until = std::chrono::steady_clock::now() + std::chrono::seconds(1); // Продлеваем визуализацию активного lock.
			return;
		}
	}

	if (local->velocity().z >= 0.f) return;
	if (edgebug_data.last_search_command == cmd->command_number()) return; 
	edgebug_data.last_search_command = cmd->command_number();
	edgebug_data.edgebugged = false;

    auto result = ::detect_eb(cmd, apply_input);
    if (!result.found) return;
    edgebug_data.edgebugging = true;
    edgebug_data.ducking = result.ducking;
    edgebug_data.target_tick = tick + static_cast<int>(result.contact_tick);
    edgebug_data.target_command = cmd->command_number() + static_cast<int>(result.contact_tick);
    const float interval = globals->intervalpertick > 0.f ? globals->intervalpertick : 1.f / 64.f;
    const auto duration = std::chrono::duration<float>((std::clamp)((result.contact_tick + 2) * interval, 0.05f, 2.f));
    edgebug_data.lock_until = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(duration);
    edgebug_visual.player = local;
    edgebug_visual.path = std::move(result.path);
    edgebug_visual.path_until = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    if (!edgebug_visual.path.empty()) edgebug_visual.position = edgebug_visual.path.back();
    if (config::context.eb_lock) {
		apply_input(result.ducking);
	}
}

//delu pasted
void movement::visualize_eb()
{
	if (!config::context.visualize_edgebug) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;
	if (!interfaces::engine || !interfaces::engine->is_in_game()) return;
	std::vector<vec3_t> path;
	vec3_t position{};
	{
		std::lock_guard<std::mutex> guard(edgebug_mutex);
		if (edgebug_visual.player != sdk::local_player || edgebug_visual.path.empty()) return;
		if (std::chrono::steady_clock::now() >= edgebug_visual.path_until) return;
		path = edgebug_visual.path;
		position = edgebug_visual.position;
	}
	const auto& rgb = config::context.edgebug_path_color;
	const color_t color(static_cast<std::uint8_t>((std::clamp)(rgb[0], 0.f, 1.f) * 255.f), static_cast<std::uint8_t>((std::clamp)(rgb[1], 0.f, 1.f) * 255.f), static_cast<std::uint8_t>((std::clamp)(rgb[2], 0.f, 1.f) * 255.f), 255);
	int projected = w2r::polyline(path, color, 2.f);
	projected += config::context.edgebug_visual_shape == 1
		? w2r::circle(position, 16.f, color, 64, 2.f)
		: w2r::box(position, 16.f, color, 2.f);
	static bool projection_logged[2]{};
	const int status = projected > 0;
	if (!projection_logged[status])
	{
		projection_logged[status] = true;
	//	console::log(status ? "[?] EB visual: projected lines submitted\n" : "[?] EB visual: no segments in front of camera\n");
	}
}
landing_prediction predict_landing()
{
	landing_prediction result{};
	auto local = sdk::local_player;
	if (!local || !interfaces::trace || !interfaces::globals || !interfaces::cvar) return result;
	auto services = local->movement_services();
	if (!services) return result;
	auto gravity = interfaces::cvar->FindVar("sv_gravity");
	auto standable = interfaces::cvar->FindVar("sv_standable_normal");
	if (!gravity || !standable) return result;
	vec3_t origin{}, mins{}, maxs{};
	if (!local->try_abs_origin(origin) || !get_player_hull(local, &mins, &maxs)) return result;
	const float interval = interfaces::globals->intervalpertick;
	if (interval <= 0.f) return result;
	auto velocity = local->velocity();
	velocity.z -= gravity->get_float() * interval;
	const vec3_t end(origin.x + velocity.x * interval, origin.y + velocity.y * interval, origin.z + velocity.z * interval - 2.f);
	trace_filter_t filter{};
	if (!i_trace::make_movement_filter(&filter, local, k_trace_mask, COLLISION_GROUP_PLAYER_MOVEMENT)) return result;
	game_trace_t trace{};
	if (!interfaces::trace->trace_hull_player(origin, end, mins, maxs, &filter, services, &trace)) return result;
	result.normal = trace.normal;
	if (trace.start_solid || trace.fraction <= 0.f || trace.fraction >= 1.f || trace.normal.z <= standable->get_float()) return result;
	result.valid = true;
	result.fraction = std::round(trace.fraction * 64.f) / 64.f;
	return result;
}

void movement::jump_bug(user_cmd_t* cmd)
{
	jumpbug_active = false;
	if (!cmd || !config::context.jumpbug) return;
	static std::uint32_t debug_seen = 0;
	auto debug = [&](std::uint32_t bit, const char* reason)
	{
		if (debug_seen & bit) return;
		debug_seen |= bit;
		console::log("[?] jb: %s\n", reason);
	};
	auto local = sdk::local_player;
	if (!local || !local->is_alive()) return;
	const auto move_type = local->actual_move_type();
	if (move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer) return;
	if (local->flags() & fl_onground) return;
	if (local->velocity().z >= 0.f) return;
	const bool wants_jump = (cmd->buttons | cmd->buttons3) & in_jump;
	if (!wants_jump)
	{
		debug(1u, "waiting for jump input");
		return;
	}
	{
		std::lock_guard<std::mutex> guard(edgebug_mutex);
		if (config::context.edgebug && config::context.eb_lock && edgebug_data.edgebugging) return;
	}
	auto base = cmd->get_base();
	if (!base) return;
	bool landed = false;
	const bool simulated = prediction::simulate_ticks_until(cmd, 1, [&](std::size_t)
	{
		landed = local->flags() & fl_onground;
		return landed;
	});
	if (!simulated || !landed)
	{
		debug(8u, simulated ? "simulation did not confirm landing" : "simulation unavailable");
		return;
	}
	debug(16u, "landing confirmed");
	const auto landing = predict_landing();
	if (!landing.valid || landing.fraction <= 0.015625f)
	{
		debug(2u, "waiting for landing trace");
		return;
	}
	debug(4u, "landing trace found");
	auto services = local->movement_services();
	const std::uint64_t previous = services ? services->button_down_mask_prev() : cmd->buttons;
	constexpr std::uint64_t mask = in_duck | in_jump;
	if (landing.fraction >= 0.96875f)
	{
		cmd->buttons |= mask;
		cmd->buttons2 = (cmd->buttons2 & ~mask) | ((previous ^ cmd->buttons) & mask);
		cmd->buttons3 &= ~mask;
		cmd->sync_buttons();
		jumpbug_active = true;
		debug(32u, "late landing: duck and jump applied");
		return;
	}
	cmd->clear_subticks();
	subtick_move_step_t* steps[4]{};
	for (auto& step : steps)
	{
		step = cmd->acquire_subtick();
		if (!step)
		{
			base->subtick_moves.current_size = 0;
			debug(64u, "subtick allocation failed");
			return;
		}
		step->has_bits = 0;
		step->cached_size = 0;
	}
	for (int i = 0; i < 4; i++)
	{
		steps[i]->set_button(i < 2 ? in_duck : in_jump);
		steps[i]->set_pressed(i == 0 || i == 3);
		steps[i]->set_when(i == 0 ? 0.f : landing.fraction);
		steps[i]->set_analog_forward_delta(0.f);
		steps[i]->set_analog_left_delta(0.f);
	}
	cmd->buttons &= ~in_duck;
	cmd->buttons2 &= ~in_duck;
	cmd->sync_buttons();
	jumpbug_active = true;
	debug(128u, "duck/jump subticks applied");
}

#include "prediction.hpp"

#include "../../../source2-sdk/classes/entity.hpp"
#include "../../../source2-sdk/classes/global_vars.hpp"
#include "../../../source2-sdk/classes/players.hpp"
#include "../../../source2-sdk/classes/user_cmd.hpp"
#include "../../../source2-sdk/interfaces/i_netclient.hpp"
#include "../../../source2-sdk/interfaces/i_prediction.hpp"
#include "../../../source2-sdk/interfaces/interfaces.hpp"
#include "../../../source2-sdk/schema_system/schema_system.hpp"
#include "../../../source2-sdk/sdk.hpp"
#include "../../../utilities/utilities.hpp"

#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

namespace
{
	using interface_callback_fn = void*(__cdecl*)();

	typedef struct _interface_reg_t
	{
		interface_callback_fn callback;
		const char* name;
		_interface_reg_t* flink;
	} interface_reg_t;

	class state_guard
	{
	public:
		~state_guard()
		{
			restore();
		}

		template <typename T>
		void save(T& value)
		{
			save_raw(&value, sizeof(T));
		}

		void save_raw(void* address, std::size_t size)
		{
			if (!address || size == 0) return;

			entry& item = entries.emplace_back();
			item.address = reinterpret_cast<std::uintptr_t>(address);
			item.data.resize(size);
			std::memcpy(item.data.data(), address, size);
		}

		void save_raw(std::uintptr_t address, std::size_t size)
		{
			save_raw(reinterpret_cast<void*>(address), size);
		}

		void restore()
		{
			for (auto it = entries.rbegin(); it != entries.rend(); ++it) std::memcpy(reinterpret_cast<void*>(it->address), it->data.data(), it->data.size());
			entries.clear();
		}

	private:
		struct entry
		{
			std::uintptr_t address{};
			std::vector<std::uint8_t> data{};
		};

		std::vector<entry> entries{};
	};

	prediction::pred_data_t pred_data = {};

	struct prediction_context
	{
		user_cmd_t* cmd{};
		cbase_user_cmd_pb_t* base{};
		player_t* player{};
		controller_t* controller{};
		player_movement_services_t* movement{};
		game_scene_node_t* scene_node{};
		cprediction_t* prediction{};
		INetworkClient* network{};
		global_vars_t* globals{};
	};

	void* find_interface(const wchar_t* module_name, const char* interface_name)
	{
		if (!module_name || !interface_name) return nullptr;

		HMODULE module = GetModuleHandle(module_name);
		if (!module) return nullptr;

		std::uint8_t* create_interface = reinterpret_cast<std::uint8_t*>(GetProcAddress(module, "CreateInterface"));
		if (!create_interface) return nullptr;

		interface_reg_t* interface_list = *reinterpret_cast<interface_reg_t**>(utilities::resolve_rip(create_interface, 3, 7));
		if (!interface_list) return nullptr;

		for (interface_reg_t* it = interface_list; it; it = it->flink)
		{
			if (!std::strcmp(it->name, interface_name)) return it->callback();
		}

		return nullptr;
	}

	cprediction_t* get_prediction_interface()
	{
		static cprediction_t* prediction_interface = reinterpret_cast<cprediction_t*>(find_interface(L"client.dll", "Source2ClientPrediction001"));
		return prediction_interface;
	}

	INetworkClient* get_network_client()
	{
		static I_NetworkClientService* service = reinterpret_cast<I_NetworkClientService*>(find_interface(L"engine2.dll", "NetworkClientService_001"));
		if (!service) return nullptr;
		return service->get_network_client();
	}

	bool save_schema(state_guard& guard, void* base, const char* class_name, const char* property_name, std::size_t size)
	{
		if (!base) return false;

		const std::uint32_t offset = schema_system::get_schema(class_name, property_name);
		if (offset == 0) return false;

		guard.save_raw(reinterpret_cast<std::uint8_t*>(base) + offset, size);
		return true;
	}

	void save_globals(state_guard& guard, global_vars_t* globals)
	{
		if (!globals) return;

		std::uint8_t* base = reinterpret_cast<std::uint8_t*>(globals);
		guard.save_raw(base + 0x30, sizeof(float));
		guard.save_raw(base + 0x34, sizeof(float));
		guard.save_raw(base + 0x38, sizeof(float));
		guard.save_raw(base + 0x3C, sizeof(float));
		guard.save_raw(base + 0x40, sizeof(float));
		guard.save_raw(base + 0x44, sizeof(std::int32_t));
		guard.save_raw(base + 0x48, sizeof(std::int32_t));
		guard.save_raw(base + 0x50, sizeof(float));
		guard.save_raw(base + 0x58, sizeof(std::uint32_t));
	}

	void save_command(state_guard& guard, user_cmd_t* cmd)
	{
		if (!cmd) return;

		guard.save_raw(cmd, sizeof(user_cmd_t));

		cbase_user_cmd_pb_t* base = cmd->get_base();
		if (!base) return;

		guard.save_raw(base, sizeof(cbase_user_cmd_pb_t));

		if (base->buttons) guard.save_raw(base->buttons, sizeof(cin_button_state_pb_t));
		if (base->view_angles) guard.save_raw(base->view_angles, sizeof(cmd_qangle_t));

		protobuf_repeated_ptr_field_t& field = base->subtick_moves;
		protobuf_repeated_ptr_field_t::rep_t* rep = field.rep;
		if (!rep) return;

		const std::int32_t count = (std::min)(field.current_size, (std::min)(field.capacity, rep->allocated_size));
		for (std::int32_t i = 0; i < count; i++)
		{
			subtick_move_step_t* step = cmd->get_subtick(i);
			if (step) guard.save_raw(step, sizeof(subtick_move_step_t));
		}
	}

	void save_state(state_guard& guard, const prediction_context& ctx, bool command)
	{
		save_globals(guard, ctx.globals);

		if (ctx.prediction)
		{
			guard.save(ctx.prediction->in_prediction);
			guard.save(ctx.prediction->first_prediction);
		}

		if (ctx.network) guard.save(ctx.network->m_bShouldPredict);
		if (ctx.cmd)
		{
			guard.save_raw(reinterpret_cast<std::uint8_t*>(ctx.cmd) + 0x88, sizeof(bool));
			guard.save(ctx.cmd->subtick_state);
		}

		if (ctx.controller) guard.save(ctx.controller->tick_base());
		if (ctx.player)
		{
			guard.save(ctx.player->flags());
			save_schema(guard, ctx.player, "C_BaseEntity", "m_MoveType", sizeof(std::uint8_t));
			save_schema(guard, ctx.player, "C_BaseEntity", "m_nActualMoveType", sizeof(std::uint8_t));
			guard.save(ctx.player->velocity());
			guard.save(ctx.player->abs_velocity());
			guard.save(ctx.player->base_velocity());
			guard.save(ctx.player->ground_entity());
			guard.save(ctx.player->simulation_time());
			guard.save(ctx.player->view_offset());
			guard.save(ctx.player->old_origin());
			guard.save(ctx.player->eye_angles());
		}

		if (ctx.scene_node) guard.save(ctx.scene_node->abs_origin());

		if (ctx.movement)
		{
			guard.save(ctx.movement->button_down_mask_prev());
			guard.save(ctx.movement->queued_button_down_mask());
			guard.save(ctx.movement->queued_button_change_mask());
			guard.save(ctx.movement->toggle_button_down_mask());
			guard.save(ctx.movement->buttondoublepressed());
			save_schema(guard, ctx.movement, "CPlayer_MovementServices", "m_nLastCommandNumberProcessed", sizeof(std::uint32_t));
			save_schema(guard, ctx.movement, "CPlayer_MovementServices", "m_flCmdForwardMove", sizeof(float));
			save_schema(guard, ctx.movement, "CPlayer_MovementServices", "m_flCmdLeftMove", sizeof(float));
			save_schema(guard, ctx.movement, "CPlayer_MovementServices", "m_flCmdUpMove", sizeof(float));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_flStamina", sizeof(float));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_flAccumulatedJumpError", sizeof(float));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_bDucked", sizeof(bool));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_bDucking", sizeof(bool));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_bDesiresDuck", sizeof(bool));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_bDuckOverride", sizeof(bool));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_duckUntilOnGround", sizeof(bool));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_flDuckAmount", sizeof(float));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_flDuckSpeed", sizeof(float));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_flDuckRootOffset", sizeof(float));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_flDuckViewOffset", sizeof(float));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_flLastDuckTime", sizeof(float));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_bSpeedCropped", sizeof(bool));
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_vecLastPositionAtFullCrouchSpeed", sizeof(float) * 2);
			save_schema(guard, ctx.movement, "CCSPlayer_MovementServices", "m_ModernJump", 56);
			save_schema(guard, ctx.movement, "CPlayer_MovementServices_Humanoid", "m_flSurfaceFriction", sizeof(float));
			guard.save_raw(reinterpret_cast<std::uint8_t*>(ctx.movement) + 0x3F8, sizeof(std::uint32_t));
			guard.save_raw(reinterpret_cast<std::uint8_t*>(ctx.movement) + 0xD48, sizeof(std::uint64_t));
		}

		if (command) save_command(guard, ctx.cmd);
	}

	bool build_context(user_cmd_t* cmd, prediction_context& out)
	{
		if (!cmd) return false;

		out.cmd = cmd;
		out.base = cmd->get_base();
		out.player = sdk::local_player;
		out.controller = sdk::local_controller;
		out.prediction = get_prediction_interface();
		out.network = get_network_client();
		out.globals = interfaces::globals;

		if (!out.player || !out.controller || !out.player->is_alive()) return false;
		if (!out.prediction || !out.network || !out.globals) return false;

		out.movement = out.player->movement();
		out.scene_node = out.player->game_scene_node();

		return out.movement && out.scene_node;
	}

	void capture_prestate(const prediction_context& ctx)
	{
		prediction::pred_data_t& data = pred_data;
		std::uint8_t* globals = reinterpret_cast<std::uint8_t*>(ctx.globals);

		data.valid = true;
		data.tick_base = ctx.controller->tick_base();
		data.interval_per_subtick = *reinterpret_cast<float*>(globals + 0x34);
		data.current_time = *reinterpret_cast<float*>(globals + 0x30);
		data.current_time2 = *reinterpret_cast<float*>(globals + 0x38);
		data.tick_count = *reinterpret_cast<std::int32_t*>(globals + 0x48);
		data.frame_time = *reinterpret_cast<float*>(globals + 0x3C);
		data.frame_time2 = *reinterpret_cast<float*>(globals + 0x40);
		data.pre_prediction_flags = static_cast<std::int64_t>(ctx.player->flags());
		data.pre_prediction_landing = ctx.movement->modern_jump().m_flLastLandedFrac();
		data.pre_abs_origin = ctx.scene_node->abs_origin();
		data.networked_velocity = ctx.player->velocity();
		data.abs_velocity_backup = ctx.player->abs_velocity();
		data.velocity_backup = ctx.player->velocity();
		data.eye_pos = ctx.player->eye_position();
		data.in_prediction = ctx.prediction->in_prediction;
		data.first_prediction = ctx.prediction->first_prediction;
		data.has_been_predicted = ctx.cmd->has_been_predicted();
		data.should_predict = ctx.network->m_bShouldPredict;
	}

	void capture_poststate(const prediction_context& ctx)
	{
		prediction::pred_data_t& data = pred_data;
		data.post_prediction_flags = static_cast<std::int64_t>(ctx.player->flags());
		data.post_prediction_landing = ctx.movement->modern_jump().m_flLastLandedFrac();
	}

	void write_prediction_globals(const prediction_context& ctx, std::int32_t tick)
	{
		const float interval = ctx.globals->intervalpertick > 0.f ? ctx.globals->intervalpertick : (1.f / 64.f);
		const float time = static_cast<float>(tick) * interval;
		std::uint8_t* globals = reinterpret_cast<std::uint8_t*>(ctx.globals);

		*reinterpret_cast<float*>(globals + 0x30) = time;
		*reinterpret_cast<float*>(globals + 0x34) = interval;
		*reinterpret_cast<float*>(globals + 0x3C) = interval;
		*reinterpret_cast<float*>(globals + 0x40) = interval;
		*reinterpret_cast<std::int32_t*>(globals + 0x44) = tick;
		*reinterpret_cast<std::int32_t*>(globals + 0x48) = tick;
		*reinterpret_cast<float*>(globals + 0x50) = 0.f;
		*reinterpret_cast<std::uint32_t*>(globals + 0x58) = GetCurrentThreadId();
	}

	bool run_command(const prediction_context& ctx)
	{
		if (ctx.cmd->subtick_state == 3) ctx.cmd->subtick_state = 0;

		ctx.cmd->set_has_been_predicted(false);
		ctx.network->m_bShouldPredict = true;
		ctx.prediction->first_prediction = false;
		ctx.prediction->in_prediction = true;

		ctx.movement->set_prediction_command(ctx.cmd);
		ctx.movement->run_command(ctx.cmd);
		ctx.movement->reset_prediction_command();

		return true;
	}

	state_guard live_guard;
	std::mutex simulation_mutex;
	std::int32_t last_sequence_processed = 0;
	bool live_started = false;
}


void prediction::start(user_cmd_t* cmd)
{
	if (!cmd) return;
	if (cmd->command_number() == last_sequence_processed) return;

	prediction_context ctx{};
	if (!build_context(cmd, ctx)) return;

	live_guard.restore();
	live_started = false;

	save_state(live_guard, ctx, false);
	capture_prestate(ctx);

	if (!run_command(ctx))
	{
		live_guard.restore();
		return;
	}

	live_started = true;
}

void prediction::end(user_cmd_t* cmd)
{
	if (!cmd) return;
	if (!live_started) return;

	prediction_context ctx{};
	if (build_context(cmd, ctx)) capture_poststate(ctx);

	last_sequence_processed = cmd->command_number();
	live_guard.restore();
	live_started = false;
}

bool prediction::simulate(user_cmd_t* cmd, const std::function<void()>& fn)
{
	return simulate_ticks(cmd, 1, [&](std::size_t) { if (fn) fn(); });
}

bool prediction::simulate_ticks(user_cmd_t* cmd, std::size_t ticks, const std::function<void(std::size_t)>& fn)
{
	return simulate_ticks_until(cmd, ticks, [&](std::size_t tick)
	{
		if (fn) fn(tick);
		return false;
	});
}

bool prediction::simulate_ticks_until(user_cmd_t* cmd, std::size_t ticks, const std::function<bool(std::size_t)>& fn, std::size_t* stopped_tick, const std::function<void(std::size_t)>& prepare)
{
	if (!cmd || ticks == 0 || ticks > max_simulation_ticks) return false;
	if (!fn) return false;

	prediction_context ctx{};
	if (!build_context(cmd, ctx)) return false;
	if (!ctx.base) return false;

	std::lock_guard<std::mutex> lock(simulation_mutex);
	state_guard guard;

	save_state(guard, ctx, true);
	guard.save(ctx.movement->step_sound_time());
	if (interfaces::cvar)
	{
		auto landing_sound = interfaces::cvar->FindVar("sv_min_jump_landing_sound");
		auto footsteps = interfaces::cvar->FindVar("sv_footsteps");
		if (landing_sound)
		{
			guard.save(landing_sound->value);
			landing_sound->set_float(1.0e9f);
		}
		if (footsteps)
		{
			guard.save(footsteps->value);
			footsteps->set_int(0);
		}
	}
	capture_prestate(ctx);

	const std::int64_t initial_command = cmd->command_number_value;
	const std::int32_t initial_legacy = ctx.base->legacy_command_number;
	const std::int32_t initial_client_tick = ctx.base->client_tick;
	const std::uint32_t initial_tick_base = ctx.controller->tick_base();
	const float forward_move = ctx.base->forward_move;
	const float side_move = ctx.base->side_move;
	const float up_move = ctx.base->up_move;

	bool result = true;

	for (std::size_t tick_index = 0; tick_index < ticks; tick_index++)
	{
		if (tick_index > 0)
		{
			cmd->command_number_value = initial_command + static_cast<std::int64_t>(tick_index);
			ctx.base->legacy_command_number = initial_legacy + static_cast<std::int32_t>(tick_index);
			ctx.base->client_tick = initial_client_tick + static_cast<std::int32_t>(tick_index);
			ctx.base->forward_move = forward_move;
			ctx.base->side_move = side_move;
			ctx.base->up_move = up_move;
		}

		const std::int32_t predicted_tick = static_cast<std::int32_t>(initial_tick_base + tick_index + 1);
		ctx.controller->tick_base() = predicted_tick;
		write_prediction_globals(ctx, predicted_tick);
		if (prepare) prepare(tick_index);
		ctx.movement->step_sound_time() = 0.f;

		if (!run_command(ctx))
		{
			result = false;
			break;
		}

		if (fn(tick_index))
		{
			if (stopped_tick) *stopped_tick = tick_index + 1;
			break;
		}
	}

	capture_poststate(ctx);
	guard.restore();
	return result;
}

const prediction::pred_data_t* prediction::data()
{
	return &pred_data;
}
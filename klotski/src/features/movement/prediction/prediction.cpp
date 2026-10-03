#include "prediction.hpp"

#include "../../../source2-sdk/classes/entity.hpp"
#include "../../../source2-sdk/classes/global_vars.hpp"
#include "../../../source2-sdk/classes/players.hpp"
#include "../../../source2-sdk/classes/user_cmd.hpp"
#include "../../../source2-sdk/interfaces/cprediction.hpp"
#include "../../../source2-sdk/interfaces/inetclient.hpp"
#include "../../../source2-sdk/interfaces/interfaces.hpp"
#include "../../../source2-sdk/sdk.hpp"
#include "../../../utilities/utilities.hpp"
#include "../../../utilities/vfunc.hpp"

#include <cstring>
#include <Windows.h>

 // temple pasted :D

namespace
{
	using interface_callback_fn = void*(__cdecl*)();

	typedef struct _interface_reg_t
	{
		interface_callback_fn callback;
		const char* name;
		_interface_reg_t* flink;
	} interface_reg_t;

	void* find_interface(const wchar_t* module_name, const char* interface_name)
	{
		HMODULE module = GetModuleHandle(module_name);

		if (!module) return nullptr;

		std::uint8_t* create_interface = reinterpret_cast<std::uint8_t*>(GetProcAddress(module, "CreateInterface"));

		if (!create_interface) return nullptr;

		interface_reg_t* interface_list = *reinterpret_cast<interface_reg_t**>(utilities::resolve_rip(create_interface, 3, 7));

		if (!interface_list) return nullptr;

		for (interface_reg_t* it = interface_list; it; it = it->flink)
		{
			if (!strcmp(it->name, interface_name)) return it->callback();
		}

		return nullptr;
	}

	cprediction_t* get_prediction_interface()
	{
		static cprediction_t* prediction = reinterpret_cast<cprediction_t*>(find_interface(L"client.dll", "Source2ClientPrediction001"));

		return prediction;
	}

	INetworkClient* get_network_client()
	{
		static I_NetworkClientService* service = reinterpret_cast<I_NetworkClientService*>(find_interface(L"engine2.dll", "NetworkClientService_001"));

		if (!service) return nullptr;

		return utilities::call_virtual<23U, INetworkClient*>(service);
	}

	prediction::pred_data_t pred_data = { };
	std::int32_t last_sequence_processed = 0;
}

void prediction::start(user_cmd_t* cmd)
{
	if (!utilities::is_valid_pointer(cmd)) return;

	if (cmd->command_number() == last_sequence_processed) return;

	cprediction_t* prediction_interface = get_prediction_interface();

	if (!utilities::is_valid_pointer(prediction_interface)) return;

	player_t* local_player = sdk::local_player;
	controller_t* local_controller = sdk::local_controller;

	if (!utilities::is_valid_pointer(local_player) || !utilities::is_valid_pointer(local_controller)) return;
	if (local_player->health() <= 0) return;

	player_movement_services_t* movement_services = local_player->movement_services();

	if (!utilities::is_valid_pointer(movement_services)) return;
	if (!utilities::is_valid_pointer(interfaces::globals)) return;

	INetworkClient* network_client = get_network_client();

	if (!utilities::is_valid_pointer(network_client)) return;

	std::uint8_t* globals = reinterpret_cast<std::uint8_t*>(interfaces::globals);

	pred_data.abs_velocity_backup = local_player->abs_velocity();
	pred_data.velocity_backup = local_player->velocity();
	pred_data.pre_abs_origin = pred_data.abs_velocity_backup;

	pred_data.interval_per_subtick = *reinterpret_cast<float*>(globals + 0x34);
	pred_data.current_time = *reinterpret_cast<float*>(globals + 0x30);
	pred_data.current_time2 = *reinterpret_cast<float*>(globals + 0x38);
	pred_data.tick_count = *reinterpret_cast<std::int32_t*>(globals + 0x48);
	pred_data.frame_time = *reinterpret_cast<float*>(globals + 0x3C);
	pred_data.frame_time2 = *reinterpret_cast<float*>(globals + 0x40);
	pred_data.pre_prediction_flags = static_cast<std::int64_t>(local_player->flags());
	pred_data.pre_prediction_landing = movement_services->modern_jump().m_flLastLandedFrac();

	pred_data.tick_base = local_controller->tick_base();
	pred_data.in_prediction = prediction_interface->in_prediction;
	pred_data.first_prediction = prediction_interface->first_prediction;
	pred_data.has_been_predicted = cmd->has_been_predicted();
	pred_data.should_predict = network_client->m_bShouldPredict;

	cmd->set_has_been_predicted(false);
	network_client->m_bShouldPredict = true;
	prediction_interface->first_prediction = false;
	prediction_interface->in_prediction = true;

	movement_services->set_prediction_command(cmd);
	movement_services->run_command(cmd);
	movement_services->reset_prediction_command();

	local_player->abs_velocity() = pred_data.abs_velocity_backup;
	local_player->velocity() = pred_data.velocity_backup;
}

void prediction::end(user_cmd_t* cmd)
{
	if (!utilities::is_valid_pointer(cmd)) return;

	if (cmd->command_number() == last_sequence_processed) return;

	cprediction_t* prediction_interface = get_prediction_interface();

	if (!utilities::is_valid_pointer(prediction_interface)) return;

	player_t* local_player = sdk::local_player;
	controller_t* local_controller = sdk::local_controller;

	if (!utilities::is_valid_pointer(local_player) || !utilities::is_valid_pointer(local_controller)) return;

	player_movement_services_t* movement_services = local_player->movement_services();

	if (!utilities::is_valid_pointer(movement_services)) return;
	if (!utilities::is_valid_pointer(interfaces::globals)) return;

	INetworkClient* network_client = get_network_client();

	if (!utilities::is_valid_pointer(network_client)) return;

	std::uint8_t* globals = reinterpret_cast<std::uint8_t*>(interfaces::globals);

	*reinterpret_cast<float*>(globals + 0x34) = pred_data.interval_per_subtick;
	*reinterpret_cast<float*>(globals + 0x30) = pred_data.current_time;
	*reinterpret_cast<float*>(globals + 0x38) = pred_data.current_time2;
	*reinterpret_cast<std::int32_t*>(globals + 0x48) = pred_data.tick_count;
	*reinterpret_cast<float*>(globals + 0x3C) = pred_data.frame_time;
	*reinterpret_cast<float*>(globals + 0x40) = pred_data.frame_time2;

	local_controller->tick_base() = pred_data.tick_base;

	prediction_interface->first_prediction = pred_data.first_prediction;
	prediction_interface->in_prediction = pred_data.in_prediction;
	cmd->set_has_been_predicted(pred_data.has_been_predicted);
	network_client->m_bShouldPredict = pred_data.should_predict;

	pred_data.post_prediction_flags = static_cast<std::int64_t>(local_player->flags());
	pred_data.post_prediction_landing = movement_services->modern_jump().m_flLastLandedFrac();

	last_sequence_processed = cmd->command_number();
}

bool prediction::sim_setup()
{
	cprediction_t* prediction_interface = get_prediction_interface();

	if (!utilities::is_valid_pointer(prediction_interface)) return false;

	player_t* local_player = sdk::local_player;
	controller_t* local_controller = sdk::local_controller;

	if (!utilities::is_valid_pointer(local_player) || !utilities::is_valid_pointer(local_controller)) return false;
	if (local_player->health() <= 0) return false;

	player_movement_services_t* movement_services = local_player->movement_services();

	if (!utilities::is_valid_pointer(movement_services)) return false;
	if (!utilities::is_valid_pointer(interfaces::globals)) return false;

	INetworkClient* network_client = get_network_client();

	if (!utilities::is_valid_pointer(network_client)) return false;

	std::uint8_t* globals = reinterpret_cast<std::uint8_t*>(interfaces::globals);

	pred_data.abs_velocity_backup = local_player->abs_velocity();
	pred_data.velocity_backup = local_player->velocity();
	pred_data.pre_abs_origin = local_player->game_scene_node()->abs_origin();

	pred_data.interval_per_subtick = *reinterpret_cast<float*>(globals + 0x34);
	pred_data.current_time = *reinterpret_cast<float*>(globals + 0x30);
	pred_data.current_time2 = *reinterpret_cast<float*>(globals + 0x38);
	pred_data.tick_count = *reinterpret_cast<std::int32_t*>(globals + 0x48);
	pred_data.frame_time = *reinterpret_cast<float*>(globals + 0x3C);
	pred_data.frame_time2 = *reinterpret_cast<float*>(globals + 0x40);
	pred_data.pre_prediction_flags = static_cast<std::int64_t>(local_player->flags());
	pred_data.pre_prediction_landing = movement_services->modern_jump().m_flLastLandedFrac();

	pred_data.tick_base = local_controller->tick_base();
	pred_data.in_prediction = prediction_interface->in_prediction;
	pred_data.first_prediction = prediction_interface->first_prediction;
	pred_data.should_predict = network_client->m_bShouldPredict;

	pred_data.sim_origin = local_player->game_scene_node()->abs_origin();
	pred_data.sim_old_origin = local_player->old_origin();
	pred_data.sim_velocity = local_player->velocity();
	pred_data.sim_abs_velocity = local_player->abs_velocity();
	pred_data.sim_base_velocity = local_player->base_velocity();
	pred_data.sim_flags = local_player->flags();
	pred_data.sim_tick_base = local_controller->tick_base();
	pred_data.sim_move_type = static_cast<std::int32_t>(local_player->move_type());
	pred_data.sim_move_collide = local_player->move_collide();
	pred_data.sim_ground_entity = local_player->ground_entity();
	pred_data.sim_simulation_time = local_player->simulation_time();

	if (collision_property_t* collision = local_player->collision_property(); utilities::is_valid_pointer(collision))
	{
		pred_data.sim_hull_mins = collision->mins();
		pred_data.sim_hull_maxs = collision->maxs();
		pred_data.sim_solid_flags = collision->solid_flags();
		pred_data.sim_solid_type = collision->solid_type();
		pred_data.sim_trigger_bloat = collision->trigger_bloat();
		pred_data.sim_surround_type = collision->surround_type();
		pred_data.sim_radius = collision->radius();
		pred_data.sim_specified_surrounding_mins = collision->specified_surrounding_mins();
		pred_data.sim_specified_surrounding_maxs = collision->specified_surrounding_maxs();
	}

	pred_data.sim_last_landed_frac = movement_services->modern_jump().m_flLastLandedFrac();
	pred_data.sim_button_down_mask_prev = movement_services->button_down_mask_prev();
	pred_data.sim_surface_friction = movement_services->surface_friction();
	pred_data.sim_max_speed = movement_services->max_speed();

	pred_data.sim_active = true;

	network_client->m_bShouldPredict = true;
	prediction_interface->first_prediction = false;
	prediction_interface->in_prediction = true;

	return true;
}

void prediction::sim_run(user_cmd_t* cmd) {
	if (!utilities::is_valid_pointer(cmd)) return;
	player_t* local_player = sdk::local_player;
	if (!utilities::is_valid_pointer(local_player)) return;
	player_movement_services_t* movement_services = local_player->movement_services();
	if (!utilities::is_valid_pointer(movement_services)) return;
	cmd->set_has_been_predicted(false);

	movement_services->set_prediction_command(cmd);
	movement_services->run_command(cmd);
	movement_services->reset_prediction_command();

	controller_t* local_controller = sdk::local_controller;

	if (utilities::is_valid_pointer(local_controller)) local_controller->tick_base()++;
}

void prediction::sim_restore()
{
	cprediction_t* prediction_interface = get_prediction_interface();
	if (!utilities::is_valid_pointer(prediction_interface)) return;
	player_t* local_player = sdk::local_player;
	controller_t* local_controller = sdk::local_controller;
	if (!utilities::is_valid_pointer(local_player) || !utilities::is_valid_pointer(local_controller)) return;
	player_movement_services_t* movement_services = local_player->movement_services();
	if (!utilities::is_valid_pointer(movement_services)) return;
	if (!utilities::is_valid_pointer(interfaces::globals)) return;
	std::uint8_t* globals = reinterpret_cast<std::uint8_t*>(interfaces::globals);

	*reinterpret_cast<float*>(globals + 0x34) = pred_data.interval_per_subtick;
	*reinterpret_cast<float*>(globals + 0x30) = pred_data.current_time;
	*reinterpret_cast<float*>(globals + 0x38) = pred_data.current_time2;
	*reinterpret_cast<std::int32_t*>(globals + 0x48) = pred_data.tick_count;
	*reinterpret_cast<float*>(globals + 0x3C) = pred_data.frame_time;
	*reinterpret_cast<float*>(globals + 0x40) = pred_data.frame_time2;

	local_controller->tick_base() = pred_data.tick_base;

	local_player->abs_velocity() = pred_data.abs_velocity_backup;
	local_player->velocity() = pred_data.velocity_backup;
	local_player->flags() = static_cast<std::uint32_t>(pred_data.pre_prediction_flags);
	movement_services->modern_jump().m_flLastLandedFrac() = pred_data.pre_prediction_landing;

	if (pred_data.sim_active)
	{
		local_player->old_origin() = pred_data.sim_old_origin;
		local_player->base_velocity() = pred_data.sim_base_velocity;
		local_player->move_type() = static_cast<movetype_t>(pred_data.sim_move_type);
		local_player->move_collide() = pred_data.sim_move_collide;
		local_player->ground_entity() = pred_data.sim_ground_entity;
		local_player->simulation_time() = pred_data.sim_simulation_time;

		if (collision_property_t* collision = local_player->collision_property(); utilities::is_valid_pointer(collision))
		{
			collision->mins() = pred_data.sim_hull_mins;
			collision->maxs() = pred_data.sim_hull_maxs;
			collision->solid_flags() = pred_data.sim_solid_flags;
			collision->solid_type() = pred_data.sim_solid_type;
			collision->trigger_bloat() = pred_data.sim_trigger_bloat;
			collision->surround_type() = pred_data.sim_surround_type;
			collision->radius() = pred_data.sim_radius;
			collision->specified_surrounding_mins() = pred_data.sim_specified_surrounding_mins;
			collision->specified_surrounding_maxs() = pred_data.sim_specified_surrounding_maxs;
		}

		movement_services->button_down_mask_prev() = pred_data.sim_button_down_mask_prev;
		movement_services->surface_friction() = pred_data.sim_surface_friction;
		movement_services->max_speed() = pred_data.sim_max_speed;

		pred_data.sim_active = false;
	}

	game_scene_node_t* scene_node = local_player->game_scene_node();
	if (utilities::is_valid_pointer(scene_node)) scene_node->abs_origin() = pred_data.pre_abs_origin;
	prediction_interface->first_prediction = pred_data.first_prediction;
	prediction_interface->in_prediction = pred_data.in_prediction;

	INetworkClient* network_client = get_network_client();

	if (utilities::is_valid_pointer(network_client)) network_client->m_bShouldPredict = pred_data.should_predict;
}

const prediction::pred_data_t* prediction::data() {return &pred_data;}


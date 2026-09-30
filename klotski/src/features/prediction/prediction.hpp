#pragma once

#include <cstdint>

#include "../../source2-sdk/math/math.hpp"
#include "../../source2-sdk/classes/types.hpp"

class user_cmd_t;

namespace prediction
{
	struct pred_data_t
	{
		std::uint32_t tick_base{};
		float frame_time{};
		float frame_time2{};
		float interval_per_subtick{};
		float current_time{};
		float current_time2{};
		std::int32_t tick_count{};

		float player_tick_fraction{};

		bool in_prediction{};
		bool first_prediction{};
		bool has_been_predicted{};
		bool should_predict{};

		std::int64_t pre_prediction_flags{};
		std::int64_t post_prediction_flags{};

		float pre_prediction_landing{};
		float post_prediction_landing{};

		float spread{};
		float inaccuracy{};

		vec3_t pre_abs_origin{};
		vec3_t abs_velocity_backup{};
		vec3_t velocity_backup{};
		vec3_t eye_pos{};

		vec3_t sim_origin{};
		vec3_t sim_old_origin{};
		vec3_t sim_velocity{};
		vec3_t sim_abs_velocity{};
		vec3_t sim_base_velocity{};
		std::uint32_t sim_flags{};
		std::uint32_t sim_tick_base{};
		std::int32_t sim_move_type{};
		std::uint8_t sim_move_collide{};
		entity_handle_t sim_ground_entity{};
		float sim_simulation_time{};

		vec3_t sim_hull_mins{};
		vec3_t sim_hull_maxs{};
		std::uint16_t sim_solid_flags{};
		std::uint8_t sim_solid_type{};
		std::uint8_t sim_trigger_bloat{};
		std::uint8_t sim_surround_type{};
		float sim_radius{};
		vec3_t sim_specified_surrounding_mins{};
		vec3_t sim_specified_surrounding_maxs{};

		float sim_last_landed_frac{};
		std::uint64_t sim_button_down_mask_prev{};
		float sim_surface_friction{};
		float sim_max_speed{};

		bool sim_active{};
	};

	void start(user_cmd_t* cmd);
	void end(user_cmd_t* cmd);

	bool sim_setup();
	void sim_run(user_cmd_t* cmd);
	void sim_restore();

	const pred_data_t* data();
}
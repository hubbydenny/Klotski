#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "../../../source2-sdk/math/math.hpp"
#include "../../../source2-sdk/classes/types.hpp"

class user_cmd_t;

namespace prediction
{
	constexpr std::size_t max_simulation_ticks = 64;

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
		bool valid{};

		std::int64_t pre_prediction_flags{};
		std::int64_t post_prediction_flags{};

		float pre_prediction_landing{};
		float post_prediction_landing{};

		float spread{};
		float inaccuracy{};

		vec3_t pre_abs_origin{};
		vec3_t networked_velocity{};
		vec3_t abs_velocity_backup{};
		vec3_t velocity_backup{};
		vec3_t eye_pos{};
	};

	void start(user_cmd_t* cmd);
	void end(user_cmd_t* cmd);
	bool simulate(user_cmd_t* cmd, const std::function<void()>& fn);
	bool simulate_ticks(user_cmd_t* cmd, std::size_t ticks, const std::function<void(std::size_t)>& fn);
	bool simulate_ticks_until(user_cmd_t* cmd, std::size_t ticks, const std::function<bool(std::size_t)>& fn, std::size_t* stopped_tick = nullptr, const std::function<void(std::size_t)>& prepare = {});
	const pred_data_t* data();
}

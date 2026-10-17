#pragma once
#include "../../source2-sdk/math/math.hpp"

class user_cmd_t;

namespace movement
{
	void run_bhop(user_cmd_t* cmd);
	void pixelsurf(user_cmd_t* cmd);
	void ej(user_cmd_t* cmd);
	void mini_jump(user_cmd_t* cmd);
	void ladder_bug(user_cmd_t* cmd);
	void edge_bug(user_cmd_t* cmd);
	void visualize_eb();
	void ladder_ej(user_cmd_t* cmd);
	void jump_bug(user_cmd_t* cmd);
	void long_jump(user_cmd_t* cmd);
	void detect_eb(user_cmd_t* cmd);
}
struct landing_prediction
{
	bool valid{ false };
	float fraction{ -1.0f };
	vec3_t normal{};
};
[[nodiscard]] landing_prediction predict_landing();


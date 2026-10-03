#pragma once

class controller_t;
class vec3_t;

namespace combat
{
	void run_aimbot(controller_t* local_controller);
	void run_rcs();
	void update_standalone_rcs(const vec3_t& view_angles, const vec3_t& aim_punch, int amount, int rand_min, int rand_max, bool apply);
}

#include "../../utilities/utilities.hpp"
#include "../classes/trace.hpp"
#include "../math/math.hpp"
#include "../../signatures.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"

static bool is_visible(player_t* local_player, player_t* target, const vec3_t& target_position)
{
	trace_filter_t filter = { };

	if (!i_trace::make_movement_filter(&filter, local_player, MASK_SHOT_HULL | CONTENTS_HITBOX, COLLISION_GROUP_PLAYER_MOVEMENT)) return false;

	vec3_t start = local_player->get_eye_position();
	vec3_t end = target_position;
	ray_t ray = { };
	game_trace_t trace = { };

	interfaces::trace->trace_shape(&ray, start, end, &filter, &trace);

	return trace.fraction == 1.f || trace.player == target;
}

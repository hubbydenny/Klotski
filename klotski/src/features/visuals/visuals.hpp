#include <cstdint>
#pragma once

namespace visuals
{
	void initialize_hitmarker_events();
	void shutdown_hitmarker_events();
	void run_player_esp();
	void run_glow();
	bool glow_override(void* glow_property, float* color);
	bool should_chams(std::uintptr_t scene_object);
	bool apply_chams(std::uintptr_t primitive_buffer, int first, int material_id, bool ignore_z, const float* color);
}


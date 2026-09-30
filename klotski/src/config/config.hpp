#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "../source2-sdk/classes/types.hpp"

namespace config
{
	constexpr std::uint32_t version = 2;

	typedef struct _config_t
	{
		bool player_esp = true;
		bool teammate_esp = false;
		bool draw_box = false;
		bool draw_name = false;
		bool draw_health = false;
		bool draw_armor = false;
		bool draw_weapon = false;
		bool draw_skeleton = false;
		bool draw_flags = false;
		bool esp_flags[esp_flag_count] = { true };
		bool glow = false;
		float glow_color[3] = { 1.f, 0.25f, 0.25f };
		float glow_color_invincible[3] = { 0.f, 1.f, 1.f };
		bool glow_invincible = true;
		float glow_alpha = 1.f;
		float glow_brightness = 1.f;

		bool watermark = false;
		float watermark_x = 12.f;
		float watermark_y = 12.f;
		bool velocity = false;
		float velo_x = 500.f;
		float velo_y = 100.f;
		float velo_size = 16.f;

		bool aimbot = false;
		float aimbot_fov = 10.f;
		bool onlyvisible = false;
		bool aim_bones[aim_bone_count] = { true };
		bool rcs = false;

		bool bhop = false;
		bool pixelsurf = false;
		bool pixelsurf_silent = false;
		bool edgebug = false;
		bool edgebug_key_state = false;
		int edgebug_ticks = 16;
		bool edgebug_chat = true;
		bool edgebug_visual = false;
		bool edgebug_debug = false;

		bool fov_changer = false;
		bool scopefov = false;
		float fov = 90.f;

	} config_t;

	extern config_t context;

	std::string get_directory();
	std::vector<std::string> list_files();
	bool save(const char* name);
	bool load(const char* name);
	bool remove(const char* name);
	bool* find_bool(const char* name);
}

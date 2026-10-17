#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "../../source2-sdk/math/math.hpp"
#include "../../source2-sdk/classes/types.hpp"
#include "../../utilities/imgui/imgui.h"

namespace config
{
	constexpr std::uint32_t version = 3;


	typedef struct _config_t
	{
		bool player_esp = true;
		bool teammate_esp = false;
		bool draw_box = false;
		bool draw_name = false;
		bool draw_health = false;
		bool health_smooth = true;
		bool health_gradient = true;
		bool health_custom_color = false;
		bool health_text = true;
		bool health_suffix = false;
		int health_text_position = 1;
		bool ammo_smooth = true;
		bool draw_weapon = false;
		bool draw_skeleton = false;
		bool draw_flags = false;
		bool is_flat = false;

		bool box_corner = false;
		float box_corner_size = 25.f;
		float box_thickness = 1.f;
		bool box_outline = true;
		bool box_fill = false;
		float box_fill_alpha = 0.15f;
		float esp_max_distance = 0.f;
		bool esp_distance_fade = false;
		float esp_opacity = 1.f;
		bool out_of_fov_arrows = false;
		float out_of_fov_arrow_radius = 240.f;
		float out_of_fov_arrow_size = 12.f;
		float out_of_fov_arrow_width = 0.4f;
		float out_of_fov_arrow_opacity = 1.f;
		bool out_of_fov_arrow_outline = false;
		bool out_of_fov_arrow_pulse = false;
		bool draw_ammo_bar = false;
		bool hitmarker = false;
		float hitmarker_duration = 0.35f;
		float hitmarker_size = 9.f;
		float hitmarker_gap = 5.f;
		float hitmarker_thickness = 1.5f;
		int esp_flags_position = 1;
		float esp_colors[3] = { 1.f, 0.3f, 0.3f };
		float esp_colors_teammate[3] = { 0.3f, 1.f, 0.3f };
		float esp_box_color[3] = { 1.f, 1.f, 1.f };
		float esp_name_color[3] = { 1.f, 1.f, 1.f };
		float esp_health_color[3] = { 0.3f, 1.f, 0.3f };
		float esp_weapon_color[3] = { 1.f, 1.f, 1.f };
		float esp_ammo_color[3] = { 0.4f, 0.75f, 1.f };
		float esp_skeleton_color[3] = { 1.f, 1.f, 1.f };
		float esp_flags_color[3] = { 1.f, 1.f, 1.f };
		float out_of_fov_arrow_color[3] = { 1.f, 1.f, 1.f };
		float hitmarker_color[3] = { 1.f, 1.f, 1.f };
		bool esp_color_by_health = false;
		bool esp_color_by_distance = false;
		bool esp_flags[esp_flag_count] = { true };
		bool chams = false;
		bool chams_visible = true;
		bool chams_invisible = false;
		int chams_material = 0;
		int chams_invisible_material = 0;
		float chams_visible_color[3] = { 1.f, 0.25f, 0.25f };
		float chams_invisible_color[3] = { 0.25f, 0.45f, 1.f };
		bool glow = false;
		float glow_color[3] = { 1.f, 0.25f, 0.25f };
		float glow_color_invincible[3] = { 0.f, 1.f, 1.f };
		bool glow_invincible = true;
		float glow_alpha = 1.f;
		float glow_brightness = 1.f;

		bool watermark = true;
		float watermark_x = 12.f;
		float watermark_y = 12.f;
		bool velocity = false;
		bool velo_pre_speed = true;
		bool velo_fade = false;
		bool velo_gradient = false;
		float velo_colors[5][3] = { {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 199.f / 255.f, 89.f / 255.f}, {1.f, 119.f / 255.f, 119.f / 255.f}, {30.f / 255.f, 1.f, 109.f / 255.f} };
		bool skybox = false;
		float skybox_color[3] = { 1.f, 1.f, 1.f };
		float skybox_intensity = 1.f;
		float velo_x = 500.f;
		float velo_y = 100.f;
		float velo_size = 30.f;

		bool aimbot = false;
		float aimbot_fov = 10.f;
		bool onlyvisible = false;
		bool aimbot_smooth = false;
		float aimbot_speed = 60.f;
		float aimbot_max_distance = 0.f;
		bool aim_bones[aim_bone_count] = { true };
		bool standalone_rcs = false;
		int standalone_rcs_strength = 100;
		int standalone_rcs_min = 40;
		int standalone_rcs_max = 70;

		float keystrokes_size{ 16.f };

		bool spec_list = false;
		bool spec_list_hide_empty = false;
		vec2_t speclist_pos{ 0.0f,0.0f };
		float spec_name_color[4]{ 1.f,1.f,1.f,1.f };
		float spec_player_name_color[4]{ 1.f,1.f,1.f,1.f };
		float spec_player_self_color[4]{ 1.f,1.f,1.f,1.f };

		bool show_crouch = false;
		bool show_in_menu = false;
		bool show_in_jump = false;
		bool grenade_trail = false;
		bool practice = false;
		int practice_save_key = 0;
		int practice_load_key = 0;

		bool keystrokes = false;
		vec2_t keystroke_pos{ 500.f, 500.f };
		
		bool bhop = false;
		bool ej = false;
		bool ladder_ej = false;
		bool ladder_bug = false;
		bool edgebug = false;
		bool jumpbug = false;
		int edgebug_ticks = 16;
		bool eb_lock = false;
		bool visualize_edgebug = false;
		int edgebug_visual_shape = 0;
		float edgebug_path_color[3] = { 0.3f, 1.f, 0.5f };
		bool pixelsurf = false;
		bool pixelsurf_silent = false;

		bool fov_changer = false;
		bool scopefov = false;
		float fov = 90.f;
		bool viewmodel = false;
		float viewmodel_fov = 68.f;
		float viewmodel_x = 2.5f;
		float viewmodel_y = 1.f;
		float viewmodel_z = -2.f;


	} config_t;

	extern config_t context;

	std::string get_directory();
	std::vector<std::string> list_files();
	bool save(const char* name);
	bool load(const char* name);
	bool remove(const char* name);
	bool* find_bool(const char* name);

}

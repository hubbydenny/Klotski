#include "config.hpp"

#include "binds.hpp"
#include "../../utilities/utilities.hpp"

#include <Windows.h>
#include <shlobj.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

config::config_t config::context = { };

namespace
{
	enum field_type_t : std::uint8_t
	{
		field_bool,
		field_float,
		field_bool_array
	};

	struct field_t
	{
		const char* name;
		field_type_t type;
		void* pointer;
		std::size_t count;
	};

	const field_t fields[] =
	{
		{ "player_esp", field_bool, &config::context.player_esp, 1 },
		{ "teammate_esp", field_bool, &config::context.teammate_esp, 1 },
		{ "draw_box", field_bool, &config::context.draw_box, 1 },
		{ "draw_name", field_bool, &config::context.draw_name, 1 },
		{ "draw_health", field_bool, &config::context.draw_health, 1 },
		{ "draw_armor", field_bool, &config::context.draw_armor, 1 },
		{ "draw_weapon", field_bool, &config::context.draw_weapon, 1 },
		{ "draw_skeleton", field_bool, &config::context.draw_skeleton, 1 },
		{ "draw_flags", field_bool, &config::context.draw_flags, 1 },
		{ "box_corner", field_bool, &config::context.box_corner, 1 },
		{ "box_corner_size", field_float, &config::context.box_corner_size, 1 },
		{ "box_thickness", field_float, &config::context.box_thickness, 1 },
		{ "box_outline", field_bool, &config::context.box_outline, 1 },
		{ "box_fill", field_bool, &config::context.box_fill, 1 },
		{ "box_fill_alpha", field_float, &config::context.box_fill_alpha, 1 },
		{ "esp_max_distance", field_float, &config::context.esp_max_distance, 1 },
		{ "esp_distance_fade", field_bool, &config::context.esp_distance_fade, 1 },
		{ "esp_opacity", field_float, &config::context.esp_opacity, 1 },
		{ "esp_color_r", field_float, &config::context.esp_colors[0], 1 },
		{ "esp_color_g", field_float, &config::context.esp_colors[1], 1 },
		{ "esp_color_b", field_float, &config::context.esp_colors[2], 1 },
		{ "esp_color_teammate_r", field_float, &config::context.esp_colors_teammate[0], 1 },
		{ "esp_color_teammate_g", field_float, &config::context.esp_colors_teammate[1], 1 },
		{ "esp_color_teammate_b", field_float, &config::context.esp_colors_teammate[2], 1 },
		{ "esp_color_by_health", field_bool, &config::context.esp_color_by_health, 1 },
		{ "esp_color_by_distance", field_bool, &config::context.esp_color_by_distance, 1 },
		{ "esp_flags", field_bool_array, config::context.esp_flags, esp_flag_count },
		{ "glow", field_bool, &config::context.glow, 1 },
		{ "glow_color_r", field_float, &config::context.glow_color[0], 1 },
		{ "glow_color_g", field_float, &config::context.glow_color[1], 1 },
		{ "glow_color_b", field_float, &config::context.glow_color[2], 1 },
		{ "glow_color_invincible_r", field_float, &config::context.glow_color_invincible[0], 1 },
		{ "glow_color_invincible_g", field_float, &config::context.glow_color_invincible[1], 1 },
		{ "glow_color_invincible_b", field_float, &config::context.glow_color_invincible[2], 1 },
		{ "glow_invincible", field_bool, &config::context.glow_invincible, 1 },
		{ "glow_alpha", field_float, &config::context.glow_alpha, 1 },
		{ "glow_brightness", field_float, &config::context.glow_brightness, 1 },{ "watermark", field_bool, &config::context.watermark, 1 },
		{ "watermark_x", field_float, &config::context.watermark_x, 1 },
		{ "watermark_y", field_float, &config::context.watermark_y, 1 },

		{ "velocity", field_bool, &config::context.velocity, 1 },
		{ "skybox", field_bool, &config::context.skybox, 1 },
		{ "scaleform_hud", field_bool, &config::context.scaleform_hud, 1 },
		{ "skybox_color_r", field_float, &config::context.skybox_color[0], 1 },
		{ "skybox_color_g", field_float, &config::context.skybox_color[1], 1 },
		{ "skybox_color_b", field_float, &config::context.skybox_color[2], 1 },
		{ "velo_x", field_float, &config::context.velo_x, 1 },
		{ "velo_y", field_float, &config::context.velo_y, 1 },
		{ "velo_size", field_float, &config::context.velo_size, 1 },

		{ "aimbot", field_bool, &config::context.aimbot, 1 },
		{ "aimbot_fov", field_float, &config::context.aimbot_fov, 1 },
		{ "onlyvisible", field_bool, &config::context.onlyvisible, 1 },
		{ "aimbot_smooth", field_bool, &config::context.aimbot_smooth, 1 },
		{ "aimbot_speed", field_float, &config::context.aimbot_speed, 1 },
		{ "aimbot_max_distance", field_float, &config::context.aimbot_max_distance, 1 },
		
		{ "aim_bones", field_bool_array, config::context.aim_bones, aim_bone_count },
		{ "bhop", field_bool, &config::context.bhop, 1 },
		{ "pixelsurf", field_bool, &config::context.pixelsurf, 1 },
		{ "pixelsurf_silent", field_bool, &config::context.pixelsurf_silent, 1 },
		

		{ "fov_changer", field_bool, &config::context.fov_changer, 1 },
		{ "scopefov", field_bool, &config::context.scopefov, 1 },
		{ "fov", field_float, &config::context.fov, 1 },
	};

	std::string get_config_directory()
	{
		char path[MAX_PATH] = { };

		if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, path)))
		{
			std::string directory = std::string(path) + "\\klotski";
			CreateDirectoryA(directory.c_str(), nullptr);
			return directory;
		}

		return std::string(".");
	}

	void write_field(std::ofstream& file, const field_t& field)
	{
		switch (field.type)
		{
		case field_bool:
			file << (*static_cast<bool*>(field.pointer) ? 1 : 0);
			break;

		case field_float:
			file << *static_cast<float*>(field.pointer);
			break;

		case field_bool_array:
		{
			bool* values = static_cast<bool*>(field.pointer);

			for (std::size_t i = 0; i < field.count; i++)
			{
				if (i != 0) file << ',';

				file << (values[i] ? 1 : 0);
			}

			break;
		}

		default:
			break;
		}
	}

	void read_field(const field_t& field, const std::string& value)
	{
		switch (field.type)
		{
		case field_bool:
			*static_cast<bool*>(field.pointer) = std::atoi(value.c_str()) != 0;
			break;

		case field_float:
			*static_cast<float*>(field.pointer) = std::strtof(value.c_str(), nullptr);
			break;

		case field_bool_array:
		{
			bool* values = static_cast<bool*>(field.pointer);
			std::stringstream stream(value);
			std::string token;
			std::size_t index = 0;

			while (index < field.count && std::getline(stream, token, ','))
			{
				values[index++] = std::atoi(token.c_str()) != 0;
			}

			break;
		}

		default:
			break;
		}
	}
}

std::string config::get_directory()
{
	return get_config_directory();
}

std::vector<std::string> config::list_files()
{
	std::vector<std::string> files;

	WIN32_FIND_DATAA find_data = { };
	HANDLE handle = FindFirstFileA((get_config_directory() + "\\*.cfg").c_str(), &find_data);

	if (handle == INVALID_HANDLE_VALUE)
	{
		return files;
	}

	do
	{
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			std::string name = find_data.cFileName;
			files.push_back(name.substr(0, name.length() - 4));
		}
	} while (FindNextFileA(handle, &find_data));

	FindClose(handle);

	return files;
}

bool config::save(const char* name)
{
	if (!name || !name[0])
	{
		return false;
	}

	const std::string path = get_config_directory() + "\\" + name + ".cfg";

	std::ofstream file(path.c_str(), std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	file << "version=" << version << '\n';

	for (const field_t& field : fields)
	{
		file << field.name << '=';
		write_field(file, field);
		file << '\n';
	}

	for (const binds::bind_t& bind : binds::list)
	{
		if (bind.name.empty() || !bind.key) continue;

		file << "bind." << bind.name << '=' << bind.key << ',' << bind.mode << '\n';
	}

	return file.good();
}

bool config::load(const char* name)
{
	if (!name || !name[0])
	{
		return false;
	}

	const std::string path = get_config_directory() + "\\" + name + ".cfg";

	std::ifstream file(path.c_str(), std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	binds::clear();

	std::string line;

	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#') continue;

		const std::size_t separator = line.find('=');

		if (separator == std::string::npos) continue;

		const std::string key = line.substr(0, separator);
		const std::string value = line.substr(separator + 1);

		if (key.rfind("bind.", 0) == 0)
		{
			const std::string bind_name = key.substr(5);
			bool* pointer = find_bool(bind_name.c_str());

			if (!pointer) continue;

			const std::size_t comma = value.find(',');
			const std::uint32_t bind_key = static_cast<std::uint32_t>(std::strtoul(value.c_str(), nullptr, 10));
			const std::int32_t bind_mode = comma == std::string::npos ? binds::bind_hold : static_cast<std::int32_t>(std::strtol(value.c_str() + comma + 1, nullptr, 10));
			const std::size_t index = binds::get(bind_name, pointer);

			binds::list[index].key = bind_key;
			binds::list[index].mode = bind_mode < 0 || bind_mode >= binds::bind_mode_count ? binds::bind_hold : bind_mode;
			binds::list[index].previous = false;
			binds::list[index].capturing = false;

			continue;
		}

		for (const field_t& field : fields)
		{
			if (key != field.name) continue;

			read_field(field, value);
			break;
		}
	}

	return true;
}

bool config::remove(const char* name)
{
	if (!name || !name[0])
	{
		return false;
	}

	const std::string path = get_config_directory() + "\\" + name + ".cfg";

	return DeleteFileA(path.c_str()) != 0;
}

bool* config::find_bool(const char* name)
{
	if (!name) return nullptr;

	for (const field_t& field : fields)
	{
		if (field.type != field_bool) continue;
		if (std::strcmp(field.name, name) != 0) continue;

		return static_cast<bool*>(field.pointer);
	}

	return nullptr;
}

#include "menu.hpp"

#include "../../source2-sdk/sdk.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"
#include "../config/config.hpp"
#include "../config/binds.hpp"

#include "../../utilities/imgui/imgui.h"
#include "../../utilities/imgui/imgui_internal.h"
#include "../../utilities/imgui/imgui_impl_win32.h"
#include "../../utilities/imgui/imgui_impl_dx11.h"

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

extern bool menu::open = true;

static char config_name_buffer[128] = "";
static int selected_config = -1;
static std::vector<std::string> config_files;

static void refresh_config_files()
{
	config_files = config::list_files();

	if (selected_config >= static_cast<int>(config_files.size()))
	{
		selected_config = -1;
	}
}

template <typename entry_t, std::size_t count>
static const char* multi_select_preview(const entry_t (&entries)[count], const bool* values)
{
	static char buffer[128] = { };

	std::size_t length = 0;

	buffer[0] = '\0';

	for (std::size_t i = 0; i < count; i++)
	{
		if (!values[i]) continue;

		const char* label = entries[i].label;
		const std::size_t label_length = std::strlen(label);

		if (length + label_length + 3 >= sizeof(buffer)) break;

		if (length != 0)
		{
			buffer[length++] = ',';
			buffer[length++] = ' ';
		}

		std::memcpy(buffer + length, label, label_length);
		length += label_length;
		buffer[length] = '\0';
	}

	return length == 0 ? "none" : buffer;
}

template <typename entry_t, std::size_t count>
static void multi_select_combo(const char* label, const entry_t (&entries)[count], bool* values)
{
	if (!ImGui::BeginCombo(label, multi_select_preview(entries, values)))
	{
		return;
	}

	for (std::size_t i = 0; i < count; i++)
	{
		if (ImGui::Selectable(entries[i].label, values[i], ImGuiSelectableFlags_DontClosePopups))
		{
			values[i] = !values[i];
		}
	}

	ImGui::EndCombo();
}

static const char* key_to_string(std::uint32_t key)
{
	static char buffer[16];

	if (!key)
	{
		return "none";
	}

	if (key >= VK_F1 && key <= VK_F12)
	{
		sprintf_s(buffer, sizeof(buffer), "F%d", key - VK_F1 + 1);
		return buffer;
	}

	if (key >= '0' && key <= '9' || key >= 'A' && key <= 'Z')
	{
		buffer[0] = static_cast<char>(key);
		buffer[1] = '\0';
		return buffer;
	}

	switch (key)
	{
	case VK_SPACE: return "space";
	case VK_SHIFT: return "shift";
	case VK_CONTROL: return "ctrl";
	case VK_MENU: return "alt";
	case VK_INSERT: return "insert";
	case VK_DELETE: return "delete";
	case VK_HOME: return "home";
	case VK_END: return "end";
	case VK_PRIOR: return "pgup";
	case VK_NEXT: return "pgdn";
	case 0x05: return "mouse4";
	case 0x06: return "mouse5";
	default: break;
	}

	sprintf_s(buffer, sizeof(buffer), "0x%02X", key);
	return buffer;
}

static bool settings_button(const char* name)
{
	ImGui::PushID(name);

	const bool pressed = ImGui::SmallButton("...");

	if (ImGui::IsItemHovered()) ImGui::SetTooltip("settings");

	ImGui::PopID();

	return pressed;
}

static const char* bind_modes[] = { "hold", "toggle", "always" };

static void bind_checkbox(const char* name, const char* label, bool* value)
{
	ImGui::Checkbox(label, value);
	ImGui::SameLine();

	binds::bind_t& bind = binds::list[binds::get(name, value)];

	ImGui::PushID(name);

	const char* text = bind.capturing ? "..." : key_to_string(bind.key);

	if (ImGui::Button(text, ImVec2(60.f, 0.f)))
	{
		bind.capturing = !bind.capturing;
	}

	if (bind.capturing)
	{
		for (std::uint32_t vkey = 1; vkey < 255; vkey++)
		{
			if (vkey == VK_LBUTTON || vkey == VK_RBUTTON || vkey == VK_MBUTTON)
			{
				continue;
			}

			if (binds::key_down(vkey))
			{
				bind.key = (vkey == VK_ESCAPE) ? 0 : vkey;
				bind.previous = false;
				bind.capturing = false;
				break;
			}
		}
	}

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("mode");
	}

	if (ImGui::BeginPopup("mode"))
	{
		for (int m = 0; m < binds::bind_mode_count; m++)
		{
			if (ImGui::Selectable(bind_modes[m], bind.mode == m))
			{
				bind.mode = m;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	ImGui::PopID();
}

void menu::render()
{
	if (!menu::open)
	{
		if (interfaces::input_system->is_relative_mouse_mode())
		{
			sdl::set_relative_mouse_mode(true);
			sdl::set_window_polling_mode(interfaces::input_system->get_sdl_window(), true);
			sdl::set_mouse_warp_position(nullptr, sdk::screen_width / 2, sdk::screen_height / 2);
		}

		return;
	}

	sdl::set_relative_mouse_mode(false);
	sdl::set_window_polling_mode(interfaces::input_system->get_sdl_window(), false);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(800.f, 400.f));

	if (ImGui::Begin("klotski", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		if (ImGui::BeginTabBar("##features"))
		{
			if (ImGui::BeginTabItem("combat"))
			{
				bind_checkbox("aimbot", "aimbot", &config::context.aimbot);

				if (settings_button("aimbot"))
				{
					ImGui::OpenPopup("aimbot_popup");
				}

				if (ImGui::BeginPopup("aimbot_popup"))
				{
					ImGui::SliderFloat("fov", &config::context.aimbot_fov, 1.f, 30.f, "%.1f");
					ImGui::Checkbox("visible only", &config::context.onlyvisible);
					multi_select_combo("aim bones", aim_bones_table, config::context.aim_bones);
					ImGui::Separator();
					ImGui::Checkbox("smooth", &config::context.aimbot_smooth);

					if (config::context.aimbot_smooth)
					{
						ImGui::SliderFloat("smooth speed", &config::context.aimbot_speed, 1.f, 100.f, "%.0f");
					}

					ImGui::Separator();
					ImGui::Checkbox("rcs", &config::context.standalone_rcs);

					if (config::context.standalone_rcs)
					{
						ImGui::SliderInt("strength", &config::context.standalone_rcs_strength, 0, 100);
						ImGui::SliderInt("rcs min", &config::context.standalone_rcs_min, 0, 100);
						ImGui::SliderInt("rcs max", &config::context.standalone_rcs_max, 0, 100);
					}

					ImGui::EndPopup();
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("visuals"))
			{
				bind_checkbox("players", "players", &config::context.player_esp);

				ImGui::SameLine();
				ImGui::Checkbox("teammates", &config::context.teammate_esp);

				ImGui::SameLine();
				if (settings_button("esp"))
				{
					ImGui::OpenPopup("esp_popup");
				}

				if (ImGui::BeginPopup("esp_popup"))
				{
					ImGui::SeparatorText("box");

					ImGui::Checkbox("draw box", &config::context.draw_box);

					if (config::context.draw_box)
					{
						ImGui::Checkbox("corner", &config::context.box_corner);

						if (config::context.box_corner)
						{
							ImGui::SliderFloat("corner size", &config::context.box_corner_size, 1.f, 50.f, "%.0f%%");
						}

						ImGui::SliderFloat("thickness", &config::context.box_thickness, 1.f, 6.f, "%.0f");
						ImGui::Checkbox("outline", &config::context.box_outline);
						ImGui::Checkbox("fill", &config::context.box_fill);

						if (config::context.box_fill)
						{
							ImGui::SliderFloat("fill alpha", &config::context.box_fill_alpha, 0.01f, 1.f, "%.2f");
						}
					}

					ImGui::SeparatorText("elements");

					ImGui::Checkbox("name", &config::context.draw_name);
					ImGui::Checkbox("health", &config::context.draw_health);
					ImGui::Checkbox("armor", &config::context.draw_armor);
					ImGui::Checkbox("weapon", &config::context.draw_weapon);
					ImGui::Checkbox("skeleton", &config::context.draw_skeleton);
					ImGui::Checkbox("flags", &config::context.draw_flags);

					if (config::context.draw_flags)
					{
						multi_select_combo("flags list", esp_flags_table, config::context.esp_flags);
					}

					ImGui::SeparatorText("colors");

					ImGui::ColorEdit3("enemy", config::context.esp_colors);
					ImGui::Checkbox("by health", &config::context.esp_color_by_health);
					ImGui::ColorEdit3("teammate", config::context.esp_colors_teammate);
					ImGui::SliderFloat("opacity", &config::context.esp_opacity, 0.05f, 1.f, "%.2f");

					ImGui::SeparatorText("distance");

					ImGui::SliderFloat("max", &config::context.esp_max_distance, 0.f, 3000.f, "%.0f");
					ImGui::Checkbox("fade", &config::context.esp_distance_fade);

					if (config::context.esp_distance_fade)
					{
						ImGui::Checkbox("color by distance", &config::context.esp_color_by_distance);
					}

					ImGui::EndPopup();
				}

				ImGui::SameLine();
				ImGui::Checkbox("glow", &config::context.glow);

				if (settings_button("glow"))
				{
					ImGui::OpenPopup("glow_popup");
				}

				if (ImGui::BeginPopup("glow_popup"))
				{
					ImGui::ColorEdit3("color", config::context.glow_color);
					ImGui::Checkbox("invincible", &config::context.glow_invincible);

					if (config::context.glow_invincible)
					{
						ImGui::ColorEdit3("invincible color", config::context.glow_color_invincible);
					}

					ImGui::SliderFloat("brightness", &config::context.glow_brightness, 0.1f, 3.f, "%.2f");
					ImGui::SliderFloat("alpha", &config::context.glow_alpha, 0.1f, 1.f, "%.2f");

					ImGui::EndPopup();
				}

				ImGui::SameLine();
				ImGui::Checkbox("fov changer", &config::context.fov_changer);

				if (settings_button("fov"))
				{
					ImGui::OpenPopup("fov_popup");
				}

				if (ImGui::BeginPopup("fov_popup"))
				{
					ImGui::SliderFloat("value", &config::context.fov, 60.f, 150.f, "%.1f");
					ImGui::Checkbox("scopefov", &config::context.scopefov);

					ImGui::EndPopup();
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("movement"))
			{
				bind_checkbox("bhop", "bhop", &config::context.bhop);

				ImGui::SameLine();
				ImGui::TextUnformatted("hold space");

				ImGui::SameLine();
				bind_checkbox("pixelsurf", "pixelsurf", &config::context.pixelsurf);

				if (settings_button("pixelsurf"))
				{
					ImGui::OpenPopup("pixelsurf_popup");
				}

				if (ImGui::BeginPopup("pixelsurf_popup"))
				{
					ImGui::Checkbox("silent", &config::context.pixelsurf_silent);

					ImGui::EndPopup();
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("misc"))
			{
				ImGui::Checkbox("watermark", &config::context.watermark);

				if (settings_button("watermark"))
				{
					ImGui::OpenPopup("watermark_popup");
				}

				if (ImGui::BeginPopup("watermark_popup"))
				{
					ImGui::SliderFloat("x", &config::context.watermark_x, 0.f, static_cast<float>(sdk::screen_width), "%.0f");
					ImGui::SliderFloat("y", &config::context.watermark_y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");

					ImGui::EndPopup();
				}

				ImGui::SameLine();
				ImGui::Checkbox("velocity", &config::context.velocity);

				if (settings_button("velocity"))
				{
					ImGui::OpenPopup("velocity_popup");
				}

				if (ImGui::BeginPopup("velocity_popup"))
				{
					ImGui::SliderFloat("x", &config::context.velo_x, 0.f, static_cast<float>(sdk::screen_width), "%.0f");
					ImGui::SliderFloat("y", &config::context.velo_y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
					ImGui::SliderFloat("size", &config::context.velo_size, 8.f, 48.f, "%.0f");

					ImGui::EndPopup();
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("configs"))
			{
				static bool needs_refresh = true;

				if (needs_refresh)
				{
					refresh_config_files();
					needs_refresh = false;
				}

				if (ImGui::BeginListBox("##configs", ImVec2(-1.f, 0.f)))
				{
					for (int i = 0; i < static_cast<int>(config_files.size()); i++)
					{
						if (ImGui::Selectable(config_files[i].c_str(), selected_config == i))
						{
							selected_config = i;
						}
					}

					ImGui::EndListBox();
				}

				if (ImGui::Button("load", ImVec2(90.f, 0.f)) && selected_config >= 0)
				{
					config::load(config_files[selected_config].c_str());
				}

				ImGui::SameLine();

				if (ImGui::Button("save", ImVec2(90.f, 0.f)) && selected_config >= 0)
				{
					config::save(config_files[selected_config].c_str());
				}

				ImGui::SameLine();

				if (ImGui::Button("delete", ImVec2(90.f, 0.f)) && selected_config >= 0)
				{
					config::remove(config_files[selected_config].c_str());
					refresh_config_files();
				}

				ImGui::Separator();
				ImGui::InputText("name", config_name_buffer, sizeof(config_name_buffer));

				if (ImGui::Button("create", ImVec2(90.f, 0.f)))
				{
					if (config_name_buffer[0])
					{
						config::save(config_name_buffer);
						refresh_config_files();
					}
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
	}
	ImGui::PopStyleVar();
}

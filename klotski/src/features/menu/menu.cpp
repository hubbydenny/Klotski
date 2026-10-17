#include "menu.hpp"

#include "../../source2-sdk/sdk.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"
#include "../config/config.hpp"
#include "../config/binds.hpp"
#include "../misc/misc.hpp"

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

static bool menu_button(const char* label, ImVec2 size = ImVec2(150.f, 20.f))
{
    if (size.y <= 0.f) size.y = 20.f;
    const auto pos = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(label, size);
    auto draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(ImVec2(pos.x - 1.f, pos.y - 1.f), ImVec2(pos.x + size.x + 1.f, pos.y + size.y + 1.f), IM_COL32(0, 0, 0, 255));
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::IsItemHovered() ? IM_COL32(44, 44, 44, 255) : IM_COL32(36, 36, 36, 255));
    draw->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    draw->AddText(ImVec2(pos.x + 10.f, pos.y + 3.f), IM_COL32(153, 153, 153, 255), label);
    draw->PopClipRect();
    return pressed;
}

static void refresh_config_files()
{
	config_files = config::list_files();

	if (selected_config >= static_cast<int>(config_files.size()))
	{
		selected_config = -1;
	}
}

static bool color_picker(const char* label, float color[4], bool alpha, bool compact = false)
{
    ImGui::PushID(label);
    const auto pos = ImGui::GetCursorScreenPos();
    if (!compact) ImGui::Dummy(ImVec2(190.f, 12.f));
    const float swatch_offset = compact ? 0.f : 150.f;
    auto draw = ImGui::GetWindowDrawList();
    if (!compact) {
        draw->AddText(ImVec2(pos.x + 21.f, pos.y + 1.f), IM_COL32(0, 0, 0, 255), label);
        draw->AddText(ImVec2(pos.x + 20.f, pos.y), ImGui::GetColorU32(ImGuiCol_Text), label);
    }
    ImGui::SetCursorScreenPos(ImVec2(pos.x + swatch_offset, pos.y + 3.f));
    const bool pressed = ImGui::InvisibleButton("##swatch", ImVec2(20.f, 8.f));
    const ImVec4 top(color[0], color[1], color[2], 1.f);
    const ImVec4 bottom(ImMax(0.f, color[0] - 25.f / 255.f), ImMax(0.f, color[1] - 25.f / 255.f), ImMax(0.f, color[2] - 25.f / 255.f), 1.f);
    draw->AddRectFilled(ImVec2(pos.x + swatch_offset - 1.f, pos.y + 2.f), ImVec2(pos.x + swatch_offset + 21.f, pos.y + 12.f), IM_COL32(0, 0, 0, 255));
    draw->AddRectFilledMultiColor(ImVec2(pos.x + swatch_offset, pos.y + 3.f), ImVec2(pos.x + swatch_offset + 20.f, pos.y + 11.f),
        ImGui::ColorConvertFloat4ToU32(top), ImGui::ColorConvertFloat4ToU32(top),
        ImGui::ColorConvertFloat4ToU32(bottom), ImGui::ColorConvertFloat4ToU32(bottom));
    if (!compact) ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 15.f));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", label);
    if (pressed) ImGui::OpenPopup("##picker");
    bool changed = false;
    ImGui::SetNextWindowPos(ImVec2(pos.x + swatch_offset, pos.y + 13.f), ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.f, 1.f));
    if (ImGui::BeginPopup("##picker"))
    {
        const auto origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##palette", ImVec2(160.f, 160.f));
        auto palette = ImGui::GetWindowDrawList();
        for (int i = 0; i < 160; ++i)
        {
            float red, green, blue;
            ImGui::ColorConvertHSVtoRGB(static_cast<float>(i) / 160.f, 1.f, 1.f, red, green, blue);
            const auto middle = ImGui::ColorConvertFloat4ToU32(ImVec4(red, green, blue, 1.f));
            palette->AddRectFilledMultiColor(ImVec2(origin.x + i, origin.y), ImVec2(origin.x + i + 1.f, origin.y + 80.f),
                IM_COL32_WHITE, IM_COL32_WHITE, middle, middle);
            palette->AddRectFilledMultiColor(ImVec2(origin.x + i, origin.y + 80.f), ImVec2(origin.x + i + 1.f, origin.y + 160.f),
                middle, middle, IM_COL32_BLACK, IM_COL32_BLACK);
        }
        if (ImGui::IsItemActive() || (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)))
        {
            const auto mouse = ImGui::GetIO().MousePos;
            const float hue = ImClamp((mouse.x - origin.x) / 160.f, 0.f, 1.f);
            const float y = ImClamp(mouse.y - origin.y, 0.f, 160.f);
            ImGui::ColorConvertHSVtoRGB(hue, y < 80.f ? y / 80.f : 1.f,
                y < 80.f ? 1.f : 1.f - (y - 80.f) / 80.f, color[0], color[1], color[2]);
            changed = true;
        }
        
        if (alpha) {
            ImGui::SetNextItemWidth(110.f);
            changed |= ImGui::SliderFloat("alpha", &color[3], 0.f, 1.f, "%.2f");
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
    return changed;
}

bool menu::color_edit4(const char* label, float (&color)[4])
{
	return color_picker(label, color, true);
}
bool menu::color_edit4(const char* label, float (&color)[3], float* alpha)
{
	float rgba[4] = { color[0], color[1], color[2], alpha ? *alpha : 1.f };
	if (!color_picker(label, rgba, alpha != nullptr)) return false;
	for (int i = 0; i < 3; i++) color[i] = rgba[i];
	if (alpha) *alpha = rgba[3];
	return true;
}


template <std::size_t count>
static void inline_color(const char* id, float (&color)[count], float* alpha = nullptr)
{
    static_assert(count == 3 || count == 4);
    if constexpr (count == 4) {
        if (!alpha) alpha = &color[3];
    }
    const auto next = ImGui::GetCursorScreenPos();
    ImGui::SameLine(ImGui::GetWindowWidth() - 52.f);
    float rgba[4] = { color[0], color[1], color[2], alpha ? *alpha : 1.f };
    if (color_picker(id, rgba, alpha != nullptr, true)) {
        for (int i = 0; i < 3; ++i) color[i] = rgba[i];
        if (alpha) *alpha = rgba[3];
    }
    ImGui::SetCursorScreenPos(next);
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

static bool menu_combo_begin(const char* label, const char* preview)
{
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    const auto pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::CalcItemWidth();
    const float height = 20.f;
    const bool pressed = ImGui::InvisibleButton("##dropdown", ImVec2(width, height));
    auto draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(36, 36, 36, 255));
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + height), ImGui::IsItemHovered() ? IM_COL32(100, 100, 100, 255) : IM_COL32(1, 1, 1, 255));
    draw->PushClipRect(pos, ImVec2(pos.x + width - 19.f, pos.y + height), true);
    draw->AddText(ImVec2(pos.x + 10.f, pos.y + 3.f), IM_COL32(153, 153, 153, 255), preview);
    draw->PopClipRect();
    draw->AddTriangleFilled(ImVec2(pos.x + width - 13.f, pos.y + height * 0.5f - 2.f),
        ImVec2(pos.x + width - 5.f, pos.y + height * 0.5f - 2.f),
        ImVec2(pos.x + width - 9.f, pos.y + height * 0.5f + 2.f), IM_COL32(153, 153, 153, 255));
    if (pressed) ImGui::OpenPopup("##choices");
    ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + height));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.f), ImVec2(width, 300.f));
    if (ImGui::BeginPopup("##choices")) return true;
    ImGui::PopID();
    return false;
}

static void menu_combo_end()
{
    ImGui::EndPopup();
    ImGui::PopID();
}

static bool menu_combo(const char* label, int* selected, const char* const* items, int count)
{
    bool changed = false;
    if (!menu_combo_begin(label, *selected >= 0 && *selected < count ? items[*selected] : "none")) return false;
    for (int i = 0; i < count; ++i) {
        if (ImGui::Selectable(items[i], *selected == i)) { *selected = i; changed = true; }
        if (*selected == i) ImGui::SetItemDefaultFocus();
    }
    menu_combo_end();
    return changed;
}

static bool menu_combo(const char* label, int* selected, const char* items)
{
    std::vector<const char*> entries;
    for (auto item = items; *item; item += std::strlen(item) + 1) entries.push_back(item);
    return menu_combo(label, selected, entries.data(), static_cast<int>(entries.size()));
}

template <typename entry_t, std::size_t count>
static void multi_select_combo(const char* label, const entry_t (&entries)[count], bool* values)
{
	if (!menu_combo_begin(label, multi_select_preview(entries, values))) return;

	for (std::size_t i = 0; i < count; i++)
	{
		if (ImGui::Selectable(entries[i].label, values[i], ImGuiSelectableFlags_DontClosePopups))
		{
			values[i] = !values[i];
		}
	}

	menu_combo_end();
}

static const char* key_to_string(std::uint32_t key)
{
	static char buffer[16];

	if (!key) return "none";
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

	switch (key) {
	case VK_SPACE: return "space";
	case VK_SHIFT: return "shift";
	case VK_CONTROL: return "ctrl";
	case VK_MENU: return "alt";
	case VK_INSERT: return "insert";
	case VK_DELETE: return "delete";
	case VK_HOME: return "home";
	case VK_CAPITAL: return "caps";
	case VK_END: return "end";
	case VK_PRIOR: return "pgup";
	case VK_NEXT: return "pgdn";
	case VK_LBUTTON:  return "mouse1";
	case VK_RBUTTON:  return "mouse2";
	case VK_MBUTTON:  return "mouse3";
	case VK_XBUTTON1: return "mouse4";
	case VK_XBUTTON2: return "mouse5";
	default: break;
	}

	sprintf_s(buffer, sizeof(buffer), "0x%02X", key);
	return buffer;
}

static void practice_keybind(const char* label, int& key)
{
	static int* capturing = nullptr;
	static bool ready = false;
	static bool previous[256]{};
	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	if (menu_button(capturing == &key ? "press key" : key_to_string(key)))
	{
		capturing = capturing == &key ? nullptr : &key;
		ready = false;
		for (int i = 1; i < 256; i++) previous[i] = binds::key_down(i);
	}
	if (capturing == &key)
	{
		if (!binds::key_down(VK_LBUTTON)) ready = true;
		for (int i = 1; i < 256; i++)
		{
			const bool down = binds::key_down(i);
			if (ready && down && !previous[i])
			{
				const bool mouse = i == VK_LBUTTON || i == VK_RBUTTON || i == VK_MBUTTON || i == VK_XBUTTON1 || i == VK_XBUTTON2;
				if (!mouse || !ImGui::IsAnyItemHovered())
				{
					key = i == VK_ESCAPE ? 0 : i;
					capturing = nullptr;
				}
			}
			previous[i] = down;
			if (!capturing) break;
		}
	}
	ImGui::PopID();
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
static const char* flag_positions[] = { "bottom", "right" };
static const char* chams_materials[] = { "flat", "glow", "bloom", "hologram", "textured", "illuminate", "metallic", "pearlescent" };

static bool menu_checkbox(const char* label, bool* value);
static void bind_checkbox(const char* name, const char* label, bool* value) {
	menu_checkbox(label, value);

	binds::bind_t& bind = binds::list[binds::get(name, value)];

	ImGui::PushID(name);

	const char* text = bind.capturing ? "..." : key_to_string(bind.key);

	ImGui::Indent(20.f);
	ImGui::TextUnformatted("key");
	if (menu_button(text))
	{
		for (auto& other : binds::list)
		{
			if (&other == &bind) continue;
			other.capturing = false;
			other.capture_ready = false;
		}
		bind.capturing = !bind.capturing;
		bind.capture_ready = false;
	}

	const bool capture_active = bind.capturing;
	if (bind.capturing && !bind.capture_ready && !binds::key_down(VK_LBUTTON)) bind.capture_ready = true;
	if (bind.capturing && bind.capture_ready)
	{
		for (std::uint32_t vkey = 1; vkey < 255; vkey++)
		{
			const bool mouse = vkey == VK_LBUTTON || vkey == VK_RBUTTON || vkey == VK_MBUTTON;
			const bool pressed = vkey == VK_LBUTTON ? ImGui::IsMouseClicked(ImGuiMouseButton_Left) : vkey == VK_RBUTTON ? ImGui::IsMouseClicked(ImGuiMouseButton_Right) : vkey == VK_MBUTTON ? ImGui::IsMouseClicked(ImGuiMouseButton_Middle) : binds::key_down(vkey);
			if (pressed)
			{
				if (mouse && ImGui::IsAnyItemHovered()) continue;
				bind.key = (vkey == VK_ESCAPE) ? 0 : vkey;
				bind.previous = binds::key_down(bind.key);
				bind.capturing = false;
				bind.capture_ready = false;
				break;
			}
		}
	}

	if (!capture_active && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup("mode");
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
	ImGui::Unindent(20.f);
}


static bool menu_checkbox(const char* label, bool* value)
{
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const ImGuiID id = window->GetID(label);
    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 text = ImGui::CalcTextSize(label, nullptr, true);
    const float height = 12.f;
    const ImRect bounds(pos, ImVec2(pos.x + text.x + 23.f, pos.y + height));
    ImGui::ItemSize(bounds);
    if (!ImGui::ItemAdd(bounds, id)) return false;
    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bounds, id, &hovered, &held);
    if (pressed) { *value = !*value; ImGui::MarkItemEdited(id); }
    const ImVec2 box(pos.x, pos.y + 3.f);
    window->DrawList->AddRectFilled(box, ImVec2(box.x + 8.f, box.y + 8.f), IM_COL32(0, 0, 0, 255));
    window->DrawList->AddRectFilledMultiColor(ImVec2(box.x + 1.f, box.y + 1.f), ImVec2(box.x + 7.f, box.y + 7.f),*value ? IM_COL32(181, 0, 41, 255) : IM_COL32(69, 69, 69, 255), *value ? IM_COL32(181, 0, 41, 255) : IM_COL32(69, 69, 69, 255), *value ? IM_COL32(156, 0, 16, 255) : IM_COL32(44, 44, 44, 255), *value ? IM_COL32(156, 0, 16, 255) : IM_COL32(44, 44, 44, 255));
    window->DrawList->AddText(ImVec2(pos.x + 21.f, pos.y + 1.f), IM_COL32(0, 0, 0, 255), label);
    ImGui::RenderText(ImVec2(pos.x + 20.f, pos.y), label);
    ImGui::RenderNavHighlight(bounds, id);
    return pressed;
}

static bool menu_slider(const char* label, void* value, ImGuiDataType type, const void* minimum, const void* maximum, const char* format)
{
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    const auto pos = ImGui::GetCursorScreenPos();
    const float width = ImGui::CalcItemWidth();
    static ImGuiID editing_id = 0;
    static int editing_frame = -1;
    static int editing_int = 0;
    static float editing_float = 0.f;
    const ImGuiID slider_id = ImGui::GetID("##slider");
    const int frame = ImGui::GetFrameCount();
    if (editing_id && frame > editing_frame + 1) editing_id = 0;
    const bool is_float = type == ImGuiDataType_Float;
    const double original = is_float ? *static_cast<float*>(value) : *static_cast<int*>(value);
    char preview[64]{};
    if (is_float) ImFormatString(preview, sizeof(preview), format, original);
    else ImFormatString(preview, sizeof(preview), format, static_cast<int>(original));
    const float hit_width = width + 8.f + ImGui::CalcTextSize(preview).x;
    bool focus_input = false;
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + hit_width, pos.y + 16.f)) &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        editing_id = slider_id;
        editing_frame = frame;
        if (is_float) editing_float = *static_cast<float*>(value);
        else editing_int = *static_cast<int*>(value);
        if (GImGui->ActiveId == slider_id) ImGui::ClearActiveID();
        focus_input = true;
    }
    if (editing_id == slider_id) {
        editing_frame = frame;
        if (focus_input) ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(width);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.f, 2.f));
        void* temporary = is_float ? static_cast<void*>(&editing_float) : static_cast<void*>(&editing_int);
        const bool enter = ImGui::InputScalar("##value", type, temporary, nullptr, nullptr,
            is_float ? "%.6f" : "%d", ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        const bool cancel = ImGui::IsKeyPressed(ImGuiKey_Escape);
        const bool leave = !focus_input && ImGui::IsItemDeactivated();
        ImGui::PopStyleVar();
        bool changed = false;
        if (!cancel && (enter || leave)) {
            if (is_float && std::isfinite(editing_float)) {
                const float next = ImClamp(editing_float, *static_cast<const float*>(minimum), *static_cast<const float*>(maximum));
                changed = next != *static_cast<float*>(value);
                *static_cast<float*>(value) = next;
            } else if (!is_float) {
                const int next = ImClamp(editing_int, *static_cast<const int*>(minimum), *static_cast<const int*>(maximum));
                changed = next != *static_cast<int*>(value);
                *static_cast<int*>(value) = next;
            }
        }
        if (cancel || enter || leave) editing_id = 0;
        ImGui::PopID();
        return changed;
    }
    ImGui::InvisibleButton("##slider", ImVec2(width, 16.f));
    const double low = type == ImGuiDataType_Float ? *static_cast<const float*>(minimum) : *static_cast<const int*>(minimum);
    const double high = type == ImGuiDataType_Float ? *static_cast<const float*>(maximum) : *static_cast<const int*>(maximum);
    const double old = type == ImGuiDataType_Float ? *static_cast<float*>(value) : *static_cast<int*>(value);
    double next = old;
    bool interacting = false;
	bool left = false;
	bool right = false;
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(0) && high > low) {
        next = low + ImClamp((ImGui::GetIO().MousePos.x - pos.x) / width, 0.f, 1.f) * (high - low);
        interacting = true;
    }
    if (ImGui::IsItemFocused() && (GImGui->ActiveId == 0 || GImGui->ActiveId == ImGui::GetItemID())) {
        const double step = type == ImGuiDataType_Float ? (high - low) / 100.0 : 1.0;
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { next -= step; interacting = true; left = true; }
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { next += step; interacting = true; right = true; }
    }
    if (interacting) {
        next = ImClamp(next, low, high);
        if (type == ImGuiDataType_Float) *static_cast<float*>(value) = static_cast<float>(next);
        else *static_cast<int*>(value) = static_cast<int>(std::round(next));
    }
    const double current = type == ImGuiDataType_Float ? *static_cast<float*>(value) : *static_cast<int*>(value);
    const float fraction = high > low ? ImClamp(static_cast<float>((current - low) / (high - low)), 0.f, 1.f) : 0.f;
    auto draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + 7.f), IM_COL32(0, 0, 0, 255));
    draw->AddRectFilledMultiColor(ImVec2(pos.x + 1.f, pos.y + 1.f), ImVec2(pos.x + width - 1.f, pos.y + 6.f),IM_COL32(69, 69, 69, 255), IM_COL32(69, 69, 69, 255), IM_COL32(44, 44, 44, 255), IM_COL32(44, 44, 44, 255));
    if (fraction > 0.f) draw->AddRectFilledMultiColor(ImVec2(pos.x + 1.f, pos.y + 1.f), ImVec2(pos.x + 1.f + (width - 2.f) * fraction, pos.y + 6.f), IM_COL32(181, 0, 41, 255), IM_COL32(181, 0, 41, 255), IM_COL32(105, 0, 24, 255), IM_COL32(105, 0, 24, 255));
    char buffer[64]{};
    if (type == ImGuiDataType_Float) ImFormatString(buffer, sizeof(buffer), format, current);
    else ImFormatString(buffer, sizeof(buffer), format, static_cast<int>(current));
    draw->AddText(ImVec2(pos.x + width + 8.f, pos.y - 3.f), ImGui::GetColorU32(ImGuiCol_Text), buffer);
    if (current != old) ImGui::MarkItemEdited(ImGui::GetItemID());
    ImGui::PopID();
    return current != old;
}

static bool menu_slider_float(const char* label, float* value, float minimum, float maximum, const char* format = "%.3f") {
    return menu_slider(label, value, ImGuiDataType_Float, &minimum, &maximum, format);
}

static bool menu_slider_int(const char* label, int* value, int minimum, int maximum, const char* format = "%d")
{
    return menu_slider(label, value, ImGuiDataType_S32, &minimum, &maximum, format);
}

static ImVec2 group_start{};
static const char* group_title = nullptr;

static void menu_group_end()
{
    if (!group_title) return;
    const auto end = ImGui::GetCursorScreenPos();
    auto draw = ImGui::GetWindowDrawList();
    const float right = group_start.x + 211.f;
    const auto color = IM_COL32(44, 44, 44, 255);
    const float title_width = ImGui::CalcTextSize(group_title).x;
    draw->AddLine(group_start, ImVec2(group_start.x + 8.f, group_start.y), color);
    draw->AddLine(ImVec2(group_start.x + 16.f + title_width, group_start.y), ImVec2(right, group_start.y), color);
    draw->AddLine(group_start, ImVec2(group_start.x, end.y + 5.f), color);
    draw->AddLine(ImVec2(right, group_start.y), ImVec2(right, end.y + 5.f), color);
    draw->AddLine(ImVec2(group_start.x, end.y + 5.f), ImVec2(right, end.y + 5.f), color);
    draw->AddText(ImVec2(group_start.x + 12.f, group_start.y - 6.f), ImGui::GetColorU32(ImGuiCol_Text), group_title);
    group_title = nullptr;
    ImGui::Dummy(ImVec2(0.f, 18.f));
}

static void menu_group(const char* title)
{
    menu_group_end();
    group_start = ImGui::GetCursorScreenPos();
    group_start.x -= 10.f;
    group_title = title;
    ImGui::Dummy(ImVec2(0.f, 14.f));
}

static void menu_column(const char* id)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::BeginChild(id, ImVec2(230.f, 470.f), false);
    ImGui::PopStyleVar();
    ImGui::Indent(20.f);
    ImGui::PushItemWidth(150.f);
    ImGui::Dummy(ImVec2(0.f, 8.f));

}

static void menu_column_end()
{
    menu_group_end();
    ImGui::PopItemWidth();
    ImGui::Unindent(20.f);
    ImGui::EndChild();
}

void menu::render() {
    binds::get("aimbot", &config::context.aimbot);
    binds::get("players", &config::context.player_esp);
    binds::get("ej", &config::context.ej);
    binds::get("ladder_ej", &config::context.ladder_ej);
    binds::get("ladderbug", &config::context.ladder_bug);
    binds::get("edgebug", &config::context.edgebug);
    binds::get("jumpbug", &config::context.jumpbug);
    binds::get("pixelsurf", &config::context.pixelsurf);
    binds::get("visualize", &config::context.visualize_edgebug);
	if (!menu::open) return;

	if (menu::font) ImGui::PushFont(menu::font);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(522.f, 560.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 10.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.f, 3.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(4.f / 255.f, 4.f / 255.f, 4.f / 255.f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.18f, 0.22f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.075f, 0.075f, 0.09f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.12f, 0.15f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.025f, 0.05f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.48f, 0.035f, 0.09f, 0.85f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.65f, 0.05f, 0.12f, 0.95f));
	ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.055f, 0.055f, 0.07f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.55f, 0.04f, 0.1f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.68f, 0.045f, 0.12f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.95f, 0.18f, 0.28f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.82f, 0.08f, 0.16f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.f, 0.18f, 0.28f, 1.f));

	ImGui::SetNextWindowSize(ImVec2(522.f, 560.f), ImGuiCond_Always);
	if (ImGui::Begin("klotski", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
	{

        static int selected_tab = 0;
        const char* tabs[] = { "combat", "visuals", "movement", "misc", "configs" };
        auto draw = ImGui::GetWindowDrawList();
        const auto pos = ImGui::GetWindowPos();
        const auto size = ImGui::GetWindowSize();
        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(1, 1, 1, 255));
        draw->AddRect(ImVec2(pos.x + 1.f, pos.y + 1.f), ImVec2(pos.x + size.x - 1.f, pos.y + size.y - 1.f), IM_COL32(57, 57, 57, 255));
        draw->AddRect(ImVec2(pos.x + 5.f, pos.y + 5.f), ImVec2(pos.x + size.x - 5.f, pos.y + size.y - 5.f), IM_COL32(57, 57, 57, 255));
        draw->AddRectFilled(ImVec2(pos.x + 2.f, pos.y + 2.f), ImVec2(pos.x + size.x - 2.f, pos.y + size.y - 2.f), IM_COL32(35, 35, 35, 255));
        draw->AddRectFilled(ImVec2(pos.x + 5.f, pos.y + 5.f), ImVec2(pos.x + size.x - 5.f, pos.y + size.y - 5.f), IM_COL32(57, 57, 57, 255));
        draw->AddRectFilled(ImVec2(pos.x + 6.f, pos.y + 6.f), ImVec2(pos.x + size.x - 6.f, pos.y + size.y - 6.f), IM_COL32(4, 4, 4, 255));
		// add leftandright
        const float middle = pos.x + size.x * 0.5f;
        ImGui::SetCursorPos(ImVec2(20.f, 16.f));
        const float tab_width = (482.f - 15.f) / 5.f;
        for (int i = 0; i < 5; ++i)
        {
            if (i) ImGui::SameLine(0.f, 3.f);
            ImGui::PushID(i);
            const auto tab_pos = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("##tab", ImVec2(tab_width, 20.f))) selected_tab = i;
            const auto text_size = ImGui::CalcTextSize(tabs[i]);
            draw->AddText(ImVec2(tab_pos.x + (tab_width - text_size.x) * 0.5f, tab_pos.y),
                selected_tab == i ? IM_COL32(220, 220, 220, 255) : IM_COL32(153, 153, 153, 255), tabs[i]);
            draw->AddRectFilled(ImVec2(tab_pos.x, tab_pos.y + 14.f), ImVec2(tab_pos.x + tab_width, tab_pos.y + 18.f),
                selected_tab == i ? IM_COL32(181, 0, 41, 255) : ImGui::IsItemHovered() ? IM_COL32(90, 0, 20, 255) : IM_COL32(36, 36, 36, 255));
            draw->AddRectFilled(ImVec2(tab_pos.x, tab_pos.y + 16.f), ImVec2(tab_pos.x + tab_width, tab_pos.y + 18.f), IM_COL32(36, 36, 36, 127));
            ImGui::PopID();
        }
        draw->AddRectFilled(ImVec2(pos.x + 20.f, pos.y + 40.f), ImVec2(pos.x + 502.f, pos.y + 540.f), IM_COL32(1, 1, 1, 255));
        draw->AddRect(ImVec2(pos.x + 21.f, pos.y + 41.f), ImVec2(pos.x + 501.f, pos.y + 539.f), IM_COL32(44, 44, 44, 255));
        draw->AddRectFilled(ImVec2(pos.x + 22.f, pos.y + 42.f), ImVec2(pos.x + 500.f, pos.y + 538.f), IM_COL32(9, 9, 9, 255));
        ImGui::SetCursorPos(ImVec2(30.f, 60.f));
        if (ImGui::BeginTable("##panels", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX, ImVec2(460.f, 470.f)))
        {
            ImGui::TableSetupColumn("##left", ImGuiTableColumnFlags_WidthFixed, 230.f);
            ImGui::TableSetupColumn("##right", ImGuiTableColumnFlags_WidthFixed, 230.f);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
			if (selected_tab == 0)
			{
				menu_column("aimbot");
                menu_group("aimbot");
				bind_checkbox("aimbot", "aimbot", &config::context.aimbot);
                if (config::context.aimbot) {
					menu_slider_float("fov", &config::context.aimbot_fov, 1.f, 180.f, "%.1f");
					menu_slider_float("max distance", &config::context.aimbot_max_distance, 0.f, 5000.f, "%.0f");
					menu_checkbox("visible only", &config::context.onlyvisible);
					multi_select_combo("aim bones", aim_bones_table, config::context.aim_bones);
					ImGui::Spacing();
					menu_checkbox("smooth", &config::context.aimbot_smooth);

					if (config::context.aimbot_smooth)
					{
						menu_slider_float("smooth speed", &config::context.aimbot_speed, 1.f, 100.f, "%.0f");
					}

					ImGui::Spacing();
                }
                menu_column_end();
                ImGui::TableSetColumnIndex(1);
                menu_column("combat_right");
                menu_group("recoil control");
                menu_checkbox("rcs", &config::context.standalone_rcs);
					if (config::context.standalone_rcs)
					{
						menu_slider_int("strength", &config::context.standalone_rcs_strength, 0, 100);
						menu_slider_int("rcs min", &config::context.standalone_rcs_min, 0, 100);
						menu_slider_int("rcs max", &config::context.standalone_rcs_max, 0, 100);
					}
                menu_column_end();
			}

			if (selected_tab == 1)
			{
				menu_column("player esp / chams");
                menu_group("players");
				bind_checkbox("players", "players", &config::context.player_esp);
				
				if (config::context.player_esp) {
                    menu_group("player esp");
					menu_checkbox("teammates", &config::context.teammate_esp);
					ImGui::Spacing();
                    ImGui::TextDisabled("box");

					menu_checkbox("draw box", &config::context.draw_box);
                    inline_color("draw box color", config::context.esp_box_color, &config::context.esp_opacity);

					if (config::context.draw_box)
					{
						menu_checkbox("corner", &config::context.box_corner);

						if (config::context.box_corner)
						{
							menu_slider_float("corner size", &config::context.box_corner_size, 1.f, 50.f, "%.0f%%");
						}

						menu_slider_float("thickness", &config::context.box_thickness, 1.f, 6.f, "%.0f");
						menu_checkbox("outline", &config::context.box_outline);
						menu_checkbox("fill", &config::context.box_fill);

						if (config::context.box_fill)
						{
							menu_slider_float("fill alpha", &config::context.box_fill_alpha, 0.01f, 1.f, "%.2f");
						}
					}

					ImGui::Spacing();
                    ImGui::TextDisabled("elements");

					menu_checkbox("name", &config::context.draw_name);
                    inline_color("name color", config::context.esp_name_color, &config::context.esp_opacity);
					menu_checkbox("health", &config::context.draw_health);
                    inline_color("health color", config::context.esp_health_color, &config::context.esp_opacity);
					if (config::context.draw_health)
					{
						menu_checkbox("smooth health", &config::context.health_smooth);
						menu_checkbox("gradient health", &config::context.health_gradient);
						menu_checkbox("custom health color", &config::context.health_custom_color);
						menu_checkbox("health text", &config::context.health_text);
						if (config::context.health_text)
						{
							menu_combo("health text position", &config::context.health_text_position, "below box\0beside bar\0");
							menu_checkbox("hp suffix", &config::context.health_suffix);
						}
					}
					menu_checkbox("weapon", &config::context.draw_weapon);
                    inline_color("weapon color", config::context.esp_weapon_color, &config::context.esp_opacity);
					menu_checkbox("ammo bar", &config::context.draw_ammo_bar);
                    inline_color("ammo bar color", config::context.esp_ammo_color, &config::context.esp_opacity);
					if (config::context.draw_ammo_bar) menu_checkbox("smooth ammo", &config::context.ammo_smooth);
					menu_checkbox("skeleton", &config::context.draw_skeleton);
                    inline_color("skeleton color", config::context.esp_skeleton_color, &config::context.esp_opacity);
					menu_checkbox("flags", &config::context.draw_flags);
                    inline_color("flags color", config::context.esp_flags_color, &config::context.esp_opacity);
					menu_checkbox("out of fov arrow", &config::context.out_of_fov_arrows);
                    inline_color("out of fov arrow color", config::context.out_of_fov_arrow_color, &config::context.out_of_fov_arrow_opacity);
					menu_checkbox("hitmarker", &config::context.hitmarker);
                    inline_color("hitmarker color", config::context.hitmarker_color);

					if (config::context.draw_flags)
					{
						multi_select_combo("flags list", esp_flags_table, config::context.esp_flags);
						menu_combo("flags position", &config::context.esp_flags_position, flag_positions, IM_ARRAYSIZE(flag_positions));
					}

					if (config::context.out_of_fov_arrows)
					{
						menu_slider_float("arrow radius", &config::context.out_of_fov_arrow_radius, 40.f, 600.f, "%.0f");
						menu_slider_float("arrow size", &config::context.out_of_fov_arrow_size, 6.f, 32.f, "%.0f");
						menu_slider_float("arrow width", &config::context.out_of_fov_arrow_width, 0.2f, 1.f, "%.2f");
						menu_slider_float("arrow opacity", &config::context.out_of_fov_arrow_opacity, 0.f, 1.f, "%.2f");
						menu_checkbox("arrow outline", &config::context.out_of_fov_arrow_outline);
						menu_checkbox("arrow pulse", &config::context.out_of_fov_arrow_pulse);
					}

					if (config::context.hitmarker)
					{
						menu_slider_float("hitmarker time", &config::context.hitmarker_duration, 0.05f, 1.5f, "%.2f");
						menu_slider_float("hitmarker size", &config::context.hitmarker_size, 2.f, 32.f, "%.0f");
						menu_slider_float("hitmarker gap", &config::context.hitmarker_gap, 0.f, 24.f, "%.0f");
						menu_slider_float("hitmarker thickness", &config::context.hitmarker_thickness, 1.f, 6.f, "%.1f");
					}

					ImGui::Spacing();
                    ImGui::TextDisabled("colors");
					ImGui::PushID("esp_colors");
                    menu::color_edit4("enemy", config::context.esp_colors, &config::context.esp_opacity);
					menu_checkbox("by health", &config::context.esp_color_by_health);
					menu_slider_float("opacity", &config::context.esp_opacity, 0.05f, 1.f, "%.2f");
                    ImGui::PopID();

					ImGui::Spacing();
                    ImGui::TextDisabled("distance");

					menu_slider_float("max", &config::context.esp_max_distance, 0.f, 3000.f, "%.0f");
					menu_checkbox("fade", &config::context.esp_distance_fade);

					if (config::context.esp_distance_fade)
					{
						menu_checkbox("color by distance", &config::context.esp_color_by_distance);
					}

					
				menu_group_end();
                }

				menu_column_end();
                ImGui::TableSetColumnIndex(1);
                menu_column("visuals_right");
                menu_group("materials / camera");
                menu_checkbox("chams", &config::context.chams);
				

				if (config::context.chams) {
                    menu_group("chams");
					menu_checkbox("visible", &config::context.chams_visible);
                    inline_color("visible color", config::context.chams_visible_color, &config::context.esp_opacity);

					if (config::context.chams_visible)
					{
						menu_combo("visible material", &config::context.chams_material, chams_materials, IM_ARRAYSIZE(chams_materials));
					}

					ImGui::Spacing();
					menu_checkbox("invisible", &config::context.chams_invisible);
                    inline_color("invisible color", config::context.chams_invisible_color, &config::context.esp_opacity);

					if (config::context.chams_invisible)
					{
						menu_combo("invisible material", &config::context.chams_invisible_material, chams_materials, IM_ARRAYSIZE(chams_materials));
					}
				menu_group_end();
                }

				menu_checkbox("glow", &config::context.glow);
                    inline_color("glow color", config::context.glow_color, &config::context.glow_alpha);
				if (config::context.glow) {
					menu_group("glow"); {
						menu_checkbox("invincible", &config::context.glow_invincible);
                    inline_color("invincible color", config::context.glow_color_invincible, &config::context.glow_alpha);

						if (config::context.glow_invincible)
						{
						}

						menu_slider_float("brightness", &config::context.glow_brightness, 0.1f, 3.f, "%.2f");
						menu_slider_float("alpha", &config::context.glow_alpha, 0.1f, 1.f, "%.2f");
					}
				menu_group_end();
                }

				menu_checkbox("fov changer", &config::context.fov_changer);
				if (config::context.fov_changer) {
					menu_group("camera"); {
						menu_slider_float("value", &config::context.fov, 60.f, 150.f, "%.1f");
						menu_checkbox("scopefov", &config::context.scopefov);
					}
				menu_group_end();
                }

				menu_checkbox("viewmodel", &config::context.viewmodel);
				if (config::context.viewmodel) {
					menu_group("viewmodel");
					{
						menu_slider_float("viewmodel fov", &config::context.viewmodel_fov, 40.f, 120.f, "%.0f");
						menu_slider_float("x", &config::context.viewmodel_x, -20.f, 20.f, "%.1f");
						menu_slider_float("y", &config::context.viewmodel_y, -20.f, 20.f, "%.1f");
						menu_slider_float("z", &config::context.viewmodel_z, -20.f, 20.f, "%.1f");
					}
				menu_group_end();
                }
				menu_column_end();
			}


			if (selected_tab == 2) {
				menu_column("movement");
                menu_group("movement");
				menu_checkbox("bhop", &config::context.bhop);
				bind_checkbox("ej", "ej", &config::context.ej);
				bind_checkbox("ladder_ej", "ladder_ej", &config::context.ladder_ej);
				bind_checkbox("ladderbug", "ladderbug", &config::context.ladder_bug);
				bind_checkbox("jumpbug", "jumpbug", &config::context.jumpbug);
				bind_checkbox("pixelsurf", "pixelsurf", &config::context.pixelsurf);
				if (config::context.pixelsurf) menu_checkbox("silent", &config::context.pixelsurf_silent);
                menu_column_end();
                ImGui::TableSetColumnIndex(1);
                menu_column("movement_right");
                menu_group("edgebug / pixelsurf");
                bind_checkbox("edgebug", "edgebug", &config::context.edgebug);
				if (config::context.edgebug) {
					menu_group("edgebug");
					menu_slider_int("ticks", &config::context.edgebug_ticks, 1, 128);
					menu_checkbox("lock", &config::context.eb_lock);
					bind_checkbox("visualize", "visualize", &config::context.visualize_edgebug);
					if (config::context.visualize_edgebug)
					{
						menu::color_edit4("eb path", config::context.edgebug_path_color);
						menu_combo("shape", &config::context.edgebug_visual_shape, "box\0circle\0");
					}
				menu_group_end();
                }
				menu_column_end();
			}

			if (selected_tab == 3) {
				menu_column("misc");
                menu_group("indicators");
				menu_checkbox("watermark", &config::context.watermark);
				menu_checkbox("spectator list", &config::context.spec_list);
                    inline_color("spectator list color", config::context.spec_name_color);
				if (config::context.spec_list) { 
					menu_checkbox("hide empty spectators", &config::context.spec_list_hide_empty);
				};
				if (config::context.watermark) {
                    menu_group("watermark");
					menu_slider_float("y", &config::context.watermark_y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
				menu_group_end();
                }
				menu_checkbox("velocity", &config::context.velocity);
				if (config::context.velocity) {
                    menu_group("velocity");
					menu_slider_float("bottom offset", &config::context.velo_y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
					menu_slider_float("size", &config::context.velo_size, 8.f, 48.f, "%.0f");
					menu_checkbox("pre speed", &config::context.velo_pre_speed);
					menu_checkbox("fade by speed", &config::context.velo_fade);
					menu_checkbox("gradient color", &config::context.velo_gradient);
					if (menu_button("reset velocity colors"))
					{
						const config::config_t defaults{};
						for (int i = 0; i < 5; i++)
							for (int channel = 0; channel < 3; channel++)
								config::context.velo_colors[i][channel] = defaults.velo_colors[i][channel];
						config::context.velo_gradient = false;
						config::context.velo_fade = false;
					}
					if (config::context.velo_gradient)
					{
						menu::color_edit4("low speed", config::context.velo_colors[0]);
						menu::color_edit4("high speed", config::context.velo_colors[1]);
					}
					else
					{
						menu::color_edit4("steady", config::context.velo_colors[2]);
						menu::color_edit4("slowing", config::context.velo_colors[3]);
						menu::color_edit4("accelerating", config::context.velo_colors[4]);
					}
				menu_group_end();
                }

				menu_checkbox("keystrokes", &config::context.keystrokes); 
				if (config::context.keystrokes) {
                    menu_group("keystrokes");
					// should be vector + combo but later
					menu_checkbox("Show in menu", &config::context.show_in_menu);
					menu_checkbox("Show crouch", &config::context.show_crouch);
					menu_checkbox("Show jump", &config::context.show_in_jump);
					menu_slider_float("x", &config::context.keystroke_pos.x, 0.f, static_cast<float>(sdk::screen_width), "%.0f");
					menu_slider_float("y", &config::context.keystroke_pos.y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
					menu_slider_float("size", &config::context.keystrokes_size, 16.f, 48.f, "%.0f");
				menu_group_end();
                }

				menu_column_end();
                ImGui::TableSetColumnIndex(1);
                menu_column("misc_right");
                menu_group("world");
                menu_checkbox("skybox", &config::context.skybox);
                    inline_color("skybox color", config::context.skybox_color);
				if (config::context.skybox) {
                    menu_group("skybox");
					menu_slider_float("intensity", &config::context.skybox_intensity, 0.f, 5.f, "%.2f");
				menu_group_end();
                }
				menu_checkbox("grenade trail", &config::context.grenade_trail);
				menu_checkbox("practice", &config::context.practice);
				if (config::context.practice) {
                    menu_group("practice");
					practice_keybind("save checkpoint", config::context.practice_save_key);
					practice_keybind("load checkpoint", config::context.practice_load_key);
					ImGui::BeginDisabled(!config::context.practice);
					if (menu_button("save")) misc::practice_request(false);
					if (menu_button("load")) misc::practice_request(true);
					ImGui::EndDisabled();
					
				menu_group_end();
                }
				menu_column_end();
			}
			
			if (selected_tab == 4)
			{
				menu_column("configs");
                menu_group("configs");
				static bool needs_refresh = true;

				if (needs_refresh)
				{
					refresh_config_files();
					needs_refresh = false;
				}

				if (ImGui::BeginListBox("##configs", ImVec2(190.f, 112.f)))
				{
					for (int i = 0; i < static_cast<int>(config_files.size()); i++)
					{
						if (ImGui::Selectable(config_files[i].c_str(), selected_config == i))selected_config = i;
					}

					ImGui::EndListBox();
				}

				if (menu_button("load", ImVec2(150.f, 20.f)) && selected_config >= 0)
				{
					config::load(config_files[selected_config].c_str());
				}
				

				if (menu_button("save", ImVec2(150.f, 20.f)) && selected_config >= 0)
				{
					config::save(config_files[selected_config].c_str());
				}
				

				if (menu_button("delete", ImVec2(150.f, 20.f)) && selected_config >= 0)
				{
					config::remove(config_files[selected_config].c_str());
					refresh_config_files();
				}
				//menu::color_edit4("accent", accent);
				menu_column_end();
                ImGui::TableSetColumnIndex(1);
                menu_column("create config");
                menu_group("create config");
				ImGui::TextUnformatted("name");
                ImGui::InputText("##config_name", config_name_buffer, sizeof(config_name_buffer));

				if (menu_button("create", ImVec2(150.f, 20.f)))
				{
					if (config_name_buffer[0])
					{
						config::save(config_name_buffer);
						refresh_config_files();
					}
				}
				menu_column_end();
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor(14);
	ImGui::PopStyleVar(7);
	if (menu::font) ImGui::PopFont();
}


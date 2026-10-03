#include "renderer.hpp"

#include <cfloat>
#include <string>

#include "../imgui/imgui.h"

void renderer::rect(float x, float y, float w, float h, color_t color)
{
	ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color.dump());
}

void renderer::line(float x1, float y1, float x2, float y2, color_t color)
{
	ImGui::GetBackgroundDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color.dump(), 1.5f);
}

void renderer::filled_rect(float x, float y, float w, float h, color_t color)
{
	ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color.dump());
}

void renderer::circle(float x, float y, float r, color_t color, int num_segments, float thickness)
{
	ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x, y), r, color.dump(), num_segments, thickness);
}

static ImFont* indicator_font()
{
	ImFont* font = ImGui::GetIO().Fonts->Fonts[1];

	return font ? font : ImGui::GetIO().Fonts->Fonts[0];
}

static void draw_text(float x, float y, color_t color, const char* text, const renderer::text_options_t& options)
{
	if (!text || !text[0]) return;

	ImFont* font = options.font ? options.font : indicator_font();

	if (!font) return;

	const float font_size = options.font_size > 0.f ? options.font_size : font->FontSize;

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	ImVec2 position(x, y);

	if (options.centered)
	{
		const ImVec2 measured = font->CalcTextSizeA(font_size, FLT_MAX, 0.f, text);

		position.x -= measured.x * 0.5f;
		position.y -= measured.y * 0.5f;
	}

	if (options.outline)
	{
		static constexpr float offsets[8][2] = {
			{ -1.f,  0.f }, { 1.f,  0.f }, { 0.f, -1.f }, { 0.f,  1.f },
			{ -1.f, -1.f }, { 1.f, -1.f }, { -1.f, 1.f }, { 1.f,  1.f }
		};

		for (const auto& offset : offsets)
		{
			draw_list->AddText(font, font_size, ImVec2(position.x + offset[0], position.y + offset[1]), color_t::black().dump(), text);
		}
	}
	else if (options.drop_shadow)
	{
		draw_list->AddText(font, font_size, ImVec2(position.x + 1.f, position.y + 1.f), color_t::black().dump(), text);
	}

	draw_list->AddText(font, font_size, position, color.dump(), text);
}

void renderer::text(float x, float y, float size, color_t color, const char* text, const text_options_t& options)
{
	text_options_t sized_options = options;
	sized_options.font_size = size;

	draw_text(x, y, color, text, sized_options);
}

void renderer::text_centered(float x, float y, float w, float h, color_t color, const char* text, const text_options_t& options)
{
	const ImVec2 text_size = ImGui::CalcTextSize(text);
	const ImVec2 text_position = { (x + (x + w) - text_size.x) / 2.f, y - text_size.y - 4.f };

	draw_text(text_position.x, text_position.y, color, text, options);
}
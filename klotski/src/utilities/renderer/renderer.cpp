#include "renderer.hpp"

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

void renderer::circle(float x, float y, float r,  color_t color, int num_segments, float thickness) {
	ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x, y), r, color.dump(), num_segments, thickness);
}

static ImFont* indicator_font()
{
	ImFont* font = ImGui::GetIO().Fonts->Fonts[1];

	return font ? font : ImGui::GetIO().Fonts->Fonts[0];
}

void renderer::text(float x, float y, color_t color, const char* text)
{
	ImFont* font = indicator_font();

	ImGui::GetBackgroundDrawList()->AddText(font, font->FontSize, ImVec2(x + 1.f, y + 1.f), color_t::black().dump(), text);
	ImGui::GetBackgroundDrawList()->AddText(font, font->FontSize, ImVec2(x, y), color.dump(), text);
}

void renderer::text_sized(float x, float y, float size, color_t color, const char* text)
{
	ImFont* font = indicator_font();

	ImGui::GetBackgroundDrawList()->AddText(font, size, ImVec2(x + 1.f, y + 1.f), color_t::black().dump(), text);
	ImGui::GetBackgroundDrawList()->AddText(font, size, ImVec2(x, y), color.dump(), text);
}

void renderer::text_centered(float x, float y, float w, float h, color_t color, const char* text)
{
	ImVec2 text_size = ImGui::CalcTextSize(text);
	ImVec2 text_position = { (x + (x + w) - text_size.x) / 2.f, y - text_size.y - 4.f };

	ImGui::GetBackgroundDrawList()->AddText(ImVec2(text_position.x + 1.f, text_position.y + 1.f), color_t::black().dump(), text);
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(text_position.x, text_position.y), color.dump(), text);
}

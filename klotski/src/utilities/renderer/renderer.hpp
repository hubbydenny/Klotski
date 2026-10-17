#pragma once

#include <string>

#include "../../source2-sdk/math/math.hpp"

struct ImFont;

namespace renderer
{
	inline ImFont* velocity_fonts[49]{};
	struct text_options_t
	{
		float font_size = 0.f;
		ImFont* font = nullptr;
		bool centered = false;
		bool drop_shadow = true;
		bool outline = false;
	};

	inline ImFont* visuals_fonts[49]{};
	inline vec2_t is_left{ 12.f, 12.f }; inline vec2_t is_right{ -12.f, 12.f }; inline vec2_t is_down_left{}; inline vec2_t is_down_right{}; inline vec2_t is_up{}; inline vec2_t is_down{};

	void rect(float x, float y, float w, float h, color_t color);
	void line(float x1, float y1, float x2, float y2, color_t color);
	void line(float x1, float y1, float x2, float y2, color_t color, float thickness);
	void filled_rect(float x, float y, float w, float h, color_t color);
	void filled_triangle(float x1, float y1, float x2, float y2, float x3, float y3, color_t color);
	void circle(float x, float y, float r, color_t color, int num_segments, float thickness);

	void text(float x, float y, float size, color_t color, const char* text, const text_options_t& options = {});
	void text_centered(float x, float y, float w, float h, color_t color, const char* text, const text_options_t& options = {});
}

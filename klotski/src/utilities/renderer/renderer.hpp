#pragma once

#include "../../source2-sdk/math/math.hpp"
#include "../../source2-sdk/math/math.hpp"

namespace renderer
{
	void rect(float x, float y, float w, float h, color_t color);
	void line(float x1, float y1, float x2, float y2, color_t color);
	void filled_rect(float x, float y, float w, float h, color_t color);
	void circle(float x, float y, float r, color_t color, int num_segments, float thickness);
	//void triangle(float x, float y, float w, float h, color_t color);
	void text(float x, float y, color_t color, const char* text);
	void text_sized(float x, float y, float size, color_t color, const char* text);
	void text_centered(float x, float y, float w, float h, color_t color, const char* text);
}
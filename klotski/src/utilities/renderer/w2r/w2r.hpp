#pragma once

#include <span>
#include "../../../source2-sdk/math/math.hpp"

namespace w2r
{
	int line(const vec3_t& start, const vec3_t& end, color_t color, float thickness = 1.5f);
	int polyline(std::span<const vec3_t> points, color_t color, float thickness = 1.5f, bool closed = false);
	int box(const vec3_t& center, float half_size, color_t color, float thickness = 1.5f);
	int box(const vec3_t& origin, const vec3_t& mins, const vec3_t& maxs, color_t color, float thickness = 1.5f);
	int circle(const vec3_t& center, float radius, color_t color, int segments = 64, float thickness = 1.5f);
}

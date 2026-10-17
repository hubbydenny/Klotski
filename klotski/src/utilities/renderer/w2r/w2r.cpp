#include "w2r.hpp"
#include "../renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

static bool finite_point(const vec3_t& point)
{
	return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

int w2r::line(const vec3_t& start, const vec3_t& end, color_t color, float thickness)
{
	if (!finite_point(start) || !finite_point(end) || !std::isfinite(thickness) || thickness <= 0.f) return 0;
	vec3_t world_start = start, world_end = end;
	vec3_t screen_start{}, screen_end{};
	if (!math::world_to_screen(world_start, screen_start) || !math::world_to_screen(world_end, screen_end)) return 0;
	if (!std::isfinite(screen_start.x) || !std::isfinite(screen_start.y) || !std::isfinite(screen_end.x) || !std::isfinite(screen_end.y)) return 0;
	renderer::line(screen_start.x, screen_start.y, screen_end.x, screen_end.y, color, thickness);
	return 1;
}

int w2r::polyline(std::span<const vec3_t> points, color_t color, float thickness, bool closed)
{
	if (points.size() < 2) return 0;
	int drawn = 0;
	for (std::size_t i = 1; i < points.size(); i++) drawn += line(points[i - 1], points[i], color, thickness);
	if (closed && points.size() > 2) drawn += line(points.back(), points.front(), color, thickness);
	return drawn;
}

int w2r::box(const vec3_t& center, float half_size, color_t color, float thickness)
{
	if (!std::isfinite(half_size) || half_size <= 0.f) return 0;
	const std::array<vec3_t, 4> corners = {{
		{ center.x - half_size, center.y - half_size, center.z },
		{ center.x - half_size, center.y + half_size, center.z },
		{ center.x + half_size, center.y + half_size, center.z },
		{ center.x + half_size, center.y - half_size, center.z }
	}};
	return polyline(corners, color, thickness, true);
}

int w2r::box(const vec3_t& origin, const vec3_t& mins, const vec3_t& maxs, color_t color, float thickness)
{
	if (!finite_point(origin) || !finite_point(mins) || !finite_point(maxs)) return 0;
	if (mins.x > maxs.x || mins.y > maxs.y || mins.z > maxs.z) return 0;
	std::array<vec3_t, 8> corners{};
	for (int i = 0; i < 8; i++)
		corners[i] = { origin.x + ((i & 1) ? maxs.x : mins.x), origin.y + ((i & 2) ? maxs.y : mins.y), origin.z + ((i & 4) ? maxs.z : mins.z) };
	int drawn = 0;
	for (int i = 0; i < 8; i++)
		for (int axis = 1; axis <= 4; axis <<= 1)
			if (!(i & axis)) drawn += line(corners[i], corners[i | axis], color, thickness);
	return drawn;
}

int w2r::circle(const vec3_t& center, float radius, color_t color, int segments, float thickness)
{
	if (!finite_point(center) || !std::isfinite(radius) || radius <= 0.f) return 0;
	segments = std::clamp(segments, 3, 256);
	vec3_t previous(center.x + radius, center.y, center.z);
	const vec3_t first = previous;
	int drawn = 0;
	for (int i = 1; i <= segments; i++)
	{
		const float angle = 2.f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
		const vec3_t point = i == segments ? first : vec3_t(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius, center.z);
		drawn += line(previous, point, color, thickness);
		previous = point;
	}
	return drawn;
}

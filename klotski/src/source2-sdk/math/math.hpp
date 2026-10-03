#pragma once

#include <cstdint>

class vec2_t
{
public:
	float x;
	float y;

	vec2_t();
	vec2_t(float x, float y);
	~vec2_t();

	vec2_t operator+(vec2_t& other);
};

class vec3_t
{
public:
	float x;
	float y;
	float z;

	vec3_t();
	vec3_t(float x, float y, float z);
	~vec3_t();

	vec3_t operator+(const vec3_t& other) const;
	vec3_t operator-(vec3_t& other);
	vec3_t& operator+=(vec3_t& other);
	vec3_t operator*(float scale) const;

	void clamp();
	bool is_zero() const;
};

class vec4_t
{
public:
	float x;
	float y;
	float z;
	float w;

	vec4_t();
	vec4_t(float x, float y, float z, float w);
	~vec4_t();
};

class color_t
{
public:
	std::uint8_t r, g, b, a;

	color_t();
	color_t(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255);
	~color_t();

	std::uint32_t dump();

	static color_t black(std::uint8_t a = 255);
	static color_t white(std::uint8_t a = 255);
	static color_t red(std::uint8_t a = 255);
	static color_t green(std::uint8_t a = 255);
	static color_t blue(std::uint8_t a = 255);
	static color_t yellow(std::uint8_t a = 255);
};

namespace math
{
	bool world_to_screen(vec3_t& origin, vec3_t& screen);
	vec3_t calculate_angle(const vec3_t& source, const vec3_t& destination, const vec3_t& view_angles);
	float angle_distance(const vec3_t& source, const vec3_t& destination, const vec3_t& view_angles);
}
float normalize_angle(float angle);
float normalize_yaw(float yaw);
float vel2d(const vec3_t & velocity);
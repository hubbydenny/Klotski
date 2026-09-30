#include "math.hpp"

#include "../../signatures.hpp"
#include "../../utilities/utilities.hpp"
#include "../../source2-sdk/sdk.hpp"

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <cmath>

#define deg_to_rad(x)  ((float)(x) * (float)(M_PI / 180.f))
#define rad_to_deg(x)  ((float)(x) * (float)(180.f / M_PI))


vec2_t::vec2_t()
{
	this->x = 0.f;
	this->y = 0.f;
}

vec2_t::vec2_t(float x, float y)
{
	this->x = x;
	this->y = y;
}

vec2_t::~vec2_t()
{

}

vec2_t vec2_t::operator+(vec2_t& other)
{
	return vec2_t(x + other.x, y + other.y);
}


vec3_t::vec3_t()
{
	this->x = 0.f;
	this->y = 0.f;
	this->z = 0.f;
}

vec3_t::vec3_t(float x, float y, float z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

vec3_t::~vec3_t()
{

}

vec3_t vec3_t::operator+(const vec3_t& other) const
{
	return vec3_t(this->x + other.x, this->y + other.y, this->z + other.z);
}

vec3_t vec3_t::operator-(vec3_t& other)
{
	return vec3_t(this->x - other.x, this->y - other.y, this->z - other.z);
}

vec3_t& vec3_t::operator+=(vec3_t& other)
{
	this->x += other.x; this->y += other.y; this->z += other.z;
	return *this;
}

vec3_t vec3_t::operator*(float scale) const
{
	return vec3_t(this->x * scale, this->y * scale, this->z * scale);
}

void vec3_t::clamp()
{
	this->x = (std::clamp)(x, -89.0f, 89.0f);
	this->y = (std::clamp)(std::remainder(y, 360.0f), -180.0f, 180.0f);
	this->z = (std::clamp)(z, -50.0f, 50.0f);
}

bool vec3_t::is_zero() const
{
	if (this->x == 0 && this->y == 0 && this->z == 0)
	{
		return true;
	}

	return false;
}


vec4_t::vec4_t()
{
	this->x = 0.f;
	this->y = 0.f;
	this->z = 0.f;
	this->w = 0.f;
}

vec4_t::vec4_t(float x, float y, float z, float w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

vec4_t::~vec4_t()
{

}


color_t::color_t()
{
	this->r = 0;
	this->g = 0;
	this->b = 0;
	this->a = 0;
}

color_t::color_t(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha)
{
	this->r = red;
	this->g = green;
	this->b = blue;
	this->a = alpha;
}

color_t::~color_t()
{

}

std::uint32_t color_t::dump()
{
	std::uint32_t out = 0;

	out = static_cast<std::uint32_t>(this->r) << 0;
	out |= static_cast<std::uint32_t>(this->g) << 8;
	out |= static_cast<std::uint32_t>(this->b) << 16;
	out |= static_cast<std::uint32_t>(this->a) << 24;

	return out;
}

color_t color_t::black(std::uint8_t a)
{
	return { 0, 0, 0, a };
}

color_t color_t::white(std::uint8_t a)
{
	return { 255, 255, 255, a };
}

color_t color_t::red(std::uint8_t a)
{
	return { 255, 0, 0, a };
}

color_t color_t::green(std::uint8_t a)
{
	return { 0, 255, 0, a };
}

color_t color_t::blue(std::uint8_t a)
{
	return { 0, 0, 255, a };
}

color_t color_t::yellow(std::uint8_t a)
{
	return { 247, 202, 24, a };
}


bool math::world_to_screen(vec3_t& origin, vec3_t& screen)
{
	using function_t = bool(__fastcall*)(vec3_t&, vec3_t&);
	static function_t function = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", WORLD_TO_SCREEN));

	if (!function) return false;
    bool status = !function(origin, screen);

    screen.x = static_cast<float>((screen.x + 1.0) * 0.5) * sdk::screen_width;
    screen.y = sdk::screen_height - (static_cast<float>((screen.y + 1.0) * 0.5) * sdk::screen_height);

    return status;
}

static float normalize_angle(float angle)
{
	angle = std::fmodf(angle, 360.f);

	if (angle > 180.f)
	{
		angle -= 360.f;
	}
	else if (angle < -180.f)
	{
		angle += 360.f;
	}

	return angle;
}

vec3_t math::calculate_angle(const vec3_t& source, const vec3_t& destination, const vec3_t& view_angles)
{
	vec3_t trace(destination.x - source.x, destination.y - source.y, destination.z - source.z);
	const float flat_distance = std::hypotf(trace.x, trace.y);
	vec3_t angles;

	angles.x = flat_distance == 0.f ? view_angles.x : rad_to_deg(std::atan2f(-trace.z, flat_distance));
	angles.y = flat_distance == 0.f ? view_angles.y : rad_to_deg(std::atan2f(trace.y, trace.x));
	angles.z = 0.f;

	angles.x = normalize_angle(angles.x - view_angles.x);
	angles.y = normalize_angle(angles.y - view_angles.y);

	return angles;
}

float math::angle_distance(const vec3_t& source, const vec3_t& destination, const vec3_t& view_angles)
{
	vec3_t delta = calculate_angle(source, destination, view_angles);

	return std::hypotf(delta.x, delta.y);
}

float normalize_yaw(float yaw)
{
	yaw = std::fmodf(yaw + 180.0f, 360.0f);
	if (yaw < 0.0f) yaw += 360.0f;
	return yaw - 180.0f;
}
float vel2d(const vec3_t & velocity) {return std::sqrtf(velocity.x * velocity.x + velocity.y * velocity.y);}
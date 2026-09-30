#pragma once

#include <cstddef>
#include <cstdint>
#include "entities.hpp"
#include "../math/math.hpp"

typedef struct _ray_t
{
	vec3_t start;
	vec3_t end;
	vec3_t mins;
	vec3_t maxs;
	char pad1[0x4];
	std::uint8_t type;
} ray_t;

static_assert(sizeof(ray_t) == 0x38, "ray_t has wrong size");

class hitbox_t
{
public:
	const char* name;
	const char* surface_property;
	const char* bone_name;
	vec3_t mins;
	vec3_t maxs;
	float shape_radius;
	std::uint32_t bone_name_hash;
	std::int32_t group_id;
	std::uint8_t shape_type;
	bool translation_only;
	char pad1[0x2];
	std::uint32_t crc;
	std::uint32_t collision_attribute_index;
	std::uint16_t hitbox_index;
	char pad2[0x6];
};

static_assert(sizeof(hitbox_t) == 0x50, "hitbox_t has wrong size");

typedef struct _trace_filter_t
{
	std::byte data[0x48];
} trace_filter_t;

static_assert(sizeof(trace_filter_t) == 0x48, "trace_filter_t has wrong size");

typedef struct _game_trace_t
{
	void* surface;
	player_t* player;
	hitbox_t* hitbox;
	char pad1[0x38];
	std::uint32_t surface_flags;
	char pad2[0x24];
	vec3_t startpos;
	vec3_t endpos;
	vec3_t normal;
	char pad3[0xC];
	char pad4[0x4];
	float fraction;
	char pad5[0xB];
	bool start_solid;
	char pad6[0x4];
} game_trace_t;

static_assert(sizeof(game_trace_t) == 0xC0, "game_trace_t has wrong size");

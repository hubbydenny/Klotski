#pragma once

#include <cstddef>
#include <cstdint>

#include "../math/math.hpp"
#include "../schema_system/schema_system.hpp"

typedef struct alignas(16) _bone_data_t
{
	vec3_t position;
	float scale;
	vec3_t rotation;
} bone_data_t;

static_assert(sizeof(bone_data_t) == 0x20, "bone_data_t has wrong size");

class model_t
{
public:
	char pad1[0x130 + 0x38];
	const char** bone_names;
	std::uint32_t bone_count;
};

static_assert(offsetof(model_t, bone_names) == 0x168, "model_t::bone_names has wrong offset");

class model_state_t
{
public:
	char pad1[0x80];
	bone_data_t* bones;
	char pad2[0x4];
	std::uint32_t bone_count;

	SCHEMA("CModelState", "m_hModel", model, std::uint64_t);
	SCHEMA("CModelState", "m_ModelName", model_name, const char*);
};

static_assert(offsetof(model_state_t, bones) == 0x80, "model_state_t::bones has wrong offset");
static_assert(offsetof(model_state_t, bone_count) == 0x8C, "model_state_t::bone_count has wrong offset");

class skeleton_instance_t
{
public:
	SCHEMA("CSkeletonInstance", "m_modelState", model_state, model_state_t);
	SCHEMA("CSkeletonInstance", "m_nHitboxSet", hitbox_set, std::uint8_t);
};

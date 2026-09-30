#pragma once

#include <cstddef>
#include <cstdint>

#include "../math/math.hpp"
#include "../math/math.hpp"
#include "../schema_system/schema_system.hpp"
#include "skeleton.hpp"
#include "types.hpp"

class entity_instance_t;

class glow_property_t
{
public:
	SCHEMA("CGlowProperty", "m_iGlowType", glow_type, std::int32_t);
	SCHEMA("CGlowProperty", "m_iGlowTeam", glow_team, std::int32_t);
	SCHEMA("CGlowProperty", "m_nGlowRange", glow_range, std::int32_t);
	SCHEMA("CGlowProperty", "m_nGlowRangeMin", glow_range_min, std::int32_t);
	SCHEMA("CGlowProperty", "m_glowColorOverride", glow_color, color_t);
	SCHEMA("CGlowProperty", "m_bFlashing", flashing, bool);
	SCHEMA("CGlowProperty", "m_flGlowTime", glow_time, float);
	SCHEMA("CGlowProperty", "m_flGlowStartTime", glow_start_time, float);
	SCHEMA("CGlowProperty", "m_bEligibleForScreenHighlight", eligible_for_screen_highlight, bool);
	SCHEMA("CGlowProperty", "m_bGlowing", glowing, bool);

	entity_instance_t* owner()
	{
		return *reinterpret_cast<entity_instance_t**>(reinterpret_cast<std::uintptr_t>(this) + 0x18);
	}
};

class collision_property_t
{
public:
	SCHEMA("CCollisionProperty", "m_vecMins", mins, vec3_t);
	SCHEMA("CCollisionProperty", "m_vecMaxs", maxs, vec3_t);
	SCHEMA("CCollisionProperty", "m_usSolidFlags", solid_flags, std::uint16_t);
	SCHEMA("CCollisionProperty", "m_nSolidType", solid_type, std::uint8_t);
	SCHEMA("CCollisionProperty", "m_triggerBloat", trigger_bloat, std::uint8_t);
	SCHEMA("CCollisionProperty", "m_nSurroundType", surround_type, std::uint8_t);
	SCHEMA("CCollisionProperty", "m_flRadius", radius, float);
	SCHEMA("CCollisionProperty", "m_vecSpecifiedSurroundingMins", specified_surrounding_mins, vec3_t);
	SCHEMA("CCollisionProperty", "m_vecSpecifiedSurroundingMaxs", specified_surrounding_maxs, vec3_t);

	std::uint16_t get_collision_mask()
	{
		return *reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uintptr_t>(this) + 0x38);
	}
};

class game_scene_node_t
{
public:
	SCHEMA("CGameSceneNode", "m_pOwner", owner, void*);
	SCHEMA("CGameSceneNode", "m_pChild", child, game_scene_node_t*);
	SCHEMA("CGameSceneNode", "m_pNextSibling", next_sibling, game_scene_node_t*);
	SCHEMA("CGameSceneNode", "m_vecAbsOrigin", abs_origin, vec3_t);
	SCHEMA("CGameSceneNode", "m_bDormant", dormant, bool);

	skeleton_instance_t* skeleton_instance()
	{
		return reinterpret_cast<skeleton_instance_t*>(this);
	}
};

class entity_identity_t
{
public:
	std::uintptr_t base_entity;
	char pad1[0x8];
	entity_handle_t handle;
	char pad2[0x4];

	SCHEMA("CEntityIdentity", "m_name", name, const char*);
	SCHEMA("CEntityIdentity", "m_designerName", designer_name, const char*);
};

class entity_instance_t
{
public:
	SCHEMA("CEntityInstance", "m_pEntity", entity_identity, entity_identity_t*);
};

class entity_spotted_state_t
{
public:
	SCHEMA("EntitySpottedState_t", "m_bSpotted", spotted, bool);
};

class econ_item_view_t
{
public:
	SCHEMA("C_EconItemView", "m_bRestoreCustomMaterialAfterPrecache", restore_custom_material, bool);
	SCHEMA("C_EconItemView", "m_iItemDefinitionIndex", item_definition_index, std::uint16_t);
	SCHEMA("C_EconItemView", "m_iItemID", item_id, std::uint64_t);
	SCHEMA("C_EconItemView", "m_iItemIDLow", item_id_low, std::uint32_t);
	SCHEMA("C_EconItemView", "m_iItemIDHigh", item_id_high, std::uint32_t);
	SCHEMA("C_EconItemView", "m_iAccountID", account_id, std::uint32_t);
	SCHEMA("C_EconItemView", "m_bInitialized", initialized, bool);
	SCHEMA("C_EconItemView", "m_bDisallowSOC", disallow_soc, bool);
};

class attribute_container_t
{
public:
	SCHEMA("C_AttributeContainer", "m_Item", item, econ_item_view_t*);
};

class entity_t : public entity_instance_t
{
public:
	SCHEMA("C_BaseEntity", "m_pGameSceneNode", game_scene_node, game_scene_node_t*);
	SCHEMA("C_BaseEntity", "m_pCollision", collision_property, collision_property_t*);
	SCHEMA("C_BaseEntity", "m_iMaxHealth", max_health, std::int32_t);
	SCHEMA("C_BaseEntity", "m_iHealth", health, std::int32_t);
	SCHEMA("C_BaseEntity", "m_iTeamNum", team, std::uint8_t);
	SCHEMA("C_BaseEntity", "m_fFlags", flags, std::uint32_t);
	SCHEMA("C_BaseEntity", "m_MoveType", move_type, movetype_t);
	SCHEMA("C_BaseEntity", "m_hOwnerEntity", owner_handle, entity_handle_t);
	SCHEMA("C_BaseEntity", "m_nSubclassID", subclass_id, std::uint32_t);
	SCHEMA("C_BaseEntity", "m_flSimulationTime", simulation_time, float);
	SCHEMA("C_BaseEntity", "m_hGroundEntity", ground_entity, entity_handle_t);
	SCHEMA("C_BaseEntity", "m_vecBaseVelocity", base_velocity, vec3_t);
	SCHEMA("C_BaseEntity", "m_MoveCollide", move_collide, std::uint8_t);

	bool is_alive()
	{
		return this->health() > 0;
	}

	bool has_flag(std::uint32_t flag)
	{
		static const std::uint32_t offset = schema_system::get_schema("C_BaseEntity", "m_fFlags");

		return offset != 0 && (this->flags() & flag) != 0;
	}

	vec3_t get_origin()
	{
		game_scene_node_t* scene_node = this->game_scene_node();

		if (!scene_node)
		{
			return vec3_t();
		}

		return scene_node->abs_origin();
	}

	const char* get_entity_name()
	{
		entity_identity_t* identity = this->entity_identity();

		return identity ? identity->name() : nullptr;
	}

	const char* get_designer_name()
	{
		entity_identity_t* identity = this->entity_identity();

		return identity ? identity->designer_name() : nullptr;
	}
};

class model_entity_t : public entity_t
{
public:
	SCHEMA("C_BaseModelEntity", "m_vecViewOffset", view_offset, vec3_t);
	SCHEMA("C_BaseModelEntity", "m_Collision", collision, collision_property_t);
	SCHEMA("C_BaseModelEntity", "m_Glow", glow, glow_property_t);
};

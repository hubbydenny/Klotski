#pragma once

#include <cstdint>

class user_cmd_t;

#include "../../signatures.hpp"
#include "../../utilities/utilities.hpp"
#include "../../utilities/vfunc.hpp"
#include "../math/math.hpp"
#include "entity.hpp"
#include "skeleton.hpp"
#include "types.hpp"
#include "weapons.hpp"

class player_weapon_services_t
{
public:
	SCHEMA("CPlayer_WeaponServices", "m_hActiveWeapon", active_weapon, entity_handle_t);
	SCHEMA("CPlayer_WeaponServices", "m_hMyWeapons", weapons, utl_vector_t<entity_handle_t>);
};

class cs_player_weapon_services_t : public player_weapon_services_t
{
public:
};

class player_item_services_t
{
public:
};

class cs_player_item_services_t : public player_item_services_t
{
public:
	SCHEMA("CCSPlayer_ItemServices", "m_bHasDefuser", has_defuser, bool);
	SCHEMA("CCSPlayer_ItemServices", "m_bHasHelmet", has_helmet, bool);
};

class player_observer_services_t
{
public:
	SCHEMA("CPlayer_ObserverServices", "m_iObserverMode", observer_mode, std::uint8_t);
	SCHEMA("CPlayer_ObserverServices", "m_hObserverTarget", observer_target, entity_handle_t);
};

class player_camera_services_t
{
public:
	SCHEMA("CPlayer_CameraServices", "m_vecCsViewPunchAngle", view_punch_angle, vec3_t);
	SCHEMA("CPlayer_CameraServices", "m_hActivePostProcessingVolume", active_post_processing_volume, entity_handle_t);
};

class cs_player_base_camera_services_t : public player_camera_services_t
{
public:
	SCHEMA("CCSPlayerBase_CameraServices", "m_iFOV", fov, std::uint32_t);
};

class CCSPlayerModernJump { // templeware
public:
	SCHEMA("CCSPlayerModernJump", "m_flLastLandedFrac", m_flLastLandedFrac, float);
};


class player_movement_services_t
{
public:
	SCHEMA("CPlayer_MovementServices", "m_nQueuedButtonDownMask", queued_button_down_mask, std::uint64_t);
	SCHEMA("CPlayer_MovementServices", "m_nQueuedButtonChangeMask", queued_button_change_mask, std::uint64_t)

	SCHEMA("CPlayer_MovementServices", "m_nButtonDoublePressed", buttondoublepressed, std::uint64_t);
	SCHEMA("CPlayer_MovementServices", "m_nToggleButtonDownMask", toggle_button_down_mask, std::uint64_t);

	SCHEMA("CPlayer_MovementServices", "m_nLastCommandNumberProcessed", last_command_number_processed, std::uint32_t);
	SCHEMA("CPlayer_MovementServices", "m_flMaxSpeed", max_speed, float);

	SCHEMA("CPlayer_MovementServices_Humanoid", "m_flSurfaceFriction", surface_friction, float);

	SCHEMA("CCSPlayer_MovementServices", "m_nButtonDownMaskPrev", button_down_mask_prev, std::uint64_t);
	SCHEMA("CCSPlayer_MovementServices", "m_flCmdForwardMove", cmd_forward_mv, std::uint64_t);
	SCHEMA("CCSPlayer_MovementServices", "m_ModernJump", modern_jump, CCSPlayerModernJump);
	//SCHEMA_ADD_OFFSET(CInButtonStatePB, m_button_state, 0x50);

	std::uint64_t trace_mask() const
	{
		return *reinterpret_cast<const std::uint64_t*>(reinterpret_cast<const std::uint8_t*>(this) + 0xd48);
	}

	std::uint32_t trace_state() const
	{
		return *reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const std::uint8_t*>(this) + 0x3f8);
	}

	void set_prediction_command(user_cmd_t* cmd)
	{
		utilities::call_virtual<46U, void>(this, cmd);
	}

	void run_command(user_cmd_t* cmd)
	{
		utilities::call_virtual<32U, void>(this, cmd);
	}

	void reset_prediction_command()
	{
		utilities::call_virtual<47U, void>(this);
	}
};

class base_player_pawn_t : public model_entity_t
{
public:
	SCHEMA("C_BasePlayerPawn", "m_pMovementServices", movement_services, player_movement_services_t*);
	SCHEMA("C_BasePlayerPawn", "m_pWeaponServices", weapon_services, cs_player_weapon_services_t*);
	SCHEMA("C_BasePlayerPawn", "m_pItemServices", item_services, cs_player_item_services_t*);
	SCHEMA("C_BasePlayerPawn", "m_pObserverServices", observer_services, player_observer_services_t*);
	SCHEMA("C_BasePlayerPawn", "m_pCameraServices", camera_services, cs_player_base_camera_services_t*);
	SCHEMA("C_BasePlayerPawn", "m_vOldOrigin", old_origin, vec3_t);
	SCHEMA("C_BasePlayerPawn", "m_hController", controller_handle, entity_handle_t);
	SCHEMA("C_BasePlayerPawn", "m_flFOVSensitivityAdjust", fov_sensitivity_adjust, float);
};

class cs_player_pawn_base_t : public base_player_pawn_t
{
public:
	SCHEMA("C_CSPlayerPawnBase", "m_flFlashBangTime", flash_bang_time, float);
	SCHEMA("C_CSPlayerPawnBase", "m_flFlashMaxAlpha", flash_max_alpha, float);
	SCHEMA("C_CSPlayerPawnBase", "m_flFlashDuration", flash_duration, float);
	SCHEMA("C_CSPlayerPawnBase", "m_flLastSpawnTimeIndex", last_spawn_time_index, float);
};

class player_t : public cs_player_pawn_base_t
{
public:
	SCHEMA("C_CSPlayerPawn", "m_ArmorValue", armor_value, std::int32_t);
	SCHEMA("C_CSPlayerPawn", "m_bGunGameImmunity", gun_immunity, bool);
	SCHEMA("C_CSPlayerPawn", "m_angEyeAngles", eye_angles, vec3_t);
	SCHEMA("C_CSPlayerPawn", "m_iShotsFired", shots_fired, std::int32_t);
	SCHEMA("C_CSPlayerPawn", "m_bIsDefusing", is_defusing, bool);
	SCHEMA("C_CSPlayerPawn", "m_bIsScoped", is_scoped, bool);
	SCHEMA("C_CSPlayerPawn", "m_bWaitForNoAttack", wait_for_no_attack, bool);
	SCHEMA("C_CSPlayerPawn", "m_bNeedToReApplyGloves", need_to_reapply_gloves, bool);
	SCHEMA("C_CSPlayerPawn", "m_entitySpottedState", spotted_state, entity_spotted_state_t);
	SCHEMA("C_CSPlayerPawn", "m_EconGloves", econ_gloves, econ_item_view_t);
	SCHEMA("C_CSPlayerPawn", "m_hHudModelArms", hud_model_arms, entity_handle_t);
	SCHEMA("C_CSPlayerPawn", "m_flDetectedByEnemySensorTime", detected_by_enemy_sensor_time, float);
	SCHEMA("C_BaseEntity", "m_vecVelocity", velocity, vec3_t);
	SCHEMA("C_BaseEntity", "m_vecAbsVelocity", abs_velocity, vec3_t);

	vec3_t get_eye_position()
	{
		game_scene_node_t* scene_node = this->game_scene_node();

		if (!utilities::is_valid_pointer(scene_node))
		{
			return vec3_t();
		}

		vec3_t position = scene_node->abs_origin();
		vec3_t offset = this->view_offset();

		position.x += offset.x;
		position.y += offset.y;
		position.z += offset.z;

		return position;
	}

	vec3_t get_aim_punch()
	{
		cs_player_base_camera_services_t* services = this->camera_services();

		if (!utilities::is_valid_pointer(services)) return vec3_t();
		return services->view_punch_angle();
	}

	bool has_defuser()
	{
		cs_player_item_services_t* services = this->item_services();
		return utilities::is_valid_pointer(services) && services->has_defuser();
	}

	bool has_helmet()
	{
		cs_player_item_services_t* services = this->item_services();
		return utilities::is_valid_pointer(services) && services->has_helmet();
	}

	bool has_gun_immunity()
	{
		static const std::uint32_t offset = schema_system::get_schema("C_CSPlayerPawn", "m_bGunGameImmunity");

		return offset != 0 && this->gun_immunity();
	}

	bool has_armor(std::int32_t hitgroup)
	{
		if (hitgroup == 1) 	return this->has_helmet();
		return this->armor_value() > 0;
	}

	std::int32_t get_bone_index(const char* bone_name)
	{
		using function_t = std::int32_t(__fastcall*)(player_t*, const char*);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", GET_BONE_ID_BY_NAME));

		if (!fn || !bone_name) return -1;
		const std::int32_t bone_index = fn(this, bone_name);

		return bone_index >= 0 && bone_index <= 127 ? bone_index : -1;
	}

	void update_bones()
	{
		game_scene_node_t* scene_node = this->game_scene_node();

		if (!utilities::is_valid_pointer(scene_node)) return;
		

		using function_t = void(__fastcall*)(skeleton_instance_t*, std::uint32_t);
		static function_t update_bones_fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", CALC_WORLD_SPACE_BONES));

		if (update_bones_fn)
		{
			update_bones_fn(scene_node->skeleton_instance(), 0xFFFFF);
		}
	}

	bone_data_t* get_bone_cache(std::int32_t* out_count)
	{
		if (out_count)
		{
			*out_count = 0;
		}

		game_scene_node_t* scene_node = this->game_scene_node();

		if (!utilities::is_valid_pointer(scene_node))
		{
			return nullptr;
		}

		model_state_t& model_state = scene_node->skeleton_instance()->model_state();

		bone_data_t* bones = model_state.bones;
		const std::int32_t count = static_cast<std::int32_t>(model_state.bone_count);

		if (!utilities::is_readable(bones, sizeof(bone_data_t) * count))
		{
			return nullptr;
		}

		if (out_count)
		{
			*out_count = count;
		}

		return bones;
	}

	vec3_t get_bone_position(std::int32_t bone_index)
	{
		if (bone_index < 0 || bone_index > 127)	return vec3_t();
		

		this->update_bones();

		game_scene_node_t* scene_node = this->game_scene_node();

		if (!utilities::is_valid_pointer(scene_node))	return vec3_t();
		

		bone_data_t* bones = scene_node->skeleton_instance()->model_state().bones;

		if (!utilities::is_readable(bones, sizeof(bone_data_t) * (bone_index + 1)))	return vec3_t();
		return bones[bone_index].position;
	}

	vec3_t get_bone_position(const char* bone_name)
	{
		return get_bone_position(get_bone_index(bone_name));
	}
};

class base_player_controller_t : public entity_t
{
public:
	SCHEMA("CBasePlayerController", "m_nTickBase", tick_base, std::uint32_t);
	SCHEMA("CBasePlayerController", "m_hPawn", pawn_handle, entity_handle_t);
	SCHEMA("CBasePlayerController", "m_steamID", steam_id, std::uint64_t);
	SCHEMA("CBasePlayerController", "m_bIsLocalPlayerController", is_local_player_controller, bool);
};

class in_game_money_services_t
{
public:
	SCHEMA("CCSPlayerController_InGameMoneyServices", "m_iAccount", account, std::int32_t);
};

class inventory_services_t
{
public:
	SCHEMA("CCSPlayerController_InventoryServices", "m_unMusicID", music_id, std::uint16_t);
};

class controller_t : public base_player_controller_t
{
public:
	SCHEMA("CCSPlayerController", "m_pInGameMoneyServices", money_services, in_game_money_services_t*);
	SCHEMA("CCSPlayerController", "m_pInventoryServices", inventory_services, inventory_services_t*);
	SCHEMA("CCSPlayerController", "m_sSanitizedPlayerName", name, const char*);
	SCHEMA("CCSPlayerController", "m_bPawnIsAlive", pawn_is_alive, bool);
};

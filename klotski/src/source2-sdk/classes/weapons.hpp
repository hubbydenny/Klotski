#pragma once

#include <cstdint>

#include "../../signatures.hpp"
#include "../../utilities/utilities.hpp"
#include "entity.hpp"
#include "types.hpp"

class base_player_weapon_vdata_t
{
public:
	SCHEMA("CBasePlayerWeaponVData", "m_iMaxClip1", max_clip, std::int32_t);
};

class cs_weapon_base_vdata_t : public base_player_weapon_vdata_t
{
public:
	SCHEMA("CCSWeaponBaseVData", "m_WeaponType", weapon_type, std::int32_t);
	SCHEMA("CCSWeaponBaseVData", "m_szName", name, const char*);
	SCHEMA("CCSWeaponBaseVData", "m_nDamage", damage, std::int32_t);
	SCHEMA("CCSWeaponBaseVData", "m_flHeadshotMultiplier", headshot_multiplier, float);
	SCHEMA("CCSWeaponBaseVData", "m_flArmorRatio", armor_ratio, float);
	SCHEMA("CCSWeaponBaseVData", "m_flPenetration", penetration, float);
	SCHEMA("CCSWeaponBaseVData", "m_flRange", range, float);
	SCHEMA("CCSWeaponBaseVData", "m_flRangeModifier", range_modifier, float);
	SCHEMA("CCSWeaponBaseVData", "m_flCycleTime", cycle_time, float);
	SCHEMA("CCSWeaponBaseVData", "m_flSpread", spread, float);
	SCHEMA("CCSWeaponBaseVData", "m_flInaccuracyStand", inaccuracy_stand, float);
	SCHEMA("CCSWeaponBaseVData", "m_flInaccuracyMove", inaccuracy_move, float);
	SCHEMA("CCSWeaponBaseVData", "m_flMaxSpeed", max_speed, float);
	SCHEMA("CCSWeaponBaseVData", "m_nNumBullets", num_bullets, std::int32_t);
	SCHEMA("CCSWeaponBaseVData", "m_flRecoilMagnitude", recoil_magnitude, float);
};

class econ_entity_t : public entity_t
{
public:
	SCHEMA("C_EconEntity", "m_AttributeManager", attribute_manager, attribute_container_t);
	SCHEMA("C_EconEntity", "m_OriginalOwnerXuidLow", original_owner_xuid_low, std::uint32_t);
	SCHEMA("C_EconEntity", "m_OriginalOwnerXuidHigh", original_owner_xuid_high, std::uint32_t);
	SCHEMA("C_EconEntity", "m_nFallbackPaintKit", fallback_paint_kit, std::int32_t);
	SCHEMA("C_EconEntity", "m_nFallbackSeed", fallback_seed, std::int32_t);
	SCHEMA("C_EconEntity", "m_flFallbackWear", fallback_wear, float);

	std::uint64_t get_original_owner_xuid()
	{
		return (static_cast<std::uint64_t>(this->original_owner_xuid_high()) << 32) | this->original_owner_xuid_low();
	}

	cs_weapon_base_vdata_t* get_vdata();
};

class base_player_weapon_t : public econ_entity_t
{
public:
	SCHEMA("C_BasePlayerWeapon", "m_nNextPrimaryAttackTick", next_primary_attack_tick, std::int32_t);
	SCHEMA("C_BasePlayerWeapon", "m_nNextSecondaryAttackTick", next_secondary_attack_tick, std::int32_t);
	SCHEMA("C_BasePlayerWeapon", "m_iClip1", clip, std::int32_t);
};

class cs_weapon_base_t : public base_player_weapon_t
{
public:
	SCHEMA("C_CSWeaponBase", "m_bInReload", in_reload, bool);
	SCHEMA("C_CSWeaponBase", "m_bBurstMode", burst_mode, bool);
	SCHEMA("C_CSWeaponBase", "m_iOriginalTeamNumber", original_team_number, std::int32_t);
};

class cs_weapon_base_gun_t : public cs_weapon_base_t
{
public:
	SCHEMA("C_CSWeaponBaseGun", "m_iBurstShotsRemaining", burst_shots_remaining, std::int32_t);
};

class c4_t : public cs_weapon_base_t
{
public:
	SCHEMA("C_C4", "m_bStartedArming", started_arming, bool);
	SCHEMA("C_C4", "m_bBombPlacedAnimation", bomb_placed_animation, bool);
	SCHEMA("C_C4", "m_bBombPlanted", bomb_planted, bool);
};

class smoke_grenade_projectile_t : public entity_t
{
public:
	SCHEMA("C_SmokeGrenadeProjectile", "m_bDidSmokeEffect", did_smoke_effect, bool);
	SCHEMA("C_SmokeGrenadeProjectile", "m_vSmokeColor", smoke_color, vec3_t);
};

class planted_c4_t : public model_entity_t
{
public:
	SCHEMA("C_PlantedC4", "m_nBombSite", bomb_site, std::int32_t);
	SCHEMA("C_PlantedC4", "m_flC4Blow", c4_blow, float);
	SCHEMA("C_PlantedC4", "m_bHasExploded", has_exploded, bool);
	SCHEMA("C_PlantedC4", "m_bBeingDefused", being_defused, bool);
	SCHEMA("C_PlantedC4", "m_bBombDefused", bomb_defused, bool);
};

inline cs_weapon_base_vdata_t* econ_entity_t::get_vdata()
{
	using function_t = void* (__fastcall*)(void*);
	static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", GET_VDATA));

	if (!fn) return nullptr;

	attribute_container_t* container = &this->attribute_manager();

	if (!utilities::is_valid_pointer(container)) return nullptr;

	econ_item_view_t* item = container->item();

	if (!utilities::is_valid_pointer(item)) return nullptr;

	void* vdata = fn(item);

	return utilities::is_valid_pointer(vdata) ? reinterpret_cast<cs_weapon_base_vdata_t*>(vdata) : nullptr;
}

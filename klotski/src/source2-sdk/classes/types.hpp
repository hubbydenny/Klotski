#pragma once

#include <array>
#include <cstdint>

typedef std::uint32_t entity_handle_t;

constexpr entity_handle_t invalid_entity_handle = 0xFFFFFFFF;
constexpr std::uint32_t entity_handle_entry_mask = 0x7FFF;

constexpr std::uint32_t handle_index(entity_handle_t handle)
{
	return handle & entity_handle_entry_mask;
}

constexpr bool handle_is_valid(entity_handle_t handle)
{
	return handle != invalid_entity_handle;
}

enum buttons_t : std::uint64_t
{
	in_attack = 1ull << 0,
	in_jump = 1ull << 1,
	in_duck = 1ull << 2,
	in_forward = 1ull << 3,
	in_back = 1ull << 4,
	in_use = 1ull << 5,
	in_cancel = 1ull << 6,
	in_turnleft = 1ull << 7,
	in_turnright = 1ull << 8,
	in_moveleft = 1ull << 9,
	in_moveright = 1ull << 10,
	in_attack2 = 1ull << 11,
	in_run = 1ull << 12,
	in_reload = 1ull << 13,
	in_alt1 = 1ull << 14,
	in_alt2 = 1ull << 15,
	in_score = 1ull << 16,
	in_speed = 1ull << 17,
	in_walk = 1ull << 18,
	in_zoom = 1ull << 19,
	in_weapon1 = 1ull << 20,
	in_weapon2 = 1ull << 21,
	in_bullrush = 1ull << 22,
	in_grenade1 = 1ull << 23,
	in_grenade2 = 1ull << 24,
	in_attack3 = 1ull << 25
};

enum movetype_t : std::uint32_t
{
	movetype_none = 0,
	movetype_isometric,
	movetype_walk,
	movetype_step,
	movetype_fly,
	movetype_flygravity,
	movetype_vphysics,
	movetype_push,
	movetype_noclip,
	movetype_ladder,
	movetype_observer,
	movetype_custom,
	movetype_last = movetype_custom,
	movetype_max_bits = 4
};

enum flags_t : std::uint32_t
{
	fl_onground = (1 << 0),
	fl_ducking = (1 << 1),
	fl_waterjump = (1 << 2),
	fl_ontrain = (1 << 3),
	fl_inrain = (1 << 4),
	fl_frozen = (1 << 5),
	fl_atcontrols = (1 << 6),
	fl_client = (1 << 7),
	fl_fakeclient = (1 << 8),
	fl_inwater = (1 << 9),
	fl_fly = (1 << 10),
	fl_swim = (1 << 11),
	fl_conveyor = (1 << 12),
	fl_npc = (1 << 13),
	fl_godmode = (1 << 14),
	fl_notarget = (1 << 15),
	fl_aimtarget = (1 << 16),
	fl_partialground = (1 << 17),
	fl_staticprop = (1 << 18),
	fl_graphed = (1 << 19),
	fl_grenade = (1 << 20),
	fl_stepmovement = (1 << 21),
	fl_donttouch = (1 << 22),
	fl_basevelocity = (1 << 23),
	fl_worldbrush = (1 << 24),
	fl_object = (1 << 25),
	fl_killme = (1 << 26),
	fl_onfire = (1 << 27),
	fl_dissolving = (1 << 28),
	fl_transragdoll = (1 << 29),
	fl_unblockable_by_player = (1 << 30)
};

enum bone_ids : std::uint32_t
{
	head = 7u,
	neck = 6u,
	spine_1 = 2u,
	spine_2 = 3u,
	spine_3 = 4u,
	spine_4 = 23u,
	pelvis = 1u,

	left_clavicle = 8u,
	left_shoulder = 9u,
	left_elbow = 10u,
	left_hand = 11u,

	right_clavicle = 12u,
	right_shoulder = 13u,
	right_elbow = 14u,
	right_hand = 15u,

	left_hip = 17u,
	left_knee = 18u,
	left_foot = 19u,

	right_hip = 20u,
	right_knee = 21u,
	right_foot = 22u,
};
constexpr bone_ids bone_none = static_cast<bone_ids>(~0u);
constexpr std::uint32_t bone_slot_count = 7;

struct bone_chain_t
{
	std::array<bone_ids, bone_slot_count> bones;
};

inline constexpr std::array<bone_chain_t, 5> bone_chains =
{{
	{{ bone_ids::head, bone_ids::neck, bone_ids::spine_4, bone_ids::spine_3, bone_ids::spine_2, bone_ids::spine_1, bone_ids::pelvis }},
	{{ bone_ids::left_hand, bone_ids::left_elbow, bone_ids::left_shoulder, bone_ids::left_clavicle, bone_ids::spine_4, bone_none, bone_none }},
	{{ bone_ids::right_hand, bone_ids::right_elbow, bone_ids::right_shoulder, bone_ids::right_clavicle, bone_ids::spine_4, bone_none, bone_none }},
	{{ bone_ids::left_foot, bone_ids::left_knee, bone_ids::left_hip, bone_ids::pelvis, bone_none, bone_none, bone_none }},
	{{ bone_ids::right_foot, bone_ids::right_knee, bone_ids::right_hip, bone_ids::pelvis, bone_none, bone_none, bone_none }},
}};

constexpr std::uint64_t CONTENTS_EMPTY = 0ull;
constexpr std::uint64_t CONTENTS_SOLID = 1ull << 0;
constexpr std::uint64_t CONTENTS_HITBOX = 1ull << 1;
constexpr std::uint64_t CONTENTS_TRIGGER = 1ull << 2;
constexpr std::uint64_t CONTENTS_SKY = 1ull << 3;

constexpr std::uint64_t CONTENTS_PLAYER_CLIP = 1ull << 4;
constexpr std::uint64_t CONTENTS_NPC_CLIP = 1ull << 5;
constexpr std::uint64_t CONTENTS_BLOCK_LOS = 1ull << 6;
constexpr std::uint64_t CONTENTS_BLOCK_LIGHT = 1ull << 7;
constexpr std::uint64_t CONTENTS_LADDER = 1ull << 8;
constexpr std::uint64_t CONTENTS_PICKUP = 1ull << 9;
constexpr std::uint64_t CONTENTS_BLOCK_SOUND = 1ull << 10;
constexpr std::uint64_t CONTENTS_NODRAW = 1ull << 11;
constexpr std::uint64_t CONTENTS_WINDOW = 1ull << 12;
constexpr std::uint64_t CONTENTS_PASS_BULLETS = 1ull << 13;
constexpr std::uint64_t CONTENTS_WORLD_GEOMETRY = 1ull << 14;
constexpr std::uint64_t CONTENTS_WATER = 1ull << 15;
constexpr std::uint64_t CONTENTS_SLIME = 1ull << 16;
constexpr std::uint64_t CONTENTS_TOUCH_ALL = 1ull << 17;
constexpr std::uint64_t CONTENTS_PLAYER = 1ull << 18;
constexpr std::uint64_t CONTENTS_NPC = 1ull << 19;
constexpr std::uint64_t CONTENTS_DEBRIS = 1ull << 20;
constexpr std::uint64_t CONTENTS_PHYSICS_PROP = 1ull << 21;
constexpr std::uint64_t CONTENTS_NAV_IGNORE = 1ull << 22;
constexpr std::uint64_t CONTENTS_NAV_LOCAL_IGNORE = 1ull << 23;
constexpr std::uint64_t CONTENTS_POST_PROCESSING_VOLUME = 1ull << 24;
constexpr std::uint64_t CONTENTS_UNUSED_LAYER3 = 1ull << 25;
constexpr std::uint64_t CONTENTS_CARRIED_OBJECT = 1ull << 26;
constexpr std::uint64_t CONTENTS_PUSHAWAY = 1ull << 27;
constexpr std::uint64_t CONTENTS_SERVER_ENTITY_ON_CLIENT = 1ull << 28;
constexpr std::uint64_t CONTENTS_CARRIED_WEAPON = 1ull << 29;
constexpr std::uint64_t CONTENTS_STATIC_LEVEL = 1ull << 30;

constexpr std::uint64_t CONTENTS_CSGO_TEAM1 = 1ull << 31;
constexpr std::uint64_t CONTENTS_CSGO_TEAM2 = 1ull << 32;
constexpr std::uint64_t CONTENTS_CSGO_GRENADE_CLIP = 1ull << 33;
constexpr std::uint64_t CONTENTS_CSGO_DRONE_CLIP = 1ull << 34;
constexpr std::uint64_t CONTENTS_CSGO_MOVEABLE = 1ull << 35;
constexpr std::uint64_t CONTENTS_CSGO_OPAQUE = 1ull << 36;
constexpr std::uint64_t CONTENTS_CSGO_MONSTER = 1ull << 37;
constexpr std::uint64_t CONTENTS_CSGO_UNUSED_LAYER = 1ull << 38;
constexpr std::uint64_t CONTENTS_CSGO_THROWN_GRENADE = 1ull << 39;

enum frame_stage_t : std::int32_t
{
	FRAME_UNDEFINED = -1,
	FRAME_START,
	FRAME_NET_UPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	FRAME_NET_UPDATE_END,
	FRAME_RENDER_START,
	FRAME_RENDER_END
};

enum weapon_type_t : std::int32_t
{
	weapontype_knife = 0,
	weapontype_pistol,
	weapontype_submachinegun,
	weapontype_rifle,
	weapontype_shotgun,
	weapontype_sniperrifle,
	weapontype_machinegun,
	weapontype_c4,
	weapontype_taser,
	weapontype_grenade,
	weapontype_equipment,
	weapontype_stackableitem
};

struct aim_bone_entry_t
{
	bone_ids bone;
	const char* label;
};

inline constexpr aim_bone_entry_t aim_bones_table[] = {
	{ bone_ids::head, "head" },
	{ bone_ids::neck, "neck" },
	{ bone_ids::spine_4, "spine 4" },
	{ bone_ids::spine_3, "spine 3" },
	{ bone_ids::spine_2, "spine 2" },
	{ bone_ids::spine_1, "spine 1" },
	{ bone_ids::pelvis, "pelvis" },
	{ bone_ids::left_clavicle, "left clavicle" },
	{ bone_ids::left_shoulder, "left shoulder" },
	{ bone_ids::left_elbow, "left elbow" },
	{ bone_ids::left_hand, "left hand" },
	{ bone_ids::right_clavicle, "right clavicle" },
	{ bone_ids::right_shoulder, "right shoulder" },
	{ bone_ids::right_elbow, "right elbow" },
	{ bone_ids::right_hand, "right hand" },
	{ bone_ids::left_hip, "left hip" },
	{ bone_ids::left_knee, "left knee" },
	{ bone_ids::left_foot, "left foot" },
	{ bone_ids::right_hip, "right hip" },
	{ bone_ids::right_knee, "right knee" },
	{ bone_ids::right_foot, "right foot" },
};

inline constexpr std::uint32_t aim_bone_count = sizeof(aim_bones_table) / sizeof(aim_bones_table[0]);

enum esp_flag_t : std::uint32_t
{
	esp_flag_armor = 0,
	esp_flag_helmet,
	esp_flag_kit,
	esp_flag_defusing,
	esp_flag_distance,
};

struct esp_flag_entry_t
{
	esp_flag_t flag;
	const char* label;
};

inline constexpr esp_flag_entry_t esp_flags_table[] = {
	{ esp_flag_armor, "armor" },
	{ esp_flag_helmet, "helmet" },
	{ esp_flag_kit, "kit" },
	{ esp_flag_defusing, "defusing" },
	{ esp_flag_distance, "distance" },
};

inline constexpr std::uint32_t esp_flag_count = sizeof(esp_flags_table) / sizeof(esp_flags_table[0]);

struct weapon_name_entry_t
{
	const char* designer_name;
	const char* display_name;
};

inline constexpr weapon_name_entry_t weapon_names[] = {
	{ "weapon_ak47", "AK-47" },
	{ "weapon_aug", "AUG" },
	{ "weapon_awp", "AWP" },
	{ "weapon_bizon", "PP-Bizon" },
	{ "weapon_cz75a", "CZ75-Auto" },
	{ "weapon_deagle", "Desert Eagle" },
	{ "weapon_elite", "Dual Berettas" },
	{ "weapon_famas", "FAMAS" },
	{ "weapon_fiveseven", "Five-SeveN" },
	{ "weapon_g3sg1", "G3SG1" },
	{ "weapon_galilar", "Galil AR" },
	{ "weapon_glock", "Glock-18" },
	{ "weapon_hkp2000", "P2000" },
	{ "weapon_m249", "M249" },
	{ "weapon_m4a1", "M4A4" },
	{ "weapon_m4a1_silencer", "M4A1-S" },
	{ "weapon_mac10", "MAC-10" },
	{ "weapon_mag7", "MAG-7" },
	{ "weapon_mp5sd", "MP5-SD" },
	{ "weapon_mp7", "MP7" },
	{ "weapon_mp9", "MP9" },
	{ "weapon_negev", "Negev" },
	{ "weapon_nova", "Nova" },
	{ "weapon_p250", "P250" },
	{ "weapon_p90", "P90" },
	{ "weapon_revolver", "R8 Revolver" },
	{ "weapon_sawedoff", "Sawed-Off" },
	{ "weapon_scar20", "SCAR-20" },
	{ "weapon_sg556", "SG 553" },
	{ "weapon_ssg08", "SSG 08" },
	{ "weapon_tec9", "Tec-9" },
	{ "weapon_ump45", "UMP-45" },
	{ "weapon_usp_silencer", "USP-S" },
	{ "weapon_xm1014", "XM1014" },
	{ "weapon_knife", "Knife" },
	{ "weapon_knife_t", "Knife" },
	{ "weapon_taser", "Zeus x27" },
	{ "weapon_c4", "C4" },
	{ "weapon_flashbang", "Flashbang" },
	{ "weapon_hegrenade", "HE Grenade" },
	{ "weapon_smokegrenade", "Smoke Grenade" },
	{ "weapon_molotov", "Molotov" },
	{ "weapon_incgrenade", "Incendiary" },
	{ "weapon_decoy", "Decoy" },
};

template <typename t>
class utl_vector_t
{
public:
	std::int32_t size;
	std::int32_t dbg_info;
	t* memory;
	std::int32_t allocation_count;
	std::int32_t grow_size;

	bool is_valid_index(std::int32_t index) const {return index >= 0 && index < size;}
};

static_assert(sizeof(utl_vector_t<std::uint32_t>) == 0x18, "utl_vector_t has wrong size");

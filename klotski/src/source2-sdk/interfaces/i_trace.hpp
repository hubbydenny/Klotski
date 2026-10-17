#pragma once
#include "../../utilities/utilities.hpp"
#include "../classes/trace.hpp"
#include "../math/math.hpp"
#include "../../utilities/signatures.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"

constexpr std::uint64_t MASK_ALL = ~0ull;
constexpr std::uint64_t MASK_SOLID = CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_PLAYER | CONTENTS_NPC | CONTENTS_PASS_BULLETS;
constexpr std::uint64_t MASK_PLAYERSOLID = CONTENTS_SOLID | CONTENTS_PLAYER_CLIP | CONTENTS_WINDOW | CONTENTS_PLAYER | CONTENTS_NPC | CONTENTS_PASS_BULLETS;
constexpr std::uint64_t MASK_NPCSOLID = CONTENTS_SOLID | CONTENTS_NPC_CLIP | CONTENTS_WINDOW | CONTENTS_PLAYER | CONTENTS_NPC | CONTENTS_PASS_BULLETS;
constexpr std::uint64_t MASK_NPCFLUID = CONTENTS_SOLID | CONTENTS_NPC_CLIP | CONTENTS_WINDOW | CONTENTS_PLAYER | CONTENTS_NPC;
constexpr std::uint64_t MASK_WATER = CONTENTS_WATER | CONTENTS_SLIME;
constexpr std::uint64_t MASK_SHOT = CONTENTS_SOLID | CONTENTS_PLAYER | CONTENTS_NPC | CONTENTS_WINDOW | CONTENTS_DEBRIS | CONTENTS_HITBOX;
constexpr std::uint64_t MASK_SHOT_BRUSHONLY = CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_DEBRIS;
constexpr std::uint64_t MASK_SHOT_HULL = CONTENTS_SOLID | CONTENTS_PLAYER | CONTENTS_NPC | CONTENTS_WINDOW | CONTENTS_DEBRIS | CONTENTS_PASS_BULLETS;
constexpr std::uint64_t MASK_SHOT_PORTAL = CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_PLAYER | CONTENTS_NPC;
constexpr std::uint64_t MASK_SOLID_BRUSHONLY = CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_PASS_BULLETS;
constexpr std::uint64_t MASK_PLAYERSOLID_BRUSHONLY = CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_PLAYER_CLIP | CONTENTS_PASS_BULLETS;
constexpr std::uint64_t MASK_NPCSOLID_BRUSHONLY = CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_NPC_CLIP | CONTENTS_PASS_BULLETS;
constexpr std::uint64_t MASK_PLAYER_VISIBLE = CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_PLAYER | CONTENTS_NPC | CONTENTS_DEBRIS | CONTENTS_HITBOX | CONTENTS_BLOCK_LOS;
constexpr std::uint8_t TRACE_LAYER_DEFAULT = 4;
constexpr std::uint8_t TRACE_FILTER_TYPE = 7;
constexpr std::int32_t COLLISION_GROUP_PLAYER_MOVEMENT = 11;

class i_trace
{
public:
	void clip_trace_to_players(vec3_t& start, vec3_t& end, trace_filter_t* filter, game_trace_t* trace, float min, float max, float length)
	{
		using function_t = void(__fastcall*)(vec3_t&, vec3_t&, trace_filter_t*, game_trace_t*, float, float, float);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", CLIP_TRACE_TO_PLAYERS));
		if (!fn) return;
		fn(start, end, filter, trace, min, max, length);
	}

	bool trace_shape(ray_t* ray, vec3_t& start, vec3_t& end, trace_filter_t* filter, game_trace_t* trace) { return trace_shape_world(this, ray, start, end, filter, trace); }
	static bool trace_shape_world(i_trace* world, ray_t* ray, vec3_t& start, vec3_t& end, trace_filter_t* filter, game_trace_t* trace)
	{
		using function_t = bool(__fastcall*)(i_trace*, ray_t*, vec3_t&, vec3_t&, trace_filter_t*, game_trace_t*);
		function_t fn = reinterpret_cast<function_t>(get_trace_shape());

		if (!fn || !world) return false;
		return fn(world, ray, start, end, filter, trace);
	}

	static bool make_filter(trace_filter_t* filter, void* skip_entity, std::uint64_t mask, std::uint8_t layer)
	{
		if (!filter) return false;

		using function_t = void(__fastcall*)(trace_filter_t*, void*, std::uint64_t, std::int32_t, std::int32_t);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", TRACE_FILTER_INIT));

		if (!fn) return false;

		fn(filter, skip_entity, mask, layer, TRACE_FILTER_TYPE);

		return true;
	}

	static bool make_movement_filter(trace_filter_t* filter, void* skip_entity, std::uint64_t mask, std::int32_t collision_group)
	{
		if (!filter) return false;

		using function_t = void(__fastcall*)(trace_filter_t*, void*, std::uint64_t, std::int32_t);
		static function_t fn = []() -> function_t {
			auto match = utilities::pattern_scan(L"client.dll", "00 00 41 B8 00 01 00 00 E8 ? ? ? ? 4C 39 7F 38");
			return match ? reinterpret_cast<function_t>(utilities::resolve_rip(match + 8, 1, 5)) : nullptr;
		}();

		if (!fn) return false;

		fn(filter, skip_entity, mask, collision_group);

		return true;
	}

	bool trace_line(const vec3_t& start, const vec3_t& end, trace_filter_t* filter, game_trace_t* trace)
	{
		ray_t ray = { };
		vec3_t trace_start = start;
		vec3_t trace_end = end;
		ray.start = trace_start;
		ray.end = trace_end;

		return trace_shape(&ray, trace_start, trace_end, filter, trace);
	}

	bool trace_line(const vec3_t& start, const vec3_t& end, void* skip_entity, game_trace_t* trace, std::uint64_t mask = MASK_PLAYER_VISIBLE, std::uint8_t layer = TRACE_LAYER_DEFAULT)
	{
		trace_filter_t filter = { };

		if (!make_filter(&filter, skip_entity, mask, layer)) return false;

		return trace_line(start, end, &filter, trace);
	}

	static std::uint8_t* trace_hull_call_site()
	{
		static std::uint8_t* call_site = utilities::scan_function(L"client.dll", TRACE_HULL_CALL);

		return call_site;
	}

	static void* trace_hull_target()
	{
		static void* target = utilities::pattern_scan(L"client.dll", "48 89 74 24 18 55 57 41 54 41 55 41 56 48 8D AC 24 ? ? ? ? 48 81 EC A0 01 00 00 4C 8B B5");

		return target;
	}

	static bool make_player_movement_filter(trace_filter_t* filter, player_t* player)
	{
		if (!filter || !player) return false;
		using function_t = void(__fastcall*)(trace_filter_t*, void*, std::uint64_t, int);
		static function_t fn = []() -> function_t {
			auto match = utilities::pattern_scan(L"client.dll", "00 00 41 B8 00 01 00 00 E8 ? ? ? ? 4C 39 7F 38");
			return match ? reinterpret_cast<function_t>(utilities::resolve_rip(match + 8, 1, 5)) : nullptr;
		}();
		if (!fn) return false;
		const auto collision = schema_system::get_schema("C_BaseModelEntity", "m_Collision");
		const auto attributes = schema_system::get_schema("CCollisionProperty", "m_collisionAttribute");
		const auto interacts = schema_system::get_schema("VPhysicsCollisionAttribute_t", "m_nInteractsWith");
		if (!collision || !schema_system::has_schema("CCollisionProperty", "m_collisionAttribute")
			|| !schema_system::has_schema("VPhysicsCollisionAttribute_t", "m_nInteractsWith")) return false;
		auto mask = *reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uintptr_t>(player) + collision + attributes + interacts);
		if (player->flags() & 0x10u) mask |= 0x20ull;
		fn(filter, player, mask, 11);
		return true;
	}

	bool trace_hull_player(const vec3_t& start, const vec3_t& end, const vec3_t& hull_mins, const vec3_t& hull_maxs, trace_filter_t* filter, void* movement_services, game_trace_t* trace)
	{
		struct bbox_t { vec3_t mins; vec3_t maxs; };
		struct native_result_t { game_trace_t trace; std::byte tail[0x48]; };
		static_assert(sizeof(native_result_t) == 0x108);
		using function_t = void(__fastcall*)(void*, native_result_t*, const vec3_t*, const vec3_t*, const bbox_t*, trace_filter_t*);

		void* target = trace_hull_target();

		if (!utilities::is_valid_pointer(target)) return false;
		if (!utilities::is_valid_pointer(movement_services)) return false;

		function_t fn = reinterpret_cast<function_t>(target);

		if (!trace || !filter) return false;
		const bbox_t bounds{ hull_mins, hull_maxs };
		native_result_t result{};
		auto mutable_filter = *filter;
		fn(reinterpret_cast<std::uint8_t*>(movement_services) + 0x7B0, &result, &start, &end, &bounds, &mutable_filter);
		*trace = result.trace;

		return true;
	}

	static bool is_ready() { return get_trace_shape() != nullptr; }

	private:
	static void* get_trace_shape()
	{
		static void* trace_shape_address = utilities::scan_function(L"client.dll", TRACE_SHAPE);
		return trace_shape_address;
	}
};

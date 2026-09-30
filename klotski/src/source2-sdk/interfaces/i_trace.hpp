#pragma once
#include "../../utilities/utilities.hpp"
#include "../classes/trace.hpp"
#include "../math/math.hpp"
#include "../../signatures.hpp"
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
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", INIT_PLAYER_MOVEMENT_TRACE_FILTER));

		if (!fn) return make_filter(filter, skip_entity, mask, TRACE_LAYER_DEFAULT);

		fn(filter, skip_entity, mask, collision_group);

		return true;
	}

	bool trace_line(const vec3_t& start, const vec3_t& end, trace_filter_t* filter, game_trace_t* trace)
	{
		ray_t ray = { };
		vec3_t trace_start = start;
		vec3_t trace_end = end;

		return trace_shape(&ray, trace_start, trace_end, filter, trace);
	}

	static std::uint8_t* trace_hull_call_site()
	{
		static std::uint8_t* call_site = utilities::scan_function(L"client.dll", TRACE_HULL_CALL);

		return call_site;
	}

	static void* trace_hull_target()
	{
		static void* target = []() -> void*
		{
			std::uint8_t* call_site = trace_hull_call_site();

			if (!call_site) return nullptr;

			return utilities::resolve_rip(call_site, 1, 5);
		}();

		return target;
	}

	bool trace_hull_player(const vec3_t& start, const vec3_t& end, const vec3_t& hull_mins, const vec3_t& hull_maxs, trace_filter_t* filter, void* movement_services, game_trace_t* trace)
	{
		using function_t = void(__fastcall*)(void*, game_trace_t*, const vec3_t*, const vec3_t*, const vec3_t*, const vec3_t*, trace_filter_t*);

		void* target = trace_hull_target();

		if (!utilities::is_valid_pointer(target)) return false;
		if (!utilities::is_valid_pointer(movement_services)) return false;

		function_t fn = reinterpret_cast<function_t>(target);

		fn(reinterpret_cast<std::uint8_t*>(movement_services) + 1592, trace, &start, &end, &hull_mins, &hull_maxs, filter);

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

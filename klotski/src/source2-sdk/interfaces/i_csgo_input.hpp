#pragma once

#include <cstdint>

#include "../../signatures.hpp"
#include "../../utilities/utilities.hpp"
#include "../classes/user_cmd.hpp"
#include "../math/math.hpp"

class controller_t;

enum csgo_input_vtable
{
	CREATEMOVE = 5,
	MOUSE_INPUT = 10
};

class i_csgo_input
{
public:
	vec3_t* get_view_angles(std::int32_t slot)
	{
		using function_t = vec3_t*(__fastcall*)(i_csgo_input*, std::int32_t);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", GET_VIEW_ANGLES));

		if (!fn)
		{
			return nullptr;
		}

		vec3_t* angles = fn(this, slot);

		return utilities::is_valid_pointer(angles) ? angles : nullptr;
	}

	void set_view_angles(vec3_t& angles)
	{
		using function_t = void(__fastcall*)(i_csgo_input*, std::int32_t, vec3_t&);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", SET_VIEW_ANGLES));

		if (!fn)
		{
			return;
		}

		fn(this, 0, angles);
	}

	user_cmd_t* get_user_cmd(controller_t* controller)
	{
		const std::uint32_t sequence_number = get_sequence_number(controller);

		if (!sequence_number)
		{
			return nullptr;
		}

		using function_t = user_cmd_t*(__fastcall*)(controller_t*, std::uint32_t);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", GET_USER_CMD_BY_SEQUENCE_NUMBER));

		if (!fn)
		{
			return nullptr;
		}

		user_cmd_t* user_cmd = fn(controller, sequence_number);

		return utilities::is_valid_pointer(user_cmd) ? user_cmd : nullptr;
	}

private:
	std::uint32_t get_sequence_number(controller_t* controller)
	{
		if (!controller)
		{
			return 0;
		}

		using tick_fn_t = void(__fastcall*)(controller_t*, std::int32_t*);
		using array_fn_t = void*(__fastcall*)(void**, std::int32_t);

		static tick_fn_t get_tick = reinterpret_cast<tick_fn_t>(utilities::scan_function(L"client.dll", GET_USER_CMD_TICK));
		static array_fn_t get_array = reinterpret_cast<array_fn_t>(utilities::scan_function(L"client.dll", GET_USER_CMD_ARRAY));
		static void** first_user_cmd_array = get_first_user_cmd_array();

		if (!get_tick || !get_array || !first_user_cmd_array)
		{
			return 0;
		}

		std::int32_t tick = 0;
		get_tick(controller, &tick);

		tick = tick == -1 ? -1 : tick - 1;

		void* user_cmd_array = get_array(first_user_cmd_array, tick);

		if (!user_cmd_array)
		{
			return 0;
		}

		return *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uintptr_t>(user_cmd_array) + 0x5910);
	}

	static void** get_first_user_cmd_array()
	{
		std::uint8_t* address = utilities::scan_function(L"client.dll", FIRST_USER_CMD_ARRAY);

		if (!address)
		{
			return nullptr;
		}

		void** first_user_cmd_array = *reinterpret_cast<void***>(utilities::resolve_rip(address, 3, 7));

		return utilities::is_valid_pointer(first_user_cmd_array) ? first_user_cmd_array : nullptr;
	}
};

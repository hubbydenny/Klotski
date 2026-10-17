#pragma once
#include <cstdint>
#include "../../utilities/signatures.hpp"
#include "../../utilities/utilities.hpp"

class i_engine_client
{
public:
	bool execute_cmd(const char* command)
	{
		using function_t = void(__fastcall*)(i_engine_client*, int, const char*, int);
		static function_t fn = reinterpret_cast<function_t>(utilities::pattern_scan(L"engine2.dll", "48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 ? 41 0F B6 E9"));
		if (!fn || !command || !*command) return false;
		fn(this, 0, command, 0x7ffef001);
		return true;
	}

	bool is_in_game()
	{
		using function_t = bool(__fastcall*)(i_engine_client*);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"engine2.dll", IS_IN_GAME));

		if (!fn) return false;
		return fn(this);
	}

	bool is_connected()
	{
		using function_t = bool(__fastcall*)(i_engine_client*);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"engine2.dll", IS_CONNECTED));

		if (!fn) return false;
		return fn(this);
	}

	void get_screen_size(std::int32_t& width, std::int32_t& height)
	{
		using function_t = void(__thiscall*)(i_engine_client*, int&, int&);
		return (*reinterpret_cast<function_t**>(this))[61](this, width, height);
	}

	const char* get_level_name()
	{
		using function_t = const char*(__thiscall*)(i_engine_client*);
		return (*reinterpret_cast<function_t**>(this))[62](this);
	}

	const char* get_level_name_short()
	{
		using function_t = const char*(__thiscall*)(i_engine_client*);
		return (*reinterpret_cast<function_t**>(this))[63](this);
	}
};

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "console/console.hpp"
#include "renderer/renderer.hpp"
#include "vfunc.hpp"

#include "../utilities/signatures.hpp"

namespace utilities {
	bool chat_print(const std::string& message);
	std::uint8_t* pattern_scan(const wchar_t* module_name, const char* signature);
	std::uint8_t* pattern_scan_next(std::uint8_t* previous);
	std::uint8_t* resolve_rip(std::uint8_t* address, std::uint32_t rva_offset, std::uint32_t rip_offset);
	std::uint8_t* scan_function(const wchar_t* module_name, const char* signature);
	std::uint8_t* scan_call(const wchar_t* module_name, const char* signature);
	bool is_valid_pointer(const void* address);
	bool is_readable(const void* address, std::size_t size);
}

namespace memory
{
	template <typename T>
	inline T read(std::uintptr_t address)
	{
		return *reinterpret_cast<T*>(address);
	}

	template <typename T>
	inline void write(std::uintptr_t address, const T& value)
	{
		*reinterpret_cast<T*>(address) = value;
	}

	template <typename return_t, typename... args_t>
	inline return_t call(std::uintptr_t function, args_t... args)
	{
		using function_t = return_t(__fastcall*)(args_t...);
		return reinterpret_cast<function_t>(function)(args...);
	}

	template <typename return_t, typename... args_t>
	inline return_t call_vfunc(std::uintptr_t instance, std::size_t index, args_t... args)
	{
		using function_t = return_t(__fastcall*)(void*, args_t...);

		void* object = reinterpret_cast<void*>(static_cast<std::uintptr_t>(instance));
		const void** vtable = *reinterpret_cast<const void***>(object);

		return reinterpret_cast<function_t>(vtable[index])(object, args...);
	}

	inline std::string read_string(std::uintptr_t address)
	{
		if (!utilities::is_valid_pointer(reinterpret_cast<void*>(address))) return {};

		std::string result;
		char character = { };

		while (result.size() < 512)
		{
			character = read<char>(address + result.size());

			if (!character) break;

			result.push_back(character);
		}

		return result;
	}
}


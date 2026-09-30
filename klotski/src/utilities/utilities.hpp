#pragma once

#include <cstddef>
#include <cstdint>

#include "console/console.hpp"
#include "debug_console/debug.hpp"
#include "renderer/renderer.hpp"

#include "../signatures.hpp"

namespace utilities
{
	std::uint8_t* pattern_scan(const wchar_t* module_name, const char* signature);
	std::uint8_t* pattern_scan_next(std::uint8_t* previous);
	std::uint8_t* resolve_rip(std::uint8_t* address, std::uint32_t rva_offset, std::uint32_t rip_offset);
	std::uint8_t* scan_function(const wchar_t* module_name, const char* signature);
	std::uint8_t* scan_call(const wchar_t* module_name, const char* signature);
	bool is_valid_pointer(const void* address);
	bool is_readable(const void* address, std::size_t size);
}

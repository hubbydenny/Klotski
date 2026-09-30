#include "utilities.hpp"

#include <vector>
#include <Windows.h>

static std::vector<std::uint32_t> pattern_to_byte(const char* pattern)
{
	std::vector<std::uint32_t> bytes;
	char* start = const_cast<char*>(pattern);
	char* end = const_cast<char*>(pattern) + std::strlen(pattern);

	for (char* current = start; current < end; current++)
	{
		if (*current == '?')
		{
			current++;

			if (*current == '?')
			{
				current++;
			}

			bytes.push_back(-1);
		}
		else
		{
			bytes.push_back(std::strtoul(current, &current, 16));
		}
	}

	return bytes;
}

std::uint8_t* utilities::pattern_scan(const wchar_t* module_name, const char* signature)
{
	HMODULE module_handle = GetModuleHandle(module_name);

	if (!module_handle)
	{
		return nullptr;
	}

	PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
	PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

	std::size_t size_of_code = nt_headers->OptionalHeader.SizeOfCode;
	std::uint8_t* image_base = reinterpret_cast<std::uint8_t*>(module_handle) + nt_headers->OptionalHeader.BaseOfCode;

	std::vector<std::uint32_t> pattern_bytes = pattern_to_byte(signature);

	std::size_t pattern_size = pattern_bytes.size();
	std::uint32_t* array_of_bytes = pattern_bytes.data();

	for (std::size_t i = 0; i < size_of_code - pattern_size; i++)
	{
		bool found = true;

		for (std::size_t j = 0; j < pattern_size; j++)
		{
			if (image_base[i + j] != array_of_bytes[j] && array_of_bytes[j] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			return &image_base[i];
		}
	}

	return nullptr;
}

std::uint8_t* utilities::pattern_scan_next(std::uint8_t* previous)
{
	if (!previous) return nullptr;

	HMODULE module_handle = GetModuleHandle(L"client.dll");

	if (!module_handle) return nullptr;

	PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
	PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

	std::size_t size_of_code = nt_headers->OptionalHeader.SizeOfCode;
	std::uint8_t* image_base = reinterpret_cast<std::uint8_t*>(module_handle) + nt_headers->OptionalHeader.BaseOfCode;

	std::size_t previous_offset = previous - image_base;

	std::vector<std::uint32_t> pattern_bytes = pattern_to_byte(DRAW_GLOW_PATTERN);

	std::size_t pattern_size = pattern_bytes.size();
	std::uint32_t* array_of_bytes = pattern_bytes.data();

	for (std::size_t i = previous_offset + 1; i < size_of_code - pattern_size; i++)
	{
		bool found = true;

		for (std::size_t j = 0; j < pattern_size; j++)
		{
			if (image_base[i + j] != array_of_bytes[j] && array_of_bytes[j] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			return &image_base[i];
		}
	}

	return nullptr;
}

std::uint8_t* utilities::resolve_rip(std::uint8_t* address, std::uint32_t rva_offset, std::uint32_t rip_offset)
{
	if (!address || !rva_offset || !rip_offset)
	{
		return nullptr;
	}

	std::int32_t rva = *reinterpret_cast<std::int32_t*>(address + rva_offset);
	std::uint64_t rip = reinterpret_cast<std::uint64_t>(address) + rip_offset;

	return reinterpret_cast<std::uint8_t*>(rip + rva);
}

static bool address_in_module(const wchar_t* module_name, const std::uintptr_t address)
{
	HMODULE module_handle = GetModuleHandle(module_name);

	if (!module_handle)
	{
		return false;
	}

	PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
	PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

	const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(module_handle);
	const std::uintptr_t end = base + nt_headers->OptionalHeader.SizeOfImage;

	return address >= base && address < end;
}

std::uint8_t* utilities::scan_function(const wchar_t* module_name, const char* signature)
{
	std::uint8_t* address = pattern_scan(module_name, signature);

	if (!address || !address_in_module(module_name, reinterpret_cast<std::uintptr_t>(address)))
	{
		return nullptr;
	}

	return address;
}

std::uint8_t* utilities::scan_call(const wchar_t* module_name, const char* signature)
{
	std::uint8_t* address = scan_function(module_name, signature);

	if (!address)
	{
		return nullptr;
	}

	std::uint8_t* target = resolve_rip(address, 1, 5);

	if (!target || !address_in_module(module_name, reinterpret_cast<std::uintptr_t>(target)))
	{
		return nullptr;
	}

	return target;
}

bool utilities::is_valid_pointer(const void* address)
{
	const std::uintptr_t value = reinterpret_cast<std::uintptr_t>(address);

	return value > 0x10000 && value < 0x7FFFFFFFFFFF;
}

bool utilities::is_readable(const void* address, std::size_t size)
{
	if (!is_valid_pointer(address))
	{
		return false;
	}

	MEMORY_BASIC_INFORMATION info = { };

	if (VirtualQuery(address, &info, sizeof(info)) == 0)
	{
		return false;
	}

	if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
	{
		return false;
	}

	const std::uintptr_t start = reinterpret_cast<std::uintptr_t>(address);
	const std::uintptr_t region_end = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;

	return start + size <= region_end;
}
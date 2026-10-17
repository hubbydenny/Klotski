#include "convars.hpp"

#include "../../../utilities/console/console.hpp"
#include "../../interfaces/interfaces.hpp"

// pasted

std::unordered_map<std::uint32_t, std::uintptr_t> IEngineCvar::ConvarMap;
namespace
{
	constexpr std::uint16_t invalid_index = static_cast<std::uint16_t>(-1);
	constexpr std::uint16_t capacity_mask = 0x7FFF;
}

CConVar* IEngineCvar::FindVar(std::uint32_t name_hash)
{
	const std::uint16_t capacity = static_cast<std::uint16_t>(allocation_count & capacity_mask);

	if (head != invalid_index && (!container || capacity == 0)) return nullptr;

	std::uint16_t current = head;

	for (std::size_t visited = 0; current != invalid_index && visited < capacity; visited++)
	{
		if (current >= capacity) return nullptr;

		const convar_container_t& entry = container[current];

		if (entry.cvar && entry.cvar->szName && hash_name(entry.cvar->szName) == name_hash) return entry.cvar;

		current = entry.next_index;
	}

	return nullptr;
}

CConVar* IEngineCvar::FindVar(const char* name)
{
	if (!name) return nullptr;
	return FindVar(hash_name(name));
}

bool IEngineCvar::unlock_all()
{
	constexpr std::uint64_t restriction_mask = FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN | FCVAR_SOMETHING_THAT_HIDES;
	const std::uint16_t capacity = static_cast<std::uint16_t>(allocation_count & capacity_mask);

	if (head != invalid_index && (!container || capacity == 0)) return false;

	std::uint16_t current = head;

	for (std::size_t visited = 0; current != invalid_index && visited < capacity; visited++)
	{
		if (current >= capacity) return false;

		const convar_container_t& entry = container[current];

		if (entry.cvar) entry.cvar->nFlags &= ~restriction_mask;

		current = entry.next_index;
	}

	return current == invalid_index;
}

CConVar* IEngineCvar::GetConVar(const char* name)
{
	if (!name) return nullptr;

	const auto hash = hash_name(name);
	const auto it = ConvarMap.find(hash);
	if (it != ConvarMap.end()) return reinterpret_cast<CConVar*>(it->second);
	if (!interfaces::cvar) return nullptr;

	CConVar* convar = interfaces::cvar->FindVar(hash);
	if (!convar) return nullptr;

	ConvarMap.emplace(hash, reinterpret_cast<std::uintptr_t>(convar));
	return convar;
}

void IEngineCvar::Setup()
{
	ConvarMap.clear();

	if (!interfaces::cvar)
	{
		console::log("[-] IEngineCvar unavailable\n");
		return;
	}

	const std::uint16_t capacity = static_cast<std::uint16_t>(interfaces::cvar->allocation_count & capacity_mask);
	std::uint16_t current = interfaces::cvar->head;
	std::uint32_t count = 0;

	for (std::size_t visited = 0; current != invalid_index && visited < capacity; visited++)
	{
		if (current >= capacity) break;

		const IEngineCvar::convar_container_t& entry = interfaces::cvar->container[current];

		if (entry.cvar && entry.cvar->szName)
		{
			ConvarMap.emplace(hash_name(entry.cvar->szName), reinterpret_cast<std::uintptr_t>(entry.cvar));
			count++;
		}

		current = entry.next_index;
	}

	console::log("[+] IEngineCvar: %u convars\n", count);
}

#pragma once

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <unordered_map>

#define FCVAR_NONE                    0
#define FCVAR_LINKED_CONCOMMAND       (1 << 0)
#define FCVAR_DEVELOPMENTONLY         (1 << 1)
#define FCVAR_GAMEDLL                 (1 << 2)
#define FCVAR_CLIENTDLL               (1 << 3)
#define FCVAR_HIDDEN                  (1 << 4)
#define FCVAR_PROTECTED               (1 << 5)
#define FCVAR_SPONLY                  (1 << 6)
#define FCVAR_ARCHIVE                 (1 << 7)
#define FCVAR_NOTIFY                  (1 << 8)
#define FCVAR_USERINFO                (1 << 9)
#define FCVAR_SOMETHING_THAT_HIDES    (1 << 10)
#define FCVAR_UNLOGGED                (1 << 11)
#define FCVAR_MISSING1                (1 << 12)
#define FCVAR_REPLICATED              (1 << 13)
#define FCVAR_CHEAT                   (1 << 14)
#define FCVAR_PER_USER                (1 << 15)
#define FCVAR_DEMO                    (1 << 16)
#define FCVAR_DONTRECORD              (1 << 17)
#define FCVAR_MISSING2                (1 << 18)
#define FCVAR_RELEASE                 (1 << 19)
#define FCVAR_MENUBAR_ITEM            (1 << 20)
#define FCVAR_NOT_CONNECTED           (1 << 22)
#define FCVAR_VCONSOLE_FUZZY_MATCHING (1 << 23)
#define FCVAR_SERVER_CAN_EXECUTE      (1 << 28)
#define FCVAR_CLIENT_CAN_EXECUTE      (1 << 29)
#define FCVAR_SERVER_CANNOT_QUERY     (1 << 30)
#define FCVAR_VCONSOLE_SET_FOCUS      (1 << 31)

struct convar_value_t
{
	union
	{
		bool          b;
		std::int16_t  i16;
		float         f32;
		std::int32_t  i32;
		std::int64_t  i64;
		std::uint64_t u64;
		const char*   str;
	};
};

class CConVar
{
public:
	const char*     szName;
	const void*     default_value_ptr;
	const void*     min_value;
	const void*     max_value;
	const char*     szDescription;
	std::int16_t    type;
	char            pad0[0x2];
	std::uint32_t   change_count;
	std::uint64_t   nFlags;
	char            pad1[0x20];
	convar_value_t  value;
	char            pad2[0x8];

	[[nodiscard]] bool         get_bool()   const { return value.b; }
	[[nodiscard]] float        get_float()  const { return value.f32; }
	[[nodiscard]] std::int32_t get_int()    const { return value.i32; }
	[[nodiscard]] const char*  get_string() const { return value.str; }

	void set_float( float v )         { value.f32 = v; }
	void set_int( std::int32_t v )    { value.i32 = v; }
};

static_assert(offsetof(CConVar, szName) == 0x0, "CConVar::szName has wrong offset");
static_assert(offsetof(CConVar, nFlags) == 0x30, "CConVar::nFlags has wrong offset");
static_assert(offsetof(CConVar, value) == 0x58, "CConVar::value has wrong offset");

class IEngineCvar
{
public:
	struct convar_container_t
	{
		CConVar* cvar;
		std::uint16_t generation;
		std::uint16_t next_index;
		std::uint32_t links;
	};

	char pad0[0x4A];
	std::uint16_t allocation_count;
	char pad1[0x4];
	convar_container_t* container;
	std::uint16_t head;

	CConVar* FindVar( const char* name );
	CConVar* FindVar( std::uint32_t name_hash );
	bool unlock_all();

	static CConVar* GetConVar( const char* name );
	static void     Setup();

private:
	static std::unordered_map<std::uint32_t, std::uintptr_t> ConvarMap;

public:
	static std::uint32_t hash_name( const char* s )
	{
		std::uint32_t h = 0x811c9dc5u;
		while ( *s ) { h ^= static_cast<std::uint8_t>( *s++ ); h *= 0x01000193u; }
		return h;
	}
};

static_assert(sizeof(IEngineCvar::convar_container_t) == 0x10, "IEngineCvar::convar_container_t has wrong size");
static_assert(offsetof(IEngineCvar, container) == 0x50, "IEngineCvar::container has wrong offset");
static_assert(offsetof(IEngineCvar, head) == 0x58, "IEngineCvar::head has wrong offset");

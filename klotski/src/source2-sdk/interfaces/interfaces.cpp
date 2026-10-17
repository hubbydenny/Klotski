#include "interfaces.hpp"

#include "../../utilities/signatures.hpp"
#include "../../utilities/utilities.hpp"
#include "../../utilities/console/console.hpp"

#include <Windows.h>

i_client* interfaces::client = nullptr;
i_csgo_input* interfaces::csgo_input = nullptr;
i_engine_client* interfaces::engine = nullptr;
i_entity_list* interfaces::entity_list = nullptr;
i_input_system* interfaces::input_system = nullptr;
i_file_system* interfaces::file_system = nullptr;
i_localize* interfaces::localize = nullptr;
i_schema_system* interfaces::schema_system = nullptr;
i_trace* interfaces::trace = nullptr;
IEngineCvar* interfaces::cvar = nullptr;
global_vars_t* interfaces::globals = nullptr;

template<typename T>
static T* get_interface(const wchar_t* module_name, const char* interface_name)
{
	if (!module_name || !interface_name)	return nullptr;

	HMODULE module = GetModuleHandle(module_name);

	if (!module) return nullptr;

	std::uint8_t* create_interface = reinterpret_cast<std::uint8_t*>(GetProcAddress(module, "CreateInterface"));

	if (!create_interface) return nullptr;

	using interface_callback_fn = void*(__cdecl*)();

	typedef struct _interface_reg_t
	{
		interface_callback_fn callback;
		const char* name;
		_interface_reg_t* flink;
	} interface_reg_t;

	interface_reg_t* interface_list = *reinterpret_cast<interface_reg_t**>(utilities::resolve_rip(create_interface, 3, 7));

	if (!interface_list) return nullptr;
	for (interface_reg_t* it = interface_list; it; it = it->flink)
	{
		if (!strcmp(it->name, interface_name)) return reinterpret_cast<T*>(it->callback());

	}
	return nullptr;
}

bool interfaces::initialize()
{
	interfaces::client = get_interface<i_client>(L"client.dll", "Source2Client002");
	interfaces::engine = get_interface<i_engine_client>(L"engine2.dll", "Source2EngineToClient001");
	interfaces::input_system = get_interface<i_input_system>(L"inputsystem.dll", "InputSystemVersion001");
	interfaces::schema_system = get_interface<i_schema_system>(L"schemasystem.dll", "SchemaSystem_001");
	interfaces::file_system = get_interface<i_file_system>(L"filesystem_stdio.dll", "VFileSystem017");
	interfaces::localize = get_interface<i_localize>(L"localize.dll", "Localize_001");
	interfaces::cvar = get_interface<IEngineCvar>(L"tier0.dll", "VEngineCvar007");

	std::uint8_t* csgo_input_address = utilities::pattern_scan(L"client.dll", CSGO_INPUT);
	std::uint8_t* entity_list_address = utilities::pattern_scan(L"client.dll", ENTITY_LIST);
	std::uint8_t* trace_address = utilities::pattern_scan(L"client.dll", TRACE_MANAGER);
	std::uint8_t* global_vars_address = utilities::pattern_scan(L"client.dll", GLOBAL_VARS);

	if (csgo_input_address)
	{
		interfaces::csgo_input = *reinterpret_cast<i_csgo_input**>(utilities::resolve_rip(csgo_input_address, 3, 7));
	}

	if (entity_list_address)
	{
		interfaces::entity_list = *reinterpret_cast<i_entity_list**>(utilities::resolve_rip(entity_list_address, 3, 7));
	}

	if (trace_address)
	{
		interfaces::trace = *reinterpret_cast<i_trace**>(utilities::resolve_rip(trace_address, 3, 7));
	}

	if (global_vars_address)
	{
		interfaces::globals = *reinterpret_cast<global_vars_t**>(utilities::resolve_rip(global_vars_address, 3, 7));
	}

//	console::log("[?] trace: match=%p manager=%p\n", trace_address, interfaces::trace);

	if (!interfaces::client) console::log("[-] failed: client\n");
	if (!interfaces::engine) console::log("[-] failed: engine\n");
	if (!interfaces::entity_list) console::log("[-] failed: entitylist\n");
	if (!interfaces::input_system) console::log("[-] failed: input_system\n");
	if (!interfaces::file_system) console::log("[-] failed: file_system\n");
	if (!interfaces::localize) console::log("[-] failed: localize\n");
	if (!interfaces::schema_system) console::log("[-] failed: schema_system\n");
	if (!interfaces::trace) console::log("[-] failed: tracemgr\n");
	if (!interfaces::cvar) console::log("[-] failed: convars\n");
	if (!interfaces::globals) console::log("[-] failed: globals\n");

	IEngineCvar::Setup();

	if (!interfaces::engine || !interfaces::entity_list || !interfaces::input_system || !interfaces::schema_system || !interfaces::trace || !interfaces::globals)
	{
		console::log("[-] failed to initialize interfaces\n");
		return false;
	}

	console::log("[+] interfaces initialized\n");
	return true;
}

#include <chrono>
#include <thread>

#include "source2-sdk/interfaces/interfaces.hpp"
#include "source2-sdk/schema_system/schema_system.hpp"
#include "utilities/debug_console/debug.hpp"
#include "hooks/hooks.hpp"
#include "menu/menu.hpp"


DWORD WINAPI initialize(void* instance)
{
#ifdef _DEBUG
	debug::initialize(L"klotski");
#endif

	debug::log(L"[!] initializing\n");

	if (!interfaces::initialize())
	{
		debug::log(L"[-] step failed: interfaces::initialize\n");
		MessageBox(nullptr, L"failed to initialize! (interfaces)", L"cs2 error", MB_OK | MB_ICONERROR);
		FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);
		return FALSE;
	}


	if (!schema_system::initialize())
	{
		debug::log(L"[-] step failed: schema_system::initialize\n");
		MessageBox(nullptr, L"failed to initialize! (schema)", L"cs2 error", MB_OK | MB_ICONERROR);
		FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);
		return FALSE;
	}


	if (!hooks::initialize())
	{
		debug::log(L"[-] step failed: hooks::initialize\n");
		MessageBox(nullptr, L"failed to initialize! (hooks)", L"cs2 error", MB_OK | MB_ICONERROR);
		FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);
		return FALSE;
	}

	while (!GetAsyncKeyState(VK_F9))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	menu::open = false;

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);

	return TRUE;
}

DWORD WINAPI release()
{
	hooks::release();

#ifdef _DEBUG
	debug::release();
#endif

	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD call_reason, void* reserved)
{
	switch (call_reason)
	{
		case DLL_PROCESS_ATTACH:
			if (HANDLE handle = CreateThread(nullptr, 0, initialize, instance, 0, nullptr))
			{
				CloseHandle(handle);
			}
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			release();
			break;
		default:
			break;
	}
	return TRUE;
}
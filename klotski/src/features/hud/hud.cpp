#include "hud.hpp"

#include "../../utilities/utilities.hpp"
#include "../../utilities/debug_console/debug.hpp"
#include "../../utilities/minhook/MinHook.h"
#include "../config/config.hpp"
#include "../../signatures.hpp"

#include "hud_assets.hpp"
#include "hud_scripts.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace
{
	void* g_engine{};
	hud::sdk::RunFrameFn g_runFrame{};
	hud::sdk::RunScriptFn g_runScript{};
	hud::sdk::ParseImageUrlFn g_parseImageUrl{};

	hud::sdk::CPanel2D** g_mainMenu{};
	hud::sdk::CPanel2D** g_hudRoot{};

	hud::sdk::CUIPanel* g_loadedHud{};
	hud::sdk::CUIPanel* g_pendingHud{};
	std::uint64_t g_hudSeenAt{};

	void* g_frameTarget{};
	void* g_imageTarget{};
	bool g_installed{};

	constexpr std::uint64_t loadDelay = 1500;
	constexpr const char* originFile = "";
	constexpr const char* hudScheme = "file://{hud}/";
	constexpr std::uint64_t noCache = 1;

	struct Asset
	{
		const char* file;
		const unsigned char* bytes;
		std::size_t size;
	};

	const Asset assets[] = {
		{"notice_bar.png", hud_assets::kNoticeBarPng, sizeof(hud_assets::kNoticeBarPng)},
		{"topleft.png", hud_assets::kTopLeftPng, sizeof(hud_assets::kTopLeftPng)},
		{"alive_ct.png", hud_assets::kAliveCtPng, sizeof(hud_assets::kAliveCtPng)},
		{"alive_t.png", hud_assets::kAliveTPng, sizeof(hud_assets::kAliveTPng)},
		{"alive_skull.png", hud_assets::kAliveSkullPng, sizeof(hud_assets::kAliveSkullPng)},
		{"team_ct.png", hud_assets::kTeamCtPng, sizeof(hud_assets::kTeamCtPng)},
		{"team_t.png", hud_assets::kTeamTPng, sizeof(hud_assets::kTeamTPng)},
		{"weapon_plate.png", hud_assets::kWeaponPlatePng, sizeof(hud_assets::kWeaponPlatePng)},
		{"ammo_plate.png", hud_assets::kAmmoPlatePng, sizeof(hud_assets::kAmmoPlatePng)},
		{"hp_plate.png", hud_assets::kHpPlatePng, sizeof(hud_assets::kHpPlatePng)},
		{"hp_red.png", hud_assets::kHpRedPlatePng, sizeof(hud_assets::kHpRedPlatePng)},
		{"armor_plate.png", hud_assets::kArmorPlatePng, sizeof(hud_assets::kArmorPlatePng)},
		{"hp_icon.png", hud_assets::kHpIconPng, sizeof(hud_assets::kHpIconPng)},
		{"armor_icon.png", hud_assets::kArmorIconPng, sizeof(hud_assets::kArmorIconPng)},
		{"helmet_icon.png", hud_assets::kHelmetIconPng, sizeof(hud_assets::kHelmetIconPng)},
		{"kill_badge.png", hud_assets::kKillBadgePng, sizeof(hud_assets::kKillBadgePng)},
		{"win_ct.png", hud_assets::kWinPlateCtPng, sizeof(hud_assets::kWinPlateCtPng)},
		{"win_t.png", hud_assets::kWinPlateTPng, sizeof(hud_assets::kWinPlateTPng)},
		{"alert_icon.png", hud_assets::kAlertIconPng, sizeof(hud_assets::kAlertIconPng)},
		{"yesvote.png", hud_assets::kYesVotePng, sizeof(hud_assets::kYesVotePng)},
		{"novote.png", hud_assets::kNoVotePng, sizeof(hud_assets::kNoVotePng)},
		{"defuse_icon.png", hud_assets::kDefuseIconPng, sizeof(hud_assets::kDefuseIconPng)},
		{"fade.png", hud_assets::kFadePng, sizeof(hud_assets::kFadePng)},
		{"gotv_ct.png", hud_assets::kGotvCtPng, sizeof(hud_assets::kGotvCtPng)},
		{"gotv_t.png", hud_assets::kGotvTPng, sizeof(hud_assets::kGotvTPng)},
	};

	const std::string& assetsFolder()
	{
		static const std::string folder = [] {
			char temp[MAX_PATH]{};
			std::string dir{ temp, GetTempPathA(sizeof(temp), temp) };

			dir += "scaleform";

			CreateDirectoryA(dir.c_str(), nullptr);
			return dir;
		}();

		return folder;
	}

	void extractAssets()
	{
		const std::string& dir = assetsFolder();

		for (const auto& asset : assets)
		{
			const std::string path = dir + "\\" + asset.file;

			FILE* file = nullptr;

			if (fopen_s(&file, path.c_str(), "wb") == 0 && file)
			{
				std::fwrite(asset.bytes, 1, asset.size, file);
				std::fclose(file);
			}
		}
	}

	void loadScripts(void* engine, hud::sdk::CUIPanel* panel)
	{
		constexpr const char* header = "var contextPanel = $.GetContextPanel();\n";

		for (std::size_t i = 0; i < hud_scripts::kScriptCount; ++i)
		{
			const hud_scripts::ScriptEntry& script = hud_scripts::kScripts[i];

			std::string js{ "try {\n" };
			js += header;
			js.append(reinterpret_cast<const char*>(script.bytes), script.size);
			js += "\n} catch (error) { $.Msg(\"[hud] " + std::string{ script.name } + ": \" + error); }";

			g_runScript(engine, panel, js.c_str(), originFile, noCache);
		}

		debug::log("[hud] %zu script(s) embedded\n", hud_scripts::kScriptCount);
	}

	void reloadOnHotkey()
	{
		if (!(GetAsyncKeyState(VK_F7) & 1))
			return;

		g_loadedHud = nullptr;
		g_pendingHud = nullptr;
	}

	void followHudPanel(void* engine)
	{
		hud::sdk::CUIPanel* hud_panel = hud::isPanelOf(g_hudRoot);

		if (!hud_panel)
		{
			g_loadedHud = nullptr;
			g_pendingHud = nullptr;
			return;
		}

		if (hud_panel == g_loadedHud)
			return;

		if (hud_panel != g_pendingHud)
		{
			g_pendingHud = hud_panel;
			g_hudSeenAt = GetTickCount64();
		}
		else if (GetTickCount64() - g_hudSeenAt >= loadDelay)
		{
			loadScripts(engine, hud_panel);
			g_loadedHud = hud_panel;
		}
	}

	void __fastcall hkRunFrame(void* engine)
	{
		g_runFrame(engine);

		__try
		{
			reloadOnHotkey();
			followHudPanel(engine);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	void __fastcall hkParseImageUrl(void* out, const char* url)
	{
		const std::size_t schemeLen = std::strlen(hudScheme);

		if (url && std::strncmp(url, hudScheme, schemeLen) == 0)
		{
			const char* name = url + schemeLen;
			const char* query = std::strchr(name, '?');
			const int nameLen = query ? static_cast<int>(query - name) : static_cast<int>(std::strlen(name));

			char rewritten[1024];
			std::snprintf(rewritten, sizeof(rewritten), "raw://%s\\%.*s", assetsFolder().c_str(), nameLen, name);

			g_parseImageUrl(out, rewritten);
			return;
		}

		g_parseImageUrl(out, url);
	}

	void* findEngine()
	{
		void** engineSlot{};

		for (const char* pattern : patterns::kEngineGlobals)
		{
			auto* hit = utilities::pattern_scan(L"panorama.dll", pattern);

			if (!hit)
				continue;

			auto** slot = reinterpret_cast<void**>(utilities::resolve_rip(hit, 3, 7));

			if (!engineSlot)
				engineSlot = slot;
		}

		if (!engineSlot)
		{
			debug::log("[-] hud: no engine-slot pattern matched\n");
			return nullptr;
		}

		for (int attempt = 0; attempt < 300 && !*engineSlot; ++attempt)
			Sleep(100);

		if (!*engineSlot)
			debug::log("[-] hud: engine slot is still null\n");

		return *engineSlot;
	}

	bool resolveVTable(void* engine, hud::sdk::VTable& layout)
	{
		auto* runScriptCode = utilities::pattern_scan(L"panorama.dll", patterns::kRunScript);
		auto* runFrameCode = utilities::pattern_scan(L"panorama.dll", patterns::kRunFrame);

		if (!runScriptCode || !runFrameCode)
		{
			debug::log("[-] hud: RunScript/RunFrame pattern not found\n");
			return false;
		}

		layout.vtable = *reinterpret_cast<void***>(engine);

		layout.slots = 0;
		layout.runScriptIndex = patterns::kMaxVTableSlots;
		layout.runFrameIndex = patterns::kMaxVTableSlots;

		while (layout.slots < patterns::kMaxVTableSlots && layout.vtable[layout.slots])
		{
			if (layout.vtable[layout.slots] == runScriptCode && layout.runScriptIndex == patterns::kMaxVTableSlots)
				layout.runScriptIndex = layout.slots;

			if (layout.vtable[layout.slots] == runFrameCode && layout.runFrameIndex == patterns::kMaxVTableSlots)
				layout.runFrameIndex = layout.slots;

			++layout.slots;
		}

		if (layout.runScriptIndex == patterns::kMaxVTableSlots || layout.runFrameIndex == patterns::kMaxVTableSlots)
		{
			debug::log("[-] hud: that vtable is not the UI engine's\n");
			return false;
		}

		return true;
	}

	bool findPanelGlobals()
	{
		struct PanelPattern
		{
			const char* signature;
			int offset;
			hud::sdk::CPanel2D*** out;
			const char* what;
		};

		const PanelPattern panelPatterns[] = {
			{"EC ?? 48 8B 05 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B 48 08", 5, &g_mainMenu, "main menu"},
			{"48 89 35 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85", 3, &g_hudRoot, "CSGOHud"}
		};

		for (const auto& panel : panelPatterns)
		{
			auto* hit = utilities::pattern_scan(L"client.dll", panel.signature);

			if (!hit)
			{
				debug::log("[-] hud: %s pattern not found in client.dll\n", panel.what);
				continue;
			}

			*panel.out = reinterpret_cast<hud::sdk::CPanel2D**>(utilities::resolve_rip(hit, panel.offset, 7));
		}

		if (!g_mainMenu && !g_hudRoot)
		{
			debug::log("[-] hud: neither panel global was found\n");
			return false;
		}

		return true;
	}

	void waitForModules()
	{
		for (int attempt = 0; attempt < 300; ++attempt)
		{
			if (GetModuleHandleA("panorama.dll") && GetModuleHandleA("client.dll") && GetModuleHandleA("rendersystemdx11.dll"))
				return;

			Sleep(100);
		}
	}
}

hud::sdk::CUIPanel* hud::isPanelOf(sdk::CPanel2D** global)
{
	sdk::CPanel2D* clientPanel = global ? *global : nullptr;
	return clientPanel ? clientPanel->uiPanel : nullptr;
}

bool hud::initialize()
{
	waitForModules();

	g_engine = findEngine();

	if (!g_engine) return false;

	sdk::VTable layout{};

	if (!resolveVTable(g_engine, layout)) return false;

	g_runScript = reinterpret_cast<sdk::RunScriptFn>(layout.vtable[layout.runScriptIndex]);

	if (!findPanelGlobals()) return false;

	g_frameTarget = layout.vtable[layout.runFrameIndex];

	if (MH_CreateHook(g_frameTarget, reinterpret_cast<void*>(&hkRunFrame), reinterpret_cast<void**>(&g_runFrame)) != MH_OK)
	{
		debug::log("[-] hud: failed to hook RunFrame\n");
		return false;
	}

	auto* imageTarget = utilities::pattern_scan(L"panorama.dll", patterns::kImageUrl);

	if (imageTarget)
	{
		extractAssets();

		g_imageTarget = imageTarget;

		if (MH_CreateHook(g_imageTarget, reinterpret_cast<void*>(&hkParseImageUrl), reinterpret_cast<void**>(&g_parseImageUrl)) != MH_OK)
		{
			debug::log("[-] hud: failed to hook ParseImageURL\n");
			g_imageTarget = nullptr;
		}
	}
	else
	{
		debug::log("[-] hud: ParseImageURL pattern not found\n");
	}

	debug::log("[+] hud: RunFrame hooked, %zu scripts embedded\n", hud_scripts::kScriptCount);
	debug::log("[?] hud: art folder %s, image hook %p\n", assetsFolder().c_str(), g_imageTarget);

	g_installed = true;

	set_enabled(config::context.scaleform_hud);

	return true;
}

bool hud::is_installed()
{
	return g_installed;
}

void hud::set_enabled(bool enabled)
{
	if (!g_installed)
		return;

	const MH_STATUS status = enabled
		? MH_EnableHook(g_frameTarget)
		: MH_DisableHook(g_frameTarget);

	if (g_imageTarget)
	{
		const MH_STATUS imageStatus = enabled
			? MH_EnableHook(g_imageTarget)
			: MH_DisableHook(g_imageTarget);

		if (imageStatus != MH_OK && imageStatus != MH_ERROR_DISABLED && imageStatus != MH_ERROR_ENABLED)
		{
			debug::log("[-] hud: failed to %s image hook (status %d)\n", enabled ? "enable" : "disable", static_cast<int>(imageStatus));
		}
	}

	if (status != MH_OK && status != MH_ERROR_DISABLED && status != MH_ERROR_ENABLED)
	{
		debug::log("[-] hud: failed to %s (status %d)\n", enabled ? "enable" : "disable", static_cast<int>(status));
		return;
	}

	if (!enabled)
	{
		g_loadedHud = nullptr;
		g_pendingHud = nullptr;
	}

	debug::log("[+] hud: %s\n", enabled ? "enabled" : "disabled");
}

void hud::release()
{
	if (g_frameTarget) MH_DisableHook(g_frameTarget);
	if (g_imageTarget) MH_DisableHook(g_imageTarget);
}
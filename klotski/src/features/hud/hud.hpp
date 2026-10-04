#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace hud {
	namespace sdk
	{
		struct CUIPanel;

		struct CPanel2D
		{
			const void* vmt;
			CUIPanel* uiPanel;
		};

		struct VTable
		{
			void** vtable{};
			std::size_t slots{};
			std::size_t runScriptIndex{};
			std::size_t runFrameIndex{};
		};

		using RunScriptFn = void(*)(void* engine, CUIPanel* contextPanel, const char* js, const char* originFile, std::uint64_t line);
		using RunFrameFn = void(*)(void* engine);
		using ParseImageUrlFn = void(*)(void* out, const char* url);
	}
	bool initialize();
	void release();
	void set_enabled(bool enabled);
	bool is_installed();
	sdk::CUIPanel* isPanelOf(sdk::CPanel2D** global);
}
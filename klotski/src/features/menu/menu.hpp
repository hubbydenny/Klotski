#pragma once
struct ImFont;

namespace menu
{
	extern bool open;
	inline ImFont* font = nullptr;
	inline bool relative_mouse;
	void render();
	bool color_edit4(const char* label, float (&color)[4]);
	bool color_edit4(const char* label, float (&color)[3], float* alpha = nullptr);
}

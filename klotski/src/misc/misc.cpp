#include "misc.hpp"

#include "../config/config.hpp"
#include "../source2-sdk/sdk.hpp"
#include "../utilities/utilities.hpp"
#include "../utilities/renderer/renderer.hpp"
#include "../utilities/imgui/imgui.h"
#include "../source2-sdk/classes/players.hpp"

#include <cstdio>

static void draw_watermark()
{
	if (!config::context.watermark) return;

	char buffer[64]{};
	sprintf_s(buffer, sizeof(buffer), "klotski | %.0f fps", ImGui::GetIO().Framerate);
	renderer::text(config::context.watermark_x, config::context.watermark_y, color_t::white(), buffer);
}

static void velocityind()
{
	if (!config::context.velocity) return;
	if (!sdk::local_player || !utilities::is_valid_pointer(sdk::local_player)) return;

	const float center = static_cast<float>(sdk::screen_width) / 2.f;
	const float velocity2d = vel2d(sdk::local_player->velocity());

	char buffer[64]{};
	sprintf_s(buffer, sizeof(buffer), "%.0f", velocity2d);

	const float text_width = ImGui::CalcTextSize(buffer).x;

	renderer::text_sized(center - text_width / 2.f - 4.f, config::context.velo_y, config::context.velo_size, color_t::white(), buffer);
}
static void keystrokes() {}
void misc::run()
{
	draw_watermark();
	velocityind();
	keystrokes();
}

#include "Scale.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace launcher::ui {

namespace {
float g_rem = 20.0F;
bool g_portrait = false;
float g_minSide = 540.0F;
} // namespace

void Scale::beginFrame(float dpiScale)
{
	const ImVec2 size = ImGui::GetMainViewport()->WorkSize;
	const float diagonal = std::sqrt(size.x * size.x + size.y * size.y);

	g_rem = std::clamp(diagonal / 44.0F, 12.0F * dpiScale, 26.0F * dpiScale);
	g_portrait = size.y > size.x;
	g_minSide = std::min(size.x, size.y);
	static bool logged = false;
	if (!logged) {
		logged = true;
		spdlog::info("Scale: work={}x{} dpi={} rem={}", size.x, size.y, dpiScale, g_rem);
	}
}

float Scale::rem()
{
	return g_rem;
}

bool Scale::portrait()
{
	return g_portrait;
}

float Scale::minSide()
{
	return g_minSide;
}

} // namespace launcher::ui

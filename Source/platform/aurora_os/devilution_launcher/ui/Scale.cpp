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
bool g_screenSizeKnown = false;
float g_screenMinSide = 540.0F;
} // namespace

void Scale::BeginFrame(float dpiScale)
{
	const ImVec2 size = ImGui::GetMainViewport()->WorkSize;
	// Свёрнутое окно рапортует 0x0 (десктопный MINIMIZED): обновлять
	// масштабы нулем нельзя — шрифты с размером 0 роняют assert
	// imgui_draw. Держим последние валидные значения.
	if (size.x <= 0.0F || size.y <= 0.0F) {
		return;
	}
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

float Scale::Rem()
{
	return g_rem;
}

bool Scale::Portrait()
{
	return g_portrait;
}

float Scale::MinSide()
{
	return g_minSide;
}

void Scale::SetScreenSize(int landscapeWidth, int landscapeHeight)
{
	if (landscapeWidth > 0 && landscapeHeight > 0) {
		g_screenMinSide = static_cast<float>(std::min(landscapeWidth, landscapeHeight));
		g_screenSizeKnown = true;
	}
}

float Scale::ScreenMinSide()
{
	return g_screenSizeKnown ? g_screenMinSide : g_minSide;
}

} // namespace launcher::ui

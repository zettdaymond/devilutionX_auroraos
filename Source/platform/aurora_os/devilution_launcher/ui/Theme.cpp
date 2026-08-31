#include "Theme.hpp"
#include "Scale.hpp"

#include <cmrc/cmrc.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>

CMRC_DECLARE(assets);

namespace launcher::ui::Theme {

namespace {

// Палитра: тёмно-коричневые, золото Diablo и глубокий красный.
constexpr ImVec4 kColors[] {
	/* Bg */ ImVec4(0.040F, 0.027F, 0.020F, 1.00F),      // #0A0705
	/* Panel */ ImVec4(0.102F, 0.059F, 0.039F, 0.92F),  // #1A0F0A
	/* PanelHover */ ImVec4(0.165F, 0.090F, 0.063F, 0.95F),
	/* BorderGold */ ImVec4(0.780F, 0.647F, 0.408F, 1.00F), // #C7A568
	/* GoldBright */ ImVec4(0.910F, 0.780F, 0.494F, 1.00F), // #E8C77E
	/* GoldDim */ ImVec4(0.541F, 0.427F, 0.231F, 1.00F),    // #8A6D3B
	/* Red */ ImVec4(0.545F, 0.102F, 0.063F, 1.00F),    // #8B1A10
	/* RedHover */ ImVec4(0.702F, 0.153F, 0.102F, 1.00F),
	/* RedPressed */ ImVec4(0.369F, 0.063F, 0.035F, 1.00F),
	/* TextBody */ ImVec4(0.847F, 0.784F, 0.659F, 1.00F),   // #D8C8A8
	/* TextHeading */ ImVec4(0.910F, 0.780F, 0.494F, 1.00F),
	/* TextDim */ ImVec4(0.420F, 0.365F, 0.282F, 1.00F),
	/* Error */ ImVec4(0.878F, 0.314F, 0.220F, 1.00F),
	/* Success */ ImVec4(0.490F, 0.643F, 0.353F, 1.00F),
};

static_assert(std::size(kColors) == static_cast<size_t>(ColorRole::Success) + 1);

std::unordered_map<FontRole, ImFont *> &fonts()
{
	static std::unordered_map<FontRole, ImFont *> map;
	return map;
}

ImFont *loadFont(const std::string &asset, float pixelSize, const ImWchar *ranges, bool mergeIntoPrevious)
{
	cmrc::file file;
	try {
		file = cmrc::assets::get_filesystem().open(asset);
	} catch (const std::system_error &err) {
		spdlog::error("Missing font asset {}: {}", asset, err.what());
		return nullptr;
	}

// Скопированным буфером владеет ImGui после Build.
	void *data = IM_ALLOC(file.size());
	std::memcpy(data, file.begin(), file.size());

	ImFontConfig config;
	config.MergeMode = mergeIntoPrevious;
	config.FontDataOwnedByAtlas = true;
	return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(data, static_cast<int>(file.size()), pixelSize, &config, ranges);
}

// Диапазон глифов FontAwesome 4 для иконок лаунчера.
const ImWchar *iconRanges()
{
	static const ImWchar ranges[] = { 0xF000, 0xF3FF, 0 };
	return ranges;
}

} // namespace

void Init(float dpiScale)
{
	auto &io = ImGui::GetIO();

// Сбрасываем ранее загруженные шрифты (перезапуск на десктопе).
	io.Fonts->Clear();
	fonts().clear();

	const float bodySize = 22.0F * dpiScale;

	ImFont *body = loadFont("assets/Beaufort-Regular.ttf", bodySize, io.Fonts->GetGlyphRangesCyrillic(), false);
	loadFont("assets/fontawesome-webfont.ttf", bodySize, iconRanges(), true); // icons merged into body
	ImFont *bodyBold = loadFont("assets/Beaufort-Bold.ttf", bodySize, io.Fonts->GetGlyphRangesCyrillic(), false);
	loadFont("assets/fontawesome-webfont.ttf", bodySize, iconRanges(), true);
	ImFont *heading = loadFont("assets/exocet2.ttf", 40.0F * dpiScale, nullptr, false);

	ImFont *iconBig = nullptr;
	try {
		auto iconFile = cmrc::assets::get_filesystem().open("assets/fontawesome-webfont.ttf");
		void *iconData = IM_ALLOC(iconFile.size());
		std::memcpy(iconData, iconFile.begin(), iconFile.size());
		ImFontConfig iconConfig;
		iconConfig.GlyphMinAdvanceX = 56.0F * dpiScale;
		iconBig = io.Fonts->AddFontFromMemoryTTF(iconData, static_cast<int>(iconFile.size()),
		    56.0F * dpiScale, &iconConfig, iconRanges());
	} catch (const std::system_error &err) {
		spdlog::error("Missing icon font asset: {}", err.what());
	}

	fonts()[FontRole::Body] = body;
	fonts()[FontRole::BodyBold] = bodyBold;
	fonts()[FontRole::Heading] = heading;
	fonts()[FontRole::IconBig] = iconBig;
	io.FontDefault = body;

	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowRounding = 6.0F * dpiScale;
	style.FrameRounding = 5.0F * dpiScale;
	style.GrabRounding = 4.0F * dpiScale;
	style.PopupRounding = 6.0F * dpiScale;
	style.ScrollbarRounding = 4.0F * dpiScale;
	style.WindowBorderSize = 1.0F * dpiScale;
	style.FrameBorderSize = 0.0F;
	style.PopupBorderSize = 1.0F * dpiScale;
	style.WindowPadding = ImVec2(12.0F * dpiScale, 12.0F * dpiScale);
	style.FramePadding = ImVec2(10.0F * dpiScale, 8.0F * dpiScale);
	style.ItemSpacing = ImVec2(8.0F * dpiScale, 8.0F * dpiScale);

	ImVec4 *c = style.Colors;
	c[ImGuiCol_Text] = Color(ColorRole::TextBody);
	c[ImGuiCol_TextDisabled] = Color(ColorRole::TextDim);
	c[ImGuiCol_WindowBg] = Color(ColorRole::Bg);
	c[ImGuiCol_PopupBg] = ImVec4(0.055F, 0.035F, 0.025F, 0.97F);
	c[ImGuiCol_Border] = Color(ColorRole::GoldDim);
	c[ImGuiCol_FrameBg] = ImVec4(0.08F, 0.05F, 0.035F, 1.00F);
	c[ImGuiCol_FrameBgHovered] = Color(ColorRole::PanelHover);
	c[ImGuiCol_FrameBgActive] = Color(ColorRole::RedPressed);
	c[ImGuiCol_TitleBg] = Color(ColorRole::Panel);
	c[ImGuiCol_TitleBgActive] = Color(ColorRole::Panel);
	c[ImGuiCol_Button] = Color(ColorRole::Red);
	c[ImGuiCol_ButtonHovered] = Color(ColorRole::RedHover);
	c[ImGuiCol_ButtonActive] = Color(ColorRole::RedPressed);
	c[ImGuiCol_Header] = Color(ColorRole::Panel);
	c[ImGuiCol_HeaderHovered] = Color(ColorRole::PanelHover);
	c[ImGuiCol_HeaderActive] = Color(ColorRole::RedPressed);
	c[ImGuiCol_Separator] = Color(ColorRole::GoldDim);
	c[ImGuiCol_CheckMark] = Color(ColorRole::GoldBright);
	c[ImGuiCol_SliderGrab] = Color(ColorRole::GoldBright);
	c[ImGuiCol_SliderGrabActive] = Color(ColorRole::GoldBright);
	c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.70F);
	c[ImGuiCol_NavCursor] = Color(ColorRole::GoldBright);
}

ImFont *Font(FontRole role)
{
	if (auto it = fonts().find(role); it != fonts().end()) {
		return it->second;
	}
	return nullptr;
}

void PushFont(FontRole role)
{
	ImGui::PushFont(Font(role));
}

void PopFont()
{
	ImGui::PopFont();
}

ImVec4 Color(ColorRole role)
{
	return kColors[static_cast<size_t>(role)];
}

ImU32 ColorU32(ColorRole role)
{
	return ImGui::ColorConvertFloat4ToU32(Color(role));
}

void PushButtonStyle(bool accent)
{
	ImGui::PushStyleColor(ImGuiCol_Button, Color(accent ? ColorRole::Red : ColorRole::Panel));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Color(accent ? ColorRole::RedHover : ColorRole::PanelHover));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, Color(ColorRole::RedPressed));
	ImGui::PushStyleColor(ImGuiCol_Border, Color(ColorRole::BorderGold));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Scale::Px(0.08F));
}

void PopButtonStyle()
{
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(4);
}

void DrawDivider(ImVec2 from, ImVec2 to, float alpha)
{
	ImDrawList *draw = ImGui::GetWindowDrawList();
	const ImU32 gold = ImGui::GetColorU32(ImVec4(Color(ColorRole::BorderGold).x, Color(ColorRole::BorderGold).y,
	    Color(ColorRole::BorderGold).z, alpha));
	const ImU32 transparent = gold & 0x00FFFFFF;
	draw->AddRectFilledMultiColor(from, to, transparent, gold, gold, transparent);
}

} // namespace launcher::ui::Theme

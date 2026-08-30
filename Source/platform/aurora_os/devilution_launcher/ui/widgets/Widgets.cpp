#include "Widgets.hpp"

#include "../Icons.hpp"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <string>

namespace launcher::ui::widgets {

namespace {

/// Largest font size (<= startSize) at which text fits maxWidth.
float FitFontSize(ImFont *font, float startSize, const char *text, float maxWidth, float minSize)
{
	float size = startSize;
	while (size > minSize && font != nullptr) {
		if (font->CalcTextSizeA(size, FLT_MAX, 0.0F, text).x <= maxWidth) {
			return size;
		}
		size -= 1.0F;
	}
	return std::max(size, minSize);
}

} // namespace

float FitFontSizeFor(FontRole role, float startSize, const char *text, float maxWidth, float minSize)
{
	return FitFontSize(Theme::font(role), startSize, text, maxWidth, minSize);
}

namespace {

/// Truncates text with "…" so it fits maxWidth at the given size.
/// Removes whole UTF-8 code points so Cyrillic glyphs stay intact.
std::string FitTextEllipsis(ImFont *font, float size, const char *text, float maxWidth)
{
	if (font == nullptr || font->CalcTextSizeA(size, FLT_MAX, 0.0F, text).x <= maxWidth) {
		return text;
	}
	std::string result(text);
	auto popCodePoint = [&result]() {
		result.pop_back();
		while (!result.empty() && (static_cast<unsigned char>(result.back()) & 0xC0) == 0x80) {
			result.pop_back();
		}
	};
	while (!result.empty() && font->CalcTextSizeA(size, FLT_MAX, 0.0F, (result + "…").c_str()).x > maxWidth) {
		popCodePoint();
	}
	return result + "…";
}

} // namespace

void GameCard(const char *title, const char *status, const char *icon, bool available,
    bool showPlayBadge, const ImVec2 &size, float titleSizeOverride,
    const std::function<void()> &onClick)
{
	// The whole card is one invisible button; visuals are drawn on top
	// with the draw list.
	if (ImGui::InvisibleButton(title, size, ImGuiButtonFlags_None)) {
		onClick();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImDrawList *draw = ImGui::GetWindowDrawList();
	const ImVec2 pos = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const bool hovered = ImGui::IsItemHovered();
	const bool held = ImGui::IsItemActive();

	// Panel
	ImU32 fillColor = Theme::colorU32(hovered ? ColorRole::PanelHover : ColorRole::Panel);
	if (!available) {
		fillColor = ImGui::GetColorU32(ImVec4(0.07F, 0.05F, 0.04F, 0.92F));
	}
	const float rounding = Scale::px(0.35F);
	draw->AddRectFilled(pos, max, fillColor, rounding);

	// Border: gold when available, dim when locked; bright while pressed.
	ImU32 borderColor = Theme::colorU32(available ? ColorRole::BorderGold : ColorRole::GoldDim);
	float borderThickness = Scale::px(0.075F);
	if (held && available) {
		borderColor = Theme::colorU32(ColorRole::GoldBright);
		borderThickness = Scale::px(0.11F);
	}
	draw->AddRect(pos, max, borderColor, rounding, 0, borderThickness);

	// Corner accents (small gold triangles) for available cards.
	if (available) {
		const float t = Scale::px(0.55F);
		const ImVec2 tl = pos + ImVec2(rounding * 0.5F, rounding * 0.5F);
		const ImVec2 br = max - ImVec2(rounding * 0.5F, rounding * 0.5F);
		const ImU32 gold = Theme::colorU32(ColorRole::BorderGold);
		draw->AddTriangleFilled(tl, tl + ImVec2(t, 0), tl + ImVec2(0, t), gold);
		draw->AddTriangleFilled(br, br - ImVec2(t, 0), br - ImVec2(0, t), gold);
	}

	// Icon on the left (shrinks on narrow cards).
	const float iconSize = std::min(Scale::px(2.4F), size.y * 0.62F);
	const float sidePadding = Scale::px(0.5F);
	const ImVec2 iconPos = pos + ImVec2(sidePadding, (size.y - iconSize) * 0.5F);
	draw->AddText(Theme::font(FontRole::IconBig), iconSize, iconPos,
	    available ? Theme::colorU32(ColorRole::GoldBright) : Theme::colorU32(ColorRole::TextDim), icon);

	// Space for the play/lock badge on the right.
	const char *badge = available ? (showPlayBadge ? icons::Play : nullptr) : icons::Lock;
	float badgeWidth = 0.0F;
	if (badge != nullptr) {
		ImGui::PushFont(Theme::font(FontRole::IconBig));
		badgeWidth = ImGui::CalcTextSize(badge).x + Scale::px(0.4F);
		ImGui::PopFont();
	}

	// Text block; font sizes shrink so text always fits the card.
	// Cards in one row share the caller-supplied titleSize for a
	// consistent baseline. The whole block is vertically centered.
	const float rightReserve = std::max(badgeWidth, Scale::px(1.2F));
	const float textMaxWidth = size.x - sidePadding - rightReserve - iconSize - Scale::px(0.5F);
	const float titleSize = titleSizeOverride > 0.0F
	    ? titleSizeOverride
	    : FitFontSize(Theme::font(FontRole::Heading), Scale::px(2.0F), title, textMaxWidth, Scale::px(0.95F));
	const float statusSize = std::min(Scale::px(0.9F), titleSize * 0.45F);
	const std::string statusText = FitTextEllipsis(Theme::font(FontRole::Body), statusSize, status, textMaxWidth);

	const float blockHeight = titleSize + Scale::px(0.35F) + statusSize * 1.25F;
	ImVec2 textPos(iconPos.x + iconSize + Scale::px(0.5F), pos.y + (size.y - blockHeight) * 0.5F);

	draw->AddText(Theme::font(FontRole::Heading), titleSize, textPos,
	    available ? Theme::colorU32(ColorRole::TextHeading) : Theme::colorU32(ColorRole::TextDim), title);
	textPos.y += titleSize + Scale::px(0.35F);
	draw->AddText(Theme::font(FontRole::Body), statusSize, textPos,
	    available ? Theme::colorU32(ColorRole::TextBody) : Theme::colorU32(ColorRole::TextDim), statusText.c_str());

	// Play badge / lock in the right corner.
	if (badge != nullptr) {
		ImGui::PushFont(Theme::font(FontRole::IconBig));
		const ImVec2 badgeSize = ImGui::CalcTextSize(badge);
		const ImVec2 badgePos = ImVec2(max.x - badgeSize.x - Scale::px(0.8F), pos.y + (size.y - badgeSize.y) * 0.5F);
		draw->AddText(badgePos, Theme::colorU32(available ? ColorRole::GoldBright : ColorRole::TextDim), badge);
		ImGui::PopFont();
	}
}

void IconButton(const char *icon, const char *label, bool accent, const ImVec2 &size,
    const std::function<void()> &onClick)
{
	Theme::pushButtonStyle(accent);
	const std::string text = std::string(icon) + "  " + label;
	if (ImGui::Button(text.c_str(), size)) {
		onClick();
	}
	Theme::popButtonStyle();
}

void CenteredText(const char *text, ColorRole role)
{
	const ImVec2 textSize = ImGui::CalcTextSize(text);
	const float width = ImGui::GetContentRegionAvail().x;
	ImGui::SetCursorPosX(std::max(0.0F, (width - textSize.x) * 0.5F));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(role));
	ImGui::TextUnformatted(text);
	ImGui::PopStyleColor();
}

void FileStatusLine(bool present, const char *text, const char *detail)
{
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(present ? ColorRole::Success : ColorRole::TextDim));
	ImGui::TextUnformatted(present ? icons::Check : icons::Times);
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	if (!present) {
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	}
	ImGui::TextWrapped("%s", text);
	if (!present) {
		ImGui::PopStyleColor();
	}

	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	ImGui::TextWrapped("%s", detail);
	ImGui::PopStyleColor();
}

} // namespace launcher::ui::widgets

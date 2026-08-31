#include "Widgets.hpp"

#include "../Icons.hpp"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
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

void CornerAccents(ImDrawList *draw, const ImVec2 &min, const ImVec2 &max, float rounding)
{
	const float t = Scale::px(0.55F);
	const ImVec2 tl = min + ImVec2(rounding * 0.5F, rounding * 0.5F);
	const ImVec2 br = max - ImVec2(rounding * 0.5F, rounding * 0.5F);
	const ImU32 gold = Theme::colorU32(ColorRole::BorderGold);
	draw->AddTriangleFilled(tl, tl + ImVec2(t, 0), tl + ImVec2(0, t), gold);
	draw->AddTriangleFilled(br, br - ImVec2(t, 0), br - ImVec2(0, t), gold);
}

} // namespace

void HeroPanel(const char *eyebrow, const char *title, const char *status, const BackgroundArt &art,
    const ImVec2 &uv0, const ImVec2 &uv1, const ImVec4 &tint, const ImVec2 &size,
    std::initializer_list<HeroAction> actions)
{
	ImDrawList *draw = ImGui::GetWindowDrawList();
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max = min + size;
	const float rounding = Scale::px(0.5F);

	// Reserve layout space; interaction happens through the buttons only.
	ImGui::Dummy(size);

	// Artwork (aspect-fill of the requested crop). Едва заметное
	// «дыхание» масштаба оживляет панель, не отвлекая от контента.
	if (art.texture != nullptr && art.size.x > 0 && art.size.y > 0) {
		const float breathe = 0.004F + 0.004F * std::sin(static_cast<float>(ImGui::GetTime()) * 0.15F);
		const float cropW = (uv1.x - uv0.x) * art.size.x * (1.0F - 2.0F * breathe);
		const float cropH = (uv1.y - uv0.y) * art.size.y * (1.0F - 2.0F * breathe);
		const float scale = std::max(size.x / cropW, size.y / cropH);
		const ImVec2 shown(cropW * scale, cropH * scale);
		const ImVec2 crop(0.5F - (size.x / shown.x) * 0.5F, 0.5F - (size.y / shown.y) * 0.5F);
		const ImVec2 cuv0(uv0.x + breathe + crop.x * (uv1.x - uv0.x), uv0.y + breathe + crop.y * (uv1.y - uv0.y));
		const ImVec2 cuv1(uv1.x - breathe - crop.x * (uv1.x - uv0.x), uv1.y - breathe - crop.y * (uv1.y - uv0.y));
		draw->AddImageRounded(art.texture, min, max, cuv0, cuv1, IM_COL32_WHITE, rounding);
	} else {
		draw->AddRectFilled(min, max, Theme::colorU32(ColorRole::Panel), rounding);
	}

	// Legibility: full-canvas tint + bottom gradient (transparent → dark).
	draw->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(tint), rounding);
	const ImU32 transparent = IM_COL32(0, 0, 0, 0);
	const ImU32 dark = ImGui::GetColorU32(ImVec4(0.03F, 0.02F, 0.01F, 0.88F));
	draw->AddRectFilledMultiColor(min, max, transparent, transparent, dark, dark);
	draw->AddRect(min, max, Theme::colorU32(ColorRole::BorderGold), rounding, 0, Scale::px(0.08F));
	CornerAccents(draw, min, max, rounding);

	// Text block, anchored bottom-left and kept clear of the button row.
	const float pad = Scale::px(0.9F);
	const size_t actionCount = actions.size();
	const float btnH = Scale::px(2.4F);
	const float btnW = actionCount > 0
	    ? std::min(Scale::px(9.0F), (size.x - pad * 2.0F - Scale::px(0.5F) * static_cast<float>(actionCount - 1))
	                                  / static_cast<float>(actionCount))
	    : 0.0F;
	const float actionsHeight = actionCount > 0 ? btnH + Scale::px(0.6F) : 0.0F;

	const float statusSize = Scale::px(0.9F);
	int statusLines = 1;
	for (const char *c = status; c != nullptr && *c != '\0'; ++c) {
		if (*c == '\n') {
			++statusLines;
		}
	}
	// The text block sits fully ABOVE the button row, so it always spans
	// the panel width (buttons are anchored to the bottom edge).
	const float textMaxWidth = size.x - pad * 2.0F;

	// Measure the text block first so it never grows into the buttons.
	const float eyebrowH = Scale::px(0.95F);
	const float titleSize = FitFontSize(Theme::font(FontRole::Heading), Scale::px(2.7F), title, textMaxWidth,
	    Scale::px(1.2F));
	const float statusH = (status != nullptr) ? statusLines * statusSize * 1.3F : 0.0F;
	const float blockHeight = eyebrowH + titleSize + Scale::px(0.15F) + statusH;

	float textY = max.y - pad - actionsHeight - blockHeight;

	draw->AddText(Theme::font(FontRole::BodyBold), Scale::px(0.75F), ImVec2(min.x + pad, textY),
	    Theme::colorU32(ColorRole::GoldBright), eyebrow);
	textY += eyebrowH;
	draw->AddText(Theme::font(FontRole::Heading), titleSize, ImVec2(min.x + pad, textY),
	    Theme::colorU32(ColorRole::TextHeading), title);
	textY += titleSize + Scale::px(0.15F);
	if (status != nullptr) {
		// Draw status line by line so embedded '\n' breaks stay aligned.
		// The font first shrinks to fit (dense phone DPI), ellipsis is the
		// last resort only.
		const char *lineStart = status;
		while (lineStart != nullptr && *lineStart != '\0') {
			const char *lineEnd = std::strchr(lineStart, '\n');
			const std::string line(lineStart, lineEnd != nullptr ? lineEnd : lineStart + std::strlen(lineStart));
			const float lineSize = FitFontSize(Theme::font(FontRole::Body), statusSize, line.c_str(), textMaxWidth,
			    Scale::px(0.62F));
			const std::string fitted = FitTextEllipsis(Theme::font(FontRole::Body), lineSize, line.c_str(),
			    textMaxWidth);
			draw->AddText(Theme::font(FontRole::Body), lineSize, ImVec2(min.x + pad, textY),
			    Theme::colorU32(ColorRole::TextBody), fitted.c_str());
			textY += lineSize * 1.3F;
			lineEnd != nullptr ? lineStart = lineEnd + 1 : lineStart = nullptr;
		}
	}

	// CTA buttons, anchored bottom-right (first action is rightmost).
	if (actionCount > 0) {
		float x = max.x - pad - btnW;
		const float y = max.y - pad - btnH;
		for (const HeroAction &action : actions) {
			ImGui::SetCursorScreenPos(ImVec2(x, y));
			if (action.primary) {
				PrimaryButton(action.label, ImVec2(btnW, btnH), action.onClick);
			} else {
				GhostButton(nullptr, action.label, ImVec2(btnW, btnH), action.onClick);
			}
			x -= btnW + Scale::px(0.5F);
		}
	}
}

void GameTile(const char *title, const char *status, const char *icon, bool available,
    const ImVec4 &accent, const ImVec2 &size, const std::function<void()> &onClick)
{
	if (ImGui::InvisibleButton(title, size, ImGuiButtonFlags_None)) {
		onClick();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImDrawList *draw = ImGui::GetWindowDrawList();
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const bool hovered = ImGui::IsItemHovered();
	const bool held = ImGui::IsItemActive();
	const float rounding = Scale::px(0.4F);

	ImU32 fill = ImGui::GetColorU32(ImVec4(0.10F, 0.06F, 0.04F, 0.92F));
	if (hovered) {
		fill = ImGui::GetColorU32(ImVec4(0.16F, 0.10F, 0.06F, 0.95F));
	}
	draw->AddRectFilled(min, max, fill, rounding);

	ImU32 border = Theme::colorU32(available ? ColorRole::BorderGold : ColorRole::GoldDim);
	float borderThickness = Scale::px(0.07F);
	if (held) {
		border = Theme::colorU32(ColorRole::GoldBright);
		borderThickness = Scale::px(0.1F);
	}
	draw->AddRect(min, max, border, rounding, 0, borderThickness);

	// Icon chip with the game accent.
	const float chip = size.y - Scale::px(1.0F);
	const ImVec2 chipMin = min + ImVec2(Scale::px(0.5F), Scale::px(0.5F));
	const ImVec2 chipMax = chipMin + ImVec2(chip, chip);
	draw->AddRectFilled(chipMin, chipMax,
	    ImGui::ColorConvertFloat4ToU32(ImVec4(accent.x, accent.y, accent.z, available ? 0.35F : 0.15F)),
	    Scale::px(0.3F));
	ImGui::PushFont(Theme::font(FontRole::IconBig));
	const ImVec2 iconSize = ImGui::CalcTextSize(icon);
	draw->AddText(ImVec2((chipMin.x + chipMax.x - iconSize.x) * 0.5F, (chipMin.y + chipMax.y - iconSize.y) * 0.5F),
	    Theme::colorU32(available ? ColorRole::GoldBright : ColorRole::TextDim), icon);
	ImGui::PopFont();

	// Text block.
	const float textX = chipMax.x + Scale::px(0.6F);
	const float textMaxWidth = max.x - textX - Scale::px(2.0F);
	const float titleSize = FitFontSize(Theme::font(FontRole::BodyBold), Scale::px(1.15F), title, textMaxWidth,
	    Scale::px(0.8F));
	const std::string statusText = FitTextEllipsis(Theme::font(FontRole::Body), Scale::px(0.8F), status,
	    textMaxWidth);
	const float blockH = titleSize + Scale::px(0.2F) + Scale::px(0.95F);
	ImVec2 textPos(textX, min.y + (size.y - blockH) * 0.5F);
	draw->AddText(Theme::font(FontRole::BodyBold), titleSize, textPos,
	    Theme::colorU32(available ? ColorRole::TextHeading : ColorRole::TextDim), title);
	textPos.y += titleSize + Scale::px(0.2F);
	draw->AddText(Theme::font(FontRole::Body), Scale::px(0.8F), textPos,
	    Theme::colorU32(available ? ColorRole::TextBody : ColorRole::TextDim), statusText.c_str());

	// Trailing play/lock indicator.
	const char *badge = available ? icons::Play : icons::Lock;
	ImGui::PushFont(Theme::font(FontRole::IconBig));
	const ImVec2 badgeSize = ImGui::CalcTextSize(badge);
	draw->AddText(ImVec2(max.x - badgeSize.x - Scale::px(0.7F), min.y + (size.y - badgeSize.y) * 0.5F),
	    Theme::colorU32(available ? ColorRole::GoldBright : ColorRole::TextDim), badge);
	ImGui::PopFont();
}

void PrimaryButton(const char *label, const ImVec2 &size, const std::function<void()> &onClick)
{
	if (ImGui::InvisibleButton(label, size, ImGuiButtonFlags_None)) {
		onClick();
	}

	ImDrawList *draw = ImGui::GetWindowDrawList();
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const bool hovered = ImGui::IsItemHovered();
	const bool held = ImGui::IsItemActive();
	const float rounding = Scale::px(0.45F);

	// Drop shadow, then a vertical gold gradient.
	draw->AddRectFilled(min + ImVec2(0, Scale::px(0.15F)), max + ImVec2(0, Scale::px(0.15F)),
	    IM_COL32(0, 0, 0, 110), rounding);
	ImVec4 top = Theme::color(held ? ColorRole::GoldDim : ColorRole::GoldBright);
	ImVec4 bottom = Theme::color(ColorRole::BorderGold);
	if (hovered && !held) {
		top.x = std::min(top.x * 1.08F, 1.0F);
		top.y = std::min(top.y * 1.08F, 1.0F);
		top.z = std::min(top.z * 1.08F, 1.0F);
	}
	draw->AddRectFilledMultiColor(min, max,
	    ImGui::ColorConvertFloat4ToU32(top), ImGui::ColorConvertFloat4ToU32(top),
	    ImGui::ColorConvertFloat4ToU32(bottom), ImGui::ColorConvertFloat4ToU32(bottom));
	draw->AddRect(min, max, Theme::colorU32(ColorRole::GoldDim), rounding, 0, Scale::px(0.06F));

	ImFont *font = Theme::font(FontRole::BodyBold);
	const std::string text = FitTextEllipsis(font, Scale::px(1.0F), label, size.x - Scale::px(1.2F));
	const ImVec2 textSize = font->CalcTextSizeA(Scale::px(1.0F), FLT_MAX, 0.0F, text.c_str());
	draw->AddText(font, Scale::px(1.0F),
	    ImVec2((min.x + max.x - textSize.x) * 0.5F, (min.y + max.y - textSize.y) * 0.5F),
	    IM_COL32(38, 18, 7, 255), text.c_str());
}

void GhostButton(const char *icon, const char *label, const ImVec2 &size,
    const std::function<void()> &onClick)
{
	// Opaque fill so hero artwork doesn't bleed through the label.
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.055F, 0.035F, 0.025F, 0.94F));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::color(ColorRole::PanelHover));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::color(ColorRole::RedPressed));
	ImGui::PushStyleColor(ImGuiCol_Border, Theme::color(ColorRole::BorderGold));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextHeading));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Scale::px(0.08F));
	const std::string text = icon != nullptr ? std::string(icon) + "  " + label : std::string(label);
	if (ImGui::Button(text.c_str(), size)) {
		onClick();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(5);
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

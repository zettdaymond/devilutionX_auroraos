#pragma once

#include <imgui.h>

namespace launcher::ui {

enum class FontRole {
	Body,      // Beaufort Regular + Cyrillic + icons
	BodyBold,  // Beaufort Bold + Cyrillic + icons
	Heading,   // Exocet (latin only — Diablo logo style)
	IconBig,   // large standalone icons
};

enum class ColorRole {
	Bg,
	Panel,
	PanelHover,
	BorderGold,
	GoldBright,
	GoldDim,
	Red,
	RedHover,
	RedPressed,
	TextBody,
	TextHeading,
	TextDim,
	Error,
	Success,
};

/// Diablo-styled look: fonts by role, palette and ImGui style.
/// Call init() once after creating the ImGui context.
namespace Theme {

void init(float dpiScale);

[[nodiscard]] ImFont *font(FontRole role);
void pushFont(FontRole role);
void popFont();

[[nodiscard]] ImVec4 color(ColorRole role);
[[nodiscard]] ImU32 colorU32(ColorRole role);

/// Push/pop a Diablo button appearance (red fill, gold border).
void pushButtonStyle(bool accent = false);
void popButtonStyle();

/// Gold hairline with faded ends, for separating sections.
void drawDivider(ImVec2 from, ImVec2 to, float alpha = 1.0F);

} // namespace Theme

} // namespace launcher::ui

#pragma once

#include "../Scale.hpp"
#include "../Theme.hpp"
#include "core/Intent.hpp"
#include "core/LauncherState.hpp"

#include <imgui.h>

#include <functional>

namespace launcher::ui::widgets {

using Dispatcher = std::function<void(Intent)>;

/// Artwork shared with the view (the launcher background texture).
struct BackgroundArt {
	void *texture = nullptr;
	ImVec2 size { 0.0F, 0.0F };
};

/// A call-to-action button for hero panels.
struct HeroAction {
	const char *label = nullptr;
	bool primary = false;                 // gold gradient vs outlined
	std::function<void()> onClick;
};

/// Featured-game hero panel: artwork with a bottom gradient (web-style
/// "key art" banner), eyebrow label, large title, status line and up to
/// two CTA buttons. The whole panel is clickable only via its buttons.
///
/// @param uv0/uv1  crop of the artwork to show (normalized)
/// @param tint     per-game accent mixed over the artwork; its alpha sets
///                 the mix strength (bg crops ~0.16, dedicated arts ~0.08)
void HeroPanel(const char *eyebrow, const char *title, const char *status, const BackgroundArt &art,
    const ImVec2 &uv0, const ImVec2 &uv1, const ImVec4 &tint, const ImVec2 &size,
    std::initializer_list<HeroAction> actions);

/// Compact shelf tile: icon in a tinted chip, title + status, trailing
/// play/lock indicator.
void GameTile(const char *title, const char *status, const char *icon, bool available,
    const ImVec4 &accent, const ImVec2 &size, const std::function<void()> &onClick);

/// Gold gradient button with dark bold text — the primary action.
void PrimaryButton(const char *label, const ImVec2 &size, const std::function<void()> &onClick);

/// Outlined button with gold text — the secondary action.
void GhostButton(const char *icon, const char *label, const ImVec2 &size,
    const std::function<void()> &onClick);

/// Secondary button with icon (kept for screens/dialogs).
void IconButton(const char *icon, const char *label, bool accent, const ImVec2 &size,
    const std::function<void()> &onClick);

/// Text with a given role color, centered in the current content width.
void CenteredText(const char *text, ColorRole role = ColorRole::TextBody);

/// Status bullet: green check or dim cross + text + wrapped detail.
void FileStatusLine(bool present, const char *text, const char *detail);

} // namespace launcher::ui::widgets

#pragma once

#include "../Scale.hpp"
#include "../Theme.hpp"
#include "core/Intent.hpp"
#include "core/LauncherState.hpp"

#include <imgui.h>

#include <functional>

namespace launcher::ui::widgets {

using Dispatcher = std::function<void(Intent)>;

/// One launchable game entry: an ornate panel with icon, title and a
/// status line. The whole panel acts as a button.
///
/// @param title     latin title (rendered with the Exocet heading font)
/// @param status    status line under the title
/// @param icon      FontAwesome icon for the card
/// @param available whether the game can start (locked look otherwise)
/// @param titleSize uniform title size for the whole row (0 = auto-fit)
/// @param onClick   invoked when the card is tapped
void GameCard(const char *title, const char *status, const char *icon, bool available,
    bool showPlayBadge, const ImVec2 &size, float titleSize, const std::function<void()> &onClick);

/// Largest font size (<= startSize) for `text` in the given font role
/// that still fits maxWidth, not going below minSize.
float FitFontSizeFor(FontRole role, float startSize, const char *text, float maxWidth, float minSize);

/// Secondary button with icon, e.g. "Выбрать папку".
void IconButton(const char *icon, const char *label, bool accent, const ImVec2 &size,
    const std::function<void()> &onClick);

/// Text with a given role color, centered in the current content width.
void CenteredText(const char *text, ColorRole role = ColorRole::TextBody);

/// Status bullet: green check or dim cross + text.
void FileStatusLine(bool present, const char *text, const char *detail);

} // namespace launcher::ui::widgets

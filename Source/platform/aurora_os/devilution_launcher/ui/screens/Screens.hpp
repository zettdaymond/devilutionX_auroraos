#pragma once

#include "../widgets/Widgets.hpp"

namespace launcher::ui::screens {

using widgets::Dispatcher;

/// Main screen: game cards (or first-run hero) plus optional
/// Russian-voice-pack offer.
void Home(const LauncherState &state, const Dispatcher &dispatch);

/// Game data management: folder picker, file checklist, deletions,
/// free space.
void Data(const LauncherState &state, const Dispatcher &dispatch);

/// About the port.
void About(const LauncherState &state);

} // namespace launcher::ui::screens

#pragma once

#include "../widgets/Widgets.hpp"

namespace launcher::ui::screens {

using widgets::BackgroundArt;
using widgets::Dispatcher;

/// All artwork the screens render. The per-mode entries are optional
/// (null texture): hero panels then fall back to a crop of `background`.
struct ArtSet {
    BackgroundArt background;
    BackgroundArt diablo;
    BackgroundArt hellfire;
    BackgroundArt demo;
};

/// Main screen: featured-game hero panel (web "key art" banner) plus a
/// shelf of the remaining modes, or the first-run hero when no game
/// files are present.
void Home(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art);

/// Game data management: folder picker, file checklist, deletions,
/// free space.
void Data(const LauncherState &state, const Dispatcher &dispatch);

/// About the port.
void About(const LauncherState &state);

} // namespace launcher::ui::screens

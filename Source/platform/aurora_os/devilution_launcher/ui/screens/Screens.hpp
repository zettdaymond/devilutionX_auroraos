#pragma once

#include "../widgets/Widgets.hpp"

namespace launcher::ui::screens {

using widgets::BackgroundArt;
using widgets::Dispatcher;

/// Per-mode artwork: the dedicated hero key art and the golden silhouette
/// icon set for shelf tiles. Both are optional (null textures) — the hero
/// falls back to a crop of `background`, the icon to a FontAwesome glyph.
struct ModeArt {
    BackgroundArt hero;
    widgets::IconSet icon;
};

/// All artwork the screens render.
struct ArtSet {
    BackgroundArt background;
    ModeArt diablo;
    ModeArt hellfire;
    ModeArt demo;
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

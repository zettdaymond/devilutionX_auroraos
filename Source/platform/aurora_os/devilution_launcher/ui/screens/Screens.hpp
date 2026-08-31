#pragma once

#include "../widgets/Widgets.hpp"

namespace launcher::ui::screens {

using widgets::BackgroundArt;
using widgets::Dispatcher;

/// Арты режима: собственная картина hero-панели и набор золотых
/// силуэтов-иконок плиток. Оба необязательны (пустые текстуры):
/// без арта — обрезка общего фона, без иконки — глиф FontAwesome.
struct ModeArt {
    BackgroundArt hero;
    widgets::IconSet icon;
};

/// Все арты, которыми рисуют экраны.
struct ArtSet {
    BackgroundArt background;
    ModeArt diablo;
    ModeArt hellfire;
    ModeArt demo;
};

/// Главный экран: hero-панель выбранного режима (большая обложка с
/// артом и кнопкой) плюс полка остальных режимов; при отсутствии
/// файлов — панель первого запуска.
void Home(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art);

/// Работа с данными игры: выбор папки, чек-лист файлов, удаление
/// скачанного, свободное место.
void Data(const LauncherState &state, const Dispatcher &dispatch);

/// Экран «О порте».
void About(const LauncherState &state);

} // namespace launcher::ui::screens

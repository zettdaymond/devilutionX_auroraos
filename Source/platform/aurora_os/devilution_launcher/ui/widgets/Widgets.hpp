#pragma once

#include "../Scale.hpp"
#include "../Theme.hpp"
#include "core/Intent.hpp"
#include "core/LauncherState.hpp"

#include <imgui.h>

#include <functional>
#include <string>

namespace launcher::ui::widgets {

using Dispatcher = std::function<void(Intent)>;

/// Обрезает текст с «…» под заданную ширину (кегль и шрифт передаются
/// явно — мерить нужно теми же, которыми рисуем).
[[nodiscard]] std::string FitTextEllipsis(ImFont *font, float size, const char *text, float maxWidth);

/// Арт, разделяемый с view-слоем (текстура лаунчера и её размер).
struct BackgroundArt {
	void *texture = nullptr;
	ImVec2 size { 0.0F, 0.0F };
};

/// Иконка режима, заранее уменьшенная в несколько копий — замена мипмапам,
/// которые SDL_Renderer не генерирует: рисуем ближайший уровень сверху, и
/// минификация не превышает ~2x при любом размере окна.
struct IconSet {
	static constexpr int kMaxLevels = 3;
	BackgroundArt levels[kMaxLevels]; ///< от крупной к мелкой
	int count = 0;
};

/// Кнопка действия для hero-панелей.
struct HeroAction {
	const char *label = nullptr;
	bool primary = false;                 // gold gradient vs outlined
	std::function<void()> onClick;
};

/// Hero-панель главного режима — большая «обложка» игры: арт с нижним
/// градиентом, надзаголовок, крупное название, строка статуса и до двух
/// кнопок действий. Панель нажимается только своими кнопками.
///
/// @param uv0/uv1  видимая часть арта (в долях единицы)
/// @param tint     цветовой оттенок поверх арта; его альфа — сила
///                 подкраски (обрезки фона ~0.16, свои арты ~0.08)
void HeroPanel(const char *eyebrow, const char *title, const char *status, const BackgroundArt &art,
    const ImVec2 &uv0, const ImVec2 &uv1, const ImVec4 &tint, const ImVec2 &size,
    std::initializer_list<HeroAction> actions);

/// Компактная плитка полки: иконка в плашке, название и статус, справа —
/// значок воспроизведения или замка. В плашке — золотой силуэт
/// (ближайший по размеру уровень), а без него — глиф FontAwesome.
void GameTile(const char *title, const char *status, const char *icon, const IconSet *icons,
    bool available, const ImVec4 &accent, const ImVec2 &size, const std::function<void()> &onClick);

/// Золотая градиентная кнопка с тёмным жирным текстом — главное действие.
void PrimaryButton(const char *label, const ImVec2 &size, const std::function<void()> &onClick);

/// Контурная кнопка с золотым текстом — второстепенное действие.
void GhostButton(const char *icon, const char *label, const ImVec2 &size,
    const std::function<void()> &onClick);

/// Второстепенная кнопка с иконкой (для экранов и диалогов).
void IconButton(const char *icon, const char *label, bool accent, const ImVec2 &size,
    const std::function<void()> &onClick);

/// Текст цветом роли, по центру текущей ширины контента.
void CenteredText(const char *text, ColorRole role = ColorRole::TextBody);

/// Тумблер настройки: трек с анимированной ручкой, включённое
/// состояние — тёплое золото. Тап по всей площади переключает.
void ToggleSwitch(const char *strId, bool value, const std::function<void(bool)> &onChange);

/// Слайдер настройки на всю ширину строки: тонкий трек, золотая
/// заливка и ручка. Нажатие и перетаскивание задают значение
/// положением пальца.
void OptionSlider(const char *strId, int value, int minValue, int maxValue,
    const std::function<void(int)> &onChange);

/// Тащат ли сейчас слайдер (данные прошлого кадра) — жест прокрутки
/// страницы на это время не включается.
[[nodiscard]] bool IsSliderDragging();

/// Строка файла в чек-листе. Первая строка: маркер, имя, справа размер
/// (или «можно скачать»/«не найден») и действие — корзинка для
/// скачанного, кнопка «Скачать» для отсутствующего скачиваемого.
/// Ниже — путь целиком на всю ширину (перенос только по разделителям
/// «/» и «\\»).
void FileRow(bool present, const char *name, const char *status, const char *path,
    const std::function<void()> &onDownload, const std::function<void()> &onDelete);

/// Шапка второстепенного экрана: кнопка «←» слева (возврат на главный),
/// заголовок, золотой разделитель снизу.
void ScreenHeader(const char *title, const std::function<void()> &onBack);

} // namespace launcher::ui::widgets

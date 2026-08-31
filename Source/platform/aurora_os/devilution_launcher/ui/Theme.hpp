#pragma once

#include <imgui.h>

namespace launcher::ui {

/// Роль шрифта: шриты грузятся по назначению, а не по имени файла.
enum class FontRole {
	Body,     ///< Beaufort Regular + кириллица + иконки
	BodyBold, ///< Beaufort Bold + кириллица + иконки
	Heading,  ///< Exocet (только латиница — стиль логотипа Diablo)
	IconBig,  ///< крупные одиночные иконки
};

/// Роль цвета из палитры Diablo-темы.
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

/**
 * @brief Diablo-стиль: шрифты по ролям, палитра и стиль ImGui.
 *
 * Init() вызывается один раз после создания ImGui-контекста; остальные
 * функции можно звать только внутри кадра.
 */
namespace Theme {

/// Инициализирует шрифты, палитру и стиль. Затратная операция — один раз при старте.
void Init(float dpiScale);

/// Шрифт по роли; nullptr, если шрифт не загрузился.
[[nodiscard]] auto Font(FontRole role) -> ImFont *;
void PushFont(FontRole role);
void PopFont();

/// Цвет по роли.
[[nodiscard]] auto Color(ColorRole role) -> ImVec4;
[[nodiscard]] auto ColorU32(ColorRole role) -> ImU32;

/// Внешний вид кнопки в стиле Diablo (красная заливка, золотой контур).
void PushButtonStyle(bool accent = false);
void PopButtonStyle();

/// Золотая волосяная линия с затухающими концами — разделитель секций.
void DrawDivider(ImVec2 from, ImVec2 to, float alpha = 1.0F);

} // namespace Theme

} // namespace launcher::ui

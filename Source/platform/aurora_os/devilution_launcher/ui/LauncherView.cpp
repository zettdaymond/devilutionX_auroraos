#include "LauncherView.hpp"

#include "Animation.hpp"
#include "Format.hpp"
#include "Icons.hpp"
#include "Scale.hpp"
#include "Theme.hpp"
#include "dialogs/Dialogs.hpp"
#include "screens/Screens.hpp"
#include "thirdparty/FileBrowser.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

#ifndef LAUNCHER_APP_VERSION
#	define LAUNCHER_APP_VERSION "dev"
#endif

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>

namespace launcher::ui {

namespace {
/// Длительность iris-анимации при запуске игры (секунды).
constexpr float kIrisDuration = 0.35F;
}

LauncherView::LauncherView(float dpiScale)
    : m_dpiScale(dpiScale)
{
	// Начинаем обзор с домашней папки пользователя — вероятнее всего
	// MPQ лежат именно там (~/Documents, ~/Downloads).
	std::filesystem::path startDir;
#ifdef _WIN32
	if (const char *profile = std::getenv("USERPROFILE")) {
		startDir = profile;
	}
#else
	if (const char *home = std::getenv("HOME")) {
		startDir = home;
	}
#endif
	if (startDir.empty() || !std::filesystem::is_directory(startDir)) {
		startDir = std::filesystem::current_path();
	}

	m_fileBrowser = std::make_unique<ImGui::FileBrowser>(
	    ImGuiFileBrowserFlags_Fullscreen | ImGuiFileBrowserFlags_NoResize | ImGuiFileBrowserFlags_NoMove
	    | ImGuiFileBrowserFlags_NoTitleBar,
	    startDir);
	m_fileBrowser->SetTypeFilters({ "*.mpq", "*.MPQ" });
	m_fileBrowser->SetTitle("Выберите DIABDAT.MPQ");
	m_fileBrowser->SetMobileBehavior(true);
}

LauncherView::~LauncherView() = default;

void LauncherView::SetBackgroundTexture(void *texture, ImVec2 size)
{
	m_backgroundTexture = texture;
	m_backgroundTextureSize = size;
}

void LauncherView::SetHeroTexture(ExitAction mode, void *texture, ImVec2 size)
{
	m_heroArts[static_cast<size_t>(mode)] = widgets::BackgroundArt { texture, size };
}

void LauncherView::SetModeIconLevels(ExitAction mode, const widgets::BackgroundArt *levels, int count)
{
	widgets::IconSet &set = m_iconSets[static_cast<size_t>(mode)];
	set.count = std::min(count, widgets::IconSet::kMaxLevels);
	for (int i = 0; i < set.count; ++i) {
		set.levels[i] = levels[i];
	}
}

void LauncherView::Render(const LauncherState &state, const Dispatcher &dispatch)
{
	Scale::BeginFrame(m_dpiScale);

	// Жест «потянул — прокрутил»: если палец увести дальше порога нажатия,
	// кадр считается прокруткой, и клики на время жеста
	// игнорируются. Исключение — перетаскивание слайдера: движение
	// принадлежит ползунку и страницу прокручивать не должно.
	ImGuiIO &io = ImGui::GetIO();
	if (ImGui::IsMouseDown(0) && !widgets::IsSliderDragging()) {
		const float dragDistance = std::sqrt(io.MouseDragMaxDistanceSqr[0]);
		if (dragDistance > Scale::Px(0.6F)) {
			m_gestureDrag = true;
		}
	}
	Dispatcher guardedDispatch = [this, &dispatch](Intent intent) {
		if (!m_gestureDrag) {
			dispatch(intent);
		}
	};

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	// ~16dp на устройстве: как базовые поля мобильных платформ — текст
	// дышит, интерактив не лазит в жестовую зону у края.
	const float pad = Scale::Px(1.5F);
	// Небольшой отступ от края экрана, чтобы системные жесты телефона
	// не обрезали контент.
	const float bottomInset = Scale::Px(0.35F);

	// Корневое окно занимает весь экран, без рамок и заголовка.
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	// Корень — прозрачный контейнер: флаг NoInputs не даёт ему перекрыть
	// прочие окна и перехватывать клики при пересортировке окон ImGui
	// (дочерние окна остаются кликабельными сами по себе).
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
	    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
	    | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;

	ImGui::Begin("##launcher-root", nullptr, flags);
	RenderBackground();

	// Контент занимает всё окно. Родной скроллбар скрыт (десктопная
	// идиома, крадущая ширину) — позицию прокрутки показывает тонкий
	// оверлей-индикатор у края, контент держит симметричные поля. На
	// главном экране снизу резерв под плавающие кнопки разделов.
	const bool isHome = (state.screen == Screen::Home);
	// Квадратные кнопки разделов стали выше прежних широких — резерв
	// держим с запасом над кластером (высота кнопки + отступ).
	const float bottomReserve = isHome ? Scale::Px(4.2F) : pad * 0.5F;
	const ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 contentPos(0.0F, pad * 0.5F);
	ImVec2 contentSize(windowSize.x,
	    windowSize.y - pad - bottomReserve - bottomInset);
	if (!isHome) {
		// Вторичные экраны (Данные/Настройки/Инфо): в ландшафте столбец
		// ограничиваем и центрируем — как контентный блок главной, иначе
		// строки и кнопки растягиваются на всю ширину простынёй. Сужаем
		// сам child: вся раскладка строк (FileRow, SameLine-выравнивание)
		// считает ширину от его GetContentRegionAvail.
		const float capped = std::min(contentSize.x, Scale::Px(kLandscapeContentMaxRem) + pad * 2.0F);
		contentPos.x = std::max(0.0F, (windowSize.x - capped) * 0.5F);
		contentSize.x = capped;
	}

	// Вторичные экраны: шапка с кнопкой «назад» закреплена над списком и
	// не прокручивается — «назад» всегда в одном касании, как в
	// нативных приложениях (корневое окно NoInputs, поэтому шапка —
	// отдельное прозрачное окно, как кластер кнопок главной).
	if (!isHome) {
		const char *title = state.screen == Screen::Data   ? "Данные"
		    : state.screen == Screen::Settings             ? "Настройки"
		                                                   : "Инфо";
		const float headerHeight = RenderScreenHeader(title, contentPos, contentSize.x, guardedDispatch);
		contentPos.y += headerHeight;
		contentSize.y = std::max(64.0F, contentSize.y - headerHeight);
	}

	ImGui::SetCursorPos(contentPos);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, 0));
	float contentScrollY = 0.0F;
	float contentScrollMaxY = 0.0F;
	ImVec2 contentTopLeft;
	float contentHeight = 0.0F;
	// AlwaysUseWindowPadding обязателен: child-окна без рамки получают
	// WindowPadding(0,0) от ImGui, и контент прилипает к краям.
	if (ImGui::BeginChild("##content", contentSize, ImGuiChildFlags_AlwaysUseWindowPadding,
	        ImGuiWindowFlags_NoScrollbar)) {
		contentTopLeft = ImGui::GetWindowPos();
		contentHeight = ImGui::GetWindowHeight();
		contentScrollY = ImGui::GetScrollY();
		contentScrollMaxY = ImGui::GetScrollMaxY();
		const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows
		    | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		// Пока жест активен и палец над контентом — применяем дельту
		// движения и копим след позиций для расчёта скорости броска.
		if (m_gestureDrag && hovered) {
			ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
			if (m_flickTrailLen < static_cast<int>(std::size(m_flickTrail))) {
				++m_flickTrailLen;
			}
			std::memmove(m_flickTrail + 1, m_flickTrail,
			    static_cast<size_t>(m_flickTrailLen - 1) * sizeof(FlickSample));
			m_flickTrail[0] = FlickSample { ImGui::GetTime(), io.MousePos.y };
			m_flickActive = true;
		} else if (m_flickActive && !ImGui::IsMouseDown(0)) {
			// Подобранные на устройстве константы затухания инерции.
#ifdef LAUNCHER_EXPONENTIAL_GLIDE
			constexpr float flickK = 6.0F;
#else
			constexpr float flickK = 2800.0F;
#endif
			if (m_flickTrailLen > 0) {
				// Скорость — по окну ~120 мс: палец тормозит перед
				// подъёмом, скорость последних кадров занижена. Берём
				// САМЫЙ СТАРЫЙ образец не старше 120 мс (след хранится
				// от свежего к старому, ищем с конца).
				const double now = ImGui::GetTime();
				int base = 0;
				for (int i = m_flickTrailLen - 1; i >= 0; --i) {
					if (now - m_flickTrail[i].time <= 0.12) {
						base = i;
						break;
					}
				}
				const double window = now - m_flickTrail[base].time;
				m_flickSpeed = window > 1.0e-3
				    ? -(io.MousePos.y - m_flickTrail[base].y) / static_cast<float>(window)
				    : 0.0F;
				m_flickTrailLen = 0;
			}
			ImGui::SetScrollY(ImGui::GetScrollY() + m_flickSpeed * io.DeltaTime);
#ifdef LAUNCHER_EXPONENTIAL_GLIDE
			// Вариант А: экспоненциальное затухание (длинное «планирование»,
			// мягкий хвост).
			m_flickSpeed *= std::exp(-flickK * io.DeltaTime);
			const bool glideStopped = std::abs(m_flickSpeed) < 40.0F;
#else
			// Вариант Б (по умолчанию): равномерное торможение и решительный
			// стоп — экспоненциальный хвост на 60 Гц экране читался как
			// «лаг». У краёв списка и при новом касании — стоп.
			const float decel = flickK * io.DeltaTime;
			m_flickSpeed -= std::copysign(std::min(std::abs(m_flickSpeed), decel), m_flickSpeed);
			const bool glideStopped = m_flickSpeed == 0.0F;
#endif
			const float y = ImGui::GetScrollY();
			if (glideStopped || y <= 0.0F || y >= contentScrollMaxY) {
				m_flickActive = false;
				m_flickSpeed = 0.0F;
			}
		}
		// Появление экрана: fade + лёгкий подъём снизу.
		if (state.screen != m_lastScreen) {
			m_lastScreen = state.screen;
			m_screenShownAt = ImGui::GetTime();
			// Индикатор мигает при входе на экран: даёт понять, что ниже
			// есть контент.
			m_scrollActiveAt = ImGui::GetTime();
			m_flickActive = false;
			m_flickSpeed = 0.0F;
			m_flickTrailLen = 0;
		}
		// Новое нажатие стопит инерцию — но не во время самого жеста:
		// иначе след позиций стирается каждый кадр зажатой кнопки и
		// скорость броска всегда нулевая.
		if (ImGui::IsMouseDown(0) && !m_gestureDrag) {
			m_flickActive = false;
			m_flickSpeed = 0.0F;
			m_flickTrailLen = 0;
		}
		const float appearK = EaseOutCubic(ElapsedFraction(m_screenShownAt, ImGui::GetTime(), 0.20F));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (1.0F - appearK) * Scale::Px(0.6F));
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, appearK);
		RenderScreen(state, guardedDispatch);
		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
	RenderScrollIndicator(contentScrollY, contentScrollMaxY, contentTopLeft, contentHeight);

	ImGui::End();
	ImGui::PopStyleVar();

	if (isHome) {
		RenderQuickActions(guardedDispatch);
	}
	RenderDialogs(state, guardedDispatch);
	RenderFileBrowser(state, dispatch);
	dialogs::Toast(state, guardedDispatch);
	RenderLaunchIris(state);

	// Жест заканчивается отпусканием кнопки — сброс ПОСЛЕ отрисовки,
	// чтобы клик в кадре отпускания ещё подавлялся.
	if (!ImGui::IsMouseDown(0)) {
		m_gestureDrag = false;
	}
}

void LauncherView::NotifyShown()
{
	// RenderDialogs открывает ImGui-попап только на СМЕНУ state.dialog.
	// За кадры обложки (ImGui без окон) попап умер — сбрасываем кэш, и
	// первый видимый кадр переоткроет активный диалог (с fade).
	m_lastDialog = Dialog::None;
}

bool LauncherView::LaunchIrisDone() const
{
	if (m_irisStartedAt < 0.0) {
		return false;
	}
	return ElapsedFraction(m_irisStartedAt, ImGui::GetTime(), kIrisDuration) >= 1.0F;
}

/// Затемняет всё вне круга: четыре полосы вокруг описанного квадрата плюс
/// угловые веера треугольников для квадрата минус диск. Хорды дуг
/// (размах 90°) отделяют угол квадрата от центра, поэтому треугольники
/// не залезают внутрь круга.
void DrawIrisHole(ImDrawList *draw, const ImVec2 &center, float radius, const ImVec2 &a, const ImVec2 &b, ImU32 dark)
{
	const float x0 = std::max(a.x, center.x - radius);
	const float x1 = std::min(b.x, center.x + radius);
	const float y0 = std::max(a.y, center.y - radius);
	const float y1 = std::min(b.y, center.y + radius);
	if (x0 > a.x) {
		draw->AddRectFilled(ImVec2(a.x, a.y), ImVec2(x0, b.y), dark);
	}
	if (x1 < b.x) {
		draw->AddRectFilled(ImVec2(x1, a.y), ImVec2(b.x, b.y), dark);
	}
	if (y0 > a.y) {
		draw->AddRectFilled(ImVec2(a.x, a.y), ImVec2(b.x, y0), dark);
	}
	if (y1 < b.y) {
		draw->AddRectFilled(ImVec2(a.x, y1), ImVec2(b.x, b.y), dark);
	}
	if (x0 >= x1 || y0 >= y1) {
		return; // круг шире экрана — полосы уже всё закрыли
	}

	constexpr int kSegments = 8;
	constexpr float kPi = 3.14159265F;
	struct Corner {
		ImVec2 v;
		float angle0;
		float angle1;
	};
	const Corner corners[] {
		{ ImVec2(x0, y0), kPi, 1.5F * kPi },        // верхний левый
		{ ImVec2(x1, y0), 1.5F * kPi, 2.0F * kPi }, // верхний правый
		{ ImVec2(x1, y1), 0.0F, 0.5F * kPi },       // нижний правый
		{ ImVec2(x0, y1), 0.5F * kPi, kPi },        // нижний левый
	};
	for (const Corner &corner : corners) {
		ImVec2 prev(center.x + radius * std::cos(corner.angle0), center.y + radius * std::sin(corner.angle0));
		for (int i = 1; i <= kSegments; ++i) {
			const float angle = corner.angle0 + (corner.angle1 - corner.angle0) * (static_cast<float>(i) / kSegments);
			const ImVec2 point(center.x + radius * std::cos(angle), center.y + radius * std::sin(angle));
			draw->AddTriangleFilled(corner.v, prev, point, dark);
			prev = point;
		}
	}
}

/// Тлеющие угольки, всплывающие над артом — отсылка к огню в главном меню
/// Diablo. Частицы бессостоятельные: позиция — чистая функция времени и
/// индекса, поэтому анимация бесплатна и для паузы в фоне ничего
/// сохранять не нужно.
void DrawEmbers(ImDrawList *draw, const ImGuiViewport *viewport)
{
	constexpr int kEmberCount = 26;
	const float time = static_cast<float>(ImGui::GetTime());
	for (int i = 0; i < kEmberCount; ++i) {
		const float seed = static_cast<float>(i) * 0.618034F; // золотое сечение — рассинхрон фаз
		const float cycle = 9.0F + 6.0F * std::sin(seed * 13.7F); // период всплытия 3–15 с
		const float phase = std::fmod(time / cycle + seed, 1.0F); // 0..1 за виток

		const float x01 = 0.08F + 0.84F * std::fmod(seed * 7.31F, 1.0F)
		    + 0.02F * std::sin(time * (0.6F + 0.3F * std::sin(seed * 3.1F)) + seed * 9.0F);
		const float y01 = 1.02F - phase * 1.08F; // от низа до чуть выше верха

		const float fade = std::sin(phase * 3.14159265F); // появление и растворение
		const float alpha = 0.32F * fade;
		if (alpha <= 0.01F) {
			continue;
		}
		const float radius = Scale::Px(0.08F + 0.06F * std::sin(seed * 5.9F));
		const ImVec2 center(viewport->WorkPos.x + x01 * viewport->WorkSize.x,
		    viewport->WorkPos.y + y01 * viewport->WorkSize.y);

		// Тёплые оттенки: от глубокого красного к золоту.
		const ImVec4 tint(0.91F - 0.35F * std::sin(seed * 2.3F), 0.52F - 0.28F * std::sin(seed * 2.3F),
		    0.16F, alpha);
		draw->AddCircleFilled(center, radius, ImGui::ColorConvertFloat4ToU32(tint), 6);
	}
}

/// Iris-out: экран закрывается сужающимся кругом перед уходом в движок —
/// как титры старых игр. Полностью чисто view-слой: Store уже зафиксировал
/// pendingLaunch, мы лишь догружаем последний кадр анимацией.
void LauncherView::RenderLaunchIris(const LauncherState &state)
{
	if (!state.pendingLaunch.has_value()) {
		m_irisStartedAt = -1.0;
		return;
	}
	if (m_irisStartedAt < 0.0) {
		m_irisStartedAt = ImGui::GetTime();
	}
	const float eased = EaseInQuad(ElapsedFraction(m_irisStartedAt, ImGui::GetTime(), kIrisDuration));

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const ImVec2 a = viewport->WorkPos;
	const ImVec2 b = viewport->WorkPos + viewport->WorkSize;
	const ImVec2 center((a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F);
	const float maxRadius = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)) * 0.5F;
	const float radius = maxRadius * (1.0F - eased);

	ImDrawList *draw = ImGui::GetForegroundDrawList();
	const ImU32 dark = ImGui::GetColorU32(ImVec4(0.02F, 0.01F, 0.01F, 1.0F));
	if (radius <= 1.0F) {
		draw->AddRectFilled(a, b, dark);
		return;
	}
	DrawIrisHole(draw, center, radius, a, b, dark);

	// Тлеющий обод — круг закрывается не тьмой, а догорающим огнём.
	const float time = static_cast<float>(ImGui::GetTime());
	const float flicker = 0.70F + 0.30F * std::sin(time * 17.0F + 2.0F * std::sin(time * 6.3F));
	const float rimAlpha = 0.55F * flicker * (1.0F - eased * 0.5F);
	draw->AddCircle(center, radius, ImGui::GetColorU32(ImVec4(1.0F, 0.58F, 0.16F, rimAlpha)), 48, Scale::Px(0.1F));
	draw->AddCircle(center, radius * 0.96F,
	    ImGui::GetColorU32(ImVec4(0.91F, 0.55F, 0.16F, rimAlpha * 0.6F)), 48, Scale::Px(0.18F));
}

void LauncherView::RenderCover(const LauncherState &state)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	// Дегенеративный вьюпорт (свёрнутое окно) — рисовать нечего, а
	// нулевые размеры дальше дают шрифт 0 (assert imgui_draw).
	if (viewport->WorkSize.x <= 0.0F || viewport->WorkSize.y <= 0.0F) {
		return;
	}
	// Текст на background-списке в этой паре ImGui/SDL_Renderer не
	// рендерится (картинки — рендерятся, глифы — нет), поэтому вся
	// обложка рисуется в foreground-списке: в кадре обложки больше
	// ничего нет, так что «поверх всего» — то, что нужно.
	ImDrawList *draw = ImGui::GetForegroundDrawList();
	const ImVec2 &top = viewport->WorkPos;
	const ImVec2 &v = viewport->WorkSize;
	const float centerX = top.x + v.x * 0.5F;

	const auto argb = [](ColorRole role) {
		return ImGui::GetColorU32(Theme::Color(role));
	};

	// Фон — тот же aspect-fill кроп, что у главного экрана.
	if (m_backgroundTexture != nullptr && m_backgroundTextureSize.x > 0.0F && m_backgroundTextureSize.y > 0.0F) {
		const ImVec2 &t = m_backgroundTextureSize;
		const float scale = std::max(v.x / t.x, v.y / t.y);
		const ImVec2 shown(t.x * scale, t.y * scale);
		const ImVec2 crop(0.5F - (v.x / shown.x) * 0.5F, 0.5F - (v.y / shown.y) * 0.5F);
		draw->AddImage(m_backgroundTexture, top, top + v,
		    ImVec2(crop.x, crop.y), ImVec2(1.0F - crop.x, 1.0F - crop.y));
	}

	// Затемнение плотнее, чем на главном экране: плитка мелкая, тексту
	// нужен контраст. Угольки не рисуем — кадр статичный между правками.
	draw->AddRectFilled(top, top + v, ImGui::GetColorU32(ImVec4(0.02F, 0.01F, 0.01F, 0.45F)));

	// Композитор кропает буфер под пропорции плитки — контент держим
	// в центральной полосе шириной ~72%, края небезопасны.
	const float contentWidth = v.x * 0.72F;
	float y = top.y + v.y * 0.24F;

	// Логотип Exocet. Размер — ДОЛЯ КОРОТКОЙ СТОРОНЫ окна, а не DPI:
	// композитор вписывает буфер в плитку с сильным уменьшением, и на
	// больших экранах с низким DPI (эмулятор планшета, dpi~0.6)
	// DPI-масштабированный шрифт в карточке неразличим. 10% короткой
	// стороны = выверенные на телефоне 72px при 720-широком окне.
	ImFont *heading = Theme::Font(FontRole::Heading);
	if (heading != nullptr) {
		constexpr const char *kTitle = "DEVILUTIONX";
		const float size = Scale::MinSide() * 0.10F;
		const ImVec2 textSize = heading->CalcTextSizeA(size, FLT_MAX, 0.0F, kTitle);
		draw->AddText(heading, size, ImVec2(centerX - textSize.x * 0.5F, y),
		    argb(ColorRole::GoldBright), kTitle);
		y += textSize.y + Scale::Px(0.55F);
	}

	// Подпись с версией и разделитель. Та же пропорция от короткой
	// стороны (~5.5% ≈ 40px на телефоне).
	{
		ImFont *body = Theme::Font(FontRole::Body);
		const std::string subtitle = std::string("порт для Aurora OS · ") + LAUNCHER_APP_VERSION;
		const float subtitleSize = Scale::MinSide() * 0.055F;
		const ImVec2 textSize = body != nullptr
		    ? body->CalcTextSizeA(subtitleSize, FLT_MAX, 0.0F, subtitle.c_str())
		    : ImGui::CalcTextSize(subtitle.c_str());
		if (body != nullptr) {
			draw->AddText(body, subtitleSize, ImVec2(centerX - textSize.x * 0.5F, y),
			    argb(ColorRole::TextDim), subtitle.c_str());
		}
		y += textSize.y + Scale::Px(0.9F);
	}
	Theme::DrawDivider(draw, ImVec2(centerX - contentWidth * 0.5F, y), ImVec2(centerX + contentWidth * 0.5F, y),
	    0.7F);

	// Активная загрузка: имя файла, полоса с «горячим» краем, процент и
	// скорость — упрощённый вариант бара экрана данных.
	if (state.DownloadInProgress()) {
		const DownloadState &d = *state.download;
		const FileSpec &spec = FileSpecOf(d.file);
		// Отрезаем пояснение в скобках («демо-версия») — имя файла в
		// плитке узнаётся и без него.
		const std::string_view displayName = spec.displayName.substr(0, spec.displayName.find(" ("));

		// Все размеры прогресса — доли короткой стороны (как логотип
		// выше): низкий DPI не должен прятать загрузку в плитке.
		const float nameSizePx = Scale::MinSide() * 0.055F;
		const float statSizePx = Scale::MinSide() * 0.045F;
		ImFont *bodyBold = Theme::Font(FontRole::BodyBold);
		ImFont *body = Theme::Font(FontRole::Body);

		float dy = top.y + v.y * 0.50F;
		if (bodyBold != nullptr) {
			const ImVec2 nameSize = bodyBold->CalcTextSizeA(nameSizePx, FLT_MAX, 0.0F,
			    displayName.data(), displayName.data() + displayName.size());
			draw->AddText(bodyBold, nameSizePx, ImVec2(centerX - nameSize.x * 0.5F, dy),
			    argb(ColorRole::TextBody),
			    displayName.data(), displayName.data() + displayName.size());
			dy += nameSize.y + Scale::Px(0.7F);
		}

		// Полоса: тёмный трек в золотой рамке, заливка и мягкий свет
		// на переднем крае.
		const float barHeight = Scale::MinSide() * 0.028F;
		const float barLeft = centerX - contentWidth * 0.5F;
		draw->AddRectFilled(ImVec2(barLeft, dy), ImVec2(barLeft + contentWidth, dy + barHeight),
		    argb(ColorRole::Panel));
		const float fill = contentWidth * std::clamp(d.fraction, 0.0F, 1.0F);
		if (fill > 0.0F) {
			draw->AddRectFilled(ImVec2(barLeft, dy), ImVec2(barLeft + fill, dy + barHeight),
			    argb(ColorRole::GoldDim));
			draw->AddCircleFilled(ImVec2(barLeft + fill, dy + barHeight * 0.5F), barHeight,
			    ImGui::GetColorU32(Theme::Color(ColorRole::GoldBright)
			        * ImVec4(1.0F, 1.0F, 1.0F, 0.35F)),
			    12);
		}
		draw->AddRect(ImVec2(barLeft, dy), ImVec2(barLeft + contentWidth, dy + barHeight),
		    argb(ColorRole::BorderGold), 1.0F);

		if (body != nullptr) {
			dy += barHeight + Scale::Px(0.55F);
			const std::string stat = std::to_string(static_cast<int>(d.fraction * 100.0F + 0.5F)) + "% · "
			    + FormatBytes(d.bytesPerSec) + "/с";
			const ImVec2 statSize = body->CalcTextSizeA(statSizePx, FLT_MAX, 0.0F, stat.c_str());
			draw->AddText(body, statSizePx, ImVec2(centerX - statSize.x * 0.5F, dy),
			    argb(ColorRole::TextDim), stat.c_str());
		}
	}
}

void LauncherView::RenderBackground() const
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImDrawList *draw = ImGui::GetBackgroundDrawList();

	if (m_backgroundTexture != nullptr && m_backgroundTextureSize.x > 0 && m_backgroundTextureSize.y > 0) {
		// Aspect-fill: кропим картинку так, чтобы она закрыла весь вьюпорт.
		const ImVec2 &v = viewport->WorkSize;
		const ImVec2 &t = m_backgroundTextureSize;
		const float scale = std::max(v.x / t.x, v.y / t.y);
		const ImVec2 shown(t.x * scale, t.y * scale);
		const ImVec2 crop(0.5F - (v.x / shown.x) * 0.5F, 0.5F - (v.y / shown.y) * 0.5F);
		draw->AddImage(m_backgroundTexture, viewport->WorkPos, viewport->WorkPos + v,
		    ImVec2(crop.x, crop.y), ImVec2(1.0F - crop.x, 1.0F - crop.y));
	}

	DrawEmbers(draw, viewport);

	// Лёгкая виньетка, чтобы текст читался поверх арта; полупрозрачность
	// сохраняет угольки видимыми сквозь затемнение.
	const ImU32 shade = ImGui::GetColorU32(ImVec4(0.02F, 0.01F, 0.01F, 0.28F));
	draw->AddRectFilled(viewport->WorkPos, viewport->WorkPos + viewport->WorkSize, shade);
}

void LauncherView::RenderScrollIndicator(float scrollY, float scrollMaxY, const ImVec2 &topLeft, float height)
{
	if (scrollMaxY <= 0.0F) {
		m_lastContentScrollY = scrollY;
		return;
	}
	const double now = ImGui::GetTime();
	if (std::abs(scrollY - m_lastContentScrollY) > 0.1F) {
		m_scrollActiveAt = now;
	}
	m_lastContentScrollY = scrollY;
	if (m_scrollActiveAt < 0.0) {
		m_scrollActiveAt = now;
	}

	// Мгновенное появление, полусекунды покоя, растворение за 0.35 с —
	// как индикаторы прокрутки на мобильных платформах.
	const float since = static_cast<float>(now - m_scrollActiveAt);
	const float alpha = std::clamp((0.85F - since) / 0.35F, 0.0F, 1.0F);
	if (alpha <= 0.0F) {
		return;
	}

	const float barW = Scale::Px(0.14F);
	const float inset = Scale::Px(0.1F);
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float x0 = viewport->WorkPos.x + viewport->WorkSize.x - inset - barW;

	// Доля видимой части экрана → высота бегунка (не короче 1.5rem).
	const float fraction = height / (height + scrollMaxY);
	const float thumbH = std::max(height * fraction, Scale::Px(1.5F));
	const float travel = height - thumbH;
	const float thumbY = topLeft.y + (scrollMaxY > 0.0F ? travel * (scrollY / scrollMaxY) : 0.0F);

	ImDrawList *draw = ImGui::GetWindowDrawList();
	draw->AddRectFilled(ImVec2(x0, thumbY), ImVec2(x0 + barW, thumbY + thumbH),
	    ImGui::GetColorU32(ImVec4(0.91F, 0.78F, 0.49F, 0.85F * alpha)), barW * 0.5F);
}

/// Закреплённая шапка вторичного экрана: собственное прозрачное окно
/// (корневое окно NoInputs и не принимает нажатия), содержимое —
/// готовый виджет ScreenHeader с паддингом колонки контента.
float LauncherView::RenderScreenHeader(const char *title, const ImVec2 &pos, float width,
    const Dispatcher &dispatch)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float pad = Scale::Px(1.5F);
	// Небольшой вертикальный воздух: без него рамка кнопки-стрелки
	// подрезалась верхним краем окна на устройстве.
	const float vpad = Scale::Px(0.15F);
	// Кнопка ScreenHeader + воздух до/после разделителя.
	const float height = vpad * 2.0F + Scale::Px(2.3F) + Scale::Px(0.35F) + Scale::Px(0.06F) + Scale::Px(0.5F);

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(width, height));
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, vpad));
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.0F);
	ImGui::Begin("##screen-header", nullptr,
	    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
	        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar
	        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
	widgets::ScreenHeader(title, [&dispatch] { dispatch(intent::UiNavigate { Screen::Home }); });
	ImGui::End();
	ImGui::PopStyleVar(2);
	return height;
}

/// Плавающие кнопки разделов в правом нижнем углу главного экрана:
/// «Данные», «Настройки», «Инфо» — второстепенные действия и не
/// заслуживают вкладок; кнопки висят поверх контента, всегда в одном
/// жесте от любой прокрутки. Квадратные с иконкой и короткой подписью:
/// три широкие в ряд уже не влезают в телефон.
void LauncherView::RenderQuickActions(const Dispatcher &dispatch)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float buttonSize = Scale::Px(3.7F);
	const float gap = Scale::Px(0.4F);
	const float sideInset = Scale::Px(0.6F);
	const float bottomInset = Scale::Px(0.35F) + Scale::Px(0.3F);
	const ImVec2 clusterSize(buttonSize * 3.0F + gap * 2.0F, buttonSize);
	const ImVec2 pos(viewport->WorkPos.x + viewport->WorkSize.x - clusterSize.x - sideInset,
	    viewport->WorkPos.y + viewport->WorkSize.y - clusterSize.y - bottomInset);

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(clusterSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Scale::Px(0.4F));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
	ImGui::Begin("##quick-actions", nullptr,
	    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
	        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

	struct QuickItem {
		Screen screen;
		const char *icon;
		const char *label;
	};
	const QuickItem items[] {
		{ Screen::Data, icons::Folder, "Данные" },
		{ Screen::Settings, icons::Cog, "Настройки" },
		{ Screen::About, icons::Info, "Инфо" },
	};
	int index = 0;
	for (const QuickItem &item : items) {
		ImGui::PushStyleColor(ImGuiCol_Button, Theme::Color(ColorRole::Panel));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::Color(ColorRole::PanelHover));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::Color(ColorRole::RedPressed));
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextBody));
		char buttonId[32];
		std::snprintf(buttonId, sizeof(buttonId), "##quick-%d", index++);
		if (ImGui::Button(buttonId, ImVec2(buttonSize, buttonSize))) {
			dispatch(intent::UiNavigate { item.screen });
		}

		// Иконка сверху по центру, под ней короткая подпись — у кнопки
		// двухстрочное содержимое, обычный label ImGui не переносится.
		ImDrawList *draw = ImGui::GetWindowDrawList();
		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();
		ImFont *font = Theme::Font(FontRole::Body);
		const float iconSize = Scale::Px(1.35F);
		const ImVec2 iconSize2 = font != nullptr ? font->CalcTextSizeA(iconSize, FLT_MAX, 0.0F, item.icon) : ImVec2(0, 0);
		const float labelSize = Scale::Px(0.78F);
		const std::string label = widgets::FitTextEllipsis(font, labelSize, item.label, buttonSize - Scale::Px(0.6F));
		const ImVec2 labelSize2 = font != nullptr ? font->CalcTextSizeA(labelSize, FLT_MAX, 0.0F, label.c_str()) : ImVec2(0, 0);
		const float blockH = iconSize2.y + Scale::Px(0.35F) + labelSize2.y;
		const float topY = min.y + (buttonSize - blockH) * 0.5F;
		draw->AddText(font, iconSize, ImVec2(min.x + (buttonSize - iconSize2.x) * 0.5F, topY),
		    Theme::ColorU32(ColorRole::GoldBright), item.icon);
		draw->AddText(font, labelSize,
		    ImVec2(min.x + (buttonSize - labelSize2.x) * 0.5F, topY + iconSize2.y + Scale::Px(0.35F)),
		    Theme::ColorU32(ColorRole::TextBody), label.c_str());

		ImGui::SameLine(0, gap);
		ImGui::PopStyleColor(4);
	}

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void LauncherView::RenderScreen(const LauncherState &state, const Dispatcher &dispatch)
{
	const auto modeArt = [this](ExitAction mode) {
		const size_t i = static_cast<size_t>(mode);
		return screens::ModeArt { m_heroArts[i], m_iconSets[i] };
	};
	const screens::ArtSet art {
		{ m_backgroundTexture, m_backgroundTextureSize },
		modeArt(ExitAction::LaunchDiablo),
		modeArt(ExitAction::LaunchHellfire),
		modeArt(ExitAction::LaunchDemo),
	};
	switch (state.screen) {
	case Screen::Home:
		screens::Home(state, dispatch, art);
		break;
	case Screen::Data:
		screens::Data(state, dispatch);
		break;
	case Screen::Settings:
		screens::Settings(state, dispatch);
		break;
	case Screen::About:
		screens::About(state, dispatch);
		break;
	}
}

void LauncherView::RenderDialogs(const LauncherState &state, const Dispatcher &dispatch)
{
	if (state.dialog != m_lastDialog) {
		if (state.dialog != Dialog::None) {
			dialogs::OpenFor(state.dialog);
			m_dialogShownAt = ImGui::GetTime();
		}
		m_lastDialog = state.dialog;
	}

	// Появление диалога: быстрый fade.
	const float dialogAlpha = EaseOutCubic(ElapsedFraction(m_dialogShownAt, ImGui::GetTime(), 0.15F));
	if (dialogAlpha < 1.0F) {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dialogAlpha);
	}

	switch (state.dialog) {
	case Dialog::ConfirmDownloadDemo:
		dialogs::confirm::Download(state, dispatch, KnownFile::Spawn);
		break;
	case Dialog::ConfirmDownloadRu:
		dialogs::confirm::Download(state, dispatch, KnownFile::RuVoice);
		break;
	case Dialog::DownloadProgress:
		dialogs::overlay::Download(state, dispatch);
		break;
	case Dialog::HellfireMissingFiles:
		dialogs::MissingFiles(state, dispatch);
		break;
	case Dialog::ConfirmResetSettings:
		dialogs::confirm::ResetSettings(state, dispatch);
		break;
	case Dialog::Error:
		dialogs::Error(state, dispatch);
		break;
	case Dialog::None:
		break;
	}

	if (dialogAlpha < 1.0F) {
		ImGui::PopStyleVar();
	}
}

void LauncherView::RenderFileBrowser(const LauncherState &state, const Dispatcher &dispatch)
{
	if (state.fileBrowserOpen && !m_fileBrowser->IsOpened()) {
		m_fileBrowser->Open();
	} else if (!state.fileBrowserOpen && m_fileBrowser->IsOpened()) {
		m_fileBrowser->Close();
	}

	m_fileBrowser->Display();

	// The browser closed itself (Отмена / × / Esc) — sync the state,
	// иначе блок выше сразу же открыл бы его снова.
	if (state.fileBrowserOpen && !m_fileBrowser->IsOpened()) {
		dispatch(intent::CancelFolderSelection {});
	}

	if (m_fileBrowser->HasSelected()) {
		const std::filesystem::path selected = m_fileBrowser->GetSelected();
		m_fileBrowser->ClearSelected();
		dispatch(intent::DataFolderSelected { selected.parent_path() });
	}
}

} // namespace launcher::ui

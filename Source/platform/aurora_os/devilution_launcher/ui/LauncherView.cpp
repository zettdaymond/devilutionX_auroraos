#include "LauncherView.hpp"

#include "Animation.hpp"
#include "Icons.hpp"
#include "Scale.hpp"
#include "Theme.hpp"
#include "dialogs/Dialogs.hpp"
#include "screens/Screens.hpp"
#include "thirdparty/FileBrowser.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
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
	// Start browsing from the user's home directory — the most likely
	// place for transferred MPQ files (~/Documents, ~/Downloads).
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

void LauncherView::SetModeIconTexture(ExitAction mode, void *texture, ImVec2 size)
{
	m_iconArts[static_cast<size_t>(mode)] = widgets::BackgroundArt { texture, size };
}

void LauncherView::Render(const LauncherState &state, const Dispatcher &dispatch)
{
	Scale::beginFrame(m_dpiScale);

	// Drag-to-scroll gesture: once the pointer moves further than a tap
	// threshold while held down, the frame is in "scrolling" mode —
	// intents from clicks are swallowed for its duration.
	ImGuiIO &io = ImGui::GetIO();
	if (ImGui::IsMouseDown(0)) {
		const float dragDistance = std::sqrt(io.MouseDragMaxDistanceSqr[0]);
		if (dragDistance > Scale::px(0.6F)) {
			m_gestureDrag = true;
		}
	}
	Dispatcher guardedDispatch = [this, &dispatch](Intent intent) {
		if (!m_gestureDrag) {
			dispatch(intent);
		}
	};

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float navHeight = Scale::px(3.2F);
	// ~16dp на устройстве: как базовые поля мобильных платформ — текст
	// дышит, интерактив не лазит в жестовую зону у края.
	const float pad = Scale::px(1.5F);
	// Keep a small gap above the screen edge so the nav bar is never
	// clipped by system gesture areas on phones.
	const float bottomInset = Scale::px(0.35F);

	// Root window: covers the viewport, no chrome.
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	// The root is a transparent layout container: NoInputs keeps it from
	// covering the navbar and stealing clicks when ImGui reorders windows
	// (children of a NoInputs window remain interactive on their own).
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
	    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
	    | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;

	ImGui::Begin("##launcher-root", nullptr, flags);
	RenderBackground();

	// Content area: the nav bar reserves space at the bottom (portrait)
	// or at the top (landscape). Родной скроллбар скрыт (десктопная
	// идиома, крадущая ширину) — позицию прокрутки показывает тонкий
	// оверлей-индикатор у края, контент держит симметричные поля.
	const ImVec2 windowSize = ImGui::GetWindowSize();
	const ImVec2 contentPos(0.0F, Scale::portrait() ? pad : navHeight + pad * 0.5F);
	const ImVec2 contentSize(windowSize.x,
	    windowSize.y - navHeight - pad * 1.5F - bottomInset);

	ImGui::SetCursorPos(contentPos);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, 0));
	float contentScrollY = 0.0F;
	float contentScrollMaxY = 0.0F;
	ImVec2 contentTopLeft;
	float contentHeight = 0.0F;
	if (ImGui::BeginChild("##content", contentSize, ImGuiChildFlags_None,
	        ImGuiWindowFlags_NoScrollbar)) {
		contentTopLeft = ImGui::GetWindowPos();
		contentHeight = ImGui::GetWindowHeight();
		contentScrollY = ImGui::GetScrollY();
		contentScrollMaxY = ImGui::GetScrollMaxY();
		// Apply the drag delta to the content scroll while the gesture
		// is active and the pointer is over the content area.
		if (m_gestureDrag
		    && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
			ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
		}
		// Появление экрана: fade + лёгкий подъём снизу.
		if (state.screen != m_lastScreen) {
			m_lastScreen = state.screen;
			m_screenShownAt = ImGui::GetTime();
			// Индикатор мигает при входе на экран: даёт понять, что ниже
			// есть контент.
			m_scrollActiveAt = ImGui::GetTime();
		}
		const float appearK = EaseOutCubic(ElapsedFraction(m_screenShownAt, ImGui::GetTime(), 0.20F));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (1.0F - appearK) * Scale::px(0.6F));
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, appearK);
		RenderScreen(state, guardedDispatch);
		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
	RenderScrollIndicator(contentScrollY, contentScrollMaxY, contentTopLeft, contentHeight);

	ImGui::End();
	ImGui::PopStyleVar();

	RenderNavBar(state, guardedDispatch);
	RenderDialogs(state, guardedDispatch);
	RenderFileBrowser(state, dispatch);
	dialogs::Toast(state, guardedDispatch);
	RenderLaunchIris(state);

	// The gesture ends with the button release — reset AFTER rendering so
	// a click fired on the release frame of a drag is still suppressed.
	if (!ImGui::IsMouseDown(0)) {
		m_gestureDrag = false;
	}
}

bool LauncherView::LaunchIrisDone() const
{
	if (m_irisStartedAt < 0.0) {
		return false;
	}
	return ElapsedFraction(m_irisStartedAt, ImGui::GetTime(), kIrisDuration) >= 1.0F;
}

void LauncherView::RenderLaunchIris(const LauncherState &state)
{
	if (!state.pendingLaunch.has_value()) {
		m_irisStartedAt = -1.0;
		return;
	}
	if (m_irisStartedAt < 0.0) {
		m_irisStartedAt = ImGui::GetTime();
	}

	// Iris-out: экран закрывается сужающимся кругом перед уходом в движок —
	// как титры старых игр. Полностью чисто view-слой: Store уже зафиксировал
	// pendingLaunch, мы лишь догружаем последний кадр анимацией.
	const float eased = EaseInQuad(ElapsedFraction(m_irisStartedAt, ImGui::GetTime(), kIrisDuration));

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const ImVec2 a = viewport->WorkPos;
	const ImVec2 b = viewport->WorkPos + viewport->WorkSize;
	const ImVec2 c((a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F);
	const float maxR = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)) * 0.5F;
	const float radius = maxR * (1.0F - eased);

	ImDrawList *draw = ImGui::GetForegroundDrawList();
	const ImU32 dark = ImGui::GetColorU32(ImVec4(0.02F, 0.01F, 0.01F, 1.0F));
	if (radius <= 1.0F) {
		draw->AddRectFilled(a, b, dark);
		return;
	}

	// Затемнение вне круга: четыре полосы вокруг описанного квадрата…
	const float x0 = std::max(a.x, c.x - radius);
	const float x1 = std::min(b.x, c.x + radius);
	const float y0 = std::max(a.y, c.y - radius);
	const float y1 = std::min(b.y, c.y + radius);
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

	// …и угловые веера треугольников для квадрата минус диск. Хорды дуг
	// (размах 90°) отделяют угол квадрата от центра, поэтому треугольники
	// не залезают внутрь круга.
	constexpr int kSegments = 8;
	struct Corner {
		ImVec2 v;
		float angle0;
		float angle1;
	};
	const float kPi = 3.14159265F;
	const Corner corners[] {
		{ ImVec2(x0, y0), kPi, 1.5F * kPi },          // верхний левый
		{ ImVec2(x1, y0), 1.5F * kPi, 2.0F * kPi },   // верхний правый
		{ ImVec2(x1, y1), 0.0F, 0.5F * kPi },         // нижний правый
		{ ImVec2(x0, y1), 0.5F * kPi, kPi },          // нижний левый
	};
	for (const Corner &corner : corners) {
		ImVec2 prev(c.x + radius * std::cos(corner.angle0), c.y + radius * std::sin(corner.angle0));
		for (int i = 1; i <= kSegments; ++i) {
			const float angle = corner.angle0 + (corner.angle1 - corner.angle0) * (static_cast<float>(i) / kSegments);
			const ImVec2 point(c.x + radius * std::cos(angle), c.y + radius * std::sin(angle));
			draw->AddTriangleFilled(corner.v, prev, point, dark);
			prev = point;
		}
	}

	// Тлеющий обод — круг закрывается не тьмой, а догорающим огнём.
	const float time = static_cast<float>(ImGui::GetTime());
	const float flicker = 0.70F + 0.30F * std::sin(time * 17.0F + 2.0F * std::sin(time * 6.3F));
	const float rimAlpha = 0.55F * flicker * (1.0F - eased * 0.5F);
	draw->AddCircle(c, radius, ImGui::GetColorU32(ImVec4(1.0F, 0.58F, 0.16F, rimAlpha)), 48, Scale::px(0.1F));
	draw->AddCircle(c, radius * 0.96F,
	    ImGui::GetColorU32(ImVec4(0.91F, 0.55F, 0.16F, rimAlpha * 0.6F)), 48, Scale::px(0.18F));
}

void LauncherView::RenderBackground() const
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImDrawList *draw = ImGui::GetBackgroundDrawList();

	if (m_backgroundTexture != nullptr && m_backgroundTextureSize.x > 0 && m_backgroundTextureSize.y > 0) {
		// Aspect-fill: crop the image so it covers the viewport.
		const ImVec2 &v = viewport->WorkSize;
		const ImVec2 &t = m_backgroundTextureSize;
		const float scale = std::max(v.x / t.x, v.y / t.y);
		const ImVec2 shown(t.x * scale, t.y * scale);
		const ImVec2 crop(0.5F - (v.x / shown.x) * 0.5F, 0.5F - (v.y / shown.y) * 0.5F);
		draw->AddImage(m_backgroundTexture, viewport->WorkPos, viewport->WorkPos + v,
		    ImVec2(crop.x, crop.y), ImVec2(1.0F - crop.x, 1.0F - crop.y));
	}

	// Тлеющие угольки, всплывающие над артом — отсылка к огню в главном
	// меню Diablo. Частицы бессостоятельные: позиция — чистая функция
	// времени и индекса, поэтому анимация бесплатна и для паузы в фоне
	// ничего сохранять не нужно.
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
		const float radius = Scale::px(0.08F + 0.06F * std::sin(seed * 5.9F));
		const ImVec2 center(viewport->WorkPos.x + x01 * viewport->WorkSize.x,
		    viewport->WorkPos.y + y01 * viewport->WorkSize.y);

		// Тёплые оттенки: от глубокого красного к золоту.
		const ImVec4 tint(0.91F - 0.35F * std::sin(seed * 2.3F), 0.52F - 0.28F * std::sin(seed * 2.3F),
		    0.16F, alpha);
		draw->AddCircleFilled(center, radius, ImGui::ColorConvertFloat4ToU32(tint), 6);
	}

	// Dark vignette so text stays readable over the artwork.
	// Полупрозрачность сохраняет угольки видимыми сквозь затемнение.
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

	const float barW = Scale::px(0.14F);
	const float inset = Scale::px(0.1F);
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float x0 = viewport->WorkPos.x + viewport->WorkSize.x - inset - barW;

	// Доля видимой части экрана → высота бегунка (не короче 1.5rem).
	const float fraction = height / (height + scrollMaxY);
	const float thumbH = std::max(height * fraction, Scale::px(1.5F));
	const float travel = height - thumbH;
	const float thumbY = topLeft.y + (scrollMaxY > 0.0F ? travel * (scrollY / scrollMaxY) : 0.0F);

	ImDrawList *draw = ImGui::GetWindowDrawList();
	draw->AddRectFilled(ImVec2(x0, thumbY), ImVec2(x0 + barW, thumbY + thumbH),
	    ImGui::GetColorU32(ImVec4(0.91F, 0.78F, 0.49F, 0.85F * alpha)), barW * 0.5F);
}

void LauncherView::RenderNavBar(const LauncherState &state, const Dispatcher &dispatch)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float height = Scale::px(3.2F);
	const ImVec2 size(viewport->WorkSize.x, height);
	// Slightly above the screen bottom so system gesture areas never
	// clip the buttons on phones.
	const float bottomInset = Scale::px(0.35F);
	const ImVec2 pos = Scale::portrait()
	    ? ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - height - bottomInset)
	    : viewport->WorkPos;

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale::px(0.4F), Scale::px(0.4F)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::color(ColorRole::Panel));
	ImGui::Begin("##navbar", nullptr,
	    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
	        | ImGuiWindowFlags_NoBringToFrontOnFocus);

	struct NavItem {
		Screen screen;
		const char *icon;
		const char *label;
	};
	const NavItem items[] {
		{ Screen::Home, icons::Home, "Главная" },
		{ Screen::Data, icons::Folder, "Данные" },
		{ Screen::About, icons::Info, "О порте" },
	};

	const float buttonWidth = ImGui::GetContentRegionAvail().x / std::size(items);
	for (const NavItem &item : items) {
		const bool selected = (state.screen == item.screen);
		ImGui::PushStyleColor(ImGuiCol_Button, Theme::color(selected ? ColorRole::Red : ColorRole::Bg));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::color(selected ? ColorRole::RedHover : ColorRole::Panel));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::color(ColorRole::RedPressed));
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(selected ? ColorRole::GoldBright : ColorRole::TextBody));

		const std::string label = std::string(item.icon) + "  " + item.label;
		if (ImGui::Button(label.c_str(), ImVec2(buttonWidth - Scale::px(0.4F), height - Scale::px(0.8F)))) {
			dispatch(intent::UiNavigate { item.screen });
		}
		ImGui::SameLine(0, Scale::px(0.4F));
		ImGui::PopStyleColor(4);
	}

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void LauncherView::RenderScreen(const LauncherState &state, const Dispatcher &dispatch)
{
	const auto modeArt = [this](ExitAction mode) {
		const size_t i = static_cast<size_t>(mode);
		return screens::ModeArt { m_heroArts[i], m_iconArts[i] };
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
	case Screen::About:
		screens::About(state);
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
	// otherwise the block above would immediately reopen it.
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

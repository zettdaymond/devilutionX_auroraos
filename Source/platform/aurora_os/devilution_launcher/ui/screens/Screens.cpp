#include "Screens.hpp"

#include "../Format.hpp"
#include "../Icons.hpp"
#include "../Scale.hpp"
#include "../Theme.hpp"

#include "core/EngineOptions.hpp"
#include "core/GameFiles.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstring>
#include <string>
#include <string_view>

#ifdef AURORA_OS
#include <unistd.h>
#endif

#ifndef LAUNCHER_APP_VERSION
#   define LAUNCHER_APP_VERSION "dev"
#endif

namespace launcher::ui::screens {

namespace {

#ifdef AURORA_OS
/// Полная версия УСТАНОВЛЕННОЙ сборки: на Авроре бинарник живёт в
/// /opt/app/org.diasurgical.devilutionx/<версия-релиз>/bin/ — путь
/// собственного исполняемого файла сообщает её сам, всегда совпадая с
/// rpm -q. Фолбэк — версия из файла VERSION (она же на десктопе).
std::string InstalledBuildName()
{
	static const std::string cached = []() -> std::string {
		char buffer[512];
		const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
		if (length <= 0) {
			return LAUNCHER_APP_VERSION;
		}
		buffer[length] = '\0';
		const std::string_view path(buffer, static_cast<size_t>(length));
		constexpr std::string_view kAppDir = "/org.diasurgical.devilutionx/";
		const auto dirPos = path.find(kAppDir);
		if (dirPos == std::string_view::npos) {
			return LAUNCHER_APP_VERSION;
		}
		const auto versionStart = dirPos + kAppDir.size();
		const auto versionEnd = path.find('/', versionStart);
		if (versionEnd == std::string_view::npos || versionEnd == versionStart) {
			return LAUNCHER_APP_VERSION;
		}
		return std::string(path.substr(versionStart, versionEnd - versionStart));
	}();
	return cached;
}
#endif


// Размеры в rem: rem плавно зависит от вьюпорта (см. Scale), поэтому
// конкретные значения здесь — единственное место, где они живут.
constexpr float kFirstRunHeroMaxRem = 17.0F;
constexpr float kPortraitHeroMaxRem = 14.0F;
constexpr float kLandscapeHeroRem = 11.0F;
constexpr float kTileHeightRem = 4.2F;

/// Визуальная идентичность режима: акцентный цвет (альфа — сила тинта
/// hero-панели) и запасной кроп общего фона, когда нет своего арта.
struct GameStyle {
	ImVec4 accent;
	ImVec2 uv0;
	ImVec2 uv1;
};

const GameStyle kDiabloStyle { ImVec4(0.545F, 0.10F, 0.06F, 0.16F), ImVec2(0.04F, 0.04F), ImVec2(0.72F, 0.86F) };
const GameStyle kHellfireStyle { ImVec4(0.70F, 0.39F, 0.10F, 0.16F), ImVec2(0.30F, 0.0F), ImVec2(1.0F, 0.80F) };
const GameStyle kDemoStyle { ImVec4(0.42F, 0.36F, 0.20F, 0.16F), ImVec2(0.14F, 0.20F), ImVec2(0.86F, 0.95F) };

/// Что рисует hero-панель: арт плюс применяемые кроп и тинт.
struct HeroArtRef {
	const BackgroundArt *art;
	ImVec2 uv0;
	ImVec2 uv1;
	ImVec4 tint;
};

/**
 * @brief Выбирает арт для hero-панели режима.
 *
 * Предпочитает собственный арт режима (у него уже своя палитра, поэтому
 * хватает лёгкого тинта); если файл не вшит в сборку — кроп общего фона.
 */
auto ResolveHeroArt(const ArtSet &arts, const BackgroundArt &modeArt, const GameStyle &style) -> HeroArtRef
{
	if (modeArt.texture != nullptr && modeArt.size.x > 0.0F && modeArt.size.y > 0.0F) {
		return { &modeArt, ImVec2(0.0F, 0.0F), ImVec2(1.0F, 1.0F),
			ImVec4(style.accent.x, style.accent.y, style.accent.z, 0.08F) };
	}
	return { &arts.background, style.uv0, style.uv1, style.accent };
}

/// Русская форма слова «файл» для числа: файл / файла / файлов.
auto PluralFiles(size_t count) -> std::string
{
	if (count % 10 == 1 && count % 100 != 11) {
		return std::to_string(count) + " файл";
	}
	if (count % 10 >= 2 && count % 10 <= 4 && (count % 100 < 10 || count % 100 >= 20)) {
		return std::to_string(count) + " файла";
	}
	return std::to_string(count) + " файлов";
}

// ---------------------------------------------------------------------------
// Первый запуск: файлов игры нет вообще.
// ---------------------------------------------------------------------------

void RenderFirstRun(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art)
{
	// Геометрия — та же, что у главного экрана с найденной игрой: в
	// ландшафте контентный блок капится 30rem и центрируется (без капа
	// hero-панель растягивалась на всю ширину планшета), высота hero в
	// ландшафте — как у featured-панели.
	const float fullWidth = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetContentRegionAvail().y;
	const bool portrait = Scale::Portrait();
	const float width = portrait ? fullWidth : std::min(fullWidth, Scale::Px(kLandscapeContentMaxRem));
	const float heroHeight = portrait ? std::min(height * 0.62F, Scale::Px(kFirstRunHeroMaxRem))
	                                  : Scale::Px(kLandscapeHeroRem);

	// Сдвиг к вертикальному центру, как на главной (там Dummy(free*0.25)
	// перед hero; здесь блока меньше — берём долю поменьше).
	ImGui::Dummy(ImVec2(0, height * 0.10F));

	const float sidePad = portrait ? 0.0F : std::max(0.0F, (fullWidth - width) * 0.5F);
	if (sidePad > 0.0F) {
		ImGui::Indent(sidePad);
	}

	const HeroArtRef hero = ResolveHeroArt(art, art.diablo.hero, kDiabloStyle);
	widgets::HeroPanel("DEVILUTIONX ДЛЯ AURORA OS", "DIABLO",
	    "Файлы оригинальной игры не найдены.\nСкопируйте DIABDAT.MPQ с диска или купите оригинальную игру,\nлибо скачайте бесплатное демо.",
	    *hero.art, hero.uv0, hero.uv1, hero.tint, ImVec2(width, heroHeight),
	    {
	        widgets::HeroAction { "Скачать демо", true, [&dispatch] {
	                                 dispatch(intent::UiOpenDialog { Dialog::ConfirmDownloadDemo });
	                             } },
	        widgets::HeroAction { "Выбрать файлы", false, [&dispatch] {
	                                 dispatch(intent::SelectDataFolder {});
	                             } },
	    });

	ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::TextWrapped("%s",
	    "Для полной версии скопируйте DIABDAT.MPQ с диска\nили купите оригинальную игру. Для Hellfire нужны hellfire.mpq,\nhfmonk.mpq, hfmusic.mpq и hfvoice.mpq.");
	ImGui::PopStyleColor();

	if (sidePad > 0.0F) {
		ImGui::Unindent(sidePad);
	}
}

// ---------------------------------------------------------------------------
// Главная с найденными файлами: hero избранного режима + полка остальных.
// ---------------------------------------------------------------------------

/// Режим, вынесенный в большую hero-панель.
struct Featured {
	ExitAction game;
	const char *title;
	const char *eyebrow;
	std::string status;
	bool playable;
	const GameStyle *style;
};

/// Плитка режима на полке под hero-панелью.
struct ShelfItem {
	ExitAction game;
	const char *title;
	std::string status;
	bool playable;
	const char *icon;              ///< FontAwesome-глиф — фолбэк без ассета
	const widgets::IconSet *icons; ///< собственные силуэты (может быть null)
	const GameStyle *style;
};

/// Выбирает featured-режим: первый запускаемый. Если не запускается
/// ничего — главный экран вообще показывает first-run (Home), сюда
/// такой случай не доходит; непровоцируемый Hellfire-замок убран:
/// он дублировался карточкой+плиткой и ломал полку третьим блоком.
auto MakeFeatured(const LauncherState &state) -> Featured
{
	if (state.diablo.available) {
		return { ExitAction::LaunchDiablo, "DIABLO", "ГОТОВО К ЗАПУСКУ", "Оригинальный Diablo", true,
			&kDiabloStyle };
	}
	if (state.hellfire.available) {
		return { ExitAction::LaunchHellfire, "HELLFIRE", "ГОТОВО К ЗАПУСКУ", "Официальное дополнение", true,
			&kHellfireStyle };
	}
	return { ExitAction::LaunchDemo, "DEMO", "ДЕМО-ВЕРСИЯ", "Бесплатная shareware-версия Diablo", true,
		&kDemoStyle };
}

/// Описания плиток всех трёх режимов (featured-режим с полки уберётся).
auto MakeShelfItems(const LauncherState &state, const ArtSet &art) -> std::array<ShelfItem, 3>
{
	return {
		ShelfItem { ExitAction::LaunchDiablo, "DIABLO",
		    state.diablo.available ? "Готово к запуску" : "Нужен DIABDAT.MPQ", state.diablo.available, icons::Fire,
		    &art.diablo.icon, &kDiabloStyle },
		ShelfItem { ExitAction::LaunchHellfire, "HELLFIRE",
		    state.hellfire.available ? "Готово к запуску"
		        : (state.diablo.available ? "Не хватает: " + PluralFiles(state.hellfire.missingFiles.size())
		                                  : "Нужны файлы Diablo и Hellfire"),
		    state.hellfire.available, icons::Gamepad, &art.hellfire.icon, &kHellfireStyle },
		ShelfItem { ExitAction::LaunchDemo, "DEMO",
		    state.demo.available ? "Бесплатная демо-версия"
		        : "Скачать · " + FormatBytes(FileSpecOf(KnownFile::Spawn).expectedSizeBytes),
		    true, icons::Download, &art.demo.icon, &kDemoStyle },
	};
}

void RenderHeroAndShelf(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art)
{
	const float fullWidth = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetContentRegionAvail().y;
	const bool portrait = Scale::Portrait();
	const float width = portrait ? fullWidth : std::min(fullWidth, Scale::Px(kLandscapeContentMaxRem));

	// Центрируем ограниченный блок. Indent, а не SetCursorPosX: тот действует
	// лишь до первого переноса строки, а заголовок и плитки начинаются с новой.
	const float sidePad = portrait ? 0.0F : std::max(0.0F, (fullWidth - width) * 0.5F);
	if (sidePad > 0.0F) {
		ImGui::Indent(sidePad);
	}

	const Featured featured = MakeFeatured(state);
	const std::array<ShelfItem, 3> tiles = MakeShelfItems(state, art);

	const float heroHeight = portrait ? std::min(height * 0.52F, Scale::Px(kPortraitHeroMaxRem))
	                                  : Scale::Px(kLandscapeHeroRem);
	const ModeArt &featuredMode = featured.game == ExitAction::LaunchDiablo ? art.diablo
	    : featured.game == ExitAction::LaunchHellfire                    ? art.hellfire
	                                                                    : art.demo;
	const HeroArtRef hero = ResolveHeroArt(art, featuredMode.hero, *featured.style);
	widgets::HeroPanel(featured.eyebrow, featured.title, featured.status.c_str(), *hero.art, hero.uv0, hero.uv1,
	    hero.tint, ImVec2(width, heroHeight),
	    {
	        widgets::HeroAction { featured.playable ? "Играть" : "Выбрать папку", true, [&dispatch, featured] {
	                                 if (featured.playable) {
	                                     dispatch(intent::LaunchGame { featured.game });
	                                 } else {
	                                     dispatch(intent::SelectDataFolder {});
	                                 }
	                             } },
	    });

	ImGui::Dummy(ImVec2(0, Scale::Px(0.8F)));
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::GoldBright));
	ImGui::TextUnformatted("ДРУГИЕ РЕЖИМЫ");
	ImGui::PopStyleColor();
	ImGui::PopFont();
	ImGui::Dummy(ImVec2(0, Scale::Px(0.3F)));

	// Полка: по одной плитке в строку в портрете, по две — в альбомной.
	const ShelfItem *visible[3] = {};
	int visibleCount = 0;
	for (const ShelfItem &item : tiles) {
		if (featured.game != item.game || !featured.playable) {
			visible[visibleCount++] = &item;
		}
	}
	for (int i = 0; i < visibleCount; ++i) {
		const ShelfItem &item = *visible[i];
		const ImVec2 tileSize(portrait ? width : width * 0.5F - Scale::Px(0.25F), Scale::Px(kTileHeightRem));
		widgets::GameTile(item.title, item.status.c_str(), item.icon, item.icons, item.playable,
		    item.style->accent, tileSize, [&dispatch, item] { dispatch(intent::LaunchGame { item.game }); });
		if (i + 1 < visibleCount) {
			if (portrait) {
				ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
			} else {
				ImGui::SameLine(0, Scale::Px(0.5F));
			}
		}
	}

	if (sidePad > 0.0F) {
		ImGui::Unindent(sidePad);
	}
}

} // namespace

void Home(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art)
{
	// First-run показываем, пока НЕ ЗАПУСКАЕТСЯ ни один режим: один
	// скачанный ru.mpq — это всё ещё «файлов нет» (HasAnyFiles считал
	// бы его и уводил на замок «Hellfire: требуются файлы»).
	if (!state.HasPlayableMode()) {
		RenderFirstRun(state, dispatch, art);
		return;
	}

	// Сдвигаем блок к вертикальному центру контентной области.
	const float heroHeight = Scale::Portrait() ? Scale::Px(kPortraitHeroMaxRem) : Scale::Px(kLandscapeHeroRem);
	const float shelfHeight = Scale::Px(kTileHeightRem) * 2.0F + Scale::Px(3.5F);
	const float free = ImGui::GetContentRegionAvail().y - (heroHeight + shelfHeight);
	ImGui::Dummy(ImVec2(0, std::max(free * 0.25F, Scale::Px(0.5F))));

	RenderHeroAndShelf(state, dispatch, art);
}

namespace {

/// Группа чек-листа: файлы одного режима игры.
struct FileGroup {
	const char *title;
	std::vector<KnownFile> files;
};

/// Рисует одну группу чек-листа: заголовок со сводкой и строки файлов.
void RenderFileGroup(const LauncherState &state, const Dispatcher &dispatch, const FileGroup &group)
{
	size_t missing = 0;
	for (KnownFile file : group.files) {
		if (state.fileSizes[static_cast<size_t>(file)] < 0) {
			++missing;
		}
	}

	ImGui::Dummy(ImVec2(0, Scale::Px(0.2F)));
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted(group.title);
	ImGui::PopFont();
	ImGui::SameLine();
	ImGui::PushFont(Theme::Font(FontRole::Body));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(missing == 0 ? ColorRole::Success : ColorRole::TextDim));
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale::Px(0.15F));
	if (strcmp(group.title, "Прочее") == 0) {
		ImGui::TextUnformatted("— не обязательны для запуска");
	} else if (missing == 0) {
		ImGui::TextUnformatted("— всё на месте");
	} else if (group.files.size() == 1) {
		ImGui::TextUnformatted("— файл не найден");
	} else {
		ImGui::TextWrapped("— не хватает %zu из %zu", missing, group.files.size());
	}
	ImGui::PopStyleColor();
	ImGui::PopFont();
	ImGui::Dummy(ImVec2(0, Scale::Px(0.2F)));

	for (KnownFile id : group.files) {
		const size_t i = static_cast<size_t>(id);
		const FileSpec &spec = kFileCatalog[i];
		const bool present = state.fileSizes[i] >= 0;
		std::string status;
		std::string path;
		if (present) {
			status = FormatBytes(state.fileSizes[i]);
			path = state.fileFolders[i].string();
		} else {
			status = spec.downloadable ? "можно скачать" : "не найден";
		}

		std::function<void()> onDownload;
		if (!present && spec.downloadable) {
			const Dialog dialog
			    = (id == KnownFile::Spawn) ? Dialog::ConfirmDownloadDemo : Dialog::ConfirmDownloadRu;
			onDownload = [&dispatch, dialog] { dispatch(intent::UiOpenDialog { dialog }); };
		}
		std::function<void()> onDelete;
		if (present && spec.downloadable) {
			onDelete = [&dispatch, id] { dispatch(intent::DeleteDownloadedFile { id }); };
		}

		widgets::FileRow(present, spec.displayName.data(), status.c_str(),
		    path.empty() ? nullptr : path.c_str(), onDownload, onDelete);
	}
}

/// Чек-лист файлов по группам режимов: пользователь сразу видит, что для
/// Diablo достаточно одного файла, а hf*.mpq нужны только Hellfire.
void RenderChecklist(const LauncherState &state, const Dispatcher &dispatch)
{
	const FileGroup groups[] {
		{ "Diablo", { KnownFile::Diabdat } },
		{ "Hellfire", { KnownFile::Hellfire, KnownFile::HfMonk, KnownFile::HfMusic, KnownFile::HfVoice } },
		{ "Прочее", { KnownFile::Spawn, KnownFile::RuVoice } },
	};
	for (const FileGroup &group : groups) {
		RenderFileGroup(state, dispatch, group);
	}
}

} // namespace

void Data(const LauncherState &state, const Dispatcher &dispatch)
{
	const float width = ImGui::GetContentRegionAvail().x;

	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted("Папка с файлами игры");
	ImGui::PopFont();

	if (state.dataFolder.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
		ImGui::TextWrapped("Не выбрана — используются папки приложения.");
		ImGui::PopStyleColor();
	} else {
		ImGui::TextWrapped("%s", state.dataFolder.string().c_str());
	}

	widgets::IconButton(icons::Folder, "Изменить папку", false, ImVec2(width, Scale::Px(2.2F)), [&dispatch] {
		dispatch(intent::SelectDataFolder {});
	});

	ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
	Theme::DrawDivider(ImGui::GetCursorScreenPos(),
	    ImGui::GetCursorScreenPos() + ImVec2(width, 0), 0.6F);
	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));

	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted("Файлы игры");
	ImGui::PopFont();
	RenderChecklist(state, dispatch);

	// Свободное место — отдельной строкой под чек-листом: это свойство
	// хранилища, а не какого-то файла из списка.
	ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::Text("%s  Свободно: %s", icons::Hdd, FormatBytes(state.freeDiskBytes).c_str());
	ImGui::PopStyleColor();
}

// ---------------------------------------------------------------------------
// Настройки игры: движковые опции diablo.ini по группам.
// ---------------------------------------------------------------------------

/// Короткая подпись текущего значения справа от имени строки.
std::string SettingValueText(const SettingSpec &spec, int value)
{
	switch (spec.kind) {
	case SettingKind::PercentVolume:
		return std::to_string(value) + "%";
	case SettingKind::Cycle:
		return std::string(spec.optionNames[static_cast<size_t>(SettingCycleIndex(spec, value))]);
	case SettingKind::Slider:
	case SettingKind::Toggle:
		break;
	}
	return std::to_string(value);
}

/// Ширина самого широкого названия варианта перебора.
float WidestOptionWidth(const SettingSpec &spec)
{
	float width = 0.0F;
	for (size_t i = 0; i < spec.optionCount; ++i) {
		width = std::max(width, ImGui::CalcTextSize(spec.optionNames[i].data()).x);
	}
	return width;
}

/// Геометрия степпера «‹ значение ›» (общая для резерва под контрол и
/// самой отрисовки, чтобы имя строки переносилось до его левого края).
struct StepperGeometry {
	float button;
	float gap;
	float cellWidth;
	float totalWidth;
};

StepperGeometry StepperMetricsOf(const SettingSpec &spec, const std::vector<int> &resolutionOptions)
{
	StepperGeometry metrics;
	metrics.button = Scale::Px(1.7F);
	metrics.gap = Scale::Px(0.25F);
	if (spec.secondaryKey.empty()) {
		metrics.cellWidth = WidestOptionWidth(spec) + Scale::Px(0.3F);
	} else {
		// Разрешение: ячейка — по самой широкой метке зеркала игрового
		// списка («1440p», «2160p»…).
		ImFont *font = ImGui::GetFont();
		const float fontSize = ImGui::GetFontSize();
		float widest = 0.0F;
		for (int height : resolutionOptions) {
			const std::string label = std::to_string(height) + "p";
			widest = std::max(widest,
			    font != nullptr ? font->CalcTextSizeA(fontSize, FLT_MAX, 0.0F, label.c_str()).x : fontSize * 4.0F);
		}
		metrics.cellWidth = widest + Scale::Px(0.3F);
	}
	metrics.totalWidth = metrics.button * 2.0F + metrics.gap * 2.0F + metrics.cellWidth;
	return metrics;
}

/// Строка настройки — та же схема, что у FileRow на экране данных:
/// имя и прижатый вправо контрол на первой линии, описание прямо под
/// именем (SetCursorPosY под линию имени, как путь у файла). Имя, как
/// и описание, переносится левее контрола, а сам контрол ставится на
/// первую линию абсолютной позицией — SameLine после многострочного
/// имени относился бы к его последней строке. Контролы только
/// отправляют интенты.
void RenderSettingRow(const SettingSpec &spec, int value, const Dispatcher &dispatch,
    const std::vector<int> &resolutionOptions)
{
	ImGui::PushID(spec.key.data());

	const float startX = ImGui::GetCursorPosX();
	const ImVec2 rowStart = ImGui::GetCursorScreenPos();
	const float rowWidth = ImGui::GetContentRegionAvail().x;

	// Правая зона контрола: в неё не заезжают ни имя, ни описание.
	// Ширина известна до отрисовки — по ней же переносится имя.
	const StepperGeometry stepper = StepperMetricsOf(spec, resolutionOptions);
	float controlWidth = 0.0F;
	switch (spec.kind) {
	case SettingKind::Toggle:
		controlWidth = Scale::Px(2.4F);
		break;
	case SettingKind::Cycle:
		controlWidth = stepper.totalWidth;
		break;
	case SettingKind::Slider:
	case SettingKind::PercentVolume:
		controlWidth = ImGui::CalcTextSize(SettingValueText(spec, value).c_str()).x;
		break;
	}
	const float controlGap = controlWidth > 0.0F ? controlWidth + Scale::Px(0.6F) : 0.0F;

	ImGui::PushTextWrapPos(startX + rowWidth - controlGap);
	ImGui::TextUnformatted(spec.nameRu.data());
	ImGui::PopTextWrapPos();
	const float nameBottom = ImGui::GetCursorPosY();

	ImGui::SetCursorScreenPos(ImVec2(rowStart.x + rowWidth - controlWidth, rowStart.y));
	switch (spec.kind) {
	case SettingKind::Toggle:
		widgets::ToggleSwitch("switch", value != 0, [&dispatch, id = spec.id](bool next) {
			dispatch(intent::SettingChanged { id, next ? 1 : 0 });
		});
		break;
	case SettingKind::Cycle: {
		// Степпер «‹ значение ›»: компактнее кнопки с названием варианта
		// и очевидно, что значение переключается. Ячейка значения — по
		// самому широкому варианту, чтобы степпер не прыгал при смене.
		// Варианты переключаются по индексу, а в интент уходит
		// optionValues индекса — у зелий значения 0/1/2/4/8/16.
		// Разрешение: варианты — зеркало игрового списка из состояния
		// (BuildResolutionOptions: режимы дисплея + сырое значение ini).
		const bool fromState = !spec.secondaryKey.empty();
		const std::vector<int> &stateOptions = resolutionOptions;
		if (fromState && stateOptions.empty()) {
			break; // без данных экрана список не строился — нечем листать
		}
		const int count = fromState ? static_cast<int>(stateOptions.size())
		                            : static_cast<int>(spec.optionCount);
		// Список разрешений — по возрастанию; текущая позиция — первая
		// ступень ≥ значения (сырое значение между ступенями остаётся
		// со своей меткой, стрелки идут к соседям от его позиции).
		int currentIndex = SettingCycleIndex(spec, value) % std::max(count, 1);
		if (fromState) {
			currentIndex = static_cast<int>(stateOptions.size()) - 1;
			for (size_t i = 0; i < stateOptions.size(); ++i) {
				if (stateOptions[i] >= value) {
					currentIndex = static_cast<int>(i);
					break;
				}
			}
		}
		const auto valueOfIndex = [fromState, &stateOptions, &spec](int index) {
			return fromState ? stateOptions[static_cast<size_t>(index)]
			                 : spec.optionValues[static_cast<size_t>(index)];
		};
		widgets::GhostButton(icons::ChevronLeft, "", ImVec2(stepper.button, stepper.button),
		    [&dispatch, id = spec.id, count, currentIndex, valueOfIndex] {
			    dispatch(intent::SettingChanged { id, valueOfIndex((currentIndex + count - 1) % count) });
		    });
		ImGui::SameLine(0, stepper.gap);
		ImGui::Dummy(ImVec2(stepper.cellWidth, stepper.button));
		const ImVec2 cellMin = ImGui::GetItemRectMin();
		const ImVec2 cellMax = ImGui::GetItemRectMax();
		ImGui::SameLine(0, stepper.gap);
		widgets::GhostButton(icons::ChevronRight, "", ImVec2(stepper.button, stepper.button),
		    [&dispatch, id = spec.id, count, currentIndex, valueOfIndex] {
			    dispatch(intent::SettingChanged { id, valueOfIndex((currentIndex + 1) % count) });
		    });

		const std::string current = fromState
		    ? std::to_string(value) + "p"
		    : SettingValueText(spec, value);
		ImFont *font = ImGui::GetFont();
		const float font_size = ImGui::GetFontSize();
		const ImVec2 textSize = font->CalcTextSizeA(font_size, FLT_MAX, 0.0F, current.c_str());
		ImGui::GetWindowDrawList()->AddText(font, font_size,
		    ImVec2((cellMin.x + cellMax.x - textSize.x) * 0.5F, (cellMin.y + cellMax.y - textSize.y) * 0.5F),
		    Theme::ColorU32(ColorRole::TextHeading), current.c_str());
		break;
	}
	case SettingKind::Slider:
	case SettingKind::PercentVolume: {
		const std::string text = SettingValueText(spec, value);
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextHeading));
		ImGui::TextUnformatted(text.c_str());
		ImGui::PopStyleColor();
		break;
	}
	}

	// Конец линии имени — низ имени (могло занять несколько строк)
	// или контрола первой линии, что ниже.
	const float textBottom = std::max(nameBottom, ImGui::GetCursorPosY());

	if (!spec.descriptionRu.empty()) {
		ImGui::SetCursorPosY(textBottom);
		ImGui::PushTextWrapPos(startX + rowWidth - controlGap);
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
		ImGui::TextUnformatted(spec.descriptionRu.data());
		ImGui::PopStyleColor();
		ImGui::PopTextWrapPos();
	}
	if (spec.kind == SettingKind::Slider || spec.kind == SettingKind::PercentVolume) {
		ImGui::Dummy(ImVec2(0, Scale::Px(0.25F)));
		widgets::OptionSlider("slider", value, spec.minValue, spec.maxValue,
		    [&dispatch, id = spec.id](int next) { dispatch(intent::SettingChanged { id, next }); });
	}

	// Контрол может висеть ниже последней строки описания — не даём
	// следующей строке наехать на него (guard из FileRow).
	const float controlBottom = textBottom + Scale::Px(0.35F);
	if (ImGui::GetCursorPosY() < controlBottom) {
		ImGui::SetCursorPosY(controlBottom);
	}

	ImGui::Dummy(ImVec2(0, Scale::Px(0.55F)));
	ImGui::PopID();
}

void Settings(const LauncherState &state, const Dispatcher &dispatch)
{
	// Виртуализация: полный список в 41 строку стоил ~6 мс/кадр на
	// устройстве — кадр перестал попадать в развёртку (45 fps вместо 60).
	// Высота строки переносима (описания переносятся по ширине), поэтому
	// рендерим только строки у окна, а высоту остальных берём из кэша
	// прошлого кадра и просто резервируем место. Смена ширины сбрасывает
	// кэш; запас в 4rem покрывает скачок прокрутки за кадр.
	static float rowHeights[kSettingCount] = {};
	static float cachedWidth = -1.0F;
	const float listWidth = ImGui::GetContentRegionAvail().x;
	if (listWidth != cachedWidth) {
		cachedWidth = listWidth;
		std::fill(rowHeights, rowHeights + kSettingCount, 0.0F);
	}
	const float viewTop = ImGui::GetScrollY() - Scale::Px(4.0F);
	const float viewBottom = ImGui::GetScrollY() + ImGui::GetWindowHeight() + Scale::Px(4.0F);

	// Раскладка как на экране данных: строки на всю ширину контента,
	// без капа и Indent — SameLine-выравнивание FileRow на Indent не
	// рассчитывает.
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::TextWrapped("%s", "Настройки применяются при следующем запуске игры.");
	ImGui::PopStyleColor();

	for (const SettingGroupSpec &group : kSettingGroups) {
		ImGui::Dummy(ImVec2(0, Scale::Px(0.9F)));
		ImGui::PushFont(Theme::Font(FontRole::BodyBold));
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::GoldBright));
		ImGui::TextUnformatted(group.titleRu.data());
		ImGui::PopStyleColor();
		ImGui::PopFont();
		ImGui::Dummy(ImVec2(0, Scale::Px(0.35F)));
		Theme::DrawDivider(ImGui::GetCursorScreenPos(),
		    ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionAvail().x, 0), 0.7F);
		ImGui::Dummy(ImVec2(0, Scale::Px(0.5F)));

		for (const SettingSpec &spec : kSettingCatalog) {
			if (spec.group != group.group) {
				continue;
			}
			const size_t index = static_cast<size_t>(spec.id);
			const float rowTop = ImGui::GetCursorPosY();
			if (rowHeights[index] > 0.0F
			    && (rowTop + rowHeights[index] < viewTop || rowTop > viewBottom)) {
				ImGui::SetCursorPosY(rowTop + rowHeights[index]);
				continue;
			}
			RenderSettingRow(spec, state.settingValues[index], dispatch, state.resolutionOptions);
			rowHeights[index] = ImGui::GetCursorPosY() - rowTop;
		}
	}

	ImGui::Dummy(ImVec2(0, Scale::Px(0.9F)));
	widgets::GhostButton(icons::Refresh, "Сбросить настройки",
	    ImVec2(ImGui::GetContentRegionAvail().x, Scale::Px(2.2F)), [&dispatch] {
		    dispatch(intent::UiOpenDialog { Dialog::ConfirmResetSettings });
	    });
}

void About(const LauncherState &, const Dispatcher &dispatch)
{
	Theme::PushFont(FontRole::Heading);
	widgets::CenteredText("\nDevilutionX", ColorRole::TextHeading);
	Theme::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextBody));
	widgets::CenteredText("Порт для Aurora OS");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0, Scale::Px(0.8F)));

	ImGui::TextWrapped(
	    "DevilutionX — современный открытый порт классического Diablo (1996) "
	    "и дополнения Hellfire с исправлением сотен ошибок оригинала.");

	ImGui::TextWrapped(
	    "Эта сборка — неофициальный порт DevilutionX на Aurora OS: адаптированы "
	    "управление (сенсорный экран, виртуальный геймпад), звук и графика "
	    "под устройства с Aurora OS.");

	ImGui::TextWrapped(
	    "Этот лаунчер помогает настроить игру: найти файлы оригинала, скачать "
	    "бесплатную демо-версию или русскую озвучку.");

	ImGui::Dummy(ImVec2(0, Scale::Px(0.8F)));
	Theme::DrawDivider(ImGui::GetCursorScreenPos(),
	    ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionAvail().x, 0), 0.6F);
	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));

	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted("Версия");
	ImGui::PopFont();
#ifdef AURORA_OS
	ImGui::Text("DevilutionX %s (порт для Aurora OS)", InstalledBuildName().c_str());
#else
	ImGui::Text("DevilutionX %s (порт для Aurora OS)", LAUNCHER_APP_VERSION);
#endif

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted("Данные и сохранения");
	ImGui::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::TextWrapped("Файлы игры ищутся в папке, выбранной на экране «Данные»,"
	                   " и в каталоге приложения (~/.local/share/org.diasurgical/devilutionx).");
	ImGui::TextWrapped("Сохранения и настройки хранятся там же.");
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted("Ссылки");
	ImGui::PopFont();
	ImGui::TextWrapped("%s  github.com/diasurgical/DevilutionX", icons::Globe);
	ImGui::TextWrapped("%s  github.com/zettdaymond/devilutionX_auroraos", icons::Globe);
	ImGui::TextWrapped("%s  Diablo © 1996 Blizzard Entertainment", icons::Book);

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted("Связь");
	ImGui::PopFont();
	ImGui::TextWrapped("%s  По вопросам: zettday@gmail.com", icons::Envelope);

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::TextWrapped(
	    "Порт предоставляется «как есть», без каких-либо гарантий. "
	    "Лицензия: Sustainable Use License — использование в некоммерческих целях. "
	    "Diablo, Blizzard Entertainment — товарные знаки Blizzard Entertainment, Inc. "
	    "Порт не связан с Blizzard и не одобрен ею.");
	ImGui::PopStyleColor();
}

} // namespace launcher::ui::screens

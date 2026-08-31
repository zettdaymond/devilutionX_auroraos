#include "Screens.hpp"

#include "../Format.hpp"
#include "../Icons.hpp"
#include "../Scale.hpp"
#include "../Theme.hpp"

#include "core/GameFiles.hpp"

#include <imgui.h>

#include <array>
#include <cfloat>
#include <cstring>
#include <string>

#ifndef LAUNCHER_APP_VERSION
#   define LAUNCHER_APP_VERSION "dev"
#endif

namespace launcher::ui::screens {

namespace {

// Размеры в rem: rem плавно зависит от вьюпорта (см. Scale), поэтому
// конкретные значения здесь — единственное место, где они живут.
constexpr float kFirstRunHeroMaxRem = 17.0F;
constexpr float kPortraitHeroMaxRem = 14.0F;
constexpr float kLandscapeHeroRem = 11.0F;
constexpr float kTileHeightRem = 4.2F;
constexpr float kVoiceBannerHeightRem = 2.4F;

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
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetContentRegionAvail().y;
	const float heroHeight = std::min(height * 0.62F, Scale::Px(kFirstRunHeroMaxRem));

	const HeroArtRef hero = ResolveHeroArt(art, art.diablo.hero, kDiabloStyle);
	widgets::HeroPanel("DEVILUTIONX ДЛЯ AURORA OS", "DIABLO",
	    "Файлы оригинальной игры не найдены.\nСкопируйте DIABDAT.MPQ с диска или купите на GoG,\nлибо скачайте бесплатное демо.",
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
	    "Для полной версии скопируйте DIABDAT.MPQ с диска\nили купите игру на GoG.com. Для Hellfire нужны hellfire.mpq,\nhfmonk.mpq, hfmusic.mpq и hfvoice.mpq.");
	ImGui::PopStyleColor();
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

/// Выбирает featured-режим: первый запускаемый, иначе Hellfire «требуются файлы».
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
	if (state.demo.available) {
		return { ExitAction::LaunchDemo, "DEMO", "ДЕМО-ВЕРСИЯ", "Бесплатная shareware-версия Diablo", true,
			&kDemoStyle };
	}
	return { ExitAction::LaunchHellfire, "HELLFIRE", "ТРЕБУЮТСЯ ФАЙЛЫ",
		"Не хватает: " + PluralFiles(state.hellfire.missingFiles.size()), false, &kHellfireStyle };
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

/// Тонкий баннер предложения русской озвучки; появляется при готовом Diablo.
void RenderVoiceBanner(const LauncherState &state, const Dispatcher &dispatch, float width)
{
	ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
	const ImVec2 bannerSize(width, Scale::Px(kVoiceBannerHeightRem));
	if (ImGui::InvisibleButton("ruvoice", bannerSize)) {
		dispatch(intent::UiOpenDialog { Dialog::ConfirmDownloadRu });
	}
	ImDrawList *draw = ImGui::GetWindowDrawList();
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const bool hovered = ImGui::IsItemHovered();
	draw->AddRectFilled(min, max,
	    Theme::ColorU32(hovered ? ColorRole::PanelHover : ColorRole::Panel), Scale::Px(0.35F));
	draw->AddRect(min, max, Theme::ColorU32(ColorRole::GoldDim), Scale::Px(0.35F), 0, Scale::Px(0.06F));

	// Иконки рисуем в меру плашки: у IconBig-шрифта нативный кегль крупнее
	// тонкого баннера и торчит за его границы.
	ImFont *bannerIconFont = Theme::Font(FontRole::IconBig);
	const float iconSize = bannerSize.y * 0.55F;
	const float iconWidth = bannerIconFont != nullptr
	    ? bannerIconFont->CalcTextSizeA(iconSize, FLT_MAX, 0.0F, icons::Music).x
	    : iconSize;
	draw->AddText(bannerIconFont, iconSize,
	    ImVec2(min.x + Scale::Px(0.8F), min.y + (bannerSize.y - iconSize) * 0.5F),
	    Theme::ColorU32(ColorRole::GoldBright), icons::Music);

	draw->AddText(Theme::Font(FontRole::Body), Scale::Px(0.95F),
	    ImVec2(min.x + Scale::Px(0.8F) + iconWidth + Scale::Px(0.7F), min.y + (bannerSize.y - Scale::Px(1.1F)) * 0.5F),
	    Theme::ColorU32(ColorRole::TextBody),
	    "Русская озвучка и тексты · ru.mpq");

	const float chevronSize = bannerSize.y * 0.45F;
	const float chevronWidth = bannerIconFont != nullptr
	    ? bannerIconFont->CalcTextSizeA(chevronSize, FLT_MAX, 0.0F, icons::Play).x
	    : chevronSize;
	draw->AddText(bannerIconFont, chevronSize,
	    ImVec2(max.x - chevronWidth - Scale::Px(0.7F), min.y + (bannerSize.y - chevronSize) * 0.5F),
	    Theme::ColorU32(ColorRole::GoldBright), icons::Play);
}

void RenderHeroAndShelf(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art)
{
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetContentRegionAvail().y;
	const bool portrait = Scale::Portrait();

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

	if (state.diablo.available && !state.russianVoiceInstalled) {
		RenderVoiceBanner(state, dispatch, width);
	}
}

} // namespace

void Home(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art)
{
	if (!state.HasAnyFiles()) {
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

/// Рисует одну группу чек-листа: заголовок со сводкой и таблицу файлов.
void RenderFileGroup(const LauncherState &state, const Dispatcher &dispatch, const FileGroup &group, int groupIndex)
{
	size_t missing = 0;
	bool anyDeletable = false;
	for (KnownFile file : group.files) {
		const size_t idx = static_cast<size_t>(file);
		if (state.fileSizes[idx] < 0) {
			++missing;
		} else if (kFileCatalog[idx].downloadable) {
			anyDeletable = true;
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
	} else {
		ImGui::Text("— не хватает %zu из %zu", missing, group.files.size());
	}
	ImGui::PopStyleColor();
	ImGui::PopFont();
	ImGui::Dummy(ImVec2(0, Scale::Px(0.2F)));

	// Колонка действий добавляется только когда есть хотя бы одна кнопка:
	// иначе на устройстве она съедала ~140px справа, и список выглядел
	// смещённым влево.
	const int columnCount = anyDeletable ? 4 : 3;
	const std::string tableName = "files_" + std::to_string(groupIndex);
	if (!ImGui::BeginTable(tableName.c_str(), columnCount, ImGuiTableFlags_SizingStretchProp)) {
		return;
	}
	ImGui::TableSetupColumn("status", ImGuiTableColumnFlags_WidthFixed, Scale::Px(1.6F));
	ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("detail", ImGuiTableColumnFlags_WidthStretch);
	if (anyDeletable) {
		ImGui::TableSetupColumn("actions", ImGuiTableColumnFlags_WidthFixed, Scale::Px(2.0F));
	}

	for (KnownFile id : group.files) {
		const size_t i = static_cast<size_t>(id);
		const FileSpec &spec = kFileCatalog[i];
		ImGui::TableNextRow();

		const bool present = state.fileSizes[i] >= 0;
		std::string detail;
		if (present) {
			detail = FormatBytes(state.fileSizes[i]);
			if (!state.fileFolders[i].empty()) {
				detail += "\n" + state.fileFolders[i].string();
			}
			if (spec.downloadable) {
				detail += "\nскачан лаунчером";
			}
		} else {
			detail = spec.downloadable ? "можно скачать" : "не найден";
		}
		widgets::FileStatusLine(present, spec.displayName.data(), detail.c_str());

		if (anyDeletable) {
			ImGui::TableNextColumn();
			if (present && spec.downloadable) {
				Theme::PushButtonStyle(false);
				const std::string label = std::string(icons::Trash) + "##del" + std::to_string(i);
				if (ImGui::SmallButton(label.c_str())) {
					dispatch(intent::DeleteDownloadedFile { id });
				}
				Theme::PopButtonStyle();
			}
		}
	}
	ImGui::EndTable();
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
	for (int i = 0; i < 3; ++i) {
		RenderFileGroup(state, dispatch, groups[i], i);
	}
}

} // namespace

void Data(const LauncherState &state, const Dispatcher &dispatch)
{
	widgets::ScreenHeader("Данные", [&dispatch] { dispatch(intent::UiNavigate { Screen::Home }); });

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

	ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::Text("%s Свободно: %s", icons::Hdd, FormatBytes(state.freeDiskBytes).c_str());
	ImGui::PopStyleColor();
}

void About(const LauncherState &, const Dispatcher &dispatch)
{
	widgets::ScreenHeader("О порте", [&dispatch] { dispatch(intent::UiNavigate { Screen::Home }); });
	ImGui::Dummy(ImVec2(0, Scale::Px(1.0F)));
	Theme::PushFont(FontRole::Heading);
	widgets::CenteredText("DIABLO", ColorRole::TextHeading);
	Theme::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextBody));
	widgets::CenteredText("DevilutionX — порт для Aurora OS");
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
	ImGui::Text("DevilutionX %s (порт для Aurora OS)", LAUNCHER_APP_VERSION);

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted("Ссылки");
	ImGui::PopFont();
	ImGui::TextWrapped("%s  github.com/diasurgical/DevilutionX", icons::Globe);
	ImGui::TextWrapped("%s  github.com/zettdaymond/devilutionX_auroraos", icons::Globe);
	ImGui::TextWrapped("%s  Diablo © 1996 Blizzard Entertainment", icons::Book);

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted("Поддержка");
	ImGui::PopFont();
	ImGui::TextWrapped("%s  Нашли проблему или есть предложение? Пишите: zettday@gmail.com", icons::Envelope);

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::TextWrapped(
	    "Лицензия: Sustainable Use License — использование в некоммерческих целях. "
	    "Diablo, Blizzard Entertainment — товарные знаки Blizzard Entertainment, Inc. "
	    "Порт не связан с Blizzard и не одобрен ею.");
	ImGui::PopStyleColor();
}

} // namespace launcher::ui::screens

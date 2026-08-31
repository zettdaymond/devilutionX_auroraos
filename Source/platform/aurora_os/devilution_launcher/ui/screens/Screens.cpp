#include "Screens.hpp"

#include "../Format.hpp"
#include "../Icons.hpp"
#include "../Scale.hpp"
#include "../Theme.hpp"

#include "core/GameFiles.hpp"

#include <imgui.h>

#include <cfloat>
#include <cstring>
#include <string>

#ifndef LAUNCHER_APP_VERSION
#   define LAUNCHER_APP_VERSION "dev"
#endif

namespace launcher::ui::screens {

namespace {

/// Per-mode visual identity: accent color (alpha = hero tint strength)
/// and the fallback artwork crop used when no dedicated art is bundled.
struct GameStyle {
	ImVec4 accent;
	ImVec2 uv0;
	ImVec2 uv1;
};

const GameStyle kDiabloStyle { ImVec4(0.545F, 0.10F, 0.06F, 0.16F), ImVec2(0.04F, 0.04F), ImVec2(0.72F, 0.86F) };
const GameStyle kHellfireStyle { ImVec4(0.70F, 0.39F, 0.10F, 0.16F), ImVec2(0.30F, 0.0F), ImVec2(1.0F, 0.80F) };
const GameStyle kDemoStyle { ImVec4(0.42F, 0.36F, 0.20F, 0.16F), ImVec2(0.14F, 0.20F), ImVec2(0.86F, 0.95F) };

/// What a hero panel draws: artwork plus the crop and tint to apply.
struct HeroArtRef {
	const BackgroundArt *art;
	ImVec2 uv0;
	ImVec2 uv1;
	ImVec4 tint;
};

/// Prefers the dedicated per-mode artwork (composed with its own palette,
/// so only a whisper of tint is needed); crops the shared background as
/// a fallback when the file is not bundled.
HeroArtRef ResolveHeroArt(const ArtSet &arts, const BackgroundArt &modeArt, const GameStyle &style)
{
	if (modeArt.texture != nullptr && modeArt.size.x > 0.0F && modeArt.size.y > 0.0F) {
		return { &modeArt, ImVec2(0.0F, 0.0F), ImVec2(1.0F, 1.0F),
			ImVec4(style.accent.x, style.accent.y, style.accent.z, 0.08F) };
	}
	return { &arts.background, style.uv0, style.uv1, style.accent };
}

std::string PluralFiles(size_t count)
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
// First run: no game files at all.
// ---------------------------------------------------------------------------

void RenderFirstRun(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art)
{
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetContentRegionAvail().y;
	const float heroHeight = std::min(height * 0.62F, Scale::Px(17.0F));

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
// Home with detected files: featured hero + shelf.
// ---------------------------------------------------------------------------

struct Featured {
	ExitAction game;
	const char *title;
	const char *eyebrow;
	std::string status;
	bool playable;
	const GameStyle *style;
};

struct ShelfItem {
	ExitAction game;
	const char *title;
	std::string status;
	bool playable;
	const char *icon;                  // FontAwesome fallback glyph
	const widgets::IconSet *icons;     // dedicated silhouettes (may be null)
	const GameStyle *style;
};

void RenderHeroAndShelf(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art)
{
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetContentRegionAvail().y;
	const bool portrait = Scale::Portrait();

	// Pick the featured game: the first launchable mode.
	Featured featured;
	if (state.diablo.available) {
		featured = { ExitAction::LaunchDiablo, "DIABLO", "ГОТОВО К ЗАПУСКУ", "Оригинальный Diablo", true,
			&kDiabloStyle };
	} else if (state.hellfire.available) {
		featured = { ExitAction::LaunchHellfire, "HELLFIRE", "ГОТОВО К ЗАПУСКУ", "Официальное дополнение", true,
			&kHellfireStyle };
	} else if (state.demo.available) {
		featured = { ExitAction::LaunchDemo, "DEMO", "ДЕМО-ВЕРСИЯ", "Бесплатная shareware-версия Diablo", true,
			&kDemoStyle };
	} else {
		featured = { ExitAction::LaunchHellfire, "HELLFIRE", "ТРЕБУЮТСЯ ФАЙЛЫ",
			"Не хватает: " + PluralFiles(state.hellfire.missingFiles.size()), false, &kHellfireStyle };
	}

	ShelfItem diabloTile { ExitAction::LaunchDiablo, "DIABLO",
		state.diablo.available ? "Готово к запуску" : "Нужен DIABDAT.MPQ", state.diablo.available, icons::Fire,
		&art.diablo.icon, &kDiabloStyle };
	ShelfItem hellfireTile { ExitAction::LaunchHellfire, "HELLFIRE",
		state.hellfire.available ? "Готово к запуску"
		    : (state.diablo.available ? "Не хватает: " + PluralFiles(state.hellfire.missingFiles.size())
		                              : "Нужны файлы Diablo и Hellfire"),
		state.hellfire.available, icons::Gamepad, &art.hellfire.icon, &kHellfireStyle };
	ShelfItem demoTile { ExitAction::LaunchDemo, "DEMO",
		state.demo.available ? "Бесплатная демо-версия"
		    : "Скачать · " + FormatBytes(FileSpecOf(KnownFile::Spawn).expectedSizeBytes),
		true, icons::Download, &art.demo.icon, &kDemoStyle };

	const bool diabloFeatured = (featured.game == ExitAction::LaunchDiablo && featured.playable);
	const bool hellfireFeatured = (featured.game == ExitAction::LaunchHellfire && featured.playable);
	const bool demoFeatured = (featured.game == ExitAction::LaunchDemo && featured.playable);

	const float heroHeight = portrait ? std::min(height * 0.52F, Scale::Px(14.0F)) : Scale::Px(11.0F);
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

	// Shelf header.
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::GoldBright));
	ImGui::TextUnformatted("ДРУГИЕ РЕЖИМЫ");
	ImGui::PopStyleColor();
	ImGui::PopFont();
	ImGui::Dummy(ImVec2(0, Scale::Px(0.3F)));

	auto renderTile = [&](const ShelfItem &item) {
		const ImVec2 tileSize(portrait ? width : width * 0.5F - Scale::Px(0.25F), Scale::Px(4.2F));
		widgets::GameTile(item.title, item.status.c_str(), item.icon, item.icons, item.playable,
		    item.style->accent, tileSize, [&dispatch, item] { dispatch(intent::LaunchGame { item.game }); });
	};

	if (!diabloFeatured) {
		renderTile(diabloTile);
		if (!portrait) {
			ImGui::SameLine(0, Scale::Px(0.5F));
		} else {
			ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
		}
	}
	if (!hellfireFeatured) {
		renderTile(hellfireTile);
		if (!portrait) {
			ImGui::SameLine(0, Scale::Px(0.5F));
		} else {
			ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
		}
	}
	if (!demoFeatured) {
		renderTile(demoTile);
	}

	// Russian voice-pack offer as a slim banner.
	if (state.diablo.available && !state.russianVoiceInstalled) {
		ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
		const ImVec2 bannerSize(width, Scale::Px(2.4F));
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

		// Иконки рисуем в меру плачки: у IconBig-шрифта нативный кегль
		// крупнее тонкого баннера и торчит за его границы.
		ImFont *bannerIconFont = Theme::Font(FontRole::IconBig);
		const float iconSize = bannerSize.y * 0.55F;
		const float iconW = bannerIconFont != nullptr ? bannerIconFont->CalcTextSizeA(iconSize, FLT_MAX, 0.0F, icons::Music).x : iconSize;
		draw->AddText(bannerIconFont, iconSize,
		    ImVec2(min.x + Scale::Px(0.8F), min.y + (bannerSize.y - iconSize) * 0.5F),
		    Theme::ColorU32(ColorRole::GoldBright), icons::Music);

		draw->AddText(Theme::Font(FontRole::Body), Scale::Px(0.95F),
		    ImVec2(min.x + Scale::Px(0.8F) + iconW + Scale::Px(0.7F), min.y + (bannerSize.y - Scale::Px(1.1F)) * 0.5F),
		    Theme::ColorU32(ColorRole::TextBody),
		    "Русская озвучка и тексты · ru.mpq");

		const float chevSize = bannerSize.y * 0.45F;
		const float chevW = bannerIconFont != nullptr ? bannerIconFont->CalcTextSizeA(chevSize, FLT_MAX, 0.0F, icons::Play).x : chevSize;
		draw->AddText(bannerIconFont, chevSize,
		    ImVec2(max.x - chevW - Scale::Px(0.7F), min.y + (bannerSize.y - chevSize) * 0.5F),
		    Theme::ColorU32(ColorRole::GoldBright), icons::Play);
	}
}

} // namespace

void Home(const LauncherState &state, const Dispatcher &dispatch, const ArtSet &art)
{
	if (!state.hasAnyFiles()) {
		RenderFirstRun(state, dispatch, art);
		return;
	}

	// Nudge the block towards the vertical center of the content area.
	const float heroH = Scale::Portrait() ? Scale::Px(14.0F) : Scale::Px(11.0F);
	const float shelfH = Scale::Px(4.2F) * 2.0F + Scale::Px(3.5F);
	const float free = ImGui::GetContentRegionAvail().y - (heroH + shelfH);
	ImGui::Dummy(ImVec2(0, std::max(free * 0.25F, Scale::Px(0.5F))));

	RenderHeroAndShelf(state, dispatch, art);
}

namespace {

void RenderChecklist(const LauncherState &state, const Dispatcher &dispatch)
{
	// Файлы сгруппированы по режимам игры: пользователь сразу видит,
	// что для Diablo достаточно одного файла, а hf*.mpq нужны только
	// Hellfire, и их отсутствие не мешает игре в оригинал.
	struct FileGroup {
		const char *title;
		std::vector<KnownFile> files;
	};
	const FileGroup groups[] {
		{ "Diablo", { KnownFile::Diabdat } },
		{ "Hellfire", { KnownFile::Hellfire, KnownFile::HfMonk, KnownFile::HfMusic, KnownFile::HfVoice } },
		{ "Прочее", { KnownFile::Spawn, KnownFile::RuVoice } },
	};

	auto renderGroup = [&](const FileGroup &group, int groupIndex) {
		// Заголовок группы + сводка.
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

		// Колонка действий добавляется только когда есть хотя бы одна
		// кнопка — иначе на устройстве она съедала ~140px справа и
		// список выглядел смещённым влево.
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
	};

	for (int g = 0; g < 3; ++g) {
		renderGroup(groups[g], g);
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

	ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::Text("%s Свободно: %s", icons::Hdd, FormatBytes(state.freeDiskBytes).c_str());
	ImGui::PopStyleColor();
}

void About(const LauncherState &)
{
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

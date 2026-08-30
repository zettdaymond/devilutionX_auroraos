#include "Screens.hpp"

#include "../Format.hpp"
#include "../Icons.hpp"
#include "../Scale.hpp"
#include "../Theme.hpp"

#include "core/GameFiles.hpp"

#include <imgui.h>

#include <string>

#ifndef LAUNCHER_APP_VERSION
#   define LAUNCHER_APP_VERSION "dev"
#endif

namespace launcher::ui::screens {

namespace {

/// Per-mode visual identity: accent color and artwork crop (uv).
struct GameStyle {
	ImVec4 accent;
	ImVec2 uv0;
	ImVec2 uv1;
};

const GameStyle kDiabloStyle { ImVec4(0.545F, 0.10F, 0.06F, 1.0F), ImVec2(0.04F, 0.04F), ImVec2(0.72F, 0.86F) };
const GameStyle kHellfireStyle { ImVec4(0.70F, 0.39F, 0.10F, 1.0F), ImVec2(0.30F, 0.0F), ImVec2(1.0F, 0.80F) };
const GameStyle kDemoStyle { ImVec4(0.42F, 0.36F, 0.20F, 1.0F), ImVec2(0.14F, 0.20F), ImVec2(0.86F, 0.95F) };

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

void RenderFirstRun(const LauncherState &state, const Dispatcher &dispatch, const BackgroundArt &art)
{
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetContentRegionAvail().y;
	const float heroHeight = std::min(height * 0.62F, Scale::px(17.0F));

	widgets::HeroPanel("DEVILUTIONX ДЛЯ AURORA OS", "DIABLO",
	    "Файлы оригинальной игры не найдены.\nСкопируйте DIABDAT.MPQ с диска или купите на GoG,\nлибо скачайте бесплатное демо.",
	    art, kDiabloStyle.uv0, kDiabloStyle.uv1, kDiabloStyle.accent, ImVec2(width, heroHeight),
	    {
	        widgets::HeroAction { "Скачать демо", true, [&dispatch] {
		                             dispatch(intent::UiOpenDialog { Dialog::ConfirmDownloadDemo });
	                             } },
	        widgets::HeroAction { "Выбрать файлы", false, [&dispatch] {
		                             dispatch(intent::SelectDataFolder {});
	                             } },
	    });

	ImGui::Dummy(ImVec2(0, Scale::px(0.6F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
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
	const char *icon;
	const GameStyle *style;
};

void RenderHeroAndShelf(const LauncherState &state, const Dispatcher &dispatch, const BackgroundArt &art)
{
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetContentRegionAvail().y;
	const bool portrait = Scale::portrait();

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
		&kDiabloStyle };
	ShelfItem hellfireTile { ExitAction::LaunchHellfire, "HELLFIRE",
		state.hellfire.available ? "Готово к запуску"
		    : (state.diablo.available ? "Не хватает: " + PluralFiles(state.hellfire.missingFiles.size())
		                              : "Нужны файлы Diablo и Hellfire"),
		state.hellfire.available, icons::Gamepad, &kHellfireStyle };
	ShelfItem demoTile { ExitAction::LaunchDemo, "DEMO",
		state.demo.available ? "Бесплатная демо-версия"
		    : "Скачать · " + FormatBytes(FileSpecOf(KnownFile::Spawn).expectedSizeBytes),
		true, icons::Download, &kDemoStyle };

	const bool diabloFeatured = (featured.game == ExitAction::LaunchDiablo && featured.playable);
	const bool hellfireFeatured = (featured.game == ExitAction::LaunchHellfire && featured.playable);
	const bool demoFeatured = (featured.game == ExitAction::LaunchDemo && featured.playable);

	const float heroHeight = portrait ? std::min(height * 0.52F, Scale::px(14.0F)) : Scale::px(11.0F);
	widgets::HeroPanel(featured.eyebrow, featured.title, featured.status.c_str(), art, featured.style->uv0,
	    featured.style->uv1, featured.style->accent, ImVec2(width, heroHeight),
	    {
	        widgets::HeroAction { featured.playable ? "Играть" : "Выбрать папку", true, [&dispatch, featured] {
		                             if (featured.playable) {
			                             dispatch(intent::LaunchGame { featured.game });
		                             } else {
			                             dispatch(intent::SelectDataFolder {});
		                             }
	                             } },
	    });

	ImGui::Dummy(ImVec2(0, Scale::px(0.8F)));

	// Shelf header.
	ImGui::PushFont(Theme::font(FontRole::BodyBold));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::GoldBright));
	ImGui::TextUnformatted("ДРУГИЕ РЕЖИМЫ");
	ImGui::PopStyleColor();
	ImGui::PopFont();
	ImGui::Dummy(ImVec2(0, Scale::px(0.3F)));

	auto renderTile = [&](const ShelfItem &item) {
		const ImVec2 tileSize(portrait ? width : width * 0.5F - Scale::px(0.25F), Scale::px(4.2F));
		widgets::GameTile(item.title, item.status.c_str(), item.icon, item.playable, item.style->accent, tileSize,
		    [&dispatch, item] { dispatch(intent::LaunchGame { item.game }); });
	};

	if (!diabloFeatured) {
		renderTile(diabloTile);
		if (!portrait) {
			ImGui::SameLine(0, Scale::px(0.5F));
		} else {
			ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
		}
	}
	if (!hellfireFeatured) {
		renderTile(hellfireTile);
		if (!portrait) {
			ImGui::SameLine(0, Scale::px(0.5F));
		} else {
			ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
		}
	}
	if (!demoFeatured) {
		renderTile(demoTile);
	}

	// Russian voice-pack offer as a slim banner.
	if (state.diablo.available && !state.russianVoiceInstalled) {
		ImGui::Dummy(ImVec2(0, Scale::px(0.6F)));
		const ImVec2 bannerSize(width, Scale::px(2.4F));
		if (ImGui::InvisibleButton("ruvoice", bannerSize)) {
			dispatch(intent::UiOpenDialog { Dialog::ConfirmDownloadRu });
		}
		ImDrawList *draw = ImGui::GetWindowDrawList();
		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();
		const bool hovered = ImGui::IsItemHovered();
		draw->AddRectFilled(min, max,
		    Theme::colorU32(hovered ? ColorRole::PanelHover : ColorRole::Panel), Scale::px(0.35F));
		draw->AddRect(min, max, Theme::colorU32(ColorRole::GoldDim), Scale::px(0.35F), 0, Scale::px(0.06F));
		ImGui::PushFont(Theme::font(FontRole::IconBig));
		const ImVec2 iconSize = ImGui::CalcTextSize(icons::Music);
		ImGui::PopFont();
		ImGui::PushFont(Theme::font(FontRole::IconBig));
		draw->AddText(ImVec2(min.x + Scale::px(0.7F), min.y + (bannerSize.y - iconSize.y) * 0.5F),
		    Theme::colorU32(ColorRole::GoldBright), icons::Music);
		ImGui::PopFont();
		draw->AddText(Theme::font(FontRole::Body), Scale::px(0.95F),
		    ImVec2(min.x + Scale::px(2.6F), min.y + (bannerSize.y - Scale::px(1.1F)) * 0.5F),
		    Theme::colorU32(ColorRole::TextBody),
		    "Русская озвучка и тексты · ru.mpq");
		ImGui::PushFont(Theme::font(FontRole::IconBig));
		const ImVec2 chevSize = ImGui::CalcTextSize(icons::Play);
		ImGui::PopFont();
		ImGui::PushFont(Theme::font(FontRole::IconBig));
		draw->AddText(ImVec2(max.x - chevSize.x - Scale::px(0.7F), min.y + (bannerSize.y - chevSize.y) * 0.5F),
		    Theme::colorU32(ColorRole::GoldBright), icons::Play);
		ImGui::PopFont();
	}
}

} // namespace

void Home(const LauncherState &state, const Dispatcher &dispatch, const BackgroundArt &art)
{
	if (!state.hasAnyFiles()) {
		RenderFirstRun(state, dispatch, art);
		return;
	}

	// Nudge the block towards the vertical center of the content area.
	const float heroH = Scale::portrait() ? Scale::px(14.0F) : Scale::px(11.0F);
	const float shelfH = Scale::px(4.2F) * 2.0F + Scale::px(3.5F);
	const float free = ImGui::GetContentRegionAvail().y - (heroH + shelfH);
	ImGui::Dummy(ImVec2(0, std::max(free * 0.25F, Scale::px(0.5F))));

	RenderHeroAndShelf(state, dispatch, art);
}

namespace {

void RenderChecklist(const LauncherState &state, const Dispatcher &dispatch)
{
	if (ImGui::BeginTable("files", 4, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("status", ImGuiTableColumnFlags_WidthFixed, Scale::px(1.6F));
		ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("detail", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("actions", ImGuiTableColumnFlags_WidthFixed, Scale::px(2.4F));

		for (size_t i = 0; i < kKnownFileCount; ++i) {
			const KnownFile id = static_cast<KnownFile>(i);
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

			ImGui::TableNextColumn();
			if (present && spec.downloadable) {
				Theme::pushButtonStyle(false);
				const std::string label = std::string(icons::Trash) + "##del" + std::to_string(i);
				if (ImGui::SmallButton(label.c_str())) {
					dispatch(intent::DeleteDownloadedFile { id });
				}
				Theme::popButtonStyle();
			}
		}
		ImGui::EndTable();
	}
}

} // namespace

void Data(const LauncherState &state, const Dispatcher &dispatch)
{
	const float width = ImGui::GetContentRegionAvail().x;

	ImGui::PushFont(Theme::font(FontRole::BodyBold));
	ImGui::TextUnformatted("Папка с файлами игры");
	ImGui::PopFont();

	if (state.dataFolder.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
		ImGui::TextWrapped("Не выбрана — используются папки приложения.");
		ImGui::PopStyleColor();
	} else {
		ImGui::TextWrapped("%s", state.dataFolder.string().c_str());
	}

	widgets::IconButton(icons::Folder, "Изменить папку", false,
	    ImVec2(std::min(width * 0.6F, Scale::px(16.0F)), Scale::px(2.2F)), [&dispatch] {
		    dispatch(intent::SelectDataFolder {});
	    });

	ImGui::Dummy(ImVec2(0, Scale::px(0.6F)));
	Theme::drawDivider(ImGui::GetCursorScreenPos(),
	    ImGui::GetCursorScreenPos() + ImVec2(width, 0), 0.6F);
	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));

	ImGui::PushFont(Theme::font(FontRole::BodyBold));
	ImGui::TextUnformatted("Файлы игры");
	ImGui::PopFont();
	RenderChecklist(state, dispatch);

	ImGui::Dummy(ImVec2(0, Scale::px(0.6F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	ImGui::Text("%s Свободно: %s", icons::Hdd, FormatBytes(state.freeDiskBytes).c_str());
	ImGui::PopStyleColor();
}

void About(const LauncherState &)
{
	ImGui::Dummy(ImVec2(0, Scale::px(1.0F)));
	Theme::pushFont(FontRole::Heading);
	widgets::CenteredText("DIABLO", ColorRole::TextHeading);
	Theme::popFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextBody));
	widgets::CenteredText("DevilutionX — порт для Aurora OS");
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0, Scale::px(0.8F)));

	ImGui::TextWrapped(
	    "DevilutionX — современный открытый порт классического Diablo (1996) "
	    "и дополнения Hellfire с исправлением сотен ошибок оригинала.\n\n"
	    "Этот лаунчер помогает настроить игру: найти файлы оригинала, скачать "
	    "бесплатную демо-версию или русскую озвучку.");

	ImGui::Dummy(ImVec2(0, Scale::px(0.8F)));
	Theme::drawDivider(ImGui::GetCursorScreenPos(),
	    ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionAvail().x, 0), 0.6F);
	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));

	ImGui::PushFont(Theme::font(FontRole::BodyBold));
	ImGui::TextUnformatted("Версия");
	ImGui::PopFont();
	ImGui::Text("DevilutionX %s (порт для Aurora OS)", LAUNCHER_APP_VERSION);

	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
	ImGui::PushFont(Theme::font(FontRole::BodyBold));
	ImGui::TextUnformatted("Ссылки");
	ImGui::PopFont();
	ImGui::Text("%s  github.com/diasurgical/DevilutionX", icons::Globe);
	ImGui::Text("%s  Diablo © 1996 Blizzard Entertainment", icons::Book);

	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	ImGui::TextWrapped(
	    "Лицензия: Sustainable Use License — использование в некоммерческих целях. "
	    "Diablo, Blizzard Entertainment — товарные знаки Blizzard Entertainment, Inc. "
	    "Порт не связан с Blizzard и не одобрен ею.");
	ImGui::PopStyleColor();
}

} // namespace launcher::ui::screens

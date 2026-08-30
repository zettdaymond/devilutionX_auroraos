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

void RenderHero(const LauncherState &state, const Dispatcher &dispatch)
{
	const float width = ImGui::GetContentRegionAvail().x;

	ImGui::Dummy(ImVec2(0, Scale::px(2.0F)));
	Theme::pushFont(FontRole::Heading);
	widgets::CenteredText("DIABLO", ColorRole::TextHeading);
	Theme::popFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextBody));
	widgets::CenteredText("DevilutionX для Aurora OS");
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, Scale::px(1.6F)));

	const ImVec2 buttonSize(std::min(width, Scale::px(20.0F)), Scale::px(2.6F));
	widgets::CenteredText("Файлы оригинальной игры не найдены.");
	ImGui::Dummy(ImVec2(0, Scale::px(0.6F)));
	widgets::IconButton(icons::Folder, "Выбрать файлы игры", false, buttonSize, [&dispatch] {
		dispatch(intent::SelectDataFolder {});
	});
	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
	widgets::CenteredText("или");
	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
	widgets::IconButton(icons::Download, "Скачать бесплатное демо", true, buttonSize, [&dispatch] {
		dispatch(intent::UiOpenDialog { Dialog::ConfirmDownloadDemo });
	});

	ImGui::Dummy(ImVec2(0, Scale::px(1.0F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	const char *hint = "Для полной версии скопируйте DIABDAT.MPQ с диска\nили купите игру на GoG.com. Для Hellfire нужны hellfire.mpq,\nhfmonk.mpq, hfmusic.mpq и hfvoice.mpq.";
	widgets::CenteredText(hint);
	ImGui::PopStyleColor();
}

void RenderCardRow(const LauncherState &state, const Dispatcher &dispatch)
{
	const bool portrait = Scale::portrait();
	const float available = ImGui::GetContentRegionAvail().x;
	const float cardWidth = portrait
	    ? available
	    : std::min(available * 0.32F, Scale::px(22.0F));
	const ImVec2 cardSize(cardWidth, Scale::px(4.6F));

	// Status lines
	const std::string diabloStatus = state.diablo.available ? "Готово к запуску" : "Нужен DIABDAT.MPQ";
	std::string hellfireStatus;
	if (state.hellfire.available) {
		hellfireStatus = "Готово к запуску";
	} else if (state.diablo.available) {
		hellfireStatus = "Не хватает: " + PluralFiles(state.hellfire.missingFiles.size());
	} else {
		hellfireStatus = "Нужны файлы Diablo и Hellfire";
	}
	const std::string demoStatus = state.demo.available
	    ? "Бесплатная демо-версия"
	    : std::string("Скачать · ") + FormatBytes(FileSpecOf(KnownFile::Spawn).expectedSizeBytes);

	// One title size for the whole row, fitted to the longest title
	// ("HELLFIRE") so all cards share the same baseline.
	const float iconSize = std::min(Scale::px(2.4F), cardSize.y * 0.62F);
	const float titleMaxWidth = cardWidth - iconSize - Scale::px(2.6F);
	const float uniformTitleSize = widgets::FitFontSizeFor(FontRole::Heading, Scale::px(2.0F), "HELLFIRE",
	    titleMaxWidth, Scale::px(0.95F));

	if (portrait) {
		widgets::GameCard("DIABLO", diabloStatus.c_str(), icons::Fire, state.diablo.available, true, cardSize,
		    uniformTitleSize,
		    [&dispatch] { dispatch(intent::LaunchGame { ExitAction::LaunchDiablo }); });
		ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
		widgets::GameCard("HELLFIRE", hellfireStatus.c_str(), icons::Gamepad, state.hellfire.available, true,
		    cardSize, uniformTitleSize,
		    [&dispatch] { dispatch(intent::LaunchGame { ExitAction::LaunchHellfire }); });
		ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
		widgets::GameCard("DEMO", demoStatus.c_str(), icons::Download, true, state.demo.available, cardSize,
		    uniformTitleSize,
		    [&dispatch] { dispatch(intent::LaunchGame { ExitAction::LaunchDemo }); });
		return;
	}

	ImGui::BeginGroup();
	widgets::GameCard("DIABLO", diabloStatus.c_str(), icons::Fire, state.diablo.available, true, cardSize,
	    uniformTitleSize,
	    [&dispatch] { dispatch(intent::LaunchGame { ExitAction::LaunchDiablo }); });
	ImGui::SameLine(0, Scale::px(0.5F));
	widgets::GameCard("HELLFIRE", hellfireStatus.c_str(), icons::Gamepad, state.hellfire.available, true, cardSize,
	    uniformTitleSize,
	    [&dispatch] { dispatch(intent::LaunchGame { ExitAction::LaunchHellfire }); });
	ImGui::SameLine(0, Scale::px(0.5F));
	widgets::GameCard("DEMO", demoStatus.c_str(), icons::Download, true, state.demo.available, cardSize,
	    uniformTitleSize,
	    [&dispatch] { dispatch(intent::LaunchGame { ExitAction::LaunchDemo }); });
	ImGui::EndGroup();
}

void RenderRuVoiceOffer(const LauncherState &state, const Dispatcher &dispatch)
{
	if (state.diablo.available && !state.russianVoiceInstalled) {
		ImGui::Dummy(ImVec2(0, Scale::px(0.8F)));
		const float width = ImGui::GetContentRegionAvail().x;
		const ImVec2 buttonSize(width, Scale::px(2.2F));
		widgets::IconButton(icons::Music, "Скачать русскую озвучку (~150 МБ)", false, buttonSize, [&dispatch] {
			dispatch(intent::UiOpenDialog { Dialog::ConfirmDownloadRu });
		});
	}
}

} // namespace

void Home(const LauncherState &state, const Dispatcher &dispatch)
{
	if (!state.hasAnyFiles()) {
		RenderHero(state, dispatch);
		return;
	}

	// Nudge the block towards the vertical center of the content area.
	const float cardHeight = Scale::px(4.6F);
	const float gap = Scale::px(0.4F);
	const float ruOffer = (state.diablo.available && !state.russianVoiceInstalled) ? Scale::px(3.0F) : 0.0F;
	const float blockHeight = Scale::portrait() ? cardHeight * 3.0F + gap * 2.0F + ruOffer
	                                            : cardHeight + ruOffer;
	const float free = ImGui::GetContentRegionAvail().y - blockHeight;
	ImGui::Dummy(ImVec2(0, std::max(free * 0.45F, Scale::px(0.8F))));

	RenderCardRow(state, dispatch);
	RenderRuVoiceOffer(state, dispatch);
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

	Theme::pushFont(FontRole::BodyBold);
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

	Theme::pushFont(FontRole::BodyBold);
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

	Theme::pushFont(FontRole::BodyBold);
	ImGui::TextUnformatted("Версия");
	ImGui::PopFont();
	ImGui::Text("DevilutionX %s (порт для Aurora OS)", LAUNCHER_APP_VERSION);

	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
	Theme::pushFont(FontRole::BodyBold);
	ImGui::TextUnformatted("Ссылки");
	ImGui::PopFont();
	ImGui::Text("%s  github.com/diasurgical/DevilutionX", icons::Globe);
	ImGui::Text("%s  Diablo © 1996 Blizzard Entertainment", icons::Book);

	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	ImGui::TextWrapped(
	    "Лицензия: Sustainable Use License — использование в некоммерческих целях. "
	    "Диablo, Blizzard Entertainment — товарные знаки Blizzard Entertainment, Inc. "
	    "Порт не связан с Blizzard и не одобрен ею.");
	ImGui::PopStyleColor();
}

} // namespace launcher::ui::screens

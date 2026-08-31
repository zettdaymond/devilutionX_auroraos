#pragma once

#include "IPathProvider.hpp"

namespace launcher {

/// Пути для Aurora OS. Оборачивает AuroraOsStandartPaths (он на Qt),
/// чтобы остальной лаунчер жил без Qt:
/// - папка настроек — базовая, переданная снаружи (на устройстве это
///   путь SDL); папка загрузок — дополнительный путь поиска MPQ
///   движка (~/.local/share/org.diasurgical/devilutionx), чтобы игра
///   находила скачанные spawn.mpq и ru.mpq без лишних настроек;
/// - среди папок-кандидатов есть и папка вшитых ресурсов (только чтение).
class AuroraPathProvider final : public IPathProvider {
public:
	explicit AuroraPathProvider(std::filesystem::path baseDir);

	[[nodiscard]] std::filesystem::path ConfigDir() override;
	[[nodiscard]] std::filesystem::path DownloadsDir() override;
	[[nodiscard]] std::vector<std::filesystem::path> CandidateDataDirs(const LauncherConfig &config) override;

private:
	std::filesystem::path m_baseDir;
};

} // namespace launcher

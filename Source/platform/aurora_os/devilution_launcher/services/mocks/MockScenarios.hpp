#pragma once

#include "../ServiceFactory.hpp"

#include "MockDownloadService.hpp"
#include "MockWorld.hpp"

#include <memory>
#include <string>

namespace launcher {

/// Именованный сценарий для прогонов на десктопе (без устройства,
/// сети и настоящих файлов). Соединяет набор «существующих» файлов
/// с поведением загрузки:
///
///   empty          файлов нет, загрузки завершаются мгновенно
///   slow-download  файлов нет, загрузка идёт ~12 с (прогресс и отмена)
///   fail-download  файлов нет, загрузка обрывается на 50 %
///   diablo-found   есть DIABDAT.MPQ
///   hellfire-partial DIABDAT + 2 из 4 файлов Hellfire
///   full           все файлы на месте (работа с данными)
class MockScenario {
public:
/// Разбирает имя сценария; возвращает сценарий на сервисах MockWorld.
/// Для неизвестного имени выбрасывает std::invalid_argument.
	[[nodiscard]] static MockScenario ByName(const std::string &name);

	[[nodiscard]] ServiceBundle MakeBundle() const;

	[[nodiscard]] const std::string &name() const { return m_name; }

/// Доступ к «миру» — например, чтобы тест менял состояние по ходу сценария.
	[[nodiscard]] MockWorld &world() const { return *m_world; }

	static const std::vector<std::string> &Names();

private:
	MockScenario(std::string name, MockDownloadBehavior behavior);

	std::string m_name;
	MockDownloadBehavior m_behavior;
	std::shared_ptr<MockWorld> m_world;
};

} // namespace launcher

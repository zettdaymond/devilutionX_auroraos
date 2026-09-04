# -*- coding: utf-8 -*-
"""Часть 2: services/ и services/mocks/."""
import io

ROOT = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher"
R = []

def add(f, ln, marker, new):
    R.append((f, ln, marker, new))

add("services/mocks/MockDownloadService.cpp", 14, "Figure out which", "/// Определяет, какому известному файлу соответствует путь загрузки,")
add("services/mocks/MockDownloadService.cpp", 15, "so the mock can", "/// чтобы имитация обновила нужную запись в MockWorld.")
add("services/mocks/MockDownloadService.cpp", 28, "Sleep in small slices", "/// Спит короткими порциями, чтобы быстро проснуться при отмене.")

add("services/mocks/MockDownloadService.hpp", 12, "How a mock download", "/// Как ведёт себя имитируемая загрузка.")
add("services/mocks/MockDownloadService.hpp", 19, "Simulated downloader", "/// Имитация загрузчика. Фоновый поток присылает события прогресса,")
add("services/mocks/MockDownloadService.hpp", 20, "events, updates", "/// при успехе обновляет общий MockWorld и поддерживает отмену —")
add("services/mocks/MockDownloadService.hpp", 21, "cancellation — mirrors", "/// повторяет правила работы с потоками настоящей ZoeDownloadService.")
add("services/mocks/MockDownloadService.hpp", 24, "Owns a share", "/// Держит долю владения «миром», чтобы набор сервисов оставался жив")
add("services/mocks/MockDownloadService.hpp", 25, "regardless of the", "/// независимо от времени жизни сценария-имитации.")

add("services/mocks/MockScenarios.hpp", 13, "Named scenario", "/// Именованный сценарий для прогонов на десктопе (без устройства,")
add("services/mocks/MockScenarios.hpp", 14, "network, no real", "/// сети и настоящих файлов). Соединяет набор «существующих» файлов")
add("services/mocks/MockScenarios.hpp", 17, "empty          no files", "///   empty          файлов нет, загрузки завершаются мгновенно")
add("services/mocks/MockScenarios.hpp", 18, "slow-download  no files", "///   slow-download  файлов нет, загрузка идёт ~12 с (прогресс и отмена)")
add("services/mocks/MockScenarios.hpp", 19, "fail-download  no files", "///   fail-download  файлов нет, загрузка обрывается на 50 %")
add("services/mocks/MockScenarios.hpp", 20, "diablo-found   DIABDAT", "///   diablo-found   есть DIABDAT.MPQ")
add("services/mocks/MockScenarios.hpp", 21, "hellfire-partial DIABDAT", "///   hellfire-partial DIABDAT + 2 из 4 файлов Hellfire")
add("services/mocks/MockScenarios.hpp", 22, "full           everything", "///   full           все файлы на месте (работа с данными)")
add("services/mocks/MockScenarios.hpp", 25, "Parses a scenario", "/// Разбирает имя сценария; возвращает сценарий на сервисах MockWorld.")
add("services/mocks/MockScenarios.hpp", 26, "Throws std::", "/// Для неизвестного имени выбрасывает std::invalid_argument.")
add("services/mocks/MockScenarios.hpp", 33, "Access to the world", "/// Доступ к «миру» — например, чтобы тест менял состояние по ходу сценария.")

add("services/mocks/MockServices.hpp", 12, "In-memory config", "/// Настройки в памяти; запоминает последнюю сохранённую версию,")
add("services/mocks/MockServices.hpp", 13, "on what the Store", "/// чтобы тест проверял, что именно Store записал.")
add("services/mocks/MockServices.hpp", 25, 'File "system" backed', "/// Файловая «система» поверх MockWorld: любая просканированная папка")
add("services/mocks/MockServices.hpp", 26, "same scripted contents", "/// выдаёт одно и то же предписанное содержимое, свободное место —")
add("services/mocks/MockServices.hpp", 27, "clears the entry", "/// из «мира», удаление убирает запись. Держит долю владения «миром»")
add("services/mocks/MockServices.hpp", 28, "valid regardless", "/// ради времени жизни набора сервисов.")

add("services/mocks/MockWorld.cpp", 92, "All four Hellfire", "\t// Все четыре MPQ Hellfire без DIABDAT: запустить нечего,")
add("services/mocks/MockWorld.cpp", 93, "hero falls back", "\t// hero-панель показывает состояние «требуются файлы».")

add("services/mocks/MockWorld.hpp", 13, "Shared fake environment", "/// Общая поддельная среда для сервисов-имитаций: какие файлы «существуют»,")
add("services/mocks/MockWorld.hpp", 14, "their sizes", "/// их размеры и поддельное свободное место. MockGameFilesService")
add("services/mocks/MockWorld.hpp", 15, "reads it; MockDownload", "/// читает её; MockDownloadService обновляет при успехе и отмене.")
add("services/mocks/MockWorld.hpp", 16, "Thread-safe because", "/// Потокобезопасна: имитация загрузки работает в отдельном потоке.")
add("services/mocks/MockWorld.hpp", 32, "Consistent copy", "\t/// Непротиворечивая копия всего состояния «мира».")
add("services/mocks/MockWorld.hpp", 37, "Apply a named preset", "\t/// Применяет именованный набор (\"empty\", \"diablo-found\", \"hellfire-partial\",")
add("services/mocks/MockWorld.hpp", 38, "Returns false for unknown", '\t/// "full"). Для неизвестного имени возвращает false.')
add("services/mocks/MockWorld.hpp", 41, "All valid preset", "\t/// Все допустимые имена наборов — для вывода в --help.")

add("services/AuroraPathProvider.hpp", 7, "Aurora OS path provider", "/// Пути для Aurora OS. Оборачивает AuroraOsStandartPaths (он на Qt),")
add("services/AuroraPathProvider.hpp", 8, "the rest of the launcher", "/// чтобы остальной лаунчер жил без Qt:")
add("services/AuroraPathProvider.hpp", 9, "configDir: base dir", "/// - папка настроек — базовая, переданная снаружи (на устройстве это")
add("services/AuroraPathProvider.hpp", 10, "downloadsDir: the engine", "///   путь SDL); папка загрузок — дополнительный путь поиска MPQ")
add("services/AuroraPathProvider.hpp", 11, "(~/.local/share", "///   движка (~/.local/share/org.diasurgical/devilutionx), чтобы игра")
add("services/AuroraPathProvider.hpp", 12, "spawn.mpq / ru.mpq", "///   находила скачанные spawn.mpq и ru.mpq без лишних настроек;")
add("services/AuroraPathProvider.hpp", 13, "read-only bundled", "/// - среди папок-кандидатов есть и папка вшитых ресурсов (только чтение).")

add("services/ConfigService.cpp", 65, "Fall back to a plain", "\t// Если переименование невозможно — просто перезаписываем файл.")

add("services/ConfigService.hpp", 7, "TOML-backed config", "/// Настройки в TOML (launcher.conf). Файл записывается атомарно:")
add("services/ConfigService.hpp", 8, "first to a temporary", "/// сначала во временный файл рядом с целевым, затем переименовывается.")

add("services/DesktopPathProvider.hpp", 7, "Portable path provider", "/// Переносимый вариант путей: всё живёт в одной базовой папке")
add("services/DesktopPathProvider.hpp", 8, "on desktop builds", "/// (в десктопной сборке — путь SDL, его передаёт вызывающий код;")
add("services/DesktopPathProvider.hpp", 9, "free of SDL", "/// сам класс не зависит от SDL).")

add("services/GameFilesService.hpp", 7, "std::filesystem-based", "/// Реализация на std::filesystem — одинакова для Aurora OS и десктопа.")

add("services/IConfigService.hpp", 8, "Launcher settings", "/// Настройки лаунчера, живущие между запусками.")
add("services/IConfigService.hpp", 10, "Folder with the user", "/// Папка с файлами пользователя (DIABDAT.MPQ, hellfire*.mpq).")
add("services/IConfigService.hpp", 11, "Empty when", "/// Пуста, пока пользователь не выбрал папку.")
add("services/IConfigService.hpp", 15, "Persistence for", "/// Хранилище LauncherConfig. Создание должно быть дешёвым;")
add("services/IConfigService.hpp", 16, "Save() writes", "/// Save() пишет файл целиком атомарно (временный файл + переименование).")

add("services/IDownloadService.hpp", 9, "Downloads one file", "/// Качает по одному файлу за раз. Обратные вызовы могут приходить")
add("services/IDownloadService.hpp", 10, "background thread", "/// из фонового потока — реализация гарантирует их вызов,")
add("services/IDownloadService.hpp", 11, "but the receiver", "/// но получатель обязан сам переносить данные в свой поток")
add("services/IDownloadService.hpp", 12, "(the Store does", "/// (Store делает это через Dispatch()).")
add("services/IDownloadService.hpp", 16, "Progress report", "/// Отчёт о прогрессе; bytesPerSec — сглаженная мгновенная скорость.")
add("services/IDownloadService.hpp", 18, "Terminal event", "/// Завершающее событие; вызывается ровно один раз на каждый Start().")
add("services/IDownloadService.hpp", 19, "On cancel or failure", "/// При отмене или ошибке success = false, а error объясняет причину.")
add("services/IDownloadService.hpp", 25, "Begin downloading", "\t/// Начинает загрузку `url` в файл `destination`.")
add("services/IDownloadService.hpp", 26, "Starting while another", "\t/// Параллельный старт второй загрузки — ошибка программирования;")
add("services/IDownloadService.hpp", 27, "implementations log", "\t/// реализации записывают это в лог и игнорируют вызов.")
add("services/IDownloadService.hpp", 30, "Abort the active", "\t/// Прерывает текущую загрузку: вызывает onFinished(false, \"cancelled\"),")
add("services/IDownloadService.hpp", 31, "removes the partial", "\t/// и удаляет недокачанный файл.")

add("services/IGameFilesService.hpp", 10, "Result of scanning", "/// Результат поиска известных файлов игры в одной или нескольких папках:")
add("services/IGameFilesService.hpp", 11, "sizes[i] is", "/// sizes[i] — размер файла в байтах или -1, если файл не найден.")
add("services/IGameFilesService.hpp", 17, "File system access", "/// Доступ к файловой системе для данных игры. Все методы синхронные,")
add("services/IGameFilesService.hpp", 18, "safe to call", "/// их можно звать на главном потоке (папки маленькие).")
add("services/IGameFilesService.hpp", 23, "Scan folders in order", "\t/// Сканирует папки по порядку; побеждает первое найденное место.")
add("services/IGameFilesService.hpp", 24, "Comparison of file", "\t/// Имена файлов сравниваются без учёта регистра.")
add("services/IGameFilesService.hpp", 27, "Free space on", "\t/// Свободное место на разделе, где лежит путь, в байтах.")
add("services/IGameFilesService.hpp", 30, "Remove a known", "\t/// Удаляет известный файл из папки. true, если файл существовал")

add("services/IPathProvider.hpp", 10, "Platform-specific", "/// Пути, зависящие от платформы. Это единственный шов между")
add("services/IPathProvider.hpp", 11, "seam between", "/// переносимым кодом лаунчера и платформой-хостом")
add("services/IPathProvider.hpp", 12, "(Aurora OS with Qt", "/// (Aurora OS с Qt против десктопа на SDL).")
add("services/IPathProvider.hpp", 17, "Directory for launcher", "\t/// Папка служебных файлов лаунчера (launcher.conf и прочие).")
add("services/IPathProvider.hpp", 20, "Writable directory", "\t/// Папка для скачиваемого (spawn.mpq, ru.mpq), доступная для записи.")
add("services/IPathProvider.hpp", 21, "is placed into", "\t/// Она же — путь поиска MPQ для самой игры.")
add("services/IPathProvider.hpp", 24, "Folders scanned", "\t/// Папки, в которых ищутся файлы игры, по убыванию приоритета:")
add("services/IPathProvider.hpp", 25, "user-selected folder", "\t/// сначала выбранная пользователем, затем пути платформы.")

add("services/ServiceFactory.hpp", 13, "A complete set", "/// Полный набор сервисов, готовый для передачи в Store.")
add("services/ServiceFactory.hpp", 14, "the Store. Built", "/// Собирается либо из настоящих сервисов платформы, либо из имитаций.")
add("services/ServiceFactory.hpp", 22, "Real implementations", "/// Настоящие реализации. `baseDir` — путь настроек SDL, который")
add("services/ServiceFactory.hpp", 23, "by the caller", "/// получает вызывающий код (Application или десктопный main).")
add("services/ServiceFactory.hpp", 24, "provider additionally", "/// На Aurora OS поставщик путей дополнительно отдаёт папки MPQ платформы.")

add("services/ZoeDownloadService.cpp", 16, "GlobalInit/GlobalUnInit", "/// zoe::GlobalInit/GlobalUnInit действуют на весь процесс; счётчик")
add("services/ZoeDownloadService.cpp", 17, "instances (and tests)", "/// ссылок держит пары вызовов в балансе (включая тесты).")

add("services/ZoeDownloadService.hpp", 14, "backed by the zoe", "/// Реализация IDownloadService поверх загрузчика zoe.")
add("services/ZoeDownloadService.hpp", 16, "One download at", "/// - Одна загрузка за раз; повторный Start() при активной игнорируется.")
add("services/ZoeDownloadService.hpp", 17, "Progress reports", "/// - Прогресс присылается не чаще ~4 раз в секунду; скорость берётся")
add("services/ZoeDownloadService.hpp", 18, "from zoe's realtime", "///   из колбэка мгновенной скорости zoe и идёт вместе с прогрессом.")
add("services/ZoeDownloadService.hpp", 19, "Cancel() aborts", "/// - Cancel() прерывает загрузку; слушатель получает onFinished(false,")
add("services/ZoeDownloadService.hpp", 20, "the partial file", "///   \"cancelled\"), недокачанный файл удаляется.")
add("services/ZoeDownloadService.hpp", 31, "Guards m_active", "/// Охраняет m_active, m_listener и состояние троттлинга (колбэки zoe")
add("services/ZoeDownloadService.hpp", 32, "arrive on background", "/// приходят из фоновых потоков).")

def main():
    by_file = {}
    for f, ln, marker, new in R:
        by_file.setdefault(f, []).append((ln, marker, new))
    ok, bad = 0, []
    for f, items in by_file.items():
        path = f"{ROOT}\\{f}"
        lines = io.open(path, encoding="utf-8").read().splitlines()
        for ln, marker, new in items:
            cur = lines[ln - 1]
            if marker not in cur:
                bad.append((f, ln, cur.strip()[:70]))
                continue
            lines[ln - 1] = new
            ok += 1
        io.open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    print("replaced:", ok, "missed:", len(bad))
    for b in bad:
        print("MISS", b)

main()

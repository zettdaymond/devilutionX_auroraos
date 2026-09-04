# -*- coding: utf-8 -*-
"""Построчная замена английских комментариев на русский (launcher).
Ключ — (файл, номер строки); перед заменой проверяется подстрока-ориентир."""
import io, sys

ROOT = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher"

# (файл, строка): (ориентир в текущей строке, новая строка с отступом)
R = []

def add(f, ln, marker, new):
    R.append((f, ln, marker, new))

add("Application.cpp", 38, "Wraps raw RGBA", "\t/// Оборачивает сырые пиксели (RGB или RGBA) в поверхность SDL и")
add("Application.cpp", 39, "linear filtering", "\t/// загружает их как текстуру с линейной фильтрацией. По умолчанию")
add("Application.cpp", 40, "stair-steps", "\t/// SDL2 масштабирует «ближайшим» пикселем — на уменьшении видны ступеньки.")
add("Application.cpp", 113, "Decodes an embedded", "\t/// Раскодирует вшитую в бинарник картинку (JPEG-арт или PNG-иконку)")
add("Application.cpp", 114, "Returns nullptr", "\t/// в текстуру SDL. Если ассета нет или он битый — вернёт nullptr")
add("Application.cpp", 115, "callers fall back", "\t/// и запишет предупреждение в лог; вызывающий код использует запасной вариант.")
add("Application.cpp", 263, "setup() creates", "\t// Setup() создаёт контекст ImGui и грузит шрифты, поэтому вид (и его")
add("Application.cpp", 264, "FileBrowser, which queries", "\t// файловый браузер, которому нужен шрифтовый атлас) создаётся после него.")

add("Application.hpp", 28, "Owns the SDL", "/// Владеет рендерером SDL, контекстом ImGui и тройкой MVI")
add("Application.hpp", 29, "(Store + LauncherView", "/// (Store + LauncherView поверх набора сервисов) и крутит цикл")
add("Application.hpp", 30, "UI loop until", "/// интерфейса, пока пользователь не запустит игру или не закроет окно.")
add("Application.hpp", 33, "(window, company, app)", "/// - (окно, компания, приложение) собирает настоящие сервисы платформы")
add("Application.hpp", 34, "game entry point", "///   (так лаунчер вызывается из входной точки игры в Source/main.cpp);")
add("Application.hpp", 35, "(window, services)", "/// - (окно, сервисы) принимает уже готовый набор снаружи — например,")
add("Application.hpp", 36, "mock scenarios", "///   сценарии-имитации на десктопе.")
add("Application.hpp", 48, "Runs the UI loop", "/// Запускает цикл интерфейса; результат — какую игру запустить.")
add("Application.hpp", 51, "Override the first screen", "/// Задать первый экран (утилита для отладки на десктопе).")
add("Application.hpp", 54, "Open the MPQ folder", "/// Открыть выбор папки с MPQ сразу при старте (отладка на десктопе).")
add("Application.hpp", 57, "Open a dialog on startup", "/// Открыть диалог сразу при старте (отладка на десктопе):")
add("Application.hpp", 58, "confirm-demo", "/// confirm-demo | confirm-ru | hellfire-missing | error.")
add("Application.hpp", 88, "Dedicated hero artworks", "\t/// Арты hero-панелей режимов; индекс — значение ExitAction (пусто = обрезка общего фона).")
add("Application.hpp", 92, "Golden tile icons", "\t/// Золотые иконки плиток в виде заранее уменьшенных копий (256/128/64);")
add("Application.hpp", 93, "ExitAction then level", "\t/// индексы: сначала режим, затем уровень; 0 уровней = глиф FontAwesome.")

add("core/AppResult.hpp", 7, "What the launcher", "/// Какую игру лаунчер должен запустить, когда его цикл завершился.")
add("core/AppResult.hpp", 14, "Final answer", "/// Итоговый ответ лаунчера главной программе")
add("core/AppResult.hpp", 15, "see Source/main.cpp", "/// (см. Source/main.cpp — он превращает действие в аргументы командной строки движка).")

add("core/EngineLaunch.cpp", 21, "The shareware demo", "\t// Демо-версия рассчитывает на стандартные пути поиска движка")
add("core/EngineLaunch.cpp", 22, "spawn.mpq is downloaded", "\t// (spawn.mpq скачивается в дополнительную папку MPQ).")

add("core/EngineLaunch.hpp", 11, "DevilutionX command-line", "/// Аргументы командной строки DevilutionX для режима, выбранного")
add("core/EngineLaunch.hpp", 12, "the program name", "/// в лаунчере (имя самого исполняемого файла не входит).")
add("core/EngineLaunch.hpp", 14, "Diablo:    --diablo", "/// - Diablo:    --diablo  (пропускает диалог движка «какую игру запустить?»")
add("core/EngineLaunch.hpp", 15, "both Diablo and Hellfire", "///                        при установленных файлах и Diablo, и Hellfire)")
add("core/EngineLaunch.hpp", 17, "Shareware: --spawn", "/// - Shareware: --spawn   (файл spawn.mpq движок находит сам через")
add("core/EngineLaunch.hpp", 18, "no extra argument", "///                        свои пути поиска, лишние аргументы не нужны)")
add("core/EngineLaunch.hpp", 20, "For the full games", "/// Для полных версий выбранная папка передаётся ещё и как")
add("core/EngineLaunch.hpp", 21, "`--data-dir <path>`", "/// `--data-dir <путь>`: движок ставит её первой в порядок поиска MPQ,")
add("core/EngineLaunch.hpp", 22, "QSettings round-trip", "/// поэтому DIABDAT.MPQ находится гарантированно, без обхода через")
add("core/EngineLaunch.hpp", 23, "silent fallback", "/// настроек QSettings (и без риска незаметно подменить игру")
add("core/EngineLaunch.hpp", 24, "downloaded shareware", "/// скачанной демо-версией).")
add("core/EngineLaunch.hpp", 27, "Whether the launcher", "/// Должен ли результат лаунчера ещё и запомнить `dataPath` как")
add("core/EngineLaunch.hpp", 28, "user-defined MPQ search", "/// пользовательский путь поиска MPQ (QSettings на Aurora OS), чтобы")
add("core/EngineLaunch.hpp", 29, "remembered on the next", "/// папка сохранилась и на следующий запуск.")

add("core/GameFiles.hpp", 9, "Identifies every MPQ", "/// Перечисляет все MPQ-файлы, о которых знает лаунчер.")
add("core/GameFiles.hpp", 10, "Order matters", "/// Порядок важен: по нему индексируются массивы в LauncherState.")
add("core/GameFiles.hpp", 25, "Static description", "/// Неизменное описание известного файла: каноничное имя в нижнем регистре,")
add("core/GameFiles.hpp", 26, "alternative spellings", "/// встречающиеся в жизни варианты написания и ожидаемый размер загрузки")
add("core/GameFiles.hpp", 27, "only meaningful", "/// (имеет смысл только для скачиваемых файлов, иначе 0).")
add("core/GameFiles.hpp", 36, "Catalog of all known", "/// Каталог всех известных файлов. Держать синхронно с KnownFile.")
add("core/GameFiles.hpp", 52, "Case-insensitive", "/// Сравнение имени файла с известными вариантами без учёта регистра.")
add("core/GameFiles.hpp", 53, "Only the plain canonical", "/// Принимается только каноничное имя — на практике MPQ встречаются")
add("core/GameFiles.hpp", 54, "lower or upper case", "/// в нижнем и верхнем регистрах, оба покрыты.")
add("core/GameFiles.hpp", 57, "URL a downloadable", "/// Откуда скачивается файл (пусто, если файл не скачивается).")

add("core/Intent.hpp", 13, "User actions and", "/// Действия пользователя и внутренние события — маленькие типы-значения.")
add("core/Intent.hpp", 14, "The view dispatches", "/// Интерфейс отправляет их в Store; только Store превращает их")
add("core/Intent.hpp", 15, "translates them", "/// в изменения состояния и побочные эффекты.")
add("core/Intent.hpp", 26, "transient toast", "/// Интерфейс закрыл короткое всплывающее уведомление.")
add("core/Intent.hpp", 30, "Opens the file browser", "/// Открыть файловый браузер, чтобы пользователь указал папку")
add("core/Intent.hpp", 31, "with their MPQ files", "/// со своими MPQ-файлами.")
add("core/Intent.hpp", 33, "confirmed a folder", "/// Пользователь выбрал папку в файловом браузере.")
add("core/Intent.hpp", 37, "closed the file browser", "/// Пользователь закрыл браузер, не выбрав папку.")
add("core/Intent.hpp", 39, "Re-scan all candidate", "/// Пересканировать все папки-кандидаты (после изменений, загрузок, удалений).")
add("core/Intent.hpp", 41, "Delete a previously", "/// Удалить ранее скачанный файл (spawn.mpq / ru.mpq).")
add("core/Intent.hpp", 47, "tapped a game card", "/// Пользователь нажал на карточку игры.")
add("core/Intent.hpp", 53, "Start downloading", "/// Начать загрузку (после подтверждения размера и свободного места).")
add("core/Intent.hpp", 59, "Internal events marshalled", "// -- Внутренние события, переданные из потока загрузки --")

add("core/LauncherState.hpp", 14, "Top-level screen", "/// Верхний уровень: какой экран показывает лаунчер. Навигация")
add("core/LauncherState.hpp", 15, "one screen at a time", "/// взаимоисключающая — один экран, плюс не более одного диалога.")
add("core/LauncherState.hpp", 22, "Modal dialog", "/// Диалог поверх экрана.")
add("core/LauncherState.hpp", 32, "Availability of one", "/// Доступность одного запускаемого режима игры.")
add("core/LauncherState.hpp", 38, "State of a single", "/// Состояние одной загрузки (демо или русская озвучка).")
add("core/LauncherState.hpp", 49, "Single source of truth", "/// Единственный источник истины для всего интерфейса лаунчера.")
add("core/LauncherState.hpp", 50, "pure function", "/// Интерфейс — чистая функция от этой структуры; меняет её")
add("core/LauncherState.hpp", 51, "only by Store", "/// только Store, и только на главном потоке.")
add("core/LauncherState.hpp", 81, "Short-lived notification", "// Короткое уведомление внизу экрана")

add("core/Store.cpp", 60, "must be defined before", "// (определения должны идти до Poll()/Reduce(): специализации")
add("core/Store.cpp", 61, "implicitly instantiated", "// должны быть видны до неявного создания шаблона)")
add("core/Store.cpp", 141, "finished/failed download", "// Закрытие оверлея завершённой или упавшей загрузки очищает её целиком.")

add("core/Store.hpp", 16, 'The MVI "reducer"', "/// «Редьюсер» из схемы MVI: владеет LauncherState и превращает Intents")
add("core/Store.hpp", 17, "state changes plus", "/// в изменения состояния и побочные эффекты через переданные сервисы.")
add("core/Store.hpp", 20, "may be called from any", "/// - Dispatch() можно звать из любого потока (он только кладёт в очередь);")
add("core/Store.hpp", 21, "once per frame", "/// - Poll() вызывается раз в кадр на главном потоке;")
add("core/Store.hpp", 22, "must only be read", "/// - State() читается только на главном потоке.")
add("core/Store.hpp", 30, "Load the config", "\t/// Читает настройки и делает первичный поиск файлов.")
add("core/Store.hpp", 33, "Enqueue an intent", "\t/// Ставит интент в очередь. Потокобезопасно; применяется при")
add("core/Store.hpp", 34, "next Poll()", "\t/// следующем Poll() на главном потоке.")
add("core/Store.hpp", 37, "Apply all queued", "\t/// Применяет все накопленные интенты. Вызывается раз в кадр перед отрисовкой.")
add("core/Store.hpp", 45, "One handler per intent", "\t// По одному обработчику на тип интента; специализации определены")
add("core/Store.hpp", 46, "intent listed in", "\t// в Store.cpp для каждого варианта из Intent.")

add("main.cpp", 1, "Desktop entry point", "/// Десктопная входная точка лаунчера DevilutionX (сборка для разработки).")
add("main.cpp", 3, "Lets the launcher run", "/// Позволяет запускать лаунчер отдельно — для быстрой работы над интерфейсом")
add("main.cpp", 4, "scripted mock scenarios", "/// и прохождения пользовательских сценариев по скриптам-имитациям,")
add("main.cpp", 7, "devilution_launcher [", "///   devilution_launcher [--mock-scenario=<имя>] [--window=<ШxВ]")
add("main.cpp", 9, "Scenarios: empty", "/// Сценарии: empty, slow-download, fail-download, diablo-found,")
add("main.cpp", 10, "hellfire-partial, full", "/// hellfire-partial, full (см. services/mocks/MockScenarios.cpp).")
add("main.cpp", 12, "Must precede every", "// Должен стоять раньше всех включений SDL: свой обычный main().")

def main():
    by_file = {}
    for f, ln, marker, new in R:
        by_file.setdefault(f, []).append((ln, marker, new))
    ok, bad = 0, []
    for f, items in by_file.items():
        path = f"{ROOT}\\{f}"
        lines = io.open(path, encoding="utf-8").read().splitlines(keepends=False)
        for ln, marker, new in items:
            cur = lines[ln - 1]
            if marker not in cur:
                bad.append((f, ln, cur.strip()[:60]))
                continue
            lines[ln - 1] = new
            ok += 1
        io.open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    print("replaced:", ok, "missed:", len(bad))
    for b in bad:
        print("MISS", b)

main()

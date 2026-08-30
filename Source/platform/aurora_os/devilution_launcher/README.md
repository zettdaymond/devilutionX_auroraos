# Лаунчер DevilutionX для Aurora OS

In-game лаунчер порта DevilutionX (Diablo / Hellfire) на Aurora OS.
Отвечает за первичную настройку: поиск файлов оригинальной игры,
загрузку бесплатной демо-версии и русской озвучки, выбор режима запуска
(Diablo / Hellfire / Shareware) — и передаёт решение движку.

Написан на C++20 / Dear ImGui (SDL2-рендер), стилизован под Diablo:
тёмно-коричневая палитра с золотом, шрифт Exocet для заголовков,
Beaufort для основного текста (с кириллицей), FontAwesome для иконок.

## Архитектура — MVI (Elm)

Однонаправленный поток данных, естественный для immediate-mode ImGui:

```
LauncherState (core/LauncherState.hpp)      — единственный источник истины
      ▲  рендер каждый кадр (чистая функция от состояния)
LauncherView (ui/)                          — экраны, диалоги, виджеты
      │  dispatch(Intent)                   — единственный способ изменить что-либо
Store (core/Store.cpp)                      — очередь интентов + редьюсер
      │  побочные эффекты только через интерфейсы сервисов
services/                                    — IConfigService, IGameFilesService,
                                              IDownloadService, IPathProvider
```

Правила:
- `core/` и `services/` не знают про SDL/ImGui/Qt;
- `ui/` не обращается к сервисам и файловой системе напрямую;
- состояние мутируется только на главном потоке в `Store::poll()`;
  колбэки загрузчика (фоновый поток) приходят как интенты
  `EvDownloadProgress` / `EvDownloadFinished` через thread-safe
  `Store::dispatch()`.

### Каталоги

| Путь | Содержимое |
|---|---|
| `core/` | `LauncherState`, `Intent` (std::variant), `Store`, `AppResult`, каталог MPQ (`GameFiles`) |
| `services/` | интерфейсы + портабельные реализации (TOML-конфиг, сканер ФС, обёртка zoe) + `ServiceFactory` |
| `services/mocks/` | `MockWorld` (фейковая ФС) + мок-сервисы + сценарии `MockScenario` |
| `ui/` | `Theme` (шрифты/палитра/стиль), `Scale` (rem-масштабирование), `LauncherView` (роутер) |
| `ui/screens/` | Home (карточки игр / hero), Data (чек-лист файлов), About |
| `ui/dialogs/` | подтверждение загрузки, оверлей прогресса (скорость/ETA/отмена), диалог недостающих файлов Hellfire, ошибки, тосты |
| `tests/` | GTest: логика Store на моках + реальный GameFilesService |

### Сервисы и платформы

- **Портабельные** (десктоп и Aurora): `ConfigService` (toml++, атомарная запись),
  `GameFilesService` (std::filesystem, регистронезависимый поиск MPQ),
  `ZoeDownloadService` (zoe, троттлинг прогресса ~4 Гц, отмена),
  `DesktopPathProvider`.
- **Aurora OS**: `AuroraPathProvider` — обёртка над `StandartPaths` (Qt за
  пределами лаунчера не используется). Скачанные файлы кладутся в
  дополнительный путь поиска MPQ движка.
- **Моки** (только десктоп-сборка): общий `MockWorld` + `MockDownloadService`
  с поведениями InstantSuccess / SlowSuccess / FailAtHalf.

## Сборка

### Десктоп (итерации по UI, MSYS2 MinGW64 + Ninja)

```bash
cd Source/platform/aurora_os/devilution_launcher
cmake -S . -B build-desktop -G Ninja -DLAUNCHER_DESKTOP=ON -DLAUNCHER_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-desktop
ctest --test-dir build-desktop            # unit-тесты
./build-desktop/devilution_launcher.exe   # при запуске нужен PATH с D:\msys64\mingw64\bin
```

### Aurora OS

Сборка через Qt Creator + Aurora OS Build Engine (проект `aurora_project/`).
Лаунчер компилируется как объектная библиотека `devilution_launcher`,
линкуется в общий бинарник `org.diasurgical.devilutionx`; отдельного
приложения нет — лаунчер и игра работают в одном процессе
(см. `Source/main.cpp`, секция `AURORA_OS`).

## Запуск и пользовательские сценарии

Десктоп-билд поддерживает флаги:

```
devilution_launcher [--mock-scenario=<имя>] [--screen=home|data|about] [--window=<WxH>]
```

Сценарии (`--mock-scenario=`) позволяют пройти пользовательские потоки
без устройства и сети — мок-мир эмулирует и файлы, и загрузки:

| Сценарий | Состояние | Что проверять |
|---|---|---|
| `empty` | файлов нет | hero-экран → выбор папки / загрузка демо |
| `slow-download` | файлов нет, загрузка ~12 с | оверлей: прогресс, скорость, ETA, отмена |
| `fail-download` | файлов нет, обрыв на 50 % | панель ошибки, «Повторить» |
| `diablo-found` | есть DIABDAT.MPQ | карточка DIABLO активна, предложение ru.mpq |
| `hellfire-partial` | DIABDAT + 2 из 4 файлов Hellfire | диалог недостающих файлов |
| `full` | все файлы | чек-лист, удаление скачанного, свободное место |

`--screen=` открывает приложение сразу на нужном экране (удобно для
быстрых скриншотов), `--window=` задаёт размер окна (портрет/ландшафт).

## Пользовательский поток

1. **Первый запуск** (файлов нет): hero-экран с двумя путями —
   «Выбрать файлы игры» (проводник MPQ) или «Скачать бесплатное демо».
2. **Выбор папки**: сканирование известных MPQ (регистронезависимо),
   статус каждой карточки обновляется; путь сохраняется в `launcher.conf`.
3. **Запуск**: тап по карточке. Diablo/Hellfire — только при полном
   наборе файлов (для Hellfire — все 4 доп. файла, иначе диалог со
   списком недостающих). Demo без spawn.mpq → подтверждение загрузки.
4. **Загрузка** (демо/русская озвучка): проверка свободного места →
   оверлей с прогрессом, скоростью, ETA и отменой; после успеха —
   рескан и тост.
5. **Данные**: чек-лист найденных файлов с размерами и расположением,
   смена папки, удаление скачанного, свободное место.
6. **About**: версия порта, ссылки, лицензии.

Решение лаунчера возвращается движку как `AppResult`
(`ExitAction::LaunchDiablo | LaunchHellfire | LaunchDemo` + путь к данным);
`Source/main.cpp` транслирует его в флаги `--hellfire`/`--spawn` и
`AuroraOsStandartPaths::SetUserDefinedMPQSearchPath`.

## Адаптивная вёрстка

Все размеры выражены в rem: `rem = clamp(диагональ_вьюпорта / 44,
12·DPI, 26·DPI)` (см. `ui/Scale.cpp`) — интерфейс плавно масштабируется
между телефоном, планшетом и окном произвольного размера. Структурный
перелом один: портрет (карточки столбиком, навигация снизу) и
ландшафт (карточки в ряд, навигация сверху). Тексты в карточках
подгоняются по ширине (уменьшение кегля / многоточие с сохранением
UTF-8-глиф).

#include "Application.hpp"

#include "DPIHandler.hpp"
#include "core/Store.hpp"
#include "ui/LauncherView.hpp"
#include "ui/Theme.hpp"

// stb_image декодирует JPEG/PNG сам: сборка SDL_image для Aurora не
// содержит JPEG-загрузчика, и ради него не хочется тащить libjpeg в движок.
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"
// Ручные «мипмапы» для иконок: SDL_Renderer не генерирует уровни, а
// билинейка при сильной минификации рассыпает края.
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "thirdparty/stb_image_resize2.h"

#include <SDL2/SDL.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <cmrc/cmrc.hpp>

#ifdef AURORA_OS
#   include "../StandartPaths.hpp"
#endif

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <thread>
#include <utility>
#include <vector>

#ifdef AURORA_OS
#	include <dbus/dbus.h>
#	include <unistd.h>
#	include <wayland-client.h>
#	include <SDL_syswm.h>
#endif

CMRC_DECLARE(assets);

namespace App {

namespace {

	/// Оборачивает сырые пиксели (RGB или RGBA) в поверхность SDL и
	/// загружает их как текстуру с линейной фильтрацией. По умолчанию
	/// SDL2 масштабирует «ближайшим» пикселем — на уменьшении видны ступеньки.
SDL_Texture *MakeTextureFromPixels(SDL_Renderer *renderer, const unsigned char *pixels, int width,
    int height, int components)
{
	const Uint32 format = components == 4 ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGB24;
	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
	    const_cast<unsigned char *>(pixels), width, height, components * 8, width * components, format);
	if (surface == nullptr) {
		spdlog::warn("SDL_CreateRGBSurfaceWithFormatFrom failed: {}", SDL_GetError());
		return nullptr;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture == nullptr) {
		spdlog::warn("SDL_CreateTextureFromSurface failed: {}", SDL_GetError());
	} else {
		SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
	}
	SDL_FreeSurface(surface);
	return texture;
}

/// Строит цепочку уменьшенных копий иконки (256→128→64, box-фильтр) —
/// ручная замена мипмапам, которых SDL_Renderer не генерирует. Возвращает
/// число готовых уровней (0 = использовать FontAwesome-фолбэк).
int BuildIconLevels(SDL_Renderer *renderer, const char *path, SDL_Texture *outTextures[3], ImVec2 outSizes[3])
{
	try {
		auto file = cmrc::assets::get_filesystem().open(path);
		int width = 0;
		int height = 0;
		int components = 0;
		unsigned char *source = stbi_load_from_memory(
		    reinterpret_cast<const unsigned char *>(file.begin()), static_cast<int>(file.size()),
		    &width, &height, &components, 0);
		if (source == nullptr) {
			spdlog::warn("stbi_load({}) failed: {}", path, stbi_failure_reason());
			return 0;
		}
		constexpr int kSizes[3] = { 256, 128, 64 };
		int count = 0;
		for (int side : kSizes) {
			if (side > width) {
				continue;
			}
			const unsigned char *pixels = source;
			std::vector<unsigned char> shrunk;
			if (side != width) {
				shrunk.resize(static_cast<size_t>(side) * side * components);
				const stbir_pixel_layout layout = components == 4 ? STBIR_RGBA : STBIR_RGB;
				if (stbir_resize(source, width, height, 0, shrunk.data(), side, side, 0, layout,
			        STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_BOX)
				    == nullptr) {
					spdlog::warn("stbir_resize({} -> {}) failed", path, side);
					continue;
				}
				pixels = shrunk.data();
			}
			SDL_Texture *texture = MakeTextureFromPixels(renderer, pixels, side, side, components);
			if (texture == nullptr) {
				continue;
			}
			outTextures[count] = texture;
			outSizes[count] = ImVec2(static_cast<float>(side), static_cast<float>(side));
			++count;
		}
		stbi_image_free(source);
		return count;
	} catch (const std::system_error &err) {
		spdlog::info("{} not bundled ({}), using fallback", path, err.what());
		return 0;
	}
}

	/// Раскодирует вшитую в бинарник картинку (JPEG-арт или PNG-иконку)
	/// в текстуру SDL. Если ассета нет или он битый — вернёт nullptr
	/// и запишет предупреждение в лог; вызывающий код использует запасной вариант.
SDL_Texture *LoadAssetTexture(SDL_Renderer *renderer, const char *path, ImVec2 &outSize)
{
	try {
		auto file = cmrc::assets::get_filesystem().open(path);
		int width = 0;
		int height = 0;
		int components = 0;
		unsigned char *pixels = stbi_load_from_memory(
		    reinterpret_cast<const unsigned char *>(file.begin()), static_cast<int>(file.size()),
		    &width, &height, &components, 0);
		if (pixels == nullptr) {
			spdlog::warn("stbi_load({}) failed: {}", path, stbi_failure_reason());
			return nullptr;
		}
		SDL_Texture *texture = MakeTextureFromPixels(renderer, pixels, width, height, components);
		outSize = ImVec2(static_cast<float>(width), static_cast<float>(height));
		stbi_image_free(pixels);
		return texture;
	} catch (const std::system_error &err) {
		spdlog::info("{} not bundled ({}), using fallback", path, err.what());
		return nullptr;
	}
}

} // namespace

Application::Application(SDL_Window *window, const std::string &companyNamespace, const std::string &appName)
    : m_window(window)
    , m_companyNamespace(companyNamespace)
    , m_appName(appName)
{
	char *prefPath = SDL_GetPrefPath(m_companyNamespace.c_str(), m_appName.c_str());
	const std::filesystem::path baseDir = prefPath != nullptr ? std::filesystem::path(prefPath)
	                                                          : std::filesystem::temp_directory_path();
	SDL_free(prefPath);

	// diablo.ini живёт в конфиг-каталоге движка, а он не совпадает с
	// базовой папкой лаунчера: на Aurora движок ходит через Qt StandartPaths,
	// на десктопе — в собственный SDL_GetPrefPath("diasurgical", "devilution").
	std::filesystem::path engineIni;
#ifdef AURORA_OS
	engineIni = std::filesystem::path(devilution::AuroraOsStandartPaths::GetWritableDataPath()) / "diablo.ini";
#else
	char *enginePref = SDL_GetPrefPath("diasurgical", "devilution");
	engineIni = (enginePref != nullptr ? std::filesystem::path(enginePref) : baseDir) / "diablo.ini";
	SDL_free(enginePref);
#endif

	m_services = launcher::MakeRealServices(baseDir, std::move(engineIni));
}

Application::Application(SDL_Window *window, launcher::ServiceBundle services)
    : m_window(window)
    , m_services(std::move(services))
{
}

Application::~Application()
{
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	if (m_backgroundTexture != nullptr) {
		SDL_DestroyTexture(m_backgroundTexture);
	}
	for (SDL_Texture *texture : m_heroTextures) {
		if (texture != nullptr) {
			SDL_DestroyTexture(texture);
		}
	}
	for (const auto &levels : m_iconTextures) {
		for (SDL_Texture *texture : levels) {
			if (texture != nullptr) {
				SDL_DestroyTexture(texture);
			}
		}
	}
	if (m_renderer != nullptr) {
		SDL_DestroyRenderer(m_renderer);
	}
}

bool Application::Setup()
{
	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (m_renderer == nullptr) {
		spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
		return false;
	}

	SDL_RendererInfo info;
	SDL_GetRendererInfo(m_renderer, &info);
	spdlog::info("Launcher SDL_Renderer: {}", info.name);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
	io.IniFilename = nullptr; // no imgui.ini — the launcher layout is fixed

	launcher::ui::Theme::Init(DPIHandler::GetScale());

	ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
	ImGui_ImplSDLRenderer2_Init(m_renderer);

	LoadArtTextures();
	return true;
}

/// Грузит арт из вшитых ассетов: общий фон, hero-панели режимов и золотые
/// иконки плиток. Отсутствующий файл — не ошибка: экраны уходят в фолбэк
/// (кроп общего фона / FontAwesome-глиф).
void Application::LoadArtTextures()
{
	struct ModeAsset {
		ExitAction mode;
		const char *heroPath;
		const char *iconPath;
	};
	m_backgroundTexture = LoadAssetTexture(m_renderer, "assets/bg.jpg", m_backgroundSize);
	for (const ModeAsset &asset : std::initializer_list<ModeAsset> {
	         { ExitAction::LaunchDiablo, "assets/hero_diablo.jpg", "assets/icon_diablo.png" },
	         { ExitAction::LaunchHellfire, "assets/hero_hellfire.jpg", "assets/icon_hellfire.png" },
	         { ExitAction::LaunchDemo, "assets/hero_demo.jpg", "assets/icon_demo.png" },
	     }) {
		const size_t idx = static_cast<size_t>(asset.mode);
		m_heroTextures[idx] = LoadAssetTexture(m_renderer, asset.heroPath, m_heroSizes[idx]);
		m_iconLevelCounts[idx]
		    = BuildIconLevels(m_renderer, asset.iconPath, m_iconTextures[idx].data(), m_iconSizes[idx].data());
	}
}

void Application::AttachFileLog()
{
	// Дублируем лог в файл рядом с настройками: на устройстве системный
	// журнал читается только рутом, а файл доступен пользователю напрямую.
	try {
		std::error_code ec;
		std::filesystem::create_directories(m_services.paths->ConfigDir(), ec);
		const auto logPath = m_services.paths->ConfigDir() / "launcher.log";
		auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		    logPath.string(), 512 * 1024, 2);
		spdlog::default_logger()->sinks().push_back(std::move(fileSink));
		// По умолчанию spdlog не сбрасывает буфер ofstream — при убийстве
		// процесса композитором файл оставался пустым. Сбрасываем каждую
		// запись и дополнительно раз в секунду.
		spdlog::flush_on(spdlog::level::debug);
		spdlog::flush_every(std::chrono::seconds(1));
		spdlog::info("File log: {}", logPath.string());
	} catch (const std::exception &err) {
		spdlog::warn("File log unavailable: {}", err.what());
	}
}

AppResult Application::Run()
{
	AttachFileLog();

	m_store = std::make_unique<launcher::Store>(
	    *m_services.config, *m_services.files, *m_services.downloads, *m_services.paths,
	    *m_services.engineOptions);

	// Свёрнутый цикл спит в блокирующем SDL_WaitEvent: фоновые потоки
	// (прогресс/финиш загрузок zoe) будят его пользовательским событием.
	m_wakeEventType = SDL_RegisterEvents(1);
	m_store->SetWakeCallback([this]() {
		SDL_Event wake {};
		wake.type = m_wakeEventType;
		SDL_PushEvent(&wake);
	});
#ifdef AURORA_OS
	// ВРЕМЕННАЯ телеметрия «aurora-probe»: вдруг патченная SDL Авроры
	// шлёт системные события в начале жеста сворачивания.
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
	StartDisplayWatch();
	InitCoverWatch();
#endif

	// Setup() создаёт контекст ImGui и грузит шрифты, поэтому вид (и его
	// файловый браузер, которому нужен шрифтовый атлас) создаётся после него.
	if (m_renderer == nullptr && !Setup()) {
		AppResult result;
		result.success = false;
		return result;
	}
	m_view = std::make_unique<launcher::ui::LauncherView>(DPIHandler::GetScale());
	if (m_backgroundTexture != nullptr) {
		m_view->SetBackgroundTexture(m_backgroundTexture, m_backgroundSize);
	}
	for (size_t i = 0; i < std::size(m_heroTextures); ++i) {
		if (m_heroTextures[i] != nullptr) {
			m_view->SetHeroTexture(static_cast<launcher::ExitAction>(i), m_heroTextures[i], m_heroSizes[i]);
		}
		if (m_iconLevelCounts[i] > 0) {
			launcher::ui::widgets::BackgroundArt levels[3];
			for (int l = 0; l < m_iconLevelCounts[i]; ++l) {
				levels[l] = launcher::ui::widgets::BackgroundArt { m_iconTextures[i][l], m_iconSizes[i][l] };
			}
			m_view->SetModeIconLevels(static_cast<launcher::ExitAction>(i), levels, m_iconLevelCounts[i]);
		}
	}

	m_store->Init();
	if (m_initialScreen.has_value()) {
		m_store->Dispatch(launcher::intent::UiNavigate { *m_initialScreen });
	}
	if (m_initialBrowser) {
		m_store->Dispatch(launcher::intent::SelectDataFolder {});
	}
	if (m_initialDialog.has_value()) {
		m_store->Dispatch(launcher::intent::UiOpenDialog { *m_initialDialog });
	}
	if (m_initialDownload) {
		m_store->Dispatch(launcher::intent::StartDownload { launcher::KnownFile::Spawn });
	}

	auto dispatch = [this](launcher::Intent intent) {
		m_store->Dispatch(std::move(intent));
	};

	auto processEvent = [this](const SDL_Event &event) {
		ImGui_ImplSDL2_ProcessEvent(&event);

#ifdef AURORA_OS
		// ВРЕМЕННАЯ телеметрия «aurora-probe»: редкие типы событий SDL —
		// ищем хоть один сигнал, приходящий в НАЧАЛЕ жеста сворачивания.
		switch (event.type) {
		case SDL_FINGERDOWN:
			spdlog::info("aurora-probe: FINGERDOWN");
			break;
		case SDL_FINGERUP:
			spdlog::info("aurora-probe: FINGERUP");
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_KEYDOWN:
		case SDL_TEXTEDITING:
		case SDL_TEXTINPUT:
			break;
		default:
			if (event.type == SDL_WINDOWEVENT || event.type == SDL_QUIT
			    || event.type == SDL_MOUSEMOTION || event.type == SDL_FINGERMOTION
			    || event.type == SDL_MOUSEWHEEL || event.type >= SDL_USEREVENT) {
				break;
			}
			spdlog::info("aurora-probe: event type={} (display={})",
			    event.type, event.type == SDL_DISPLAYEVENT ? static_cast<int>(event.display.event) : -1);
			break;
		}
#endif

		if (event.type == m_wakeEventType) {
			// Фоновый поток положил интент в Store — в свёрнутом состоянии
			// это значит, что обложку (прогресс в плитке) надо перерисовать.
			m_coverDirty = true;
		}
#ifdef AURORA_OS
		if (event.type == m_displayEventType) {
			// Сменилось состояние дисплея/блокировки/верхнего окна: кадр
			// надо переоценить — на вернувшийся экран плитка показывает
			// последний буфер.
			m_coverDirty = true;
			const bool wasAwake = m_displayOn && !m_tkLocked;
			switch (event.user.code) {
			case 0:
				m_displayOn = event.user.data1 != nullptr;
				break;
			case 1:
				m_tkLocked = event.user.data1 != nullptr;
				break;
			case 2:
				// Верхнее окно композитора — авторитетный источник: реагируем
				// мгновенно, без дебаунса (он нужен только SDL-фокусу).
				if (event.user.data1 != nullptr) {
					m_topmostLost = false;
					m_focusLostAt.reset();
				} else {
					m_topmostLost = true;
					if (!m_focusLostAt.has_value()) {
						m_focusLostAt = std::chrono::steady_clock::now();
					}
				}
				break;
			default:
				break;
			}
			// Пробуждение: пару секунд считаем себя передним планом и
			// рендерим интерфейс — между «экран разблокирован» и «окно
			// поднято» идёт анимация локскрина, и обложка в этом зазоре
			// мелькает. Перед сном были плиткой — грейс не нужен: после
			// разблокировки сразу остаёмся обложкой.
			const bool awake = m_displayOn && !m_tkLocked;
			if (wasAwake != awake) {
				if (awake) {
					if (!m_skipWakeGrace) {
						m_wakeGraceUntil =
						    std::chrono::steady_clock::now() + std::chrono::milliseconds(1500);
					}
				} else {
					m_skipWakeGrace = m_wasHidden;
				}
			}
		}
#endif
		if (event.type == SDL_QUIT) {
			Stop();
		}
		if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(m_window)) {
			OnEvent(event.window);
		}
	};

	m_running = true;
	// После «Играть» цикл дорисовывает iris-анимацию (сужающийся круг
	// поверх последнего кадра) и только затем отдаёт управление движку.
	while (m_running
	    && (!m_store->State().pendingLaunch.has_value() || !m_view->LaunchIrisDone())) {
		const auto frameStart = std::chrono::steady_clock::now();
		// В фоне (свёрнуто в плитку/скрыто) интерфейс не рендерится: рисуем
		// один кадр «обложки» для плитки композитора и спим в блокирующем
		// SDL_WaitEvent — никакого опроса по таймеру. Будят только события
		// окна (разворачивание) и wake-пинки фоновых интентов (прогресс
		// загрузок), по которым обложка перерисовывается с новым процентом.
#ifdef AURORA_OS
		// ВРЕМЕННАЯ телеметрия «aurora-probe»: переходы в/из режима плитки.
		if (IsTiled() != m_wasTiledProbe) {
			m_wasTiledProbe = !m_wasTiledProbe;
			spdlog::info("aurora-probe: {} (display={} tklock={} focusLostMs={})",
			    m_wasTiledProbe ? "TILED" : "VISIBLE", m_displayOn ? 1 : 0, m_tkLocked ? 1 : 0,
			    m_focusLostAt.has_value()
			        ? std::chrono::duration_cast<std::chrono::milliseconds>(
			              std::chrono::steady_clock::now() - *m_focusLostAt)
			              .count()
			         : -1);
		}
#endif
		if (IsTiled()) {
			// Анимировать закрытое окно не для кого: iris не стартует без
			// рендера, и цикл выше никогда не увидел бы его завершения.
			if (m_store->State().pendingLaunch.has_value()) {
				break;
			}
			// Погашенный/заблокированный экран — не рисуем вовсе; обложку
			// рисуем только на включённый разблокированный экран
			// (плитка/переключатель задач).
			if (m_displayOn && !m_tkLocked && (!m_wasHidden || m_coverDirty)) {
				RenderCoverFrame();
				m_coverDirty = false;
			}
			m_wasHidden = true;
			SDL_Event wait {};
			if (SDL_WaitEvent(&wait) == 1) {
				processEvent(wait);
			}
		} else {
			m_wasHidden = false;
		}

		SDL_Event event {};
		while (SDL_PollEvent(&event) == 1) {
			processEvent(event);
		}

		m_store->Poll();

		// Обработанные события могли развернуть окно — видим ли мы ещё?
		if (IsTiled()) {
			continue;
		}

		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		if (m_coverPreview) {
			m_view->RenderCover(m_store->State());
		} else {
			m_view->Render(m_store->State(), dispatch);
		}

		ImGui::Render();

		SDL_SetRenderDrawColor(m_renderer, 10, 7, 5, 255);
		SDL_RenderClear(m_renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
		SDL_RenderPresent(m_renderer);

		// Кадровый темп ~60 fps: без паузы цикл крутится вхолостую на
		// 100% CPU — на устройстве governor сбрасывает частоту от нагрева,
		// и кадры начинают опаздывать мимо развёртки (наблюдали 45 fps
		// при простое). Сон отдаляет следующий кадр не раньше 16 мс;
		// vsync в SDL_Renderer при этом остаётся включённым.
		const auto deadline = frameStart + std::chrono::milliseconds(16);
		if (const auto remaining = deadline - std::chrono::steady_clock::now();
		    remaining.count() > 0) {
			SDL_Delay(static_cast<Uint32>(
			    std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count()));
		}
	}

#ifdef AURORA_OS
	StopDisplayWatch();
	StopCoverWatch();
#endif

	AppResult result;
	if (const auto launch = m_store->State().pendingLaunch) {
		result.success = true;
		result.action = *launch;
		result.dataPath = m_store->State().dataFolder;
	}
	return result;
}

void Application::Stop()
{
	m_running = false;
}

void Application::RenderCoverFrame()
{
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	m_view->RenderCover(m_store->State());

	ImGui::Render();

	SDL_SetRenderDrawColor(m_renderer, 10, 7, 5, 255);
	SDL_RenderClear(m_renderer);
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
	SDL_RenderPresent(m_renderer);
}

#ifdef AURORA_OS

void Application::StartDisplayWatch()
{
	// Состоянием экрана на Авроре ведают два источника в системной шине
	// D-Bus: демон mce (наследие Sailfish; дисплей и блокировка) и сам
	// композитор Lipstick (верхнее окно). События SDL не различают
	// блокировку и сворачивание в плитку (в обоих случаях лишь
	// FOCUS_LOST/GAINED), а сигнал композитора приходит раньше SDL-фокуса
	// и покрывает случай «жест сворачивания ещё держат».
	m_displayEventType = SDL_RegisterEvents(1);
	m_displayWatch = std::thread([this]() { DisplayWatchLoop(); });
}

void Application::StopDisplayWatch()
{
	if (m_displayWatch.joinable()) {
		m_displayWatchStop.store(true);
		m_displayWatch.join();
	}
}

void Application::PushStateEvent(int what, bool value)
{
	// ВРЕМЕННАЯ телеметрия «aurora-probe»: логируем приход каждого
	// сигнала — на устройстве сверим тайминги с SDL-фокусом. Убрать
	// после отладки жеста/блокировки.
	static const char *const kNames[] = { "display", "tklock", "topmost" };
	spdlog::info("aurora-probe: sig {} = {}", kNames[what], value ? 1 : 0);

	SDL_Event event {};
	event.type = m_displayEventType;
	event.user.code = what;
	event.user.data1 = value ? reinterpret_cast<void *>(1) : nullptr;
	SDL_PushEvent(&event);
}

void Application::DisplayWatchLoop()
{
	dbus_threads_init_default();

	DBusError error;
	dbus_error_init(&error);
	DBusConnection *bus = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if (bus == nullptr) {
		spdlog::warn("aurora-watch: системная шина недоступна ({}), работаем по SDL-фокусу",
		    error.message != nullptr ? error.message : "?");
		dbus_error_free(&error);
		return;
	}

	// Дисплей + блокировка от mce, верхнее окно от Lipstick.
	dbus_bus_add_match(bus, "type='signal',sender='com.nokia.mce',interface='com.nokia.mce.signal'", &error);
	const bool mceOk = !dbus_error_is_set(&error);
	if (!mceOk) {
		spdlog::warn("aurora-watch: подписка на mce не удалась ({})", error.message);
		dbus_error_free(&error);
	}
	dbus_bus_add_match(bus,
	    "type='signal',interface='org.nemomobile.compositor',member='privateTopmostWindowProcessIdChanged'",
	    &error);
	const bool compositorOk = !dbus_error_is_set(&error);
	if (!compositorOk) {
		spdlog::warn("aurora-watch: подписка на композитор не удалась ({})", error.message);
		dbus_error_free(&error);
	}
	if (!mceOk && !compositorOk) {
		dbus_connection_unref(bus);
		return;
	}

	// Начальные состояния — синхронными запросами, чтобы не ждать первых
	// переключений (лаунчер могут запустить уже заблокированным).
	const auto queryMceString = [&bus](const char *method) -> std::string {
		DBusMessage *call = dbus_message_new_method_call(
		    "com.nokia.mce", "/com/nokia/mce/request", "com.nokia.mce.request", method);
		if (call == nullptr) {
			return {};
		}
		DBusError queryError;
		dbus_error_init(&queryError);
		DBusMessage *reply = dbus_connection_send_with_reply_and_block(bus, call, 1000, &queryError);
		dbus_message_unref(call);
		std::string status;
		if (reply != nullptr) {
			const char *value = nullptr;
			if (dbus_message_get_args(reply, &queryError, DBUS_TYPE_STRING, &value, DBUS_TYPE_INVALID)
			    && value != nullptr) {
				status = value;
			}
			dbus_message_unref(reply);
		}
		dbus_error_free(&queryError);
		return status;
	};
	if (mceOk) {
		const std::string display = queryMceString("get_display_status");
		if (!display.empty()) {
			spdlog::info("aurora-watch: дисплей '{}'", display);
			PushStateEvent(0, display != "off");
		}
		const std::string lock = queryMceString("get_tklock_mode");
		if (!lock.empty()) {
			spdlog::info("aurora-watch: tklock '{}'", lock);
			PushStateEvent(1, lock == "locked");
		}
	}
	const int32_t ourPid = static_cast<int32_t>(::getpid());

	// Блокирующее чтение с таймаутом: просыпаемся четыре раза в секунду
	// только чтобы проверить флаг завершения — дешевле интеграции шины
	// в цикл событий.
	while (!m_displayWatchStop.load()) {
		if (!dbus_connection_read_write(bus, 250)) {
			spdlog::warn("aurora-watch: соединение с шиной потеряно");
			break;
		}
		while (DBusMessage *message = dbus_connection_pop_message(bus)) {
			DBusError parse;
			dbus_error_init(&parse);
			const char *status = nullptr;
			if (dbus_message_is_signal(message, "com.nokia.mce.signal", "display_status_ind")
			    && dbus_message_get_args(message, &parse, DBUS_TYPE_STRING, &status, DBUS_TYPE_INVALID)
			    && status != nullptr) {
				// «dimmed» и прочие промежуточные состояния считаем
				// включённым экраном: рисовать ещё есть для кого.
				PushStateEvent(0, std::strcmp(status, "off") != 0);
			} else if (dbus_message_is_signal(message, "com.nokia.mce.signal", "tklock_mode_ind")
			    && dbus_message_get_args(message, &parse, DBUS_TYPE_STRING, &status, DBUS_TYPE_INVALID)
			    && status != nullptr) {
				PushStateEvent(1, std::strcmp(status, "locked") == 0);
			} else if (dbus_message_is_signal(message, "org.nemomobile.compositor",
			               "privateTopmostWindowProcessIdChanged")) {
				int32_t pid = 0;
				if (dbus_message_get_args(message, &parse, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID)) {
					PushStateEvent(2, pid == ourPid);
				}
			}
			dbus_error_free(&parse);
			dbus_message_unref(message);
		}
	}
	dbus_connection_unref(bus);
}

// ---------------------------------------------------------------------------
// Wayland-хук «плитки»: приватное расширение Qt (qt_surface_extension из
// QtWayland, поддерживается Lipstick). Композитор сообщает окну состояние
// обложки свойством cover_status через qt_extended_surface — в отличие от
// D-Bus/SDL-событий это происходит в НАЧАЛЕ жеста сворачивания. Биндинги
// протокола собраны руками (в SDK Авроры сгенерированных заголовков нет,
// интерфейсы тривиальны: один запрос у расширения, три события у поверхности).
// ---------------------------------------------------------------------------

namespace {

const wl_message kQtSurfaceExtensionRequests[] = {
	{ "get_extended_surface", "no", nullptr },
};
const wl_message kQtExtendedSurfaceRequests[] = {
	{ "destroy", "", nullptr },
};
const wl_message kQtExtendedSurfaceEvents[] = {
	{ "onscreen_visibility", "i", nullptr },
	{ "set_generic_property", "sa", nullptr },
	{ "close", "", nullptr },
};

const wl_interface kQtSurfaceExtensionInterface = {
	"qt_surface_extension", 1,
	1, kQtSurfaceExtensionRequests,
	0, nullptr,
};
const wl_interface kQtExtendedSurfaceInterface = {
	"qt_extended_surface", 1,
	1, kQtExtendedSurfaceRequests,
	3, kQtExtendedSurfaceEvents,
};

struct qt_extended_surface_listener {
	void (*onscreen_visibility)(void *data, qt_extended_surface *surface, int32_t visible);
	void (*set_generic_property)(void *data, qt_extended_surface *surface, const char *name, wl_array *value);
	void (*close)(void *data, qt_extended_surface *surface);
};

/// Заполняется слушателем реестра (глобалы приходят до roundtrip).
qt_surface_extension *g_coverExtensionBound = nullptr;

qt_extended_surface *QtGetExtendedSurface(qt_surface_extension *extension, wl_surface *surface)
{
	return static_cast<qt_extended_surface *>(wl_proxy_marshal_constructor(reinterpret_cast<wl_proxy *>(extension),
	    0, &kQtExtendedSurfaceInterface, nullptr, surface));
}

void QtDestroyExtendedSurface(qt_extended_surface *extended)
{
	wl_proxy_marshal(reinterpret_cast<wl_proxy *>(extended), 0);
	wl_proxy_destroy(reinterpret_cast<wl_proxy *>(extended));
}

void CoverOnscreenVisibility(void *, qt_extended_surface *, int32_t visible)
{
	// ВРЕМЕННАЯ телеметрия «cover-probe» — убрать после девайс-прогона.
	spdlog::info("cover-probe: onscreen_visibility={}", visible);
}

void CoverSetGenericProperty(void *data, qt_extended_surface *, const char *name, wl_array *value)
{
	static_cast<Application *>(data)->OnCoverProperty(name, value);
}

void CoverClose(void *, qt_extended_surface *)
{
	spdlog::info("cover-probe: close от композитора");
}

qt_extended_surface_listener kCoverExtendedListener = {
	CoverOnscreenVisibility,
	CoverSetGenericProperty,
	CoverClose,
};

void CoverRegistryGlobal(void *, wl_registry *registry, uint32_t name, const char *interface, uint32_t)
{
	if (std::strcmp(interface, "qt_surface_extension") == 0) {
		g_coverExtensionBound = static_cast<qt_surface_extension *>(
		    wl_registry_bind(registry, name, &kQtSurfaceExtensionInterface, 1u));
	}
}
void CoverRegistryGlobalRemove(void *, wl_registry *, uint32_t)
{
}

const wl_registry_listener kCoverRegistryListener = {
	CoverRegistryGlobal,
	CoverRegistryGlobalRemove,
};

} // namespace

void Application::InitCoverWatch()
{
	SDL_SysWMinfo wm;
	SDL_VERSION(&wm.version);
	if (!SDL_GetWindowWMInfo(m_window, &wm) || wm.subsystem != SDL_SYSWM_WAYLAND) {
		spdlog::info("cover-watch: окно не wayland — хук плитки пропущен");
		return;
	}
	wl_display *display = wm.info.wl.display;
	wl_surface *surface = wm.info.wl.surface;
	if (display == nullptr || surface == nullptr) {
		spdlog::warn("cover-watch: нет wayland display/surface");
		return;
	}

	g_coverExtensionBound = nullptr;
	m_coverRegistry = wl_display_get_registry(display);
	wl_registry_add_listener(m_coverRegistry, &kCoverRegistryListener, nullptr);
	wl_display_roundtrip(display);
	if (g_coverExtensionBound == nullptr) {
		spdlog::info("cover-watch: композитор не экспортирует qt_surface_extension");
		wl_registry_destroy(m_coverRegistry);
		m_coverRegistry = nullptr;
		return;
	}
	m_coverExtension = g_coverExtensionBound;

	m_coverSurface = QtGetExtendedSurface(m_coverExtension, surface);
	wl_proxy_add_listener(reinterpret_cast<wl_proxy *>(m_coverSurface),
	    reinterpret_cast<wl_notify_func_t *>(&kCoverExtendedListener), this);
	wl_display_roundtrip(display);
	spdlog::info("cover-watch: подписан на события qt_extended_surface");
}

void Application::StopCoverWatch()
{
	if (m_coverSurface != nullptr) {
		QtDestroyExtendedSurface(m_coverSurface);
		m_coverSurface = nullptr;
	}
	if (m_coverRegistry != nullptr) {
		wl_registry_destroy(m_coverRegistry);
		m_coverRegistry = nullptr;
	}
}

void Application::OnCoverProperty(const char *name, const wl_array *value)
{
	// ВРЕМЕННАЯ телеметрия «cover-probe»: сырые байты значения — формат
	// свойства документирован плохо, сверим на устройстве. Убрать.
	std::string hex;
	const auto *bytes = static_cast<const unsigned char *>(value->data);
	for (size_t i = 0; i < value->size && i < 16; ++i) {
		char buf[4];
		std::snprintf(buf, sizeof(buf), "%02x ", bytes[i]);
		hex += buf;
	}
	spdlog::info("cover-probe: property '{}' = [{}] ({} байт)", name, hex, value->size);

	if (std::strcmp(name, "cover_status") != 0 && std::strcmp(name, "jolla.cover_status") != 0) {
		return;
	}
	int32_t status = 0;
	if (value->size >= sizeof(status)) {
		std::memcpy(&status, value->data, sizeof(status));
	}
	m_coverActive = status != 0;
	if (m_coverActive) {
		if (!m_focusLostAt.has_value()) {
			m_focusLostAt = std::chrono::steady_clock::now();
		}
	} else {
		m_focusLostAt.reset();
		m_topmostLost = false;
	}
	m_coverDirty = true;
}

#endif

void Application::OnEvent(const SDL_WindowEvent &event)
{
#ifdef AURORA_OS
	if (event.event == SDL_WINDOWEVENT_FOCUS_LOST) {
		spdlog::info("aurora-probe: SDL FOCUS_LOST");
		m_focusLostAt = std::chrono::steady_clock::now();
	} else if (event.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
		spdlog::info("aurora-probe: SDL FOCUS_GAINED");
		m_focusLostAt.reset();
		m_topmostLost = false;
	}
    spdlog::info("aurora-probe: SDL WINDOW EVVENT: {}", event.event);
#endif

	if (event.event == SDL_WINDOWEVENT_CLOSE) {
		Stop();
	}
}

bool Application::IsTiled() const
{
#ifdef AURORA_OS
	// Погашенный или заблокированный экран: не рендерить ничего, даже если
	// окно формально в фокусе — кадры в тёмную матрицу тратят батарею, а
	// на экран блокировки обложке показываться незачем.
	if (!m_displayOn || m_tkLocked) {
		return true;
	}
	// Слово композитора через qt_extended_surface (cover_status): жест
	// сворачивания ещё держат — уже показываем обложку, не ждём фокуса.
	if (m_coverActive) {
		return true;
	}
	// Грейс после пробуждения: рендерим интерфейс как передний план.
	// Просроченный грейс просто проваливается дальше (метод константный,
	// сбрасывать optional не нужно — следующий пробой его перепишет).
	if (m_wakeGraceUntil.has_value() && std::chrono::steady_clock::now() < *m_wakeGraceUntil) {
		return false;
	}
	if ((SDL_GetWindowFlags(m_window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) != 0) {
		return true;
	}
	// Аврора не шлёт MINIMIZED/HIDDEN при сворачивании в плитку, поэтому
	// «в плитке» = потеря переднего плана: композитор сказал — мгновенно,
	// SDL-фокус (фолбэк при мёртой шине) — с дебаунсом от мигания шторками.
	return m_topmostLost
	    || (m_focusLostAt.has_value()
	        && std::chrono::steady_clock::now() - *m_focusLostAt >= std::chrono::milliseconds(100));
#else
	return (SDL_GetWindowFlags(m_window) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) != 0;
#endif
}

} // namespace App

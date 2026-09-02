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
			case 3:
				// coverstatus от Lipstick: 1 и 2 прилетают ПАРОЙ в самом
				// начале жеста, 3 и 0 — парой при возврате из плитки.
				// Вход по ним не делаем (обложка включается по потере
				// верхнего окна, в момент отпускания пальца), а вот выход —
				// мгновенный, не ждём FOCUS_GAINED. «1» до «2» безвредна:
				// фокус ещё не потерян, сбрасывать нечего.
				if (event.user.data1 == nullptr) {
					m_focusLostAt.reset();
					m_topmostLost = false;
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
#ifdef AURORA_OS
		// Кросс-фейд на входе в плитку: первые kCoverFade секунд кадр —
		// интерфейс с обложкой поверх (непрозрачность обложки растёт от
		// нуля), чтобы переход не был резким скачком; затем обычный
		// режим плитки (одна обложка и сон). Разворачивание мгновенное,
		// без фейда.
		bool coverFading = false;
		float coverAlpha = 1.0F;
		if (IsTiled() && m_displayOn && !m_tkLocked
		    && !m_store->State().pendingLaunch.has_value()) {
			if (m_coverFadeStartedAt < 0.0) {
				m_coverFadeStartedAt = ImGui::GetTime();
			}
			constexpr float kCoverFade = 0.2F;
			const float fade = static_cast<float>(ImGui::GetTime() - m_coverFadeStartedAt);
			if (fade < kCoverFade) {
				coverFading = true;
				coverAlpha = fade / kCoverFade;
			} else {
				m_coverFadeStartedAt = -1.0;
			}
		} else {
			m_coverFadeStartedAt = -1.0;
		}
#else
		constexpr bool coverFading = false;
#endif
		if (IsTiled() && !coverFading) {
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
		if (IsTiled() && !coverFading) {
			continue;
		}

		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		if (m_coverPreview) {
			m_view->RenderCover(m_store->State());
		} else {
			m_view->Render(m_store->State(), dispatch);
#ifdef AURORA_OS
			// Кадр кросс-фейда: обложка поверх интерфейса (проверка IsTiled
			// свежая — событие выше могло успеть развернуть окно).
			if (coverFading && IsTiled()) {
				m_view->RenderCover(m_store->State(), coverAlpha);
			}
#endif
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

	// Дисплей + блокировка от mce, верхнее окно от композитора — системная
	// шина. А вот coverstatus (начало жеста сворачивания, до потери
	// фокуса) композитор вещает под именем com.jolla.lipstick на
	// СЕССИОННОЙ шине — на системной этого имени нет вовсе.
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

	// Сигнал без идентификатора окна: атрибуируем «жест наш», пока мы
	// в фокусе (жест бывает только на переднем плане).
	DBusConnection *session = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if (session == nullptr) {
		spdlog::warn("aurora-watch: сессионная шина недоступна ({}), жесты не увидим",
		    error.message != nullptr ? error.message : "?");
		dbus_error_free(&error);
	} else {
		dbus_bus_add_match(session, "type='signal',interface='com.jolla.lipstick',member='coverstatus'", &error);
		if (dbus_error_is_set(&error)) {
			spdlog::warn("aurora-watch: подписка на coverstatus не удалась ({})", error.message);
			dbus_error_free(&error);
			dbus_connection_unref(session);
			session = nullptr;
		}
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

	// Диспетчеризация одного сообщения: интерфейс определяет источник,
	// шина значения не имеет (coverstatus ходит по сессии, остальное —
	// по системной).
	const auto drainBus = [this, ourPid](DBusConnection *connection) {
		while (DBusMessage *message = dbus_connection_pop_message(connection)) {
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
			} else if (dbus_message_is_signal(message, "com.jolla.lipstick", "coverstatus")) {
				int32_t cover = 0;
				if (dbus_message_get_args(message, &parse, DBUS_TYPE_INT32, &cover, DBUS_TYPE_INVALID)) {
					spdlog::info("aurora-probe: coverstatus = {}", cover);
					// 1 — жест начался, окно ещё полноэкранное; 2 — драг
					// дошёл до порога и окно сжалось в плитку (порог — на
					// совести композитора, у нас данных о пальце нет);
					// 3 — возврат из плитки; 0 — обычный режим.
					PushStateEvent(3, cover == 2);
				}
			}
			dbus_error_free(&parse);
			dbus_message_unref(message);
		}
	};

	// Блокирующее чтение с таймаутом по каждой из шин по очереди:
	// просыпаемся ~7 раз в секунду только чтобы проверить флаг
	// завершения — дешевле интеграции шин в цикл событий.
	while (!m_displayWatchStop.load()) {
		if (!dbus_connection_read_write(bus, 70)) {
			spdlog::warn("aurora-watch: системная шина потеряна");
			break;
		}
		drainBus(bus);
		if (session != nullptr) {
			if (!dbus_connection_read_write(session, 70)) {
				spdlog::warn("aurora-watch: сессионная шина потеряна");
				dbus_connection_unref(session);
				session = nullptr;
				continue;
			}
			drainBus(session);
		}
	}
	dbus_connection_unref(bus);
	if (session != nullptr) {
		dbus_connection_unref(session);
	}
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

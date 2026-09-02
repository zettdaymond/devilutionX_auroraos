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
#include <filesystem>
#include <iterator>
#include <utility>
#include <vector>

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

	auto dispatch = [this](launcher::Intent intent) {
		m_store->Dispatch(std::move(intent));
	};

	auto processEvent = [this](const SDL_Event &event) {
		ImGui_ImplSDL2_ProcessEvent(&event);

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
		// В фоне (свёрнуто/скрыто) цикл продолжает обслуживать события и
		// загрузки, но не рендерит. Вместо слепого сна ждём событие в ОС:
		// разворачивание обрабатывается мгновенно, а таймаут 250 мс равен
		// периоду троттлинга прогресса загрузок — просыпаемся ровно в такт
		// прибытию интентов из фоновой очереди (они приходят не как
		// SDL-события, поэтому бесконечное ожидание недопустимо).
		const Uint32 windowFlags = SDL_GetWindowFlags(m_window);
		const bool hidden = (windowFlags & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) != 0;

		if (hidden) {
			// Анимировать закрытое окно не для кого: iris не стартует без
			// рендера, и цикл выше никогда не увидел бы его завершения.
			if (m_store->State().pendingLaunch.has_value()) {
				break;
			}
			SDL_Event wake {};
			if (SDL_WaitEventTimeout(&wake, 250) == 1) {
				processEvent(wake);
			}
		}

		SDL_Event event {};
		while (SDL_PollEvent(&event) == 1) {
			processEvent(event);
		}

		m_store->Poll();
		if (hidden) {
			continue;
		}

		const auto renderStart = std::chrono::steady_clock::now();

		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		m_view->Render(m_store->State(), dispatch);

		ImGui::Render();

		SDL_SetRenderDrawColor(m_renderer, 10, 7, 5, 255);
		SDL_RenderClear(m_renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
		SDL_RenderPresent(m_renderer);
		const auto presentDone = std::chrono::steady_clock::now();

		// TODO(кинетика): временная телеметрия секций кадра — убрать
		// после локализации просадки на устройстве.
		{
			const auto pollMs = std::chrono::duration<double, std::milli>(renderStart - frameStart).count();
			const auto renderMs = std::chrono::duration<double, std::milli>(presentDone - renderStart).count();
			static double pollAcc = 0.0;
			static double renderAcc = 0.0;
			static int frames = 0;
			pollAcc += pollMs;
			renderAcc += renderMs;
			++frames;
			if (frames >= 60) {
				spdlog::info("frame: poll={:.2f}ms render+present={:.2f}ms fps={}",
				    pollAcc / frames, renderAcc / frames, static_cast<int>(ImGui::GetIO().Framerate));
				pollAcc = renderAcc = 0.0;
				frames = 0;
			}
		}

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

void Application::OnEvent(const SDL_WindowEvent &event)
{
	if (event.event == SDL_WINDOWEVENT_CLOSE) {
		Stop();
	}
}

} // namespace App

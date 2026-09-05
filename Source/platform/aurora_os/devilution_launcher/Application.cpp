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

#include <cstdlib>

#ifdef AURORA_OS
#   include "../NativeCover.hpp"
#   include "../StandartPaths.hpp"
#endif

#include <chrono>
#include <filesystem>
#include <iterator>
#include <utility>
#include <vector>

#ifdef AURORA_OS
#	include "AuroraStateWatch.hpp"
#	include "GameCover.hpp"
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
	// базовой папкой лаунчера: на Aurora движок ходит через собственный
	// SDL_GetPrefPath("org.diasurgical", "devilutionx"), на десктопе —
	// SDL_GetPrefPath("diasurgical", "devilution").
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

		// SDL-логи движка (Log(), SDL_LogInfo платформы) при иконочном
		// запуске иначе теряются вместе с stderr — мостим в spdlog.
		// Фаза движка работает в том же процессе, мост действует и на неё.
		SDL_LogSetOutputFunction(
		    [](void *, int category, SDL_LogPriority priority, const char *message) {
			    const spdlog::level::level_enum level = priority >= SDL_LOG_PRIORITY_ERROR ? spdlog::level::err
			        : priority == SDL_LOG_PRIORITY_WARN                    ? spdlog::level::warn
			                                                                    : spdlog::level::info;
			    spdlog::log(level, "[sdl/cat{}] {}", category, message != nullptr ? message : "");
		    },
		    nullptr);
	} catch (const std::exception &err) {
		spdlog::warn("File log unavailable: {}", err.what());
	}
}

AppResult Application::Run()
{
	AttachFileLog();

	// Данные экрана для настройки «Разрешение»: аспект — сервису (Width
	// по Height), перечисленные режимы — зеркалу игрового списка в Store.
	std::vector<int> displayHeights;
	SDL_DisplayMode mode;
	if (SDL_GetDesktopDisplayMode(0, &mode) == 0) {
		if (mode.w < mode.h) {
			std::swap(mode.w, mode.h);
		}
		if (m_services.engineOptions != nullptr) {
			m_services.engineOptions->SetResolutionAspect(mode.w, mode.h);
		}
		launcher::ui::Scale::SetScreenSize(mode.w, mode.h);

		for (int i = 0; i < SDL_GetNumDisplayModes(0); ++i) {
			SDL_DisplayMode candidate;
			if (SDL_GetDisplayMode(0, i, &candidate) != 0) {
				continue;
			}
			if (candidate.w < candidate.h) {
				std::swap(candidate.w, candidate.h);
			}
			displayHeights.push_back(candidate.h);
		}
	}

	m_store = std::make_unique<launcher::Store>(
	    *m_services.config, *m_services.files, *m_services.downloads, *m_services.paths,
	    *m_services.engineOptions, std::move(displayHeights));

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
#ifdef AURORA_OS
	// Наблюдатель состояния Авроры: дисплей для WindowHidden(); события
	// из его потока дрена (смена дисплея) будят цикл обложки.
	m_stateWatch = std::make_unique<launcher::aurora::StateWatch>();
	// Обложка для фазы движка запекается офскрин уже здесь: к моменту
	// «Играть» пиксели готовы, из пути выхода рендер убран совсем.
	BakeGameCover();
	// POC нативной обложки Lipstick: то же изображение уезжает в окно
	// категории cover — плитку показывает композитор, не наш буфер.
	TryNativeCover();
	// Кардиограмма отладки: DEVILUTIONX_NATIVE_COVER_DEBUG=1 — каждые
	// 500 мс перезаливать кадр обложки (красным), чтобы видеть, живут ли
	// коммиты окна обложки (в плитке или где-либо ещё).
	if (m_nativeCoverActive) {
		const char *debugEnv = SDL_getenv("DEVILUTIONX_NATIVE_COVER_DEBUG");
		if (debugEnv != nullptr && debugEnv[0] == '1') {
			const Uint32 heartbeat = SDL_RegisterEvents(1);
			m_nativeCoverHeartbeat = heartbeat;
			SDL_AddTimer(500, [](Uint32, void *userdata) -> Uint32 {
				const auto type = *static_cast<Uint32 *>(userdata);
				SDL_Event pulse {};
				pulse.type = type;
				SDL_PushEvent(&pulse);
				return 500;
			}, &m_nativeCoverHeartbeat);
		}
	}
#endif
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

	m_running = true;
	// После «Играть» цикл дорисовывает iris-анимацию (сужающийся круг
	// поверх последнего кадра) и только затем отдаёт управление движку.
	// Плитку свёрнутого окна ведёт нативная обложка Lipstick (TryNativeCover):
	// скрытое окно не рендерится вовсе — цикл уходит в блокирующее
	// ожидание (RunCoverLoop) до возврата видимости.
	while (m_running
	    && (!m_store->State().pendingLaunch.has_value() || !m_view->LaunchIrisDone())) {
		const auto frameStart = std::chrono::steady_clock::now();

		SDL_Event event {};
		while (SDL_PollEvent(&event) == 1) {
			ProcessEvent(event);
		}

		m_store->Poll();

		if (WindowHidden()) {
			RunCoverLoop();
			// Выход из приложения или запуск игры: iris дорисовывать
			// некому — сразу отдаём результат.
			if (!m_running || m_store->State().pendingLaunch.has_value()) {
				break;
			}
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

#ifndef AURORA_OS
		// Dev-дамп кадра ДО композитора (только десктоп-сборка): ReadPixels
		// из бэкбуфера рендерера отдаёт чистые пиксели (PrintWindow/
		// CopyFromScreen идут через DWM и ловят его пересборку кадра).
		// DEVILUTIONX_DUMP_FRAME=N задаёт номер кадра, DEVILUTIONX_DUMP_PATH —
		// куда писать BMP. В сборку для устройства код не попадает.
		static const int dumpFrameNo = [] {
			const char *env = SDL_getenv("DEVILUTIONX_DUMP_FRAME");
			return env != nullptr ? std::atoi(env) : -1;
		}();
		static int frameCounter = 0;
		if (dumpFrameNo == frameCounter) {
			int dumpW = 0;
			int dumpH = 0;
			SDL_GetRendererOutputSize(m_renderer, &dumpW, &dumpH);
			SDL_Surface *shot = SDL_CreateRGBSurfaceWithFormat(
			    0, dumpW, dumpH, 0, SDL_PIXELFORMAT_RGB24);
			if (shot != nullptr) {
				if (SDL_RenderReadPixels(m_renderer, nullptr, SDL_PIXELFORMAT_RGB24, shot->pixels, shot->pitch) == 0) {
					const char *dumpPath = SDL_getenv("DEVILUTIONX_DUMP_PATH");
					SDL_SaveBMP(shot, dumpPath != nullptr ? dumpPath : "devilutionx_frame.bmp");
				}
				SDL_FreeSurface(shot);
			}
		}
		++frameCounter;
#endif

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
	m_stateWatch.reset();
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

void Application::ProcessEvent(const SDL_Event &event)
{
	ImGui_ImplSDL2_ProcessEvent(&event);

#ifdef AURORA_OS
	if (m_nativeCoverHeartbeat != 0 && event.type == m_nativeCoverHeartbeat) {
		// Кардиограмма обложки: свежий кадр в её окно (в режиме
		// DEVILUTIONX_NATIVE_COVER_DEBUG=1 — красным).
		std::vector<unsigned char> pixels;
		int width = 0;
		int height = 0;
		if (launcher::aurora::GameCover::CopyBakedPixels(pixels, width, height)) {
			devilution::NativeCover::UpdateFrame(pixels.data(), width * 3, width, height);
		}
	}
#endif
	if (event.type == SDL_QUIT) {
		Stop();
	}
	if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(m_window)) {
		OnEvent(event.window);
	}
}

bool Application::WindowHidden() const
{
	if (m_window == nullptr) {
		return false;
	}
#ifdef AURORA_OS
	// Зашёлка от дедлока старта: FOCUS_LOST на Авроре приходит ДО первого
	// кадра, а фокус композитор даёт только показанному окну — до первого
	// FOCUS_GAINED обязаны рендерить, что бы ни говорили флаги.
	if (!m_focusKnown) {
		return false;
	}
	if (m_stateWatch != nullptr && !m_stateWatch->DisplayOn()) {
		return true;
	}
#endif
	const Uint32 flags = SDL_GetWindowFlags(m_window);
	return (flags & SDL_WINDOW_INPUT_FOCUS) == 0
	    || (flags & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) != 0;
}

void Application::RunCoverLoop()
{
	// Плитку ведёт нативная обложка Lipstick, интерфейс в свёрнутом окне
	// не меняется — рендерить не для кого. Спим в блокирующем SDL_WaitEvent:
	// будит любое событие (возврат фокуса/закрытие окна, кардиограмма
	// отладки, события наблюдателя дисплея), а wake-интенты стора
	// применяются сразу, чтобы состояние не старело. Выход: окно снова
	// видимо, приложение закрывается или запускается игра (iris в скрытом
	// окне анимировать некому).
	while (m_running
	    && !m_store->State().pendingLaunch.has_value()
	    && WindowHidden()) {
		SDL_Event wait {};
		if (SDL_WaitEvent(&wait) == 1) {
			ProcessEvent(wait);
			m_store->Poll();
		} else {
			SDL_Delay(100);
		}
	}
}

#ifdef AURORA_OS

void Application::BakeGameCover()
{
	// Кадр тот же, что рисует плитка лаунчера (RenderCover), но рендерим
	// его офскрин — в текстуру-таргет: кадр не касается буферов окна
	// (прежняя запечка на выходе.present'илась в окно, и обложка мигала
	// перед стартом игры). Пиксели снимаются сразу и переживают смерть
	// окна лаунчера.
	int width = 0;
	int height = 0;
	if (SDL_GetRendererOutputSize(m_renderer, &width, &height) != 0 || width <= 0 || height <= 0) {
		return;
	}
	SDL_Texture *target = SDL_CreateTexture(
	    m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
	if (target == nullptr) {
		spdlog::warn("aurora: SDL_CreateTexture(таргет запечки) не удалась: {}", SDL_GetError());
		return;
	}
	SDL_Texture *previousTarget = SDL_GetRenderTarget(m_renderer);
	SDL_SetRenderTarget(m_renderer, target);

	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	// Вьюха обновляет масштаб только в живом кадре UI; без этого
	// офскрин-кадр запечки брал дефолтный MinSide=540 и обложка игры
	// запекалась со старыми мелкими шрифтами (плитка лаунчера при этом
	// рисовала уже увеличенные).
	launcher::ui::Scale::BeginFrame(DPIHandler::GetScale());

	m_view->RenderCover(m_store->State());

	ImGui::Render();

	SDL_SetRenderDrawColor(m_renderer, 10, 7, 5, 255);
	SDL_RenderClear(m_renderer);
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
	// ReadPixels читает текущий таргет — снимаем офскрин-кадр.
	launcher::aurora::GameCover::CaptureFromBackbuffer(m_renderer);

	SDL_SetRenderTarget(m_renderer, previousTarget);
	SDL_DestroyTexture(target);
}

void Application::TryNativeCover()
{
	// Гейт для A/B-проверки на устройстве: DEVILUTIONX_NATIVE_COVER=0 —
	// только старая схема (запечённый кадр в буфере главного окна).
	const char *disabled = SDL_getenv("DEVILUTIONX_NATIVE_COVER");
	if (disabled != nullptr && disabled[0] == '0') {
		spdlog::info("aurora-native-cover: выключен (DEVILUTIONX_NATIVE_COVER=0)");
		return;
	}
	std::vector<unsigned char> pixels;
	int width = 0;
	int height = 0;
	if (!launcher::aurora::GameCover::CopyBakedPixels(pixels, width, height)) {
		spdlog::info("aurora-native-cover: запечённых пикселей нет — пропускаем");
		return;
	}
	m_nativeCoverActive = devilution::NativeCover::CreateAndLink(m_window, width, height, pixels.data(), width * 3);
	if (!m_nativeCoverActive) {
		spdlog::info("aurora-native-cover: композитор не поддержал, работает старая схема");
	}
}

#endif

void Application::OnEvent(const SDL_WindowEvent &event)
{
	// Зашёлка «окно уже показывалось»: только FOCUS_GAINED считается
	// доказательством (FOCUS_LOST приходит и до первого кадра).
	if (event.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
		m_focusKnown = true;
	}
	if (event.event == SDL_WINDOWEVENT_CLOSE) {
		Stop();
	}
}

} // namespace App

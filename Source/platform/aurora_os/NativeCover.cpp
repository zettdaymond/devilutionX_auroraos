// Компилируется только в сборке под Aurora OS (см. CMakeLists).

#ifdef AURORA_OS

#include "NativeCover.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>

namespace devilution {

// Патч SDL-форка (3rdParty/SDL2/sdl-wayland-generic-property.patch):
// ставит generic-свойство через qt_extended_surface, созданный самим SDL.
extern "C" void SDL_WaylandSetWindowGenericProperty(
    SDL_Window *window, const char *name, const void *value, size_t length);

namespace {

// ---------------------------------------------------------------------------
// Протокол qt_surface_extension (wayland-protocols-plasma/surface-extension,
// v1). Генерировать wayland-scanner'ом ради трёх запросов не хочется —
// интерфейсы описаны вручную; сигнатуры обязаны совпадать с XML, иначе
// композитор порвёт соединение (события демаршализуются по нашим же
// сигнатурам даже без слушателя). Core-протоколы (compositor/shm/shell)
// берём из wayland-client-protocol.h — там всё сгенерировано.
// ---------------------------------------------------------------------------

extern const wl_interface qt_extended_surface_interface_impl;

// wl_message::types — const wl_interface** (массив мутабелен, как в
// коде wayland-scanner).
const wl_interface *kGetExtendedSurfaceTypes[2] = {
	&qt_extended_surface_interface_impl,
	&wl_surface_interface,
};

const wl_message kSurfaceExtensionRequests[1] = {
	{ "get_extended_surface", "no", kGetExtendedSurfaceTypes },
};

const wl_interface kSurfaceExtensionInterface = {
	"qt_surface_extension", 1, 1, kSurfaceExtensionRequests, 0, nullptr,
};

const wl_message kExtendedSurfaceEvents[3] = {
	{ "onscreen_visibility", "i", nullptr }, // 0
	{ "set_generic_property", "sa", nullptr }, // 1
	{ "close", "", nullptr },                  // 2
};

const wl_message kExtendedSurfaceRequests[5] = {
	{ "update_generic_property", "sa", nullptr },     // 0
	{ "set_content_orientation_mask", "i", nullptr }, // 1
	{ "set_window_flags", "i", nullptr },             // 2
	{ "raise", "", nullptr },                          // 3
	{ "lower", "", nullptr },                          // 4
};

const wl_interface qt_extended_surface_interface_impl = {
	"qt_extended_surface", 1, 5, kExtendedSurfaceRequests, 3, kExtendedSurfaceEvents,
};

enum ExtendedSurfaceRequest {
	RequestUpdateGenericProperty = 0,
};

enum SurfaceExtensionRequest {
	RequestGetExtendedSurface = 0,
};

// ---------------------------------------------------------------------------
// QVariant-сериализация значений свойств: [u32 тип BE][байт isNull][поле
// BE]. Строки — UTF-16BE с байтовой длиной. Кодирует то же, что и Qt/
// aurora-gui (aurora_app/src/q_variant_compat.rs) — вплоть до байтов.
// ---------------------------------------------------------------------------

enum QMetaTypeKind : uint32_t {
	kTypeBool = 1,
	kTypeUInt = 3,
	kTypeULongLong = 5,
	kTypeString = 10,
};

void AppendBe32(std::vector<unsigned char> &out, uint32_t value)
{
	out.push_back(static_cast<unsigned char>(value >> 24));
	out.push_back(static_cast<unsigned char>(value >> 16));
	out.push_back(static_cast<unsigned char>(value >> 8));
	out.push_back(static_cast<unsigned char>(value));
}

void AppendBe64(std::vector<unsigned char> &out, uint64_t value)
{
	for (int shift = 56; shift >= 0; shift -= 8) {
		out.push_back(static_cast<unsigned char>(value >> shift));
	}
}

std::vector<unsigned char> QVariantBool(bool value)
{
	std::vector<unsigned char> out;
	AppendBe32(out, kTypeBool);
	out.push_back(0);
	out.push_back(value ? 1 : 0);
	return out;
}

std::vector<unsigned char> QVariantUInt(uint64_t value)
{
	std::vector<unsigned char> out;
	AppendBe32(out, kTypeULongLong);
	out.push_back(0);
	AppendBe64(out, value);
	return out;
}

std::vector<unsigned char> QVariantString(const char *value)
{
	std::vector<unsigned char> out;
	AppendBe32(out, kTypeString);
	out.push_back(0);
	// Длина — в байтах UTF-16 (2 байта на BMP-символ).
	const size_t chars = std::strlen(value);
	AppendBe32(out, static_cast<uint32_t>(chars * 2));
	for (size_t i = 0; i < chars; ++i) {
		// ASCII-подмножества ("cover", "__winref:N") достаточно.
		const unsigned char ch = static_cast<unsigned char>(value[i]);
		out.push_back(0);
		out.push_back(ch);
	}
	return out;
}

// ---------------------------------------------------------------------------
// Транспорт: один проход по registry дисплея окна — qt_surface_extension +
// core-глобалы для собственной поверхности обложки (тот же приём, что в
// ComposerAdapter::GetScreenDpi).
// ---------------------------------------------------------------------------

struct RegistryState {
	uint32_t extensionName = 0;
	uint32_t compositorName = 0;
	uint32_t shmName = 0;
	uint32_t shellName = 0;
	bool extensionFound = false;
};

void RegistryGlobal(void *data, wl_registry *registry, uint32_t name,
    const char *interface, uint32_t version)
{
	(void)registry;
	(void)version;
	auto *state = static_cast<RegistryState *>(data);
	if (std::strcmp(interface, "qt_surface_extension") == 0) {
		state->extensionName = name;
		state->extensionFound = true;
	} else if (std::strcmp(interface, "wl_compositor") == 0) {
		state->compositorName = name;
	} else if (std::strcmp(interface, "wl_shm") == 0) {
		state->shmName = name;
	} else if (std::strcmp(interface, "wl_shell") == 0) {
		state->shellName = name;
	}
}

void RegistryGlobalRemove(void *data, wl_registry *registry, uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

const wl_registry_listener kRegistryListener = {
	RegistryGlobal,
	RegistryGlobalRemove,
};

wl_proxy *ExtendedSurfaceOf(wl_proxy *extension, wl_surface *surface)
{
	if (extension == nullptr || surface == nullptr) {
		return nullptr;
	}
	// Вариадик конструктора повторяет сигнатуру запроса: слот new_id
	// занимает NULL-заглушка (объект создаст сам конструктор), фактические
	// аргументы идут после неё — как в коде wayland-scanner.
	return wl_proxy_marshal_constructor(
	    extension, RequestGetExtendedSurface, &qt_extended_surface_interface_impl,
	    nullptr, surface);
}

void SetProperty(wl_proxy *extendedSurface, const char *name, const std::vector<unsigned char> &value)
{
	if (extendedSurface == nullptr) {
		return;
	}
	wl_array wire;
	wl_array_init(&wire);
	void *payload = wl_array_add(&wire, value.size());
	if (payload == nullptr) {
		wl_array_release(&wire);
		return;
	}
	std::memcpy(payload, value.data(), value.size());
	wl_proxy_marshal(extendedSurface, RequestUpdateGenericProperty, name, &wire);
	wl_array_release(&wire);
}

// ---------------------------------------------------------------------------
// Поверхность обложки. Своя, минуя SDL: SDL делает окно toplevel и не
// отдаёт wl_shell_surface, а обложка обязана быть transient — иначе она
// поднимается в стек обычных окон (реестр ошибки Aurora 5.2 от автора
// winit-форка). Кадр — свой SHM-буфер ARGB8888.
// ---------------------------------------------------------------------------

struct NativeCoverState {
	wl_display *display = nullptr;
	wl_proxy *extension = nullptr;
	wl_shm *shm = nullptr;
	wl_surface *surface = nullptr;
	wl_shell_surface *shellSurface = nullptr;
	wl_shm_pool *pool = nullptr;
	wl_buffer *buffer = nullptr;
	void *poolPixels = nullptr;
	size_t poolSize = 0;
	int width = 0;
	int height = 0;
	std::vector<unsigned char> content;
	int contentWidth = 0;
	int contentHeight = 0;
	int contentStride = 0;
	uint64_t winId = 0;
	bool linked = false;
};

NativeCoverState &State()
{
	static NativeCoverState state;
	return state;
}

bool EnsureShmBuffer(NativeCoverState &s, wl_shm *shm);
bool CommitFrame(const unsigned char *rgb24, int strideBytes, int srcWidth, int srcHeight);

/// Тип события «configure обложки». Регистрируется лениво, один на
/// процесс; отдельное имя — чтобы метод NativeCover::ConfigureEventType
/// не звал сам себя (класс-скоуп перекрывает неймспейс).
Uint32 CoverResizeEventType()
{
	static const Uint32 type = SDL_RegisterEvents(1);
	return type;
}

void ShellSurfacePing(void *data, wl_shell_surface *shellSurface, uint32_t serial)
{
	(void)data;
	// Не отвечать на ping = «окно зависло» с точки зрения композитора.
	wl_shell_surface_pong(shellSurface, serial);
}

void ShellSurfaceConfigure(void *data, wl_shell_surface *shellSurface,
    uint32_t edges, int32_t width, int32_t height)
{
	(void)data;
	(void)shellSurface;
	(void)edges;
	// Свитчер ресайзит окно обложки под карточку плитки (cover.resize):
	// без ответа на configure плитка остаётся пустой. Пересоздаём пул под
	// новый размер и перезаливаем контент с масштабированием.
	NativeCoverState &s = State();
	if (!s.linked || width <= 0 || height <= 0 || (width == s.width && height == s.height)) {
		return;
	}
	spdlog::info("aurora-native-cover: configure {}x{}", width, height);
	s.width = width;
	s.height = height;
	// Будим цикл обложки: карточку надо перерисовать в новом размере
	// (лого/шрифты ложатся под аспект плитки, кроп в UpdateFrame —
	// тождество). Слушатель живёт на дефолтной очереди дисплея SDL,
	// диспетчеризует его сам SDL из Poll/WaitEvent — главный поток,
	// поэтому SDL_PushEvent здесь безопасен (как из потоков StateWatch).
	{
		SDL_Event resize {};
		resize.type = CoverResizeEventType();
		SDL_PushEvent(&resize);
	}
	if (s.buffer != nullptr) {
		wl_buffer_destroy(s.buffer);
		s.buffer = nullptr;
	}
	if (s.pool != nullptr) {
		wl_shm_pool_destroy(s.pool);
		s.pool = nullptr;
	}
	if (s.poolPixels != nullptr) {
		::munmap(s.poolPixels, s.poolSize);
		s.poolPixels = nullptr;
		s.poolSize = 0;
	}
	if (s.shm == nullptr || s.content.empty()) {
		return;
	}
	if (EnsureShmBuffer(s, s.shm) && !CommitFrame(s.content.data(), s.contentStride, s.contentWidth, s.contentHeight)) {
		spdlog::warn("aurora-native-cover: перезаливка после configure не удалась");
	}
}

void ShellSurfacePopupDone(void *data, wl_shell_surface *shellSurface)
{
	(void)data;
	(void)shellSurface;
}

const wl_shell_surface_listener kShellSurfaceListener = {
	ShellSurfacePing,
	ShellSurfaceConfigure,
	ShellSurfacePopupDone,
};


std::optional<wl_surface *> WindowSurface(SDL_Window *window)
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_WAYLAND) {
		return std::nullopt;
	}
	return info.info.wl.surface;
}

/// Один постоянный SHM-пул на всё время жизни обложки: memfd + mmap,
/// буфер создаётся раз. Кадры пишутся прямо в mmap и коммитятся тем же
/// wl_buffer — никакого пересоздания пулов (уничтожение пула под ногами
/// композитора давало «спектр» из переработанной памяти).
bool EnsureShmBuffer(NativeCoverState &s, wl_shm *shm)
{
	if (s.buffer != nullptr) {
		return true;
	}
	const size_t size = static_cast<size_t>(s.width) * s.height * 4;
	const int fd = static_cast<int>(::syscall(SYS_memfd_create, "dx-cover", 1 /* MFD_CLOEXEC */));
	if (fd < 0) {
		return false;
	}
	if (::ftruncate(fd, static_cast<off_t>(size)) != 0) {
		::close(fd);
		return false;
	}
	s.poolPixels = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (s.poolPixels == MAP_FAILED) {
		s.poolPixels = nullptr;
		::close(fd);
		return false;
	}
	s.pool = wl_shm_create_pool(shm, fd, static_cast<int32_t>(size));
	::close(fd);
	if (s.pool == nullptr) {
		::munmap(s.poolPixels, size);
		s.poolPixels = nullptr;
		return false;
	}
	s.buffer = wl_shm_pool_create_buffer(s.pool, 0, s.width, s.height, s.width * 4, WL_SHM_FORMAT_ARGB8888);
	if (s.buffer == nullptr) {
		wl_shm_pool_destroy(s.pool);
		s.pool = nullptr;
		::munmap(s.poolPixels, size);
		s.poolPixels = nullptr;
		return false;
	}
	s.poolSize = size;
	return true;
}

bool CommitFrame(const unsigned char *rgb24, int strideBytes, int srcWidth, int srcHeight)
{
	NativeCoverState &s = State();
	if (!EnsureShmBuffer(s, s.shm)) {
		spdlog::warn("aurora-native-cover: SHM-буфер не создан");
		return false;
	}
	// Кадр сохраняем в исходном разрешении — после configure свитчера
	// (окно обложки ресайзится под карточку плитки) перезаливаем его
	// с масштабированием.
	s.content.assign(rgb24, rgb24 + static_cast<size_t>(srcHeight) * strideBytes);
	s.contentWidth = srcWidth;
	s.contentHeight = srcHeight;
	s.contentStride = strideBytes;
	// Дебаг-гейт: сплошной красный вместо контента.
	const bool debugRed = [] {
		const char *env = SDL_getenv("DEVILUTIONX_NATIVE_COVER_DEBUG");
		return env != nullptr && env[0] == '1';
	}();
	auto *dst = static_cast<uint32_t *>(s.poolPixels);
	// Aspect crop: заполняем карточку целиком, сохраняя пропорции —
	// избыток исходника режется по центру (без полей, full-bleed).
	const int scaledH = srcWidth > 0 ? s.width * srcHeight / srcWidth : s.height;
	const int fillByWidth = srcWidth <= 0 || scaledH >= s.height;
	const int fitW = fillByWidth ? s.width : (srcHeight > 0 ? srcWidth * s.height / srcHeight : s.width);
	const int fitH = fillByWidth ? (srcWidth > 0 ? scaledH : s.height) : s.height;
	const int cropX = (fitW - s.width) / 2;
	const int cropY = (fitH - s.height) / 2;
	for (int y = 0; y < s.height; ++y) {
		uint32_t *row = dst + static_cast<size_t>(y) * s.width;
		const int sy = fitH > 0 ? (y + cropY) * srcHeight / fitH : 0;
		const unsigned char *src = rgb24 + static_cast<size_t>(sy) * strideBytes;
		for (int x = 0; x < s.width; ++x) {
			const int sx = fitW > 0 ? (x + cropX) * srcWidth / fitW : 0;
			row[x] = debugRed
			    ? 0xFFFF0000u
			    : (0xFF000000u
			          | (static_cast<uint32_t>(src[sx * 3]) << 16)
			          | (static_cast<uint32_t>(src[sx * 3 + 1]) << 8)
			          | static_cast<uint32_t>(src[sx * 3 + 2]));
		}
	}
	wl_surface_attach(s.surface, s.buffer, 0, 0);
	wl_surface_damage(s.surface, 0, 0, s.width, s.height);
	wl_surface_commit(s.surface);
	wl_display_flush(s.display);
	return true;
}

} // namespace

bool NativeCover::CreateAndLink(
    SDL_Window *mainWindow, int width, int height, const unsigned char *rgb24, int strideBytes)
{
	NativeCoverState &s = State();
	if (s.linked || mainWindow == nullptr || rgb24 == nullptr || width <= 0 || height <= 0) {
		return s.linked;
	}

	auto mainSurface = WindowSurface(mainWindow);
	if (!mainSurface.has_value() || *mainSurface == nullptr) {
		spdlog::info("aurora-native-cover: окно без wayland-поверхности, нативная обложка выключена");
		return false;
	}
	wl_display *display = nullptr;
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (SDL_GetWindowWMInfo(mainWindow, &info) && info.subsystem == SDL_SYSWM_WAYLAND) {
		display = info.info.wl.display;
	}
	if (display == nullptr) {
		return false;
	}

	RegistryState registry;
	wl_registry *reg = wl_display_get_registry(display);
	wl_registry_add_listener(reg, &kRegistryListener, &registry);
	wl_display_roundtrip(display);
	wl_proxy *extension = nullptr;
	wl_proxy *shmProxy = nullptr;
	wl_proxy *compositorProxy = nullptr;
	wl_proxy *shellProxy = nullptr;
	if (registry.extensionFound) {
		extension = static_cast<wl_proxy *>(wl_registry_bind(reg, registry.extensionName, &kSurfaceExtensionInterface, 1));
	}
	if (registry.compositorName != 0) {
		compositorProxy = static_cast<wl_proxy *>(wl_registry_bind(reg, registry.compositorName, &wl_compositor_interface, 1));
	}
	if (registry.shmName != 0) {
		shmProxy = static_cast<wl_proxy *>(wl_registry_bind(reg, registry.shmName, &wl_shm_interface, 1));
	}
	if (registry.shellName != 0) {
		shellProxy = static_cast<wl_proxy *>(wl_registry_bind(reg, registry.shellName, &wl_shell_interface, 1));
	}
	wl_registry_destroy(reg);
	if (extension == nullptr || compositorProxy == nullptr || shmProxy == nullptr || shellProxy == nullptr) {
		spdlog::info("aurora-native-cover: композитор не отдал полный набор глобалов "
		             "(ext={} comp={} shm={} shell={})",
		    registry.extensionFound, compositorProxy != nullptr, shmProxy != nullptr, shellProxy != nullptr);
		return false;
	}

	// Собственная поверхность обложки: transient к ГЛАВНОМУ окну, как у
	// Qt/Silica, — так окно выпадает из стека обычных окон и живёт только
	// в слое обложек (transient к самой себе на 5.2.1.200 не мапился).
	auto *compositor = reinterpret_cast<wl_compositor *>(compositorProxy);
	auto *shell = reinterpret_cast<wl_shell *>(shellProxy);
	wl_surface *coverSurface = wl_compositor_create_surface(compositor);
	if (coverSurface == nullptr) {
		return false;
	}
	wl_shell_surface *shellSurface = wl_shell_get_shell_surface(shell, coverSurface);
	if (shellSurface == nullptr) {
		wl_surface_destroy(coverSurface);
		return false;
	}
	wl_shell_surface_add_listener(shellSurface, &kShellSurfaceListener, nullptr);
	// DEVILUTIONX_COVER_SEQ — порядок установки роли/свойств/мапа:
	//   a: transient -> свойства -> commit (классификация до мапа);
	//   b: toplevel -> commit -> свойства -> transient (флоу aurora-gui);
	//   c: toplevel -> свойства -> transient -> commit.
	const char *seqEnv = SDL_getenv("DEVILUTIONX_COVER_SEQ");
	const char seq = (seqEnv != nullptr && seqEnv[0] >= 'a' && seqEnv[0] <= 'c') ? seqEnv[0] : 'a';
	const bool roleTopFirst = seq == 'b' || seq == 'c';
	if (roleTopFirst) {
		wl_shell_surface_set_toplevel(shellSurface);
	} else {
		// Родитель transient — ГЛАВНОЕ окно (так делает Qt/Silica; transient
		// к самой себе — вырожденный случай, lipstick мог не создавать
		// оконный айтем вовсе).
		wl_shell_surface_set_transient(shellSurface, *mainSurface, 0, 0, 0);
	}
	wl_shell_surface_set_title(shellSurface, "cover");
	// Class обложки = class главного окна (у SDL это SDL_VIDEO_WAYLAND_WMCLASS
	// либо имя бинарника) — home ассоциирует окна приложения.
	const char *wmClass = SDL_getenv("SDL_VIDEO_WAYLAND_WMCLASS");
	if (wmClass == nullptr) {
		char exe[512];
		const ssize_t n = ::readlink("/proc/self/exe", exe, sizeof(exe) - 1);
		exe[n > 0 ? n : 0] = '\0';
		const char *base = std::strrchr(exe, '/');
		wmClass = base != nullptr ? base + 1 : exe;
	}
	wl_shell_surface_set_class(shellSurface, wmClass);

	s.display = display;
	s.extension = extension;
	s.shm = reinterpret_cast<wl_shm *>(shmProxy);
	s.surface = coverSurface;
	s.shellSurface = shellSurface;
	s.width = width;
	s.height = height;
	const auto applyProperties = [&]() {
		// Свойства главного окна — ТОЛЬКО через SDL (патч форка): lipstick
		// читает их с первого qt_extended_surface, принадлежащего SDL.
		// Свойства обложки — через наш extended surface: он первый у её
		// собственной поверхности. Порядок зеркалит эталон (jolla-settings).
		s.winId = 2;
		char winRef[32];
		std::snprintf(winRef, sizeof(winRef), "__winref:%llu", static_cast<unsigned long long>(s.winId));
		const std::vector<unsigned char> mainWinId = QVariantUInt(1);
		SDL_WaylandSetWindowGenericProperty(mainWindow, "WINID", mainWinId.data(), mainWinId.size());
		// Зонд живости SDL-пути: DEVILUTIONX_NATIVE_COVER_PROBE=1 —
		// статусбар должен появиться поверх приложения.
		const char *probe = SDL_getenv("DEVILUTIONX_NATIVE_COVER_PROBE");
		if (probe != nullptr && probe[0] == '1') {
			const std::vector<unsigned char> statusBar = QVariantBool(true);
			SDL_WaylandSetWindowGenericProperty(mainWindow, "STATUSBAR_VISIBLE", statusBar.data(), statusBar.size());
		}
		const std::vector<unsigned char> haveCover = QVariantBool(true);
		SDL_WaylandSetWindowGenericProperty(mainWindow, "SAILFISH_HAVE_COVER", haveCover.data(), haveCover.size());
		const std::vector<unsigned char> coverWindow = QVariantString(winRef);
		SDL_WaylandSetWindowGenericProperty(mainWindow, "SAILFISH_COVER_WINDOW", coverWindow.data(), coverWindow.size());

		wl_proxy *coverExtended = ExtendedSurfaceOf(extension, coverSurface);
		SetProperty(coverExtended, "WINID", QVariantUInt(s.winId));
		SetProperty(coverExtended, "CATEGORY", QVariantString("cover"));
		SetProperty(coverExtended, "TRANSPARENT", QVariantBool(false));
		wl_display_flush(display);
	};
	const auto makeTransient = [&]() {
		wl_shell_surface_set_transient(shellSurface, *mainSurface, 0, 0, 0);
		wl_display_flush(display);
	};

	if (seq == 'b') {
		if (!CommitFrame(rgb24, strideBytes, width, height)) {
			spdlog::warn("aurora-native-cover: первый кадр не залит");
		}
		wl_display_roundtrip(display);
		applyProperties();
		wl_display_roundtrip(display);
		makeTransient();
	} else if (seq == 'c') {
		applyProperties();
		makeTransient();
		if (!CommitFrame(rgb24, strideBytes, width, height)) {
			spdlog::warn("aurora-native-cover: первый кадр не залит");
		}
	} else {
		applyProperties();
		if (!CommitFrame(rgb24, strideBytes, width, height)) {
			spdlog::warn("aurora-native-cover: первый кадр не залит");
		}
	}
	wl_display_roundtrip(display);

	s.linked = true;
	spdlog::info("aurora-native-cover: нативная обложка связана (seq={}, WINID={}, {}x{})",
	    seq, s.winId, width, height);
	return true;
}

bool NativeCover::UpdateFrame(const unsigned char *rgb24, int strideBytes, int srcWidth, int srcHeight)
{
	NativeCoverState &s = State();
	if (!s.linked || s.surface == nullptr || rgb24 == nullptr) {
		return false;
	}
	return CommitFrame(rgb24, strideBytes, srcWidth, srcHeight);
}

void NativeCover::Size(int &outWidth, int &outHeight)
{
	NativeCoverState &s = State();
	outWidth = s.linked ? s.width : 0;
	outHeight = s.linked ? s.height : 0;
}

Uint32 NativeCover::ConfigureEventType()
{
	return CoverResizeEventType();
}

} // namespace devilution

#endif

// Компилируется только в сборке под Aurora OS (см. CMakeLists).

#ifdef AURORA_OS

#include "NativeCover.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include <cstdio>
#include <cstring>
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>

namespace devilution {

namespace {

// ---------------------------------------------------------------------------
// Протокол qt_surface_extension (wayland-protocols-plasma/surface-extension,
// v1). Генерировать wayland-scanner'ом ради трёх запросов не хочется —
// интерфейсы описаны вручную; сигнатуры обязаны совпадать с XML, иначе
// композитор порвёт соединение (события демаршализуются по нашим же
// сигнатурам даже без слушателя).
// ---------------------------------------------------------------------------

extern const wl_interface qt_extended_surface_interface_impl;

const wl_interface *const kGetExtendedSurfaceTypes[2] = {
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
	QVariantVoid = 0,
	QVariantBool = 1,
	QVariantUInt = 3,
	QVariantULongLong = 5,
	QVariantString = 10,
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
	AppendBe32(out, QVariantBool);
	out.push_back(0);
	out.push_back(value ? 1 : 0);
	return out;
}

std::vector<unsigned char> QVariantUInt(uint64_t value)
{
	std::vector<unsigned char> out;
	AppendBe32(out, QVariantULongLong);
	out.push_back(0);
	AppendBe64(out, value);
	return out;
}

std::vector<unsigned char> QVariantString(const char *value)
{
	std::vector<unsigned char> out;
	AppendBe32(out, QVariantString);
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
// Транспорт: бинд глобала на дисплейном коннекте окна (тот же приём, что в
// ComposerAdapter::GetScreenDpi) и по qt_extended_surface на каждое окно.
// ---------------------------------------------------------------------------

struct RegistryState {
	uint32_t extensionName = 0;
	bool found = false;
};

void RegistryGlobal(void *data, wl_registry *registry, uint32_t name,
    const char *interface, uint32_t version)
{
	(void)registry;
	(void)version;
	auto *state = static_cast<RegistryState *>(data);
	if (std::strcmp(interface, "qt_surface_extension") == 0) {
		state->extensionName = name;
		state->found = true;
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

wl_proxy *BindSurfaceExtension(wl_display *display)
{
	wl_registry *registry = wl_display_get_registry(display);
	if (registry == nullptr) {
		return nullptr;
	}
	RegistryState state;
	wl_registry_add_listener(registry, &kRegistryListener, &state);
	wl_display_roundtrip(display);
	wl_registry_destroy(registry);
	if (!state.found) {
		return nullptr;
	}
	return static_cast<wl_proxy *>(
	    wl_registry_bind(registry, state.extensionName, &kSurfaceExtensionInterface, 1));
}

wl_proxy *ExtendedSurfaceOf(wl_proxy *extension, wl_surface *surface)
{
	if (extension == nullptr || surface == nullptr) {
		return nullptr;
	}
	return wl_proxy_marshal_constructor(
	    extension, RequestGetExtendedSurface, &qt_extended_surface_interface_impl, surface, nullptr);
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
	wl_proxy_marshal(extendedSurface, RequestUpdateGenericProperty, name, &wire, nullptr);
	wl_array_release(&wire);
}

// ---------------------------------------------------------------------------
// Состояние POC: окно обложки и его расширенная поверхность живут до конца
// процесса (обложка нужна всё время работы приложения).
// ---------------------------------------------------------------------------

struct NativeCoverState {
	SDL_Window *window = nullptr;
	wl_proxy *extension = nullptr;
	wl_proxy *extendedSurface = nullptr;
	uint64_t winId = 0;
	bool linked = false;
};

NativeCoverState &State()
{
	static NativeCoverState state;
	return state;
}

std::optional<wl_surface *> WindowSurface(SDL_Window *window)
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_WAYLAND) {
		return std::nullopt;
	}
	return info.info.wl.surface;
}

bool BlitFrame(SDL_Window *window, int width, int height, const unsigned char *rgb24, int strideBytes)
{
	SDL_Surface *src = SDL_CreateRGBSurfaceWithFormatFrom(
	    const_cast<unsigned char *>(rgb24), width, height, 24, strideBytes, SDL_PIXELFORMAT_RGB24);
	if (src == nullptr) {
		spdlog::warn("aurora-native-cover: SDL_CreateRGBSurfaceWithFormatFrom: {}", SDL_GetError());
		return false;
	}
	SDL_Surface *dst = SDL_GetWindowSurface(window);
	bool ok = false;
	if (dst != nullptr) {
		ok = SDL_BlitScaled(src, nullptr, dst, nullptr) == 0;
		if (ok && SDL_UpdateWindowSurface(window) != 0) {
			ok = false;
		}
	}
	if (!ok) {
		spdlog::warn("aurora-native-cover: заливка кадра не удалась: {}", SDL_GetError());
	}
	SDL_FreeSurface(src);
	return ok;
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
	wl_display *display = {};
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (SDL_GetWindowWMInfo(mainWindow, &info) && info.subsystem == SDL_SYSWM_WAYLAND) {
		display = info.info.wl.display;
	}
	if (display == nullptr) {
		return false;
	}

	s.extension = BindSurfaceExtension(display);
	if (s.extension == nullptr) {
		spdlog::info("aurora-native-cover: qt_surface_extension не предоставлен композитором");
		return false;
	}

	s.window = SDL_CreateWindow("cover", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	    width, height, SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI);
	if (s.window == nullptr) {
		spdlog::warn("aurora-native-cover: SDL_CreateWindow(обложка): {}", SDL_GetError());
		return false;
	}
	auto coverSurface = WindowSurface(s.window);
	if (!coverSurface.has_value() || *coverSurface == nullptr) {
		spdlog::warn("aurora-native-cover: у окна обложки нет wayland-поверхности");
		SDL_DestroyWindow(s.window);
		s.window = nullptr;
		return false;
	}

	// Свойства ставим ДО первого коммита окна обложки: категория cover
	// должна встать раньше, чем композитор увидит буфер.
	s.winId = 1;
	wl_proxy *coverExtended = ExtendedSurfaceOf(s.extension, *coverSurface);
	SetProperty(coverExtended, "WINID", QVariantUInt(s.winId));
	SetProperty(coverExtended, "CATEGORY", QVariantString("cover"));
	SetProperty(coverExtended, "TRANSPARENT", QVariantBool(false));

	wl_proxy *mainExtended = ExtendedSurfaceOf(s.extension, *mainSurface);
	char winRef[32];
	std::snprintf(winRef, sizeof(winRef), "__winref:%llu", static_cast<unsigned long long>(s.winId));
	SetProperty(mainExtended, "SAILFISH_HAVE_COVER", QVariantBool(true));
	SetProperty(mainExtended, "SAILFISH_COVER_WINDOW", QVariantString(winRef));

	wl_display_flush(display);

	// Мапим окно и заливаем первый кадр: без закоммиченного буфера плитке
	// нечего показывать. Свойства уже на месте — Lipstick обязан отнести
	// окно к слою обложек, а не к стеку обычных окон.
	SDL_ShowWindow(s.window);
	if (!BlitFrame(s.window, width, height, rgb24, strideBytes)) {
		// Обложка без кадра бессмысленна, но связку не рвём: кадр можно
		// долить через UpdateFrame по ходу работы.
		spdlog::warn("aurora-native-cover: первый кадр не залит, продолжаем");
	}

	s.extendedSurface = coverExtended;
	s.linked = true;
	spdlog::info("aurora-native-cover: нативная обложка связана (WINID={}, {}x{})",
	    s.winId, width, height);
	return true;
}

bool NativeCover::UpdateFrame(const unsigned char *rgb24, int strideBytes)
{
	NativeCoverState &s = State();
	if (!s.linked || s.window == nullptr || rgb24 == nullptr) {
		return false;
	}
	int width = 0;
	int height = 0;
	SDL_GetWindowSize(s.window, &width, &height);
	return BlitFrame(s.window, width, height, rgb24, strideBytes);
}

} // namespace devilution

#endif

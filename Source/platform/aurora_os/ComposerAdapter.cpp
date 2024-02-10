#include "ComposerAdapter.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>

namespace devilution
{

struct ctx {
  struct wl_list outputs;
};

struct output_t {
  int id;
  int size;
  int physicalWidth;
  int physicalHeight;

  std::optional<float> dpi_w;
  std::optional<float> dpi_h;

  struct ctx *ctx;
  struct wl_output *output;
  struct wl_list link;
};

static void output_handle_geometry(void *data, struct wl_output *wl_output,
                                   int32_t x, int32_t y, int32_t physical_width,
                                   int32_t physical_height, int32_t subpixel,
                                   const char *make, const char *model,
                                   int32_t output_transform)
{
  struct output_t *out = (struct output_t *)data;

  out->size = physical_width * physical_height;
  if(output_transform == WL_OUTPUT_TRANSFORM_270 || output_transform == WL_OUTPUT_TRANSFORM_90){
      out->physicalWidth = physical_width;
      out->physicalHeight = physical_height;
  }
  else {
      out->physicalWidth = physical_height;
      out->physicalHeight = physical_width;
  }
}

static void output_handle_mode(void *data, struct wl_output *wl_output,
                               uint32_t flags, int32_t width, int32_t height,
                               int32_t refresh)
{
  auto out = (struct output_t *)data;

  out->dpi_w = float(width) * 25.4f / float(out->physicalWidth);
  out->dpi_h = float(height) * 25.4f / float(out->physicalHeight);

  SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Display DPI: w: %f, h: %f", float(*out->dpi_w), float(*out->dpi_h));
}

static void output_handle_done(void *data, struct wl_output *wl_output) {}

static void output_handle_scale(void *data, struct wl_output *wl_output,
                                int32_t scale) {
  // TODO: handle scale
}

static const struct wl_output_listener output_listener = {
    output_handle_geometry, output_handle_mode, output_handle_done,
    output_handle_scale,
};

static void global_registry_handler(void *data, struct wl_registry *registry,
                                    uint32_t id, const char *interface,
                                    uint32_t version) {
  if (!strcmp(interface, "wl_output")) {
    struct ctx *ctx = (struct ctx *)data;
    struct output_t *output = new output_t;
    output->ctx = ctx;
    output->id = id;
    output->output = (decltype (output->output))wl_registry_bind(registry, id, &wl_output_interface, version);
    wl_list_insert(&ctx->outputs, &output->link);
    wl_output_add_listener(output->output, &output_listener, output);
  }
}

static void global_registry_remover(void *data, struct wl_registry *registry,
                                    uint32_t id)
{}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler, global_registry_remover};

void WaylandComposerAdapter::SetWindowOrientation(SDL_Window* window, SDL_DisplayOrientation orientation)
{
    if(!window) {
        return;
    }

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version)
    SDL_GetWindowWMInfo(window, &info);

    auto wayland_orientation = (orientation == SDL_ORIENTATION_LANDSCAPE_FLIPPED)
            ? WL_OUTPUT_TRANSFORM_270
            : (orientation == SDL_ORIENTATION_LANDSCAPE)
                ? WL_OUTPUT_TRANSFORM_90
                : WL_OUTPUT_TRANSFORM_NORMAL;

    wl_surface *sdl_wl_surface = info.info.wl.surface;
    wl_surface_set_buffer_transform(sdl_wl_surface, wayland_orientation);
}

std::optional<vec2> WaylandComposerAdapter::GetScreenDpi(SDL_Window *window)
{
    if(!window) {
        return vec2(0,0);
    }

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version)
    SDL_GetWindowWMInfo(window, &info);

    struct ctx ctx;
    wl_list_init(&ctx.outputs);
    wl_display *sdl_wl_display = info.info.wl.display;

    struct wl_registry *registry = wl_display_get_registry(sdl_wl_display);
    wl_registry_add_listener(registry, &registry_listener, &ctx);

    wl_display_dispatch(sdl_wl_display);

    wl_display_roundtrip(sdl_wl_display);
    wl_display_roundtrip(sdl_wl_display);

    std::optional<vec2> dpi;
    struct output_t *out, *tmp;
    wl_list_for_each_safe(out, tmp, &ctx.outputs, link) {
        if(out->dpi_w && out->dpi_h) {
            dpi = vec2(*out->dpi_w, *out->dpi_h);
        }

        wl_output_destroy(out->output);
        wl_list_remove(&out->link);
        free(out);
    }

    wl_registry_destroy(registry);

    return dpi;
}

}

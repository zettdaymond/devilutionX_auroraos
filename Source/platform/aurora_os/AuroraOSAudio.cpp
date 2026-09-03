#include "AuroraOSAudio.hpp"

#include <audioresource/audioresource.h>
#include <glib.h>

#include <SDL2/SDL.h>

namespace devilution
{

extern void snd_init();
extern void snd_deinit();

struct AudioResource::Impl
{
    bool callback_finished = false;
    bool audio_resource_aquired = false;
    audioresource_t* audio_resource = nullptr;
};

static void on_audio_resource_aquired(audioresource_t* resource, bool aquired, void* user_data)
{
    auto result = static_cast<AudioResource::Impl*>(user_data);

    result->audio_resource_aquired = aquired;
    result->callback_finished = true;
}

std::unique_ptr<AudioResource> AudioResource::Aquire()
{
    auto impl = std::make_unique<AudioResource::Impl>();

    auto audio_resource = audioresource_init(AUDIO_RESOURCE_GAME, on_audio_resource_aquired, impl.get());
    audioresource_acquire(audio_resource);

    // Ждём ответа сервиса аудиополитики блокирующе (may_block=false в
    // старом цикле крутил CPU вхолостую) и не дольше тайм-аута:
    // g_timeout-источник будит итерацию, если ответа так и нет.
    constexpr guint kAcquireTimeoutMs = 3000;
    const guint wakeSource = g_timeout_add(kAcquireTimeoutMs,
        [](gpointer) -> gboolean { return G_SOURCE_REMOVE; }, nullptr);
    const gint64 deadline = g_get_monotonic_time()
        + static_cast<gint64>(kAcquireTimeoutMs) * 1000;
    while(!impl->callback_finished && g_get_monotonic_time() < deadline) {
        g_main_context_iteration(nullptr, TRUE);
    }
    g_source_remove(wakeSource);

    impl->audio_resource = audio_resource;

    if(impl->callback_finished && impl->audio_resource_aquired) {
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Audioresource with tag 'GAME' successfully aquired");
        return std::unique_ptr<AudioResource>( new AudioResource(std::move(impl)) );
    }

    if(impl->callback_finished) {
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Could not aquire audioresource with tag 'GAME'");
        audioresource_free(audio_resource);
    }
    else {
        // Ответ не пришёл, но может прийти позже: освобождать resource и
        // Impl нельзя (колбэк получил бы висячий указатель) — оставляем
        // их висеть, игра стартует без аудиоресурса.
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Audioresource acquire timed out");
        impl.release();
    }

    return nullptr;
}

AudioResource::~AudioResource()
{
    audioresource_release(m_impl->audio_resource);
    audioresource_free(m_impl->audio_resource);

    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Audioresource with tag 'GAME' has been released");
}

AudioResource::AudioResource(std::unique_ptr<Impl> && impl)
    : m_impl(std::move(impl))
{}

}

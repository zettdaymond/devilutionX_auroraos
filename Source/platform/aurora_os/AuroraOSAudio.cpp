#include "AuroraOSAudio.hpp"

#include <audioresource/audioresource.h>
#include <glib.h>

#include <SDL2/SDL.h>

#include <atomic>
#include <mutex>
#include <thread>

namespace devilution
{

namespace
{
std::mutex audio_resource_mutex;
}

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
    // Aquire вызывается из SDL-фильтра событий (InputAdapter на
    // FOCUS_GAINED), то есть прямо из внутренностей SDL_PollEvent —
    // виснуть здесь нельзя: кадр не будет показан и композитор убьёт
    // окно. После тайм-аута больше не пытаемся: каждый следующий
    // FOCUS_GAINED снова блокировал бы цикл на весь тайм-аут.
    static bool acquisitionAbandoned = false;
    if(acquisitionAbandoned) {
        return nullptr;
    }

    auto impl = std::make_unique<AudioResource::Impl>();

    auto audio_resource = audioresource_init(AUDIO_RESOURCE_GAME, on_audio_resource_aquired, impl.get());
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "audioresource: init done");
    audioresource_acquire(audio_resource);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "audioresource: acquire sent");

    // Ждём ответа, перебирая дефолтный glib-контекст БЕЗ блокировки:
    // g_main_context_iteration(nullptr, TRUE) уходила в poll() без
    // будильников и не просыпалась никогда (репродукция 2026-09-03:
    // чёрный экран на старте движка, wchan главного потока =
    // poll_schedule_timeout). Пустые проходы спят 200 мкс, общий
    // срок — 3 секунды.
    constexpr gint64 kAcquireTimeoutMs = 3000;
    const gint64 deadline = g_get_monotonic_time() + kAcquireTimeoutMs * 1000;
    while(!impl->callback_finished && g_get_monotonic_time() < deadline) {
        if(!g_main_context_iteration(nullptr, FALSE)) {
            g_usleep(200);
        }
    }

    impl->audio_resource = audio_resource;

    if(impl->callback_finished && impl->audio_resource_aquired) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "audioresource: granted");
        return std::unique_ptr<AudioResource>( new AudioResource(std::move(impl)) );
    }

    if(impl->callback_finished) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "audioresource: refused by policy");
        audioresource_free(audio_resource);
    }
    else {
        // Ответ не пришёл, но может прийти позже: освобождать resource и
        // Impl нельзя (колбэк получил бы висячий указатель) — оставляем
        // их висеть, игра стартует без аудиоресурса.
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Audioresource acquire timed out");
        acquisitionAbandoned = true;
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

void AcquireAudioResourceAsync()
{
    static std::atomic<bool> acquireInFlight { false };
    if(acquireInFlight.exchange(true)) {
        return; // запрос уже в полёте
    }

    std::thread([] {
        const gint64 startedAt = g_get_monotonic_time();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "audioresource: async acquire started");
        auto resource = AudioResource::Aquire();
        const gint64 tookMs = (g_get_monotonic_time() - startedAt) / 1000;
        {
            std::lock_guard lock(audio_resource_mutex);
            AudioresourceHolder::audio_resource = std::move(resource);
        }
        acquireInFlight.store(false);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "audioresource: async acquire finished in %lld ms (%s)",
            static_cast<long long>(tookMs),
            AudioresourceHolder::audio_resource != nullptr ? "acquired" : "not acquired");
    }).detach();
}

void ReleaseAudioResource()
{
    std::lock_guard lock(audio_resource_mutex);
    if(AudioresourceHolder::audio_resource != nullptr) {
        AudioresourceHolder::audio_resource = nullptr;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "audioresource: released");
    }
}

}

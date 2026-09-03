#pragma once

#include <memory>

typedef struct audioresource_t audioresource_t;

namespace devilution
{

static void on_audio_resource_aquired(audioresource_t* resource, bool aquired, void* user_data);

class AudioResource
{
public:
    static std::unique_ptr<AudioResource> Aquire();

    ~AudioResource();

private:
    friend void on_audio_resource_aquired(audioresource_t* resource, bool aquired, void* user_data);

    struct Impl;
    AudioResource(std::unique_ptr<Impl> && impl);

    std::unique_ptr<Impl> m_impl;

};

struct AudioresourceHolder
{
    static inline std::unique_ptr<AudioResource> audio_resource = nullptr;
};

/// Запросить аудиоресурс в фоновом потоке. Вызов не блокируется: раньше
/// Aquire() шёл прямо из SDL-фильтра событий (InputAdapter на FOCUS_GAINED),
/// то есть изнутри SDL_PollEvent, и зависание в libdbus намертво
/// замораживало первый кадр движка (чёрный экран).
void AcquireAudioResourceAsync();

/// Освободить аудиоресурс (потокобезопасно относительно AcquireAsync).
void ReleaseAudioResource();

}

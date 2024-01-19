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

//    if(aquired) {
//        snd_init();
//    }
//    else {
//        snd_deinit();
//    }
    result->audio_resource_aquired = aquired;
    result->callback_finished = true;
}

std::unique_ptr<AudioResource> AudioResource::Aquire()
{
    auto impl = std::make_unique<AudioResource::Impl>();

    auto audio_resource = audioresource_init(AUDIO_RESOURCE_GAME, on_audio_resource_aquired, impl.get());
    audioresource_acquire(audio_resource);


    while(!impl->callback_finished) {
        g_main_context_iteration(nullptr, false);
    }

    impl->audio_resource = audio_resource;

    if(impl->audio_resource_aquired) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Audioresource with tag 'GAME' successfully aquired");
        return std::unique_ptr<AudioResource>( new AudioResource(std::move(impl)) );
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Could not aquire audioresource with tag 'GAME'");
    audioresource_free(audio_resource);

    return nullptr;
}

AudioResource::~AudioResource()
{
    audioresource_release(m_impl->audio_resource);
    audioresource_free(m_impl->audio_resource);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Audioresource with tag 'GAME' has been released");
}

AudioResource::AudioResource(std::unique_ptr<Impl> && impl)
    : m_impl(std::move(impl))
{}

}

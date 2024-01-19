#pragma once

#include <memory>
#include <functional>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

namespace devilution
{
class VirtualPadSDLRenderer
{
public:
    static std::unique_ptr<VirtualPadSDLRenderer> Create(SDL_Renderer* renderer, int w, int h);

    void RenderVirtualPadToRenderTarget(std::function<void(SDL_Renderer *renderer)> const& render_function);

    SDL_Texture* GetSDLTexture() const;

    ~VirtualPadSDLRenderer();

private:
    struct Pimpl;

    VirtualPadSDLRenderer(std::unique_ptr<Pimpl> && pimpl);

    std::unique_ptr<Pimpl> m_pimpl;
};
} // namespace devilution

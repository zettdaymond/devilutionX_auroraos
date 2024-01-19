#include "VirtualPadSDLRender.hpp"

namespace devilution
{

struct SDLTextureDeleter
{
    void operator()( SDL_Texture* texture )
    {
        SDL_DestroyTexture( texture );
    }
};

struct VirtualPadSDLRenderer::Pimpl
{
    using SDLTextureUPtr = std::unique_ptr< SDL_Texture, SDLTextureDeleter >;
    SDL_Renderer* renderer;

    SDLTextureUPtr texture;
};

std::unique_ptr< VirtualPadSDLRenderer > VirtualPadSDLRenderer::Create( SDL_Renderer* renderer, int w, int h )
{
    using SDLTextureUPtr = std::unique_ptr< SDL_Texture, SDLTextureDeleter >;

    if( !renderer )
    {
        return nullptr;
    }

    auto impl = std::make_unique< Pimpl >();
    impl->renderer = renderer;

    // int w, h;
    // SDL_GetRendererOutputSize(renderer, &w, &h);
    impl->texture =
       SDLTextureUPtr( SDL_CreateTexture( renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_TARGET, w, h ) );

    SDL_SetTextureScaleMode( impl->texture.get(), SDL_ScaleModeBest );
    // SDL_SetTextureBlendMode( impl->texture.get(), SDL_BLENDMODE_BLEND);

    return std::unique_ptr< VirtualPadSDLRenderer >( new VirtualPadSDLRenderer( std::move( impl ) ) );
}

void VirtualPadSDLRenderer::RenderVirtualPadToRenderTarget(
   std::function< void( SDL_Renderer* renderer ) > const& render_function )
{
    auto prev_rt = SDL_GetRenderTarget( m_pimpl->renderer );

    SDL_SetRenderTarget( m_pimpl->renderer, m_pimpl->texture.get() );

    Uint8 r, g, b, a;
    if( SDL_GetRenderDrawColor( m_pimpl->renderer, &r, &g, &b, &a ) <= -1 )
    {
    }

    if( SDL_SetRenderDrawColor( m_pimpl->renderer, 0, 0, 0, 0 ) <= -1 )
    { // TODO only do this if window was resized
    }

    if( SDL_RenderClear( m_pimpl->renderer ) <= -1 )
    {
    }

    if( SDL_SetRenderDrawColor( m_pimpl->renderer, r, g, b, a ) <= -1 )
    { // TODO only do this if window was resized
    }

    std::invoke( render_function, m_pimpl->renderer );


    SDL_SetRenderTarget( m_pimpl->renderer, prev_rt );
}

SDL_Texture* VirtualPadSDLRenderer::GetSDLTexture() const
{
    return m_pimpl->texture.get();
}

VirtualPadSDLRenderer::~VirtualPadSDLRenderer()
{
}

VirtualPadSDLRenderer::VirtualPadSDLRenderer( std::unique_ptr< Pimpl >&& pimpl )
   : m_pimpl( std::move( pimpl ) )
{
}

} // namespace devilution

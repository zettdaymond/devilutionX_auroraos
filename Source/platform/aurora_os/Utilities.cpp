#include "Utilities.hpp"

#include <algorithm>
#include <math.h>

#include <QGuiApplication>

namespace devilution
{

vec2 rotateUV( const vec2& uv, float rotation )
{
    float mid = 0.5;

    return vec2( std::cos( rotation ) * ( uv.x - mid ) + std::sin( rotation ) * ( uv.y - mid ) + mid,
                 std::cos( rotation ) * ( uv.y - mid ) - std::sin( rotation ) * ( uv.x - mid ) + mid );
}

vec2 scaleToAspectFit( const vec2& uv, const vec2& inputSize, const vec2& outputSize )
{
    float textureAspect = inputSize.y / inputSize.x;
    float outputAspect = outputSize.x / outputSize.y;

    vec2 textureScale = vec2( 1.0f, outputAspect / textureAspect );

    vec2 newUV(textureScale.x * ( uv.x - 0.5f ) + 0.5f,
               textureScale.y * ( uv.y - 0.5f ) + 0.5f);

    return newUV;
}

float degreesToRadians( float degrees )
{
    constexpr auto kPi = 3.14159265358979323846264;
    return degrees * ( kPi / 180.0 );
}

ivec2 ApplyMouseFixes( const ivec2& originalMouse, const ivec2& sourceSize, const ivec2& destanationSize )
{
    using namespace devilution;

    float x = float( originalMouse.x ) / sourceSize.x;
    float y = float( originalMouse.y ) / sourceSize.y;

    y = 1.0f - y;

    vec2 rotated = rotateUV( vec2( x, y ), degreesToRadians( -90.0 ) );
    vec2 rescaled =
       scaleToAspectFit( rotated, vec2( sourceSize.x, sourceSize.y ), vec2( destanationSize.x, destanationSize.y ) );

    rescaled.x = std::clamp( rescaled.x, 0.0f, 1.0f );
    rescaled.y = std::clamp( rescaled.y, 0.0f, 1.0f );

    rescaled.y = 1.0 - rescaled.y;

    return ivec2{ int( rescaled.x * destanationSize.x ), int( rescaled.y * destanationSize.y ) };
}

} // namespace devilution

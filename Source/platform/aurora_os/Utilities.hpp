#pragma once

#include <cstdint>

namespace devilution
{
template< typename T >
struct Tvec2
{
//    Tvec2();
    Tvec2(T newX, T newY)
        : x(newX)
        , y(newY)
    {}

    T x;
    T y;
};

using ivec2 = Tvec2< int32_t >;
using vec2 = Tvec2< float >;

float degreesToRadians( float degrees );

vec2 rotateUV( const vec2& uv, float rotation );

vec2 scaleToAspectFit( const vec2& uv, const vec2& inputSize, const vec2& outputSize );

ivec2 ApplyMouseFixes( const ivec2& originalMouse, const ivec2& sourceSize, const ivec2& destanationSize );

} // namespace devilution

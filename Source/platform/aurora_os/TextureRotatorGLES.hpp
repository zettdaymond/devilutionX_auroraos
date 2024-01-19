#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "Utilities.hpp"

namespace devilution
{
class AuroraOsTextureAdapter
{
public:
    static std::unique_ptr< AuroraOsTextureAdapter > Create(const ivec2 &output_size);

    ~AuroraOsTextureAdapter();

    void Draw(ivec2 const& tex_size, bool with_blend);
    void ClearRT();

private:
    struct Impl;

    AuroraOsTextureAdapter( std::unique_ptr< Impl >&& impl );

    std::unique_ptr< Impl > m_impl;
};
} // namespace devilution

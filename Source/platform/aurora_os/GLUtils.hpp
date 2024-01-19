#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <GLES2/gl2.h>

namespace devilution::glutils
{

struct ShaderGLES
{
    ShaderGLES( uint32_t shader );
    ShaderGLES( ShaderGLES&& other );
    ShaderGLES( ShaderGLES& other ) = delete;
    ShaderGLES( ShaderGLES const& other ) = delete;
    ~ShaderGLES();

    uint32_t id = -1;
};

struct ShaderProgramGLES
{
    ShaderProgramGLES( uint32_t shader );
    ShaderProgramGLES( ShaderProgramGLES&& shader );
    ShaderProgramGLES( ShaderProgramGLES& shader ) = delete;
    ShaderProgramGLES( ShaderProgramGLES const& shader ) = delete;
    ~ShaderProgramGLES();

    uint32_t id = -1;
};

auto CompileShader( std::string_view code, GLenum shader_type ) -> std::optional< GLuint >;
auto CompileAndLink( std::string_view vertex_code, std::string_view fragment_code ) -> std::optional< ShaderProgramGLES >;
} // namespace devilution::glutils

#include "GLUtils.hpp"

#include <sstream>

#include <SDL2/SDL_log.h>

#include "GLFunctions.hpp"

namespace devilution::glutils
{

ShaderGLES::ShaderGLES(uint32_t shader)
    : id( shader )
{
}

ShaderGLES::ShaderGLES(ShaderGLES &&other)
    : id( other.id )
{
    other.id = 0;
}

ShaderGLES::~ShaderGLES()
{
    if( id > 0 )
    {
        gl::glDeleteShader( id );
    }
}

ShaderProgramGLES::ShaderProgramGLES(uint32_t shader)
    : id( shader )
{
}

ShaderProgramGLES::ShaderProgramGLES(ShaderProgramGLES &&shader)
    : id( shader.id )
{
    shader.id = 0;
}

ShaderProgramGLES::~ShaderProgramGLES()
{
    if( id > 0 )
    {
        gl::glDeleteProgram( id );
    }
}

std::optional<GLuint> CompileShader(std::string_view code, GLenum shader_type)
{
    GLuint shader = gl::glCreateShader( shader_type );

    if( !shader )
    {
        return {};
    }

    auto string = code.data();
    gl::glShaderSource( shader, 1, ( const char* const* )&string, nullptr );
    gl::glCompileShader( shader );

    GLint compiled = 0;
    gl::glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );

    if( compiled )
    {
        return shader;
    }

    std::stringstream ss;
    ss << "Could not compile " << ( shader_type == GL_VERTEX_SHADER ? "Vertex" : "Fragment" ) << " shader";
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", ss.str().c_str());

    GLint infoLen = 0;
    gl::glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infoLen );
    if( infoLen )
    {
        std::string buf( infoLen, 0 );
        gl::glGetShaderInfoLog( shader, infoLen, nullptr, buf.data() );

        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", buf.c_str());
    }

    gl::glDeleteShader( shader );

    return {};
}

std::optional<ShaderProgramGLES> CompileAndLink(std::string_view vertex_code, std::string_view fragment_code)
{
    std::optional< ShaderGLES > vsh = CompileShader( vertex_code, GL_VERTEX_SHADER );
    std::optional< ShaderGLES > fsh = CompileShader( fragment_code, GL_FRAGMENT_SHADER );

    if( !vsh || !fsh )
    {
        return {};
    }

    ShaderProgramGLES program = gl::glCreateProgram();

    gl::glAttachShader( program.id, vsh->id );
    gl::glAttachShader( program.id, fsh->id );
    gl::glLinkProgram( program.id );

    int infoLogLength = 0;

    GLint result = 0;
    gl::glGetProgramiv( program.id, GL_LINK_STATUS, &result );
    if( result )
    {
        return { std::move( program ) };
    }

    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Could not link shader program : ");

    gl::glGetProgramiv( program.id, GL_INFO_LOG_LENGTH, &infoLogLength );

    if( infoLogLength > 0 )
    {
        std::string errorMessage( infoLogLength, 0 );
        gl::glGetProgramInfoLog( program.id, infoLogLength, nullptr, errorMessage.data() );
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", errorMessage.c_str());
    }

    return {};
}

}

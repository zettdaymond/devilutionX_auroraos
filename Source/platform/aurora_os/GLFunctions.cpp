#include "GLFunctions.hpp"

#include <SDL2/SDL.h>

namespace devilution::glutils
{
void gl::LoadSymbols()
{
    #define LOAD_GL_FUNC(name) gl::name = (decltype(gl::name))SDL_GL_GetProcAddress( #name )

    LOAD_GL_FUNC(glDeleteShader);
    LOAD_GL_FUNC(glDeleteProgram);
    LOAD_GL_FUNC(glCreateShader);
    LOAD_GL_FUNC(glShaderSource);
    LOAD_GL_FUNC(glCompileShader);
    LOAD_GL_FUNC(glGetShaderiv);
    LOAD_GL_FUNC(glDrawArrays);
    LOAD_GL_FUNC(glEnableVertexAttribArray);
    LOAD_GL_FUNC(glVertexAttribPointer);
    LOAD_GL_FUNC(glUniform1i);
    LOAD_GL_FUNC(glActiveTexture);
    LOAD_GL_FUNC(glUseProgram);
    LOAD_GL_FUNC(glGetAttribLocation);
    LOAD_GL_FUNC(glGetUniformLocation);
    LOAD_GL_FUNC(glGetProgramInfoLog);
    LOAD_GL_FUNC(glGetProgramiv);
    LOAD_GL_FUNC(glLinkProgram);
    LOAD_GL_FUNC(glAttachShader);
    LOAD_GL_FUNC(glCreateProgram);
    LOAD_GL_FUNC(glGetShaderInfoLog);
    LOAD_GL_FUNC(glUniform2i);
    LOAD_GL_FUNC(glGetIntegerv);
    LOAD_GL_FUNC(glEnable);
    LOAD_GL_FUNC(glDisable);
    LOAD_GL_FUNC(glBlendFuncSeparate);
    LOAD_GL_FUNC(glClearColor);
    LOAD_GL_FUNC(glClear);
    LOAD_GL_FUNC(glIsEnabled);
    LOAD_GL_FUNC(glViewport);
    LOAD_GL_FUNC(glBindFramebuffer);
    LOAD_GL_FUNC(glGetError);
    LOAD_GL_FUNC(glDebugMessageCallback);
    LOAD_GL_FUNC(glGenBuffers);
    LOAD_GL_FUNC(glBindBuffer);
    LOAD_GL_FUNC(glBufferData);
    LOAD_GL_FUNC(glDeleteBuffers);

    #undef LOAD_GL_FUNC
}

bool gl::IsSymbolsLoaded()
{
    return gl::glDeleteShader && gl::glCreateShader && gl::glShaderSource && gl::glCompileShader
            && gl::glGetShaderiv && gl::glDrawArrays && gl::glEnableVertexAttribArray &&  gl::glVertexAttribPointer
            && gl::glUniform1i && gl::glActiveTexture && gl::glUseProgram && gl::glGetAttribLocation
            && gl::glGetUniformLocation && gl::glGetProgramInfoLog && gl::glGetProgramiv && gl::glLinkProgram
            && gl::glAttachShader && gl::glCreateProgram && gl::glGetShaderInfoLog && gl::glUniform2i
            && gl::glGetIntegerv && gl::glEnable && gl::glDisable && gl::glBlendFuncSeparate
            && gl::glClearColor && gl::glClear && gl::glIsEnabled && gl::glViewport && gl::glBindFramebuffer && gl::glGetError
            && gl::glDebugMessageCallback && gl::glGenBuffers && gl::glBindBuffer && gl::glBufferData;
}
}

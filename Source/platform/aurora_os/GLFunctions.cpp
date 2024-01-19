#include "GLFunctions.hpp"

#include <SDL2/SDL.h>

namespace devilution::glutils
{
void gl::LoadSymbols()
{
    gl::glDeleteShader = ( PFNGLDELETESHADERPROC )SDL_GL_GetProcAddress( "glDeleteShader" );
    gl::glDeleteProgram = ( PFNGLDELETEPROGRAMPROC )SDL_GL_GetProcAddress( "glDeleteProgram" );
    gl::glCreateShader = ( PFNGLCREATESHADERPROC )SDL_GL_GetProcAddress( "glCreateShader" );
    gl::glShaderSource = ( PFNGLSHADERSOURCEPROC )SDL_GL_GetProcAddress( "glShaderSource" );
    gl::glCompileShader = ( PFNGLCOMPILESHADERPROC )SDL_GL_GetProcAddress( "glCompileShader" );
    gl::glGetShaderiv = ( PFNGLGETSHADERIVPROC )SDL_GL_GetProcAddress( "glGetShaderiv" );
    gl::glDrawArrays = ( PFNGLDRAWARRAYSPROC )SDL_GL_GetProcAddress( "glDrawArrays" );
    gl::glEnableVertexAttribArray = ( PFNGLENABLEVERTEXATTRIBARRAYPROC )SDL_GL_GetProcAddress( "glEnableVertexAttribArray" );
    gl::glVertexAttribPointer = ( PFNGLVERTEXATTRIBPOINTERPROC )SDL_GL_GetProcAddress( "glVertexAttribPointer" );
    gl::glUniform1i = ( PFNGLUNIFORM1IPROC )SDL_GL_GetProcAddress( "glUniform1i" );
    gl::glActiveTexture = ( PFNGLACTIVETEXTUREPROC )SDL_GL_GetProcAddress( "glActiveTexture" );
    gl::glUseProgram = ( PFNGLUSEPROGRAMPROC )SDL_GL_GetProcAddress( "glUseProgram" );
    gl::glGetAttribLocation = ( PFNGLGETATTRIBLOCATIONPROC )SDL_GL_GetProcAddress( "glGetAttribLocation" );
    gl::glGetUniformLocation = ( PFNGLGETUNIFORMLOCATIONPROC )SDL_GL_GetProcAddress( "glGetUniformLocation" );
    gl::glGetProgramInfoLog = ( PFNGLGETPROGRAMINFOLOGPROC )SDL_GL_GetProcAddress( "glGetProgramInfoLog" );
    gl::glGetProgramiv = ( PFNGLGETPROGRAMIVPROC )SDL_GL_GetProcAddress( "glGetProgramiv" );
    gl::glLinkProgram = ( PFNGLLINKPROGRAMPROC )SDL_GL_GetProcAddress( "glLinkProgram" );
    gl::glAttachShader = ( PFNGLATTACHSHADERPROC )SDL_GL_GetProcAddress( "glAttachShader" );
    gl::glCreateProgram = ( PFNGLCREATEPROGRAMPROC )SDL_GL_GetProcAddress( "glCreateProgram" );
    gl::glGetShaderInfoLog = ( PFNGLGETSHADERINFOLOGPROC )SDL_GL_GetProcAddress( "glGetShaderInfoLog" );
    gl::glUniform2i = ( PFNGLUNIFORM2IPROC )SDL_GL_GetProcAddress( "glUniform2i" );
    gl::glGetIntegerv = ( PFNGLGETINTEGERVPROC )SDL_GL_GetProcAddress( "glGetIntegerv" );
    gl::glEnable = ( PFNGLENABLEPROC )SDL_GL_GetProcAddress( "glEnable" );
    gl::glDisable = ( PFNGLDISABLEPROC )SDL_GL_GetProcAddress( "glDisable" );
    gl::glBlendFuncSeparate = ( PFNGLBLENDFUNCSEPARATEPROC )SDL_GL_GetProcAddress( "glBlendFuncSeparate" );
    gl::glClearColor = ( PFNGLCLEARCOLORPROC )SDL_GL_GetProcAddress( "glClearColor" );
    gl::glClear = ( PFNGLCLEARPROC )SDL_GL_GetProcAddress( "glClear" );
    gl::glIsEnabled = ( PFNGLISENABLEDPROC )SDL_GL_GetProcAddress( "glIsEnabled" );
    gl::glViewport = ( PFNGLVIEWPORTPROC )SDL_GL_GetProcAddress( "glViewport" );
}

bool gl::IsSymbolsLoaded()
{
    return gl::glDeleteShader && gl::glCreateShader && gl::glShaderSource && gl::glCompileShader
            && gl::glGetShaderiv && gl::glDrawArrays && gl::glEnableVertexAttribArray &&  gl::glVertexAttribPointer
            && gl::glUniform1i && gl::glActiveTexture && gl::glUseProgram && gl::glGetAttribLocation
            && gl::glGetUniformLocation && gl::glGetProgramInfoLog && gl::glGetProgramiv && gl::glLinkProgram
            && gl::glAttachShader && gl::glCreateProgram && gl::glGetShaderInfoLog && gl::glUniform2i
            && gl::glGetIntegerv && gl::glEnable && gl::glDisable && gl::glBlendFuncSeparate
            && gl::glClearColor && gl::glClear && gl::glIsEnabled && gl::glViewport;
}
}

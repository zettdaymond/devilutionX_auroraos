#pragma once

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace devilution::glutils
{

class gl
{
public:
    static void LoadSymbols();
    static bool IsSymbolsLoaded();

    static inline PFNGLDELETESHADERPROC glDeleteShader = nullptr;
    static inline PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
    static inline PFNGLCREATESHADERPROC glCreateShader = nullptr;
    static inline PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
    static inline PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
    static inline PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
    static inline PFNGLDRAWARRAYSPROC glDrawArrays = nullptr;
    static inline PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
    static inline PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
    static inline PFNGLUNIFORM1IPROC glUniform1i = nullptr;
    static inline PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
    static inline PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
    static inline PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = nullptr;
    static inline PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
    static inline PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
    static inline PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
    static inline PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
    static inline PFNGLATTACHSHADERPROC glAttachShader = nullptr;
    static inline PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
    static inline PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
    static inline PFNGLUNIFORM2IPROC glUniform2i = nullptr;
    static inline PFNGLGETINTEGERVPROC glGetIntegerv = nullptr;
    static inline PFNGLENABLEPROC glEnable = nullptr;
    static inline PFNGLDISABLEPROC glDisable = nullptr;
    static inline PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate = nullptr;
    static inline PFNGLCLEARCOLORPROC glClearColor = nullptr;
    static inline PFNGLCLEARPROC glClear = nullptr;
    static inline PFNGLISENABLEDPROC glIsEnabled = nullptr;
    static inline PFNGLVIEWPORTPROC glViewport = nullptr;
    static inline PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
    static inline PFNGLGETERRORPROC glGetError = nullptr;
    static inline PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallback = nullptr;
    static inline PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
    static inline PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
    static inline PFNGLBUFFERDATAPROC glBufferData = nullptr;
    static inline PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
};
} // namespace devilutuion::glutils

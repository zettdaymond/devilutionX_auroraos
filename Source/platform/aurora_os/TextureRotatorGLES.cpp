#include "TextureRotatorGLES.hpp"

#include <iostream>
#include <sstream>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <SDL2/SDL.h>

#include "GLFunctions.hpp"
#include "GLUtils.hpp"

namespace devilution
{
using namespace glutils;

namespace
{

struct GLStateSaver
{
    struct GLViewport
    {
        GLint x, y, w, h;
    };

    GLStateSaver()
    {
        gl::glGetIntegerv(GL_CURRENT_PROGRAM,&previous_program_id);

        gl::glGetIntegerv( GL_VIEWPORT, ( GLint* )&viewport_rect );

        previousBlendState = gl::glIsEnabled(GL_BLEND);
        previousDepthTestState = gl::glIsEnabled(GL_DEPTH_TEST);
        previousStencilState = gl::glIsEnabled(GL_STENCIL_TEST);

        gl::glGetIntegerv(GL_BLEND_SRC_RGB, &last_blend_src_rgb);
        gl::glGetIntegerv(GL_BLEND_DST_RGB, &last_blend_dst_rgb);
        gl::glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_alpha);
        gl::glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_alpha);
        gl::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &drawFboId);
        gl::glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastArrayBuffer);
    }

    ~GLStateSaver()
    {
        RestoreState(previousBlendState, GL_BLEND);
        RestoreState(previousDepthTestState, GL_DEPTH_TEST);
        RestoreState(previousStencilState, GL_STENCIL_TEST);


        gl::glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
        gl::glViewport(viewport_rect.x, viewport_rect.y, viewport_rect.w, viewport_rect.h);

        gl::glUseProgram( previous_program_id );

        gl::glBindFramebuffer(GL_FRAMEBUFFER, drawFboId);
        gl::glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    }

    static void RestoreState(bool value, GLenum gl_state)
    {
        if(value) {
            gl::glEnable(gl_state);
        }
        else {
            gl::glDisable(gl_state);
        }
    }

    GLint previous_program_id;
    GLViewport viewport_rect;
    bool previousBlendState;
    bool previousDepthTestState;
    bool previousStencilState;
    GLint last_blend_src_rgb;
    GLint last_blend_dst_rgb;
    GLint last_blend_src_alpha;
    GLint last_blend_dst_alpha;
    GLint drawFboId;
    GLint lastArrayBuffer;
};

static constexpr std::string_view vertex_code =
   R"shader(
attribute vec3 position;
attribute vec2 inputTextureUV;

varying vec2 uv;

void main()
{
    gl_Position = vec4(position, 1.0);
    uv = inputTextureUV;
}

)shader";

static constexpr std::string_view fragment_code =
   R"shader(
precision highp float;

varying highp vec2 uv;

uniform sampler2D iTexture0;

uniform ivec2 OutputSize;
uniform ivec2 InputSize;

vec2 rotateUV(vec2 uv, float rotation)
{
    float mid = 0.5;
    return vec2(
        cos(rotation) * (uv.x - mid) + sin(rotation) * (uv.y - mid) + mid,
        cos(rotation) * (uv.y - mid) - sin(rotation) * (uv.x - mid) + mid
    );
}

vec2 scaleToAspectFit(vec2 uv, bool swap)
{
    float textureAspect = float(InputSize.y) / float(InputSize.x);
    float outputAspect = float(OutputSize.x) / float(OutputSize.y);

    vec2 textureScale = vec2(1.0, outputAspect / textureAspect);

    return textureScale * (uv - 0.5) + 0.5;
}

void main()
{
    highp vec2 reflected_uv = vec2(uv.x, 1.0 - uv.y);
    highp vec2 rotated_uv = rotateUV(reflected_uv, radians(90.0));
    highp vec2 scaled_uv = scaleToAspectFit(rotated_uv, true);

    gl_FragColor = texture2D(iTexture0, scaled_uv).bgra;
}

)shader";

// clang-format off
static const GLfloat squareVertices[] = {
    -1.0f,  -1.0f,  0.0f,  0.0f, 0.0f,
     1.0f,  -1.0f,  0.0f,  1.0f, 0.0f,
    -1.0f,   1.0f,  0.0f,  0.0f,  1.0f,
     1.0f,   1.0f,  0.0f,  1.0f,  1.0f,
};

// clang-format on

} // namespace

struct AuroraOsTextureAdapter::Impl
{
    Impl(const ivec2& _output_size,
         ShaderProgramGLES&& _sp,
         std::uint32_t _sampler_location,
         std::uint32_t _output_size_uniform_location,
         std::uint32_t _intput_size_uniform_location,
         std::uint32_t _pos_atrib_location,
         std::uint32_t _uv_atrib_location,
         ArrayBufferGLES&& _vertexBuffer)
        :  output_size(_output_size)
        ,  shader_program( std::move(_sp))
        ,  sampler_location(_sampler_location)
        ,  output_size_uniform_location(_output_size_uniform_location)
        ,  intput_size_uniform_location(_intput_size_uniform_location)
        ,  pos_atrib_location(_pos_atrib_location)
        ,  uv_atrib_location(_uv_atrib_location)
        ,  vertexBuffer( std::move(_vertexBuffer) )
    {
    }

    ivec2 output_size;

    ShaderProgramGLES shader_program;

    std::uint32_t sampler_location;

    std::uint32_t output_size_uniform_location;
    std::uint32_t intput_size_uniform_location;

    std::uint32_t pos_atrib_location;
    std::uint32_t uv_atrib_location;

    ArrayBufferGLES vertexBuffer;
};

std::unique_ptr< AuroraOsTextureAdapter > devilution::AuroraOsTextureAdapter::Create(ivec2 const& output_size)
{
    if(!gl::IsSymbolsLoaded()){
        gl::LoadSymbols();

        if(!gl::IsSymbolsLoaded()) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "COuld not load opengl symbols");
        }
    }

    auto shader_program = CompileAndLink( vertex_code, fragment_code );
    if( !shader_program )
    {
        return {};
    }

    auto sampler_location = gl::glGetUniformLocation( shader_program->id, "iTexture0" );

    auto output_size_uniform_location = gl::glGetUniformLocation( shader_program->id, "OutputSize" );
    auto intput_size_uniform_location = gl::glGetUniformLocation( shader_program->id, "InputSize" );

    auto pos_atrib_location = gl::glGetAttribLocation( shader_program->id, "position" );
    auto uv_atrib_location = gl::glGetAttribLocation( shader_program->id, "inputTextureUV" );

    auto vertex_buffer = MakeArrayBuffer(std::cbegin(squareVertices), sizeof(squareVertices));

    auto impl =
       std::make_unique< Impl >( output_size, std::move( shader_program.value() ), sampler_location, output_size_uniform_location,
                                 intput_size_uniform_location, pos_atrib_location, uv_atrib_location, std::move(vertex_buffer) );

    return std::unique_ptr< AuroraOsTextureAdapter >( new AuroraOsTextureAdapter( std::move( impl ) ) );
}

void AuroraOsTextureAdapter::BeginDraw()
{
    gl::glActiveTexture( GL_TEXTURE0 );
}

void AuroraOsTextureAdapter::Draw( ivec2 const& tex_size, bool with_blend )
{
    GLStateSaver gl_saver;

    gl::glUseProgram( m_impl->shader_program.id );
    gl::glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Pass output size parameters
    {
        gl::glViewport(0, 0, m_impl->output_size.x, m_impl->output_size.y);

        gl::glUniform2i( m_impl->output_size_uniform_location, m_impl->output_size.x, m_impl->output_size.y );
    }

    // Set original texture parameters
    {
        gl::glUniform2i( m_impl->intput_size_uniform_location, tex_size.x, tex_size.y );
    }

    // enable blending
    {
        if(with_blend) {
            gl::glEnable(GL_BLEND);
            gl::glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
        }
        else {
            gl::glDisable(GL_BLEND);
        }
    }

    gl::glUniform1i( m_impl->sampler_location, 0 );

    gl::glBindBuffer(GL_ARRAY_BUFFER, m_impl->vertexBuffer.id);

    size_t stride = 5 * sizeof(GLfloat);
    size_t offset = 0;
    gl::glEnableVertexAttribArray(m_impl->pos_atrib_location);
    gl::glVertexAttribPointer(m_impl->pos_atrib_location, 3, GL_FLOAT, GL_FALSE, stride, (void*) offset);

    offset += 3 * sizeof(GLfloat);

    gl::glEnableVertexAttribArray(m_impl->uv_atrib_location);
    gl::glVertexAttribPointer(m_impl->uv_atrib_location, 2, GL_FLOAT, GL_FALSE, stride, (void*) offset);

    gl::glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
}

void AuroraOsTextureAdapter::EndDraw()
{

}

void AuroraOsTextureAdapter::ClearRT()
{
//    gl::glClearColor(0,0,0,1);
//    gl::glClear(GL_COLOR_BUFFER_BIT);
}

AuroraOsTextureAdapter::AuroraOsTextureAdapter( std::unique_ptr< Impl >&& impl )
   : m_impl( std::move( impl ) )
{
}

AuroraOsTextureAdapter::~AuroraOsTextureAdapter() = default;

} // namespace devilution

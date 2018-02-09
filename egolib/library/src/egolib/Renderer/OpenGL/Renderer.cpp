//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file   egolib/Renderer/OpenGL/Renderer.cpp
/// @brief  Implementation of a renderer for OpenGL 2.1.
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/Renderer.hpp"
#include "idlib/idlib.hpp"
#include "egolib/Renderer/OpenGL/Utilities.hpp"
#include "egolib/Renderer/OpenGL/Texture.hpp"
#include "egolib/Renderer/OpenGL/RendererInfo.hpp"
#include "egolib/Renderer/OpenGL/DefaultTexture.hpp"

namespace Ego {
namespace OpenGL {

Renderer::Renderer(const std::shared_ptr<RendererInfo>& info) :
    m_info(info), m_textureUnit(info)
{
    try
    {
        m_defaultTexture1d = std::make_unique<DefaultTexture>(m_info, "<default texture 1D>", idlib::texture_type::_1D);
        try
        {
            m_defaultTexture2d = std::make_unique<DefaultTexture>(m_info, "<default texture 2D>", idlib::texture_type::_2D);
        }
        catch (...)
        {
            m_defaultTexture1d = nullptr;
            std::rethrow_exception(std::current_exception());
        }
    }
    catch (...)
    {
        std::rethrow_exception(std::current_exception());
    }
}

Renderer::Renderer() :
    Renderer(std::make_shared<RendererInfo>())
{}

Renderer::~Renderer()
{
    m_defaultTexture2d = nullptr;
    m_defaultTexture1d = nullptr;
}

std::shared_ptr<Ego::RendererInfo> Renderer::getInfo() {
    return m_info;
}

Ego::AccumulationBuffer& Renderer::getAccumulationBuffer() {
    return m_accumulationBuffer;
}

Ego::ColourBuffer& Renderer::getColourBuffer() {
    return m_colourBuffer;
}

Ego::DepthBuffer& Renderer::getDepthBuffer() {
    return m_depthBuffer;
}

Ego::StencilBuffer& Renderer::getStencilBuffer() {
    return m_stencilBuffer;
}

Ego::TextureUnit& Renderer::getTextureUnit() {
    return m_textureUnit;
}

void Renderer::setAlphaTestEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_ALPHA_TEST);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
    Utilities::isError();
}

void Renderer::setAlphaFunction(idlib::compare_function function, float value) {
    if (value < 0.0f || value > 1.0f) {
        throw std::invalid_argument("reference alpha value out of bounds");
    }
    switch (function) {
        case idlib::compare_function::always_fail:
            glAlphaFunc(GL_NEVER, value);
            break;
        case idlib::compare_function::always_pass:
            glAlphaFunc(GL_ALWAYS, value);
            break;
        case idlib::compare_function::equal:
            glAlphaFunc(GL_EQUAL, value);
            break;
        case idlib::compare_function::not_equal:
            glAlphaFunc(GL_NOTEQUAL, value);
            break;
        case idlib::compare_function::less:
            glAlphaFunc(GL_LESS, value);
            break;
        case idlib::compare_function::less_or_equal:
            glAlphaFunc(GL_LEQUAL, value);
            break;
        case idlib::compare_function::greater:
            glAlphaFunc(GL_GREATER, value);
            break;
        case idlib::compare_function::greater_or_equal:
            glAlphaFunc(GL_GEQUAL, value);
            break;
        default:
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    };
    Utilities::isError();
}

void Renderer::setBlendingEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    Utilities::isError();
}

void Renderer::setBlendFunction(idlib::color_blend_parameter sourceColour, idlib::color_blend_parameter sourceAlpha,
                                idlib::color_blend_parameter destinationColour, idlib::color_blend_parameter destinationAlpha) {
    glBlendFuncSeparate(toOpenGL(sourceColour), toOpenGL(destinationColour),
                        toOpenGL(sourceAlpha), toOpenGL(destinationAlpha));
    Utilities::isError();
}

void Renderer::setColour(const Colour4f& colour) {
    glColor4f(colour.get_r(), colour.get_g(),
              colour.get_b(), colour.get_a());
    Utilities::isError();
}

void Renderer::setCullingMode(idlib::culling_mode mode) {
    switch (mode) {
		case idlib::culling_mode::none:
            glDisable(GL_CULL_FACE);
            break;
		case idlib::culling_mode::front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
		case idlib::culling_mode::back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
		case idlib::culling_mode::back_and_front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT_AND_BACK);
            break;
        default:
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    };
    Utilities::isError();
}

void Renderer::setDepthFunction(idlib::compare_function function) {
    switch (function) {
        case idlib::compare_function::always_fail:
            glDepthFunc(GL_NEVER);
            break;
        case idlib::compare_function::always_pass:
            glDepthFunc(GL_ALWAYS);
            break;
        case idlib::compare_function::less:
            glDepthFunc(GL_LESS);
            break;
        case idlib::compare_function::less_or_equal:
            glDepthFunc(GL_LEQUAL);
            break;
        case idlib::compare_function::equal:
            glDepthFunc(GL_EQUAL);
            break;
        case idlib::compare_function::not_equal:
            glDepthFunc(GL_NOTEQUAL);
            break;
        case idlib::compare_function::greater_or_equal:
            glDepthFunc(GL_GEQUAL);
            break;
        case idlib::compare_function::greater:
            glDepthFunc(GL_GREATER);
            break;
        default:
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    };
    Utilities::isError();
}

void Renderer::setDepthTestEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    Utilities::isError();
}

void Renderer::setDepthWriteEnabled(bool enabled) {
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    Utilities::isError();
}

void Renderer::setScissorRectangle(float left, float bottom, float width, float height) {
    if (width < 0) {
        throw idlib::invalid_argument_error(__FILE__, __LINE__, "width < 0");
    }
    if (height < 0) {
        throw idlib::invalid_argument_error(__FILE__, __LINE__, "height < 0");
    }
    glScissor(left, bottom, width, height);
    Utilities::isError();
}

void Renderer::setScissorTestEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    Utilities::isError();
}

void Renderer::setStencilMaskBack(uint32_t mask) {
    static_assert(sizeof(GLint) >= sizeof(uint32_t), "GLint is smaller than uint32_t");
    glStencilMaskSeparate(GL_BACK, mask);
    Utilities::isError();
}

void Renderer::setStencilMaskFront(uint32_t mask) {
    static_assert(sizeof(GLint) >= sizeof(uint32_t), "GLint is smaller than uint32_t");
    glStencilMaskSeparate(GL_FRONT, mask);
    Utilities::isError();
}

void Renderer::setStencilTestEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_STENCIL_TEST);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    Utilities::isError();
}

void Renderer::setViewportRectangle(float left, float bottom, float width, float height) {
    if (width < 0) {
        throw std::invalid_argument("width < 0");
    }
    if (height < 0) {
        throw std::invalid_argument("height < 0");
    }
    glViewport(left, bottom, width, height);
    Utilities::isError();
}

void Renderer::setWindingMode(idlib::winding_mode mode) {
    switch (mode) {
		case idlib::winding_mode::clockwise:
            glFrontFace(GL_CW);
            break;
		case idlib::winding_mode::anti_clockwise:
            glFrontFace(GL_CCW);
            break;
        default:
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    }
    Utilities::isError();
}

void Renderer::multiplyMatrix(const Matrix4f4f& matrix) {
    // Convert from Matrix4f4f to an OpenGL matrix.
    GLfloat t[16];
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            t[i * 4 + j] = matrix(j * 4 + i);
        }
    }
    glMultMatrixf(t);
    Utilities::isError();
}

void Renderer::setPerspectiveCorrectionEnabled(bool enabled) {
    if (enabled) {
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    } else {
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    }
    Utilities::isError();
}

void Renderer::setDitheringEnabled(bool enabled) {
    if (enabled) {
        glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        glEnable(GL_DITHER);
    } else {
        glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
        glDisable(GL_DITHER);
    }
    Utilities::isError();
}

void Renderer::setPointSmoothEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_POINT_SMOOTH);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    } else {
        glDisable(GL_POINT_SMOOTH);
    }
    Utilities::isError();
}

void Renderer::setLineSmoothEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    } else {
        glDisable(GL_LINE_SMOOTH);
    }
    Utilities::isError();
}

void Renderer::setLineWidth(float width) {
    glLineWidth(width);
    Utilities::isError();
}

void Renderer::setPointSize(float size) {
    glPointSize(size);
    Utilities::isError();
}

void Renderer::setPolygonSmoothEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_POLYGON_SMOOTH);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    } else {
        glDisable(GL_POLYGON_SMOOTH);
    }
    Utilities::isError();
}

void Renderer::setMultisamplesEnabled(bool enabled) {
    // Check if MSAA is supported *at all* (by this OpenGL context).
    int multiSampleBuffers;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &multiSampleBuffers);
    int multiSamples;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &multiSamples);
    // If MSAA is not supported => Warn.
    if (!(multiSampleBuffers == 1 && multiSamples > 0)) {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "multisample antialiasing not supported", Log::EndOfEntry);
        // Otherwise => Enable/disable it depending on the argument of this function.
    } else {
        if (enabled) {
            glEnable(GL_MULTISAMPLE);
        } else {
            glDisable(GL_MULTISAMPLE);
        }
    }
    Utilities::isError();
}

void Renderer::setLightingEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING);
    }
    Utilities::isError();
}

void Renderer::setRasterizationMode(idlib::rasterization_mode mode) {
    switch (mode) {
        case idlib::rasterization_mode::point:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        case idlib::rasterization_mode::line:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case idlib::rasterization_mode::solid:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
    }
    Utilities::isError();
}

void Renderer::setGouraudShadingEnabled(bool enabled) {
    if (enabled) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }
    Utilities::isError();
}

void Renderer::render(idlib::vertex_buffer& vertexBuffer, const idlib::vertex_descriptor& vertexDescriptor, idlib::primitive_type primitiveType, size_t index, size_t length) {
    if (vertexDescriptor.get_size() != vertexBuffer.vertex_size())
    {
        throw std::invalid_argument("vertex size mismatch");
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    const char *vertices = static_cast<char *>(vertexBuffer.lock());
    for (auto it = vertexDescriptor.begin(); it != vertexDescriptor.end(); ++it) {
        const auto& vertexElementDescriptor = (*it);
        switch (vertexElementDescriptor.get_semantics()) {
        case idlib::vertex_component_semantics::POSITION:
            {
                // Enable the required client-side capabilities.
                glEnableClientState(GL_VERTEX_ARRAY);
                // Set the pointers.
                GLint size;
                GLenum type;
                switch (vertexElementDescriptor.get_syntactics())
                {
                    case idlib::vertex_component_syntactics::SINGLE_2:
                        size = 2;
                        type = GL_FLOAT;
                        break;
                    case idlib::vertex_component_syntactics::SINGLE_3:
                        size = 3;
                        type = GL_FLOAT;
                        break;
                    case idlib::vertex_component_syntactics::SINGLE_4:
                        size = 4;
                        type = GL_FLOAT;
                        break;
                    case idlib::vertex_component_syntactics::SINGLE_1:
                        throw idlib::runtime_error(__FILE__, __LINE__, "vertex format not supported");
                    default:
                        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
                };
                glVertexPointer(size, type, vertexDescriptor.get_size(),
                                vertices + vertexElementDescriptor.get_offset());
            }
            break;
            case idlib::vertex_component_semantics::COLOR:
            {
                // Enable required client-side capabilities.
                glEnableClientState(GL_COLOR_ARRAY);
                // Set the pointers.
                GLint size;
                GLenum type;
                switch (vertexElementDescriptor.get_syntactics()) {
                case idlib::vertex_component_syntactics::SINGLE_3:
                        size = 3;
                        type = GL_FLOAT;
                        break;
                case idlib::vertex_component_syntactics::SINGLE_4:
                        size = 4;
                        type = GL_FLOAT;
                        break;
                case idlib::vertex_component_syntactics::SINGLE_1:
                case idlib::vertex_component_syntactics::SINGLE_2:
                        throw idlib::runtime_error(__FILE__, __LINE__, "vertex format not supported");
                    default:
                        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
                };
                glColorPointer(size, type, vertexDescriptor.get_size(),
                               vertices + vertexElementDescriptor.get_offset());
            }
            break;
            case idlib::vertex_component_semantics::NORMAL:
            {
                // Enable the required client-side capabilities.
                glEnableClientState(GL_NORMAL_ARRAY);
                // Set the pointers.
                GLenum type;
                switch (vertexElementDescriptor.get_syntactics()) {
                    case idlib::vertex_component_syntactics::SINGLE_3:
                        type = GL_FLOAT;
                        break;
                    case idlib::vertex_component_syntactics::SINGLE_1:
                    case idlib::vertex_component_syntactics::SINGLE_2:
                    case idlib::vertex_component_syntactics::SINGLE_4:
                        throw idlib::runtime_error(__FILE__, __LINE__, "vertex format not supported");
                    default:
                        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
                };
                glNormalPointer(type, vertexDescriptor.get_size(),
                                vertices + vertexElementDescriptor.get_offset());
            }
            break;
            case idlib::vertex_component_semantics::TEXTURE:
            {
                // Enable the required client-side capabilities.
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                // Set the pointers.
                GLint size;
                GLenum type;
                switch (vertexElementDescriptor.get_syntactics()) {
                    case idlib::vertex_component_syntactics::SINGLE_1:
                        size = 1;
                        type = GL_FLOAT;
                        break;
                    case idlib::vertex_component_syntactics::SINGLE_2:
                        size = 2;
                        type = GL_FLOAT;
                        break;
                    case idlib::vertex_component_syntactics::SINGLE_3:
                        size = 3;
                        type = GL_FLOAT;
                        break;
                    case idlib::vertex_component_syntactics::SINGLE_4:
                        size = 4;
                        type = GL_FLOAT;
                        break;
                    default:
                        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
                };
                glTexCoordPointer(size, type, vertexDescriptor.get_size(),
                                  vertices + vertexElementDescriptor.get_offset());
            }
            break;
            default:
                throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
        };
    }
    const GLenum primitiveType_gl = Utilities2::toOpenGL(primitiveType);
    if (index + length > vertexBuffer.number_of_vertices()) {
        throw std::invalid_argument("out of bounds");
    }
    // Disable the enabled client-side capabilities again. 
    glDrawArrays(primitiveType_gl, index, length);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

std::array<float, 16> Renderer::toOpenGL(const Matrix4f4f& source) {
    // Convert from Matrix4f4f to an OpenGL matrix.
    std::array<float, 16> target;
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            target[i * 4 + j] = source(j * 4 + i);
        }
    }
    return target;
}

GLenum Renderer::toOpenGL(idlib::color_blend_parameter source) {
    switch (source) {
        case idlib::color_blend_parameter::zero: return GL_ZERO;
        case idlib::color_blend_parameter::one:  return GL_ONE;
        case idlib::color_blend_parameter::source0_color: return GL_SRC_COLOR;
        case idlib::color_blend_parameter::source1_color: return GL_SRC1_COLOR;
        case idlib::color_blend_parameter::one_minus_source0_color: return GL_ONE_MINUS_SRC_COLOR;
        case idlib::color_blend_parameter::one_minus_source1_color: return GL_ONE_MINUS_SRC1_COLOR;
        case idlib::color_blend_parameter::destination_color: return GL_DST_COLOR;
        case idlib::color_blend_parameter::one_minus_destination_color: return GL_ONE_MINUS_DST_COLOR;
        case idlib::color_blend_parameter::source0_alpha: return GL_SRC_ALPHA;
        case idlib::color_blend_parameter::one_minus_source0_alpha: return GL_ONE_MINUS_SRC_ALPHA;
        case idlib::color_blend_parameter::one_minus_source1_alpha: return GL_ONE_MINUS_SRC1_ALPHA;
        case idlib::color_blend_parameter::destination_alpha: return GL_DST_ALPHA;
        case idlib::color_blend_parameter::one_minus_destination_alpha: return GL_ONE_MINUS_DST_ALPHA;
		case idlib::color_blend_parameter::constant_color: return GL_CONSTANT_COLOR;
        case idlib::color_blend_parameter::one_minus_constant_color: return GL_ONE_MINUS_CONSTANT_COLOR;
        case idlib::color_blend_parameter::constant_alpha: return GL_CONSTANT_ALPHA;
        case idlib::color_blend_parameter::one_minus_constant_alpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
        case idlib::color_blend_parameter::source0_alpha_saturate: return GL_SRC_ALPHA_SATURATE;
        default:
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}

std::shared_ptr<Ego::Texture> Renderer::createTexture() {
    return std::make_shared<Texture>(this);
}

void Renderer::setProjectionMatrix(const Matrix4f4f& projectionMatrix) {
    this->Ego::Renderer::setProjectionMatrix(projectionMatrix);
    glMatrixMode(GL_PROJECTION);
    auto matrix = projectionMatrix;
    glLoadMatrixf(toOpenGL(matrix).data());
    Utilities::isError();
}

void Renderer::setViewMatrix(const Matrix4f4f& viewMatrix) {
    this->Ego::Renderer::setViewMatrix(viewMatrix);
    glMatrixMode(GL_MODELVIEW);
    // model -> world, world -> view
    auto matrix = getViewMatrix() * getWorldMatrix();
    glLoadMatrixf(toOpenGL(matrix).data());
    Utilities::isError();
}

void Renderer::setWorldMatrix(const Matrix4f4f& worldMatrix) {
    this->Ego::Renderer::setWorldMatrix(worldMatrix);
    glMatrixMode(GL_MODELVIEW);
    // model -> world, world -> view
    auto matrix = getViewMatrix() * getWorldMatrix();
    glLoadMatrixf(toOpenGL(matrix).data());
    Utilities::isError();
}

} // namespace OpenGL
} // namespace Ego

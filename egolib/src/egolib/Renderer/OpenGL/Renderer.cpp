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
/// @brief  OpenGL 2.1 based renderer
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/Renderer.hpp"
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/Core/UnhandledSwitchCaseException.hpp"
#include "egolib/Core/CollectionUtilities.hpp"
#include "egolib/Extensions/ogl_extensions.h"

// The following code ensures that for each OpenGL function variable static PF...PROC gl... = NULL; is declared/defined.
#define GLPROC(variable,type,name) \
    static type variable = NULL;
#include "egolib/Renderer/OpenGL/OpenGL.inl"
#undef GLPROC

namespace Ego
{
    namespace OpenGL
    {
        // The following function dynamically links the OpenGL function.
        static bool link()
        {
            static bool linked = false;
            if (!linked)
            {
#define GLPROC(variable,type,name) \
                variable = (type)SDL_GL_GetProcAddress(name); \
                if (!variable) \
                { \
                    return false; \
                }
#include "egolib/Renderer/OpenGL/OpenGL.inl"
#undef GLPROC
            }
            linked = true;
            return true;
        }

    }
}

namespace Ego
{
namespace OpenGL
{

/**
 * @brief
 *  Get the name of this OpenGL implementation.
 * @return
 *  the name of this OpenGL implementation
 */
std::string getName()
{
    while (GL_NO_ERROR != glGetError()) {}
    const GLubyte *bytes = glGetString(GL_RENDERER);
    GLenum error = glGetError();
    if (GL_NO_ERROR != error)
    {
        throw std::runtime_error("unable to acquire renderer back-end information");
    }
    return (const char *)bytes;
}
/**
 * @brief
 *  Get the name of the vendor of this OpenGL implementation.
 * @return
 *  the name of the vendor of this OpenGL implementation
 */
std::string getVendor()
{
    while (GL_NO_ERROR != glGetError()) {}
    const GLubyte *bytes = glGetString(GL_RENDERER);
    GLenum error = glGetError();
    if (GL_NO_ERROR != error)
    {
        throw std::runtime_error("unable to acquire renderer back-end information");
    }
    return (const char *)bytes;
}
/**
 * @brief
 *  Get the list of extensions supported by this OpenGL implementation.
 * @param [out] extensions
 *  a set of strings
 * @post
 *  the set of extensions supported by this OpenGL implementation was added to the set
 */
std::vector<std::string> getExtensions()
{
    while (GL_NO_ERROR != glGetError()) {}
    const GLubyte *bytes = glGetString(GL_EXTENSIONS);
    GLenum error = glGetError();
    if (GL_NO_ERROR != error)
    {
        throw std::runtime_error("unable to acquire renderer back-end information");
    }
    return Ego::split(std::string((const char *)bytes),std::string(" "));
}

Renderer::Renderer() :
    _extensions(Core::make_unordered_set(getExtensions())),
    _vendor(getVendor()),
    _name(getName())
{
    Ego::OpenGL::link();
}

Renderer::~Renderer()
{
    // dtor
}

Ego::AccumulationBuffer& Renderer::getAccumulationBuffer()
{
    return _accumulationBuffer;
}

Ego::ColourBuffer& Renderer::getColourBuffer()
{
    return _colourBuffer;
}

Ego::DepthBuffer& Renderer::getDepthBuffer()
{
    return _depthBuffer;
}

void Renderer::setAlphaTestEnabled(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_ALPHA_TEST);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
    Utilities::isError();
}

void Renderer::setBlendingEnabled(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }
    Utilities::isError();
}

void Renderer::setColour(const Colour4f& colour)
{
    glColor4f(colour.getRed(), colour.getGreen(),
              colour.getBlue(), colour.getAlpha());
    Utilities::isError();
}

void Renderer::setCullingMode(CullingMode mode)
{
    switch (mode)
    {
    case CullingMode::None:
        glDisable(GL_CULL_FACE);
        break;
    case CullingMode::Front:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        break;
    case CullingMode::Back:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        break;
    case CullingMode::BackAndFront:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT_AND_BACK);
        break;
    default:
        throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    };
    Utilities::isError();
}

void Renderer::setDepthFunction(CompareFunction function)
{
    switch (function)
    {
    case CompareFunction::AlwaysFail:
        glDepthFunc(GL_NEVER);
        break;
    case CompareFunction::AlwaysPass:
        glDepthFunc(GL_ALWAYS);
        break;
    case CompareFunction::Less:
        glDepthFunc(GL_LESS);
        break;
    case CompareFunction::LessOrEqual:
        glDepthFunc(GL_LEQUAL);
        break;
    case CompareFunction::Equal:
        glDepthFunc(GL_EQUAL);
        break;
    case CompareFunction::NotEqual:
        glDepthFunc(GL_NOTEQUAL);
        break;
    case CompareFunction::GreaterOrEqual:
        glDepthFunc(GL_GEQUAL);
        break;
    case CompareFunction::Greater:
        glDepthFunc(GL_GREATER);
        break;
    default:
        throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    };
    Utilities::isError();
}

void Renderer::setDepthTestEnabled(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    Utilities::isError();
}

void Renderer::setDepthWriteEnabled(bool enabled)
{
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    Utilities::isError();
}

void Renderer::setScissorRectangle(float left, float bottom, float width, float height)
{
    if (width < 0)
    {
        throw std::invalid_argument("width < 0");
    }
    if (height < 0)
    {
        throw std::invalid_argument("height < 0");
    }
    glScissor(left, bottom, width, height);
    Utilities::isError();
}

void Renderer::setScissorTestEnabled(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_SCISSOR_TEST);
    }
    else
    {
        glDisable(GL_SCISSOR_TEST);
    }
    Utilities::isError();
}

void Renderer::setStencilMaskBack(uint32_t mask)
{
    static_assert(sizeof(GLint) >= sizeof(uint32_t), "GLint is smaller than uint32_t");
    glStencilMaskSeparate(GL_BACK, mask);
    Utilities::isError();
}

void Renderer::setStencilMaskFront(uint32_t mask)
{
    static_assert(sizeof(GLint) >= sizeof(uint32_t), "GLint is smaller than uint32_t");
    glStencilMaskSeparate(GL_FRONT, mask);
    Utilities::isError();
}

void Renderer::setStencilTestEnabled(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }
    Utilities::isError();
}

void Renderer::setViewportRectangle(float left, float bottom, float width, float height)
{
    if (width < 0)
    {
        throw std::invalid_argument("width < 0");
    }
    if (height < 0)
    {
        throw std::invalid_argument("height < 0");
    }
    glViewport(left, bottom, width, height);
    Utilities::isError();
}

void Renderer::setWindingMode(WindingMode mode)
{
    switch (mode)
    {
    case WindingMode::Clockwise:
        glFrontFace(GL_CW);
        break;
    case WindingMode::AntiClockwise:
        glFrontFace(GL_CCW);
        break;
    default:
        throw Ego::Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
    Utilities::isError();
}

void Renderer::loadMatrix(const fmat_4x4_t& matrix)
{
    // fmat_4x4_t will not remain a simple array, hence the data must be packed explicitly to be passed
    // to the OpenGL API. However, currently this code is redundant.
    GLXmatrix t;
    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            t[i * 4 + j] = matrix.v[i * 4 + j];
        }
    }
    glLoadMatrixf(t);
    Utilities::isError();
}

void Renderer::multiplyMatrix(const fmat_4x4_t& matrix)
{
    // fmat_4x4_t will not remain a simple array, hence the data must be packed explicitly to be passed
    // to the OpenGL API. However, currently this code is redundant.
    GLXmatrix t;
    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            t[i * 4 + j] = matrix.v[i * 4 + j];
        }
    }
    glMultMatrixf(t);
    Utilities::isError();
}

void Renderer::setPerspectiveCorrectionEnabled(bool enabled)
{
    if (enabled)
    {
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    }
    else
    {
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    }
    Utilities::isError();
}

void Renderer::setDitheringEnabled(bool enabled)
{
    if (enabled)
    {
        glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        glEnable(GL_DITHER);
    }
    else
    {
        glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
        glDisable(GL_DITHER);
    }
    Utilities::isError();
}

void Renderer::setMultisamplesEnabled(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_MULTISAMPLE_ARB);
        Ego::OpenGL::Utilities::isError();

        // If GL_MULTISAMPLE_ARB is enabled and if SAMPLE_BUFFERS_ARB is one,
        // then the following settings are ignored - we choose to assign values to them.
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        glEnable(GL_POINT_SMOOTH);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

        glDisable(GL_POLYGON_SMOOTH);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    }
    else
    {
        glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
        glDisable(GL_DITHER);
    }
    Utilities::isError();
}


void Renderer::setGouraudShadingEnabled(bool enabled)
{
    if (enabled)
    {
        glShadeModel(GL_SMOOTH);
    }
    else
    {
        glShadeModel(GL_FLAT);
    }
    Utilities::isError();
}

void Renderer::render(VertexBuffer& vertexBuffer, PrimitiveType primitiveType, size_t index, size_t length)
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    const char *vertices = static_cast<char *>(vertexBuffer.lock());
    const auto& vertexFormatDescriptor = vertexBuffer.getVertexFormatDescriptor();
    switch (vertexFormatDescriptor.getVertexFormat())
    {
        case VertexFormat::P2F:
        {
            // Enable the required client-side capabilities.
            glEnableClientState(GL_VERTEX_ARRAY);
            // Set the pointers.
            size_t offset = 0;
            glVertexPointer(2, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
        }
        break;
        case VertexFormat::P3F:
        {
            // Enable the required client-side capabilities.
            glEnableClientState(GL_VERTEX_ARRAY);
            // Set the pointers.
            size_t offset = 0;
            glVertexPointer(3, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
        }
        break;
        case VertexFormat::P3FT2F:
        {
            // Enable the required client-side capabilities.
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            // Set the pointers.
            size_t offset = 0;
            glVertexPointer(3, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
            offset += vertexFormatDescriptor.getPositionSize();
            glTexCoordPointer(2, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                              vertices + offset);
        }
        break;
        case VertexFormat::P3FC4F:
        {
            // Enable the required client-side capabilities.
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            // Set the pointers.
            size_t offset = 0;
            glVertexPointer(3, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
            offset += vertexFormatDescriptor.getPositionSize();
            glColorPointer(4, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                           vertices + offset);
        }
        break;
        case VertexFormat::P3FC4FN3F:
        {
            // Enable the required client-side capabilities.
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glEnableClientState(GL_NORMAL_ARRAY);
            // Set the pointers.
            size_t offset = 0;
            glVertexPointer(3, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
            offset += vertexFormatDescriptor.getPositionSize();
            glColorPointer(4, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                           vertices + offset);
            offset += vertexFormatDescriptor.getColorSize();
            glNormalPointer(GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
        }
        break;
        case VertexFormat::P3FC4FT2F:
        {
            // Enable the required client-side capabilities.
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            // Set the pointers.
            size_t offset = 0;
            glVertexPointer(3, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
            offset += vertexFormatDescriptor.getPositionSize();
            glColorPointer(4, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                           vertices + offset);
            offset += vertexFormatDescriptor.getColorSize();
            glTexCoordPointer(2, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                              vertices + offset);
        }
        break;
        case VertexFormat::P3FC4FT2FN3F:
        {
            // Enable the required client-side capabilities.
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_NORMAL_ARRAY);
            // Set the pointers.
            size_t offset = 0;
            glVertexPointer(3, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
            offset += vertexFormatDescriptor.getPositionSize();
            glColorPointer(4, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                           vertices + offset);
            offset += vertexFormatDescriptor.getColorSize();
            glTexCoordPointer(2, GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                              vertices + offset);
            offset += vertexFormatDescriptor.getTextureSize();
            glNormalPointer(GL_FLOAT, vertexFormatDescriptor.getVertexSize(),
                            vertices + offset);
        }
        break;
        default:
            throw std::invalid_argument("unreachable code reached");
    };
    const GLenum primitiveType_gl = Utilities::toOpenGL(primitiveType);
    if (index + length > vertexBuffer.getNumberOfVertices())
    {
        throw std::invalid_argument("out of bounds");
    }
    // Disable the enabled client-side capabilities again. 
    glDrawArrays(primitiveType_gl, index, length);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

} // namespace OpenGL
} // namespace Ego

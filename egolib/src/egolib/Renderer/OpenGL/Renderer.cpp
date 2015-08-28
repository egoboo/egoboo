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

std::string Capabilities::getName()
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

std::string Capabilities::getVendor()
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

std::unordered_set<std::string> Capabilities::getExtensions()
{
    while (GL_NO_ERROR != glGetError()) {}
    const GLubyte *bytes = glGetString(GL_EXTENSIONS);
    GLenum error = glGetError();
    if (GL_NO_ERROR != error)
    {
        throw std::runtime_error("unable to acquire renderer back-end information");
    }
    return Core::make_unordered_set(Ego::split(std::string((const char *)bytes),std::string(" ")));
}

Renderer::Renderer() :
    _extensions(Capabilities::getExtensions()),
    _vendor(Capabilities::getVendor()),
    _name(Capabilities::getName())
{
    OpenGL::link();
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

void Renderer::setAlphaFunction(CompareFunction function, float value)
{
	if (value < 0.0f || value > 1.0f) {
		throw std::invalid_argument("reference alpha value out of bounds");
	}
	switch (function)
	{
	case CompareFunction::AlwaysFail:
		glAlphaFunc(GL_NEVER, value);
		break;
	case CompareFunction::AlwaysPass:
		glAlphaFunc(GL_ALWAYS, value);
		break;
	case CompareFunction::Equal:
		glAlphaFunc(GL_EQUAL, value);
		break;
	case CompareFunction::NotEqual:
		glAlphaFunc(GL_NOTEQUAL, value);
		break;
	case CompareFunction::Less:
		glAlphaFunc(GL_LESS, value);
		break;
	case CompareFunction::LessOrEqual:
		glAlphaFunc(GL_LEQUAL, value);
		break;
	case CompareFunction::Greater:
		glAlphaFunc(GL_GREATER, value);
		break;
	case CompareFunction::GreaterOrEqual:
		glAlphaFunc(GL_GEQUAL, value);
		break;
    default:
        throw Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
	};
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

void Renderer::setBlendFunction(BlendFunction sourceColour, BlendFunction sourceAlpha,
	                            BlendFunction destinationColour, BlendFunction destinationAlpha)
{
	glBlendFuncSeparate(toOpenGL(sourceColour), toOpenGL(sourceAlpha), 
		                toOpenGL(destinationColour), toOpenGL(destinationAlpha));
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
        throw Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
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
        throw Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
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
        throw Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
    Utilities::isError();
}

void Renderer::loadMatrix(const Matrix4f4f& matrix)
{
	// Convert from Matrix4f4f to an OpenGL matrix.
	GLfloat t[16];
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            t[i * 4 + j] = matrix(j * 4 + i);
        }
    }
    glLoadMatrixf(t);
    Utilities::isError();
}

void Renderer::multiplyMatrix(const Matrix4f4f& matrix)
{
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

void Renderer::setPointSmoothEnabled(bool enabled)
{
	if (enabled) {
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	} else {
		glDisable(GL_POINT_SMOOTH);
	}
}

void Renderer::setLineSmoothEnabled(bool enabled)
{
	if (enabled) {
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	} else {
		glDisable(GL_LINE_SMOOTH);
	}
}

void Renderer::setPolygonSmoothEnabled(bool enabled)
{
	if (enabled) {
		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	}
	else {
		glDisable(GL_POLYGON_SMOOTH);
	}
}

void Renderer::setMultisamplesEnabled(bool enabled)
{
	// Check if MSAA is supported *at all* (by this OpenGL context).
	int multiSampleBuffers;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &multiSampleBuffers);
	int multiSamples;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &multiSamples);
	// If MSAA is not supported => Warn.
	if (!(multiSampleBuffers == 1 && multiSamples > 0)) {
		log_warning("%s:%d: multisample antialiasing not supported\n", __FILE__, __LINE__);
	// Otherwise => Enable/disable it depending on the argument of this function.
	} else {
		if (enabled) {
			glEnable(GL_MULTISAMPLE);
		} else {
			glDisable(GL_MULTISAMPLE);
		}
	}
	OpenGL::Utilities::isError();
}

void Renderer::setLightingEnabled(bool enabled) {
	if (enabled) {
		glEnable(GL_LIGHTING);
	} else {
		glDisable(GL_LIGHTING);
	}
	Utilities::isError();
}

void Renderer::setRasterizationMode(RasterizationMode mode)
{
	switch (mode) {
	case RasterizationMode::Point:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	case RasterizationMode::Line:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case RasterizationMode::Solid:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
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
			throw Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
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

GLenum Renderer::toOpenGL(BlendFunction source)
{
	switch (source)
	{
	case BlendFunction::Zero: return GL_ZERO;
	case BlendFunction::One:  return GL_ONE;
	case BlendFunction::SourceColour: return GL_SRC_COLOR;
	case BlendFunction::OneMinusSourceColour: return GL_ONE_MINUS_SRC_COLOR;
	case BlendFunction::DestinationColour: return GL_DST_COLOR;
	case BlendFunction::OneMinusDestinationColour: return GL_ONE_MINUS_DST_COLOR;
	case BlendFunction::SourceAlpha: return GL_SRC_ALPHA;
	case BlendFunction::OneMinusSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
	case BlendFunction::DestinationAlpha: return GL_DST_ALPHA;
	case BlendFunction::OneMinusDestinationAlpha: return GL_ONE_MINUS_DST_ALPHA;
	case BlendFunction::ConstantColour: return GL_CONSTANT_COLOR;
	case BlendFunction::OneMinusConstantColour: return GL_ONE_MINUS_CONSTANT_COLOR;
	case BlendFunction::ConstantAlpha: return GL_CONSTANT_ALPHA;
	case BlendFunction::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
	case BlendFunction::SourceAlphaSaturate: return GL_SRC_ALPHA_SATURATE;
	default:
		throw Core::UnhandledSwitchCaseException(__FILE__, __LINE__);
	};
}

} // namespace OpenGL
} // namespace Ego

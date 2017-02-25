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
#include "egolib/Core/StringUtilities.hpp"
#include "idlib/idlib.hpp"
#include "egolib/Extensions/ogl_extensions.h"

// The following code ensures that for each OpenGL function variable static PF...PROC gl... = NULL; is declared/defined.
#define GLPROC(variable,type,name) \
    static type variable = NULL;
#include "egolib/Renderer/OpenGL/OpenGL.inl"
#undef GLPROC

namespace Ego {
namespace OpenGL {

// The following function dynamically links the OpenGL function.
static bool link() {
    static bool linked = false;
    if (!linked) {
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

} // namespace OpenGL
} // namespace Ego

namespace Ego {
namespace OpenGL {

Renderer::Renderer() :
    _extensions(Utilities::getExtensions()),
    info(Utilities::getRenderer(), Utilities::getVendor(), Utilities::getVersion()) {
    OpenGL::link();
}

Renderer::~Renderer() {}

const Ego::RendererInfo& Renderer::getInfo() {
    return info;
}

Ego::AccumulationBuffer& Renderer::getAccumulationBuffer() {
    return _accumulationBuffer;
}

Ego::ColourBuffer& Renderer::getColourBuffer() {
    return _colourBuffer;
}

Ego::DepthBuffer& Renderer::getDepthBuffer() {
    return _depthBuffer;
}

Ego::StencilBuffer& Renderer::getStencilBuffer() {
    return _stencilBuffer;
}

Ego::TextureUnit& Renderer::getTextureUnit() {
    return _textureUnit;
}

void Renderer::setAlphaTestEnabled(bool enabled) {
    if (enabled) {
        glEnable(GL_ALPHA_TEST);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
    Utilities::isError();
}

void Renderer::setAlphaFunction(CompareFunction function, float value) {
    if (value < 0.0f || value > 1.0f) {
        throw std::invalid_argument("reference alpha value out of bounds");
    }
    switch (function) {
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
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
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

void Renderer::setBlendFunction(BlendFunction sourceColour, BlendFunction sourceAlpha,
                                BlendFunction destinationColour, BlendFunction destinationAlpha) {
    glBlendFuncSeparate(toOpenGL(sourceColour), toOpenGL(destinationColour),
                        toOpenGL(sourceAlpha), toOpenGL(destinationAlpha));
    Utilities::isError();
}

void Renderer::setColour(const Colour4f& colour) {
    glColor4f(colour.getRed(), colour.getGreen(),
              colour.getBlue(), colour.getAlpha());
    Utilities::isError();
}

void Renderer::setCullingMode(CullingMode mode) {
    switch (mode) {
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
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
    };
    Utilities::isError();
}

void Renderer::setDepthFunction(CompareFunction function) {
    switch (function) {
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
            throw UnhandledSwitchCaseException(__FILE__, __LINE__);
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
        throw InvalidArgumentException(__FILE__, __LINE__, "width < 0");
    }
    if (height < 0) {
        throw InvalidArgumentException(__FILE__, __LINE__, "height < 0");
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

void Renderer::setWindingMode(WindingMode mode) {
    switch (mode) {
        case WindingMode::Clockwise:
            glFrontFace(GL_CW);
            break;
        case WindingMode::AntiClockwise:
            glFrontFace(GL_CCW);
            break;
        default:
            throw UnhandledSwitchCaseException(__FILE__, __LINE__);
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

void Renderer::setRasterizationMode(RasterizationMode mode) {
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

void Renderer::setGouraudShadingEnabled(bool enabled) {
    if (enabled) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }
    Utilities::isError();
}

void Renderer::render(VertexBuffer& vertexBuffer, PrimitiveType primitiveType, size_t index, size_t length) {
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    const char *vertices = static_cast<char *>(vertexBuffer.lock());
    const auto& vertexDescriptor = vertexBuffer.getVertexDescriptor();
    for (auto it = vertexDescriptor.begin(); it != vertexDescriptor.end(); ++it) {
        const auto& vertexElementDescriptor = (*it);
        switch (vertexElementDescriptor.getSemantics()) {
            case VertexElementDescriptor::Semantics::Position:
            {
                // Enable the required client-side capabilities.
                glEnableClientState(GL_VERTEX_ARRAY);
                // Set the pointers.
                GLint size;
                GLenum type;
                switch (vertexElementDescriptor.getSyntax())
                {
                    case VertexElementDescriptor::Syntax::F2:
                        size = 2;
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F3:
                        size = 3;
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F4:
                        size = 4;
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F1:
                        throw Id::RuntimeErrorException(__FILE__, __LINE__, "vertex format not supported");
                    default:
                        throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
                };
                glVertexPointer(size, type, vertexDescriptor.getVertexSize(),
                                vertices + vertexElementDescriptor.getOffset());
            }
            break;
            case VertexElementDescriptor::Semantics::Colour:
            {
                // Enable required client-side capabilities.
                glEnableClientState(GL_COLOR_ARRAY);
                // Set the pointers.
                GLint size;
                GLenum type;
                switch (vertexElementDescriptor.getSyntax()) {
                    case VertexElementDescriptor::Syntax::F3:
                        size = 3;
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F4:
                        size = 4;
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F1:
                    case VertexElementDescriptor::Syntax::F2:
                        throw Id::RuntimeErrorException(__FILE__, __LINE__, "vertex format not supported");
                    default:
                        throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
                };
                glColorPointer(size, type, vertexDescriptor.getVertexSize(),
                               vertices + vertexElementDescriptor.getOffset());
            }
            break;
            case VertexElementDescriptor::Semantics::Normal:
            {
                // Enable the required client-side capabilities.
                glEnableClientState(GL_NORMAL_ARRAY);
                // Set the pointers.
                GLenum type;
                switch (vertexElementDescriptor.getSyntax()) {
                    case VertexElementDescriptor::Syntax::F3:
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F1:
                    case VertexElementDescriptor::Syntax::F2:
                    case VertexElementDescriptor::Syntax::F4:
                        throw Id::RuntimeErrorException(__FILE__, __LINE__, "vertex format not supported");
                    default:
                        throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
                };
                glNormalPointer(type, vertexDescriptor.getVertexSize(),
                                vertices + vertexElementDescriptor.getOffset());
            }
            break;
            case VertexElementDescriptor::Semantics::Texture:
            {
                // Enable the required client-side capabilities.
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                // Set the pointers.
                GLint size;
                GLenum type;
                switch (vertexElementDescriptor.getSyntax()) {
                    case VertexElementDescriptor::Syntax::F1:
                        size = 1;
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F2:
                        size = 2;
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F3:
                        size = 3;
                        type = GL_FLOAT;
                        break;
                    case VertexElementDescriptor::Syntax::F4:
                        size = 4;
                        type = GL_FLOAT;
                        break;
                    default:
                        throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
                };
                glTexCoordPointer(size, type, vertexDescriptor.getVertexSize(),
                                  vertices + vertexElementDescriptor.getOffset());
            }
            break;
            default:
                throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
        };
    }
    const GLenum primitiveType_gl = Utilities::toOpenGL(primitiveType);
    if (index + length > vertexBuffer.getNumberOfVertices()) {
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

GLenum Renderer::toOpenGL(BlendFunction source) {
    switch (source) {
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
            throw UnhandledSwitchCaseException(__FILE__, __LINE__);
    };
}

SharedPtr<Ego::Texture> Renderer::createTexture() {
    return MakeShared<Texture>();
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

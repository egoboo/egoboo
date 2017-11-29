//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
//*    distributed with Egoboo.
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

/// @file egolib/Extensions/ogl_extensions.c
/// @ingroup _ogl_extensions_
/// @brief Implementation of extended functions and variables for OpenGL

#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Graphics/PixelFormat.hpp"
#include "egolib/Renderer/OpenGL/Utilities.hpp"

//--------------------------------------------------------------------------------------------

void oglx_caps_t::report() {
    auto& renderer = Ego::Renderer::get();
    Log::Entry e(Log::Level::Info, __FILE__, __LINE__);
    {
        const auto info = renderer.getInfo();
        e << info->toString();
        e << Log::EndOfLine;
    }
    {
        // Print the colour buffer colour depth.
        const auto colourDepth = renderer.getColourBuffer().getColourDepth();
        e << "  Colour Buffer Colour Depth:" << Log::EndOfLine
            << "    " << "red bits = " << colourDepth.rgb().r() << Log::EndOfLine
            << "    " << "green bits = " << colourDepth.rgb().g() << Log::EndOfLine
            << "    " << "blue bits = " << colourDepth.rgb().b() << Log::EndOfLine
            << "    " << "alpha bits = " << colourDepth.a() << Log::EndOfLine;
    }
    {
        // Print the accumulation buffer colour depth.
        const auto colourDepth = renderer.getAccumulationBuffer().getColourDepth();
        e << "  Accumulation Buffer Colour Depth:" << Log::EndOfLine
            << "    " << "red bits = " << colourDepth.rgb().r() << Log::EndOfLine
            << "    " << "green bits = " << colourDepth.rgb().g() << Log::EndOfLine
            << "    " << "blue bits = " << colourDepth.rgb().b() << Log::EndOfLine
            << "    " << "alpha bits = " << colourDepth.a() << Log::EndOfLine;
    }
    {
        // Print depth buffer and stencil buffer depth.
        e << "  Depth Buffer Depth:" << renderer.getDepthBuffer().getDepth() << Log::EndOfLine;
        e << "  Stencil Buffer Depth:" << renderer.getStencilBuffer().getDepth() << Log::EndOfLine;
    }
    e << Log::EndOfEntry;
    Log::get() << e;
}

//--------------------------------------------------------------------------------------------

namespace Ego {
namespace OpenGL {

void Utilities::clearError()
{
    Utilities2::clearError();
}

bool Utilities::isError()
{
    return Utilities2::isError();
}

const std::string& Utilities::toString(GLenum error)
{
    switch (error)
    {
        case GL_NO_ERROR:
        {
            static const std::string string = "no error";
            return string;
        }
        case GL_INVALID_ENUM:
        {
            static const std::string string = "invalid enum";
            return string;
        }
        case GL_INVALID_VALUE:
        {
            static const std::string string = "invalid value";
            return string;
        }
        case GL_INVALID_OPERATION:
        {
            static const std::string string = "invalid operation";
            return string;
        }
        case GL_STACK_OVERFLOW:
        {
            static const std::string string = "stack overflow";
            return string;
        }
        case GL_STACK_UNDERFLOW:
        {
            static const std::string string = "stack underflow";
            return string;
        }
        case GL_OUT_OF_MEMORY:
        {
            static const std::string string = "out of memory";
            return string;
        }
    #if defined(GL_INVALID_FRAMEBUFFER_OPERATION)
        case GL_INVALID_FRAMEBUFFER_OPERATION:
        {
            static const std::string string = "invalid framebuffer operation";
            return string;
        }
    #endif
        default:
        {
            static const std::string string = "<unknown error>";
            return string;
        }
    };
}

} // namespace OpenGL
} // namespace Ego

//--------------------------------------------------------------------------------------------

namespace Ego {
namespace OpenGL {

PushAttrib::PushAttrib(GLbitfield bitfield)
{
    glPushAttrib(bitfield);
}

PushAttrib::~PushAttrib()
{
    glPopAttrib();
    Utilities2::isError();
}

PushClientAttrib::PushClientAttrib(GLbitfield bitfield)
{
    glPushClientAttrib(bitfield);
}

PushClientAttrib::~PushClientAttrib()
{
    glPopClientAttrib();
    Utilities2::isError();
}

} // namespace OpenGL
} // namespace Ego

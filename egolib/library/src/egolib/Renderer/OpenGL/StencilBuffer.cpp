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

/// @file egolib/Renderer/OpenGL/StencilBuffer.cpp
/// @brief Implementation of an Stencil buffer facade for OpenGL 2.1.
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/StencilBuffer.hpp"
#include "egolib/Renderer/OpenGL/Utilities.hpp"

namespace Ego::OpenGL {

StencilBuffer::StencilBuffer() :
    Ego::StencilBuffer(), depth(Utilities2::getStencilBufferDepth()) {}

StencilBuffer::~StencilBuffer() {}

void StencilBuffer::clear() {
    glClear(GL_STENCIL_BUFFER_BIT);
    Utilities2::isError();
}

void StencilBuffer::setClearValue(const single& value) {
    glClearStencil(value);
    Utilities2::isError();
}

uint8_t StencilBuffer::getDepth() {
    return depth;
}

} // namespace Ego::OpenGL

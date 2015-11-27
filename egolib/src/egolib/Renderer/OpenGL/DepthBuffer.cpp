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

/// @file   egolib/Renderer/OpenGL/DepthBuffer.cpp
/// @brief  Implementation of a depth buffer facade for OpenGL 2.1.
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/DepthBuffer.hpp"

namespace Ego {
namespace OpenGL {

DepthBuffer::DepthBuffer() :
    Ego::DepthBuffer()
{}

DepthBuffer::~DepthBuffer()
{}

void DepthBuffer::clear() {
    glClear(GL_DEPTH_BUFFER_BIT);
    Utilities::isError();
}

void DepthBuffer::setClearValue(const float& value) {
    glClearDepth(value);
    Utilities::isError();
}

} // namespace OpenGL
} // namespace Ego

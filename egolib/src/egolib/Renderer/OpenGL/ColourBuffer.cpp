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

/// @file egolib/Renderer/OpenGL/ColourBuffer.cpp
/// @brief Implementation of a colour buffer facade for OpenGL 2.1.
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/ColourBuffer.hpp"
#include "egolib/Renderer/OpenGL/Utilities.hpp"

namespace Ego {
namespace OpenGL {

ColourBuffer::ColourBuffer() :
    Ego::ColourBuffer(), colourDepth(Utilities2::getColourBufferColourDepth())
{}

ColourBuffer::~ColourBuffer() {}

void ColourBuffer::clear() {
    glClear(GL_COLOR_BUFFER_BIT);
    Utilities2::isError();
}

void ColourBuffer::setClearValue(const Colour4f& value) {
    glClearColor(value.get_r(), value.get_g(), value.get_b(), value.get_a());
    Utilities2::isError();
}

const ColourDepth& ColourBuffer::getColourDepth() {
    return colourDepth;
}

} // namespace OpenGL
} // namespace Ego

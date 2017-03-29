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

/// @file egolib/Renderer/OpenGL/AccumulationBuffer.cpp
/// @brief Implementation of an Accumulation buffer facade for OpenGL 2.1.
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/AccumulationBuffer.hpp"

namespace Ego {
namespace OpenGL {

AccumulationBuffer::AccumulationBuffer() :
    Ego::AccumulationBuffer(), colourDepth(Utilities::getAccumulationBufferColourDepth())
{}

AccumulationBuffer::~AccumulationBuffer()
{}

void AccumulationBuffer::clear() {
    glClear(GL_ACCUM_BUFFER_BIT);
    Utilities::isError();
}

void AccumulationBuffer::setClearValue(const Colour4f& value) {
    glClearAccum(value.getRed(), value.getGreen(), value.getBlue(), value.getAlpha());
    Utilities::isError();
}

const ColourDepth& AccumulationBuffer::getColourDepth() {
    return colourDepth;
}

} // namespace OpenGL
} // namespace Ego

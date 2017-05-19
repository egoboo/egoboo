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

/// @file egolib/Graphics/VertexBuffer.cpp
/// @brief Vertex buffers.
/// @author Michael Heilmann

#include "egolib/Graphics/VertexBuffer.hpp"

namespace Ego {

VertexBuffer::VertexBuffer(size_t numberOfVertices, size_t vertexSize) :
    Buffer(numberOfVertices * vertexSize), numberOfVertices(numberOfVertices), vertexSize(vertexSize),
    vertices(new char[numberOfVertices * vertexSize])
{}

VertexBuffer::~VertexBuffer() {
    delete[] vertices;
	vertices = nullptr;
}

size_t VertexBuffer::getNumberOfVertices() const {
    return numberOfVertices;
}

size_t VertexBuffer::getVertexSize() const {
    return vertexSize;
}

void *VertexBuffer::lock() {
    return vertices;
}

void VertexBuffer::unlock() {
    /* Intentionally empty for the moment. */
}

} // namespace Ego

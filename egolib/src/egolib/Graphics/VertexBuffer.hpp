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

/// @file egolib/Graphics/VertexBuffer.hpp
/// @brief Vertex buffers.
/// @author Michael Heilmann

#pragma once

#include "egolib/Graphics/Buffer.hpp"
#include "egolib/Graphics/VertexFormat.hpp"

namespace Ego {

/// @brief A vertex buffer.
class VertexBuffer : public Ego::Buffer {
private:
    /// @brief The number of vertices.
    size_t numberOfVertices;

    /// @brief The vertex descriptor.
    VertexDescriptor vertexDescriptor;

    /// @brief The vertices.
    char *vertices;

public:
    /// @brief Construct this vertex buffer.
    /// @param numberOfVertices the number of vertices
    /// @param vertexDescriptor the vertex descriptor
    VertexBuffer(size_t numberOfVertices, const VertexDescriptor& vertexDescriptor);

    /// @brief Destruct this vertex buffer.
    virtual ~VertexBuffer();

    /// @brief Get the number of vertices of this vertex buffer.
    /// @return the number of vertices of this vertex buffer
    size_t getNumberOfVertices() const;
    
    /// @brief Get the vertex descriptor of this vertex buffer.
    /// @return the vertex descriptor of this vertex buffer
    const VertexDescriptor& getVertexDescriptor() const;
    
    /** @copydoc Buffer::lock */
    void *lock() override;
     
    /** @copydoc Buffer::unlock */
    void unlock() override;

}; // class VertexBuffer

/// @brief Provides convenient RAII-style mechanism for locking/unlocking a vertex buffer.
using VertexBufferScopedLock = BufferScopedLock;

} // namespace Ego

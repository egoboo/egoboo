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

#include "egolib/Graphics/VertexFormat.hpp"
#include "egolib/Core/LockFailedException.hpp"

namespace Ego
{

/**
 * @brief
 *  A vertex buffer.
 * @todo
 *  A not really elaborated solution for rendering vertices without resorting
 *  to deprecated OpenGL functions.
 * @author
 *  Michael Heilmann
 */
class VertexBuffer : Id::NonCopyable
{

protected:

    /**
     * @brief
     *  The number of vertices.
     */
    size_t _numberOfVertices;

    /**
     * @brief
     *  The vertex format descriptor.
     */
    VertexFormatDescriptor _vertexFormatDescriptor;

    /**
     * @brief
     *  The vertices.
     */
    char *_vertices;

public:

    /** 
     * @brief
     *  Construct this vertex buffer.
     * @param numberOfVertices
     *  the number of vertices
     * @param vertexFormatDescriptor
     *  the vertex format descriptor
     */
    VertexBuffer(size_t numberOfVertices, const VertexFormatDescriptor& vertexFormatDescriptor);

    /**
     * @brief
     *  Destruct this vertex buffer.
     */
    virtual ~VertexBuffer();

    /**
     * @brief
     *  Get the number of vertices of this vertex buffer.
     * @return
     *  the number of vertices of this vertex buffer
     */
    size_t getNumberOfVertices() const;
    
    /**
     * @brief
     *  Get the vertex descriptor of this vertex buffer.
     * @return
     *  the vertex descriptor of this vertex buffer
     */
    const VertexFormatDescriptor& getVertexFormatDescriptor() const;
    
    /**
     * @brief
     *  Lock this vertex buffer.
     * @return
     *  a pointer to the vertex data
	 * @throw Ego::Core::LockFailedException
	 *  if locking the vertex buffer failed
     */
    void *lock();
     
    /**
     * @brief
     *  Unlock this vertex buffer.
	 * @remark
	 *  If the vertex buffer is not locked, a call to this method is a no-op.
     */
    void unlock();

};

/**
 * @brief
 *	Provides convenient RAII-style mechanism for locking/unlocking a vertex buffer.
 * @author
 *  Michael Heilmann
 */
struct VertexBufferScopedLock
{
private:
	/**
	 * @brief
	 *  A pointer to the backing memory of the vertex buffer.
	 */
	void *_pointer;
	/**
	 * @brief
	 *  A pointer to the vertex buffer.
	 */
	VertexBuffer *_vertexBuffer;
public:
	/**
	 * @brief
	 *  Construct this vertex buffer scoped lock, locking the vertex buffer.
	 * @param vertexBuffer
	 *  the vertex buffer
	 * @throw Ego::Core::LockFailedException
	 *	if the vertex buffer can not be locked
	 * @todo
	 *  Use an other exception type than std::runtime_error.
	 */
	VertexBufferScopedLock(VertexBuffer& vertexBuffer)
		: _vertexBuffer(&vertexBuffer) {
		_pointer = _vertexBuffer->lock();
	}

	/**
	 * @brief
	 *  Destruct his vertex buffer scoped lock, unlocking the vertex buffer.
	 */
	virtual ~VertexBufferScopedLock() {
		_vertexBuffer->unlock();
	}

	/**
	 * @brief
	 *  Get a pointer to the backing memory of the vertex buffer.
	 * @return
	 *  a pointer to the backing memory of the vertex buffer
	 */
	template <typename _Type>
	_Type *get() {
		return static_cast<_Type *>(_pointer);
	}

};

} // namespace Ego

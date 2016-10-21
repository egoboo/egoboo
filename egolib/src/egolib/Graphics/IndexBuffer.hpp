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

/// @file egolib/Graphics/IndexBuffer.hpp
/// @brief Index buffers.
/// @author Michael Heilmann

#pragma once

#include "egolib/Graphics/Buffer.hpp"
#include "egolib/Graphics/IndexDescriptor.hpp"

namespace Ego {
    
/// @brief An index buffer.
class IndexBuffer : public Ego::Buffer {
private:
    /// @brief The number of indices.
    size_t numberOfIndices;

    /// @brief The index descriptor.
    IndexDescriptor indexDescriptor;

    /// @brief The indices.
    char *indices;

public:
    /// @brief Construct this index buffer.
    /// @param numberOfIndices the number of indices
    /// @param indexFormatDescriptor the index format descriptor
    IndexBuffer(size_t numberOfIndices, const IndexDescriptor& indexDescriptor);

    /// @brief Destruct this index buffer.
    virtual ~IndexBuffer();
    
    /// @brief Get the number of indices of this index buffer.
    /// @return the number of indices of this index buffer
    size_t getNumberOfIndices() const;

    /// @brief Get the index descriptor of this index buffer.
    /// @return the index descriptor of this index buffer
    const IndexDescriptor& getIndexDescriptor() const;
    
    /// @copydoc Buffer::lock
    void *lock() override;
     
    /// @copydoc Buffer::unlock
    void unlock() override;
    
}; // class IndexBuffer

} // namespace Ego

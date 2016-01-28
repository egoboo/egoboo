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

#include "egolib/Graphics/IndexBuffer.hpp"

namespace Ego {
IndexBuffer::IndexBuffer(size_t numberOfIndices,
                         const IndexDescriptor& indexDescriptor) :
    Buffer(indexDescriptor.getIndexSize() * numberOfIndices),
    numberOfIndices(numberOfIndices), indexDescriptor(indexDescriptor),
    indices(new char[indexDescriptor.getIndexSize() * numberOfIndices]) {}

IndexBuffer::~IndexBuffer() {
    delete[] indices;
    indices = nullptr;
}

size_t IndexBuffer::getNumberOfIndices() const {
    return numberOfIndices;
}

const IndexDescriptor& IndexBuffer::getIndexDescriptor() const {
    return indexDescriptor;
}

void *IndexBuffer::lock() {
    return indices;
}

void IndexBuffer::unlock() {
    /* Intentionally empty for the moment. */
}


} // namespace Ego

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

/// @file   egolib/Graphics/VertexDescriptor.cpp
/// @brief  Descriptions of vertices.
/// @author Michael Heilmann

#include "egolib/Graphics/VertexDescriptor.hpp"

namespace Ego {

VertexDescriptor::VertexDescriptor(std::initializer_list<VertexElementDescriptor> vertexElementDescriptors)
    : vertexElementDescriptors(vertexElementDescriptors), vertexSize(0) {
    // (1) Compute the vertex size.
    for (const auto& vertexElementDescriptor : vertexElementDescriptors) {
        vertexSize = std::max(vertexSize, vertexElementDescriptor.getOffset() + vertexElementDescriptor.getSize());
    }
}

VertexDescriptor::VertexDescriptor(const VertexDescriptor& other)
    : vertexElementDescriptors(other.vertexElementDescriptors), vertexSize(other.vertexSize) {
}

const VertexDescriptor& VertexDescriptor::operator=(const VertexDescriptor& other) {
    vertexElementDescriptors = other.vertexElementDescriptors;
    vertexSize = other.vertexSize;
    return *this;
}
    
} // namespace Ego
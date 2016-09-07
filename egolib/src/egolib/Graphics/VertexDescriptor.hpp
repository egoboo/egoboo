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

/// @file   egolib/Graphics/VertexDescriptor.hpp
/// @brief  Descriptions of vertices.
/// @author Michael Heilmann

#pragma once

#include "egolib/Graphics/VertexElementDescriptor.hpp"

namespace Ego {

/**
 * @brief
 *  The descriptor of a vertex.
 */
class VertexDescriptor {
private:
    /**
     * @brief
     *  The list of vertex element descriptors.
     */
    std::vector<VertexElementDescriptor> vertexElementDescriptors;

    /**
     * @brief
     *  The size, in Bytes, of a vertex.
     */
    size_t vertexSize;

public:
    /**
     * @brief
     *  Construct this vertex descriptor.
     * @param vertexElementDescriptors
     *  the vertex element descriptors
     */
    VertexDescriptor(std::initializer_list<VertexElementDescriptor> vertexElementDescriptors);

    /**
     * @brief
     *  Copy-construct this vertex descriptor with the values of another vertex descriptor.
     * @param other
     *  the other vertex descriptor
     */
    VertexDescriptor(const VertexDescriptor& other);

    /**
     * @brief
     *  Assign this vertex descriptor with the values of another vertex descriptor.
     * @param other
     *  the other vertex descriptor
     * @return
     *  this vertex descriptor
     */
    const VertexDescriptor& operator=(const VertexDescriptor& other);


public:
    /** @brief The type of an iterator of the vertex element descriptors. */
    using const_iterator = std::vector<VertexElementDescriptor>::const_iterator;
    /** @brief The type of an iterator over the vertex element descriptors. */
    using iterator = std::vector<VertexElementDescriptor>::iterator;

    /**@{*/
    /**
     * @brief Return an iterator to the beginning.
     * @return an iterator to the beginning
     */
    iterator begin() { return vertexElementDescriptors.begin(); }
    const_iterator begin() const { return vertexElementDescriptors.begin(); }
    const_iterator cbegin() const { return vertexElementDescriptors.cbegin(); }
    /**@}*/

    /**@{*/
    /**
    * @brief Return an iterator to the end.
    * @return an iterator to the end
    */
    iterator end() { return vertexElementDescriptors.end(); }
    const_iterator end() const { return vertexElementDescriptors.end(); }
    const_iterator cend() const { return vertexElementDescriptors.cend(); }
    /**@}*/


public:
    /**
     * @brief
     *  Get the size, in Bytes, of a vertex.
     * @return
     *  the size, in Bytes, of a vertex.
     */
    size_t getVertexSize() const {
        return vertexSize;
    }

public:
    /**
     * @brief Compare this vertex descriptor with another vertex descriptor for equality.
     * @param other the other index descriptor
     * @return @a true if this vertex descriptor is equal to the other vertex descriptor,
     *         @a false otherwise
     */
    bool operator==(const VertexDescriptor&) const;
    /**
     * @brief Compare this vertex descriptor with another index descriptor for inequality.
     * @param other the other vertex descriptor
     * @return @a true if this vertex descriptor is not equal to the other vertex descriptor,
     *         @a false otherwise
     */
    bool operator!=(const VertexDescriptor&) const;

}; // class VertexDescriptor

} // namespace Ego

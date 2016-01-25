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

/// @file   egolib/Graphics/VertexFormat.hpp
/// @brief  Vertex formats and vertex format descriptors.
/// @author Michael Heilmann

#pragma once

#include "egolib/Graphics/VertexElementDescriptor.hpp"

namespace Ego {

/**
 * @brief
 *  Enumeration of canonical identifiers for a vertex formats.
 * @author
 *  Michael Heilmann
 */
enum class VertexFormat {
    /**
     * @brief
     *  Two floats for the position component.
     */
    P2F,

	/**
	 * @brief
	 *	Two floats for the position component,
	 *	two floats for the texture component.
	 */
	P2FT2F,

    /**
     * @brief
     *  Three floats for the position component.
     */
    P3F,

    /**
     * @brief
     *  Three floats for the position component and
     *  four floats for colour component.
     */
    P3FC4F,

    /**
     * @brief
     *  Three floats for the position component and
     *  two floats for the texture component.
     */
    P3FT2F,

    /**
     * @brief
     *  Three floats for the position component,
     *  four floats for the colour component, and
     *  three floats for the normal component.
     */
    P3FC4FN3F,

    /**
     * @brief
     *  Three floats for the position component,
     *  four floats for the colour component, and
     *  two floats for the texture component.
     */
    P3FC4FT2F,

    /**
     * @brief
     *  Three floats for the position component,
     *  four floats for the colour component,
     *  two floats for the texture component, and
     *  three floats for the normal component.
     */
     P3FC4FT2FN3F,

}; // enum class VertexFormat

/**
 * @brief
 *  A vertex format descriptor suitable to describe the vertex formats
 *  as specified by the Ego::VertexFormat enumeration.
 * @author
 *  Michael Heilmann
 */
class VertexFormatDescriptor {
private:
    /**
     * @brief
     *  The list of vertex elements.
     */
    std::vector<VertexElement> vertexElements;

    /**
     * @brief
     *  The size, in Bytes, of a vertex.
     */
    size_t vertexSize;
    
public:
    /**
     * @brief
     *  Construct this vertex format descriptor.
     * @param vertexElements
     *  the vertex elements
     */
    VertexFormatDescriptor(std::initializer_list<VertexElement> vertexElements);

    /**
     * @brief
     *  Copy-construct this vertex format descriptor.
     * @param other
     *  the other vertex format descriptor
     */
    VertexFormatDescriptor(const VertexFormatDescriptor& other);

    /**
     * @brief
     *  Assign this vertex format descriptor.
     * @param other
     *  the other vertex format descriptor
     * @return
     *  this vertex format descriptor
     */
    const VertexFormatDescriptor& operator=(const VertexFormatDescriptor& other);
    
   
public:
    /** @brief The type of an iterator of the vertex element descriptors. */
    typedef std::vector<VertexElement>::const_iterator const_iterator;
    /** @brief The type of an iterator over the vertex element descriptors. */
    typedef std::vector<VertexElement>::iterator iterator;

    /**@{*/
    /**
     * @brief Return an iterator to the beginning.
     * @return an iterator to the beginning
     */
    iterator begin() { return vertexElements.begin(); }
    const_iterator begin() const { return vertexElements.begin(); }
    const_iterator cbegin() const { return vertexElements.cbegin(); }
    /**@}*/

    /**@{*/
    /**
    * @brief Return an iterator to the end.
    * @return an iterator to the end
    */
    iterator end() { return vertexElements.end(); }
    const_iterator end() const { return vertexElements.end(); }
    const_iterator cend() const { return vertexElements.cend(); }
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

    template <VertexFormat _VertexFormat>
    static const VertexFormatDescriptor& get();

    /**
     * @brief
     *  Get the vertex format descriptor for a vertex format.
     * @param vertexFormat
     *  the vertex format
     * @return
     *  the vertex format descriptor for the vertex format
     */
    static const VertexFormatDescriptor& get(VertexFormat vertexFormat);

}; // class VertexFormatDescriptor

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P2F>();

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P2FT2F>();

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3F>();

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FT2F>();

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4F>();

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FN3F>();

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FT2F>();

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FT2FN3F>();

} // namespace Ego

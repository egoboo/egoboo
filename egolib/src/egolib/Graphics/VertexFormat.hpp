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

#include "egolib/platform.h"

namespace Ego
{

/**
 * @brief
 *  Enumeration of canonical identifiers for a vertex formats.
 * @author
 *  Michael Heilmann
 */
enum class VertexFormat
{
    /**
     * @brief
     *  Three floats for the position component and four floats for colour component.
     */
    P3FC4F,
    /**
     * @brief
     *  Three floats for the position component, four floats for the colour component, and two floats for the texture component.
     */
    P3FC4FT2F,
};

/**
 * @brief
 *  A vertex format descriptor suitable to describe the vertex formats
 *  as specified by the Ego::VertexFormat enumeration.
 * @author
 *  Michael Heilmann
 */
class VertexFormatDescriptor
{

private:

    /**
     * @brief
     *  The vertex format.
     */
    VertexFormat _vertexFormat;

    /**
     * @brief
     *  The size, in Bytes, of a vertex.
     */
    size_t _vertexSize;
    
    /**
     * @brief
     *  Construct this vertex format descriptor.
     * @param vertexFormat
     *  the vertex format
     * @param vertexSize
     *  the size, in Bytes, of a vertex
     */
    VertexFormatDescriptor(VertexFormat vertexFormat, size_t vertexSize) :
        _vertexFormat(vertexFormat), _vertexSize(vertexSize)
    {}
    
    
public:
    
    /**
     * @brief
     *  Get the size, in Bytes, of a vertex.
     * @return
     *  the size, in Bytes, of a vertex.
     */
    size_t getVertexSize() const
    {
        return _vertexSize;
    }
    
    /**
     * @brief
     *  Get the vertex format.
     * @return
     *  the vertex format
     */
    VertexFormat getVertexFormat() const
    {
        return _vertexFormat;
    }

};

} // namespace Ego

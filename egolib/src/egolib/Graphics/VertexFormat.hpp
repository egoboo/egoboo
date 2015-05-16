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
     *  Two floats for the position component.
     */
    P2F,
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
     *  The size, in Bytes, of the position component.
     */
    size_t _positionSize;

    /**
     * @brief
     *  The size, in Bytes, of the colour component
     */
    size_t _colourSize;

    /**
     * @brief
     *  The size, in Bytes, of the normal component.
     */
    size_t _normalSize;

    /**
     * @brief
     *  The size, in Bytes, of the texture component.
     */
    size_t _textureSize;
    
    /**
     * @brief
     *  Construct this vertex format descriptor.
     * @param vertexFormat
     *  the vertex format
     * @param vertexSize
     *  the size, in Bytes, of a vertex
     * @param positionSize, colourSize, textureSize, normalSize
     *  the sizes, in Bytes, of the position, colour, texture, and normal components
     */
    VertexFormatDescriptor(VertexFormat vertexFormat, size_t vertexSize, size_t positionSize, size_t colourSize,
                           size_t textureSize, size_t normalSize) :
        _vertexFormat(vertexFormat), 
        _vertexSize(vertexSize),
        _positionSize(positionSize), 
        _colourSize(colourSize), 
        _normalSize(normalSize),
        _textureSize(textureSize)
    {}
    
    
public:

    /**
     * @brief
     *  Get the size, in Bytes, of the position component.
     * @return
     *  the size, in Bytes, of the position component.
     */
    size_t getPositionSize() const
    {
        return _positionSize;
    }

    /**
     * @brief
     *  Get the size, in Bytes, of the colour component.
     * @return
     *  the size, in Bytes, of the colour component
     */
    size_t getColorSize() const
    {
        return _colourSize;
    }

    /**
     * @brief
     *  Get the size, in Bytes, of the normal component.
     * @return
     *  the size, in Bytes, of the normal component
     */
    size_t getNormalSize() const
    {
        return _normalSize;
    }

    /**
     * @brief
     *  Get the size, in Bytes, of the texture component.
     * @return
     *  the size, in Bytes, of the texture component
     */
    size_t getTextureSize() const
    {
        return _textureSize;
    }
    
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

};

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P2F>();

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

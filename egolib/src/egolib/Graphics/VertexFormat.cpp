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

/// @file   egolib/Graphics/VertexFormat.cpp
/// @brief  Vertex formats and vertex format descriptors.
/// @author Michael Heilmann

#include "egolib/Graphics/VertexFormat.hpp"

namespace Ego
{

const VertexFormatDescriptor& VertexFormatDescriptor::get(VertexFormat vertexFormat)
{
    switch (vertexFormat)
    {
        case VertexFormat::P2F:          return get<VertexFormat::P2F>();
        case VertexFormat::P3F:          return get<VertexFormat::P3F>();
        case VertexFormat::P3FT2F:       return get<VertexFormat::P3FT2F>();
        case VertexFormat::P3FC4F:       return get<VertexFormat::P3FC4F>();
        case VertexFormat::P3FC4FN3F:    return get<VertexFormat::P3FC4FN3F>();
        case VertexFormat::P3FC4FT2F:    return get<VertexFormat::P3FC4FT2F>();
        case VertexFormat::P3FC4FT2FN3F: return get<VertexFormat::P3FC4FT2FN3F>();
        default:
			throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
    };
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P2F>()
{
    static const VertexFormatDescriptor INSTANCE
        (
        VertexFormat::P2F,
        sizeof(float) * 2,
        sizeof(float) * 2, // position
        0,                 // colour
        0,                 // texture
        0                  // normal
        );
    return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3F>()
{
    static const VertexFormatDescriptor INSTANCE
        (
        VertexFormat::P3F,
        sizeof(float) * 3,
        sizeof(float) * 3, // position
        0 ,                // colour
        0,                 // texture
        0                  // normal
        );
    return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4F>()
{
    static const VertexFormatDescriptor INSTANCE
        (
            VertexFormat::P3FC4F,
            sizeof(float) * 3 + sizeof(float) * 4,
            sizeof(float) * 3, // position
            sizeof(float) * 4, // colour
            0,                 // texture
            0                  // normal
        );
    return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FT2F>()
{
    static const VertexFormatDescriptor INSTANCE
        (
        VertexFormat::P3FT2F,
        sizeof(float) * 3 + sizeof(float) * 2,
        sizeof(float) * 3, // position
        0,                 // colour
        sizeof(float) * 2, // texture
        0                  // normal
        );
    return INSTANCE;
}


template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FN3F>()
{
    static const VertexFormatDescriptor INSTANCE
        (
            VertexFormat::P3FC4FN3F,
            sizeof(float) * 3 + sizeof(float) * 4 + sizeof(float) * 3,
            sizeof(float) * 3, // position
            sizeof(float) * 4, // colour
            0,                 // texture
            sizeof(float) * 3  // normal
        );
    return INSTANCE;
}


template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FT2F>()
{
    static const VertexFormatDescriptor INSTANCE
        (
            VertexFormat::P3FC4FT2F,
            sizeof(float) * 3 + sizeof(float) * 4 + sizeof(float) * 2,
            sizeof(float) * 3, // position
            sizeof(float) * 4, // colour
            sizeof(float) * 2, // texture
            0                  // normal
        );
    return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FT2FN3F>()
{
    static const VertexFormatDescriptor INSTANCE
        (
            VertexFormat::P3FC4FT2FN3F,
            sizeof(float) * 3 + sizeof(float) * 4 + sizeof(float) * 2 + sizeof(float) * 3,
            sizeof(float) * 3, // position
            sizeof(float) * 4, // colour
            sizeof(float) * 2, // texture
            sizeof(float) * 3  // normal
        );
    return INSTANCE;
}


} // namespace Ego

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
		case VertexFormat::P2FT2F:       return get<VertexFormat::P2FT2F>();
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
    static const VertexElementDescriptor position(0, VertexElementSyntax::F2, VertexElementSemantics::Position);
    static const size_t size = position.getOffset() + position.getSize();
    static const VertexFormatDescriptor INSTANCE
        (
			VertexFormat::P2F,
			size,
			position.getSize(), // position
			0,                  // colour
			0,                  // texture
			0                   // normal
        );
    return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P2FT2F>()
{
    static const VertexElementDescriptor position(0, VertexElementSyntax::F2, VertexElementSemantics::Position);
    static const VertexElementDescriptor texture(position.getOffset() + position.getSize(), VertexElementSyntax::F2, VertexElementSemantics::Texture);
    static const size_t size = std::max({position.getOffset() + position.getSize(),
                                         texture.getOffset() + texture.getSize()});
	static const VertexFormatDescriptor INSTANCE
		(
			VertexFormat::P2FT2F,
			size,
			position.getSize(), // position
			0,                  // colour
			texture.getSize(),  // texture
			0                   // normal
		);
	return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3F>()
{
    static const VertexElementDescriptor position(0, VertexElementSyntax::F3, VertexElementSemantics::Position);
    static const size_t size = std::max({position.getOffset() + position.getSize()});
    static const VertexFormatDescriptor INSTANCE
        (
			VertexFormat::P3F,
			size,
            position.getSize(), // position
			0 ,                 // colour
			0,                  // texture
			0                   // normal
        );
    return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4F>()
{
    static const VertexElementDescriptor position(0, VertexElementSyntax::F3, VertexElementSemantics::Position);
    static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementSyntax::F4, VertexElementSemantics::Colour);
    static const size_t size = std::max({position.getOffset() + position.getSize(),
                                         colour.getOffset() + colour.getSize()});
    static const VertexFormatDescriptor INSTANCE
        (
            VertexFormat::P3FC4F,
            size,
            position.getSize(), // position
            colour.getSize(),   // colour
            0,                  // texture
            0                   // normal
        );
    return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FT2F>()
{
    static const VertexElementDescriptor position(0, VertexElementSyntax::F3, VertexElementSemantics::Position);
    static const VertexElementDescriptor texture(position.getOffset() + position.getSize(), VertexElementSyntax::F2, VertexElementSemantics::Texture);
    static const size_t size = std::max({position.getOffset() + position.getSize(),
                                         texture.getOffset() + texture.getSize()});
    static const VertexFormatDescriptor INSTANCE
        (
			VertexFormat::P3FT2F,
			size,
			position.getSize(), // position
			0,                  // colour
			texture.getSize(),  // texture
			0                   // normal
        );
    return INSTANCE;
}


template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FN3F>()
{
    static const VertexElementDescriptor position(0, VertexElementSyntax::F3, VertexElementSemantics::Position);
    static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementSyntax::F4, VertexElementSemantics::Colour);
    static const VertexElementDescriptor normal(colour.getOffset() + colour.getSize(), VertexElementSyntax::F3, VertexElementSemantics::Normal);
    static const size_t size = std::max({position.getOffset() + position.getSize(),
                                         colour.getOffset() + colour.getSize(),
                                         normal.getOffset() + normal.getSize()});
    static const VertexFormatDescriptor INSTANCE
        (
            VertexFormat::P3FC4FN3F,
            size,
            position.getSize(), // position
            colour.getSize(),   // colour
            0,                  // texture
            normal.getSize()    // normal
        );
    return INSTANCE;
}


template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FT2F>()
{
    static const VertexElementDescriptor position(0, VertexElementSyntax::F3, VertexElementSemantics::Position);
    static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementSyntax::F4, VertexElementSemantics::Colour);
    static const VertexElementDescriptor texture(colour.getOffset() + colour.getSize(), VertexElementSyntax::F2, VertexElementSemantics::Texture);
    static const size_t size = std::max({position.getOffset() + position.getSize(),
                                         colour.getOffset() + colour.getSize(),
                                         texture.getOffset() + texture.getSize()});
    static const VertexFormatDescriptor INSTANCE
        (
            VertexFormat::P3FC4FT2F,
            size,
            position.getSize(), // position
            colour.getSize(),   // colour
            texture.getSize(),  // texture
            0                   // normal
        );
    return INSTANCE;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FT2FN3F>()
{
    static const VertexElementDescriptor position(0, VertexElementSyntax::F3, VertexElementSemantics::Position);
    static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementSyntax::F4, VertexElementSemantics::Colour);
    static const VertexElementDescriptor texture(colour.getOffset() + colour.getSize(), VertexElementSyntax::F2, VertexElementSemantics::Texture);
    static const VertexElementDescriptor normal(texture.getOffset() + texture.getSize(), VertexElementSyntax::F3, VertexElementSemantics::Normal);
    static const size_t size = std::max({position.getOffset() + position.getSize(),
                                         colour.getOffset() + colour.getSize(),
                                         texture.getOffset() + texture.getSize(),
                                         normal.getOffset() + normal.getOffset()});
    static const VertexFormatDescriptor INSTANCE
        (
            VertexFormat::P3FC4FT2FN3F,
            size,
            position.getSize(), // position
            colour.getSize(),   // colour
            texture.getSize(),  // texture
            normal.getSize()    // normal
        );
    return INSTANCE;
}

} // namespace Ego

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

namespace Ego {

const VertexDescriptor& GraphicsUtilities::get(VertexFormat vertexFormat) {
    switch (vertexFormat) {
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
const VertexDescriptor& GraphicsUtilities::get<VertexFormat::P2F>() {
    static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Position);
    static const VertexDescriptor descriptor({position});
    return descriptor;
}

template <>
const VertexDescriptor& GraphicsUtilities::get<VertexFormat::P2FT2F>()
{
    static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Position);
    static const VertexElementDescriptor texture(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
    static const VertexDescriptor descriptor({position, texture});
    return descriptor;
}

template <>
const VertexDescriptor& GraphicsUtilities::get<VertexFormat::P3F>()
{
    static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
    static const VertexDescriptor descriptor({position});
    return descriptor;
}

template <>
const VertexDescriptor& GraphicsUtilities::get<VertexFormat::P3FC4F>()
{
    static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
    static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F4, VertexElementDescriptor::Semantics::Colour);
    static const VertexDescriptor descriptor({position, colour});
    return descriptor;
}

template <>
const VertexDescriptor& GraphicsUtilities::get<VertexFormat::P3FT2F>()
{
    static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
    static const VertexElementDescriptor texture(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
    static const VertexDescriptor descriptor({position, texture});
    return descriptor;
}

template <>
const VertexDescriptor& GraphicsUtilities::get<VertexFormat::P3FC4FN3F>()
{
    static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
    static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F4, VertexElementDescriptor::Semantics::Colour);
    static const VertexElementDescriptor normal(colour.getOffset() + colour.getSize(), VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Normal);
    static const VertexDescriptor descriptor({position, colour, normal});
    return descriptor;
}

template <>
const VertexDescriptor& GraphicsUtilities::get<VertexFormat::P3FC4FT2F>()
{
    static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
    static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F4, VertexElementDescriptor::Semantics::Colour);
    static const VertexElementDescriptor texture(colour.getOffset() + colour.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
    static const VertexDescriptor descriptor({position, colour, texture});
    return descriptor;
}

template <>
const VertexDescriptor& GraphicsUtilities::get<VertexFormat::P3FC4FT2FN3F>()
{
    static const VertexElementDescriptor position(0, VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Position);
    static const VertexElementDescriptor colour(position.getOffset() + position.getSize(), VertexElementDescriptor::Syntax::F4, VertexElementDescriptor::Semantics::Colour);
    static const VertexElementDescriptor texture(colour.getOffset() + colour.getSize(), VertexElementDescriptor::Syntax::F2, VertexElementDescriptor::Semantics::Texture);
    static const VertexElementDescriptor normal(texture.getOffset() + texture.getSize(), VertexElementDescriptor::Syntax::F3, VertexElementDescriptor::Semantics::Normal);
    static const VertexDescriptor descriptor({position, colour, texture, normal});
    return descriptor;
}

} // namespace Ego

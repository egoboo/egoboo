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

VertexFormatDescriptor::VertexFormatDescriptor(std::initializer_list<VertexElement> vertexElements)
    : vertexElements(vertexElements), vertexSize(0) {
    // (1) Compute the vertex size.
    for (const auto& vertexElement : vertexElements) {
        vertexSize = std::max(vertexSize, vertexElement.getOffset() + vertexElement.getSize());
    }
}

VertexFormatDescriptor::VertexFormatDescriptor(const VertexFormatDescriptor& other)
    : vertexElements(other.vertexElements), vertexSize(other.vertexSize) {
}

const VertexFormatDescriptor& VertexFormatDescriptor::operator=(const VertexFormatDescriptor& other) {
    vertexElements = other.vertexElements;
    vertexSize = other.vertexSize;
    return *this;
}

const VertexFormatDescriptor& VertexFormatDescriptor::get(VertexFormat vertexFormat) {
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
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P2F>() {
    static const VertexElement position(0, VertexElement::Syntax::F2, VertexElement::Semantics::Position);
    static const VertexFormatDescriptor descriptor({position});
    return descriptor;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P2FT2F>()
{
    static const VertexElement position(0, VertexElement::Syntax::F2, VertexElement::Semantics::Position);
    static const VertexElement texture(position.getOffset() + position.getSize(), VertexElement::Syntax::F2, VertexElement::Semantics::Texture);
    static const VertexFormatDescriptor descriptor({position, texture});
    return descriptor;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3F>()
{
    static const VertexElement position(0, VertexElement::Syntax::F3, VertexElement::Semantics::Position);
    static const VertexFormatDescriptor descriptor({position});
    return descriptor;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4F>()
{
    static const VertexElement position(0, VertexElement::Syntax::F3, VertexElement::Semantics::Position);
    static const VertexElement colour(position.getOffset() + position.getSize(), VertexElement::Syntax::F4, VertexElement::Semantics::Colour);
    static const VertexFormatDescriptor descriptor({position, colour});
    return descriptor;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FT2F>()
{
    static const VertexElement position(0, VertexElement::Syntax::F3, VertexElement::Semantics::Position);
    static const VertexElement texture(position.getOffset() + position.getSize(), VertexElement::Syntax::F2, VertexElement::Semantics::Texture);
    static const VertexFormatDescriptor descriptor({position, texture});
    return descriptor;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FN3F>()
{
    static const VertexElement position(0, VertexElement::Syntax::F3, VertexElement::Semantics::Position);
    static const VertexElement colour(position.getOffset() + position.getSize(), VertexElement::Syntax::F4, VertexElement::Semantics::Colour);
    static const VertexElement normal(colour.getOffset() + colour.getSize(), VertexElement::Syntax::F3, VertexElement::Semantics::Normal);
    static const VertexFormatDescriptor descriptor({position, colour, normal});
    return descriptor;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FT2F>()
{
    static const VertexElement position(0, VertexElement::Syntax::F3, VertexElement::Semantics::Position);
    static const VertexElement colour(position.getOffset() + position.getSize(), VertexElement::Syntax::F4, VertexElement::Semantics::Colour);
    static const VertexElement texture(colour.getOffset() + colour.getSize(), VertexElement::Syntax::F2, VertexElement::Semantics::Texture);
    static const VertexFormatDescriptor descriptor({position, colour, texture});
    return descriptor;
}

template <>
const VertexFormatDescriptor& VertexFormatDescriptor::get<VertexFormat::P3FC4FT2FN3F>()
{
    static const VertexElement position(0, VertexElement::Syntax::F3, VertexElement::Semantics::Position);
    static const VertexElement colour(position.getOffset() + position.getSize(), VertexElement::Syntax::F4, VertexElement::Semantics::Colour);
    static const VertexElement texture(colour.getOffset() + colour.getSize(), VertexElement::Syntax::F2, VertexElement::Semantics::Texture);
    static const VertexElement normal(texture.getOffset() + texture.getSize(), VertexElement::Syntax::F3, VertexElement::Semantics::Normal);
    static const VertexFormatDescriptor descriptor({position, colour, texture, normal});
    return descriptor;
}

} // namespace Ego

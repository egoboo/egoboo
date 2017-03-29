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

/// @file egolib/Graphics/VertexElementDescriptor.cpp
/// @brief Descriptions of vertex elements.
/// @author Michael Heilmann

#include "egolib/Graphics/VertexElementDescriptor.hpp"

namespace Ego {

VertexElementDescriptor::VertexElementDescriptor(size_t offset, Syntax syntax, Semantics semantics)
    : offset(offset), syntax(syntax), semantics(semantics) {
}

VertexElementDescriptor::VertexElementDescriptor(const VertexElementDescriptor& other)
    : offset(other.offset), syntax(other.syntax), semantics(other.semantics) {
}

const VertexElementDescriptor& VertexElementDescriptor::operator=(const VertexElementDescriptor& other) {
    offset = other.offset;
    syntax = other.syntax;
    semantics = other.semantics;
    return *this;
}

bool VertexElementDescriptor::equal_to(const VertexElementDescriptor& other) const EGO_NOEXCEPT {
    return offset == other.offset
        && syntax == other.syntax
        && semantics == other.semantics;
}

size_t VertexElementDescriptor::getOffset() const {
    return offset;
}

VertexElementDescriptor::Syntax VertexElementDescriptor::getSyntax() const {
    return syntax;
}

VertexElementDescriptor::Semantics VertexElementDescriptor::getSemantics() const {
    return semantics;
}

size_t VertexElementDescriptor::getSize() const {
    switch (syntax) {
        case Syntax::F1:
            return sizeof(float) * 1;
        case Syntax::F2:
            return sizeof(float) * 2;
        case Syntax::F3:
            return sizeof(float) * 3;
        case Syntax::F4:
            return sizeof(float) * 4;
        default:
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
}

} // namespace Ego

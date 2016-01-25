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

VertexElement::VertexElement(size_t offset, Syntax syntax, Semantics semantics)
    : offset(offset), syntax(syntax), semantics(semantics) {
}

VertexElement::VertexElement(const VertexElement& other)
    : offset(other.offset), syntax(other.syntax), semantics(other.semantics) {
}

const VertexElement& VertexElement::operator=(const VertexElement& other) {
    offset = other.offset;
    syntax = other.syntax;
    semantics = other.semantics;
    return *this;
}

bool VertexElement::operator==(const VertexElement& other) const {
    return offset == other.offset
        && syntax == other.syntax
        && semantics == other.semantics;
}

bool VertexElement::operator!=(const VertexElement& other) const {
    return offset != other.offset
        || syntax != other.syntax
        || semantics != other.semantics;
}

size_t VertexElement::getOffset() const {
    return offset;
}

VertexElement::Syntax VertexElement::getSyntax() const {
    return syntax;
}

VertexElement::Semantics VertexElement::getSemantics() const {
    return semantics;
}

size_t VertexElement::getSize() const {
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

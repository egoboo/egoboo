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

/// @file   egolib/Graphics/IndexDescriptor.cpp
/// @brief  Descriptions of indices.
/// @author Michael Heilmann

#include "egolib/Graphics/IndexDescriptor.hpp"

namespace Ego {

IndexDescriptor::IndexDescriptor(IndexDescriptor::Syntax syntax) :
    syntax(syntax), indexSize(0) {
    // Compute the index size.
    switch (syntax) {
        case Syntax::U16:
            indexSize = 16 / 8;
            break;
        case Syntax::U32:
            indexSize = 32 / 8;
            break;
        default:
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
}

IndexDescriptor::IndexDescriptor(const IndexDescriptor& other) noexcept : syntax(other.syntax) {
}

const IndexDescriptor& IndexDescriptor::operator=(const IndexDescriptor& other) noexcept {
    syntax = other.syntax;
    return *this;
}

bool IndexDescriptor::operator==(const IndexDescriptor& other) const noexcept {
    return syntax == other.syntax;
}

bool IndexDescriptor::operator!=(const IndexDescriptor& other) const noexcept {
    return syntax != other.syntax;
}

} // namespace Ego

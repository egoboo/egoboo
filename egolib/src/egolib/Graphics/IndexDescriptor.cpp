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

/// @file egolib/Graphics/IndexDescriptor.cpp
/// @brief Descriptors of indices.
/// @author Michael Heilmann

#include "egolib/Graphics/IndexDescriptor.hpp"

namespace Ego {

IndexDescriptor::IndexDescriptor(IndexDescriptor::Syntax syntax) :
    syntax(syntax) {
}

IndexDescriptor::IndexDescriptor(const IndexDescriptor& other) noexcept : syntax(other.syntax) {
}

IndexDescriptor::Syntax IndexDescriptor::getSyntax() const {
    return syntax;
}

size_t IndexDescriptor::getIndexSize() const {
    switch (syntax) {
        case Syntax::U16:
            return sizeof(uint16_t); // 16 / 8
        case Syntax::U32:
            return sizeof(uint32_t); // 32 / 8
        case Syntax::U8:
            return sizeof(uint8_t);
    }
    throw id::unhandled_switch_case_error(__FILE__, __LINE__);
}

const IndexDescriptor& IndexDescriptor::operator=(const IndexDescriptor& other) noexcept {
    syntax = other.syntax;
    return *this;
}

bool IndexDescriptor::equal_to(const IndexDescriptor& other) const noexcept {
    return syntax == other.syntax;
}

} // namespace Ego

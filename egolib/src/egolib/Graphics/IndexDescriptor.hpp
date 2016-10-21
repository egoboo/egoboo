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

/// @file egolib/Graphics/IndexDescriptor.hpp
/// @brief Descriptors of indices.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {

/// @brief The descriptor of an index.
class IndexDescriptor {
public:
    /// @brief An enum class of the syntactic forms of indices.
    enum class Syntax {

        /**
        * @brief
        *  Unsigned 16-Bit indices.
        */
        U16,

        /**
        * @brief
        *  Unsigned 32-Bit indices.
        */
        U32,

    }; // enum class Syntax

private:
    /// @brief The syntax of this index descriptor.
    Syntax syntax;
    
public:
    /// @brief Construct this index descriptor.
    /// @param syntax the synax
    IndexDescriptor(Syntax syntax);

    /// @brief Copy-construct this index descriptor with the values of another index descriptor.
    /// @param other the other index descriptor
    IndexDescriptor(const IndexDescriptor& other) noexcept;

    /// @brief Assign this index descriptor with the values of another index descriptor.
    /// @param other the other index descriptor
    /// @return this index descriptor
    const IndexDescriptor& operator=(const IndexDescriptor& other) noexcept;

public:
    /// @brief Get the size, in Bytes, of an index.
    /// @return the size, in Bytes, of an index
    size_t getIndexSize() const {

        // Compute the index size.
        switch (syntax) {
            case Syntax::U16:
                return sizeof(uint16_t); //16 / 8
            case Syntax::U32:
                return sizeof(uint32_t); //32 / 8
        }

        throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }

public:
    /// @brief Compare this index descriptor with another index descriptor for equality.
    /// @param other the other index descriptor
    /// @return @a true if this index descriptor is equal to the other index descriptor,
    ///         @a false otherwise
    bool operator==(const IndexDescriptor&) const noexcept;

    /// @brief Compare this index descriptor with another index descriptor for inequality.
    /// @param other the other index descriptor
    /// @return @a true if this index descriptor is not equal to the other index descriptor,
    ///         @a false otherwise
    bool operator!=(const IndexDescriptor&) const noexcept;

}; // class IndexDescriptor

} // namespace Ego


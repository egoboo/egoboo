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

/// @file egolib/Graphics/VertexElementDescriptor.hpp
/// @brief Descriptions of vertex elements.
/// @author Michael Heilmann

#pragma once

#include "egolib/platform.h"

namespace Ego {

/// @brief The description of a vertex element.
class VertexElementDescriptor : public id::equal_to_expr<VertexElementDescriptor> {
public:
    /// @brief An enum class of the syntactic forms of vertex elements.
    enum class Syntax {
        /// @brief One <tt>float</tt> value.
        F1,
        /// @brief Two <tt>float</tt> values.
        F2,
        /// @brief Three <tt>float</tt> values.
        F3,
        /// @brief Four <tt>float</tt> values.
        F4,
    }; // enum class Syntax

    /// @brief An enum class of the semantic forms of vertex elements.
    enum class Semantics {
        /// None.
        None,
        /// Position.
        Position,
        /// Colour.
        Colour,
        /// Normal.
        Normal,
        /// Texture.
        Texture,
    }; // enum class Semantics
  
private:
    /// @brief The offset, in Bytes, of the vertex element from the beginning of the vertex.
    size_t offset;

    /// @brief The syntax of the vertex element.
    Syntax syntax;

    /// @brief The semantics of the vertex element.
    Semantics semantics;

    /// @brief The source.
    size_t source;

public:
    /// @brief Construct this vertex element descriptor.
    /// @param offset the offset, in Bytes, of the vertex element from the beginning of the vertex
    /// @param syntax the syntax of the vertex element
    /// @param semantics the semantics of the vertex element
    VertexElementDescriptor(size_t offset, Syntax syntax, Semantics semantics);

    /// @brief Copy-construct this vertex element descriptor from another vertex element descriptor.
    /// @param other the other vertex element descriptor
    VertexElementDescriptor(const VertexElementDescriptor& other);

    /// @brief Assign this vertex element descriptor from another vertex element descriptor.
    /// @param other the other vertex element descriptor
    /// @return this vertex element descriptor
    const VertexElementDescriptor& operator=(const VertexElementDescriptor& other);
    
public:
	// CRTP
    bool equal_to(const VertexElementDescriptor& other) const EGO_NOEXCEPT;
    
public:
    /// @brief Get the offset, in Bytes, of the vertex element from the beginning of the vertex.
    /// @return the offset, in Bytes, of the vertex element from the beginning of the vertex
    size_t getOffset() const;

    /// @brief Get the syntax of the vertex element.
    /// @return the syntax of this vertex element
    Syntax getSyntax() const;

    /// @brief Get the semantics of the vertex element.
    /// @return the semantics of the vertex element
    Semantics getSemantics() const;

    /// @brief Get the size, in Bytes, of the vertex element.
    /// @return the size, in Bytes, of the vertex element
    size_t getSize() const;

}; // class VertexElementDescriptor
    
} // namespace Ego

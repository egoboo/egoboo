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

/// The description of a vertex element.
class VertexElement {
public:
    /// An enum class of the syntactic forms of vertex elements.
    enum class Syntax {
        /// One <tt>float</tt> value.
        F1,
        /// Two <tt>float</tt> values.
        F2,
        /// Three <tt>float</tt> values.
        F3,
        /// Four <tt>float</tt> values.
        F4,
    }; // enum class Syntax

    /// An enum class of the semantic forms of vertex elements.
    enum class Semantics {
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
    /**
     * @brief
     *  The offset, in Bytes, of the vertex element from the beginning of the vertex.
     */
    size_t offset;

    /**
     * @brief
     *  The syntax of the vertex element.
     */
    Syntax syntax;

    /**
     * @brief
     *  The semantics of the vertex element.
     */
    Semantics semantics;

public:
    /** 
     * @brief
     *  Construct this vertex element descriptor.
     * @param offset
     *  the offset, in Bytes, of the vertex element from the beginning of the vertex
     * @param syntax
     *  the syntax of the vertex element
     * @param semantics
     *  the semantics of the vertex element
     */
    VertexElement(size_t offset, Syntax syntax, Semantics semantics);

    /** 
     * @brief
     *  Copy-construct this vertex element descriptor from another vertex element descriptor.
     * @param other
     *  the other vertex element descriptor
     */
    VertexElement(const VertexElement& other);

    /**
     * @brief
     *  Assign this vertex element descriptor from another vertex element descriptor.
     * @param other
     *  the other vertex element descriptor
     * @return
     *  this vertex element descriptor
     */
    const VertexElement& operator=(const VertexElement& other);
    
public:
    /** 
     * @brief
     *  Get if this vertex element descriptor is equivalent to another vertex element descriptor.
     * @param other
     *  the other vertex element descriptor
     * @return
     *  @a true if this vertex element descriptor is equivalent to the other vertex element descriptor,
     *  @a false otherwise
     */
    bool operator==(const VertexElement& other) const;

    /** 
     * @brief
     *  Get if this vertex element descriptor is not equivalent to another vertex element descriptor.
     * @param other
     *  the other vertex element descriptor
     * @return
     *  @a true if this vertex element descriptor is not equivalent to the other vertex element descriptor,
     *  @a false otherwise
     */
    bool operator!=(const VertexElement& other) const;
    
public:
    /** 
     * @brief
     *  Get the offset, in Bytes, of the vertex element from the beginning of the vertex.
     * @return
     *  the offset, in Bytes, of the vertex element from the beginning of the vertex
     */
    size_t getOffset() const;

    /**
     * @brief
     *  Get the syntax of the vertex element.
     * @return
     *  the syntax of this vertex element
     */
    Syntax getSyntax() const;

    /** 
     * @brief
     *  Get the semantics of the vertex element.
     * @return
     *  the semantics of the vertex element
     */
    Semantics getSemantics() const;

    /**
     * @brief
     *  Get the size, in Bytes, of the vertex element.
     * @return
     *  the size, in Bytes, of the vertex element
     */
    size_t getSize() const;

}; // class VertexElement
    
} // namespace Ego

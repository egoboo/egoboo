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

/// @file egolib/FileFormats/MapTileDefinitionsDictionary.hpp
/// @brief In-memory representation of <c>MapTileDefinitionsDictionary</c> files

#pragma once

#include "egolib/typedef.h"
#include "egolib/FileFormats/map_fx.hpp"

// Forward declarations.
struct ReadContext;


namespace Ego {
namespace FileFormats {

/// See <b>MapTileDefinitionsDictionary.html</b> for more information.
namespace MapTileDefinitionsDictionary {

// Forward declarations.
struct DefinitionList;
struct Definition;
struct IndexList;
struct VertexList;
struct Index;
struct Vertex;

/// <b>DefinitionList</b> block . See <c>MapTileDefinitionsDictionary.html</c> for more information.
struct DefinitionList {
	std::vector<Definition> definitions; ///< The definitions.
	
	/// @brief Read a <c>DefinitionList</c> block.
	/// @param ctxt the read context
	/// @return the <c>DefinitionList</c> block
	static DefinitionList read(ReadContext& ctxt);
}; // struct DefinitionList

/// <b>Vertex</b> block . See <c>MapTileDefinitionsDictionary.html</c> for more information.
struct Vertex {
    int position; /// "encoded" position of vertices.
    float u; ///< "horizontal" texture coordinate
    float v; ///< "vertical" texture coordinate
    static Vertex read(ReadContext& ctxt);
};

/// <b>Definition</b> block. See <c>MapTileDefinitionsDictionary.html</c> for more information.
struct Definition {
    std::vector<Vertex> vertices; /// The vertices.
    std::vector<IndexList> indexLists; ///< The index lists.

    /// @brief Read a <c>Definition</c> block.
    /// @param ctxt the read context
    /// @return the <c>Definition</c> block
    static Definition read(ReadContext& ctxt);
}; // struct Definition

/// <b>IndexList</b> block . See <c>MapTileDefinitionsDictionary.html</c> for more information.
struct IndexList {
    std::vector<int> indices;

    /// @brief Read an <c>IndexList</c> block.
    /// @param ctxt the read context
    /// @return the <c>IndexList</c> block
    static IndexList read(ReadContext& ctxt);
}; // struct IndexList

} // namespace MapTileDefinitionsDictionary
} // namespace FileFormats
} // namespace Ego

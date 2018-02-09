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

/// @file egolib/FileFormats/MapTileDefinitionsDictionary.cpp
/// @brief In-memory representation of <c>MapTileDefinitionsDictionary</c> files

#include "egolib/FileFormats/MapTileDefinitionsDictionary.hpp"
#include "egolib/fileutil.h"

namespace Ego {
namespace FileFormats {
namespace MapTileDefinitionsDictionary {

DefinitionList DefinitionList::read(ReadContext& ctxt) {
    DefinitionList definitionList;
    int numberOfDefinitions = vfs_get_next_int(ctxt);
    if (numberOfDefinitions < 0) {
        /** @todo Implement diagnostics. */
    }
    for (auto i = 0, n = numberOfDefinitions; i < n; ++i) { 
        definitionList.definitions.push_back(Definition::read(ctxt));
    }
    return definitionList;
}

Vertex Vertex::read(ReadContext& ctxt) {
    Vertex vertex;

    vertex.position = vfs_get_next_int(ctxt);
    if (vertex.position < 0 || vertex.position > 3 * 4 + 3) {
        /** @todo Implement diagnostics. */
    }
    vertex.u = vfs_get_next_float(ctxt);
    vertex.v = vfs_get_next_float(ctxt);
    
    return vertex;
}

Definition Definition::read(ReadContext& ctxt) {
    Definition definition;
    int numberOfVertices = vfs_get_next_int(ctxt);
    if (numberOfVertices < 0) {
        /** @todo Implement diagnostics. */
    }
    for (int i = 0, n = numberOfVertices; i < n; ++i) {
        definition.vertices.push_back(Vertex::read(ctxt));
    }
    int numberOfIndexLists = vfs_get_next_int(ctxt);
    if (numberOfVertices < 0) {
        /** @todo Implement diagnostics. */
    }
    for (int i = 0, n = numberOfIndexLists; i < n; ++i) {
        definition.indexLists.push_back(IndexList::read(ctxt));
    }
    return definition;
}

IndexList IndexList::read(ReadContext& ctxt) {
    IndexList indexList;
    int numberOfIndices = vfs_get_next_int(ctxt);
    if (numberOfIndices < 0) {
        /** @todo Add diagnostics. */
    }
    for (int i = 0, n = numberOfIndices; i < n; ++i) {
        int index = vfs_get_next_int(ctxt);
        if (index < 0) {
            /** @todo Implement diagnostics. */
        }
        indexList.indices.push_back(index);
    }
    return indexList;
}

} // namespace MapTileDefinitionsDictionary
} // namespace FileFormats
} // namespace Ego

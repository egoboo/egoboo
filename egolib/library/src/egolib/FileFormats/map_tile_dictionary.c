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

/// @file egolib/FileFormats/map_tile_dictionary.c
/// @brief Read the tile definitions from a file
/// @details

#include "egolib/FileFormats/map_tile_dictionary.h"
#include "egolib/FileFormats/MapTileDefinitionsDictionary.hpp"

#include "egolib/fileutil.h"
#include "egolib/Log/_Include.hpp"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------

static void tile_dictionary_finalize( tile_dictionary_t& dict );

//--------------------------------------------------------------------------------------------
bool tile_dictionary_load_vfs( const std::string& filename, tile_dictionary_t& dict )
{
    /// @details This function loads fan types for the terrain

    // "delete" the old list
    dict = tile_dictionary_t();

    // Try to open a context.
    ReadContext ctxt(filename);
    auto definitionList = Ego::FileFormats::MapTileDefinitionsDictionary::DefinitionList::read(ctxt);

    // handle default parameters
    int max_dict_size = MAP_FAN_TYPE_MAX;

    int fantype_count = definitionList.definitions.size();
    int fantype_offset = 2 * std::pow( 2.0f, std::floor( std::log( fantype_count ) / std::log( 2.0f ) ) );
    int definition_count = 2 * fantype_offset;

    if ( definition_count > MAP_FAN_TYPE_MAX )
    {
		Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "tile dictionary has too many tile definitions - received ", definition_count, ", expected at most ", MAP_FAN_TYPE_MAX, Log::EndOfEntry);
        return false;
    }

    dict.offset = fantype_offset;
    dict.def_count = definition_count;

    for (int fantype = 0; fantype < fantype_count; fantype++ )
    {
        tile_definition_t& pdef_sml = dict.def_lst[fantype];
        tile_definition_t& pdef_big = dict.def_lst[fantype + fantype_offset];

        const auto& definition = definitionList.definitions[fantype];
        int numberOfVertices = definition.vertices.size();

        pdef_sml.numvertices = numberOfVertices;
        pdef_big.numvertices = numberOfVertices;  // Dupe

        for (int i = 0; i < numberOfVertices; ++i)
        {
            const auto& vertex = definition.vertices[i];
            
            int position = vertex.position;
            pdef_sml.vertices[i].ref = position;
            pdef_sml.vertices[i].grid_ix = position & 3;
            pdef_sml.vertices[i].grid_iy = (position >> 2 ) & 3;

            pdef_sml.vertices[i].u = vertex.u;
            pdef_sml.vertices[i].v = vertex.v;

            // Dupe
            pdef_big.vertices[i].ref = pdef_sml.vertices[i].ref;
            pdef_big.vertices[i].grid_ix = pdef_sml.vertices[i].grid_ix;
            pdef_big.vertices[i].grid_iy = pdef_sml.vertices[i].grid_iy;
            pdef_big.vertices[i].u = pdef_sml.vertices[i].u;
            pdef_big.vertices[i].v = pdef_sml.vertices[i].v;
        }

        int numberOfCommands = definition.indexLists.size();
        pdef_sml.command_count = numberOfCommands;
        pdef_big.command_count = numberOfCommands;  // Dupe

        // concatenate the index lists into a single index list
        int contiguousIndex = 0;
        for (int i = 0; i < numberOfCommands; i++ )
        {
            const auto& indexList = definition.indexLists[i];
            int numberOfIndices = indexList.indices.size();
            pdef_sml.command_entries[i] = numberOfIndices;
            pdef_big.command_entries[i] = numberOfIndices;  // Dupe

            for (int j = 0; j < numberOfIndices; j++ )
            {
                int index = indexList.indices[j];
                pdef_sml.command_verts[contiguousIndex] = index;
                pdef_big.command_verts[contiguousIndex] = index;  // Dupe

                contiguousIndex++;
            }
        }
    }

    dict.loaded = true;

    tile_dictionary_finalize( dict );

    return true;
}

//--------------------------------------------------------------------------------------------
void tile_dictionary_finalize( tile_dictionary_t& dict )
{
    const int   tile_pix_sml = 32;
    const int   tile_pix_big = tile_pix_sml * 2;
    const float texture_offset = 0.5f;

    // Correct all of them silly texture positions for seamless tiling
    size_t fantype_offset = dict.offset;
    for (size_t entry = 0; entry < fantype_offset; entry++)
    {
        tile_definition_t& pdef_sml = dict.def_lst[entry];
        tile_definition_t& pdef_big = dict.def_lst[entry + fantype_offset];

        for (size_t cnt = 0; cnt < pdef_sml.numvertices; cnt++ )
        {
            pdef_sml.vertices[cnt].u = ( texture_offset + pdef_sml.vertices[cnt].u * ( tile_pix_sml - 2.0f * texture_offset ) ) / tile_pix_sml;
            pdef_sml.vertices[cnt].v = ( texture_offset + pdef_sml.vertices[cnt].v * ( tile_pix_sml - 2.0f * texture_offset ) ) / tile_pix_sml;

            pdef_big.vertices[cnt].u = ( texture_offset + pdef_big.vertices[cnt].u * ( tile_pix_big - 2.0f * texture_offset ) ) / tile_pix_big;
            pdef_big.vertices[cnt].v = ( texture_offset + pdef_big.vertices[cnt].v * ( tile_pix_big - 2.0f * texture_offset ) ) / tile_pix_big;
        }
    }
}

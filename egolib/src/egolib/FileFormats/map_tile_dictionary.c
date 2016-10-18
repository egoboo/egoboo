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

#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/Log/_Include.hpp"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------

static void tile_dictionary_finalize( tile_dictionary_t& dict );

//--------------------------------------------------------------------------------------------
bool tile_dictionary_load_vfs( const std::string& filename, tile_dictionary_t& dict, int max_dict_size )
{
    /// @author ZZ
    /// @details This function loads fan types for the terrain

    // "delete" the old list
    dict = tile_dictionary_t();

    if (filename.empty()) return false;

    // handle default parameters
    if (max_dict_size < 0)
    {
        max_dict_size = MAP_FAN_TYPE_MAX;
    }

    // Try to open a context.
    ReadContext ctxt(filename);

    int fantype_count    = vfs_get_next_int(ctxt);
    int fantype_offset   = 2 * std::pow( 2.0f, std::floor( std::log( fantype_count ) / std::log( 2.0f ) ) );
    int definition_count = 2 * fantype_offset;

    if ( definition_count > MAP_FAN_TYPE_MAX )
    {
		Log::get().error( "%s - tile dictionary has too many tile definitions (%d/%d).\n", __FUNCTION__, definition_count, MAP_FAN_TYPE_MAX );
        return false;
    }
    else if ( definition_count > max_dict_size )
    {
		Log::get().error( "%s - the number of tile difinitions has exceeded the requested number (%d/%d).\n", __FUNCTION__, definition_count, max_dict_size );
        return false;
    }

    dict.offset    = fantype_offset;
    dict.def_count = definition_count;

    for (int fantype = 0; fantype < fantype_count; fantype++ )
    {
        tile_definition_t& pdef_sml = dict.def_lst[fantype];
        tile_definition_t& pdef_big = dict.def_lst[fantype + fantype_offset];

        int numberOfVertices = vfs_get_next_int(ctxt);

        pdef_sml.numvertices = numberOfVertices;
        pdef_big.numvertices = numberOfVertices;  // Dupe

        for (int i = 0; i < numberOfVertices; ++i)
        {
            int itmp;

            itmp = vfs_get_next_int(ctxt);
            pdef_sml.vertices[i].ref = itmp;
            pdef_sml.vertices[i].grid_ix = itmp & 3;
            pdef_sml.vertices[i].grid_iy = ( itmp >> 2 ) & 3;

            float ftmp;

            ftmp = vfs_get_next_float(ctxt);
            pdef_sml.vertices[i].u = ftmp;

            ftmp = vfs_get_next_float(ctxt);
            pdef_sml.vertices[i].v = ftmp;

            // Dupe
            pdef_big.vertices[i].ref = pdef_sml.vertices[i].ref;
            pdef_big.vertices[i].grid_ix = pdef_sml.vertices[i].grid_ix;
            pdef_big.vertices[i].grid_iy = pdef_sml.vertices[i].grid_iy;
            pdef_big.vertices[i].u = pdef_sml.vertices[i].u;
            pdef_big.vertices[i].v = pdef_sml.vertices[i].v;
        }

        int numberOfCommands = vfs_get_next_int(ctxt);
        pdef_sml.command_count = numberOfCommands;
        pdef_big.command_count = numberOfCommands;  // Dupe

        // concatenate the index lists into a single index list
        int contiguousIndex = 0;
        for (int i = 0; i < numberOfCommands; i++ )
        {
            int numberOfIndices = vfs_get_next_int(ctxt);
            pdef_sml.command_entries[i] = numberOfIndices;
            pdef_big.command_entries[i] = numberOfIndices;  // Dupe

            for (int j = 0; j < numberOfIndices; j++ )
            {
                int index = vfs_get_next_int(ctxt);
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

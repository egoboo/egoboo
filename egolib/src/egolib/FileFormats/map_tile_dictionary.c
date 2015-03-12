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

#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/log.h"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

tile_dictionary_t tile_dict = TILE_DICTIONARY_INIT;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static tile_dictionary_t * tile_dictionary_finalize( tile_dictionary_t * pdict );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool tile_dictionary_load_vfs( const char * filename, tile_dictionary_t * pdict, int max_dict_size )
{
    /// @author ZZ
    /// @details This function loads fan types for the terrain

    Uint32 cnt, entry, vertices, commandsize;
    int fantype_count, fantype_offset, fantype;
    int command_count, command;
    int definition_count;
    int itmp;
    float ftmp;
    tile_definition_t * pdef_sml, * pdef_big;

    if ( NULL == pdict ) return false;

    // "delete" the old list
    BLANK_STRUCT_PTR( pdict );

    if ( !VALID_CSTR( filename ) ) return false;

    // handle default parameters
    if ( max_dict_size < 0 )
    {
        max_dict_size = MAP_FAN_TYPE_MAX;
    }

    // Try to open a context.
    ReadContext ctxt(filename);
    if (!ctxt.ensureOpen()) {
        log_error("unable to load tile definitions file `%s`\n", filename);
        return false;
    }

    fantype_count    = vfs_get_next_int(ctxt);
    fantype_offset   = 2 * POW( 2.0f, FLOOR( LOG( fantype_count ) / LOG( 2.0f ) ) );
    definition_count = 2 * fantype_offset;

    if ( definition_count > MAP_FAN_TYPE_MAX )
    {
        log_error( "%s - tile dictionary has too many tile definitions (%d/%d).\n", __FUNCTION__, definition_count, MAP_FAN_TYPE_MAX );
        goto tile_dictionary_load_vfs_fail;
    }
    else if ( definition_count > max_dict_size )
    {
        log_warning( "%s - the number of tile difinitions has exceeded the requested number (%d/%d).\n", __FUNCTION__, definition_count, max_dict_size );
        goto tile_dictionary_load_vfs_fail;
    }

    pdict->offset    = fantype_offset;
    pdict->def_count = definition_count;

    for ( fantype = 0; fantype < fantype_count; fantype++ )
    {
        pdef_sml = pdict->def_lst + fantype;
        pdef_big = pdict->def_lst + fantype + fantype_offset;

        vertices = vfs_get_next_int(ctxt);

        pdef_sml->numvertices = vertices;
        pdef_big->numvertices = vertices;  // Dupe

        for ( cnt = 0; cnt < vertices; cnt++ )
        {
            itmp = vfs_get_next_int(ctxt);
            pdef_sml->ref[cnt]    = itmp;
            pdef_sml->grid_ix[cnt] = itmp & 3;
            pdef_sml->grid_iy[cnt] = ( itmp >> 2 ) & 3;

            ftmp = vfs_get_next_float(ctxt);
            pdef_sml->u[cnt] = ftmp;

            ftmp = vfs_get_next_float(ctxt);
            pdef_sml->v[cnt] = ftmp;

            // Dupe
            pdef_big->ref[cnt]    = pdef_sml->ref[cnt];
            pdef_big->grid_ix[cnt] = pdef_sml->grid_ix[cnt];
            pdef_big->grid_iy[cnt] = pdef_sml->grid_iy[cnt];
            pdef_big->u[cnt]      = pdef_sml->u[cnt];
            pdef_big->v[cnt]      = pdef_sml->v[cnt];
        }

        command_count = vfs_get_next_int(ctxt);
        pdef_sml->command_count = command_count;
        pdef_big->command_count = command_count;  // Dupe

        for ( entry = 0, command = 0; command < command_count; command++ )
        {
            commandsize = vfs_get_next_int(ctxt);
            pdef_sml->command_entries[command] = commandsize;
            pdef_big->command_entries[command] = commandsize;  // Dupe

            for ( cnt = 0; cnt < commandsize; cnt++ )
            {
                itmp = vfs_get_next_int(ctxt);
                pdef_sml->command_verts[entry] = itmp;
                pdef_big->command_verts[entry] = itmp;  // Dupe

                entry++;
            }
        }
    }

    pdict->loaded = true;

    pdict = tile_dictionary_finalize( pdict );

    return true;

tile_dictionary_load_vfs_fail:

    BLANK_STRUCT_PTR( pdict );

    return false;
}

//--------------------------------------------------------------------------------------------
tile_dictionary_t * tile_dictionary_finalize( tile_dictionary_t * pdict )
{
    const int   tile_pix_sml = 32;
    const int   tile_pix_big = tile_pix_sml * 2;
    const float texture_offset = 0.5f;

    int vertex_count;
    tile_definition_t * pdef_sml, * pdef_big;

    if ( NULL == pdict ) return pdict;

    // Correct all of them silly texture positions for seamless tiling
    size_t fantype_offset = pdict->offset;
    for ( Uint32 entry = 0; entry < fantype_offset; entry++ )
    {
        pdef_sml = pdict->def_lst + entry;
        pdef_big = pdict->def_lst + entry + fantype_offset;

        vertex_count = pdef_sml->numvertices;
        for ( Uint32 cnt = 0; cnt < vertex_count; cnt++ )
        {
            pdef_sml->u[cnt] = ( texture_offset + pdef_sml->u[cnt] * ( tile_pix_sml - 2.0f * texture_offset ) ) / tile_pix_sml;
            pdef_sml->v[cnt] = ( texture_offset + pdef_sml->v[cnt] * ( tile_pix_sml - 2.0f * texture_offset ) ) / tile_pix_sml;

            pdef_big->u[cnt] = ( texture_offset + pdef_big->u[cnt] * ( tile_pix_big - 2.0f * texture_offset ) ) / tile_pix_big;
            pdef_big->v[cnt] = ( texture_offset + pdef_big->v[cnt] * ( tile_pix_big - 2.0f * texture_offset ) ) / tile_pix_big;
        }
    }

    return pdict;
}

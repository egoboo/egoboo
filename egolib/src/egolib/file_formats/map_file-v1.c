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

/// @file egolib/file_formats/map_file-v1.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/file_formats/map_file-v1.h"

#include "egolib/log.h"
#include "egolib/strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
map_t * map_read_v1( vfs_FILE * fileread, map_t * pmesh )
{
    /// @author ZZ
    /// @details This function loads the level.mpd file

    Uint32 itile;
    size_t tile_count;

    Uint32 ui32_tmp;

    map_mem_t   * pmem  = NULL;
    tile_info_t * ptile = NULL;

    // if there is no filename, fail
    if ( NULL == fileread ) return pmesh;

    // if there is no mesh struct, fail
    if ( NULL == pmesh ) return pmesh;
    pmem  = &( pmesh->mem );

    // a valid number of tiles?
    if ( 0 == pmem->tile_count || NULL == pmem->tile_list ) return pmesh;

    // Load itile data
    tile_count = pmem->tile_count;
    for ( itile = 0; itile < tile_count; itile++ )
    {
        ptile = pmem->tile_list + itile;

        vfs_read_Uint32( fileread, &ui32_tmp );

        ptile->type = CLIP_TO_08BITS( ui32_tmp >> 24 );
        ptile->fx   = CLIP_TO_08BITS( ui32_tmp >> 16 );
        ptile->img  = CLIP_TO_16BITS( ui32_tmp >>  0 );
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t * map_write_v1( vfs_FILE * filewrite, map_t * pmesh )
{
    size_t itile, tile_count;
    Uint32 ui32_tmp;

    map_mem_t   * pmem  = NULL;
    tile_info_t * ptile = NULL;

    // if there is no filename, fail
    if ( NULL == filewrite ) return pmesh;

    // if there is no mesh struct, fail
    if ( NULL == pmesh ) return pmesh;
    pmem  = &( pmesh->mem );

    // a valid number of tiles?
    if ( 0 == pmem->tile_count || NULL == pmem->tile_list ) return pmesh;

    // write the fx data for each tile
    tile_count = pmem->tile_count;
    for ( itile = 0; itile < tile_count; itile++ )
    {
        ptile = pmem->tile_list + itile;

        ui32_tmp  = CLIP_TO_16BITS( ptile->img ) <<  0;
        ui32_tmp |= CLIP_TO_08BITS( ptile->fx ) << 16;
        ui32_tmp |= CLIP_TO_08BITS( ptile->type ) << 24;

        vfs_write_Uint32( filewrite, ui32_tmp );
    }

    return pmesh;
}

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

/// @file file_formats/map_file-v2.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "map_file-v2.h"

#include "../log.h"
#include "../strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
map_t * map_read_v2( FILE * fileread, map_t * pmesh )
{
    /// @author ZZ
    /// @details This function loads the level.mpd file

    Uint32 itile;
    Uint32 tile_count;

    Uint8 ui08_tmp;

    map_mem_t   * pmem  = NULL;
    tile_info_t * ptile = NULL;

    // if there is no filename, fail
    if ( NULL == fileread ) return pmesh;

    // if there is no mesh struct, fail
    if ( NULL == pmesh ) return pmesh;
    pmem  = &( pmesh->mem );

    // a valid number of tiles?
    if ( 0 == pmem->tile_count || NULL == pmem->tile_list ) return pmesh;

    // Load twist data
    tile_count = pmem->tile_count;
    for ( itile = 0; itile < tile_count; itile++ )
    {
        ptile = pmem->tile_list + itile;

        endian_fread_uint08( fileread, &ui08_tmp );

        ptile->twist = ui08_tmp;
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t * map_write_v2( FILE * filewrite, map_t * pmesh )
{
    size_t itile;

    map_mem_t   * pmem  = NULL;
    tile_info_t * ptile = NULL;

    if ( NULL == filewrite ) return pmesh;

    if ( NULL == pmesh ) return pmesh;
    pmem  = &( pmesh->mem );

    // a valid number of tiles?
    if ( 0 == pmem->tile_count || NULL == pmem->tile_list ) return pmesh;

    // write the twist data for each tile
    for ( itile = 0; itile < pmem->tile_count; itile++ )
    {
        ptile = pmem->tile_list + itile;
        endian_fwrite_uint08( filewrite, ptile->twist );
    }

    return pmesh;
}
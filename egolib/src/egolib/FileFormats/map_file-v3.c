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

/// @file egolib/FileFormats/map_file-v3.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/FileFormats/map_file-v3.h"

#include "egolib/log.h"
#include "egolib/strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
map_t * map_read_v3( vfs_FILE * fileread, map_t * pmesh )
{
    /// @author ZZ
    /// @details This function loads the level.mpd file

    size_t ivert, vert_count;
    float ieee32_tmp;

    map_mem_t    * pmem = NULL;
    map_vertex_t * pvert = NULL;

    // if there is no filename, fail
    if ( NULL == fileread ) return pmesh;

    // if there is no mesh struct, fail
    if ( NULL == pmesh ) return pmesh;
    pmem  = &( pmesh->mem );

    // a valid number of vertices?
    if ( 0 == pmem->vcount || NULL == pmem->vlst ) return pmesh;

    // get the vertex count
    vert_count = pmem->vcount;

    // Load vertex x data
    for ( ivert = 0; ivert < vert_count; ivert++ )
    {
        pvert = pmem->vlst + ivert;

        vfs_read_float( fileread, &ieee32_tmp );

        pvert->pos.x = ieee32_tmp;
    }

    // Load vertex y data
    for ( ivert = 0; ivert < vert_count; ivert++ )
    {
        pvert = pmem->vlst + ivert;

        vfs_read_float( fileread, &ieee32_tmp );

        pvert->pos.y = ieee32_tmp;
    }

    // Load vertex z data
    for ( ivert = 0; ivert < vert_count; ivert++ )
    {
        pvert = pmem->vlst + ivert;

        vfs_read_float( fileread, &ieee32_tmp );

        pvert->pos.z = ieee32_tmp / 16.0f;
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t * map_write_v3( vfs_FILE * filewrite, map_t * pmesh )
{
    size_t ivert, vert_count;

    map_mem_t    * pmem = NULL;
    map_vertex_t * pvert = NULL;

    if ( NULL == filewrite ) return pmesh;

    if ( NULL == pmesh ) return pmesh;
    pmem  = &( pmesh->mem );

    // a valid number of vertices?
    if ( 0 == pmem->vcount || NULL == pmem->vlst ) return pmesh;

    // get the vertex count
    vert_count = pmem->vcount;

    // write the x-coordinate data for each vertex
    for ( ivert = 0; ivert < vert_count; ivert++ )
    {
        pvert = pmem->vlst + ivert;

        vfs_write_float( filewrite, pvert->pos.x );
    }

    // write the y-coordinate data for each vertex
    for ( ivert = 0; ivert < vert_count; ivert++ )
    {
        pvert = pmem->vlst + ivert;

        vfs_write_float( filewrite, pvert->pos.y );
    }

    // write the y-coordinate data for each vertex
    for ( ivert = 0; ivert < vert_count; ivert++ )
    {
        pvert = pmem->vlst + ivert;

        // cartman scales the z-axis based off of a 4 bit fixed precision number
        vfs_write_float( filewrite, pvert->pos.z * 16.0f );
    }

    return pmesh;
}
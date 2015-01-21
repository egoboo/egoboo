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

/// @file egolib/file_formats/map_file-v4.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/file_formats/map_file-v4.h"

#include "egolib/log.h"
#include "egolib/strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
map_t * map_read_v4( vfs_FILE * fileread, map_t * pmesh )
{
    /// @author ZZ
    /// @details This function loads the level.mpd file

    size_t ivert, vert_count;
    Uint8 ui08_tmp;

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

    // Load vertex a data
    for ( ivert = 0; ivert < vert_count; ivert++ )
    {
        pvert = pmem->vlst + ivert;

        vfs_read_Uint8(fileread, &ui08_tmp);

        pvert->a = ui08_tmp;
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t * map_write_v4( vfs_FILE * filewrite, map_t * pmesh )
{
    size_t ivert, vert_count;

    map_mem_t    * pmem  = NULL;
    map_vertex_t * pvert = NULL;

    if ( NULL == filewrite ) return pmesh;

    if ( NULL == pmesh ) return pmesh;
    pmem  = &( pmesh->mem );

    // a valid number of vertices?
    if ( 0 == pmem->vcount || NULL == pmem->vlst ) return NULL;

    // count the vertices
    vert_count = pmem->vcount;

    for ( ivert = 0; ivert < pmem->vcount; ivert++ )
    {
        pvert = pmem->vlst + ivert;

        vfs_write_Uint8(filewrite, pvert->a);
    }

    return pmesh;
}

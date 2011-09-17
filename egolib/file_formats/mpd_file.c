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

/// @file file_formats/mpd_file.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "mpd_file.h"
#include "../log.h"

#include "../endian.h"
#include "../fileutil.h"
#include "../strutil.h"
//#include "../egoboo.h"

#include "../_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static mpd_info_t * mpd_info_ctor( mpd_info_t * pinfo );
static mpd_info_t * mpd_info_dtor( mpd_info_t * pinfo );

static mpd_mem_t *  mpd_mem_ctor( mpd_mem_t * pmem );
static mpd_mem_t *  mpd_mem_dtor( mpd_mem_t * pmem );
static bool_t       mpd_mem_free( mpd_mem_t * pmem );
static bool_t       mpd_mem_alloc( mpd_mem_t * pmem, mpd_info_t * pinfo );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

tile_dictionary_t tile_dict = TILE_DICTIONARY_INIT;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t tile_dictionary_load_vfs( const char * filename, tile_dictionary_t * pdict, int max_dict_size )
{
    /// @author ZZ
    /// @details This function loads fan types for the terrain

    const int   tile_pix_sml = 32;
    const int   tile_pix_big = tile_pix_sml * 2;
    const float texture_offset = 0.5f;

    Uint32 cnt, entry, vertices, commandsize;
    int fantype_count, fantype_offset, fantype;
    int command_count, command;
    int definition_count;
    int itmp;
    float ftmp;
    vfs_FILE* fileread;
    tile_definition_t * pdef_sml, * pdef_big;

    if ( NULL == pdict ) return bfalse;

    // "delete" the old list
    BLANK_STRUCT_PTR( pdict );

    if ( !VALID_CSTR( filename ) ) return bfalse;

    // handle default parameters
    if ( max_dict_size < 0 )
    {
        max_dict_size = MPD_FAN_TYPE_MAX;
    }

    // Open the file and go to it
    fileread = vfs_openRead( filename );
    if ( NULL == fileread )
    {
        log_error( "Cannot load the tile definitions \"%s\".\n", filename );
        return bfalse;
    }

    fantype_count    = vfs_get_next_int( fileread );
    fantype_offset   = 2 * POW( 2.0f, FLOOR( LOG( fantype_count ) / LOG( 2.0f ) ) );
    definition_count = 2 * fantype_offset;

    if ( definition_count > MPD_FAN_TYPE_MAX )
    {
        log_error( "%s - tile dictionary has too many tile definitions (%d/%d).\n", __FUNCTION__, definition_count, MPD_FAN_TYPE_MAX );
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

        vertices = vfs_get_next_int( fileread );

        pdef_sml->numvertices = vertices;
        pdef_big->numvertices = vertices;  // Dupe

        for ( cnt = 0; cnt < vertices; cnt++ )
        {
            itmp = vfs_get_next_int( fileread );
            pdef_sml->ref[cnt]    = itmp;
            pdef_sml->grid_ix[cnt] = itmp & 3;
            pdef_sml->grid_iy[cnt] = ( itmp >> 2 ) & 3;

            ftmp = vfs_get_next_float( fileread );
            pdef_sml->u[cnt] = ftmp;

            ftmp = vfs_get_next_float( fileread );
            pdef_sml->v[cnt] = ftmp;

            // Dupe
            pdef_big->ref[cnt]    = pdef_sml->ref[cnt];
            pdef_big->grid_ix[cnt] = pdef_sml->grid_ix[cnt];
            pdef_big->grid_iy[cnt] = pdef_sml->grid_iy[cnt];
            pdef_big->u[cnt]      = pdef_sml->u[cnt];    
            pdef_big->v[cnt]      = pdef_sml->v[cnt];
        }

        command_count = vfs_get_next_int( fileread );
        pdef_sml->command_count = command_count;
        pdef_big->command_count = command_count;  // Dupe

        for ( entry = 0, command = 0; command < command_count; command++ )
        {
            commandsize = vfs_get_next_int( fileread );
            pdef_sml->command_entries[command] = commandsize;
            pdef_big->command_entries[command] = commandsize;  // Dupe

            for ( cnt = 0; cnt < commandsize; cnt++ )
            {
                itmp = vfs_get_next_int( fileread );
                pdef_sml->command_verts[entry] = itmp;
                pdef_big->command_verts[entry] = itmp;  // Dupe

                entry++;
            }
        }
    }

    vfs_close( fileread );

    // Correct all of them silly texture positions for seamless tiling
    for ( entry = 0; entry < fantype_count; entry++ )
    {
        pdef_sml = pdict->def_lst + entry;
        pdef_big = pdict->def_lst + entry + fantype_offset;
        for ( cnt = 0; cnt < pdef_sml->numvertices; cnt++ )
        {
            pdef_sml->u[cnt] = ( texture_offset + pdef_sml->u[cnt] * ( tile_pix_sml - 2.0f * texture_offset ) ) / tile_pix_sml;
            pdef_sml->v[cnt] = ( texture_offset + pdef_sml->v[cnt] * ( tile_pix_sml - 2.0f * texture_offset ) ) / tile_pix_sml;

            pdef_big->u[cnt] = ( texture_offset + pdef_big->u[cnt] * ( tile_pix_big - 2.0f * texture_offset ) ) / tile_pix_big;
            pdef_big->v[cnt] = ( texture_offset + pdef_big->v[cnt] * ( tile_pix_sml - 2.0f * texture_offset ) ) / tile_pix_big;
        }
    }

    pdict->loaded = btrue;

    return btrue;

tile_dictionary_load_vfs_fail:

    BLANK_STRUCT_PTR( pdict );

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_t * mpd_ctor( mpd_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    BLANK_STRUCT_PTR( pmesh )

    if ( NULL == mpd_mem_ctor( &( pmesh->mem ) ) ) return NULL;
    if ( NULL == mpd_info_ctor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
mpd_t * mpd_dtor( mpd_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    if ( NULL == mpd_mem_dtor( &( pmesh->mem ) ) ) return NULL;
    if ( NULL == mpd_info_dtor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
mpd_t * mpd_renew( mpd_t * pmesh )
{
    pmesh = mpd_dtor( pmesh );
    pmesh = mpd_ctor( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_free( mpd_t * pmesh )
{
    if ( NULL == pmesh ) return bfalse;

    mpd_mem_free( &( pmesh->mem ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_init( mpd_t * pmesh, mpd_info_t * pinfo )
{
    mpd_info_t loc_info;

    if ( NULL == pmesh || NULL == pinfo ) return bfalse;

    // make a copy of the mesh info, in case "pinfo == &(pmesh->info)"
    memcpy( &loc_info, pinfo, sizeof( loc_info ) );

    // renew the mesh
    mpd_renew( pmesh );

    // check the tiles in the x direction
    if ( loc_info.tiles_x >= MPD_TILEY_MAX )
    {
        log_warning( "%s - invalid mpd size. Mesh too large in x direction, %d.\n", __FUNCTION__, loc_info.tiles_x );
        goto mpd_init_fail;
    }

    // check the tiles in the y direction
    if ( loc_info.tiles_y >= MPD_TILEY_MAX )
    {
        log_warning( "%s - invalid mpd size. Mesh too large in y direction, %d.\n", __FUNCTION__, loc_info.tiles_y );
        goto mpd_init_fail;
    }

    // check the total number of vertices
    if ( loc_info.vertcount > MPD_VERTICES_MAX )
    {
        log_warning( "%s - invalid mpd size. too many vertices, %d.\n", __FUNCTION__ , loc_info.vertcount );
    }

    // allocate the mesh memory
    if ( !mpd_mem_alloc( &( pmesh->mem ), pinfo ) )
    {
        log_warning( "%s - could not allocate memory for the mesh!!\n", __FUNCTION__ );
        goto mpd_init_fail;
    }

    // copy the desired mesh info into the actual mesh info
    memmove( &( pmesh->info ), &loc_info, sizeof( pmesh->info ) );

    return btrue;

mpd_init_fail:

    mpd_dtor( pmesh );

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_info_t * mpd_info_ctor( mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return pinfo;

    BLANK_STRUCT_PTR( pinfo )

    return pinfo;
}

//--------------------------------------------------------------------------------------------
mpd_info_t * mpd_info_dtor( mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return NULL;

    BLANK_STRUCT_PTR( pinfo )

    return pinfo;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_mem_t * mpd_mem_ctor( mpd_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
mpd_mem_t * mpd_mem_dtor( mpd_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    mpd_mem_free( pmem );
    BLANK_STRUCT_PTR( pmem )

    return pmem;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_mem_alloc( mpd_mem_t * pmem, mpd_info_t * pinfo )
{
    int tile_count;

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !mpd_mem_free( pmem ) ) return bfalse;

    if ( pinfo->vertcount > MPD_VERTICES_MAX )
    {
        log_warning( "mpd_mem_alloc() - mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MPD_VERTICES_MAX );
        return bfalse;
    }

    // allocate new memory
    pmem->vlst = EGOBOO_NEW_ARY( mpd_vertex_t, pinfo->vertcount );
    if ( NULL == pmem->vlst )
    {
        mpd_mem_free( pmem );
        log_error( "mpd_mem_alloc() - reduce the maximum number of vertices! (Check MPD_VERTICES_MAX)\n" );
        return bfalse;
    }
    pmem->vcount = pinfo->vertcount;

    tile_count = pinfo->tiles_x * pinfo->tiles_y;
    pmem->tile_list  = EGOBOO_NEW_ARY( tile_info_t, tile_count );
    if ( NULL == pmem->tile_list )
    {
        mpd_mem_free( pmem );
        log_error( "mpd_mem_alloc() - not enough memory to allocate the tile info\n" );
        return bfalse;
    }
    pmem->tile_count = tile_count;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_mem_free( mpd_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    EGOBOO_DELETE_ARY( pmem->vlst );
    pmem->vcount = 0;

    EGOBOO_DELETE_ARY( pmem->tile_list );
    pmem->tile_count = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_t * mpd_load( const char *loadname, mpd_t * pmesh )
{
    //// @author ZZ
    /// @details This function loads the level.mpd file

    FILE* fileread = NULL;

    Uint32 fan, cnt;
    Uint32 tiles_count;

    Uint32 ui32_tmp;
    Uint8 ui08_tmp;
    float ieee32_tmp;
    mpd_info_t loc_info;

    mpd_info_t * pinfo = NULL;
    mpd_mem_t  * pmem = NULL;

    // if there is no mesh struct, fail
    if ( NULL == pmesh ) return pmesh;
    pinfo = &( pmesh->info );
    pmem  = &( pmesh->mem );

    // if there is no filename, fail and wipe the mesh struct
    if ( INVALID_CSTR( loadname ) )
    {
        goto mpd_load_fail;
    }

    fileread = fopen( loadname, "rb" );
    if ( NULL == fileread )
    {
        log_warning( "%s - cannot find \"%s\"!!\n", __FUNCTION__, loadname );
        goto mpd_load_fail;
    }

    // read the file type
    endian_fread_uint32( fileread, &ui32_tmp );
    if ( MPD_ID != ui32_tmp )
    {
        log_warning( "%s - this is not a valid level.mpd!!\n", __FUNCTION__ );
        goto mpd_load_fail;
    }

    // read the rest of the "header"
    endian_fread_uint32( fileread, &ui32_tmp );
    loc_info.vertcount = ui32_tmp;

    endian_fread_uint32( fileread, &ui32_tmp );
    loc_info.tiles_x = ui32_tmp;

    endian_fread_uint32( fileread, &ui32_tmp );
    loc_info.tiles_y = ui32_tmp;

    // allocate the mesh memory
    if ( !mpd_init( pmesh, &loc_info ) )
    {
        goto mpd_load_fail;
    }

    // calculate the total number of tiles in the file
    tiles_count = pinfo->tiles_x * pinfo->tiles_y;

    // Load fan data
    for ( fan = 0; fan < tiles_count; fan++ )
    {
        endian_fread_uint32( fileread, &ui32_tmp );

        pmem->tile_list[fan].type = CLIP_TO_08BITS( ui32_tmp >> 24 );
        pmem->tile_list[fan].fx   = CLIP_TO_08BITS( ui32_tmp >> 16 );
        pmem->tile_list[fan].img  = CLIP_TO_16BITS( ui32_tmp >>  0 );
    }

    // Load twist data
    for ( fan = 0; fan < tiles_count; fan++ )
    {
        endian_fread_uint08( fileread, &ui08_tmp );

        pmem->tile_list[fan].twist = ui08_tmp;
    }

    // Load vertex x data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        endian_fread_ieee32( fileread, &ieee32_tmp );

        pmem->vlst[cnt].pos.x = ieee32_tmp;
    }

    // Load vertex y data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        endian_fread_ieee32( fileread, &ieee32_tmp );

        pmem->vlst[cnt].pos.y = ieee32_tmp;
    }

    // Load vertex z data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        endian_fread_ieee32( fileread, &ieee32_tmp );

        // cartman scales the z-axis based off of a 4 bit fixed precision number
        pmem->vlst[cnt].pos.z = ieee32_tmp / 16.0f;
    }

    // Load vertex a data
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        endian_fread_uint08( fileread, &ui08_tmp );

        pmem->vlst[cnt].a = ui08_tmp;
    }

    fclose( fileread );

    return pmesh;

mpd_load_fail:

    mpd_renew( pmesh );

    if ( NULL != fileread )
    {
        fclose( fileread );
        fileread = NULL;
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------
mpd_t * mpd_save( const char * savename, mpd_t * pmesh )
{
    FILE* filewrite;
    size_t cnt;

    mpd_info_t * pinfo;
    mpd_mem_t  * pmem;

    if ( NULL == pmesh || INVALID_CSTR( savename ) ) return NULL;

    pinfo = &( pmesh->info );
    pmem  = &( pmesh->mem );

    // a valid number of tiles?
    if ( 0 == pmem->tile_count || NULL == pmem->tile_list ) return NULL;

    // a valid number of vertices?
    if ( 0 == pmem->vcount || NULL == pmem->vlst ) return NULL;

    filewrite = fopen( savename, "wb" );
    if ( NULL == filewrite ) return NULL;

    // write the file identifier
    endian_fwrite_uint32( filewrite, MPD_ID );

    // write the file vertex count
    endian_fwrite_uint32( filewrite, pinfo->vertcount );

    // write the tiles in the x direction
    endian_fwrite_uint32( filewrite, pinfo->tiles_x );

    // write the tiles in the y direction
    endian_fwrite_uint32( filewrite, pinfo->tiles_y );

    // write the fx data for each tile
    for ( cnt = 0; cnt < pmem->tile_count; cnt++ )
    {
        tile_info_t * ptile = pmem->tile_list + cnt;

        endian_fwrite_uint32( filewrite, ( ptile->type << 24 ) + ( ptile->fx << 16 ) + ptile->img );
    }

    // write the twist data for each tile
    for ( cnt = 0; cnt < pmem->tile_count; cnt++ )
    {
        tile_info_t * ptile = pmem->tile_list + cnt;

        endian_fwrite_uint08( filewrite, ptile->twist );
    }

    // write the x-coordinate data for each vertex
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        mpd_vertex_t * pvert = pmem->vlst + cnt;

        endian_fwrite_ieee32( filewrite, pvert->pos.x );
    }

    // write the y-coordinate data for each vertex
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        mpd_vertex_t * pvert = pmem->vlst + cnt;

        endian_fwrite_ieee32( filewrite, pvert->pos.y );
    }

    // write the y-coordinate data for each vertex
    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        mpd_vertex_t * pvert = pmem->vlst + cnt;

        // cartman scales the z-axis based off of a 4 bit fixed precision number
        endian_fwrite_ieee32( filewrite, pvert->pos.z * 16.0f );
    }

    for ( cnt = 0; cnt < pmem->vcount; cnt++ )
    {
        mpd_vertex_t * pvert = pmem->vlst + cnt;

        endian_fwrite_uint08( filewrite, pvert->a );
    }

    fclose( filewrite );

    return pmesh;
}

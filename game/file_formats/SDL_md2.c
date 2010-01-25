//********************************************************************************************
//*
//*    This file is part of the SDL_md2 library.
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

/// @file file_formats/SDL_md2.c
/// @brief A .md2 file reader using SDL's RWOPS interface
/// @details

#include "SDL_md2.h"
#include "egoboo_vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static SDL_bool SDL_md2_frame_free( SDL_md2_frame_t * pdata, size_t fcount );
static SDL_bool SDL_md2_alloc( SDL_md2_model_t * mdl );

static float    read_float_RW( SDL_RWops * rw );
static float  * read_SDL_vecf_RW( SDL_RWops * rw, float  vec[],  size_t count );
static Uint8  * read_SDL_vecuc_RW( SDL_RWops * rw, Uint8  vec[],  size_t count );
static Uint16 * read_SDL_vecus_RW( SDL_RWops * rw, Uint16 vec[],  size_t count );
static char   * read_SDL_string_RW( SDL_RWops * rw, char * string, size_t size );

static SDL_md2_header_t *   load_SDL_md2_header_RW( SDL_RWops * rw, SDL_md2_header_t   * pdata );
static SDL_md2_skin_t *     load_SDL_md2_skin_RW( SDL_RWops * rw, SDL_md2_skin_t     * pdata, size_t count );
static SDL_md2_texcoord_t * load_SDL_md2_texcoord_RW( SDL_RWops * rw, SDL_md2_texcoord_t * pdata, size_t count );
static SDL_md2_triangle_t * load_SDL_md2_triangle_RW( SDL_RWops * rw, SDL_md2_triangle_t * pdata, size_t count );
static SDL_md2_vertex_t *   load_SDL_md2_vertex_RW( SDL_RWops * rw, SDL_md2_vertex_t   * pdata, size_t count );
static SDL_md2_frame_t *    load_SDL_md2_frame_RW( SDL_RWops * rw, SDL_md2_frame_t    * pdata, size_t fcount, size_t vcount );
static Sint32 *             load_SDL_glcmd_RW( SDL_RWops * rw, Sint32             * pdata, size_t size );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
char * read_SDL_string_RW( SDL_RWops *  rw, char * string, size_t size )
{
    if ( NULL == string ) return string;

    *string = '\0';

    if ( NULL == rw ) return string;

    SDL_RWread( rw, string, sizeof( char ), size );

    return string;
}

//--------------------------------------------------------------------------------------------
float read_float_RW( SDL_RWops * rw )
{
    union { float f; Uint32 i; } convert;

    convert.i = SDL_ReadLE32( rw );

    return convert.f;
}

//--------------------------------------------------------------------------------------------
float * read_SDL_vecf_RW( SDL_RWops * rw, float vec[], size_t count )
{
    int i;

    if ( NULL == vec ) return vec;

    for ( i = 0; i < count; i++ )
    {
        vec[i] = read_float_RW( rw );
    }

    return vec;
}

//--------------------------------------------------------------------------------------------
Uint8 * read_SDL_vecuc_RW( SDL_RWops * rw, Uint8 vec[], size_t count )
{
    int i;

    if ( NULL == vec ) return vec;

    for ( i = 0; i < count; i++ )
    {
        SDL_RWread( rw, vec + i, 1, 1 );
    }

    return vec;
}

//--------------------------------------------------------------------------------------------
Uint16 * read_SDL_vecus_RW( SDL_RWops * rw, Uint16 vec[],  size_t count )
{
    int i;

    if ( NULL == vec ) return vec;

    for ( i = 0; i < count; i++ )
    {
        vec[i] = SDL_ReadLE16( rw );
    }

    return vec;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SDL_md2_header_t * load_SDL_md2_header_RW( SDL_RWops * rw, SDL_md2_header_t * pdata )
{
    if ( NULL == pdata ) return pdata;

    memset( pdata, 0, sizeof( SDL_md2_header_t ) );

    if ( NULL == rw ) return pdata;

    pdata->ident         = SDL_ReadLE32( rw );
    pdata->version       = SDL_ReadLE32( rw );

    pdata->skinwidth     = SDL_ReadLE32( rw );
    pdata->skinheight    = SDL_ReadLE32( rw );

    pdata->framesize     = SDL_ReadLE32( rw );

    pdata->num_skins     = SDL_ReadLE32( rw );
    pdata->num_vertices  = SDL_ReadLE32( rw );
    pdata->num_st        = SDL_ReadLE32( rw );
    pdata->num_tris      = SDL_ReadLE32( rw );
    pdata->size_glcmds   = SDL_ReadLE32( rw );
    pdata->num_frames    = SDL_ReadLE32( rw );

    pdata->offset_skins  = SDL_ReadLE32( rw );
    pdata->offset_st     = SDL_ReadLE32( rw );
    pdata->offset_tris   = SDL_ReadLE32( rw );
    pdata->offset_frames = SDL_ReadLE32( rw );
    pdata->offset_glcmds = SDL_ReadLE32( rw );
    pdata->offset_end    = SDL_ReadLE32( rw );

    pdata->num_glcmds = -1;

    return pdata;
}

//--------------------------------------------------------------------------------------------
SDL_md2_skin_t * load_SDL_md2_skin_RW( SDL_RWops * rw, SDL_md2_skin_t * pdata, size_t count )
{
    int i;

    if ( NULL == pdata ) return pdata;

    memset( pdata, 0, count * 64 * sizeof( char ) );

    if ( NULL == rw ) return pdata;

    for ( i = 0; i < count; i++ )
    {
        read_SDL_string_RW( rw, pdata[i].name, 64 );
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
SDL_md2_texcoord_t * load_SDL_md2_texcoord_RW( SDL_RWops * rw, SDL_md2_texcoord_t * pdata, size_t count )
{
    int i;

    if ( NULL == pdata ) return pdata;

    memset( pdata, 0, count * sizeof( SDL_md2_texcoord_t ) );

    if ( NULL == rw ) return pdata;

    for ( i = 0; i < count; i++ )
    {
        SDL_md2_texcoord_t * ptex = pdata + i;

        ptex->s = SDL_ReadLE16( rw );
        ptex->t = SDL_ReadLE16( rw );
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
SDL_md2_triangle_t * load_SDL_md2_triangle_RW( SDL_RWops * rw, SDL_md2_triangle_t * pdata, size_t count )
{
    int i;

    if ( NULL == pdata ) return pdata;

    memset( pdata, 0, sizeof( SDL_md2_header_t ) );

    if ( NULL == rw ) return pdata;

    for ( i = 0; i < count; i++ )
    {
        SDL_md2_triangle_t * ptri = pdata + i;

        read_SDL_vecus_RW( rw, ptri->vertex, 3 );
        read_SDL_vecus_RW( rw, ptri->st, 3 );
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
SDL_md2_vertex_t * load_SDL_md2_vertex_RW( SDL_RWops * rw, SDL_md2_vertex_t * pdata, size_t count )
{
    int i;

    if ( NULL == pdata ) return pdata;

    memset( pdata, 0, count * sizeof( SDL_md2_vertex_t ) );

    if ( NULL == rw ) return pdata;

    for ( i = 0; i < count; ++i )
    {
        SDL_md2_vertex_t * pvrt = pdata + i;

        read_SDL_vecuc_RW( rw, pvrt->v, SDL_arraysize( pdata->v ) );
        SDL_RWread( rw, &( pvrt->normalIndex ), 1, 1 );
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
SDL_md2_frame_t * load_SDL_md2_frame_RW( SDL_RWops * rw, SDL_md2_frame_t * pdata, size_t fcount, size_t vcount )
{
    int i;

    if ( NULL == pdata ) return pdata;

    SDL_md2_frame_free( pdata, fcount );

    if ( NULL == rw ) return pdata;

    /* Read frames */
    for ( i = 0; i < fcount; ++i )
    {
        SDL_md2_frame_t * pframe = pdata + i;

        /* Memory allocation for vertices of this frame */
        pframe->verts = ( SDL_md2_vertex_t * ) SDL_calloc( vcount, sizeof( SDL_md2_vertex_t ) );

        /* Read frame data */
        read_SDL_vecf_RW( rw, pframe->scale,     SDL_arraysize( pframe->scale ) );
        read_SDL_vecf_RW( rw, pframe->translate, SDL_arraysize( pframe->translate ) );
        read_SDL_string_RW( rw, pframe->name,      SDL_arraysize( pframe->name ) );
        load_SDL_md2_vertex_RW( rw, pframe->verts,     vcount );
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
Sint32 * load_SDL_glcmd_RW( SDL_RWops * rw, Sint32 * pdata, size_t size )
{
    // BB> this reads the raw command data. It will need to be scanned, switched to the
    //     proper endian type, and decoded to be used.

    int i;

    if ( NULL == pdata ) return pdata;

    memset( pdata, 0, size * sizeof( Sint32 ) );

    if ( NULL == rw ) return pdata;

    for ( i = 0; i < size; i++ )
    {
        pdata[i] = SDL_ReadLE32( rw );
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SDL_md2_model_t * SDL_md2_new( SDL_md2_model_t * mdl )
{
    if ( NULL != mdl )
    {
        memset( mdl, 0, sizeof( SDL_md2_model_t ) );
    }

    return mdl;
}

//--------------------------------------------------------------------------------------------
void SDL_md2_free( SDL_md2_model_t * mdl )
{
    int i;

    if ( mdl->skins )
    {
        SDL_free( mdl->skins );
        mdl->skins = NULL;
    }

    if ( mdl->texcoords )
    {
        SDL_free( mdl->texcoords );
        mdl->texcoords = NULL;
    }

    if ( mdl->triangles )
    {
        SDL_free( mdl->triangles );
        mdl->triangles = NULL;
    }

    if ( mdl->glcmds )
    {
        SDL_free( mdl->glcmds );
        mdl->glcmds = NULL;
    }

    if ( mdl->frames )
    {
        for ( i = 0; i < mdl->header.num_frames; ++i )
        {
            SDL_free( mdl->frames[i].verts );
            mdl->frames[i].verts = NULL;
        }

        SDL_free( mdl->frames );
        mdl->frames = NULL;
    }
}

//--------------------------------------------------------------------------------------------
SDL_md2_model_t * SDL_md2_load_RW( SDL_RWops * rw, SDL_md2_model_t * pdata )
{
    // BB> generate the md2 data from the RWops

    char error_str[256] = { "no error" };

    // rewind the RWops
    SDL_RWseek( rw, 0, RW_SEEK_SET );

    // if the data does not exist, create it
    if ( NULL == pdata )
    {
        pdata = ( SDL_md2_model_t* )SDL_calloc( 1, sizeof( SDL_md2_model_t ) );
    }
    if ( NULL == pdata ) return pdata;

    // load the header
    load_SDL_md2_header_RW( rw, &( pdata->header ) );

    if ( MD2_MAGIC_NUMBER != pdata->header.ident )
    {
        /* Error! */
        sprintf( error_str, "Error: bad identifier\n" );
        goto load_SDL_md2_model_RW_error;
    }

    // allocate all the dynamic memory
    SDL_md2_alloc( pdata );

    // load the various chunks
    SDL_RWseek( rw, pdata->header.offset_skins, RW_SEEK_SET );
    load_SDL_md2_skin_RW( rw, pdata->skins, pdata->header.num_skins );

    SDL_RWseek( rw, pdata->header.offset_st, RW_SEEK_SET );
    load_SDL_md2_texcoord_RW( rw, pdata->texcoords, pdata->header.num_st );

    SDL_RWseek( rw, pdata->header.offset_tris, RW_SEEK_SET );
    load_SDL_md2_triangle_RW( rw, pdata->triangles, pdata->header.num_tris );

    SDL_RWseek( rw, pdata->header.offset_glcmds, RW_SEEK_SET );
    load_SDL_glcmd_RW( rw, pdata->glcmds, pdata->header.size_glcmds );

    SDL_RWseek( rw, pdata->header.offset_frames, SEEK_SET );
    load_SDL_md2_frame_RW( rw, pdata->frames, pdata->header.num_frames, pdata->header.num_vertices );

    return pdata;

load_SDL_md2_model_RW_error:

    SDL_md2_free( pdata );

    fprintf( stderr, "%s", error_str );
    SDL_RWclose( rw );

    return pdata;
}

//--------------------------------------------------------------------------------------------
//SDL_md2_model_t * SDL_md2_load (const char *filename, SDL_md2_model_t * mdl)
//{
//    char error_str[256] = { "no error" };
//  SDL_RWops *rw;

//    // clear out any old data
//    SDL_md2_free( mdl );

//    // try to open the file
//    if( INVALID_CSTR(filename) ) return NULL;
//  rw = SDL_RWFromFile (filename, "rb");
//  if (NULL == rw)
//  {
//      snprintf( error_str, SDL_arraysize( error_str), "Error: couldn't open \"%s\"!\n", filename );
//      goto SDL_md2_load_error;
//  }

//    mdl = SDL_md2_load_RW( rw, mdl );

//    SDL_RWclose(rw);

//    return mdl;

//SDL_md2_load_error:
//    vfs_printf (stderr, "%s", error_str);
//    SDL_RWclose(rw);
//    return mdl;
//}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SDL_bool SDL_md2_alloc( SDL_md2_model_t * mdl )
{
    if ( NULL == mdl ) return SDL_FALSE;

    // free any allocated memory
    SDL_md2_free( mdl );

    // Memory allocations
    mdl->skins = ( SDL_md2_skin_t * ) SDL_calloc( mdl->header.num_skins, sizeof( SDL_md2_skin_t ) );
    if ( NULL == mdl->skins ) goto SDL_md2_alloc_fail;

    mdl->texcoords = ( SDL_md2_texcoord_t * ) SDL_calloc( mdl->header.num_st, sizeof( SDL_md2_texcoord_t ) );
    if ( NULL == mdl->texcoords ) goto SDL_md2_alloc_fail;

    mdl->triangles = ( SDL_md2_triangle_t * ) SDL_calloc( mdl->header.num_tris, sizeof( SDL_md2_triangle_t ) );
    if ( NULL == mdl->triangles ) goto SDL_md2_alloc_fail;

    mdl->frames = ( SDL_md2_frame_t * ) SDL_calloc( mdl->header.num_frames, sizeof( SDL_md2_frame_t ) );
    if ( NULL == mdl->frames ) goto SDL_md2_alloc_fail;

    mdl->glcmds = ( Sint32 * ) SDL_calloc( mdl->header.num_glcmds, sizeof( Sint32 ) );
    if ( NULL == mdl->glcmds ) goto SDL_md2_alloc_fail;

    return  SDL_TRUE;

SDL_md2_alloc_fail:

    // unwind any memory allocations
    SDL_md2_free( mdl );

    return SDL_FALSE;
};

//--------------------------------------------------------------------------------------------
SDL_bool SDL_md2_frame_free( SDL_md2_frame_t * pdata, size_t fcount )
{
    int i;

    if ( NULL == pdata ) return SDL_FALSE;

    for ( i = 0; i < fcount; ++i )
    {
        SDL_md2_frame_t * pframe = pdata + i;

        if ( NULL != pframe->verts )
        {
            SDL_free( pframe->verts );
            pframe->verts = NULL;
        }
    }

    return SDL_TRUE;
}


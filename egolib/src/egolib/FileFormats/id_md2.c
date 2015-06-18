//********************************************************************************************
//*
//*    This file is part of id_md2 reader library. This library is
//*    distributed with Egoboo.
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

/// @file egolib/FileFormats/id_md2.c
/// @brief A raw reader and writer for ID software's .md2 model type
/// @details

#include "egolib/FileFormats/id_md2.h"

#include "egolib/vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/* Table of precalculated normals */
float kid_md2_normals[MD2_MAX_NORMALS][3] =
{
#    include "id_normals.inl"
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Load an MD2 model from file.
///
/// @note MD2 format stores model's data in little-endian ordering.  On
/// big-endian machines, you'll have to perform proper conversions.

id_md2_model_t * id_md2_load( const char *filename, id_md2_model_t * mdl )
{
    FILE *fp;
    int i;

    fp = fopen( filename, "rb" );
    if ( !fp )
    {
        fprintf( stderr, "Error: couldn't open \"%s\"!\n", filename );
        return 0;
    }

    if ( NULL == mdl )
    {
        mdl = ( id_md2_model_t* )calloc( 1, sizeof( id_md2_model_t ) );
    }

    if ( NULL == mdl ) {
        fclose(fp);
        return NULL;
    }

    /* Read header */
    if(fread( &mdl->header, 1, sizeof( id_md2_header_t ), fp ) != sizeof(id_md2_header_t)) {
        fprintf(stderr, "Error: Unexpected EOF when reading header\n");
        fclose(fp);
        free(mdl);
        return nullptr;
    }

    if (( mdl->header.ident != MD2_MAGIC_NUMBER ) ||
        ( mdl->header.version != MD2_VERSION ) )
    {
        /* Error! */
        fprintf( stderr, "Error: bad version or identifier\n" );
        fclose( fp );
        free(mdl);
        return nullptr;
    }

    /* Memory allocations */
    mdl->skins     = ( id_md2_skin_t     * ) calloc( mdl->header.num_skins,   sizeof( id_md2_skin_t ) );
    mdl->texcoords = ( id_md2_texcoord_t * ) calloc( mdl->header.num_st,      sizeof( id_md2_texcoord_t ) );
    mdl->triangles = ( id_md2_triangle_t * ) calloc( mdl->header.num_tris,    sizeof( id_md2_triangle_t ) );
    mdl->frames    = ( id_md2_frame_t    * ) calloc( mdl->header.num_frames,  sizeof( id_md2_frame_t ) );
    mdl->glcmds    = ( int32_t           * ) calloc( mdl->header.size_glcmds, sizeof( int32_t ) );

    /* Read model data */
    fseek( fp, mdl->header.offset_skins, SEEK_SET );
    if(fread( mdl->skins, sizeof(id_md2_skin_t), mdl->header.num_skins, fp ) != sizeof(id_md2_skin_t)) {
        fprintf(stderr, "Error: skin data corrupted\n");
        fclose(fp);
        free(mdl);
        return nullptr;
    }

    fseek( fp, mdl->header.offset_st, SEEK_SET );
    if(fread( mdl->texcoords, sizeof(id_md2_texcoord_t), mdl->header.num_st, fp ) != sizeof(id_md2_texcoord_t)) {
        fprintf(stderr, "Error: texture coordinates corrupted\n");
        fclose(fp);
        free(mdl);
        return nullptr;
    }

    fseek( fp, mdl->header.offset_tris, SEEK_SET );
    if(fread( mdl->triangles, sizeof(id_md2_triangle_t), mdl->header.num_tris, fp ) != sizeof(id_md2_triangle_t)) {
        fprintf(stderr, "Error: triangles corrupted\n");
        fclose(fp);
        free(mdl);
        return nullptr;
    }

    fseek( fp, mdl->header.offset_glcmds, SEEK_SET );
    if(fread( mdl->glcmds, sizeof(int32_t), mdl->header.size_glcmds, fp ) != sizeof(int32_t)) {
        fprintf(stderr, "Error: GL commands corrupted\n");
        fclose(fp);
        free(mdl);
        return nullptr;
    }

    /* Read frames */
    fseek( fp, mdl->header.offset_frames, SEEK_SET );
    for ( i = 0; i < mdl->header.num_frames; ++i )
    {
        /* Memory allocation for vertices of this frame */
        mdl->frames[i].verts = ( id_md2_vertex_t * )calloc( mdl->header.num_vertices, sizeof( id_md2_vertex_t ) );

        /* Read frame data */
        size_t read = 0;
        read += fread( mdl->frames[i].scale, sizeof( float ), 3, fp );
        read += fread( mdl->frames[i].translate, sizeof( float ), 3, fp );
        read += fread( mdl->frames[i].name, sizeof( char ), 16, fp );
        read += fread( mdl->frames[i].verts, sizeof( id_md2_vertex_t ), mdl->header.num_vertices, fp );

        if(read != sizeof(float) + sizeof(float) +  sizeof(char) + sizeof(id_md2_vertex_t)) {
            fprintf(stderr, "Error: Frame data corrupted\n");
            fclose(fp);
            free(mdl);
            return nullptr;            
        }
    }

    fclose( fp );
    return mdl;
}

//--------------------------------------------------------------------------------------------
// Free resources allocated for the model.

void id_md2_free( id_md2_model_t * mdl )
{
    int i;

    if ( mdl->skins )
    {
        free( mdl->skins );
        mdl->skins = NULL;
    }

    if ( mdl->texcoords )
    {
        free( mdl->texcoords );
        mdl->texcoords = NULL;
    }

    if ( mdl->triangles )
    {
        free( mdl->triangles );
        mdl->triangles = NULL;
    }

    if ( mdl->glcmds )
    {
        free( mdl->glcmds );
        mdl->glcmds = NULL;
    }

    if ( mdl->frames )
    {
        for ( i = 0; i < mdl->header.num_frames; ++i )
        {
            free( mdl->frames[i].verts );
            mdl->frames[i].verts = NULL;
        }

        free( mdl->frames );
        mdl->frames = NULL;
    }
}

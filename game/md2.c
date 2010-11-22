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

///
/// @file md2.c
/// @brief Raw MD2 loader
/// @details Raw model loader for ID Software's MD2 file format

#include "md2.inl"

#include "log.h"

#include "egoboo_endian.h"
#include "egoboo_math.inl"

#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float kMd2Normals[EGO_NORMAL_COUNT][3] =
{
#include "id_normals.inl"
    , {0, 0, 0}                     ///< the "equal light" normal
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static MD2_Frame_t * MD2_Frame_ctor( MD2_Frame_t * pframe, size_t size );
static MD2_Frame_t * MD2_Frame_dtor( MD2_Frame_t * pframe );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
MD2_Frame_t * MD2_Frame_ctor( MD2_Frame_t * pframe, size_t size )
{
    if ( NULL == pframe ) return pframe;

    memset( pframe, 0, sizeof( *pframe ) );

    pframe->vertex_lst = EGOBOO_NEW_ARY( MD2_Vertex_t, size );
    if ( NULL != pframe->vertex_lst ) pframe->vertex_count = size;

    return pframe;
}

//--------------------------------------------------------------------------------------------
MD2_Frame_t * MD2_Frame_dtor( MD2_Frame_t * pframe )
{
    if ( NULL == pframe ) return pframe;

    EGOBOO_DELETE_ARY( pframe->vertex_lst );
    pframe->vertex_count = 0;

    memset( pframe, 0, sizeof( *pframe ) );

    return pframe;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void MD2_GLCommand_ctor( MD2_GLCommand_t * m )
{
    if ( NULL == m ) return;

    memset( m, 0, sizeof( *m ) );

    m->next = NULL;
    m->data = NULL;
}

//--------------------------------------------------------------------------------------------
void MD2_GLCommand_dtor( MD2_GLCommand_t * m )
{
    if ( NULL == m ) return;

    EGOBOO_DELETE_ARY( m->data );

    memset( m, 0, sizeof( *m ) );
}

//--------------------------------------------------------------------------------------------
MD2_GLCommand_t * MD2_GLCommand_create()
{
    MD2_GLCommand_t * m;

    m = EGOBOO_NEW( MD2_GLCommand_t );

    MD2_GLCommand_ctor( m );

    return m;
}

//--------------------------------------------------------------------------------------------
MD2_GLCommand_t * MD2_GLCommand_new_vector( int n )
{
    int i;
    MD2_GLCommand_t * v = EGOBOO_NEW_ARY( MD2_GLCommand_t, n );
    for ( i = 0; i < n; i++ ) MD2_GLCommand_ctor( v + i );
    return v;
}

//--------------------------------------------------------------------------------------------
void MD2_GLCommand_destroy( MD2_GLCommand_t ** m )
{
    if ( NULL == m || NULL == * m ) return;

    MD2_GLCommand_dtor( *m );

    EGOBOO_DELETE( *m );
}

//--------------------------------------------------------------------------------------------
void MD2_GLCommand_delete_list( MD2_GLCommand_t * command_ptr, int command_count )
{
    int cnt;

    if ( NULL == command_ptr ) return;

    for ( cnt = 0; cnt < command_count && NULL != command_ptr; cnt++ )
    {
        MD2_GLCommand_t * tmp = command_ptr;
        command_ptr = command_ptr->next;

        MD2_GLCommand_destroy( &tmp );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
MD2_Model_t * MD2_Model_ctor( MD2_Model_t * m )
{
    if ( NULL == m ) return m;

    memset( m, 0, sizeof( *m ) );

    return m;
}

//--------------------------------------------------------------------------------------------
void md2_free( MD2_Model_t * m )
{
    EGOBOO_DELETE_ARY( m->m_skins );
    m->m_numSkins = 0;

    EGOBOO_DELETE_ARY( m->m_texCoords );
    m->m_numTexCoords = 0;

    EGOBOO_DELETE_ARY( m->m_triangles );
    m->m_numTriangles = 0;

    if ( NULL != m->m_frames )
    {
        int i;
        for ( i = 0; i < m->m_numFrames; i++ )
        {
            MD2_Frame_dtor( m->m_frames + i );
        }

        EGOBOO_DELETE_ARY( m->m_frames );
        m->m_numFrames = 0;
    }

    MD2_GLCommand_delete_list( m->m_commands, m->m_numCommands );
    m->m_commands = NULL;
    m->m_numCommands = 0;
}

//--------------------------------------------------------------------------------------------
MD2_Model_t * MD2_Model_dtor( MD2_Model_t * m )
{
    if ( NULL == m ) return NULL;

    md2_free( m );

    return m;
}

//--------------------------------------------------------------------------------------------
MD2_Model_t * MD2_Model_create()
{
    MD2_Model_t * m = EGOBOO_NEW( MD2_Model_t );

    MD2_Model_ctor( m );

    return m;
}

//--------------------------------------------------------------------------------------------
MD2_Model_t * MD2_Model_new_vector( int n )
{
    int i;
    MD2_Model_t * v = EGOBOO_NEW_ARY( MD2_Model_t, n );
    for ( i = 0; i < n; i++ ) MD2_Model_ctor( v + i );
    return v;
}

//--------------------------------------------------------------------------------------------
void MD2_Model_destroy( MD2_Model_t ** m )
{
    if ( NULL == m || NULL == *m ) return;

    MD2_Model_dtor( *m );

    EGOBOO_DELETE( *m );
}

//--------------------------------------------------------------------------------------------
void MD2_Model_delete_vector( MD2_Model_t * v, int n )
{
    int i;
    if ( NULL == v || 0 == n ) return;
    for ( i = 0; i < n; i++ ) MD2_Model_dtor( v + i );
    EGOBOO_DELETE( v );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void md2_scale_model( MD2_Model_t * pmd2, float scale_x, float scale_y, float scale_z )
{
    /// @details BB@> scale every vertex in the md2 by the given amount

    int cnt, tnc;
    int num_frames, num_verts;
    MD2_Frame_t * pframe;

    num_frames = pmd2->m_numFrames;
    num_verts  = pmd2->m_numVertices;

    for ( cnt = 0; cnt < num_frames; cnt++ )
    {
        bool_t bfound;

        pframe = pmd2->m_frames + cnt;

        bfound = bfalse;
        for ( tnc = 0; tnc  < num_verts; tnc++ )
        {
            oct_vec_t opos;

            pframe->vertex_lst[tnc].pos.x *= scale_x;
            pframe->vertex_lst[tnc].pos.y *= scale_y;
            pframe->vertex_lst[tnc].pos.z *= scale_z;

            pframe->vertex_lst[tnc].nrm.x *= SGN( scale_x );
            pframe->vertex_lst[tnc].nrm.y *= SGN( scale_y );
            pframe->vertex_lst[tnc].nrm.z *= SGN( scale_z );

            pframe->vertex_lst[tnc].nrm = fvec3_normalize( pframe->vertex_lst[tnc].nrm.v );

            oct_vec_ctor( opos, pframe->vertex_lst[tnc].pos.v );

            // Re-calculate the bounding box for this frame
            if ( !bfound )
            {
                oct_bb_set_ovec( &( pframe->bb ), opos );
                bfound = btrue;
            }
            else
            {
                oct_bb_self_sum_ovec( &( pframe->bb ), opos );
            }
        }

        // we don't really want objects that have extent in more than one
        // dimension to be called empty
        if ( pframe->bb.empty )
        {
            if ( ABS( pframe->bb.maxs[OCT_X] - pframe->bb.mins[OCT_X] ) +
                 ABS( pframe->bb.maxs[OCT_Y] - pframe->bb.mins[OCT_Y] ) +
                 ABS( pframe->bb.maxs[OCT_Z] - pframe->bb.mins[OCT_Z] ) > 0.0f )
            {
                oct_vec_t ovec;

                ovec[OCT_X] = ovec[OCT_Y] = ovec[OCT_Z] = 1e-6;
                ovec[OCT_XY] = ovec[OCT_YX] = SQRT_TWO * ovec[OCT_X];
                oct_bb_self_grow( &( pframe->bb ), ovec );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
MD2_Model_t* md2_load( const char * szFilename, MD2_Model_t* mdl )
{
    FILE * f;
    int i, v;
    bool_t bfound;

    id_md2_header_t md2_header;
    MD2_Model_t    *model;

    // Open up the file, and make sure it's a MD2 model
    f = fopen( szFilename, "rb" );
    if ( NULL == f )
    {
        log_warning( "md2_load() - could not open model (%s)\n", szFilename );
        return NULL;
    }

    fread( &md2_header, sizeof( md2_header ), 1, f );

    // Convert the byte ordering in the md2_header, if we need to
    md2_header.ident            = ENDIAN_INT32( md2_header.ident );
    md2_header.version          = ENDIAN_INT32( md2_header.version );
    md2_header.skinwidth        = ENDIAN_INT32( md2_header.skinwidth );
    md2_header.skinheight       = ENDIAN_INT32( md2_header.skinheight );
    md2_header.framesize        = ENDIAN_INT32( md2_header.framesize );
    md2_header.num_skins        = ENDIAN_INT32( md2_header.num_skins );
    md2_header.num_vertices     = ENDIAN_INT32( md2_header.num_vertices );
    md2_header.num_st           = ENDIAN_INT32( md2_header.num_st );
    md2_header.num_tris         = ENDIAN_INT32( md2_header.num_tris );
    md2_header.size_glcmds      = ENDIAN_INT32( md2_header.size_glcmds );
    md2_header.num_frames       = ENDIAN_INT32( md2_header.num_frames );
    md2_header.offset_skins     = ENDIAN_INT32( md2_header.offset_skins );
    md2_header.offset_st        = ENDIAN_INT32( md2_header.offset_st );
    md2_header.offset_tris      = ENDIAN_INT32( md2_header.offset_tris );
    md2_header.offset_frames    = ENDIAN_INT32( md2_header.offset_frames );
    md2_header.offset_glcmds    = ENDIAN_INT32( md2_header.offset_glcmds );
    md2_header.offset_end       = ENDIAN_INT32( md2_header.offset_end );

    if ( md2_header.ident != MD2_MAGIC_NUMBER || md2_header.version != MD2_VERSION )
    {
        fclose( f );
        log_warning( "md2_load() - model does not have valid header or identifier (%s)\n", szFilename );
        return NULL;
    }

    // Allocate a MD2_Model_t to hold all this stuff
    model = ( NULL == mdl ) ? MD2_Model_create() : mdl;
    if ( NULL == model )
    {
        log_error( "md2_load() - could create MD2_Model_t\n" );
        return NULL;
    }

    model->m_numVertices  = md2_header.num_vertices;
    model->m_numTexCoords = md2_header.num_st;
    model->m_numTriangles = md2_header.num_tris;
    model->m_numSkins     = md2_header.num_skins;
    model->m_numFrames    = md2_header.num_frames;

    model->m_texCoords = EGOBOO_NEW_ARY( MD2_TexCoord_t, md2_header.num_st );
    model->m_triangles = EGOBOO_NEW_ARY( MD2_Triangle_t, md2_header.num_tris );
    model->m_skins     = EGOBOO_NEW_ARY( MD2_SkinName_t, md2_header.num_skins );
    model->m_frames    = EGOBOO_NEW_ARY( MD2_Frame_t, md2_header.num_frames );

    for ( i = 0; i < md2_header.num_frames; i++ )
    {
        MD2_Frame_ctor( model->m_frames + i, md2_header.num_vertices );
    }

    // Load the texture coordinates from the file, normalizing them as we go
    fseek( f, md2_header.offset_st, SEEK_SET );
    for ( i = 0; i < md2_header.num_st; i++ )
    {
        id_md2_texcoord_t tc;
        fread( &tc, sizeof( tc ), 1, f );

        // auto-convert the byte ordering of the texture coordinates
        tc.s = ENDIAN_INT16( tc.s );
        tc.t = ENDIAN_INT16( tc.t );

        model->m_texCoords[i].tex.s = tc.s / ( float )md2_header.skinwidth;
        model->m_texCoords[i].tex.t = tc.t / ( float )md2_header.skinheight;
    }

    // Load triangles from the file.  I use the same memory layout as the file
    // on a little endian machine, so they can just be read directly
    fseek( f, md2_header.offset_tris, SEEK_SET );
    fread( model->m_triangles, sizeof( id_md2_triangle_t ), md2_header.num_tris, f );

    // auto-convert the byte ordering on the triangles
    for ( i = 0; i < md2_header.num_tris; i++ )
    {
        for ( v = 0; v < 3; v++ )
        {
            model->m_triangles[i].vertex[v] = ENDIAN_INT16( model->m_triangles[i].vertex[v] );
            model->m_triangles[i].st[v]     = ENDIAN_INT16( model->m_triangles[i].st[v] );
        }
    }

    // Load the skin names.  Again, I can load them directly
    fseek( f, md2_header.offset_skins, SEEK_SET );
    fread( model->m_skins, sizeof( id_md2_skin_t ), md2_header.num_skins, f );

    // Load the frames of animation
    fseek( f, md2_header.offset_frames, SEEK_SET );
    for ( i = 0; i < md2_header.num_frames; i++ )
    {
        id_md2_frame_header_t frame_header;

        // read the current frame
        fread( &frame_header, sizeof( frame_header ), 1, f );

        // Convert the byte ordering on the scale & translate vectors, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
        frame_header.scale[0]     = ENDIAN_FLOAT( frame_header.scale[0] );
        frame_header.scale[1]     = ENDIAN_FLOAT( frame_header.scale[1] );
        frame_header.scale[2]     = ENDIAN_FLOAT( frame_header.scale[2] );

        frame_header.translate[0] = ENDIAN_FLOAT( frame_header.translate[0] );
        frame_header.translate[1] = ENDIAN_FLOAT( frame_header.translate[1] );
        frame_header.translate[2] = ENDIAN_FLOAT( frame_header.translate[2] );
#endif

        // unpack the md2 vertex_lst from this frame
        bfound = bfalse;
        for ( v = 0; v < md2_header.num_vertices; v++ )
        {
            oct_vec_t ovec;
            MD2_Frame_t   * pframe;
            id_md2_vertex_t frame_vert;

            // read vertex_lst one-by-one. I hope this is not endian dependent, but I have no way to check it.
            fread( &frame_vert, sizeof( id_md2_vertex_t ), 1, f );

            pframe = model->m_frames + i;

            // grab the vertex position
            pframe->vertex_lst[v].pos.x = frame_vert.v[0] * frame_header.scale[0] + frame_header.translate[0];
            pframe->vertex_lst[v].pos.y = frame_vert.v[1] * frame_header.scale[1] + frame_header.translate[1];
            pframe->vertex_lst[v].pos.z = frame_vert.v[2] * frame_header.scale[2] + frame_header.translate[2];

            // grab the normal index
            pframe->vertex_lst[v].normal = frame_vert.normalIndex;
            if ( pframe->vertex_lst[v].normal > EGO_AMBIENT_INDEX ) pframe->vertex_lst[v].normal = EGO_AMBIENT_INDEX;

            // expand the normal index into an actual normal
            pframe->vertex_lst[v].nrm.x = kMd2Normals[frame_vert.normalIndex][0];
            pframe->vertex_lst[v].nrm.y = kMd2Normals[frame_vert.normalIndex][1];
            pframe->vertex_lst[v].nrm.z = kMd2Normals[frame_vert.normalIndex][2];

            // Calculate the bounding box for this frame
            oct_vec_ctor( ovec, pframe->vertex_lst[v].pos.v );
            if ( !bfound )
            {
                oct_bb_set_ovec( &( pframe->bb ), ovec );
                bfound = btrue;
            }
            else
            {
                oct_bb_self_sum_ovec( &( pframe->bb ), ovec );
            }
        }

        //make sure to copy the frame name!
        strncpy( model->m_frames[i].name, frame_header.name, 16 );
    }

    //Load up the pre-computed OpenGL optimizations
    if ( md2_header.size_glcmds > 0 )
    {
        Uint32            cmd_cnt = 0, cmd_size;
        MD2_GLCommand_t * cmd     = NULL;
        fseek( f, md2_header.offset_glcmds, SEEK_SET );

        //count the commands
        cmd_size = 0;
        while ( cmd_size < md2_header.size_glcmds )
        {
            Sint32 commands;

            fread( &commands, sizeof( Sint32 ), 1, f );
            cmd_size += sizeof( Sint32 ) / sizeof( Uint32 );

            // auto-convert the byte ordering
            commands = ENDIAN_INT32( commands );

            if ( 0 == commands || cmd_size == md2_header.size_glcmds ) break;

            cmd = MD2_GLCommand_create();
            cmd->command_count = commands;

            //set the GL drawing mode
            if ( cmd->command_count > 0 )
            {
                cmd->gl_mode = GL_TRIANGLE_STRIP;
            }
            else
            {
                cmd->command_count = -cmd->command_count;
                cmd->gl_mode = GL_TRIANGLE_FAN;
            }

            //allocate the data
            cmd->data = EGOBOO_NEW_ARY( id_glcmd_packed_t, cmd->command_count );

            //read in the data
            fread( cmd->data, sizeof( id_glcmd_packed_t ), cmd->command_count, f );
            cmd_size += ( sizeof( id_glcmd_packed_t ) * cmd->command_count ) / sizeof( Uint32 );

            //translate the data, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
            {
                int i;
                for ( i = 0; i < cmd->command_count; i++ )
                {
                    cmd->data[i].index = SDL_swap32( cmd->data[i].s );
                    cmd->data[i].s     = ENDIAN_FLOAT( cmd->data[i].s );
                    cmd->data[i].t     = ENDIAN_FLOAT( cmd->data[i].t );
                };
            }
#endif

            // attach it to the command list
            cmd->next         = model->m_commands;
            model->m_commands = cmd;

            cmd_cnt += cmd->command_count;
        };

        model->m_numCommands = cmd_cnt;
    }

    // Close the file, we're done with it
    fclose( f );

    return model;
}


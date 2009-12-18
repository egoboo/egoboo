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
/// @file
/// @brief Raw MD2 loader
/// @details Raw model loader for ID Software's MD2 file format

#include "Md2.inl"

#include "id_md2.h"

#include "egoboo_endian.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float kMd2Normals[EGO_NORMAL_COUNT][3] =
{
#include "id_normals.inl"
    , {0, 0, 0}                     ///< the "equal light" normal
};

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
    if ( NULL == f ) return NULL;

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
        return NULL;
    }

    // Allocate a MD2_Model_t to hold all this stuff
    model = ( NULL == mdl ) ? md2_new() : mdl;
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
        model->m_frames[i].vertices = EGOBOO_NEW_ARY( MD2_Vertex_t, md2_header.num_vertices );
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

        // unpack the md2 vertices from this frame
        bfound = bfalse;
        for ( v = 0; v < md2_header.num_vertices; v++ )
        {
            MD2_Frame_t   * pframe;
            id_md2_vertex_t frame_vert;

            // read vertices one-by-one. I hope this is not endian dependent, but I have no way to check it.
            fread( &frame_vert, sizeof( id_md2_vertex_t ), 1, f );

            pframe = model->m_frames + i;

            // grab the vertex position
            pframe->vertices[v].pos.x = frame_vert.v[0] * frame_header.scale[0] + frame_header.translate[0];
            pframe->vertices[v].pos.y = frame_vert.v[1] * frame_header.scale[1] + frame_header.translate[1];
            pframe->vertices[v].pos.z = frame_vert.v[2] * frame_header.scale[2] + frame_header.translate[2];

            // grab the normal index
            pframe->vertices[v].normal = frame_vert.normalIndex;
            if ( pframe->vertices[v].normal > EGO_AMBIENT_INDEX ) pframe->vertices[v].normal = EGO_AMBIENT_INDEX;

            // expand the normal index into an actual normal
            pframe->vertices[v].nrm.x = kMd2Normals[frame_vert.normalIndex][0];
            pframe->vertices[v].nrm.y = kMd2Normals[frame_vert.normalIndex][1];
            pframe->vertices[v].nrm.z = kMd2Normals[frame_vert.normalIndex][2];

            // Calculate the bounding box for this frame
            if ( !bfound )
            {
                pframe->bb.mins[OCT_X ] = pframe->bb.maxs[OCT_X ] =  pframe->vertices[v].pos.x;
                pframe->bb.mins[OCT_Y ] = pframe->bb.maxs[OCT_Y ] =  pframe->vertices[v].pos.y;
                pframe->bb.mins[OCT_XY] = pframe->bb.maxs[OCT_XY] =  pframe->vertices[v].pos.x + pframe->vertices[v].pos.y;
                pframe->bb.mins[OCT_YX] = pframe->bb.maxs[OCT_YX] = -pframe->vertices[v].pos.x + pframe->vertices[v].pos.y;
                pframe->bb.mins[OCT_Z ] = pframe->bb.maxs[OCT_Z ] =  pframe->vertices[v].pos.z;

                bfound = btrue;
            }
            else
            {
                pframe->bb.mins[OCT_X ] = MIN( pframe->bb.mins[OCT_X ], pframe->vertices[v].pos.x );
                pframe->bb.mins[OCT_Y ] = MIN( pframe->bb.mins[OCT_Y ], pframe->vertices[v].pos.y );
                pframe->bb.mins[OCT_XY] = MIN( pframe->bb.mins[OCT_XY], pframe->vertices[v].pos.x + pframe->vertices[v].pos.y );
                pframe->bb.mins[OCT_YX] = MIN( pframe->bb.mins[OCT_YX], -pframe->vertices[v].pos.x + pframe->vertices[v].pos.y );
                pframe->bb.mins[OCT_Z ] = MIN( pframe->bb.mins[OCT_Z ], pframe->vertices[v].pos.z );

                pframe->bb.maxs[OCT_X ] = MAX( pframe->bb.maxs[OCT_X ], pframe->vertices[v].pos.x );
                pframe->bb.maxs[OCT_Y ] = MAX( pframe->bb.maxs[OCT_Y ], pframe->vertices[v].pos.y );
                pframe->bb.maxs[OCT_XY] = MAX( pframe->bb.maxs[OCT_XY], pframe->vertices[v].pos.x + pframe->vertices[v].pos.y );
                pframe->bb.maxs[OCT_YX] = MAX( pframe->bb.maxs[OCT_YX], -pframe->vertices[v].pos.x + pframe->vertices[v].pos.y );
                pframe->bb.maxs[OCT_Z ] = MAX( pframe->bb.maxs[OCT_Z ], pframe->vertices[v].pos.z );
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

            cmd = MD2_GLCommand_new();
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
            for ( int i = 0; i < cmd->command_count; i++ )
            {
                cmd->data[i].index = SDL_swap32( cmd->data[i].s );
                cmd->data[i].s     = ENDIAN_FLOAT( cmd->data[i].s );
                cmd->data[i].t     = ENDIAN_FLOAT( cmd->data[i].t );
            };
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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void MD2_GLCommand_construct( MD2_GLCommand_t * m )
{
    m->next = NULL;
    m->data = NULL;
}

//--------------------------------------------------------------------------------------------
void MD2_GLCommand_destruct( MD2_GLCommand_t * m )
{
    if ( NULL == m ) return;

    if ( NULL != m->next )
    {
        MD2_GLCommand_delete( m->next );
        m->next = NULL;
    };

    EGOBOO_DELETE( m->data );
}

//--------------------------------------------------------------------------------------------
MD2_GLCommand_t * MD2_GLCommand_new()
{
    MD2_GLCommand_t * m;
    //fprintf( stdout, "MD2_GLCommand_new()\n");

    m = EGOBOO_NEW( MD2_GLCommand_t );
    MD2_GLCommand_construct( m );
    return m;
}

//--------------------------------------------------------------------------------------------
MD2_GLCommand_t * MD2_GLCommand_new_vector( int n )
{
    int i;
    MD2_GLCommand_t * v = EGOBOO_NEW_ARY( MD2_GLCommand_t, n );
    for ( i = 0; i < n; i++ ) MD2_GLCommand_construct( v + i );
    return v;
}

//--------------------------------------------------------------------------------------------
void MD2_GLCommand_delete( MD2_GLCommand_t * m )
{
    if ( NULL == m ) return;
    MD2_GLCommand_destruct( m );
    EGOBOO_DELETE( m );
}

//--------------------------------------------------------------------------------------------
void MD2_GLCommand_delete_vector( MD2_GLCommand_t * v, int n )
{
    int i;
    if ( NULL == v || 0 == n ) return;
    for ( i = 0; i < n; i++ ) MD2_GLCommand_destruct( v + i );
    EGOBOO_DELETE( v );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void md2_construct( MD2_Model_t * m )
{
    m->m_numVertices  = 0;
    m->m_numTexCoords = 0;
    m->m_numTriangles = 0;
    m->m_numSkins     = 0;
    m->m_numFrames    = 0;

    m->m_skins     = NULL;
    m->m_texCoords = NULL;
    m->m_triangles = NULL;
    m->m_frames    = NULL;
    m->m_commands  = NULL;
}

//--------------------------------------------------------------------------------------------
void md2_deallocate( MD2_Model_t * m )
{
    EGOBOO_DELETE( m->m_skins );
    m->m_numSkins = 0;

    EGOBOO_DELETE( m->m_texCoords );
    m->m_numTexCoords = 0;

    EGOBOO_DELETE( m->m_triangles );
    m->m_numTriangles = 0;

    if ( NULL != m->m_frames )
    {
        int i;
        for ( i = 0; i < m->m_numFrames; i++ )
        {
            EGOBOO_DELETE( m->m_frames[i].vertices )
        }
        EGOBOO_DELETE( m->m_frames );
        m->m_numFrames = 0;
    }

    EGOBOO_DELETE( m->m_commands );
    m->m_numCommands = 0;
}

//--------------------------------------------------------------------------------------------
void md2_destruct( MD2_Model_t * m )
{
    if ( NULL == m ) return;
    md2_deallocate( m );
}

//--------------------------------------------------------------------------------------------
MD2_Model_t * md2_new()
{
    MD2_Model_t * m;

    //fprintf( stdout, "MD2_GLCommand_new()\n");
    m = EGOBOO_NEW( MD2_Model_t );
    md2_construct( m );

    return m;
}

//--------------------------------------------------------------------------------------------
MD2_Model_t * md2_new_vector( int n )
{
    int i;
    MD2_Model_t * v = EGOBOO_NEW_ARY( MD2_Model_t, n );
    for ( i = 0; i < n; i++ ) md2_construct( v + i );
    return v;
}

//--------------------------------------------------------------------------------------------
void md2_delete( MD2_Model_t * m )
{
    if ( NULL == m ) return;
    md2_destruct( m );
    EGOBOO_DELETE( m );
}

//--------------------------------------------------------------------------------------------
void md2_delete_vector( MD2_Model_t * v, int n )
{
    int i;
    if ( NULL == v || 0 == n ) return;
    for ( i = 0; i < n; i++ ) md2_destruct( v + i );
    EGOBOO_DELETE( v );
}

//--------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
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
            pframe->vertices[tnc].pos.x *= scale_x;
            pframe->vertices[tnc].pos.y *= scale_y;
            pframe->vertices[tnc].pos.z *= scale_z;

            pframe->vertices[tnc].nrm.x *= scale_x;
            pframe->vertices[tnc].nrm.y *= scale_y;
            pframe->vertices[tnc].nrm.z *= scale_z;

            // Re-calculate the bounding box for this frame
            if ( !bfound )
            {
                pframe->bb.mins[OCT_X ] = pframe->vertices[tnc].pos.x;
                pframe->bb.mins[OCT_Y ] = pframe->vertices[tnc].pos.y;
                pframe->bb.mins[OCT_XY] = pframe->vertices[tnc].pos.x + pframe->vertices[tnc].pos.y;
                pframe->bb.mins[OCT_YX] = -pframe->vertices[tnc].pos.x + pframe->vertices[tnc].pos.y;
                pframe->bb.mins[OCT_Z ] = pframe->vertices[tnc].pos.z;

                pframe->bb.maxs[OCT_X ] = pframe->vertices[tnc].pos.x;
                pframe->bb.maxs[OCT_Y ] = pframe->vertices[tnc].pos.y;
                pframe->bb.maxs[OCT_XY] = pframe->vertices[tnc].pos.x + pframe->vertices[tnc].pos.y;
                pframe->bb.maxs[OCT_YX] = -pframe->vertices[tnc].pos.x + pframe->vertices[tnc].pos.y;
                pframe->bb.maxs[OCT_Z ] = pframe->vertices[tnc].pos.z;

                bfound = btrue;
            }
            else
            {
                pframe->bb.mins[OCT_X ] = MIN( pframe->bb.mins[OCT_X ], pframe->vertices[tnc].pos.x );
                pframe->bb.mins[OCT_Y ] = MIN( pframe->bb.mins[OCT_Y ], pframe->vertices[tnc].pos.y );
                pframe->bb.mins[OCT_XY] = MIN( pframe->bb.mins[OCT_XY], pframe->vertices[tnc].pos.x + pframe->vertices[tnc].pos.y );
                pframe->bb.mins[OCT_YX] = MIN( pframe->bb.mins[OCT_YX], -pframe->vertices[tnc].pos.x + pframe->vertices[tnc].pos.y );
                pframe->bb.mins[OCT_Z ] = MIN( pframe->bb.mins[OCT_Z ], pframe->vertices[tnc].pos.z );

                pframe->bb.maxs[OCT_X ] = MAX( pframe->bb.maxs[OCT_X ], pframe->vertices[tnc].pos.x );
                pframe->bb.maxs[OCT_Y ] = MAX( pframe->bb.maxs[OCT_Y ], pframe->vertices[tnc].pos.y );
                pframe->bb.maxs[OCT_XY] = MAX( pframe->bb.maxs[OCT_XY], pframe->vertices[tnc].pos.x + pframe->vertices[tnc].pos.y );
                pframe->bb.maxs[OCT_YX] = MAX( pframe->bb.maxs[OCT_YX], -pframe->vertices[tnc].pos.x + pframe->vertices[tnc].pos.y );
                pframe->bb.maxs[OCT_Z ] = MAX( pframe->bb.maxs[OCT_Z ], pframe->vertices[tnc].pos.z );
            }
        }
    }
}

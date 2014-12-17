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

/// @file game/renderer_3d.c
/// @brief Implementation of the 3d renderer functions
/// @details

#include "game/renderer_3d.h"

#include "game/camera.h"
#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------
// INTERNAL VARIABLES
//--------------------------------------------------------------------------------------------

static line_data_t line_list[LINE_COUNT];
static point_data_t point_list[POINT_COUNT];

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------
void gfx_begin_3d( const camera_t * pcam )
{
    // store the GL_PROJECTION matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
	Egoboo_Renderer_OpenGL_loadMatrix( &(pcam->mProjection) );
    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	Egoboo_Renderer_OpenGL_loadMatrix( &(pcam->mView) );
}

//--------------------------------------------------------------------------------------------
void gfx_end_3d( void )
{
    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // Restore the GL_PROJECTION matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();
}

//--------------------------------------------------------------------------------------------
// LINE IMPLENTATION
//--------------------------------------------------------------------------------------------
void line_list_init()
{
    /// @details  BB@> initialize the list so that no lines are valid

    int cnt;

    for ( cnt = 0; cnt < LINE_COUNT; cnt++ )
    {
        line_list[cnt].time = -1;
    }
}

//--------------------------------------------------------------------------------------------
int line_list_get_free()
{
    /// @details  BB@> get the 1st free line

    int cnt;

    for ( cnt = 0; cnt < LINE_COUNT; cnt++ )
    {
        if ( line_list[cnt].time < 0 )
        {
            break;
        }
    }

    return cnt < LINE_COUNT ? cnt : -1;
}

//--------------------------------------------------------------------------------------------
bool line_list_add( const float src_x, const float src_y, const float src_z, const float pos_x, const float dst_y, const float dst_z, const int duration )
{
    int iline = line_list_get_free();

    if ( iline == LINE_COUNT ) return false;

    //Source
    line_list[iline].src.x = src_x;
    line_list[iline].src.y = src_y;
    line_list[iline].src.z = src_z;

    //Destination
    line_list[iline].dst.x = pos_x;
    line_list[iline].dst.y = dst_y;
    line_list[iline].dst.z = dst_z;

    //White color
    line_list[iline].color.r = 1.00f;
    line_list[iline].color.g = 1.00f;
    line_list[iline].color.b = 1.00f;
    line_list[iline].color.a = 0.50f;

    line_list[iline].time = egoboo_get_ticks() + duration;

    return true;
}

//--------------------------------------------------------------------------------------------
void line_list_draw_all( const camera_t * pcam )
{
    /// @author BB
    /// @details draw some lines for debugging purposes

    int cnt, ticks;

    GLboolean texture_1d_enabled, texture_2d_enabled;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    gfx_begin_3d( pcam );
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );     // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();   // GL_ENABLE_BIT

            GL_DEBUG( glDisable )( GL_BLEND );       // GL_ENABLE_BIT

            // we do not want texture mapped lines
            GL_DEBUG( glDisable )( GL_TEXTURE_2D );  // GL_ENABLE_BIT

            ticks = egoboo_get_ticks();

            for ( cnt = 0; cnt < LINE_COUNT; cnt++ )
            {
                if ( line_list[cnt].time < 0 ) continue;

                if ( line_list[cnt].time < ticks )
                {
                    line_list[cnt].time = -1;
                    continue;
                }

                GL_DEBUG( glColor4fv )( line_list[cnt].color.v );       // GL_CURRENT_BIT
                GL_DEBUG( glBegin )( GL_LINES );
                {
                    GL_DEBUG( glVertex3fv )( line_list[cnt].src.v );
                    GL_DEBUG( glVertex3fv )( line_list[cnt].dst.v );
                }
                GL_DEBUG_END();
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();

    // fix the texture enabling
    if ( texture_1d_enabled )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_1D );
    }
    else if ( texture_2d_enabled )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_2D );
    }

}

//--------------------------------------------------------------------------------------------
// POINT IMPLENTATION
//--------------------------------------------------------------------------------------------
void point_list_init()
{
    /// @details  BB@> initialize the list so that no points are valid

    int cnt;

    for ( cnt = 0; cnt < POINT_COUNT; cnt++ )
    {
        point_list[cnt].time = -1;
    }
}

//--------------------------------------------------------------------------------------------
bool point_list_add( const float x, const float y, const float z, const int duration )
{
    int ipoint = point_list_get_free();

    if ( ipoint == POINT_COUNT ) return false;

    //position
    point_list[ipoint].src.x = x;
    point_list[ipoint].src.y = y;
    point_list[ipoint].src.z = z;

    //Red color
    point_list[ipoint].color.r = 1.00f;
    point_list[ipoint].color.g = 0.00f;
    point_list[ipoint].color.b = 0.00f;
    point_list[ipoint].color.a = 0.50f;

    point_list[ipoint].time = egoboo_get_ticks() + duration;

    return true;
}

//--------------------------------------------------------------------------------------------
int point_list_get_free( void )
{
    int cnt;

    for ( cnt = 0; cnt < POINT_COUNT; cnt++ )
    {
        if ( point_list[cnt].time < 0 )
        {
            break;
        }
    }

    return cnt < POINT_COUNT ? cnt : -1;
}

//--------------------------------------------------------------------------------------------
void point_list_draw_all( const camera_t * pcam )
{
    /// @author BB
    /// @details draw some points for debugging purposes

    int cnt, ticks;

    GLboolean texture_1d_enabled, texture_2d_enabled;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    gfx_begin_3d( pcam );
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );     // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();   // GL_ENABLE_BIT

            GL_DEBUG( glDisable )( GL_BLEND );       // GL_ENABLE_BIT

            // we do not want texture mapped points
            GL_DEBUG( glDisable )( GL_TEXTURE_2D );  // GL_ENABLE_BIT

            ticks = egoboo_get_ticks();

            for ( cnt = 0; cnt < POINT_COUNT; cnt++ )
            {
                if ( point_list[cnt].time < 0 ) continue;

                if ( point_list[cnt].time < ticks )
                {
                    point_list[cnt].time = -1;
                    continue;
                }

                GL_DEBUG( glColor4fv )( point_list[cnt].color.v );       // GL_CURRENT_BIT
                GL_DEBUG( glBegin )( GL_POINTS );
                {
                    GL_DEBUG( glVertex3fv )( point_list[cnt].src.v );
                }
                GL_DEBUG_END();
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();

    // fix the texture enabling
    if ( texture_1d_enabled )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_1D );
    }
    else if ( texture_2d_enabled )
    {
        GL_DEBUG( glEnable )( GL_TEXTURE_2D );
    }
}

//--------------------------------------------------------------------------------------------
// AXIS BOUNDING BOX IMPLEMENTATION(S)
//--------------------------------------------------------------------------------------------
bool render_aabb( aabb_t * pbbox )
{
    GLXvector3f * pmin, * pmax;
    GLint matrix_mode[1];

    if ( NULL == pbbox ) return false;

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    {
        pmin = &( pbbox->mins );
        pmax = &( pbbox->maxs );

        // !!!! there must be an optimized way of doing this !!!!

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            // Front Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );

            // Back Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );

            // Top Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );

            // Bottom Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );

            // Right face
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmax )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );

            // Left Face
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmin )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmin )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmax )[ZZ] );
            GL_DEBUG( glVertex3f )(( *pmin )[XX], ( *pmax )[YY], ( *pmin )[ZZ] );
        }
        GL_DEBUG_END();
    }
    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // restore the matrix mode
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );

    return true;
}

//--------------------------------------------------------------------------------------------
bool render_oct_bb( oct_bb_t * bb, bool draw_square, bool draw_diamond )
{
    bool retval = false;

    if ( NULL == bb ) return false;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT );
    {
        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        GL_DEBUG( glDepthMask )( GL_FALSE );

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );

        // fix the poorly chosen normals...
        // draw draw front and back faces of polygons
        oglx_end_culling();                 // GL_ENABLE_BIT

        // make them transparent
        GL_DEBUG( glEnable )( GL_BLEND );
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        // choose a "white" texture
        oglx_texture_Bind( NULL );

        //------------------------------------------------
        // DIAGONAL BBOX
        if ( draw_diamond )
        {
            float p1_x, p1_y;
            float p2_x, p2_y;

            GL_DEBUG( glColor4f )( 0.5f, 1.0f, 1.0f, 0.1f );

            p1_x = 0.5f * ( bb->maxs[OCT_XY] - bb->maxs[OCT_YX] );
            p1_y = 0.5f * ( bb->maxs[OCT_XY] + bb->maxs[OCT_YX] );
            p2_x = 0.5f * ( bb->maxs[OCT_XY] - bb->mins[OCT_YX] );
            p2_y = 0.5f * ( bb->maxs[OCT_XY] + bb->mins[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->maxs[OCT_XY] - bb->mins[OCT_YX] );
            p1_y = 0.5f * ( bb->maxs[OCT_XY] + bb->mins[OCT_YX] );
            p2_x = 0.5f * ( bb->mins[OCT_XY] - bb->mins[OCT_YX] );
            p2_y = 0.5f * ( bb->mins[OCT_XY] + bb->mins[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->mins[OCT_XY] - bb->mins[OCT_YX] );
            p1_y = 0.5f * ( bb->mins[OCT_XY] + bb->mins[OCT_YX] );
            p2_x = 0.5f * ( bb->mins[OCT_XY] - bb->maxs[OCT_YX] );
            p2_y = 0.5f * ( bb->mins[OCT_XY] + bb->maxs[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->mins[OCT_XY] - bb->maxs[OCT_YX] );
            p1_y = 0.5f * ( bb->mins[OCT_XY] + bb->maxs[OCT_YX] );
            p2_x = 0.5f * ( bb->maxs[OCT_XY] - bb->maxs[OCT_YX] );
            p2_y = 0.5f * ( bb->maxs[OCT_XY] + bb->maxs[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            retval = true;
        }

        //------------------------------------------------
        // SQUARE BBOX
        if ( draw_square )
        {
            GL_DEBUG( glColor4f )( 1.0f, 0.5f, 1.0f, 0.1f );

            // XZ FACE, min Y
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, min X
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XZ FACE, max Y
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, max X
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, min Z
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, max Z
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->mins[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->maxs[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->maxs[OCT_X], bb->mins[OCT_Y], bb->maxs[OCT_Z] );
            GL_DEBUG_END();

            retval = true;
        }

    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}
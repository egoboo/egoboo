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

#include "game/Graphics/CameraSystem.hpp"
#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------
// INTERNAL VARIABLES
//--------------------------------------------------------------------------------------------

static line_data_t line_list[LINE_COUNT];
static point_data_t point_list[POINT_COUNT];

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------
void gfx_begin_3d(std::shared_ptr<Camera> camera)
{
    // store the GL_PROJECTION matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().loadMatrix(camera->getProjection());
    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().loadMatrix(camera->getView());
}

//--------------------------------------------------------------------------------------------
void gfx_end_3d()
{
    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // Restore the GL_PROJECTION matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();
}

//--------------------------------------------------------------------------------------------
// Lines.
//--------------------------------------------------------------------------------------------
void line_list_init()
{
    for (size_t i = 0; i < LINE_COUNT; ++i)
    {
        line_list[i].time = -1;
    }
}

size_t line_list_get_free()
{
    for (size_t i = 0; i < LINE_COUNT; ++i)
    {
        if (line_list[i].time < 0)
        {
            return i;
        }
    }
    return LINE_COUNT;
}

bool line_list_add( const float src_x, const float src_y, const float src_z, const float dst_x, const float dst_y, const float dst_z, const int duration )
{
    size_t index = line_list_get_free();
    if (index == LINE_COUNT) return false;
    line_data_t& line = line_list[index];
    
    // Source.
    line.src = fvec3_t(src_x,src_y,src_z);

    // Target.
    line.dst = fvec3_t(dst_x,dst_y,dst_z);

    // Colour: white.
    line.color = fvec4_t(1,1,1,0.5);

    // Time.
    line.time = SDL_GetTicks() + duration;

    return true;
}

void line_list_draw_all(std::shared_ptr<Camera> camera)
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

    gfx_begin_3d(camera);
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );     // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
			Ego::Renderer::get().setDepthTestEnabled(true);
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();   // GL_ENABLE_BIT

            Ego::Renderer::get().setBlendingEnabled(false);

            // we do not want texture mapped lines
            GL_DEBUG( glDisable )( GL_TEXTURE_2D );  // GL_ENABLE_BIT

            ticks = SDL_GetTicks();

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
// Points.
//--------------------------------------------------------------------------------------------
void point_list_init()
{
    for (size_t i = 0; i < POINT_COUNT; ++i)
    {
        point_list[i].time = -1;
    }
}

size_t point_list_get_free()
{
    for (size_t i = 0; i < POINT_COUNT; ++i)
    {
        if (point_list[i].time < 0)
        {
            return i;
        }
    }
    return POINT_COUNT;
}

bool point_list_add(const float x, const float y, const float z, const int duration)
{
    size_t index = point_list_get_free();
    if (index == POINT_COUNT) return false;
    point_data_t& point = point_list[index];

    // Position.
    point.src = fvec3_t(x,y,z);

    // Colour: red.
    point.color = fvec4_t(1, 0, 0, 0.5);

    // Time.
    point.time = SDL_GetTicks() + duration;

    return true;
}

void point_list_draw_all(std::shared_ptr<Camera> camera)
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

    gfx_begin_3d(camera);
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );     // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );     // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
			Ego::Renderer::get().setDepthTestEnabled(true);
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );    // GL_DEPTH_BUFFER_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();   // GL_ENABLE_BIT

            Ego::Renderer::get().setBlendingEnabled(false);

            // we do not want texture mapped points
            GL_DEBUG( glDisable )( GL_TEXTURE_2D );  // GL_ENABLE_BIT

            ticks = SDL_GetTicks();

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
bool render_aabb(aabb_t *bv)
{
    GLXvector3f * pmin, * pmax;
    GLint matrix_mode[1];

    if (!bv) return false;

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    {
        pmin = &( bv->mins.v );
        pmax = &( bv->maxs.v );

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
bool render_oct_bb(oct_bb_t *bb, bool drawSquare, bool drawDiamond,const Ego::Math::Colour4f& squareColour,const Ego::Math::Colour4f& diamondColour)
{
    bool retval = false;

    if (NULL == bb) return false;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT );
    {
        // Do not write write into the depth buffer.
        // (disable glDepthMask for transparent objects)
        GL_DEBUG(glDepthMask)(GL_FALSE);

        // do not draw hidden surfaces
		Ego::Renderer::get().setDepthTestEnabled(true);
        GL_DEBUG(glDepthFunc)(GL_LEQUAL);

        // fix the poorly chosen normals...
        // draw draw front and back faces of polygons
        GL_DEBUG(glDisable)(GL_CULL_FACE);  // GL_ENABLE_BIT

        // make them transparent
        Ego::Renderer::get().setBlendingEnabled(true);
        GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // choose a "white" texture
        oglx_texture_bind( NULL );

        //------------------------------------------------
        // DIAGONAL BBOX
        if (drawDiamond)
        {
            float p1_x, p1_y;
            float p2_x, p2_y;

			Ego::Renderer::get().setColour(diamondColour);

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
        if (drawSquare)
        {
			Ego::Renderer::get().setColour(squareColour);

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
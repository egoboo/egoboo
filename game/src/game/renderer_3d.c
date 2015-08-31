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
// MODE CONTROL
//--------------------------------------------------------------------------------------------
void gfx_begin_3d(Camera& camera)
{
    // store the GL_PROJECTION matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().loadMatrix(camera.getProjectionMatrix());
    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().loadMatrix(camera.getViewMatrix());
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

LineSegmentList g_lineSegmentList;

void LineSegmentList::init()
{
    for (size_t i = 0; i < capacity; ++i)
    {
        _elements[i].time = -1;
    }
}

size_t LineSegmentList::get_free()
{
    for (size_t i = 0; i < capacity; ++i)
    {
        if (_elements[i].time < 0)
        {
            return i;
        }
    }
    return capacity;
}

bool LineSegmentList::add(const Vector3f& p, const Vector3f& q, const int duration )
{
    size_t index = get_free();
    if (index == capacity) return false;
    auto& line = _elements[index];
    
    // Source.
    line.p = p;

    // Target.
    line.q = q;

    // Colour: white/half transparent
    line.colour = Ego::Math::Colour4f(1,1,1,0.5);

    // Time.
    line.time = SDL_GetTicks() + duration;

    return true;
}

void LineSegmentList::draw_all(Camera& camera)
{
    /// @author BB
    /// @details draw some lines for debugging purposes

	auto& renderer = Ego::Renderer::get();

    // disable texturing
	renderer.getTextureUnit().setActivated(nullptr);

    gfx_begin_3d(camera);
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            // flat shading
            renderer.setGouraudShadingEnabled(false); // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            renderer.setDepthWriteEnabled(false); // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

            // draw draw front and back faces of polygons
			renderer.setCullingMode(Ego::CullingMode::None);

            renderer.setBlendingEnabled(false);

            uint32_t ticks = SDL_GetTicks();

            for (size_t i = 0; i < capacity; ++i)
            {
                auto& line = _elements[i];
                if ( line.time < 0 ) continue;

                if ( line.time < ticks )
                {
                    line.time = -1;
                    continue;
                }

                renderer.setColour(line.colour); // GL_CURRENT_BIT
                GL_DEBUG( glBegin )( GL_LINES );
                {
                    GL_DEBUG(glVertex3f)( line.p[kX], line.p[kY], line.p[kZ] );
                    GL_DEBUG(glVertex3f)( line.q[kX], line.q[kY], line.q[kZ] );
                }
                GL_DEBUG_END();
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();
}

//--------------------------------------------------------------------------------------------
// Points.
//--------------------------------------------------------------------------------------------

PointList g_pointList;

void PointList::init()
{
    for (size_t i = 0; i < capacity; ++i)
    {
        _elements[i].time = -1;
    }
}

size_t PointList::get_free()
{
    for (size_t i = 0; i < capacity; ++i)
    {
        if (_elements[i].time < 0)
        {
            return i;
        }
    }
    return capacity;
}

bool PointList::add(const Vector3f& p, const int duration)
{
    size_t index = get_free();
    if (index == capacity) return false;
    auto& point = _elements[index];

    // Position.
    point.p = p;

    // Colour: red/half-transparent
    point.colour = Ego::Math::Colour4f(1, 0, 0, 0.5);

    // Time.
    point.time = SDL_GetTicks() + duration;

    return true;
}

void PointList::draw_all(Camera& camera)
{
    /// @author BB
    /// @details draw some points for debugging purposes

	auto& renderer = Ego::Renderer::get();

    // disable texturing
	renderer.getTextureUnit().setActivated(nullptr);

    gfx_begin_3d(camera);
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            // flat shading
            renderer.setGouraudShadingEnabled(false); // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            renderer.setDepthWriteEnabled(false); // GL_DEPTH_BUFFER_BIT

            // do not draw hidden surfaces
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

            // draw draw front and back faces of polygons
			renderer.setCullingMode(Ego::CullingMode::None);

            renderer.setBlendingEnabled(false);

            uint32_t ticks = SDL_GetTicks();

            for (size_t i = 0; i < capacity; ++i)
            {
                auto& point = _elements[i];
                if ( point.time < 0 ) continue;

                if ( point.time < ticks )
                {
                    point.time = -1;
                    continue;
                }
                renderer.setColour(point.colour); // GL_CURRENT_BIT
                GL_DEBUG( glBegin )( GL_POINTS );
                {
                    GL_DEBUG(glVertex3f)(point.p[kX], point.p[kY], point.p[kZ]);
                }
                GL_DEBUG_END();
            }
        }
        ATTRIB_POP( __FUNCTION__ );
    }
    gfx_end_3d();
}

//--------------------------------------------------------------------------------------------
// AXIS BOUNDING BOX IMPLEMENTATION(S)
//--------------------------------------------------------------------------------------------
bool render_aabb(AABB3f *bv)
{
    GLint matrix_mode[1];

    if (!bv) return false;

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    {
        const auto& pmin = (bv->getMin());
        const auto& pmax = (bv->getMax());

        // !!!! there must be an optimized way of doing this !!!!

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            // Front Face
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmin)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmin)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmax)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmax)[YY], (pmax)[ZZ]);

            // Back Face
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmin)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmax)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmax)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmin)[YY], (pmin)[ZZ]);

            // Top Face
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmax)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmax)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmax)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmax)[YY], (pmin)[ZZ]);

            // Bottom Face
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmin)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmin)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmin)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmin)[YY], (pmax)[ZZ]);

            // Right face
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmin)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmax)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmax)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmax)[XX], (pmin)[YY], (pmax)[ZZ]);

            // Left Face
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmin)[YY], (pmin)[ZZ]);
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmin)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmax)[YY], (pmax)[ZZ]);
            GL_DEBUG( glVertex3f )((pmin)[XX], (pmax)[YY], (pmin)[ZZ]);
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
		auto& renderer = Ego::Renderer::get();
        // Do not write write into the depth buffer.
        // (disable glDepthMask for transparent objects)
        GL_DEBUG(glDepthMask)(GL_FALSE);

        // do not draw hidden surfaces
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

        // fix the poorly chosen normals...
        // draw draw front and back faces of polygons
		renderer.setCullingMode(Ego::CullingMode::None); // GL_ENABLE_BIT

        // make them transparent
        renderer.setBlendingEnabled(true);
		renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

        // deactivate texturing
		renderer.getTextureUnit().setActivated(nullptr);

        //------------------------------------------------
        // DIAGONAL BBOX
        if (drawDiamond)
        {
            float p1_x, p1_y;
            float p2_x, p2_y;

			renderer.setColour(diamondColour);

            p1_x = 0.5f * ( bb->_maxs[OCT_XY] - bb->_maxs[OCT_YX] );
            p1_y = 0.5f * ( bb->_maxs[OCT_XY] + bb->_maxs[OCT_YX] );
            p2_x = 0.5f * ( bb->_maxs[OCT_XY] - bb->_mins[OCT_YX] );
            p2_y = 0.5f * ( bb->_maxs[OCT_XY] + bb->_mins[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->_maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->_maxs[OCT_XY] - bb->_mins[OCT_YX] );
            p1_y = 0.5f * ( bb->_maxs[OCT_XY] + bb->_mins[OCT_YX] );
            p2_x = 0.5f * ( bb->_mins[OCT_XY] - bb->_mins[OCT_YX] );
            p2_y = 0.5f * ( bb->_mins[OCT_XY] + bb->_mins[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->_maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->_mins[OCT_XY] - bb->_mins[OCT_YX] );
            p1_y = 0.5f * ( bb->_mins[OCT_XY] + bb->_mins[OCT_YX] );
            p2_x = 0.5f * ( bb->_mins[OCT_XY] - bb->_maxs[OCT_YX] );
            p2_y = 0.5f * ( bb->_mins[OCT_XY] + bb->_maxs[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->_maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb->_mins[OCT_XY] - bb->_maxs[OCT_YX] );
            p1_y = 0.5f * ( bb->_mins[OCT_XY] + bb->_maxs[OCT_YX] );
            p2_x = 0.5f * ( bb->_maxs[OCT_XY] - bb->_maxs[OCT_YX] );
            p2_y = 0.5f * ( bb->_maxs[OCT_XY] + bb->_maxs[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb->_maxs[OCT_Z] );
            GL_DEBUG_END();

            retval = true;
        }

        //------------------------------------------------
        // SQUARE BBOX
        if (drawSquare)
        {
			renderer.setColour(squareColour);

            // XZ FACE, min Y
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_mins[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_mins[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_mins[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_mins[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, min X
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_mins[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_mins[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_maxs[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_maxs[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG_END();

            // XZ FACE, max Y
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_maxs[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_maxs[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_maxs[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_maxs[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, max X
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_mins[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_mins[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_maxs[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_maxs[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, min Z
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_mins[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_maxs[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_maxs[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_mins[OCT_Y], bb->_mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, max Z
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_mins[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_mins[OCT_X], bb->_maxs[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_maxs[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb->_maxs[OCT_X], bb->_mins[OCT_Y], bb->_maxs[OCT_Z] );
            GL_DEBUG_END();

            retval = true;
        }

    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}
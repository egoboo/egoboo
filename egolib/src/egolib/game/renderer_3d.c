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

/// @file egolib/game/renderer_3d.c
/// @brief Implementation of the 3d renderer functions
/// @details

#include "egolib/game/renderer_3d.h"

#include "egolib/game/Graphics/CameraSystem.hpp"
#include "egolib/game/egoboo.h"

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
    line.time = Time::now<Time::Unit::Ticks>() + duration;

    return true;
}

void LineSegmentList::draw_all(Camera& camera)
{
    /// @author BB
    /// @details draw some lines for debugging purposes

	auto& renderer = Ego::Renderer::get();

    // disable texturing
	renderer.getTextureUnit().setActivated(nullptr);

    Renderer3D::begin3D(camera);
    {
        {
            Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
            {
                // flat shading
                renderer.setGouraudShadingEnabled(false); // GL_LIGHTING_BIT

                // don't write into the depth buffer (disable glDepthMask for transparent objects)
                renderer.setDepthWriteEnabled(false); // GL_DEPTH_BUFFER_BIT
                renderer.setAlphaTestEnabled(false);

                // do not draw hidden surfaces
                renderer.setDepthTestEnabled(true);
                renderer.setDepthFunction(idlib::compare_function::less_or_equal);

                // draw draw front and back faces of polygons
                renderer.setCullingMode(idlib::culling_mode::none);

                renderer.setBlendingEnabled(false);

                uint32_t ticks = Time::now<Time::Unit::Ticks>();

                for (size_t i = 0; i < capacity; ++i) {
                    auto& line = _elements[i];
                    if (line.time < 0) continue;

                    if (line.time < ticks) {
                        line.time = -1;
                        continue;
                    }

                    renderer.setColour(line.colour); // GL_CURRENT_BIT
                    GL_DEBUG(glBegin)(GL_LINES);
                    {
                        GL_DEBUG(glVertex3f)(line.p[kX], line.p[kY], line.p[kZ]);
                        GL_DEBUG(glVertex3f)(line.q[kX], line.q[kY], line.q[kZ]);
                    }
                    GL_DEBUG_END();
                }
            }
        }
    }
    Renderer3D::end3D();
}

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
    point.time = Time::now<Time::Unit::Ticks>() + duration;

    return true;
}

void PointList::draw_all(Camera& camera)
{
    /// @author BB
    /// @details draw some points for debugging purposes

	auto& renderer = Ego::Renderer::get();

    // disable texturing
	renderer.getTextureUnit().setActivated(nullptr);

    Renderer3D::begin3D(camera);
    {
        Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
        {
            // flat shading
            renderer.setGouraudShadingEnabled(false); // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            renderer.setDepthWriteEnabled(false); // GL_DEPTH_BUFFER_BIT
            renderer.setAlphaTestEnabled(false);

            // do not draw hidden surfaces
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(idlib::compare_function::less_or_equal);

            // draw draw front and back faces of polygons
			renderer.setCullingMode(idlib::culling_mode::none);

            renderer.setBlendingEnabled(false);

            uint32_t ticks = Time::now<Time::Unit::Ticks>();

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
    }
    Renderer3D::end3D();
}

PointList Renderer3D::pointList;
LineSegmentList Renderer3D::lineSegmentList;

void Renderer3D::begin3D(Camera& camera) {
    auto& renderer = Ego::Renderer::get();
    renderer.setProjectionMatrix(camera.getProjectionMatrix());
    renderer.setWorldMatrix(Matrix4f4f::identity());
    renderer.setViewMatrix(camera.getViewMatrix());
}

void Renderer3D::end3D() {}


void Renderer3D::renderAxisAlignedBox(const AxisAlignedBox3f& bv, const Ego::Math::Colour4f& colour)
{
    auto& renderer = Ego::Renderer::get();

    renderer.setViewMatrix(Matrix4f4f::identity());
    renderer.setWorldMatrix(Matrix4f4f::identity());

    Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT);
    {
        // Do not write write into the depth buffer.
        renderer.setDepthWriteEnabled(false);

        // do not draw hidden surfaces
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(idlib::compare_function::less_or_equal);

        // draw draw front and back faces of polygons
        renderer.setCullingMode(idlib::culling_mode::none); // GL_ENABLE_BIT

        // make them transparent
        renderer.setBlendingEnabled(true);
        renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);
        renderer.setAlphaTestEnabled(true);
        renderer.setAlphaFunction(idlib::compare_function::greater, 0.0f);

        // deactivate texturing
        renderer.getTextureUnit().setActivated(nullptr);
        renderer.setColour(colour);

        const auto& pmin = (bv.get_min());
        const auto& pmax = (bv.get_max());

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
}

void Renderer3D::renderOctBB(const oct_bb_t &bb, bool drawSquare, bool drawDiamond, const Ego::Math::Colour4f& squareColour, const Ego::Math::Colour4f& diamondColour)
{
    auto& renderer = Ego::Renderer::get();

    // disable texturing
    renderer.getTextureUnit().setActivated(nullptr);

    Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT);
    {

        // Do not write write into the depth buffer.
        // (disable glDepthMask for transparent objects)
        renderer.setDepthWriteEnabled(false);

        // do not draw hidden surfaces
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(idlib::compare_function::less_or_equal);

        // fix the poorly chosen normals...
        // draw draw front and back faces of polygons
		renderer.setCullingMode(idlib::culling_mode::none); // GL_ENABLE_BIT

        // make them transparent
        renderer.setBlendingEnabled(true);
		renderer.setBlendFunction(idlib::color_blend_parameter::source0_alpha, idlib::color_blend_parameter::one_minus_source0_alpha);
        renderer.setAlphaTestEnabled(true);
        renderer.setAlphaFunction(idlib::compare_function::greater, 0.0f);

        // deactivate texturing
		renderer.getTextureUnit().setActivated(nullptr);

        //------------------------------------------------
        // DIAGONAL BBOX
        if (drawDiamond)
        {
            float p1_x, p1_y;
            float p2_x, p2_y;

			renderer.setColour(diamondColour);

            p1_x = 0.5f * ( bb._maxs[OCT_XY] - bb._maxs[OCT_YX] );
            p1_y = 0.5f * ( bb._maxs[OCT_XY] + bb._maxs[OCT_YX] );
            p2_x = 0.5f * ( bb._maxs[OCT_XY] - bb._mins[OCT_YX] );
            p2_y = 0.5f * ( bb._maxs[OCT_XY] + bb._mins[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb._maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb._maxs[OCT_XY] - bb._mins[OCT_YX] );
            p1_y = 0.5f * ( bb._maxs[OCT_XY] + bb._mins[OCT_YX] );
            p2_x = 0.5f * ( bb._mins[OCT_XY] - bb._mins[OCT_YX] );
            p2_y = 0.5f * ( bb._mins[OCT_XY] + bb._mins[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb._maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb._mins[OCT_XY] - bb._mins[OCT_YX] );
            p1_y = 0.5f * ( bb._mins[OCT_XY] + bb._mins[OCT_YX] );
            p2_x = 0.5f * ( bb._mins[OCT_XY] - bb._maxs[OCT_YX] );
            p2_y = 0.5f * ( bb._mins[OCT_XY] + bb._maxs[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb._maxs[OCT_Z] );
            GL_DEBUG_END();

            p1_x = 0.5f * ( bb._mins[OCT_XY] - bb._maxs[OCT_YX] );
            p1_y = 0.5f * ( bb._mins[OCT_XY] + bb._maxs[OCT_YX] );
            p2_x = 0.5f * ( bb._maxs[OCT_XY] - bb._maxs[OCT_YX] );
            p2_y = 0.5f * ( bb._maxs[OCT_XY] + bb._maxs[OCT_YX] );

            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( p2_x, p2_y, bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( p1_x, p1_y, bb._maxs[OCT_Z] );
            GL_DEBUG_END();
        }

        //------------------------------------------------
        // SQUARE BBOX
        if (drawSquare)
        {
			renderer.setColour(squareColour);

            // XZ FACE, min Y
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._mins[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._mins[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._mins[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._mins[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, min X
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._mins[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._mins[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._maxs[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._maxs[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG_END();

            // XZ FACE, max Y
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._maxs[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._maxs[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._maxs[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._maxs[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG_END();

            // YZ FACE, max X
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._mins[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._mins[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._maxs[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._maxs[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, min Z
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._mins[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._maxs[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._maxs[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._mins[OCT_Y], bb._mins[OCT_Z] );
            GL_DEBUG_END();

            // XY FACE, max Z
            GL_DEBUG( glBegin )( GL_QUADS );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._mins[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._mins[OCT_X], bb._maxs[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._maxs[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG( glVertex3f )( bb._maxs[OCT_X], bb._mins[OCT_Y], bb._maxs[OCT_Z] );
            GL_DEBUG_END();
        }

    }
}
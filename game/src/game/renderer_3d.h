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

/// @file game/renderer_3d.h
/// @brief Routines for rendering 3d primitves

#pragma once

#include "egolib/egolib.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Forward declarations.
class Camera;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct line_data_t;
struct point_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define LINE_COUNT 100
#define POINT_COUNT 100

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// OPENGL VERTEX
struct s_GLvertex
{
    GLfloat pos[4];
    GLfloat nrm[3];
    GLfloat env[2];
    GLfloat tex[2];

    GLfloat col[4];      ///< generic per-vertex lighting
    GLint   color_dir;   ///< "optimized" per-vertex directional lighting
};

//--------------------------------------------------------------------------------------------
// some lines to be drawn in the display

struct line_data_t
{
    fvec3_t src, dst;
    fvec4_t color;
    int time;
};

/// @brief Initialize the line list so that all lines are free.
void line_list_init();
/// @brief Get the index of a free line.
/// @return the index of a free line if any, #LINE_COUNT otherwise
size_t line_list_get_free();
bool line_list_add(const float src_x, const float src_y, const float src_z, const float dst_x, const float dst_y, const float dst_z, const int duration);
void line_list_draw_all(std::shared_ptr<Camera> camera);

//--------------------------------------------------------------------------------------------
// some points to be drawn in the display

struct point_data_t
{
    fvec3_t src;
    fvec4_t color;
    int time;
};

/// @brief Initialize the point list so that all points are free.
void point_list_init();
/// @brief Get the index of a free point.
/// @return the index of a free point if any, #POINT_COUNT otherwise
size_t point_list_get_free();
bool point_list_add(const float x, const float y, const float z, const int duration);
void point_list_draw_all(std::shared_ptr<Camera> camera);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void gfx_begin_3d(std::shared_ptr<Camera> camera);
void gfx_end_3d();

bool render_oct_bb(oct_bb_t *bv, bool drawSquare, bool drawDiamond, const Ego::Math::Colour4f& squareColour = Ego::Math::Colour4f(1, 0.5, 1, 1), const Ego::Math::Colour4f& diamondColour = Ego::Math::Colour4f(0.5, 1, 1, 1));
bool render_aabb(aabb_t *bv);

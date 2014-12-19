#pragma once

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

#include "egolib/opengl/renderer.h"
#include "egolib/vec.h"
#if 0
#include "game/egoboo_typedef.h"
#include "egolib/extensions/ogl_texture.h"
#include "egolib/_math.h"
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Forward declarations.
typedef struct oct_bb_t oct_bb_t;
typedef struct aabb_t aabb_t;
typedef struct camera_t camera_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_line_data;
typedef struct s_line_data line_data_t;

struct s_point_data;
typedef struct s_point_data point_data_t;

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

struct s_line_data
{
    fvec3_t   dst;
    fvec4_t   src, color;
    int time;
};

void line_list_init();
int  line_list_get_free();
bool line_list_add( const float src_x, const float src_y, const float src_z, const float pos_x, const float dst_y, const float dst_z, const int duration );
void line_list_draw_all( const camera_t *pcam );

//--------------------------------------------------------------------------------------------
// some points to be drawn in the display

struct s_point_data
{
    fvec4_t   src, color;
    int time;
};

void point_list_init();
int  point_list_get_free();
bool point_list_add( const float x, const float y, const float z, const int duration );
void point_list_draw_all( const camera_t *pcam );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void gfx_begin_3d( const camera_t *pcam );
void gfx_end_3d();

bool render_oct_bb( oct_bb_t *bb, bool draw_square, bool draw_diamond );
bool render_aabb( aabb_t * pbbox );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define egoboo_renderer_3d_h

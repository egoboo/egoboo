#pragma once
//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
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

/// @defgroup _ogl_extensions_ Extensions to OpenGL

/// @file extensions/ogl_extensions.h
/// @ingroup _ogl_extensions_
/// @brief Definitions for extended functions and variables for OpenGL
/// @details

#include <SDL_opengl.h>
#include "file_common.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// wrapper for uploading texture information

void oglx_bind( GLenum target, GLuint id, GLint wrap_s, GLint wrap_t, GLint min_f, GLint mag_f, GLfloat aniso );
void oglx_upload_1d( GLboolean use_alpha, GLsizei w, const GLvoid * data );
void oglx_upload_2d( GLboolean use_alpha, GLsizei w, GLsizei h, const GLvoid * data );
void oglx_upload_2d_mipmap( GLboolean use_alpha, GLsizei w, GLsizei h, const GLvoid * data );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// OpenGL graphics info
struct s_oglx_caps
{
    const GLubyte * gl_version;
    const GLubyte * gl_vendor;
    const GLubyte * gl_renderer;
    const GLubyte * gl_extensions;

    const GLubyte * glu_version;
    const GLubyte * glu_extensions;

    // stack depths
    GLint max_modelview_stack_depth;     ///< Maximum modelview-matrix stack depth
    GLint max_projection_stack_depth;    ///< Maximum projection-matrix stack depth
    GLint max_texture_stack_depth;       ///< Maximum depth of texture matrix stack
    GLint max_name_stack_depth;          ///< Maximum selection-name stack depth
    GLint max_attrib_stack_depth;        ///< Maximum depth of the attribute stack
    GLint max_client_attrib_stack_depth; ///< Maximum depth of the client attribute stack

    // Antialiasing settings
    GLint   subpixel_bits;           ///< Number of bits of subpixel precision in x and y
    GLfloat point_size_range[2];     ///< Range (low to high) of antialiased point sizes
    GLfloat point_size_granularity;  ///< Antialiased point-size granularity
    GLfloat line_width_range[2];     ///< Range (low to high) of antialiased line widths
    GLfloat line_width_granularity;  ///< Antialiased line-width granularity

    // display settings
    GLint     max_viewport_dims[2];  ///< Maximum viewport dimensions
    GLboolean aux_buffers;           ///< Number of auxiliary buffers
    GLboolean rgba_mode;             ///< True if color buffers store RGBA
    GLboolean index_mode;            ///< True if color buffers store indices
    GLboolean doublebuffer;          ///< True if front and back buffers exist
    GLboolean stereo;                ///< True if left and right buffers exist
    GLint     red_bits;              ///< Number of bits per red component in color buffers
    GLint     green_bits;            ///< Number of bits per green component in color buffers
    GLint     blue_bits;             ///< Number of bits per blue component in color buffers
    GLint     alpha_bits;            ///< Number of bits per alpha component in color buffers
    GLint     index_bits;            ///< Number of bits per index in color buffers
    GLint     depth_bits;            ///< Number of depth-buffer bitplanes
    GLint     stencil_bits;          ///< Number of stencil bitplanes
    GLint     accum_red_bits;        ///< Number of bits per red component in the accumulation buffer
    GLint     accum_green_bits;      ///< Number of bits per green component in the accumulation buffer
    GLint     accum_blue_bits;       ///< Number of bits per blue component in the accumulation buffer
    GLint     accum_alpha_bits;      ///< Number of bits per blue component in the accumulation buffer

    // Misc
    GLint max_lights;                    ///< Maximum number of lights
    GLint max_clip_planes;               ///< Maximum number of user clipping planes
    GLint max_texture_size;              ///< See discussion in "Texture Proxy" in Chapter 9

    GLint max_pixel_map_table;           ///< Maximum size of a glPixelMap() translation table
    GLint max_list_nesting;              ///< Maximum display-list call nesting
    GLint max_eval_order;                ///< Maximum evaluator polynomial order

    GLboolean anisotropic_supported;
    GLfloat   maxAnisotropy;                     ///< Max anisotropic filterings (Between 1.00 and 16.00)
    GLfloat   log2Anisotropy;                    ///< Max levels of anisotropy
};
typedef struct s_oglx_caps oglx_caps_t;

void oglx_Get_Screen_Info( oglx_caps_t * pcaps );
void oglx_report_caps();

extern oglx_caps_t ogl_caps;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_oglx_video_parameters
{
    GLboolean antialiasing;            ///< current antialiasing value
    GLboolean multisample;             ///< whether multisampling is being supported through GL_MULTISAMPLE
    GLboolean multisample_arb;         ///< whether multisampling is being supported through GL_MULTISAMPLE_ARB
    GLenum    perspective;             ///< current correction hint
    GLboolean dither;                  ///< current dithering flag
    GLenum    shading;                 ///< current shading type
    GLfloat   userAnisotropy;          ///< current value of the anisotropic filtering
};
typedef struct s_oglx_video_parameters oglx_video_parameters_t;

GLboolean oglx_video_parameters_default( oglx_video_parameters_t * pvid );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
FILE * oglx_set_stdout( FILE * pfile );


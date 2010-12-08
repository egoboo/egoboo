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

/// @file extensions/ogl_debug.h
/// @ingroup _ogl_extensions_
/// @brief Definitions for the debugging extensions for OpenGL
/// @details

#include "ogl_include.h"

#undef USE_GL_DEBUG

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL texture gen state
struct s_gl_texture_gen
{
    GLboolean enabled;
    GLfloat   eye_plane[4];
    GLfloat   object_plane[4];
    GLint     mode[1];
};
typedef struct s_gl_texture_gen gl_texture_gen_t;

/// Grab the OpenGL texture gen state
void gl_grab_texture_gen_state( gl_texture_gen_t * pt, GLenum coord );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL texture state
struct s_gl_texture
{
    GLenum  target;
    GLuint  binding;              /*< The OpenGL texture number */

    GLfloat width;
    GLfloat height;
    GLint   border;
    GLint   internal_format;
    GLint   red_size;
    GLint   green_size;
    GLint   blue_size;
    GLint   alpha_size;
    GLint   luminance_size;
    GLint   intensity_size;
    GLfloat border_color[4];

    GLint   min_filter;
    GLint   mag_filter;
    GLint   wrap_s;
    GLint   wrap_t;
    GLfloat priority;
    GLint   resident;

    gl_texture_gen_t gen[4];
};
typedef struct s_gl_texture gl_texture_t;

/// Grab the OpenGL textire state
void gl_grab_texture_state( GLenum target, GLint level, gl_texture_t * pi );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL clip plane state
struct s_gl_clip_plane
{
    GLboolean enabled;
    GLdouble  coeffs[4];
};
typedef struct s_gl_clip_plane gl_clip_plane_t;

/// Grab the OpenGL clip plane state
void gl_grab_clip_plane_state( gl_clip_plane_t * ps, GLint which );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL stencil state
struct s_gl_stencil
{
    GLboolean    test_enabled;
    GLint        test_func[1];
    const char * test_func_sz;
    GLint        test_ref[1];

    GLint writemask[1];
    GLint clear_value[1];
    GLint value_mask[1];

    GLint        fail[1];
    const char * fail_sz;

    GLint        pass_depth_fail[1];
    const char * pass_depth_fail_sz;

    GLint        pass_depth_pass[1];
    const char * pass_depth_pass_sz;
};
typedef struct s_gl_stencil gl_stencil_t;

/// Grab the OpenGL stencil state
void gl_grab_stencil_state( gl_stencil_t * ps );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL packing state
struct s_gl_packing
{
    GLint     row_length[1];
    GLint     skip_rows[1];
    GLint     skip_pixels[1];
    GLint     alignment[1];
    GLboolean swap_bytes[1];
    GLboolean lsb_first[1];
};
typedef struct s_gl_packing gl_packing_t;

/// Grab the OpenGL packing state
void gl_grab_packing_state( gl_packing_t * pp );

/// Grab the OpenGL unpacking state
void gl_grab_unpacking_state( gl_packing_t * pp );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL bland state
struct s_gl_blend
{
    GLboolean enabled;

    GLint        src[1];
    const char * src_sz;

    GLint        dst[1];
    const char * dst_sz;
};
typedef struct s_gl_blend gl_blend_t;

/// Grab the OpenGL blend state
void gl_grab_blend_state( gl_blend_t * pb );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL alpha state
struct s_gl_alpha
{
    GLboolean test_enabled;

    GLint        test_func[1];
    const char * test_func_sz;
    GLint        test_ref[1];

    GLfloat   scale[1];
    GLfloat   bias[1];
};

typedef struct s_gl_alpha gl_alpha_t;

/// Grab the OpenGL alpha state
void gl_grab_alpha_state( gl_alpha_t * pa );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL depth state
struct s_gl_depth
{
    GLboolean test_enabled;

    const char * test_func_sz;
    GLint        test_func[1];

    GLboolean writemask_enabled[1];
    GLfloat   clear_value[1];

    GLfloat   range[2];
    GLfloat   scale[1];
    GLfloat   bias[1];
};
typedef struct s_gl_depth gl_depth_t;

/// Grab the OpenGL depth state
void gl_grab_depth_state( gl_depth_t * pd );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL polygon state
struct s_gl_polygon
{
    GLint        mode[1];
    const char * mode_sz;

    GLboolean    smooth;
    GLint        smooth_hint[1];
    const char * smooth_hint_sz;

    GLfloat   offset_factor[1];
    GLfloat   offset_bias[1];
    GLboolean offset_point_enabled;
    GLboolean offset_line_enabled;
    GLboolean offset_fill_enabled;

    GLboolean stipple_enabled;
};
typedef struct s_gl_polygon gl_polygon_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL line state
struct s_gl_line
{
    GLfloat width[1];

    GLboolean    smooth;
    GLint        smooth_hint[1];
    const char * smooth_hint_sz;

    GLboolean stipple_enabled;
    GLint     stipple_pattern[1];
    GLint     stipple_repeat[1];
};
typedef struct s_gl_line gl_line_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL point state
struct s_gl_point
{
    GLfloat size[1];

    GLint        smooth_hint[1];
    const char * smooth_hint_sz;
};
typedef struct s_gl_point gl_point_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL scissor state
struct s_gl_scissor
{
    GLint     box[4];
    GLboolean test_enabled;
};
typedef struct s_gl_scissor gl_scissor_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL feedback state
struct s_gl_feedback
{
    GLint     buffer_type[1];
    GLint     buffer_size[1];
    GLvoid  * buffer_pointer;
};
typedef struct s_gl_feedback gl_feedback_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL selection state
struct s_gl_selection
{
    GLint     name_stack_depth[1];
    GLvoid  * buffer_pointer;
    GLint     buffer_size[1];
};
typedef struct s_gl_selection gl_selection_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL render mode state
struct s_gl_render_mode
{
    GLint            mode[1];
    const char     * mode_sz;

    gl_feedback_t    feedback;
    gl_selection_t   selection;
};
typedef struct s_gl_render_mode gl_render_mode_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL matrix state
struct s_gl_matrix
{
    GLint        mode[1];
    const char * mode_sz;

    GLint   modelview_stack_depth[1];
    GLfloat modelview_matrix[16];

    GLint   projection_stack_depth[1];
    GLfloat projection_matrix[16];

    GLint   texture_stack_depth[1];
    GLfloat texture_matrix[16];
};
typedef struct s_gl_matrix gl_matrix_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL render options state
struct s_gl_render_options
{
    gl_render_mode_t mode;
    gl_blend_t       blend;
    gl_alpha_t       alpha;
    gl_polygon_t     polygon;
    gl_line_t        line;
    gl_point_t       point;

    GLboolean normalize_enabled;
    GLboolean dither_enabled;
    GLboolean auto_normal_enabled;

    GLint        perspective_correction_hint[1];
    const char * perspective_correction_hint_sz;

    GLint        fog_hint[1];
    const char * fog_hint_sz;

    GLint        shade_model[1];
    const char * shade_model_sz;
};
typedef struct s_gl_render_options gl_render_options_t;

/// Grab the OpenGL render options state
void gl_grab_render_options_state( gl_render_options_t * po );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL buffer state
struct s_gl_buffer
{
    gl_depth_t   depth;      ///< depth buffer
    gl_stencil_t stencil;    ///< stencil buffer

    GLint      draw_buffer[1];

    GLint     index_writemask[1];
    GLfloat   index_clear_value[1];
    GLint     index_shift[1];
    GLint     index_offset[1];

    GLfloat   accum_clear_value[1];

    GLboolean color_writemask[1];
    GLfloat   color_clear_value[1];

    GLfloat   red_scale[1];
    GLfloat   red_bias[1];

    GLfloat   green_scale[1];
    GLfloat   green_bias[1];

    GLfloat   blue_scale[1];
    GLfloat   blue_bias[1];
};
typedef struct s_gl_buffer gl_buffer_t;

/// Grab the OpenGL buffer state
void gl_grab_buffer_state( gl_buffer_t * pb );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL clipping state
struct s_gl_clipping
{
    gl_scissor_t     scissor;
    gl_clip_plane_t  clip_plane[6];

    GLint            viewport[4];

    GLboolean        cull_face_enabled;
    GLint            cull_face_mode[1];
    const char     * cull_face_mode_sz;
    GLint            front_face[1];
    const char     * front_face_sz;
};
typedef struct s_gl_clipping gl_clipping_t;

/// Grab the OpenGL clipping state
void gl_grab_clipping_state( gl_clipping_t * ps );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL texturing state
struct s_gl_texturing
{
    GLboolean        texture_1d_enabled;
    gl_texture_t     texture_1d;

    GLboolean        texture_2d_enabled;
    gl_texture_t     texture_2d;

    GLint            env_mode[1];
    GLfloat          env_color[4];

    gl_texture_gen_t texture_gen[4];
};
typedef struct s_gl_texturing gl_texturing_t;

/// Grab the OpenGL texturing state
void gl_grab_texturing_state( gl_texturing_t * pt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL attrib state
struct s_gl_attrib
{
    GLint client_stack_depth[1];
    GLint stack_depth[1];
};
typedef struct s_gl_attrib gl_attrib_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL logic op state
struct s_gl_logic_op
{
    GLint        mode[1];
    const char * mode_sz;

    GLboolean index_enabled;
    GLboolean color_enabled;
};
typedef struct s_gl_logic_op gl_logic_op_t;

/// Grab the OpenGL logic op state
void gl_grab_logic_op_state( gl_logic_op_t * pl );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// An encapsulation of the OpenGL mapping state
struct s_gl_mapping
{
    GLboolean map_color[1];
    GLboolean map_stencil[1];
};
typedef struct s_gl_mapping gl_mapping_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// An encapsulation of the OpenGL state
struct s_ogl_state
{
    gl_texturing_t      texturing;
    gl_clipping_t       clipping;
    gl_render_options_t options;
    gl_mapping_t        mapping;

    gl_packing_t        pack;
    gl_packing_t        unpack;

    gl_matrix_t         matrix;
    gl_attrib_t         attrib;
    gl_logic_op_t       logic_op;

    GLenum              error_value;
    const GLubyte *     error_value_sz;

    GLfloat zoom_x[1];
    GLfloat zoom_y[1];
};
typedef struct s_ogl_state ogl_state_t;

/// Grab the entire OpenGL state
void oglx_grab_state( ogl_state_t * ps );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_ogl_state_comp
{
    GLboolean state;

    GLboolean texturing;
    GLboolean clipping;
    GLboolean options;
    GLboolean mapping;

    GLboolean pack;
    GLboolean unpack;

    GLboolean matrix;
    GLboolean attrib;
    GLboolean logic_op;
};
typedef struct s_ogl_state_comp ogl_state_comp_t;

/// Compare two OpenGL states to see what parameters have changed.
/// If this function compares the states going into, and coming out of
/// a drawing routine, you can determine whether the routine is re-entrant,
/// or if not, which module the difference is in.
GLboolean gl_comp_state( ogl_state_comp_t * pcomp, ogl_state_t * ps1, ogl_state_t * ps2 );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern const char * next_cmd;
extern int          next_line;
extern const char * next_file;

/// grab a text representation of an OpenGL error
void handle_gl_error();

/// print out the text representation of an OpenGL error generated by handle_gl_error()
void print_gl_command();

#if defined(USE_GL_DEBUG)

/// a wrapper function for automatically debugging OpenGL function calls
/// Usage: GL_DEBUG( gl* )( param1, param2, ... )
/// @warning Wrapping glEnd() in this manner will generat a multitude of odd errors.
#    define GL_DEBUG(XX) \
    (handle_gl_error(), \
     next_cmd = #XX, \
     next_line = __LINE__, \
     next_file = __FILE__, \
     XX)

/// a special wrapper function that is the replacement for "GL_DEBUG( glEnd )()"
/// this avoids the errors mentioned with the definition of GL_DEBUG()
#    define GL_DEBUG_END() \
    handle_gl_error(); \
    next_cmd = "UNKNOWN"; \
    next_line = -1; \
    next_file = "UNKNOWN"; \
    glEnd(); \
    glGetError();

#else

/// this macro is set to do nothing if USE_GL_DEBUG is not defined
#    define GL_DEBUG(XX)   XX

/// this macro is set to the normal glEnd() USE_GL_DEBUG is not defined
#    define GL_DEBUG_END() glEnd();

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Set the FILE that ogl_debug will use to dump debugging information.
/// If not set, it will default to stderr.
FILE * set_ogl_debug_stderr( FILE * pfile );

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

/// @file extensions/ogl_extensions.c
/// @ingroup _ogl_extensions_
/// @brief Implementation of extended functions and variables for OpenGL
/// @details

#include "ogl_extensions.h"
#include "ogl_debug.h"

#include <math.h>
#include <string.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define LOCAL_STDOUT ((NULL == _oglx_stdout) ? stdout : _oglx_stdout)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oglx_caps_t ogl_caps;

static FILE * _oglx_stdout = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void oglx_report_caps()
{
    oglx_Get_Screen_Info( &ogl_caps );

    fprintf( LOCAL_STDOUT, "\nOpenGL state parameters\n" );
    fprintf( LOCAL_STDOUT, "\tgl_version    == %s\n", ogl_caps.gl_version );
    fprintf( LOCAL_STDOUT, "\tgl_vendor     == %s\n", ogl_caps.gl_vendor );
    fprintf( LOCAL_STDOUT, "\tgl_renderer   == %s\n", ogl_caps.gl_renderer );
    fprintf( LOCAL_STDOUT, "\tgl_extensions == %s\n", ogl_caps.gl_extensions );

    fprintf( LOCAL_STDOUT, "\tglu_version    == %s\n", ogl_caps.glu_version );
    fprintf( LOCAL_STDOUT, "\tglu_extensions == %s\n\n", ogl_caps.glu_extensions );

    fprintf( LOCAL_STDOUT, "\tGL_MAX_MODELVIEW_STACK_DEPTH     == %d\n", ogl_caps.max_modelview_stack_depth );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_PROJECTION_STACK_DEPTH    == %d\n", ogl_caps.max_projection_stack_depth );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_TEXTURE_STACK_DEPTH       == %d\n", ogl_caps.max_texture_stack_depth );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_NAME_STACK_DEPTH          == %d\n", ogl_caps.max_name_stack_depth );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_ATTRIB_STACK_DEPTH        == %d\n", ogl_caps.max_attrib_stack_depth );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_CLIENT_ATTRIB_STACK_DEPTH == %d\n\n", ogl_caps.max_client_attrib_stack_depth );

    fprintf( LOCAL_STDOUT, "\tGL_SUBPIXEL_BITS          == %d\n",      ogl_caps.subpixel_bits );
    fprintf( LOCAL_STDOUT, "\tGL_POINT_SIZE_RANGE       == %f - %f\n", ogl_caps.point_size_range[0], ogl_caps.point_size_range[1] );
    fprintf( LOCAL_STDOUT, "\tGL_POINT_SIZE_GRANULARITY == %f\n",      ogl_caps.point_size_granularity );
    fprintf( LOCAL_STDOUT, "\tGL_LINE_WIDTH_RANGE       == %f - %f\n", ogl_caps.line_width_range[0], ogl_caps.line_width_range[1] );
    fprintf( LOCAL_STDOUT, "\tGL_LINE_WIDTH_GRANULARITY == %f\n\n",    ogl_caps.line_width_granularity );

    fprintf( LOCAL_STDOUT, "\tGL_MAX_VIEWPORT_DIMS == %d, %d\n", ogl_caps.max_viewport_dims[0], ogl_caps.max_viewport_dims[1] );
    fprintf( LOCAL_STDOUT, "\tGL_AUX_BUFFERS       == %d\n", ogl_caps.aux_buffers );
    fprintf( LOCAL_STDOUT, "\tGL_RGBA_MODE         == %s\n", ogl_caps.rgba_mode ? "TRUE" : "FALSE" );
    fprintf( LOCAL_STDOUT, "\tGL_INDEX_MODE        == %s\n", ogl_caps.index_mode ? "TRUE" : "FALSE" );
    fprintf( LOCAL_STDOUT, "\tGL_DOUBLEBUFFER      == %s\n", ogl_caps.doublebuffer ? "TRUE" : "FALSE" );
    fprintf( LOCAL_STDOUT, "\tGL_STEREO            == %s\n", ogl_caps.stereo ? "TRUE" : "FALSE" );
    fprintf( LOCAL_STDOUT, "\tGL_RED_BITS          == %d\n", ogl_caps.red_bits );
    fprintf( LOCAL_STDOUT, "\tGL_GREEN_BITS        == %d\n", ogl_caps.green_bits );
    fprintf( LOCAL_STDOUT, "\tGL_BLUE_BITS         == %d\n", ogl_caps.blue_bits );
    fprintf( LOCAL_STDOUT, "\tGL_ALPHA_BITS        == %d\n", ogl_caps.alpha_bits );
    fprintf( LOCAL_STDOUT, "\tGL_INDEX_BITS        == %d\n", ogl_caps.index_bits );
    fprintf( LOCAL_STDOUT, "\tGL_DEPTH_BITS        == %d\n", ogl_caps.depth_bits );
    fprintf( LOCAL_STDOUT, "\tGL_STENCIL_BITS      == %d\n", ogl_caps.stencil_bits );
    fprintf( LOCAL_STDOUT, "\tGL_ACCUM_RED_BITS    == %d\n", ogl_caps.accum_red_bits );
    fprintf( LOCAL_STDOUT, "\tGL_ACCUM_GREEN_BITS  == %d\n", ogl_caps.accum_green_bits );
    fprintf( LOCAL_STDOUT, "\tGL_ACCUM_BLUE_BITS   == %d\n", ogl_caps.accum_blue_bits );
    fprintf( LOCAL_STDOUT, "\tGL_ACCUM_ALPHA_BITS  == %d\n\n", ogl_caps.accum_alpha_bits );

    fprintf( LOCAL_STDOUT, "\tGL_MAX_LIGHTS        == %d\n",   ogl_caps.max_lights );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_CLIP_PLANES   == %d\n",   ogl_caps.max_clip_planes );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_TEXTURE_SIZE  == %d\n\n", ogl_caps.max_texture_size );

    fprintf( LOCAL_STDOUT, "\tGL_MAX_PIXEL_MAP_TABLE == %d\n",   ogl_caps.max_pixel_map_table );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_LIST_NESTING    == %d\n",   ogl_caps.max_list_nesting );
    fprintf( LOCAL_STDOUT, "\tGL_MAX_EVAL_ORDER      == %d\n\n", ogl_caps.max_eval_order );

    if ( ogl_caps.anisotropic_supported )
    {
        fprintf( LOCAL_STDOUT, "\tGL_MAX_TEXTURE_MAX_ANISOTROPY_EXT == %f\n", ogl_caps.maxAnisotropy );
    }

    fprintf( LOCAL_STDOUT, "==============================================================\n" );

    fflush( LOCAL_STDOUT );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void oglx_bind( GLenum target, GLuint id, GLint wrap_s, GLint wrap_t, GLint min_f, GLint mag_f, GLfloat anisotropy )
{
    GL_DEBUG( glBindTexture )( target, id );
    GL_DEBUG( glTexParameteri )( target, GL_TEXTURE_WRAP_S, wrap_s );
    GL_DEBUG( glTexParameteri )( target, GL_TEXTURE_WRAP_T, wrap_t );

    GL_DEBUG( glTexParameteri )( target, GL_TEXTURE_MAG_FILTER, mag_f );
    GL_DEBUG( glTexParameteri )( target, GL_TEXTURE_MIN_FILTER, min_f );

    if ( GL_TEXTURE_2D == target && ogl_caps.anisotropic_supported && anisotropy > 1.0f )
    {
        GL_DEBUG( glTexParameterf )( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy );
    }
}

//--------------------------------------------------------------------------------------------
void oglx_upload_1d( GLboolean use_alpha, GLsizei w, const GLvoid * data )
{
    if ( use_alpha )
    {
        GL_DEBUG( glTexImage1D )( GL_TEXTURE_1D, 0, GL_RGBA, w, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
    }
    else
    {
        GL_DEBUG( glTexImage1D )( GL_TEXTURE_1D, 0, GL_RGB, w, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data );
    }
}

//--------------------------------------------------------------------------------------------
void oglx_upload_2d( GLboolean use_alpha, GLsizei w, GLsizei h, const GLvoid * data )
{
    if ( use_alpha )
    {
        GL_DEBUG( glTexImage2D )( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
    }
    else
    {
        GL_DEBUG( glTexImage2D )( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data );
    }
}

//--------------------------------------------------------------------------------------------
void oglx_upload_2d_mipmap( GLboolean use_alpha, GLsizei w, GLsizei h, const GLvoid * data )
{
    if ( use_alpha )
    {
        GL_DEBUG( gluBuild2DMipmaps )( GL_TEXTURE_2D, 4, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data );
    }
    else
    {
        GL_DEBUG( gluBuild2DMipmaps )( GL_TEXTURE_2D, GL_RGB, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, data );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void oglx_Get_Screen_Info( oglx_caps_t * pcaps )
{

    memset( pcaps, 0, sizeof( *pcaps ) );

    // get any pure OpenGL device caps

    pcaps->gl_version     = GL_DEBUG( glGetString )( GL_VERSION );
    pcaps->gl_vendor      = GL_DEBUG( glGetString )( GL_VENDOR );
    pcaps->gl_renderer    = GL_DEBUG( glGetString )( GL_RENDERER );
    pcaps->gl_extensions  = GL_DEBUG( glGetString )( GL_EXTENSIONS );

    pcaps->glu_version    = GL_DEBUG( gluGetString )( GL_VERSION );
    pcaps->glu_extensions = GL_DEBUG( gluGetString )( GL_EXTENSIONS );

    GL_DEBUG( glGetIntegerv )( GL_MAX_MODELVIEW_STACK_DEPTH,     &pcaps->max_modelview_stack_depth );
    GL_DEBUG( glGetIntegerv )( GL_MAX_PROJECTION_STACK_DEPTH,    &pcaps->max_projection_stack_depth );
    GL_DEBUG( glGetIntegerv )( GL_MAX_TEXTURE_STACK_DEPTH,       &pcaps->max_texture_stack_depth );
    GL_DEBUG( glGetIntegerv )( GL_MAX_NAME_STACK_DEPTH,          &pcaps->max_name_stack_depth );
    GL_DEBUG( glGetIntegerv )( GL_MAX_ATTRIB_STACK_DEPTH,        &pcaps->max_attrib_stack_depth );
    GL_DEBUG( glGetIntegerv )( GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, &pcaps->max_client_attrib_stack_depth );

    GL_DEBUG( glGetIntegerv )( GL_SUBPIXEL_BITS,        &pcaps->subpixel_bits );
    GL_DEBUG( glGetFloatv )( GL_POINT_SIZE_RANGE,        pcaps->point_size_range );
    GL_DEBUG( glGetFloatv )( GL_POINT_SIZE_GRANULARITY, &pcaps->point_size_granularity );
    GL_DEBUG( glGetFloatv )( GL_LINE_WIDTH_RANGE,        pcaps->line_width_range );
    GL_DEBUG( glGetFloatv )( GL_LINE_WIDTH_GRANULARITY, &pcaps->line_width_granularity );

    GL_DEBUG( glGetIntegerv )( GL_MAX_VIEWPORT_DIMS, pcaps->max_viewport_dims );
    GL_DEBUG( glGetBooleanv )( GL_AUX_BUFFERS,      &pcaps->aux_buffers );
    GL_DEBUG( glGetBooleanv )( GL_RGBA_MODE,        &pcaps->rgba_mode );
    GL_DEBUG( glGetBooleanv )( GL_INDEX_MODE,       &pcaps->index_mode );
    GL_DEBUG( glGetBooleanv )( GL_DOUBLEBUFFER,     &pcaps->doublebuffer );
    GL_DEBUG( glGetBooleanv )( GL_STEREO,           &pcaps->stereo );
    GL_DEBUG( glGetIntegerv )( GL_RED_BITS,         &pcaps->red_bits );
    GL_DEBUG( glGetIntegerv )( GL_GREEN_BITS,       &pcaps->green_bits );
    GL_DEBUG( glGetIntegerv )( GL_BLUE_BITS,        &pcaps->blue_bits );
    GL_DEBUG( glGetIntegerv )( GL_ALPHA_BITS,       &pcaps->alpha_bits );
    GL_DEBUG( glGetIntegerv )( GL_INDEX_BITS,       &pcaps->index_bits );
    GL_DEBUG( glGetIntegerv )( GL_DEPTH_BITS,       &pcaps->depth_bits );
    GL_DEBUG( glGetIntegerv )( GL_STENCIL_BITS,     &pcaps->stencil_bits );
    GL_DEBUG( glGetIntegerv )( GL_ACCUM_RED_BITS,   &pcaps->accum_red_bits );
    GL_DEBUG( glGetIntegerv )( GL_ACCUM_GREEN_BITS, &pcaps->accum_green_bits );
    GL_DEBUG( glGetIntegerv )( GL_ACCUM_BLUE_BITS,  &pcaps->accum_blue_bits );
    GL_DEBUG( glGetIntegerv )( GL_ACCUM_ALPHA_BITS, &pcaps->accum_alpha_bits );

    GL_DEBUG( glGetIntegerv )( GL_MAX_LIGHTS,       &pcaps->max_lights );
    GL_DEBUG( glGetIntegerv )( GL_MAX_CLIP_PLANES,  &pcaps->max_clip_planes );
    GL_DEBUG( glGetIntegerv )( GL_MAX_TEXTURE_SIZE, &pcaps->max_texture_size );

    GL_DEBUG( glGetIntegerv )( GL_MAX_PIXEL_MAP_TABLE, &pcaps->max_pixel_map_table );
    GL_DEBUG( glGetIntegerv )( GL_MAX_LIST_NESTING,    &pcaps->max_list_nesting );
    GL_DEBUG( glGetIntegerv )( GL_MAX_EVAL_ORDER,      &pcaps->max_eval_order );

    pcaps->maxAnisotropy  = 0;
    pcaps->log2Anisotropy = 0;

    /// get the supported values for anisotropic filtering
    pcaps->anisotropic_supported = GL_FALSE;
    pcaps->maxAnisotropy  = 1.0f;
    pcaps->log2Anisotropy = 0.0f;
    if ( NULL != pcaps->gl_extensions && NULL != strstr(( char* )pcaps->gl_extensions, "GL_EXT_texture_filter_anisotropic" ) )
    {
        pcaps->anisotropic_supported = GL_TRUE;
        GL_DEBUG( glGetFloatv )( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &( pcaps->maxAnisotropy ) );
        pcaps->log2Anisotropy = ( 0 == pcaps->maxAnisotropy ) ? 0.0f : floor( log( pcaps->maxAnisotropy + 1e-6 ) / log( 2.0f ) );
    }
}

//--------------------------------------------------------------------------------------------
GLboolean oglx_video_parameters_default( oglx_video_parameters_t * pvid )
{
    if ( NULL == pvid ) return GL_FALSE;

    pvid->multisample     = GL_FALSE;            ///< current antialiasing used through GL_MULTISAMPLES
    pvid->multisample_arb = GL_FALSE;            ///< current antialiasing used through GL_MULTISAMPLES_ARB
    pvid->perspective     = GL_FASTEST;          ///< current correction hint
    pvid->dither          = GL_FALSE;            ///< current dithering flag
    pvid->shading         = GL_SMOOTH;           ///< current shading type
    pvid->userAnisotropy  = 0.0f;

    return GL_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
FILE * oglx_set_stdout( FILE * pfile )
{
    FILE * pfile_old = _oglx_stdout;

    if ( NULL == pfile )
    {
        _oglx_stdout = stdout;
    }
    else
    {
        _oglx_stdout = pfile;
    }

    return pfile_old;
}

#include "ogl_extensions.h"
#include "ogl_debug.h"

#include <math.h>

oglx_caps_t ogl_caps;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void oglx_bind(GLenum target, GLuint id, GLint wrap_s, GLint wrap_t, GLfloat mag_f, GLfloat min_f, GLfloat anisotropy )
{
    glBindTexture( target, id );
    glTexParameteri( target, GL_TEXTURE_WRAP_S, wrap_s );
    glTexParameteri( target, GL_TEXTURE_WRAP_T, wrap_t );

    glTexParameterf( target, GL_TEXTURE_MAG_FILTER, mag_f );
    glTexParameterf( target, GL_TEXTURE_MIN_FILTER, min_f );

    if ( ogl_caps.anisotropic_supported )
    {
        glTexParameterf( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy );
    }
};

//------------------------------------------------------------------------------
void oglx_upload_1d(GLboolean use_alpha, GLsizei w, const GLvoid * data)
{
    if (use_alpha)
    {
        glTexImage1D( GL_TEXTURE_1D, 0, GL_RGBA, w, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
    }
    else
    {
        glTexImage1D( GL_TEXTURE_1D, 0, GL_RGB, w, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data );
    }
}

//------------------------------------------------------------------------------
void oglx_upload_2d(GLboolean use_alpha, GLsizei w, GLsizei h, const GLvoid * data)
{
    if (use_alpha)
    {
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
    }
    else
    {
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data );
    }
}

//------------------------------------------------------------------------------
void oglx_upload_2d_mipmap(GLboolean use_alpha, GLsizei w, GLsizei h, const GLvoid * data)
{
    if (use_alpha)
    {
        gluBuild2DMipmaps( GL_TEXTURE_2D, 4, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data );
    }
    else
    {
        gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, data );
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void oglx_Get_Screen_Info( oglx_caps_t * pcaps )
{

    memset(pcaps, 0, sizeof(oglx_caps_t));

    // get any pure OpenGL device caps

    pcaps->gl_version     = glGetString(GL_VERSION);
    pcaps->gl_vendor      = glGetString(GL_VENDOR);
    pcaps->gl_renderer    = glGetString(GL_RENDERER);
    pcaps->gl_extensions  = glGetString(GL_EXTENSIONS);

    pcaps->glu_version    = gluGetString(GL_VERSION);
    pcaps->glu_extensions = gluGetString(GL_EXTENSIONS);

    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH,     &pcaps->max_modelview_stack_depth);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH,    &pcaps->max_projection_stack_depth);
    glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH,       &pcaps->max_texture_stack_depth);
    glGetIntegerv(GL_MAX_NAME_STACK_DEPTH,          &pcaps->max_name_stack_depth);
    glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH,        &pcaps->max_attrib_stack_depth);
    glGetIntegerv(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, &pcaps->max_client_attrib_stack_depth);

    glGetIntegerv(GL_SUBPIXEL_BITS,        &pcaps->subpixel_bits);
    glGetFloatv(GL_POINT_SIZE_RANGE,        pcaps->point_size_range);
    glGetFloatv(GL_POINT_SIZE_GRANULARITY, &pcaps->point_size_granularity);
    glGetFloatv(GL_LINE_WIDTH_RANGE,        pcaps->line_width_range);
    glGetFloatv(GL_LINE_WIDTH_GRANULARITY, &pcaps->line_width_granularity);

    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, pcaps->max_viewport_dims);
    glGetBooleanv(GL_AUX_BUFFERS,      &pcaps->aux_buffers);
    glGetBooleanv(GL_RGBA_MODE,        &pcaps->rgba_mode);
    glGetBooleanv(GL_INDEX_MODE,       &pcaps->index_mode);
    glGetBooleanv(GL_DOUBLEBUFFER,     &pcaps->doublebuffer);
    glGetBooleanv(GL_STEREO,           &pcaps->stereo);
    glGetIntegerv(GL_RED_BITS,         &pcaps->red_bits);
    glGetIntegerv(GL_GREEN_BITS,       &pcaps->green_bits);
    glGetIntegerv(GL_BLUE_BITS,        &pcaps->blue_bits);
    glGetIntegerv(GL_ALPHA_BITS,       &pcaps->alpha_bits);
    glGetIntegerv(GL_INDEX_BITS,       &pcaps->index_bits);
    glGetIntegerv(GL_DEPTH_BITS,       &pcaps->depth_bits);
    glGetIntegerv(GL_STENCIL_BITS,     &pcaps->stencil_bits);
    glGetIntegerv(GL_ACCUM_RED_BITS,   &pcaps->accum_red_bits);
    glGetIntegerv(GL_ACCUM_GREEN_BITS, &pcaps->accum_green_bits);
    glGetIntegerv(GL_ACCUM_BLUE_BITS,  &pcaps->accum_blue_bits);
    glGetIntegerv(GL_ACCUM_ALPHA_BITS, &pcaps->accum_alpha_bits);

    glGetIntegerv(GL_MAX_LIGHTS,       &pcaps->max_lights);
    glGetIntegerv(GL_MAX_CLIP_PLANES,  &pcaps->max_clip_planes);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &pcaps->max_texture_size);

    glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &pcaps->max_pixel_map_table);
    glGetIntegerv(GL_MAX_LIST_NESTING,    &pcaps->max_list_nesting);
    glGetIntegerv(GL_MAX_EVAL_ORDER,      &pcaps->max_eval_order);

    pcaps->maxAnisotropy  = 0;
    pcaps->log2Anisotropy = 0;

    /// get the supported values for anisotropic filtering
    pcaps->anisotropic_supported = GL_FALSE;
    pcaps->maxAnisotropy  = 1.0f;
    pcaps->log2Anisotropy = 0.0f;
    if ( NULL != pcaps->gl_extensions && NULL != strstr((char*)pcaps->gl_extensions, "GL_EXT_texture_filter_anisotropic") )
    {
        pcaps->anisotropic_supported = GL_TRUE;
        glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &(pcaps->maxAnisotropy) );
        pcaps->log2Anisotropy = ( 0 == pcaps->maxAnisotropy ) ? 0 : floor( log( pcaps->maxAnisotropy + 1e-6 ) / log( 2.0f ) );
    }

}

//------------------------------------------------------------------------------
GLboolean oglx_video_parameters_default(oglx_video_parameters_t * pvid)
{
    if (NULL == pvid) return GL_FALSE;

    pvid->antialiasing   = GL_FALSE;            ///< current antialiasing value
    pvid->perspective    = GL_FASTEST;          ///< current correction hint
    pvid->dither         = GL_FALSE;            ///< current dithering flag
    pvid->shading        = GL_SMOOTH;           ///< current shading type
    pvid->userAnisotropy = 0.0f;

    return GL_TRUE;
}


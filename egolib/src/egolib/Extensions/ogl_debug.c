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

/// @file egolib/Extensions/ogl_debug.c
/// @ingroup _ogl_extensions_
/// @brief Implementation of the debugging extensions for OpenGL
/// @details

#include "egolib/Extensions/ogl_debug.h"
#include "egolib/file_common.h"
#include "egolib/log.h"

//--------------------------------------------------------------------------------------------

const char * next_cmd = NULL;
int          next_line = -1;
const char * next_file = "BAD FILE";

void handle_gl_error()
{
    GLint err = glGetError();
    if ( GL_NO_ERROR != err )
    {
        const GLubyte * err_str = gluErrorString( err );
        log_warning("%s (\"%s\" - %d)- %s\n", next_cmd, next_file, next_line, err_str);
    }
}

void print_gl_command()
{
    log_warning("%s (\"%s\" - %d)\n", next_cmd, next_file, next_line);
}

//--------------------------------------------------------------------------------------------

void gl_grab_texture_state(gl_texture_t *self, GLenum target, GLint level)
{
    GLint itmp;

    // which texture id
    if ( GL_TEXTURE_1D == target )
    {
        self->target = GL_TEXTURE_1D;
        GL_DEBUG( glGetIntegerv )( GL_TEXTURE_BINDING_1D, &itmp );
        self->binding = itmp;
    }
    else if ( GL_TEXTURE_2D == target )
    {
        self->target = GL_TEXTURE_2D;
        GL_DEBUG( glGetIntegerv )( GL_TEXTURE_BINDING_2D, &itmp );
        self->binding = itmp;
    }

    // basic parameters
    GL_DEBUG(glGetTexParameterfv)(target, GL_TEXTURE_PRIORITY, &self->priority);
    GL_DEBUG(glGetTexParameteriv)(target, GL_TEXTURE_RESIDENT, &self->resident);
    GL_DEBUG(glGetTexParameteriv)(target, GL_TEXTURE_WRAP_S, &self->wrap_s);
    GL_DEBUG(glGetTexParameteriv)(target, GL_TEXTURE_WRAP_T, &self->wrap_t);
    GL_DEBUG(glGetTexParameteriv)(target, GL_TEXTURE_MIN_FILTER, &self->min_filter);
    GL_DEBUG(glGetTexParameteriv)(target, GL_TEXTURE_MAG_FILTER, &self->mag_filter);
    GL_DEBUG(glGetTexParameterfv)(target, GL_TEXTURE_BORDER_COLOR, self->border_color);

    // format
    GL_DEBUG(glGetTexLevelParameterfv)(target, level, GL_TEXTURE_WIDTH, &self->width);
    GL_DEBUG(glGetTexLevelParameterfv)(target, level, GL_TEXTURE_HEIGHT, &self->height);
    GL_DEBUG(glGetTexLevelParameteriv)(target, level, GL_TEXTURE_INTERNAL_FORMAT, &self->internal_format);
    GL_DEBUG(glGetTexLevelParameteriv)(target, level, GL_TEXTURE_RED_SIZE, &self->red_size);
    GL_DEBUG(glGetTexLevelParameteriv)(target, level, GL_TEXTURE_GREEN_SIZE, &self->green_size);
    GL_DEBUG(glGetTexLevelParameteriv)(target, level, GL_TEXTURE_BLUE_SIZE, &self->blue_size);
    GL_DEBUG(glGetTexLevelParameteriv)(target, level, GL_TEXTURE_ALPHA_SIZE, &self->alpha_size);
    GL_DEBUG(glGetTexLevelParameteriv)(target, level, GL_TEXTURE_LUMINANCE_SIZE, &self->luminance_size);
    GL_DEBUG(glGetTexLevelParameteriv)(target, level, GL_TEXTURE_INTENSITY_SIZE, &self->intensity_size);
    GL_DEBUG(glGetTexLevelParameteriv)(target, level, GL_TEXTURE_BORDER, &self->border);
}

void gl_grab_texture_gen_state(gl_texture_gen_t *self, GLenum coord)
{
    GL_DEBUG(glGetTexGenfv)(coord, GL_EYE_PLANE, self->eye_plane);
    GL_DEBUG(glGetTexGenfv)(coord, GL_OBJECT_PLANE, self->object_plane);
    GL_DEBUG(glGetTexGenfv)(coord, GL_OBJECT_PLANE, self->object_plane);
    GL_DEBUG(glGetTexGeniv)(coord, GL_TEXTURE_GEN_MODE, self->mode);
}

void gl_grab_texturing_state(gl_texturing_t *self)
{
    // texture environment
    GL_DEBUG(glGetTexEnviv)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, self->env_mode);
    GL_DEBUG(glGetTexEnvfv)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, self->env_color);

    self->texture_1d_enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_1D);
    if (GL_TRUE == self->texture_1d_enabled)
    {
        gl_grab_texture_state(&self->texture_1d, GL_TEXTURE_1D, 0);
    }

    self->texture_2d_enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_2D);
    if (GL_TRUE == self->texture_2d_enabled)
    {
        gl_grab_texture_state(&self->texture_2d, GL_TEXTURE_2D, 0);
    }

    self->texture_gen[0].enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_GEN_S);
    if (self->texture_gen[0].enabled)
    {
        gl_grab_texture_gen_state(self->texture_gen + 0, GL_S);
    }

    self->texture_gen[1].enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_GEN_T);
    if (self->texture_gen[1].enabled)
    {
        gl_grab_texture_gen_state(self->texture_gen + 1, GL_T);
    }

    self->texture_gen[2].enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_GEN_R);
    if (self->texture_gen[2].enabled)
    {
        gl_grab_texture_gen_state(self->texture_gen + 2, GL_R);
    }

    self->texture_gen[3].enabled = GL_DEBUG(glIsEnabled)(GL_TEXTURE_GEN_Q);
    if (self->texture_gen[3].enabled)
    {
        gl_grab_texture_gen_state(self->texture_gen + 3, GL_Q);
    }
}

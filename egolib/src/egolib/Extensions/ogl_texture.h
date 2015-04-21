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

/// @file egolib/Extensions/ogl_texture.h
/// @ingroup _ogl_extensions_
/// @brief Definitions for OpenGL texture loading using SDL_image
/// @details

#pragma once

#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Renderer/TextureFilter.hpp"

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


    typedef GLfloat oglx_frect_t[4];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define INVALID_KEY ((Uint32)(~0))
#define VALID_BINDING(BIND) ((0 != (BIND)) && (INVALID_GL_ID != (BIND)))
#define ERROR_IMAGE_BINDING(BIND) (ErrorImage_get_binding() == (BIND))

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    /// An encapsulation of the OpenGL texture state.
    struct gl_texture_t
    {
        GLenum  target;
        GLuint  binding;              /*< The OpenGL texture number */

        GLfloat width;
        GLfloat height;

        GLint   wrap_s;
        GLint   wrap_t;

    };

    struct oglx_texture_t
    {
        GLboolean base_valid;
        gl_texture_t base;

        GLuint valid;          ///< whether or not the texture has been initialized
        char name[256];        ///< the name of the original file
        int imgW, imgH;        ///< the height & width of the texture data

        SDL_Surface *surface;  ///< the original texture data
        SDL_bool has_alpha;    ///< the alpha for the texture

    public:
        static oglx_texture_t *ctor(oglx_texture_t *self);
        static void dtor(oglx_texture_t *self);
        
    public:
        static oglx_texture_t *create();
        static void destroy(oglx_texture_t *self);

    public:
        static GLuint convert(oglx_texture_t *self, SDL_Surface *image, Uint32 key);
        static GLuint load(oglx_texture_t *self, const char *filename, Uint32 key);
        static GLuint load(oglx_texture_t *self, const char *name, SDL_Surface *surface, Uint32 key);
        /**
         * @brief
         *	Delete backing image, delete OpenGL ID, assign OpenGL ID of the error texture, assign no backing image.
         * @param self
         *	this texture
         */
        static void release(oglx_texture_t *self);
        static void bind(oglx_texture_t *self);

    public:
        static GLuint getTextureID(const oglx_texture_t *self);
        
    public:
        static GLsizei getTextureWidth(const oglx_texture_t *self);
        static GLsizei getTextureHeight(const oglx_texture_t *self);
        static GLsizei getImageHeight(const oglx_texture_t *self);
        static GLsizei getImageWidth(const oglx_texture_t *self);
        static GLboolean getSize(const oglx_texture_t *self, oglx_frect_t tx_rect, oglx_frect_t img_rect);
    };

    void oglx_texture_setAlpha(oglx_texture_t *self, GLfloat alpha);
    GLfloat oglx_texture_getAlpha(const oglx_texture_t *self);
    GLboolean oglx_texture_Valid(oglx_texture_t *ptex);
    GLuint oglx_bind_to_tex_params(GLuint binding, GLenum target, GLint wrap_s, GLint wrap_t);
    void ErrorImage_bind(GLenum target, GLuint id);
    GLuint ErrorImage_get_binding();

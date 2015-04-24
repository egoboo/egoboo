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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    /// An encapsulation of the OpenGL texture state.
    struct oglx_texture_t
    {
        
        /**
         * @brief
         *  The texture type.
         */
        GLenum  _type;
        
        /**
         * @brief
         *  The texture address mode along the s-axis.
         */
        GLint   _wrapS;
        
        /**
         * @brief
         *  The texture address mode along the t-axis.
         */
        GLint   _wrapT;

        char name[256];        ///< the name of the original file

    protected:

        /**
         * @brief
         *  The OpenGL texture ID.
         * @remark
         *  At any point, a texture has a valid OpenGL texture ID assigned, <em>unless</em> resources were lost.
         */
        GLuint  _id;

        /**
         * @brief
         *  The width, in pixels, of the source of this texture.
         * @remark
         *  This value might differ for technical reasons from the width of the texture.
         */
        int _sourceWidth;

        /**
         * @brief
         *  The height, in pixels, of the source of this texture.
         * @remark
         *  This value might differ for technical reasons from the height of the texture.
         */
        int _sourceHeight;

        /**
         * @brief
         *  The width, in pixels, of the the texture.
         */
        int _width;

        /**
         * @brief
         *  The height, in pixels, of the texture.
         */
        int _height;

        /**
         * @brief
         *  @a true if this texture has an alpha component, @a false otherwise.
         */
        bool _hasAlpha;

    public:

        /**
         * @brief
         *  A pointer to the source of the texture if available, a null pointer otherwise.
         */
        SDL_Surface *source;

    public:
        /**
         * @brief
         *  Construct this texture.
         * @post
         *  This texture is bound to the backing error texture.
         */
        static oglx_texture_t *ctor(oglx_texture_t *self);
        /**
         * @brief
         *  Destruct this texture.
         */
        static void dtor(oglx_texture_t *self);
        
    public:
        /**
         * @brief
         *  Create a texture.
         * @post
         *  The texture is bound to the backing error texture.
         */
        static oglx_texture_t *create();
        static void destroy(oglx_texture_t *self);

    public:
        static GLuint load(oglx_texture_t *self, const char *name, SDL_Surface *surface, Uint32 key);
        static GLuint load(oglx_texture_t *self, SDL_Surface *image, Uint32 key);
        static GLuint load(oglx_texture_t *self, const char *filename, Uint32 key);

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

        /**
         * @brief
         *  Get the width, in pixels, of this texture.
         * @param self
         *  this texture
         * @return
         *  the width, in pixels of this texture
         */
        static GLsizei getWidth(const oglx_texture_t *self);

        /**
         * @brief
         *  Get the height, in pixels, of this texture.
         * @param self
         *  this texture
         * @return
         *  the height, in pixels of this texture
         */
        static GLsizei getHeight(const oglx_texture_t *self);

        /**
         * @brief
         *  Get the width, in pixels, of the source of this texture.
         * @param self
         *  this texture
         * @return
         *  the width, in pixels, of the source of this texture
         * @remark
         *  This value might differ for technical reasons from the width of the texture.
         */
        static GLsizei getSourceWidth(const oglx_texture_t *self);

        /**
         * @brief
         *  Get the height, in pixels, of the source of this texture.
         * @param self
         *  this texture
         * @return
         *  the height, in pixels, of the source of this texture
         * @remark
         *  This value might differ for technical reasons from the height of the texture.
         */
        static GLsizei getSourceHeight(const oglx_texture_t *self);

        /**
         * @brief
         *  Get if this texture has an alpha component.
         * @param self
         *  this texture
         * @return
         *  @a true if this texture has an alpha component,
         *  @a false otherwise
         */
        static bool hasAlpha(const oglx_texture_t *self);

        static GLboolean getSize(const oglx_texture_t *self, oglx_frect_t tx_rect, oglx_frect_t img_rect);
    };

    void oglx_bind_to_tex_params(GLuint binding, GLenum target, GLint wrap_s, GLint wrap_t);

    /**
     * @brief
     *  Initialize the error textures.
     * @todo
     *  Move into texture manager.
     */
    void initializeErrorTextures();
    /**
     * @brief
     *  Uninitialize the error textures.
     * @todo
     *  Move into texture manager.
     */
    void uninitializeErrorTextures();
    GLuint get2DErrorTextureID();
    GLuint get1DErrorTextureID();
    bool isErrorTextureID(GLuint id);

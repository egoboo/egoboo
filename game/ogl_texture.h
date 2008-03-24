#pragma once

/* Egoboo - gltexture.c
 * Loads BMP files into OpenGL textures.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _GLTEXTURE_H_
#define _GLTEXTURE_H_

#include "ogl_include.h"

#define INVALID_TEXTURE (~(GLuint)0)
#define INVALID_KEY     (~(Uint32)0)

/**> DATA STRUCTURE: GLtexture <**/
typedef struct ogl_texture_t
{
  GLuint textureID;    /* The OpenGL texture number */
  GLint   internalFormat;  /* GL_RGB or GL_RGBA */
  GLsizei imgH, imgW;      /* the height & width of the original image */
  GLsizei txH,  txW;     /* the height/width of the the OpenGL texture (must be a power of two) */
  GLfloat alpha;      /* the alpha for the texture */
  GLenum  texture_target;
} GLtexture;


/**> FUNCTION PROTOTYPES: GLtexture <**/
Uint32  GLTexture_Convert( GLenum tx_target, GLtexture *texture, SDL_Surface * image, Uint32 key );
Uint32  GLTexture_Load( GLenum tx_target, GLtexture *texture, const char *filename, Uint32 key );
GLuint  GLTexture_GetTextureID( GLtexture *texture );
GLsizei GLTexture_GetImageHeight( GLtexture *texture );
GLsizei GLTexture_GetImageWidth( GLtexture *texture );
GLsizei GLTexture_GetTextureWidth( GLtexture *texture );
GLsizei GLTexture_GetTextureHeight( GLtexture *texture );
void    GLTexture_SetAlpha( GLtexture *texture, GLfloat alpha );
GLfloat GLTexture_GetAlpha( GLtexture *texture );
void    GLTexture_Release( GLtexture *texture );
void    GLTexture_Bind( GLtexture * texture, int filt_type );


#endif


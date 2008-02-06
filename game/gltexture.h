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

#include <SDL.h>
#include <SDL_opengl.h>

#define INVALID_TEXTURE ((GLuint)(-1))
#define INVALID_KEY     ((Uint32)(-1))

/**> DATA STRUCTURE: GLTexture <**/
typedef struct gltexture_t
{
  GLuint textureID;    /* The OpenGL texture number */
  GLint   internalFormat;  /* GL_RGB or GL_RGBA */
  GLsizei imgH, imgW;      /* the height & width of the original image */
  GLsizei txH,  txW;     /* the height/width of the the OpenGL texture (must be a power of two) */
  GLfloat alpha;      /* the alpha for the texture */
  GLenum  texture_target;
} GLTexture;


/**> FUNCTION PROTOTYPES: GLTexture <**/
Uint32  GLTexture_Convert( GLenum tx_target, GLTexture *texture, SDL_Surface * image, Uint32 key );
Uint32  GLTexture_Load( GLenum tx_target, GLTexture *texture, const char *filename, Uint32 key );
GLuint  GLTexture_GetTextureID( GLTexture *texture );
GLsizei GLTexture_GetImageHeight( GLTexture *texture );
GLsizei GLTexture_GetImageWidth( GLTexture *texture );
GLsizei GLTexture_GetTextureWidth( GLTexture *texture );
GLsizei GLTexture_GetTextureHeight( GLTexture *texture );
void    GLTexture_SetAlpha( GLTexture *texture, GLfloat alpha );
GLfloat GLTexture_GetAlpha( GLTexture *texture );
void    GLTexture_Release( GLTexture *texture );
void    GLTexture_Bind( GLTexture * texture, int filt_type );


#endif


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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - GLtexture.c
 * Loads BMP files into OpenGL textures.
 */

#include <SDL.h>
#include <SDL_opengl.h>


/**> DATA STRUCTURE: GLtexture <**/
struct ogl_texture_t
{
    char    name[256];
    GLuint  textureID;       /* The OpenGL texture number */
    GLint   internalFormat;  /* GL_RGB or GL_RGBA */
    GLsizei imgH, imgW;      /* the height & width of the original image */
    GLsizei txH,  txW;       /* the height/width of the the OpenGL texture (must be a power of two) */
    GLfloat alpha;           /* the alpha for the texture */

    GLenum  texture_target;
    GLint   wrap_s, wrap_t;
};
typedef struct ogl_texture_t GLtexture;

/*
GLtexture * GLtexture_new( GLtexture * ptx );
void        GLtexture_delete( GLtexture * ptx );
*/
/**> FUNCTION PROTOTYPES: GLtexture <**/
/*void    GLSetup_SupportedFormats();
Uint32  GLtexture_Load( GLenum tx_target, GLtexture *texture, const char *filename, Uint32 key );
GLuint  GLtexture_GetTextureID( GLtexture *texture );
GLsizei GLtexture_GetImageHeight( GLtexture *texture );
GLsizei GLtexture_GetImageWidth( GLtexture *texture );
GLsizei GLtexture_GetTextureWidth( GLtexture *texture );
GLsizei GLtexture_GetTextureHeight( GLtexture *texture );
void    GLtexture_SetAlpha( GLtexture *texture, GLfloat alpha );
GLfloat GLtexture_GetAlpha( GLtexture *texture );
void    GLtexture_Release( GLtexture *texture );
void    GLtexture_Bind( GLtexture *texture );
Uint32  GLtexture_Convert( GLenum tx_target, GLtexture *texture, SDL_Surface * image, Uint32 key );
*/
#define  _GLtexture_H_

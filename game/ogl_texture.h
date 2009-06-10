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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

///
/// @file
/// @brief Basic OpenGL Wrapper
/// @details Basic definitions for loading and managing OpenGL textures in Egoboo.
///   Uses SDL_image to load .tif, .png, .bmp, .dib, .xpm, and other formats into
///   OpenGL texures

//#include "egoboo_memory.h"

#include "ogl_include.h"
#include "ogl_debug.h"
#include "egoboo_typedef.h"

#include "ogl_extensions.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define INVALID_TX_ID  ( (GLuint) (~0) )
#define INVALID_KEY    ( (Uint32) (~0) )

// OpenGL Texture filtering
typedef enum e_tx_filters
{
    TX_UNFILTERED,
    TX_LINEAR,
    TX_MIPMAP,
    TX_BILINEAR,
    TX_TRILINEAR_1,
    TX_TRILINEAR_2,
    TX_ANISOTROPIC
} TX_FILTERS;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_oglx_texture
{
    gl_texture_t base;

    char          name[256];       // the name of the original file
    SDL_Surface * surface;    // the original texture data
    int           imgW, imgH; // the height & width of the texture data
    GLfloat       alpha;      // the alpha for the texture
};
typedef struct s_oglx_texture GLXtexture;

GLuint  GLXtexture_Convert( GLenum tx_target, GLXtexture *texture, SDL_Surface * image, Uint32 key );
GLuint  GLXtexture_Load( GLenum tx_target, GLXtexture *texture, const char *filename, Uint32 key );
GLuint  GLXtexture_GetTextureID( GLXtexture *texture );
GLsizei GLXtexture_GetImageHeight( GLXtexture *texture );
GLsizei GLXtexture_GetImageWidth( GLXtexture *texture );
GLsizei GLXtexture_GetTextureWidth( GLXtexture *texture );
GLsizei GLXtexture_GetTextureHeight( GLXtexture *texture );
void    GLXtexture_SetAlpha( GLXtexture *texture, GLfloat alpha );
GLfloat GLXtexture_GetAlpha( GLXtexture *texture );
void    GLXtexture_Release( GLXtexture *texture );

void    GLXtexture_Bind( GLXtexture * texture );

GLXtexture * GLXtexture_new( GLXtexture * texture );
void        GLXtexture_delete( GLXtexture * texture );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_oglx_texture_parameters
{
    TX_FILTERS texturefilter;
    GLfloat    userAnisotropy;
};
typedef struct s_oglx_texture_parameters oglx_texture_parameters_t;

extern oglx_texture_parameters_t tex_params;


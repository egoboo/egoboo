#pragma once
//********************************************************************************************
//*
//*    This file is part of the SDL extensions library.
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

/// @file extensions/SDL_GL_extensions.h
/// @ingroup _sdl_extensions_
/// @brief Definitions for OpenGL extensions to SDL
/// @details

#include "ogl_extensions.h"
#include "SDL_extensions.h"

/// find the minimum value of 2^n that is larger than input
int powerOfTwo( int input );

/// Convert a SDL_Surface to an OpenGL texture directly.
/// Uses the SDL and OpenGL graphics options to upload the texture in the correct mode
GLuint SDL_GL_convert_surface( GLenum binding, SDL_Surface * surface, GLint wrap_s, GLint wrap_t );

/// Convert a SDL surface into an OpenGL texture
SDL_bool SDL_GL_uploadSurface( SDL_Surface *surface, GLuint texture, GLfloat *texCoords );

/// Set the OpenGL screen mode using SDL
SDLX_video_parameters_t * SDL_GL_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, oglx_video_parameters_t * gl_new, SDL_bool has_valid_mode );

extern const Uint32 sdl_a_mask;        ///< the mask to pick out alpha data from SDL's current pixel format
extern const Uint32 sdl_b_mask;        ///< the mask to pick out blue  data from SDL's current pixel format
extern const Uint32 sdl_g_mask;        ///< the mask to pick out green data from SDL's current pixel format
extern const Uint32 sdl_r_mask;        ///< the mask to pick out red   data from SDL's current pixel format

extern const Uint32 sdl_a_shift;        ///< bits to shift the alpha data of SDL's current pixel format
extern const Uint32 sdl_b_shift;        ///< bits to shift the blue  data of SDL's current pixel format
extern const Uint32 sdl_g_shift;        ///< bits to shift the green data of SDL's current pixel format
extern const Uint32 sdl_r_shift;        ///< bits to shift the red   data of SDL's current pixel format

/// Set the FILE that SDL_GL_extensions will dump debugging information to.
/// If not set, it will default to stdout.
FILE * SDL_GL_set_stdout( FILE * pfile );

/// Set the FILE that SDL_GL_extensions will dump debugging errors to.
/// If not set, it will default to stderr.
FILE * SDL_GL_set_stderr( FILE * pfile );


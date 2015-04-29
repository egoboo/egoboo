//********************************************************************************************
//*
//*    This file is part of the SDL extensions library. This library is
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

/// @file egolib/Extensions/SDL_GL_extensions.h
/// @ingroup _sdl_extensions_
/// @brief Definitions for OpenGL extensions to SDL
/// @details

#pragma once

#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  Convert an arbitrary surface into a surface suited for OpenGL.
 * @param surface
 *  the surface
 * @return
 *  the surface suited for OpenGL
 * @throw std::runtime_error
 *  if conversion fails
 * @throw std::invalid_argument
 *  if @a surface is @a nullptr
 */
SDL_Surface *SDL_GL_convert_surface(SDL_Surface *surface);

/// Set the OpenGL screen mode using SDL
SDLX_video_parameters_t * SDL_GL_set_mode(SDLX_video_parameters_t *v_old, SDLX_video_parameters_t *v_new, oglx_video_parameters_t *gl_new, SDL_bool has_valid_mode);

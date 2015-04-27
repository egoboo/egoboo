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

namespace Ego
{
    enum class PixelFormat
    {
        R8G8B8,
        R8G8B8A8,
#if 0
        B8G8R8A8,
        A8R8G8B8,
        A8B8G8R8,
#endif
    };
}

/**
 * @brief
 *  A very simple pixel descriptor suitable to describe the pixel formats
 *  R8G8B8(A8),B8G8R8(A8) (A8)R8G8B8 and (A8)B8G8R8. The pixel format used
 *  by Egoboo to upload textures is R8G8B8A. A specialization of this template
 *  for Ego::PixelFormat::R8G8B8A is provided.
 * @todo
 *  Provide specializations for the other formats.
 * @author
 *  Michael Heilmann
 */
template <Ego::PixelFormat _PixelFormat>
struct PixelDescriptor;

template <>
struct PixelDescriptor <Ego::PixelFormat::R8G8B8A8>
{
    /**
     * @brief
     *  Get the used Bits per pixel.
     * @return
     *  the used Bits per pixel
     */
    static uint8_t getBitsPerPixel()
    {
        return 32;
    }

    /**
     * @brief
     *  Get the shift of the alpha Bits.
     * @return
     *  the shift of the alpha Bits
     */
    static uint32_t a_shift()
    {
        return
    #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            24;
    #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            0;
    #endif
    }

    /**
     * @brief
     *  The shift of the blue Bits.
     * @return
     *  the shift of the blue Bits
     */
    static uint32_t b_shift()
    {
        return
    #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            16;
    #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            8;
    #endif
    }
        
    /**
     * @brief
     *  The shift of the green Bits.
     * @return
     *  the shift of the green Bits
     */
    static uint32_t g_shift()
    {
        return
    #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            8;
    #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            16;
    #endif
    }
        
    /**
     * @brief
     *  The shift of the red Bits.
     * @return
     *  the shift of the red Bits
     */
    static uint32_t r_shift()
    {
        return
    #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            0;
    #else if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            24;
    #endif
    }

    /**
     * @brief
     *  The mask for the alpha Bits.
     * @return
     *  the mask of the alpha Bits
     */
    static uint32_t a_mask()
    {
        return ((uint32_t)0xff) << a_shift();
    }

    /**
     * @brief
     *  The mask for the blue Bits.
     * @return
     *  the mask of the blue Bits
     */
    static uint32_t b_mask()
    {
        return ((uint32_t)0xff) << b_shift();
    }

    /**
     * @brief
     *  The mask for the green Bits.
     * @return
     *  the mask of the green Bits
     */
    static uint32_t g_mask()
    {
        return ((uint32_t)0xff) << g_shift();
    }

    /**
     * @brief
     *  The mask for the red Bits.
     * @return
     *  the mask of the red Bits
     */
    static uint32_t r_mask()
    {
        return ((uint32_t)0xff) << r_shift();

    }

};

template <>
struct PixelDescriptor <Ego::PixelFormat::R8G8B8>
{
    /**
     * @brief
     *  Get the used Bits per pixel.
     * @return
     *  the used Bits per pixel
     */
    static uint8_t bpp()
    {
        return 24;
    }

    /**
     * @brief
     *  Get the shift of the alpha Bits.
     * @return
     *  the shift of the alpha Bits
     */
    static uint32_t a_shift()
    {
        return
    #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            24;
    #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            0;
    #endif
    }

    /**
     * @brief
     *  The shift of the blue Bits.
     * @return
     *  the shift of the blue Bits
     */
    static uint32_t b_shift()
    {
        return
    #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            16;
    #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            8;
    #endif
    }

    /**
     * @brief
     *  The shift of the green Bits.
     * @return
     *  the shift of the green Bits
     */
    static uint32_t g_shift()
    {
        return
    #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            8;
    #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            16;
    #endif
    }

    /**
     * @brief
     *  The shift of the red Bits.
     * @return
     *  the shift of the red Bits
     */
    static uint32_t r_shift()
    {
        return
    #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            0;
    #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            24;
    #endif
    }

    /**
     * @brief
     *  The mask for the alpha Bits.
     * @return
     *  the mask of the alpha Bits
     */
    static uint32_t a_mask()
    {
        return ((uint32_t)0x00) << a_shift();
    }

    /**
     * @brief
     *  The mask for the blue Bits.
     * @return
     *  the mask of the blue Bits
     */
    static uint32_t b_mask()
    {
        return ((uint32_t)0xff) << b_shift();
    }

    /**
     * @brief
     *  The mask for the green Bits.
     * @return
     *  the mask of the green Bits
     */
    static uint32_t g_mask()
    {
        return ((uint32_t)0xff) << g_shift();
    }

    /**
     * @brief
     *  The mask for the red Bits.
     * @return
     *  the mask of the red Bits
     */
    static uint32_t r_mask()
    {
        return ((uint32_t)0xff) << r_shift();

    }

};

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

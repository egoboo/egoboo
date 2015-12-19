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



namespace Ego {
namespace Graphics {
namespace SDL {

/**
 * @brief Specifies a padding.
 */
struct Padding {
    size_t left, top, right, bottom;
};

/**
 * @brief Get the enumerated SDL pixel format for a specified pixel format descriptor.
 * @param pixelFormatDescriptor the pixel format descriptor
 * @return the enumerated SDL pixel format
 * @throw Id::RuntimeErrorException if the pixel format descriptor has no corresponding enumerated SDL pixel format
 */
uint32_t getEnumeratedPixelFormat(const Ego::PixelFormatDescriptor& pixelFormatDescriptor);

/**
 * @brief Get the SDL pixel format for a specified pixel format descriptor.
 * @param pixelFormatDescriptor the pixel format descriptor
 * @return the enumerated SDL pixel format
 * @throw Id::RuntimeErrorException if the pixel format descriptor has no corresponding enumerated SDL pixel format
 * @throw Id::EnvironmentErrorException if SDL does not behave according to its specification
 */
std::shared_ptr<const SDL_PixelFormat> getPixelFormat(const Ego::PixelFormatDescriptor& pixelFormatDescriptor);

/**
 * @brief Create an SDL surface of the specified size and pixel format.
 * @param width, height the width and the height of the surface
 * @param pixelFormatDescriptor the pixel format descriptor of the surface
 * @return a pointer to the SDL surface
 * @throw std::runtime_error if @a width or @a height is negative
 * @throw std::runtime_error if SDL fails
 */
std::shared_ptr<SDL_Surface> createSurface(int width, int height, const Ego::PixelFormatDescriptor& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>());

/**
 * @brief Create a padded surface.
 * @param surface the original surface
 * @param padding the padding
 * @return the padded surface
 * @remark The padding is black (if no alpha channel is present) or transparent black (if alpha channel is present).
 */
std::shared_ptr<SDL_Surface> padSurface(const std::shared_ptr<const SDL_Surface>& surface, const Padding& padding);

/**
 * @brief Clone a surface.
 * @param surface the original surface
 * @return the cloned surface
 */
std::shared_ptr<SDL_Surface> cloneSurface(const std::shared_ptr<const SDL_Surface>& surface);

/**
 * @brief Get a pixel in a surface.
 * @param the surface
 * @param x, y the position of the pixel
 * @return the pixel
 */
uint32_t getPixel(const std::shared_ptr<const SDL_Surface>& surface, int x, int y);

/**
 * @brief Set a pixel in a surface
 * @param the surface
 * @param x, y the position of the pixel
 * @param pixel the pixel
 */
void putPixel(const std::shared_ptr<SDL_Surface>& surface, int x, int y, uint32_t pixel);

} // namespace SDL
} // namespace Graphics
} // namespace Ego

/**
 * @brief
 *  Create a surface that is identicial to the original surface
 *  but is of the specified pixel format.
 * @param surface
 *  the surface
 * @param pixelFormatDescriptor
 *  the pixel format descriptor of the pixel format
 * @return
 *  the resulting surface
 * @throw std::runtime_error
 *  if the convesion fails
 * @throw std::invalid_argument
 *  if @a a surface is @a nullptr
 */
std::shared_ptr<SDL_Surface> SDL_GL_convert(std::shared_ptr<SDL_Surface> surface, const Ego::PixelFormatDescriptor& pixelFormatDescriptor);

/**
 * @brief
 *  Get the corresponding pixel format of an SDL pixel format.
 * @param source
 *  the SDL pixel format
 * @return
 *  the pixel format
 * @throw std::runtime_error
 *  if there is no pixel format for the given SDL pixel format
 */
Ego::PixelFormatDescriptor SDL_GL_fromSDL(const SDL_PixelFormat& source);

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
std::shared_ptr<SDL_Surface> SDL_GL_convert(std::shared_ptr<SDL_Surface> surface);

/// Set the OpenGL screen mode using SDL
SDLX_video_parameters_t * SDL_GL_set_mode(SDLX_video_parameters_t *v_old, SDLX_video_parameters_t *v_new, oglx_video_parameters_t *gl_new, bool has_valid_mode);

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

template <typename T> using SharedPtr = std::shared_ptr<T>;
using RuntimeErrorException = Id::RuntimeErrorException;
using EnvironmentErrorException = Id::EnvironmentErrorException;
using InvalidArgumentException = Id::InvalidArgumentException;
using UnhandledSwitchCaseException = Id::UnhandledSwitchCaseException;

/**
 * @brief Specifies a padding.
 */
struct Padding {
    size_t left, top, right, bottom;
};

/**
 * @brief Get the corresponding pixel format of an SDL pixel format.
 * @param source the SDL pixel format
 * @return the pixel format
 * @throw RuntimeErrorException if there is no pixel format for the given SDL pixel format
 */
PixelFormatDescriptor getPixelFormat(const SDL_PixelFormat& source);

/**
 * @brief Get the enumerated SDL pixel format for a specified pixel format descriptor.
 * @param pixelFormatDescriptor the pixel format descriptor
 * @return the enumerated SDL pixel format
 * @throw RuntimeErrorException if the pixel format descriptor has no corresponding enumerated SDL pixel format
 */
uint32_t getEnumeratedPixelFormat(const PixelFormatDescriptor& pixelFormatDescriptor);

/**
 * @brief Get the SDL pixel format for a specified pixel format descriptor.
 * @param pixelFormatDescriptor the pixel format descriptor
 * @return the enumerated SDL pixel format
 * @throw RuntimeErrorException if the pixel format descriptor has no corresponding enumerated SDL pixel format
 * @throw EnvironmentErrorException if SDL does not behave according to its specification
 */
SharedPtr<const SDL_PixelFormat> getPixelFormat(const PixelFormatDescriptor& pixelFormatDescriptor);

/**
 * @brief Create an SDL surface of the specified size and pixel format.
 * @param width, height the width and the height of the surface
 * @param pixelFormatDescriptor the pixel format descriptor of the surface
 * @return a pointer to the SDL surface
 * @throw InvalidArgumentException if @a width or @a height is negative
 * @throw RuntimeErrorException if SDL fails
 */
SharedPtr<SDL_Surface> createSurface(int width, int height, const PixelFormatDescriptor& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>());

/**
 * @brief Create a padded surface.
 * @param surface the original surface
 * @param padding the padding
 * @return the padded surface
 * @remark The padding is black (if no alpha channel is present) or transparent black (if alpha channel is present).
 */
SharedPtr<SDL_Surface> padSurface(const SharedPtr<const SDL_Surface>& surface, const Padding& padding);

/**
 * @brief Clone a surface.
 * @param surface the original surface
 * @return the cloned surface
 */
SharedPtr<SDL_Surface> cloneSurface(const SharedPtr<const SDL_Surface>& surface);

/**
 * @brief Get a pixel in a surface.
 * @param the surface
 * @param x, y the position of the pixel
 * @return the pixel
 */
uint32_t getPixel(const SharedPtr<const SDL_Surface>& surface, int x, int y);

/**
 * @brief Set a pixel in a surface
 * @param the surface
 * @param x, y the position of the pixel
 * @param pixel the pixel
 */
void putPixel(const SharedPtr<SDL_Surface>& surface, int x, int y, uint32_t pixel);

/**
 * @brief Create a surface that is identicial to the original surface but is of the specified pixel format.
 * @param surface the surface
 * @param pixelFormatDescriptor the pixel format descriptor of the pixel format
 * @return the resulting surface
 * @throw RuntimeErrorException if the convesion fails
 * @throw InvalidArgumentException if @a a surface is @a nullptr
 */
SharedPtr<SDL_Surface> convertPixelFormat(const SharedPtr<SDL_Surface>& surface, const PixelFormatDescriptor& pixelFormatDescriptor);

SharedPtr<SDL_Surface> convertPowerOfTwo(const SharedPtr<SDL_Surface>& surface);

/**
 * @brief Test if a surface is non-opaque.
 * @param surface the surface
 * @return @a true if the surface is non-opaque, @a false otherwise
 * @remark A surface is non-opaque if any of the following properties hold:
 * - the surface has a color key
 *   TODO: Test if some pixel has the color of the color key
 * - the surface has a global, non-opaque alpha value
 * - the surface is palettized and there is a non-opaque palette entry
 *   TODO: Test if some pixel has the color of the palette entry
 * - the surface is not palettized, has an alpha channel, and there is a non-opaque pixel
 */
bool testAlpha(const SharedPtr<SDL_Surface>& surface);

} // namespace SDL
} // namespace Graphics
} // namespace Ego

/// Set the OpenGL screen mode using SDL
bool SDL_GL_set_mode(SDLX_video_parameters_t& v_new, oglx_video_parameters_t& gl_new);

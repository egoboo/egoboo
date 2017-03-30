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

/// @file egolib/Image/SDL_Image_Extensions.h
/// @brief Extensions to SDL image.

#pragma once

#include "egolib/Graphics/PixelFormat.hpp"
#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace SDL {

/// @brief Specifies a padding.
struct Padding
{
    size_t left, top, right, bottom;
};

/// @brief Get the corresponding pixel format of an SDL pixel format.
/// @param source the SDL pixel format
/// @return the pixel format
/// @throw RuntimeErrorException if there is no pixel format for the given SDL pixel format
PixelFormatDescriptor getPixelFormat(const SDL_PixelFormat& source);

/// @brief Get the enumerated SDL pixel format for a specified pixel format descriptor.
/// @param pixelFormatDescriptor the pixel format descriptor
/// @return the enumerated SDL pixel format
/// @throw RuntimeErrorException if the pixel format descriptor has no corresponding enumerated SDL pixel format
uint32_t getEnumeratedPixelFormat(const PixelFormatDescriptor& pixelFormatDescriptor);

/// @brief Get the SDL pixel format for a specified pixel format descriptor.
/// @param pixelFormatDescriptor the pixel format descriptor
/// @return the enumerated SDL pixel format
/// @throw RuntimeErrorException if the pixel format descriptor has no corresponding enumerated SDL pixel format
/// @throw EnvironmentErrorException if SDL does not behave according to its specification
std::shared_ptr<const SDL_PixelFormat> getPixelFormat(const PixelFormatDescriptor& pixelFormatDescriptor);

/// @brief Create an SDL surface of the specified size and pixel format.
/// @param width, height the width and the height of the surface
/// @param pixelFormatDescriptor the pixel format descriptor of the surface
/// @return a pointer to the SDL surface
/// @throw id::invalid_argument_error if @a width or @a height is negative
/// @throw id::runtime_error if SDL fails
std::shared_ptr<SDL_Surface> createSurface(int width, int height, const PixelFormatDescriptor& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>());

/// @brief Create a padded surface.
/// @param surface the original surface
/// @param padding the padding
/// @return the padded surface
/// @remark The padding is black (if no alpha channel is present) or transparent black (if alpha channel is present).
std::shared_ptr<SDL_Surface> padSurface(const std::shared_ptr<const SDL_Surface>& surface, const Padding& padding);

/// @brief Clone a surface.
/// @param surface the original surface
/// @return the cloned surface
std::shared_ptr<SDL_Surface> cloneSurface(const std::shared_ptr<const SDL_Surface>& surface);

/// @brief Get a pixel in a surface.
/// @param the surface
/// @param x, y the position of the pixel
/// @return the pixel
uint32_t getPixel(const std::shared_ptr<const SDL_Surface>& surface, int x, int y);

/// @brief Set a pixel in a surface
/// @param the surface
/// @param x, y the position of the pixel
/// @param pixel the pixel
void putPixel(const std::shared_ptr<SDL_Surface>& surface, int x, int y, uint32_t pixel);

/// @brief Create a surface that is identicial to the original surface but is of the specified pixel format.
/// @param surface the surface
/// @param pixelFormatDescriptor the pixel format descriptor of the pixel format
/// @return the resulting surface
/// @throw id::runtime_error if the convesion fails
/// @throw id::invalid_argument_error if @a a surface is @a nullptr
std::shared_ptr<SDL_Surface> convertPixelFormat(const std::shared_ptr<SDL_Surface>& surface, const PixelFormatDescriptor& pixelFormatDescriptor);

std::shared_ptr<SDL_Surface> convertPowerOfTwo(const std::shared_ptr<SDL_Surface>& surface);

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
bool testAlpha(const std::shared_ptr<SDL_Surface>& surface);

Math::Colour3b getColourMod(SDL_Surface& surface);

void setColourMod(SDL_Surface& surface, const Math::Colour3b& colourMod);

enum class BlendMode
{
    /// "source color"
    /// \f$(r_d,g_d,b_d,a_d) = (r_s, g_s, b_s, a_s)\f$
    NoBlending,
    /// "alpha blending"
    /// \f$(r_d, g_d, b_d) = (r_s a_s, g_s a_s, b_s a_s)\f$
    /// \f$a_d =  a_s + (a_d * (1-a_s))\f$
    AlphaBlending,
    /// "additive blending"
    /// \f$(r_d, g_d, b_d) = (r_s a_s, g_s a_s, b_s a_s) + (r_d, g_d, b_d)\f$
    /// \f$a_d = a_d\f$
    AdditiveBlending,
    /// "modulative blending"
    /// \f$(r_d, g_d, b_d) = (r_d * r_s, g_d * g_s, b_d * b_s)\f$
    /// \f$a_d = a_d\f$
    ModulativeBlending,
};

BlendMode blendModeToInternal(SDL_BlendMode blendMode);

SDL_BlendMode blendModeToExternal(BlendMode blendMode);

BlendMode getBlendMode(SDL_Surface& surface);

void setBlendMode(SDL_Surface& surface, BlendMode blendMode);

void fillSurface(SDL_Surface& surface, const Ego::Math::Colour4b& colour);

PixelFormat getPixelFormat(SDL_Surface& surface);

} // namespace SDL
} // namespace Ego

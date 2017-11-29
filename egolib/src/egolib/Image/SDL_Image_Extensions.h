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
#include "egolib/Image/blit.hpp"
#include "egolib/Image/convert.hpp"
#include "egolib/Image/fill.hpp"
#include "egolib/Image/get_pixel.hpp"
#include "egolib/Image/pad.hpp"
#include "egolib/Image/power_of_two.hpp"
#include "egolib/Image/set_pixel.hpp"

namespace Ego { namespace SDL {

/// @brief Get the enumerated SDL pixel format for a specified pixel format descriptor.
/// @param pixelFormatDescriptor the pixel format descriptor
/// @return the enumerated SDL pixel format
/// @throw RuntimeErrorException if the pixel format descriptor has no corresponding enumerated SDL pixel format
uint32_t getEnumeratedPixelFormat(const pixel_descriptor& pixel_descriptor);

/// @brief Get the SDL pixel format for a specified pixel format descriptor.
/// @param pixelFormatDescriptor the pixel format descriptor
/// @return the enumerated SDL pixel format
/// @throw RuntimeErrorException if the pixel format descriptor has no corresponding enumerated SDL pixel format
/// @throw EnvironmentErrorException if SDL does not behave according to its specification
std::shared_ptr<const SDL_PixelFormat> getPixelFormat(const pixel_descriptor& pixel_descriptor);

/// @brief Clone a surface.
/// @param surface the original surface
/// @return the cloned surface
std::shared_ptr<SDL_Surface> cloneSurface(const std::shared_ptr<const SDL_Surface>& surface);

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
bool testAlpha(SDL_Surface *surface);

Math::Colour3b getColourMod(SDL_Surface *surface);

void setColourMod(SDL_Surface *surface, const Math::Colour3b& colourMod);

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

BlendMode getBlendMode(SDL_Surface *surface);

void setBlendMode(SDL_Surface *surface, BlendMode blendMode);

id::pixel_format getPixelFormat(SDL_Surface *surface);

uint32_t make_rgb(SDL_Surface *surface, const Math::Colour3b& colour);

uint32_t make_rgba(SDL_Surface *surface, const Math::Colour4b& colour);

std::shared_ptr<SDL_Surface> render_glyph(TTF_Font *sdl_font, uint16_t code_point, const Math::Colour4b& color);

} } // namespace Ego::SDL

namespace Ego {

template <>
struct convert_functor<SDL_Surface>
{
    std::shared_ptr<SDL_Surface> operator()(const std::shared_ptr<SDL_Surface>& pixels, const pixel_descriptor& format) const;
};

template <>
struct power_of_two_functor<SDL_Surface>
{
    std::shared_ptr<SDL_Surface> operator()(const std::shared_ptr<SDL_Surface>& pixels) const;
};

template <>
struct pad_functor<SDL_Surface>
{
    std::shared_ptr<SDL_Surface> operator()(const std::shared_ptr<SDL_Surface>& pixels, const padding& padding) const;
};

template <>
struct blit_functor<SDL_Surface>
{
    void operator()(SDL_Surface *source, SDL_Surface *target) const;
    void operator()(SDL_Surface *source, const Rectangle2f& source_rectangle, SDL_Surface *target) const;
    void operator()(SDL_Surface *source, SDL_Surface *target, const Point2f& target_position) const;
    void operator()(SDL_Surface *source, const Rectangle2f& source_rectangle, SDL_Surface *target, const Point2f& target_position) const;
};

template <>
struct fill_functor<SDL_Surface>
{
    /// @{
    /// @brief Fill an SDL surface with the specified color.
    /// @param surface a pointer to the SDL surface
    /// @param color the fill color
    void operator()(SDL_Surface *surface, const Math::Colour3b& color) const;
    void operator()(SDL_Surface *surface, const Math::Colour4b& color) const;
    /// @}

    /// @{
    /// @brief Fill a rectangle of an SDL surface with the specified color.
    /// @param surface a pointer to the SDL surface
    /// @param rectangle the rectangle of the SDL surface to fill. Clipped against the rectangle of the image.
    /// @param color the fill color
    void operator()(SDL_Surface *surface, const Math::Colour3b& color, const Rectangle2f& rectangle) const;
    void operator()(SDL_Surface *surface, const Math::Colour4b& color, const Rectangle2f& rectangle) const;
    /// @}
};

template <>
struct get_pixel_functor<SDL_Surface>
{
    /// @{
    /// @brief Get the color of a pixel of an SDL surface.
    /// @param surface a pointer to the SDL surface
    /// @param position the position of the pixel to fill
    /// @return the color of the pixel of the SDL surface
    Math::Colour4b operator()(const SDL_Surface *surface, const Point2f& point) const;
    /// @}
};

template <>
struct set_pixel_functor<SDL_Surface>
{
    /// @{
    /// @brief Fill a pixel of an SDL surface with the specified colour.
    /// @param surface a pointer to the SDL surface
    /// @param position the position of the pixel to fill. Clipped against the rectangle of the SDL surface.
    /// @param color the fill color
    void operator()(SDL_Surface *surface, const Math::Colour3b& color, const Point2f& point) const;
    void operator()(SDL_Surface *surface, const Math::Colour4b& color, const Point2f& point) const;
    /// @}

private:
    void operator()(SDL_Surface *surface, uint32_t coded_color, const Point2f& point) const;
};

} // namespace Ego

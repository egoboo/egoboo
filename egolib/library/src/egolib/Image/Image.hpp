//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
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

#pragma once

#include "egolib/Graphics/PixelFormat.hpp"
#include "egolib/Math/_Include.hpp"
#include "egolib/integrations/color.hpp"
#include "egolib/Image/convert.hpp"
#include "idlib/game_engine.hpp"

#if !SDL_VERSION_ATLEAST(2, 0, 0)
int SDL_GetColorKey(SDL_Surface *surface, uint32_t *key);
#endif

namespace Ego {

class Image
{
protected:
    /// @brief The backing SDL surface.
    SDL_Surface *m_surface;

    /// @brief The pixel format.
    idlib::pixel_format m_pixel_format;

public:
    /// @brief Construct this image.
    /// @param surface the backing SDL surface. The image takes ownership.
    Image(SDL_Surface *surface);

    /// @brief Destruct this image.
    virtual ~Image();

    /// @brief Get a pointer to the pixels of this image.
    /// @return a pointer to the pixels of this image
    void *getPixels();

    /// @brief Get a pointer to the backing image.
    /// @return a pointer to the backing image
    SDL_Surface *getSurface();
	const SDL_Surface *getSurface() const;

    /// @brief Get the pixel format of this image.
    /// @return the pixel format of this image
    idlib::pixel_format get_pixel_format() const;

    /// @brief Get the size, in Bytes, of a pixel.
    /// @return the size, in Bytes, of a pixel
    int getBytesPerPixel() const;

    /// @brief Get the width, in pixels, of this image.
    /// @return the width
    /// @remark The width is always non-negative.
    int getWidth() const;

    /// @brief Get the height, in pixels, of this image.
    /// @return the height
    /// @remark The height is always non-negative.
    int getHeight() const;

    /// @brief Get the pitch, in pixels, of this image.
    /// @return the pitch
    /// @remark The pitch is always non-negative.
    int getPitch() const;

    /// @brief Get if this image has an alpha component.
    /// @return @a true if this image has an alpha component, @a false otherwise
    bool hasAlpha() const;

	/// @brief Clone this image.
	/// @return the clone of the image
	std::shared_ptr<Image> clone() const;

}; // class Image

template <>
struct convert_functor<Image>
{
    std::shared_ptr<Image> operator()(const std::shared_ptr<Image>& image, const pixel_descriptor& format) const;
};

} // namespace Ego

namespace idlib {

template <>
struct pad_functor<Ego::Image>
{
  std::shared_ptr<Ego::Image> operator()(const std::shared_ptr<Ego::Image>& image, const padding& padding) const;
};

template <>
struct power_of_two_functor<Ego::Image>
{
  std::shared_ptr<Ego::Image> operator()(const std::shared_ptr<Ego::Image>& image) const;
};

template <>
struct get_pixel_functor<Ego::Image>
{
  Ego::Colour4b operator()(const Ego::Image* image, const Ego::Point2f& point) const;
};

template <>
struct blit_functor<Ego::Image>
{
    void operator()(Ego::Image *source, Ego::Image *target) const;
    void operator()(Ego::Image *source, const Ego::Rectangle2f& source_rectangle, Ego::Image *target) const;
    void operator()(Ego::Image *source, Ego::Image *target, const Ego::Point2f& target_position) const;
    void operator()(Ego::Image *source, const Ego::Rectangle2f& source_rectangle, Ego::Image *target, const Ego::Point2f& target_position) const;
};

template <>
struct fill_functor<Ego::Image>
{
    void operator()(Ego::Image *image, const Ego::Colour3b& color) const;
    void operator()(Ego::Image *image, const Ego::Colour4b& color) const;

    void operator()(Ego::Image *image, const Ego::Colour3b& color, const Ego::Rectangle2f& rectangle) const;
    void operator()(Ego::Image *image, const Ego::Colour4b& color, const Ego::Rectangle2f& rectangle) const;
};

template <>
struct set_pixel_functor<Ego::Image>
{
    /// @{
    /// @brief Fill a pixel of an image with the specified colour.
    /// @param image a pointer to the image
    /// @param position the position of the pixel to fill. Clipped against the rectangle of the image.
    /// @param color the fill color
    void operator()(Ego::Image *image, const Ego::Colour3b& color, const Ego::Point2f& point) const;
    void operator()(Ego::Image *image, const Ego::Colour4b& color, const Ego::Point2f& point) const;
    /// @}
};

} // namespace idlib

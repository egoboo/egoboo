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
#include <memory>
#include "egolib/integrations/color.hpp"
#include "egolib/integrations/video.hpp"
#include "egolib/Image/convert.hpp"
#include "egolib/Image/pad.hpp"
#include "egolib/Image/power_of_two.hpp"
#include <SDL.h>

#if !SDL_VERSION_ATLEAST(2, 0, 0)
int SDL_GetColorKey(SDL_Surface *surface, uint32_t *key);
#endif

namespace Ego {

class Image : public idlib::image
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

    /// @copydoc idlib::image::get_pixels
    void *get_pixels() override;

    /// @brief Get a pointer to the backing image.
    /// @return a pointer to the backing image
    SDL_Surface *getSurface();
    const SDL_Surface *getSurface() const;

    /// @copydoc idlib::image::get_pixel_format
    idlib::pixel_format get_pixel_format() const override;

    /// @copydoc idlib::image::get_bytes_per_pixel
    int get_bytes_per_pixel() const override;

    /// @copydoc idlib::image::get_width
    int get_width() const override;

    /// @copydoc idlib::image::get_height
    int get_height() const override;

    /// @copydoc idlib::image::get_pitch
    int get_pitch() const override;

    /// @copydoc idlib::image::has_alpha
    bool has_alpha() const override;

    /// @brief Clone this image.
    /// @return the clone of the image
    std::shared_ptr<Image> clone() const;

}; // class Image

template <>
struct convert_functor<Image>
{
    std::shared_ptr<Image> operator()(const std::shared_ptr<Image>& image, const pixel_descriptor& format) const;
};

template <>
struct pad_functor<Image>
{
    std::shared_ptr<Image> operator()(const std::shared_ptr<Image>& image, const padding& padding) const;
};

template <>
struct power_of_two_functor<Image>
{
    std::shared_ptr<Image> operator()(const std::shared_ptr<Image>& image) const;
};

} // namespace Ego

namespace idlib {

template <>
struct get_pixel_functor<Ego::Image>
{
    color_4b operator()(const Ego::Image *pixels, const point_2s& point) const;
};

template <>
struct blit_functor<Ego::Image>
{
    void operator()(Ego::Image *source_pixels, Ego::Image *target_pixels) const;
    void operator()(Ego::Image *source_pixels, const rectangle_2s& source_rectangle, Ego::Image *target_pixels) const;
    void operator()(Ego::Image *source_pixels, Ego::Image *target_pixels, const point_2s& target_point) const;
    void operator()(Ego::Image *source_pixels, const rectangle_2s& source_rectangle, Ego::Image *target_pixels, const point_2s& target_point) const;
};

template <>
struct fill_functor<Ego::Image>
{
    void operator()(Ego::Image *pixels, const color_3b& color) const;
    void operator()(Ego::Image *pixels, const color_4b& color) const;

    void operator()(Ego::Image *pixels, const color_3b& color, const rectangle_2s& rectangle) const;
    void operator()(Ego::Image *pixels, const color_4b& color, const rectangle_2s& rectangle) const;
};

template <>
struct set_pixel_functor<Ego::Image>
{

    void operator()(Ego::Image *pixels, const color_3b& color, const point_2s& point) const;
    void operator()(Ego::Image *pixels, const color_4b& color, const point_2s& point) const;
};

} // namespace idlib

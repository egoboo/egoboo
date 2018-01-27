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

#include "egolib/Image/Image.hpp"
#include "egolib/Image/SDL_Image_Extensions.h"

#if !SDL_VERSION_ATLEAST(2, 0, 0)
int SDL_GetColorKey(SDL_Surface *surface, uint32_t *key)
{
    if (!surface || !key)
    {
        return -2;
    }
    if (surface->flags & SDL_SRCCOLORKEY)
    {
        *key = surface->format->colorkey;
        return 0;
    }
    return -1;
}
#endif

namespace Ego {

Image::Image(SDL_Surface *surface) :
    m_surface(surface),
    m_pixel_format(SDL::getPixelFormat(surface))
{}

Image::~Image()
{
    SDL_FreeSurface(m_surface);
    m_surface = nullptr;
}

idlib::pixel_format Image::get_pixel_format() const
{
    return m_pixel_format;
}

int Image::getBytesPerPixel() const
{
    return m_surface->format->BytesPerPixel;
}

int Image::getWidth() const
{
    assert(m_surface->w > 0);
    return m_surface->w;
}

int Image::getHeight() const
{
    assert(m_surface->h > 0);
    return m_surface->h;
}

int Image::getPitch() const
{
    return m_surface->pitch;
}

bool Image::hasAlpha() const
{
    return SDL::testAlpha(m_surface);
}

SDL_Surface *Image::getSurface()
{
	return m_surface;
}

const SDL_Surface *Image::getSurface() const
{
	return m_surface;
}

std::shared_ptr<Image> Image::clone() const
{
    static_assert(SDL_VERSION_ATLEAST(2, 0, 0), "SDL 2.x required");
    auto cloned_surface = SDL_ConvertSurface(m_surface, m_surface->format, 0);
    if (!cloned_surface)
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "SDL_ConvertSurface failed");
    }
    std::shared_ptr<Image> cloned_image = nullptr;
    try
    {
        cloned_image = std::make_shared<Image>(cloned_surface);
    }
    catch (...)
    {
        SDL_FreeSurface(cloned_surface);
        throw std::current_exception();
    }
    return cloned_image;
}

std::shared_ptr<Image> convert_functor<Image>::operator()(const std::shared_ptr<Image>& image, const pixel_descriptor& format) const
{ 
    uint32_t alphaMask = format.get_alpha().get_mask(),
             blueMask = format.get_blue().get_mask(),
             greenMask = format.get_green().get_mask(),
             redMask = format.get_red().get_mask();
    int bpp = format.get_color_depth().depth();

    uint32_t newFormat = SDL_MasksToPixelFormatEnum(bpp, redMask, greenMask, blueMask, alphaMask);
    if (newFormat == SDL_PIXELFORMAT_UNKNOWN)
    {
        throw idlib::invalid_argument_error(__FILE__, __LINE__, "pixelFormatDescriptor doesn't correspond with a SDL_PixelFormat");
    }
    SDL_Surface *newSurface = SDL_ConvertSurfaceFormat(image->getSurface(), newFormat, 0);
    if (!newSurface)
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "unable to convert surface");
    }

    try
    {
        return std::make_shared<Image>(newSurface);
    }
    catch (...)
    {
        SDL_FreeSurface(newSurface);
        throw std::current_exception();
    }
}

std::shared_ptr<Image> power_of_two_functor<Image>::operator()(const std::shared_ptr<Image>& image) const
{
    // Alias old width and old height.
    int oldWidth = image->getWidth(),
        oldHeight = image->getHeight();

    // Compute new width and new height.
    int newWidth = Math::powerOfTwo(oldWidth),
        newHeight = Math::powerOfTwo(oldHeight);

    // Only if the new dimension differ from the old dimensions, perform the scaling.
    if (newWidth != oldWidth || newHeight != oldHeight)
    {
        padding padding;
        padding.left = 0;
        padding.top = 0;
        padding.right = newWidth - oldWidth;
        padding.bottom = newHeight - oldHeight;
        return pad(image, padding);
    }
    else
    {
        return image->clone();
    }
}

std::shared_ptr<Image> pad_functor<Image>::operator()(const std::shared_ptr<Image>& image, const padding& padding) const
{
    // Alias old surface.
    auto *oldSurface = image->getSurface();

    // Alias old width and old height.
    auto oldWidth = image->getWidth(),
        oldHeight = image->getHeight();

    // Compute new width and new height.
    auto newWidth = oldWidth + padding.left + padding.right,
        newHeight = oldHeight + padding.top + padding.bottom;

    // Create the copy.
    auto *newSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                            newWidth, newHeight,
                                            oldSurface->format->BitsPerPixel,
                                            oldSurface->format->Rmask,
                                            oldSurface->format->Gmask,
                                            oldSurface->format->Bmask,
                                            oldSurface->format->Amask);
    if (!newSurface)
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "SDL_CreateRGBSurface failed");
    }
    // Fill the copy with transparent black.
    if (-1 == SDL_FillRect(newSurface, nullptr, SDL_MapRGBA(newSurface->format, 0, 0, 0, 0)))
    {
        SDL_FreeSurface(newSurface);
        throw idlib::runtime_error(__FILE__, __LINE__, "SDL_FillRect failed");
    }
    // Copy the old surface into the new surface.
    for (size_t y = 0; y < oldHeight; ++y)
    {
        for (size_t x = 0; x < oldWidth; ++x)
        {
            auto c = get_pixel(oldSurface, { x, y });
            set_pixel(newSurface, c, { padding.left + x, padding.top + y });
        }
    }
    try
    {
        return std::make_shared<Image>(newSurface);
    }
    catch (...)
    {
        SDL_FreeSurface(newSurface);
        throw std::current_exception();
    }
}

void blit_functor<Image>::operator()(Image *source, Image *target) const
{
    blit(source->getSurface(), target->getSurface());
}

void blit_functor<Image>::operator()(Image *source, const Rectangle2f& source_rectangle, Image *target) const
{
    blit(source->getSurface(), source_rectangle, target->getSurface());
}

void blit_functor<Image>::operator()(Image *source, Image *target, const Point2f& target_position) const
{
    blit(source->getSurface(), target->getSurface(), target_position);
}

void blit_functor<Image>::operator()(Image *source, const Rectangle2f& source_rectangle, Image *target, const Point2f& target_position) const
{
    blit(source->getSurface(), source_rectangle, target->getSurface(), target_position);
}

void fill_functor<Image>::operator()(Image *image, const Math::Colour3b& color) const
{
    if (!image) throw idlib::null_error(__FILE__, __LINE__, "image");
    fill(image->getSurface(), color);
}

void fill_functor<Image>::operator()(Image *image, const Math::Colour3b& color, const Rectangle2f& rectangle) const
{
    if (!image) throw idlib::null_error(__FILE__, __LINE__, "image");
    fill(image->getSurface(), color, rectangle);
}

void fill_functor<Image>::operator()(Image *image, const Math::Colour4b& color) const
{
    if (!image) throw idlib::null_error(__FILE__, __LINE__, "image");
    fill(image->getSurface(), color);
}

void fill_functor<Image>::operator()(Image *image, const Math::Colour4b& color, const Rectangle2f& rectangle) const
{
    if (!image) throw idlib::null_error(__FILE__, __LINE__, "image");
    fill(image->getSurface(), color, rectangle);
}

Math::Colour4b get_pixel_functor<Image>::operator()(const Image *image, const Point2f& point) const
{
    if (!image) throw idlib::null_error(__FILE__, __LINE__, "image");
    return get_pixel(image->getSurface(), point);
}

void set_pixel_functor<Image>::operator()(Image *image, const Math::Colour3b& color, const Point2f& point) const
{
    if (!image) throw idlib::null_error(__FILE__, __LINE__, "image");
    set_pixel(image->getSurface(), color, point);
}

void set_pixel_functor<Image>::operator()(Image *image, const Math::Colour4b& color, const Point2f& point) const
{
    if (!image) throw idlib::null_error(__FILE__, __LINE__, "image");
    set_pixel(image->getSurface(), color, point);
}

} // namespace Ego

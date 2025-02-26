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

/// @file egolib/Image/SDL_Image_Extensions.c
/// @brief Extensions to SDL image.

#include "egolib/Image/SDL_Image_Extensions.h"
#include "egolib/Image/ImageManager.hpp"

namespace Ego { namespace SDL {

uint32_t getEnumeratedPixelFormat(const pixel_descriptor& pixel_descriptor)
{
    uint32_t alphaMask = pixel_descriptor.get_alpha().get_mask(),
             blueMask = pixel_descriptor.get_blue().get_mask(),
             greenMask = pixel_descriptor.get_green().get_mask(),
             redMask = pixel_descriptor.get_red().get_mask();
    int bitsPerPixel = pixel_descriptor.get_color_depth().depth();

    uint32_t pixelFormatEnum_sdl = SDL_MasksToPixelFormatEnum(bitsPerPixel, redMask, greenMask, blueMask, alphaMask);
    if (SDL_PIXELFORMAT_UNKNOWN == pixelFormatEnum_sdl)
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "pixel format descriptor has no corresponding SDL pixel format");
    }
    return pixelFormatEnum_sdl;
}

std::shared_ptr<const SDL_PixelFormat> getPixelFormat(const pixel_descriptor& pixel_descriptor)
{
    std::shared_ptr<const SDL_PixelFormat> pixelFormat_sdl = std::shared_ptr<const SDL_PixelFormat>
        (
            SDL_AllocFormat(getEnumeratedPixelFormat(pixel_descriptor)),
            [](SDL_PixelFormat *pixelFormat) { if (pixelFormat) { SDL_FreeFormat(pixelFormat); } }
    );
    if (!pixelFormat_sdl)
    {
        throw idlib::environment_error(__FILE__, __LINE__, "SDL", "internal error");
    }
    return pixelFormat_sdl;
}

std::shared_ptr<SDL_Surface> cloneSurface(const std::shared_ptr<const SDL_Surface>& surface)
{
    static_assert(SDL_VERSION_ATLEAST(2, 0, 0), "SDL 2.x required");
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    // TODO: The signature SDL_ConvertSurface(SDL_Surface *, const SDL_PixelFormat *, uint32_t) might be considered as a bug.
    //       It should be SDL_ConvertSurface(const SDL_Surface *, const SDL_PixelFormat *, uint32_t).
    auto clone = std::shared_ptr<SDL_Surface>(SDL_ConvertSurface((SDL_Surface *)surface.get(), surface->format, 0), [](SDL_Surface *pSurface) { SDL_FreeSurface(pSurface); });
    if (!clone)
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "SDL_ConvertSurface failed");
    }
    return clone;
}

bool testAlpha(SDL_Surface *surface)
{
    // test to see whether an image requires alpha blending

    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }

    // Alias.
    SDL_PixelFormat *format = surface->format;

    // (1)
    // If the surface has a per-surface color key,
    // it is partially transparent.
    uint32_t colorKey;
    int rslt = SDL_GetColorKey(surface, &colorKey);
    if (rslt < -1)
    {
        // If a value smaller than -1 is returned, an error occured.
        throw std::invalid_argument("SDL_GetColorKey failed");
    }
    else if (rslt >= 0)
    {
        // If a value greater or equal than 0 is returned, the surface has a color key.
        return true;
    } /*else if (rslt == -1) {
      // If a value of -1 is returned, the surface has no color key: Continue.
    }*/

    // (2)
    // If the image is alpha modded with a non-opaque alpha value,
    // it is partially transparent.
    uint8_t alpha;
    SDL_GetSurfaceAlphaMod(surface, &alpha);
    if (0xff != alpha)
    {
        return true;
    }

    // (3)
    // If the image is palettized and has non-opaque colors in the color,
    // it is partially transparent.
    if (nullptr != format->palette)
    {
        for (int i = 0; i < format->palette->ncolors; ++i)
        {
            SDL_Color& color = format->palette->colors[i];
            if (0xff != color.a)
            {
                return true;
            }
        }
        return false;
    }

    // (The image is not palettized.)
    // (4)
    // If the image has no alpha channel,
    // then it is NOT partially transparent.
    if (0x00 == format->Amask)
    {
        return false;
    }

    // (The image is not palettized and has an alpha channel.)
    // If the image has an alpha channel and has non-opaque pixels,
    // then it is partially transparent.
    uint32_t bitMask = format->Rmask | format->Gmask | format->Bmask | format->Amask;
    int bytesPerPixel = format->BytesPerPixel;
    int width = surface->w;
    int height = surface->h;
    int pitch = surface->pitch;

    const char *row_ptr = static_cast<const char *>(surface->pixels);
    for (int y = 0; y < height; ++y)
    {
        const char *char_ptr = row_ptr;
        for (int x = 0; x < width; ++x)
        {
            const uint32_t *ui32_ptr = reinterpret_cast<const uint32_t*>(char_ptr);
            uint32_t pixel = (*ui32_ptr) & bitMask;
            uint8_t r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);

            if (0xFF != a)
            {
                return true;
            }
            char_ptr += bytesPerPixel;
        }
        row_ptr += pitch;
    }

    return false;
}

Colour3b getColourMod(SDL_Surface *surface)
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    uint8_t r, g, b;
    SDL_GetSurfaceColorMod(surface, &r, &g, &b);
    return Colour3b(r, g, b);
}

void setColourMod(SDL_Surface *surface, const Colour3b& colourMod)
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    SDL_SetSurfaceColorMod(surface, colourMod.get_r(), colourMod.get_g(), colourMod.get_b());
}

BlendMode blendModeToInternal(SDL_BlendMode blendMode)
{
    switch (blendMode)
    {
    case SDL_BLENDMODE_NONE:
        return BlendMode::NoBlending;
    case SDL_BLENDMODE_BLEND:
        return BlendMode::AlphaBlending;
    case SDL_BLENDMODE_ADD:
        return BlendMode::AdditiveBlending;
    case SDL_BLENDMODE_MOD:
        return BlendMode::ModulativeBlending;
    default:
        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}

SDL_BlendMode blendModeToExternal(BlendMode blendMode)
{
    switch (blendMode)
    {
    case BlendMode::NoBlending:
        return SDL_BLENDMODE_NONE;
    case BlendMode::AlphaBlending:
        return SDL_BLENDMODE_BLEND;
    case BlendMode::AdditiveBlending:
        return SDL_BLENDMODE_ADD;
    case BlendMode::ModulativeBlending:
        return SDL_BLENDMODE_MOD;
    default:
        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}

BlendMode getBlendMode(SDL_Surface *surface)
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    SDL_BlendMode blendMode;
    SDL_GetSurfaceBlendMode(surface, &blendMode);
    return blendModeToInternal(blendMode);
}

void setBlendMode(SDL_Surface *surface, BlendMode blendMode)
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    SDL_SetSurfaceBlendMode(surface, blendModeToExternal(blendMode));
}

idlib::pixel_format getPixelFormat(SDL_Surface *surface)
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    switch (surface->format->format)
    {
    case SDL_PIXELFORMAT_RGB888:
        return idlib::pixel_format::R8G8B8;
    case SDL_PIXELFORMAT_RGBA8888:
        return idlib::pixel_format::R8G8B8A8;
    case SDL_PIXELFORMAT_BGR888:
        return idlib::pixel_format::B8G8R8;
    case SDL_PIXELFORMAT_BGRA8888:
        return idlib::pixel_format::B8G8R8A8;
    case SDL_PIXELFORMAT_ABGR8888:
        return idlib::pixel_format::A8B8G8R8;
    case SDL_PIXELFORMAT_ARGB8888:
        return idlib::pixel_format::A8R8G8B8;
    case SDL_PIXELFORMAT_RGB24:
        if (idlib::get_byte_order() == idlib::byte_order::big_endian)
            return idlib::pixel_format::R8G8B8;
        else
            return idlib::pixel_format::B8G8R8;
    case SDL_PIXELFORMAT_BGR24:
        if (idlib::get_byte_order() == idlib::byte_order::big_endian)
            return idlib::pixel_format::B8G8R8;
        else
            return idlib::pixel_format::R8G8B8;
    default:
        throw idlib::runtime_error(__FILE__, __LINE__, "unsupported/unknown pixel format");
    };
}

uint32_t make_rgb(SDL_Surface *surface, const Colour3b& colour)
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    return SDL_MapRGB(surface->format, colour.get_r(), colour.get_g(), colour.get_b());
}

uint32_t make_rgba(SDL_Surface *surface, const Colour4b& colour)
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    return SDL_MapRGBA(surface->format, colour.get_r(), colour.get_g(), colour.get_b(), colour.get_a());
}

std::shared_ptr<SDL_Surface> render_glyph(TTF_Font *sdl_font, uint16_t code_point, const Colour4b& color)
{
    SDL_Color sdl_color = { color.get_red(), color.get_green(), color.get_blue(), color.get_alpha() };
    SDL_Surface *sdl_surface = nullptr;
    if (TTF_GlyphIsProvided(sdl_font, code_point))
    {
        sdl_surface = TTF_RenderGlyph_Blended(sdl_font, code_point, sdl_color);
    }
    // Note: According to C++ documentation, the deleter is invoked if the std::shared_ptr constructor
    //       raises an exception.
    return std::shared_ptr<SDL_Surface>(sdl_surface, [](SDL_Surface *sdl_surface) { SDL_FreeSurface(sdl_surface); });
}

} } // namespace Ego::SDL

namespace Ego {

std::shared_ptr<SDL_Surface> convert_functor<SDL_Surface>::operator()(const std::shared_ptr<SDL_Surface>& pixels, const pixel_descriptor& pixel_descriptor) const
{
    if (!pixels)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }

    uint32_t alphaMask = pixel_descriptor.get_alpha().get_mask(),
             blueMask = pixel_descriptor.get_blue().get_mask(),
             greenMask = pixel_descriptor.get_green().get_mask(),
             redMask = pixel_descriptor.get_red().get_mask();
    int bpp = pixel_descriptor.get_color_depth().depth();

    uint32_t newFormat = SDL_MasksToPixelFormatEnum(bpp, redMask, greenMask, blueMask, alphaMask);
    if (newFormat == SDL_PIXELFORMAT_UNKNOWN)
    {
        throw idlib::invalid_argument_error(__FILE__, __LINE__, "pixelFormatDescriptor doesn't correspond with a SDL_PixelFormat");
    }
    SDL_Surface *newPixels = SDL_ConvertSurfaceFormat(pixels.get(), newFormat, 0);
    if (!newPixels)
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "unable to convert surface");
    }

    return std::shared_ptr<SDL_Surface>(newPixels, [](SDL_Surface *pSurface) { SDL_FreeSurface(pSurface); });
}

std::shared_ptr<SDL_Surface> power_of_two_functor<SDL_Surface>::operator()(const std::shared_ptr<SDL_Surface>& pixels) const
{
    // Alias old width and old height.
    int oldWidth = pixels->w,
        oldHeight = pixels->h;

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
        return pad(pixels, padding);
    }
    else
    {
        return SDL::cloneSurface(pixels);
    }
}

std::shared_ptr<SDL_Surface> pad_functor<SDL_Surface>::operator()(const std::shared_ptr<SDL_Surface>& pixels, const padding& padding) const
{
    if (!pixels)
        throw idlib::argument_null_error(__FILE__, __LINE__, "pixels");

    if (padding.left < 0)
        throw idlib::invalid_argument_error(__FILE__, __LINE__, "left < 0");
    if (padding.top < 0)
        throw idlib::invalid_argument_error(__FILE__, __LINE__, "top < 0");
    if (padding.right < 0)
        throw idlib::invalid_argument_error(__FILE__, __LINE__, "right < 0");
    if (padding.bottom < 0)
        throw idlib::invalid_argument_error(__FILE__, __LINE__, "bottom < 0");

    if (!padding.left && !padding.top && !padding.right && !padding.bottom)
    {
        return Ego::SDL::cloneSurface(pixels);
    }

    // Alias old surface.
    const auto& oldSurface = pixels;

    // Alias old width and old height.
    size_t oldWidth = pixels->w,
        oldHeight = pixels->h;

    // Compute new width and new height.
    size_t newWidth = oldWidth + padding.left + padding.right,
        newHeight = oldHeight + padding.top + padding.bottom;

    // Create the copy.
    auto newSurface = std::shared_ptr<SDL_Surface>(SDL_CreateRGBSurface(SDL_SWSURFACE, newWidth, newHeight, oldSurface->format->BitsPerPixel,
                                                                        oldSurface->format->Rmask,
                                                                        oldSurface->format->Gmask,
                                                                        oldSurface->format->Bmask,
                                                                        oldSurface->format->Amask),
                                                                        [](SDL_Surface *pSurface) { SDL_FreeSurface(pSurface); });
    if (!newSurface)
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "SDL_CreateRGBSurface failed");
    }
    // Fill the copy with transparent black.
    SDL_FillRect(newSurface.get(), nullptr, SDL_MapRGBA(newSurface->format, 0, 0, 0, 0));
    // Copy the old surface into the new surface.
    for (size_t y = 0; y < oldHeight; ++y)
    {
        for (size_t x = 0; x < oldWidth; ++x)
        {
            auto p = get_pixel(oldSurface.get(), { x, y });
            Ego::set_pixel(newSurface.get(), p, { padding.left + x, padding.top + y });
        }
    }
    return newSurface;
}

Colour4b get_pixel_functor<SDL_Surface>::operator()(const SDL_Surface *surface, const Point2f& point) const
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }

    int32_t x = std::round(point.x()),
            y = std::round(point.y());
    if (x < 0) throw idlib::argument_out_of_bounds_error(__FILE__, __LINE__, "x");
    if (x >= surface->w) throw idlib::argument_out_of_bounds_error(__FILE__, __LINE__, "x");
    if (y < 0) throw idlib::argument_out_of_bounds_error(__FILE__, __LINE__, "y");
    if (y >= surface->h) throw idlib::argument_out_of_bounds_error(__FILE__, __LINE__, "y");

    int bpp = surface->format->BytesPerPixel;
    // Here p is the address to the pixel we want to get.
    uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
    case 1:
    {
        uint32_t v = *p;
        uint8_t r, g, b, a;
        SDL_GetRGBA(v, surface->format, &r, &g, &b, &a);
        return Colour4b(r, g, b, a);
    }
    case 2:
    {
        uint32_t v = *reinterpret_cast<uint16_t*>(p);
        uint8_t r, g, b, a;
        SDL_GetRGBA(v, surface->format, &r, &g, &b, &a);
        return Colour4b(r, g, b, a);
    }
    case 3:
    {
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        {
            uint32_t v = p[0] << 16 | p[1] << 8 | p[2];
            uint8_t r, g, b, a;
            SDL_GetRGBA(v, surface->format, &r, &g, &b, &a);
            return Colour4b(r, g, b, a);
        }
        else
        {
            uint32_t v = p[0] | p[1] << 8 | p[2] << 16;
            uint8_t r, g, b, a;
            SDL_GetRGBA(v, surface->format, &r, &g, &b, &a);
            return Colour4b(r, g, b, a);
        }
    }
    case 4:
    {
        uint32_t v = *reinterpret_cast<uint32_t*>(p);
        uint8_t r, g, b, a;
        SDL_GetRGBA(v, surface->format, &r, &g, &b, &a);
        return Colour4b(r, g, b, a);
    }
    default:
        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__, "unreachable code reached"); /* shouldn't happen, but avoids warnings */
    }
}

void blit_functor<SDL_Surface>::operator()(SDL_Surface *source, SDL_Surface *target) const
{
    if (!source)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "source");
    }
    if (!target)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "target");
    }
    SDL_BlitSurface(source, nullptr, target, nullptr);
}

void blit_functor<SDL_Surface>::operator()(SDL_Surface *source, const Rectangle2f& source_rectangle, SDL_Surface *target) const
{
    if (!source)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "source");
    }
    if (!target)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "target");
    }
    SDL_Rect sdl_source_rectangle;
    sdl_source_rectangle.x = source_rectangle.get_min().x();
    sdl_source_rectangle.y = source_rectangle.get_min().y();
    sdl_source_rectangle.w = source_rectangle.get_size().x();
    sdl_source_rectangle.h = source_rectangle.get_size().y();
    SDL_BlitSurface(source, &sdl_source_rectangle, target, nullptr);
}

void blit_functor<SDL_Surface>::operator()(SDL_Surface *source, SDL_Surface *target, const Point2f& target_position) const
{
    if (!source)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "source");
    }
    if (!target)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "target");
    }
    SDL_Rect sdl_target_rectangle;
    sdl_target_rectangle.x = target_position.x();
    sdl_target_rectangle.y = target_position.y();
    sdl_target_rectangle.w = 0;
    sdl_target_rectangle.h = 0;
    SDL_BlitSurface(source, nullptr, target, &sdl_target_rectangle);
}

void blit_functor<SDL_Surface>::operator()(SDL_Surface *source, const Rectangle2f& source_rectangle, SDL_Surface *target, const Point2f& target_position) const
{
    if (!source)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "source");
    }
    if (!target)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "target");
    }
    SDL_Rect sdl_source_rectangle;
    sdl_source_rectangle.x = source_rectangle.get_min().x();
    sdl_source_rectangle.y = source_rectangle.get_min().y();
    sdl_source_rectangle.w = source_rectangle.get_size().x();
    sdl_source_rectangle.h = source_rectangle.get_size().y();
    SDL_Rect sdl_target_rectangle;
    sdl_target_rectangle.x = target_position.x();
    sdl_target_rectangle.y = target_position.y();
    sdl_target_rectangle.w = 0;
    sdl_target_rectangle.h = 0;
    SDL_BlitSurface(source, &sdl_source_rectangle, target, &sdl_target_rectangle);
}

void fill_functor<SDL_Surface>::operator()(SDL_Surface *surface, const Colour3b& color) const
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    SDL_FillRect(surface, nullptr, SDL::make_rgb(surface, color));
}

void fill_functor<SDL_Surface>::operator()(SDL_Surface *surface, const Colour4b& color) const
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    SDL_FillRect(surface, nullptr, SDL::make_rgba(surface, color));
}

void fill_functor<SDL_Surface>::operator()(SDL_Surface *surface, const Colour3b& color, const Rectangle2f& rectangle) const
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    SDL_Rect sdl_rectangle;
    sdl_rectangle.x = rectangle.get_min().x();
    sdl_rectangle.y = rectangle.get_min().y();
    sdl_rectangle.w = rectangle.get_size().x();
    sdl_rectangle.h = rectangle.get_size().y();
    SDL_FillRect(surface, &sdl_rectangle, SDL::make_rgb(surface, color));
}

void fill_functor<SDL_Surface>::operator()(SDL_Surface *surface, const Colour4b& color, const Rectangle2f& rectangle) const
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    SDL_Rect sdl_rectangle;
    sdl_rectangle.x = rectangle.get_min().x();
    sdl_rectangle.y = rectangle.get_min().y();
    sdl_rectangle.w = rectangle.get_size().x();
    sdl_rectangle.h = rectangle.get_size().y();
    SDL_FillRect(surface, &sdl_rectangle, SDL::make_rgba(surface, color));
}

void set_pixel_functor<SDL_Surface>::operator()(SDL_Surface *surface, const Colour3b& color, const Point2f& point) const
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    uint32_t coded_color = SDL::make_rgb(surface, color);
    (*this)(surface, coded_color, point);
}

void set_pixel_functor<SDL_Surface>::operator()(SDL_Surface *surface, const Colour4b& color, const Point2f& point) const
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    uint32_t coded_color = SDL::make_rgba(surface, color);
    (*this)(surface, coded_color, point);
}

void set_pixel_functor<SDL_Surface>::operator()(SDL_Surface *surface, uint32_t color, const Point2f& point) const
{
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }
    int32_t x = std::round(point.x()),
            y = std::round(point.y());
    if (x < 0) return;
    if (x >= surface->w) return;
    if (y < 0) return;
    if (y >= surface->h) return;

    // Get Bytes per pixel.
    int bpp = surface->format->BytesPerPixel;

    // Here p is the address to the pixel we want to set.
    uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
    case 1:
        *p = color;
        break;

    case 2:
        *reinterpret_cast<uint16_t*>(p) = color;
        break;

    case 3:
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        p[0] = (color >> 16) & 0xff;
        p[1] = (color >> 8) & 0xff;
        p[2] = color & 0xff;
#else
        p[0] = color & 0xff;
        p[1] = (color >> 8) & 0xff;
        p[2] = (color >> 16) & 0xff;
#endif
        break;

    case 4:
        *reinterpret_cast<uint32_t*>(p) = color;
        break;

    default:
        throw idlib::unhandled_switch_case_error(__FILE__, __LINE__, "unreachable code reached"); /* shouldn't happen, but avoids warnings */
    }
}

} // namespace Ego

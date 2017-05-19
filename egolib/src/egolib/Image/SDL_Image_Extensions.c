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

namespace Ego {
namespace SDL {

PixelFormatDescriptor getPixelFormat(const SDL_PixelFormat& source)
{
    if (source.palette)
    {
        throw id::runtime_error(__FILE__, __LINE__, "pixel format not supported");
    }
    const PixelFormatDescriptor pfds[] = {
        PixelFormatDescriptor::get<PixelFormat::R8G8B8A8>(),
        PixelFormatDescriptor::get<PixelFormat::B8G8R8A8>(),
        PixelFormatDescriptor::get<PixelFormat::B8G8R8>(),
        PixelFormatDescriptor::get<PixelFormat::R8G8B8>()
    };
    for (size_t i = 0; i < 4; ++i)
    {
        if (source.BytesPerPixel == pfds[i].getColourDepth().getDepth() / 8 &&
            source.Amask == pfds[i].getAlphaMask() &&
            source.Rmask == pfds[i].getRedMask() &&
            source.Gmask == pfds[i].getGreenMask() &&
            source.Bmask == pfds[i].getBlueMask() &&
            source.Ashift == pfds[i].getAlphaShift() &&
            source.Rshift == pfds[i].getRedShift() &&
            source.Gshift == pfds[i].getGreenShift() &&
            source.Bshift == pfds[i].getBlueShift() &&
            source.BitsPerPixel == pfds[i].getColourDepth().getDepth())
        {
            return pfds[i];
        }
    }
    throw id::runtime_error(__FILE__, __LINE__, "pixel format not supported");
}

uint32_t getEnumeratedPixelFormat(const PixelFormatDescriptor& pixelFormatDescriptor)
{
    uint32_t alphaMask = pixelFormatDescriptor.getAlphaMask(),
        blueMask = pixelFormatDescriptor.getBlueMask(),
        greenMask = pixelFormatDescriptor.getGreenMask(),
        redMask = pixelFormatDescriptor.getRedMask();
    int bitsPerPixel = pixelFormatDescriptor.getColourDepth().getDepth();

    uint32_t pixelFormatEnum_sdl = SDL_MasksToPixelFormatEnum(bitsPerPixel, redMask, greenMask, blueMask, alphaMask);
    if (SDL_PIXELFORMAT_UNKNOWN == pixelFormatEnum_sdl)
    {
        throw id::runtime_error(__FILE__, __LINE__, "pixel format descriptor has no corresponding SDL pixel format");
    }
    return pixelFormatEnum_sdl;
}

std::shared_ptr<const SDL_PixelFormat> getPixelFormat(const PixelFormatDescriptor& pixelFormatDescriptor)
{
    std::shared_ptr<const SDL_PixelFormat> pixelFormat_sdl = std::shared_ptr<const SDL_PixelFormat>
        (
            SDL_AllocFormat(getEnumeratedPixelFormat(pixelFormatDescriptor)),
            [](SDL_PixelFormat *pixelFormat) { if (pixelFormat) { SDL_FreeFormat(pixelFormat); } }
    );
    if (!pixelFormat_sdl)
    {
        throw id::environment_error(__FILE__, __LINE__, "SDL", "internal error");
    }
    return pixelFormat_sdl;
}

std::shared_ptr<SDL_Surface> createSurface(int width, int height, const PixelFormatDescriptor& pixelFormat)
{
    if (width < 0)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "negative width");
    }
    if (height < 0)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "negative height");
    }
    std::shared_ptr<const SDL_PixelFormat> pixelFormat_sdl = getPixelFormat(pixelFormat);
    SDL_Surface *surface_sdl = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
                                                    pixelFormat_sdl->BitsPerPixel,
                                                    pixelFormat_sdl->Rmask, pixelFormat_sdl->Gmask,
                                                    pixelFormat_sdl->Bmask, pixelFormat_sdl->Amask);
    if (nullptr == surface_sdl)
    {
        throw id::runtime_error(__FILE__, __LINE__, "SDL_CreateRGBSurface failed");
    }
    try
    {
        return std::shared_ptr<SDL_Surface>(surface_sdl, [](SDL_Surface *pSurface) { SDL_FreeSurface(pSurface); });
    }
    catch (...)
    {
        SDL_FreeSurface(surface_sdl);
        std::rethrow_exception(std::current_exception());
    }
}

std::shared_ptr<SDL_Surface> padSurface(const std::shared_ptr<const SDL_Surface>& surface, const Padding& padding)
{
    if (!surface)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "nullptr == surface");
    }
    if (!padding.left && !padding.top && !padding.right && !padding.bottom)
    {
        return cloneSurface(surface);
    }

    // Alias old surface.
    const auto& oldSurface = surface;

    // Alias old width and old height.
    size_t oldWidth = surface->w,
        oldHeight = surface->h;

    // Compute new width and new height.
    size_t newWidth = oldWidth + padding.left + padding.right,
        newHeight = oldHeight + padding.top + padding.bottom;

    // Create the copy.
    auto newSurface = std::shared_ptr<SDL_Surface>(SDL_CreateRGBSurface(SDL_SWSURFACE, newWidth, newHeight, oldSurface->format->BitsPerPixel,
                                                                        oldSurface->format->Rmask, oldSurface->format->Gmask, oldSurface->format->Bmask,
                                                                        oldSurface->format->Amask), [](SDL_Surface *pSurface) { SDL_FreeSurface(pSurface); });
    if (!newSurface)
    {
        throw id::runtime_error(__FILE__, __LINE__, "SDL_CreateRGBSurface failed");
    }
    // Fill the copy with transparent black.
    SDL_FillRect(newSurface.get(), nullptr, SDL_MapRGBA(newSurface->format, 0, 0, 0, 0));
    // Copy the old surface into the new surface.
    for (size_t y = 0; y < oldHeight; ++y)
    {
        for (size_t x = 0; x < oldWidth; ++x)
        {
            uint32_t p = getPixel(oldSurface, x, y);
            uint8_t r, g, b, a;
            SDL_GetRGBA(p, surface->format, &r, &g, &b, &a);
            uint32_t q = SDL_MapRGBA(newSurface->format, r, g, b, a);
            putPixel(newSurface, padding.left + x, padding.top + y, q);
        }
    }
    return newSurface;
}

std::shared_ptr<SDL_Surface> cloneSurface(const std::shared_ptr<const SDL_Surface>& surface)
{
    static_assert(SDL_VERSION_ATLEAST(2, 0, 0), "SDL 2.x required");
    if (!surface)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "nullptr == surface");
    }
    // TODO: The signature SDL_ConvertSurface(SDL_Surface *, const SDL_PixelFormat *, uint32_t) might be considered as a bug.
    //       It should be SDL_ConvertSurface(const SDL_Surface *, const SDL_PixelFormat *, uint32_t).
    auto clone = std::shared_ptr<SDL_Surface>(SDL_ConvertSurface((SDL_Surface *)surface.get(), surface->format, 0), [](SDL_Surface *pSurface) { SDL_FreeSurface(pSurface); });
    if (!clone)
    {
        throw id::runtime_error(__FILE__, __LINE__, "SDL_ConvertSurface failed");
    }
    return clone;
}

uint32_t getPixel(const std::shared_ptr<const SDL_Surface>& surface, int x, int y)
{
    if (!surface)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "nullptr == surface");
    }
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to get. */
    uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
        case 1:
            return *p;

        case 2:
            return *reinterpret_cast<uint16_t*>(p);

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
                return p[0] << 16 | p[1] << 8 | p[2];
            }
            else
            {
                return p[0] | p[1] << 8 | p[2] << 16;
            }
            break;

        case 4:
            return *reinterpret_cast<uint32_t*>(p);

        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__, "unreachable code reached"); /* shouldn't happen, but avoids warnings */
    }
}

void putPixel(const std::shared_ptr<SDL_Surface>& surface, int x, int y, uint32_t pixel)
{
    if (!surface)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "nullptr == surface");
    }
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set. */
    uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
        case 1:
            *p = pixel;
            break;

        case 2:
            *reinterpret_cast<uint16_t*>(p) = pixel;
            break;

        case 3:
        #if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        #else
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        #endif
            break;

        case 4:
            *reinterpret_cast<uint32_t*>(p) = pixel;
            break;

        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__, "unreachable code reached"); /* shouldn't happen, but avoids warnings */
    }
}

std::shared_ptr<SDL_Surface> convertPixelFormat(const std::shared_ptr<SDL_Surface>& surface, const PixelFormatDescriptor& pixelFormatDescriptor)
{
    if (!surface)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "nullptr == surface");
    }

    uint32_t alphaMask = pixelFormatDescriptor.getAlphaMask(),
        blueMask = pixelFormatDescriptor.getBlueMask(),
        greenMask = pixelFormatDescriptor.getGreenMask(),
        redMask = pixelFormatDescriptor.getRedMask();
    int bpp = pixelFormatDescriptor.getColourDepth().getDepth();

    uint32_t newFormat = SDL_MasksToPixelFormatEnum(bpp, redMask, greenMask, blueMask, alphaMask);
    if (newFormat == SDL_PIXELFORMAT_UNKNOWN)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "pixelFormatDescriptor doesn't correspond with a SDL_PixelFormat");
    }
    SDL_Surface *newSurface = SDL_ConvertSurfaceFormat(surface.get(), newFormat, 0);
    if (!newSurface)
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to convert surface");
    }

    return std::shared_ptr<SDL_Surface>(newSurface, [](SDL_Surface *pSurface) { SDL_FreeSurface(pSurface); });
    }

std::shared_ptr<SDL_Surface> convertPowerOfTwo(const std::shared_ptr<SDL_Surface>& surface)
{
    // Alias old width and old height.
    int oldWidth = surface->w,
        oldHeight = surface->h;

    // Compute new width and new height.
    int newWidth = Math::powerOfTwo(oldWidth),
        newHeight = Math::powerOfTwo(oldHeight);

    std::shared_ptr<SDL_Surface> newSurface = nullptr;

    // Only if the new dimension differ from the old dimensions, perform the scaling.
    if (newWidth != oldWidth || newHeight != oldHeight)
    {
        Padding padding;
        padding.left = 0;
        padding.top = 0;
        padding.right = newWidth - oldWidth;
        padding.bottom = newHeight - oldHeight;
        newSurface = padSurface(surface, padding);
    }
    else
    {
        newSurface = surface;
    }

    // Return the result.
    return newSurface;
}

bool testAlpha(const std::shared_ptr<SDL_Surface>& surface)
{
    // test to see whether an image requires alpha blending

    if (!surface)
    {
        throw id::invalid_argument_error(__FILE__, __LINE__, "nullptr == surface");
    }

    // Alias.
    SDL_PixelFormat *format = surface->format;

    // (1)
    // If the surface has a per-surface color key,
    // it is partially transparent.
    uint32_t colorKey;
    int rslt = SDL_GetColorKey(surface.get(), &colorKey);
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
    SDL_GetSurfaceAlphaMod(surface.get(), &alpha);
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

Math::Colour3b getColourMod(SDL_Surface& surface)
{
    Uint8 r, g, b;
    SDL_GetSurfaceColorMod(&surface, &r, &g, &b);
    return Math::Colour3b(r, g, b);
}

void setColourMod(SDL_Surface& surface, const Math::Colour3b& colourMod)
{
    SDL_SetSurfaceColorMod(&surface, colourMod.get_r(), colourMod.get_g(), colourMod.get_b());
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
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
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
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}

BlendMode getBlendMode(SDL_Surface& surface)
{
    SDL_BlendMode blendMode;
    SDL_GetSurfaceBlendMode(&surface, &blendMode);
    return blendModeToInternal(blendMode);
}

void setBlendMode(SDL_Surface& surface, BlendMode blendMode)
{
    SDL_SetSurfaceBlendMode(&surface, blendModeToExternal(blendMode));
}

void fillSurface(SDL_Surface& surface, const Ego::Math::Colour4b& colour)
{
    SDL_FillRect(&surface, nullptr, SDL_MapRGBA(surface.format, colour.get_r(),
                                                                colour.get_g(),
                                                                colour.get_b(),
                                                                colour.get_a()));

}

Ego::PixelFormat getPixelFormat(SDL_Surface& surface)
{
    switch (surface.format->format)
    {
        case SDL_PIXELFORMAT_RGB888:
            return PixelFormat::R8G8B8;
        case SDL_PIXELFORMAT_RGBA8888:
            return PixelFormat::R8G8B8A8;
        case SDL_PIXELFORMAT_BGR888:
            return PixelFormat::B8G8R8;
        case SDL_PIXELFORMAT_BGRA8888:
            return PixelFormat::B8G8R8A8;
        case SDL_PIXELFORMAT_ABGR8888:
            return PixelFormat::A8B8G8R8;
        default:
            throw id::runtime_error(__FILE__, __LINE__, "unsupported/unknown pixel format");
    };
}

} // namespace SDL
} // namespace Ego

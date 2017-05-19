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
#include "egolib/vfs.h"
#include "egolib/Log/_Include.hpp"
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

bool testAlpha(SDL_Surface *surface)
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
    } 
#if 0
    else if (rslt == -1)
    {
      // If a value of -1 is returned, the surface has no color key: Continue.
    }
#endif
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

uint32_t getPixel(SDL_Surface& surface, int x, int y)
{
    int bpp = surface.format->BytesPerPixel;
    /* Here p is the address to the pixel we want to get. */
    uint8_t *p = (uint8_t *)surface.pixels + y * surface.pitch + x * bpp;

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

void putPixel(SDL_Surface& surface, int x, int y, uint32_t pixel)
{
    int bpp = surface.format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set. */
    uint8_t *p = (uint8_t *)surface.pixels + y * surface.pitch + x * bpp;

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

Image::Image(SDL_Surface *surface) :
    m_surface(surface),
    m_hasAlpha(testAlpha(surface)),
    m_pixelFormat(Ego::SDL::getPixelFormat(*surface))
{}

Image::~Image()
{
    SDL_FreeSurface(m_surface);
    m_surface = nullptr;
}

PixelFormat Image::getPixelFormat() const
{
    return m_pixelFormat;
}

int Image::getBytesPerPixel() const
{
    return m_surface->format->BytesPerPixel;
}

int Image::getWidth() const
{
    return m_surface->w;
}

int Image::getHeight() const
{
    return m_surface->h;
}

int Image::getPitch() const
{
    return m_surface->pitch;
}

bool Image::hasAlpha() const
{
    return m_hasAlpha;
}

void Image::pad(int left, int top, int right, int bottom)
{
    if (left < 0) throw id::invalid_argument_error(__FILE__, __LINE__, "left < 0");
    if (top < 0) throw id::invalid_argument_error(__FILE__, __LINE__, "top < 0");
    if (right < 0) throw id::invalid_argument_error(__FILE__, __LINE__, "right < 0");
    if (bottom < 0) throw id::invalid_argument_error(__FILE__, __LINE__, "bottom < 0");

    // Alias old surface.
    auto *oldSurface = m_surface;

    // Alias old width and old height.
    auto oldWidth = oldSurface->w,
        oldHeight = oldSurface->h;

    // Compute new width and new height.
    auto newWidth = oldWidth + left + right,
        newHeight = oldHeight + top + bottom;

    // Create the copy.
    auto *newSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, newWidth, newHeight, oldSurface->format->BitsPerPixel,
                                            oldSurface->format->Rmask, oldSurface->format->Gmask, oldSurface->format->Bmask,
                                            oldSurface->format->Amask);
    if (!newSurface)
    {
        throw id::runtime_error(__FILE__, __LINE__, "SDL_CreateRGBSurface failed");
    }
    // Fill the copy with transparent black.
    if (-1 == SDL_FillRect(newSurface, nullptr, SDL_MapRGBA(newSurface->format, 0, 0, 0, 0)))
    {
        SDL_FreeSurface(newSurface);
        throw id::runtime_error(__FILE__, __LINE__, "SDL_FillRect failed");
    }
    // Copy the old surface into the new surface.
    for (size_t y = 0; y < oldHeight; ++y)
    {
        for (size_t x = 0; x < oldWidth; ++x)
        {
            uint32_t p = getPixel(*oldSurface, x, y);
            uint8_t r, g, b, a;
            SDL_GetRGBA(p, oldSurface->format, &r, &g, &b, &a);
            uint32_t q = SDL_MapRGBA(newSurface->format, r, g, b, a);
            putPixel(*newSurface, left + x, top + y, q);
        }
    }
    SDL_FreeSurface(m_surface);
    m_surface = newSurface;
}

void Image::powerOfTwo()
{
    // Alias old width and old height.
    int oldWidth = m_surface->w,
        oldHeight = m_surface->h;

    // Compute new width and new height.
    int newWidth = Math::powerOfTwo(oldWidth),
        newHeight = Math::powerOfTwo(oldHeight);

    // Only if the new dimension differ from the old dimensions, perform the scaling.
    if (newWidth != oldWidth || newHeight != oldHeight)
    {
        pad(0, 0, newWidth - oldWidth, newHeight - oldHeight);
    }
}

void Image::fill(Math::Colour4b& colour)
{
    SDL_FillRect(m_surface, nullptr, SDL_MapRGBA(m_surface->format, colour.get_r(), 
                                                                    colour.get_g(),
                                                                    colour.get_b(), 
                                                                    colour.get_a()));
}

} // namespace Ego

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

#if !SDL_VERSION_ATLEAST(2, 0, 0)
int SDL_GetColorKey(SDL_Surface *surface, uint32_t *key) {
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

SDL_Surface *Image::getA8R8G8B8(int width, int height) {
    /* SDL interprets each pixel as a 32-bit number, so our masks must depend on the endianness (byte order) of the machine. */
    // A8R8G8B8 means, that the 1st Byte is alpha, the 2nd Byte is red, the 3rd Byte is green and the 4th Byte is blue.
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    static const Uint32 rmask = 0x0000ff00;
    static const Uint32 gmask = 0x00ff0000;
    static const Uint32 bmask = 0xff000000;
    static const Uint32 amask = 0x000000ff;
#else
    static const Uint32 rmask = 0x00ff0000;
    static const Uint32 gmask = 0x0000ff00;
    static const Uint32 bmask = 0x000000ff;
    static const Uint32 amask = 0xff000000;
#endif

    if (width < 0 || height < 0)
    {
        return nullptr;
    }
    return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, rmask, gmask, bmask, amask);
}

SDL_Surface *Image::getR8G8B8A8(int width, int height) {
    /* SDL interprets each pixel as a 32-bit number, so our masks must depend on the endianness (byte order) of the machine. */
    // R8G8B8A8 means, that the 1st Byte is red, the 2nd Byte is green, the 3rd Byte is blue and the 4th Byte is alpha.
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    static const Uint32 rmask = 0x000000ff;
    static const Uint32 gmask = 0x0000ff00;
    static const Uint32 bmask = 0x00ff0000;
    static const Uint32 amask = 0xff000000;
#else
    static const Uint32 rmask = 0xff000000;
    static const Uint32 gmask = 0x00ff0000;
    static const Uint32 bmask = 0x0000ff00;
    static const Uint32 amask = 0x000000ff;
#endif

    if (width < 0 || height < 0) {
        return nullptr;
    }
    return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, rmask, gmask, bmask, amask);
}

Image::Image(SDL_Surface *surface) :
    _surface(surface)
{ }

Image::~Image() {
    SDL_FreeSurface(_surface);
    _surface = nullptr;
}

Image *Image::load(const std::string& pathname) {
    // a) Load the surface.
    SDL_RWops *rwops = vfs_openRWopsRead(pathname);
    if (!rwops) {
        Log::get().warn("%s:%d: unable to load image `%s` - reason `%s`\n", __FILE__, __LINE__, pathname.c_str(), SDL_GetError());
        return nullptr;
    }
    SDL_Surface *surface = IMG_Load_RW(rwops, 1);
    if (!surface) {
        Log::get().warn("%s:%d: unable to load image `%s` - reason `%s`\n", __FILE__, __LINE__, pathname.c_str(), IMG_GetError());
        return nullptr;
    }
    // b) If surface is palettized, convert to unpalettized.
    {
        SDL_PixelFormat *pixelFormat = surface->format;
        if (pixelFormat->palette) {
            SDL_Surface *newSurface = getR8G8B8A8(surface->w, surface->h);
            if (!newSurface) {
                Log::get().warn("%s:%d: unable to convert image `%s` - reason `%s`\n", __FILE__, __LINE__, pathname.c_str(), SDL_GetError());
                SDL_FreeSurface(surface);
                return nullptr;
            }
            if (SDL_BlitSurface(surface, nullptr, newSurface, nullptr) < 0) {
                Log::get().warn("%s:%d: unable to convert image `%s` - reason `%s`\n", __FILE__, __LINE__, pathname.c_str(), SDL_GetError());
                SDL_FreeSurface(newSurface);
                SDL_FreeSurface(surface);
                return nullptr;
            }
            SDL_FreeSurface(surface);
            surface = newSurface;
        }
    }
    // c) If the image has a colour key but no alpha channel, convert to alpha channel.
    {
        uint32_t colorKey;
        int rslt = SDL_GetColorKey(surface, &colorKey);
        if (rslt < -1) {
            // If a value smaller than -1 is returned, an error occured.
            SDL_FreeSurface(surface);
            return nullptr;
        } else if (rslt >= 0) {
            // If a value greater or equal than 0 is returned, the surface has a colour key.
            SDL_Surface *newSurface = getR8G8B8A8(surface->w, surface->h);
            if (!newSurface) {
                Log::get().warn("%s:%d: unable to convert image `%s` - reason `%s`\n", __FILE__, __LINE__, pathname.c_str(), SDL_GetError());
                SDL_FreeSurface(surface);
                return nullptr;
            }
        }
        /*
        else if (rslt == -1)
        {
        // If a value of -1 is returned, the surface has no colour key.

        }
        */
    }
    Image *image;
    try {
        image = new Image(surface);
    } catch (...) {
        SDL_FreeSurface(surface);
        return nullptr;
    }
    return image;
}

int Image::getBytesPerPixel() const {
    return _surface->format->BytesPerPixel;
}

int Image::getWidth() const {
    return _surface->w;
}

int Image::getHeight() const {
    return _surface->h;
}

int Image::getPitch() const {
    // As always, MSVC is the last compiler supporting something.
#if defined(_MSC_VER)
    static_assert(UINT16_MAX <= INT_MAX,"UINT16_MAX must be smaller than or equal to INT_MAX");
#else
    static_assert(std::numeric_limits<Uint16>::max() <= std::numeric_limits<int>::max(),"UINT16_MAX must be smaller than or equal to INT_MAX");
#endif
    return _surface->pitch;
}

}

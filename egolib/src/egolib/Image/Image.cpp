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
    _surface(surface)
{}

Image::~Image()
{
    SDL_FreeSurface(_surface);
    _surface = nullptr;
}

int Image::getBytesPerPixel() const
{
    return _surface->format->BytesPerPixel;
}

int Image::getWidth() const
{
    return _surface->w;
}

int Image::getHeight() const
{
    return _surface->h;
}

int Image::getPitch() const
{
    return _surface->pitch;
}

} // namespace Ego

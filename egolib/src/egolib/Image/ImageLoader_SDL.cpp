//********************************************************************************************
//*
//*    This file is part of Egoboo.
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

#include "egolib/Image/ImageLoader_SDL.hpp"

using string = std::string;

ImageLoader_SDL::ImageLoader_SDL(const string& extension) :
    ImageLoader(extension)
{}

std::shared_ptr<SDL_Surface> ImageLoader_SDL::load(vfs_FILE *file) const {
    SDL_Surface *surface = SDL_LoadBMP_RW(vfs_openRWops(file, false), 1);
    if (!surface) {
        return nullptr;
    }
    return std::shared_ptr<SDL_Surface>(surface, [ ](SDL_Surface *pSurface) { SDL_FreeSurface(pSurface); });
}

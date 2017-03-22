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

#include "egolib/Image/ImageLoader_SDL_image.hpp"

namespace Ego {
namespace Internal {

ImageLoader_SDL_image::ImageLoader_SDL_image(const std::unordered_set<std::string>& extensions) :
    ImageLoader(extensions) {}

std::shared_ptr<SDL_Surface> ImageLoader_SDL_image::load(vfs_FILE *file) const {
    SDL_Surface *surface = IMG_Load_RW(vfs_openRWops(file, false), 1);
    if (!surface) {
        return nullptr;
    }
    return std::shared_ptr<SDL_Surface>(surface, [](SDL_Surface *surface) { SDL_FreeSurface(surface); });
}

} // namespace Internal
} // namespace Ego

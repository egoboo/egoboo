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

#pragma once

#include "egolib/Image/ImageLoader.hpp"

struct ImageLoader_SDL : public ImageLoader
{

protected:

    using string = std::string;

    // Befriend with ImageManager.
    friend class ImageManager;

#if 0
    // Questionable style. std::make_unique is well-specified but it's actually not controlled by us and hence should not be "friended".
    // Befriend with make unique.
    /*template <typename ... Args>*/
    friend unique_ptr<ImageLoader_SDL> make_unique<ImageLoader_SDL>(/*Args&&... args*/);
#endif

    ImageLoader_SDL(const string& extension);

    virtual std::shared_ptr<SDL_Surface> load(vfs_FILE *file) const override;

};

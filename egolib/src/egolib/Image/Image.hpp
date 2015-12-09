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

#include "egolib/platform.h"

#if !SDL_VERSION_ATLEAST(2, 0, 0)
int SDL_GetColorKey(SDL_Surface *surface, uint32_t *key);
#endif

namespace Ego {

struct Image {
protected:

    /**
     * @brief
     *  The backing SDL surface.
     */
    SDL_Surface *_surface;

    /**
     * @brief
     *  Construct this image.
     * @param surface
     *  the backing SDL surface. The image takes ownership.
     */
    Image(SDL_Surface *surface);

public:

    /**
     * @brief
     *  Destruct this image.
     */
    virtual ~Image();

    /**
     * @brief
     *  Get the size, in Bytes, of a pixel.
     * @return
     *  the size, in Bytes, of a pixel
     */
    int getBytesPerPixel() const;

    /**
     * @brief
     *  Get the width, in pixels, of this image.
     * @return
     *  the width
     * @remark
     *  The width is always non-negative.
     */
    int getWidth() const;

    /**
     * @brief
     *  Get the height, in pixels, of this image.
     * @return
     *  the height
     * @remark
     *  The height is always non-negative.
     */
    int getHeight() const;

    /**
     * @brief
     *  Get the pitch, in pixels, of this image.
     * @return
     *  the pitch
     * @remark
     *  The pitch is always non-negative
     */
    int getPitch() const;

};

} // namespace Ego

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
int SDL_GetColorKey(SDL_Surface *surface, Uint32 *key);
#endif

namespace Ego
{

    struct Image
    {
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
         *  Create an SDL surface with the pixel format A8R8G8B8.
         * @param width
         *  the width, in pixels, of the surface.
         *  Must be non-negative.
         * @param height
         *  the height, in pixels, of the surface.
         *  Must be non-negative.
         * @return
         *  the address of the surface on success, @a nullptr on failure
         */
        static SDL_Surface *getA8R8G8B8(int width, int height);

        /**
         * @brief
         *  Get a pixel format description for R8G8B8A8.
         * @param width
         *  the width, in pixels, of the surface.
         *  Must be non-negative.
         * @param height
         *  the height, in pixels, of the surface.
         *  Must be non-negative.
         * @return
         *  the address of the surface on success, @a nullptr on failure
         */
        static SDL_Surface *getR8G8B8A8(int width, int height);

        /**
         * @brief
         *  Destruct this image.
         */
        virtual ~Image();

        /**
         * @brief
         *  Load an image.
         * @param pathname
         *  the pathname to load the image from
         * @return
         *  a pointer to the image on success, @a nullptr on failure
         * @remark
         *  A palettized images will be converted to an RGBA image.
         *  An image with a color key will be converted to an RGBA image.
         */
        static Image *load(const char *pathname);

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

}

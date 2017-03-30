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

#include "egolib/Graphics/PixelFormat.hpp"
#include "egolib/Math/_Include.hpp"

#if !SDL_VERSION_ATLEAST(2, 0, 0)
int SDL_GetColorKey(SDL_Surface *surface, uint32_t *key);
#endif

namespace Ego {

class Image
{
protected:
    /// @brief The backing SDL surface.
    SDL_Surface *m_surface;

    /// @brief Wether the backing SDL surface has an alpha component.
    bool m_hasAlpha;

    /// @brief The pixel format.
    PixelFormat m_pixelFormat;

public:
    /// @brief Construct this image.
    /// @param surface the backing SDL surface. The image takes ownership.
    Image(SDL_Surface *surface);

    /// @brief Destruct this image.
    virtual ~Image();

    /// @brief Get a pointer to the pixels of this image.
    /// @return a pointer to the pixels of this image
    void *getPixels();

    /// @brief Get a pointer to the backing image.
    /// @return a pointer to the backing image
    SDL_Surface *getSurface();

    /// @brief Get the pixel format of this image.
    /// @return the pixel format of this image
    PixelFormat getPixelFormat() const;

    /// @brief Get the size, in Bytes, of a pixel.
    /// @return the size, in Bytes, of a pixel
    int getBytesPerPixel() const;

    /// @brief Get the width, in pixels, of this image.
    /// @return the width
    /// @remark The width is always non-negative.
    int getWidth() const;

    /// @brief Get the height, in pixels, of this image.
    /// @return the height
    /// @remark The height is always non-negative.
    int getHeight() const;

    /// @brief Get the pitch, in pixels, of this image.
    /// @return the pitch
    /// @remark The pitch is always non-negative.
    int getPitch() const;

    /// @brief Get if this image has an alpha component.
    /// @return @a true if this image has an alpha component, @a false otherwise
    bool hasAlpha() const;

    /// @brief Pad this image.
    /// @param left, top, right, bottom the paddings. Must be non-negative.
    /// @remark The padding is black (if no alpha channel is present) or transparent black (if alpha channel is present).
    void pad(int left, int top, int right, int bottom);

    /// @brief The image is padded to the right (bottom) such that its width (height)
    /// is the smallest power of two greater than or equal to its old width (height).
    void powerOfTwo();

    /// @brief Fill this image with the specified colour.
    /// @param colour the fill colour
    void fill(Math::Colour4b& colour);

}; // class Image

} // namespace Ego

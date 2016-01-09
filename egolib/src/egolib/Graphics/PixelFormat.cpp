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

/// @file   egolib/Graphics/PixelFormat.cpp
/// @brief  Pixel formats and pixel format descriptors.
/// @author Michael Heilmann

#include "egolib/Graphics/PixelFormat.hpp"

namespace Ego {


PixelFormatDescriptor::PixelFormatDescriptor(PixelFormat pixelFormat,
                                             uint32_t redShift, uint32_t greenShift,
                                             uint32_t blueShift, uint32_t alphaShift,
                                             uint32_t redMask, uint32_t greenMask,
                                             uint32_t blueMask, uint32_t alphaMask,
                                             const ColourDepth& colourDepth) :
    pixelFormat(pixelFormat),
    redShift(redShift), greenShift(greenShift), blueShift(blueShift), alphaShift(alphaShift),
    redMask(redMask), greenMask(greenMask), blueMask(blueMask), alphaMask(alphaMask),
    colourDepth(colourDepth) {}

uint32_t PixelFormatDescriptor::getAlphaShift() const {
    return alphaShift;
}

uint32_t PixelFormatDescriptor::getBlueShift() const {
    return blueShift;
}

uint32_t PixelFormatDescriptor::getGreenShift() const {
    return greenShift;
}

uint32_t PixelFormatDescriptor::getRedShift() const {
    return redShift;
}

uint32_t PixelFormatDescriptor::getAlphaMask() const {
    return alphaMask;
}

uint32_t PixelFormatDescriptor::getBlueMask() const {
    return blueMask;
}

uint32_t PixelFormatDescriptor::getGreenMask() const {
    return greenMask;
}

uint32_t PixelFormatDescriptor::getRedMask() const {
    return redMask;
}

const ColourDepth& PixelFormatDescriptor::getColourDepth() const {
    return colourDepth;
}

PixelFormat PixelFormatDescriptor::getPixelFormat() const {
    return pixelFormat;
}

const PixelFormatDescriptor& PixelFormatDescriptor::get(PixelFormat pixelFormat) {
    switch (pixelFormat) {
        case PixelFormat::B8G8R8:
        {
            return get<PixelFormat::B8G8R8>();
        }
        break;
        case PixelFormat::B8G8R8A8:
        {
            return get<PixelFormat::B8G8R8A8>();
        }
        break;
        case PixelFormat::R8G8B8:
        {
            return get<PixelFormat::R8G8B8>();
        }
        break;
        case PixelFormat::R8G8B8A8:
        {
            return get<PixelFormat::R8G8B8A8>();
        }
        break;
        default:
        {
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
        }
    };
}

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::B8G8R8>() {
    static const uint32_t redShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        16;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        0;
#endif
    static const uint32_t greenShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        8;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        8;
#endif
    static const uint32_t blueShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        0;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        16;
#endif
    static const uint32_t alphaShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        0;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        0;
#endif
    static const uint32_t redMask =
        ((uint32_t)0xff) << redShift;
    static const uint32_t greenMask =
        ((uint32_t)0xff) << greenShift;
    static const uint32_t blueMask =
        ((uint32_t)0xff) << blueShift;
    static const uint32_t alphaMask =
        ((uint32_t)0xff) << alphaShift;
    static const PixelFormatDescriptor INSTANCE
        (
            PixelFormat::R8G8B8A8,
            redShift, greenShift, blueShift, alphaShift,
            redMask, greenMask, blueMask, alphaMask,
            ColourDepth(24, 8, 8, 8, 0)
            );
    return INSTANCE;
}

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::B8G8R8A8>() {
    static const uint32_t redShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        16;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        8;
#endif
    static const uint32_t greenShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        8;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        16;
#endif
    static const uint32_t blueShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        0;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        24;
#endif
    static const uint32_t alphaShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        24;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        0;
#endif
    static const uint32_t redMask =
        ((uint32_t)0xff) << redShift;
    static const uint32_t greenMask =
        ((uint32_t)0xff) << greenShift;
    static const uint32_t blueMask =
        ((uint32_t)0xff) << blueShift;
    static const uint32_t alphaMask =
        ((uint32_t)0xff) << alphaShift;
    static const PixelFormatDescriptor INSTANCE
        (
            PixelFormat::B8G8R8A8,
            redShift, greenShift, blueShift, alphaShift,
            redMask, greenMask, blueMask, alphaMask,
            ColourDepth(32, 8, 8, 8, 8)
            );
    return INSTANCE;
}

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::R8G8B8>() {
    static const uint32_t redShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        0;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        16;
#endif
    static const uint32_t greenShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        8;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        8;
#endif
    static const uint32_t blueShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        16;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        0;
#endif
    static const uint32_t alphaShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        0; // no alpha
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        0; // no alpha
#endif
    static const uint32_t redMask =
        ((uint32_t)0xff) << redShift;
    static const uint32_t greenMask =
        ((uint32_t)0xff) << greenShift;
    static const uint32_t blueMask =
        ((uint32_t)0xff) << blueShift;
    static const uint32_t alphaMask =
        ((uint32_t)0x00) << alphaShift;
    static const PixelFormatDescriptor INSTANCE
        (
            PixelFormat::R8G8B8,
            redShift, greenShift, blueShift, alphaShift,
            redMask, greenMask, blueMask, alphaMask,
            ColourDepth(24, 8, 8, 8, 0)
            );
    return INSTANCE;
}

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::R8G8B8A8>() {
    static const uint32_t redShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        0;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        24;
#endif
    static const uint32_t greenShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        8;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        16;
#endif
    static const uint32_t blueShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        16;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        8;
#endif
    static const uint32_t alphaShift =
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
        24;
#elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        0;
#endif
    static const uint32_t redMask =
        ((uint32_t)0xff) << redShift;
    static const uint32_t greenMask =
        ((uint32_t)0xff) << greenShift;
    static const uint32_t blueMask =
        ((uint32_t)0xff) << blueShift;
    static const uint32_t alphaMask =
        ((uint32_t)0xff) << alphaShift;
    static const PixelFormatDescriptor INSTANCE
        (
            PixelFormat::R8G8B8A8,
            redShift, greenShift, blueShift, alphaShift,
            redMask, greenMask, blueMask, alphaMask,
            ColourDepth(32, 8, 8, 8, 8)
            );
    return INSTANCE;
}

} // namespace Ego

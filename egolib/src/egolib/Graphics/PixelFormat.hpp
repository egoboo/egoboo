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

/// @file   egolib/Graphics/PixelFormat.hpp
/// @brief  Pixel formats and pixel format descriptors.
/// @author Michael Heilmann


#pragma once

#include "egolib/platform.h"

namespace Ego
{

/**
 * @brief
 *  Enumeration of canonical identifiers for a pixel formats.
 * @author
 *  Michael Heilmann
 */
enum class PixelFormat
{

    /**
     * @brief
     *  8 Bits for the blue component,
     *  8 Bits for the green component, and
     *  8 Bits for the red component
     *  in this Byte order on little endian.
     *  In big endian, the order of the Bytes of the red, green, and blue components is reversed.
     */
    B8G8R8,

    /**
     * @brief
     *  8 Bits for the blue component,
     *  8 Bits for the green component,
     *  8 Bits for the red component, and
     *  8 Bits for the alpha component
     *  in this Byte order on little endian.
     *  In big endian, the order of the Bytes of the red, green, and blue components is reversed.
     */
    B8G8R8A8,

    /**
     * @brief
     *  8 Bits for the red component,
     *  8 Bits for the green component, and
     *  8 Bits for the blue component
     *  in this Byte order on little endian.
     *  In big endian, the order of the Bytes of the red, green, and blue components is reversed.
     */
    R8G8B8,

    /**
     * @brief
     *  8 Bits for the red component,
     *  8 Bits for the green component,
     *  8 Bits for the blue component, and
     *  8 Bits for the alpha component
     *  in this Byte order on little endian.
     *  In big endian, the order of the Bytes of the red, green, blue, and alpha components is reversed.
     */
    R8G8B8A8,

};

/**
 * @brief
 *  A pixel format descriptor suitable to describe the pixel formats
 *  as specified by the Ego::PixelFormat enumeration.
 * @todo
 *  Provide specializations of PixelDescriptor::get() for A8R8G8B8 and A8B8G8R8.
 * @author
 *  Michael Heilmann
 */
struct PixelFormatDescriptor
{

private:

    /**
     * @brief
     *  The pixel format.
     */
    PixelFormat _pixelFormat;
    /**
     * @brief
     *  The used Bits per pixel.
     */
    uint8_t _bitsPerPixel;

    /**
     * @brief
     *  The mask for the alpha Bits.
     */
    uint32_t _alphaMask;

    /**
    * @brief
    *  The mask for the red Bits.
    */
    uint32_t _redMask;

    /**
     * @brief
     *  The mask for the green Bits.
     */
    uint32_t _greenMask;

    /**
     * @brief
     *  The mask for the blue Bits.
     */
    uint32_t _blueMask;

    /**
     * @brief
     *  The shift for the alpha bits.
     */
    uint32_t _alphaShift;

    /**
     * @brief
     *  The shift for the red Bits.
     */
    uint32_t _redShift;

    /**
     * @brief
     *  The shift for the green Bits.
     */
    uint32_t _greenShift;

    /**
     * @brief
     *  The shift for the blue Bits.
     */
    uint32_t _blueShift;

protected:

    /**
     * @brief
     *  Construct this pixel descriptor.
     * @param pixelFormat
     *  the pixel format
     * @param bitsPerPixel
     *  the Bits per pixel
     * @param redShift, greenShift, blueShift, alphaShift
     *  the shifts for the red, green, blue and alpha Bits
     * @param redMask, greenMask, blueMask, alphaMask
     *  the masks for the red, green, blue and alpha Bits
     */
    PixelFormatDescriptor(PixelFormat pixelFormat, uint8_t bitsPerPixel,
                          uint32_t redShift, uint32_t greenShift,
                          uint32_t blueShift, uint32_t alphaShift,
                          uint32_t redMask, uint32_t greenMask,
                          uint32_t blueMask, uint32_t alphaMask) :
        _pixelFormat(pixelFormat), _bitsPerPixel(bitsPerPixel),
        _redShift(redShift), _greenShift(greenShift), _blueShift(blueShift),
        _alphaShift(alphaShift), _redMask(redMask), _greenMask(greenMask),
        _blueMask(blueMask), _alphaMask(alphaMask)
    {}

public:

    /**
     * @brief
     *  Get the used Bits per pixel.
     * @return
     *  the used Bits per pixel
     */
    uint8_t getBitsPerPixel() const
    {
        return _bitsPerPixel;
    }

    /**
     * @brief
     *  Get the shift of the alpha Bits.
     * @return
     *  the shift of the alpha Bits
     */
    uint32_t getAlphaShift() const
    {
        return _alphaShift;
    }

    /**
     * @brief
     *  Get the shift of the blue Bits.
     * @return
     *  the shift of the blue Bits
     */
    uint32_t getBlueShift() const
    {
        return _blueShift;
    }

    /**
     * @brief
     *  Get the shift of the green Bits.
     * @return
     *  the shift of the green Bits
     */
    uint32_t getGreenShift() const
    {
        return _greenShift;
    }

    /**
     * @brief
     *  Get the shift of the red Bits.
     * @return
     *  the shift of the red Bits
     */
    uint32_t getRedShift() const
    {
        return _redShift;
    }

    /**
     * @brief
     *  The mask for the alpha Bits.
     * @return
     *  the mask of the alpha Bits
     */
    uint32_t getAlphaMask() const
    {
        return _alphaMask;
    }

    /**
     * @brief
     *   Get the mask for the blue Bits.
     * @return
     *  the mask of the blue Bits
     */
    uint32_t getBlueMask() const
    {
        return _blueMask;
    }

    /**
     * @brief
     *  The mask for the green Bits.
     * @return
     *  the mask of the green Bits
     */
    uint32_t getGreenMask() const
    {
        return _greenMask;
    }

    /**
     * @brief
     *  Get the mask for the red Bits.
     * @return
     *  the mask of the red Bits
     */
    uint32_t getRedMask() const
    {
        return _redMask;
    }
    
    /**
     * @brief
     *  Get the pixel format.
     * @return
     *  the pixel format
     */
    PixelFormat pixelFormat() const
    {
        return _pixelFormat;
    }

    template<PixelFormat _PixelFormat>
    static const PixelFormatDescriptor& get();
    
    template <>
    static const PixelFormatDescriptor& get<PixelFormat::B8G8R8>()
    {
        static const uint32_t r_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            16;
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            0;
        #endif
        static const uint32_t g_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            8;
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            8;
        #endif
        static const uint32_t b_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            0;
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            16;
        #endif
        static const uint32_t a_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            0;
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            0;
        #endif
        static const uint32_t r_mask =
            ((uint32_t)0xff) << r_shift;
        static const uint32_t g_mask =
            ((uint32_t)0xff) << g_shift;
        static const uint32_t b_mask =
            ((uint32_t)0xff) << b_shift;
        static const uint32_t a_mask =
            ((uint32_t)0xff) << a_shift;
        static const PixelFormatDescriptor INSTANCE
            (
            PixelFormat::R8G8B8A8,
            32,
            r_shift, g_shift, b_shift, a_shift,
            r_mask, g_mask, b_mask, a_mask
            );
        return INSTANCE;
    }
    
    template <>
    static const PixelFormatDescriptor& get<PixelFormat::B8G8R8A8>()
    {
        static const uint32_t r_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            16; // B8 G8 R8 A8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            8;  // in big endian: A8 R8 G8 B8
        #endif
        static const uint32_t g_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            8;  // B8 G8 R8 A8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            16; // in big endian: A8 R8 G8 B8
        #endif
        static const uint32_t b_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            0;  // B8 G8 R8 A8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            24; // in big endian: A8 R8 G8 B8
        #endif
        static const uint32_t a_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            24; // B8 G8 R8 A8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            0;  // in big endian: A8 R8 G8 B8
        #endif
        static const uint32_t r_mask =
            ((uint32_t)0xff) << r_shift;
        static const uint32_t g_mask =
            ((uint32_t)0xff) << g_shift;
        static const uint32_t b_mask =
            ((uint32_t)0xff) << b_shift;
        static const uint32_t a_mask =
            ((uint32_t)0xff) << a_shift;
        static const PixelFormatDescriptor INSTANCE
            (
                PixelFormat::B8G8R8A8,
                32,
                r_shift, g_shift, b_shift, a_shift,
                r_mask, g_mask, b_mask, a_mask
            );
        return INSTANCE;
    }

    template <>
    static const PixelFormatDescriptor& get<PixelFormat::R8G8B8>()
    {
        static const uint32_t r_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            0;  // R8 G8 B8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            16; // in big endian: B8 G8 R8
        #endif
        static const uint32_t g_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            8;  // R8 G8 B8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            8;  // B8 G8 R8
        #endif
        static const uint32_t b_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            16; // R8 G8 B8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            0;  // B8 G8 R8
        #endif
        static const uint32_t a_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            0; // no alpha
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            0; // no alpha
        #endif
        static const uint32_t r_mask =
            ((uint32_t)0xff) << r_shift;
        static const uint32_t g_mask =
            ((uint32_t)0xff) << g_shift;
        static const uint32_t b_mask =
            ((uint32_t)0xff) << b_shift;
        static const uint32_t a_mask =
            ((uint32_t)0x00) << a_shift;
        static const PixelFormatDescriptor INSTANCE
            (
                PixelFormat::R8G8B8,
                24,
                r_shift, g_shift, b_shift, a_shift,
                r_mask, g_mask, b_mask, a_mask
            );
        return INSTANCE;
    }

    template <>
    static const PixelFormatDescriptor& get<PixelFormat::R8G8B8A8>()
    {
        static const uint32_t r_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            0;  // R8 G8 B8 A8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            24; // in big endian: A8 B8 G8 R8
        #endif
        static const uint32_t g_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            8;  // R8 G8 B8 A8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            16; // in big endian: A8 B8 G8 R8
        #endif
        static const uint32_t b_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            16; // R8 G8 B8 A8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            8;  // in big endian: A8 B8 G8 R8
        #endif
        static const uint32_t a_shift =
        #if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
            24; // R8 G8 B8 A8
        #elif (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            0;  // in big endian: A8 B8 G8 R8
        #endif
        static const uint32_t r_mask =
            ((uint32_t)0xff) << r_shift;
        static const uint32_t g_mask =
            ((uint32_t)0xff) << g_shift;
        static const uint32_t b_mask =
            ((uint32_t)0xff) << b_shift;
        static const uint32_t a_mask =
            ((uint32_t)0xff) << a_shift;
        static const PixelFormatDescriptor INSTANCE
            (
                PixelFormat::R8G8B8A8,
                32,
                r_shift, g_shift, b_shift, a_shift,
                r_mask, g_mask, b_mask, a_mask
            );
        return INSTANCE;
    }

    /**
     * @brief
     *  Get the pixel descriptor for a pixel format.
     * @param pixelFormat
     *  the pixel format
     * @return
     *  the pixel descriptor for the pixel format
     */
    static const PixelFormatDescriptor& getDescriptor(PixelFormat pixelFormat)
    {
        switch (pixelFormat)
        {
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
        };
    }
};

} // namespace Ego

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

#include "egolib/Graphics/ColourDepth.hpp"

namespace Ego {

/**
 * @brief
 *  Enumeration of canonical identifiers for a pixel formats.
 * @author
 *  Michael Heilmann
 */
enum class PixelFormat {

    /**
     * @brief
     *  A pixel consisting of
     *      3 Bytes x0, x1, and x2
     *  at consecutive addresses in memory such that
     *      x0
     *  is at the lowest address and
     *      x2
     *  is at the highest address.
     *  The value of x0 denotes the value of the blue component,
     *  x1 the value of the green component,
     *  and x2 the value of the red component
     *  such that components r, g, b, and a, of the corresponding colour in normalized, real-valued RGBA space are computed by
     *  r = x[2] / 255, g = x[1] / 255, b = x[0] / 255, and a = 1.
     */
    B8G8R8,

    /**
     * @brief
     *  A pixel consisting of
     *      4 Bytes x0, x1, x2, and x3
     *  at consecutive addresses in memory such that
     *      x0
     *  is at the lowest address and
     *      x3
     *  is at the highest address.
     *  x0 denotes the value of the blue component,
     *  x1 the value of the green component,
     *  x2 the value of the red component,
     *  and x3 the value of the alpha component
     *  such that components r, g, b, and a, of the corresponding colour in normalized, real-valued RGBA space are computed by
     *  r = x[2] / 255, g = x[1] / 255, b = x[0] / 255, and a = x[3] / 255.
     */
    B8G8R8A8,

    /**
     * @brief
     *  A pixel consisting of
     *      3 Bytes x0, x1, and x2
     *  at consecutive addresses in memory such that
     *      x0
     *  is at the lowest address and
     *      x2
     *  is at the highest address.
     *  x0 denotes the value of the red component,
     *  x1 the value of the green component,
     *  and x2 the value of the blue component
     *  such that components r, g, b, and a, of the corresponding colour in normalized, real-valued RGBA space are computed by
     *  r = x[0] / 255, g = x[1] / 255, b = x[2] / 255, and a = 1.
     */
    R8G8B8,

    /**
     * @brief
     *  A pixel consisting of
     *      4 Bytes x0, x1, x2, and x3
     *  at consecutive addresses in memory such that
     *      x0
     *  is at the lowest address and
     *      x3
     *  is at the highest address.
     *  x0 denotes the value of the red component,
     *  x1 the value of the green component,
     *  and x2 the value of the blue component
     *  such that components r, g, b, and a, of the corresponding colour in normalized, real-valued RGBA space are computed by
     *  r = x[0] / 255, g = x[1] / 255, b = x[2] / 255, and a = x[3] / 255.
     * @remark
     *  This means, regardless of the Endianess of the system,
     *  provided a pointer <tt>char *p</tt> to the address of
     *  the pixel, then p[0] is the Byte of the red component,
     *  p[1] is the Byte of the green component, p[2] is the
     *  Bte of the blue component and p[3] is the Byte of the
     *  alpha component of this pixel format.
     *  <br/>
     *  To achieve this
     *  the bitmask for the red component is 0xff << 0,
     *  the bitmask for the green component is 0xff << 8,
     *  the bitmask for the blue component is 0xff << 16, and
     *  the bitmask for the alpha component is 0xff << 24
     *  on little-endian systems.
     *  <br/>
     *  On big-endian systems however,
     *  the bitmask for the red component is 0xff << 24,
     *  the bitmask for the green component is 0xff << 16
     *  the bitmask for the blue component is 0xff << 8, and
     *  the bitmask for the alpha component is 0xff << 0.
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
struct PixelFormatDescriptor {

private:
    /**
     * @brief
     *  The pixel format.
     */
    PixelFormat pixelFormat;

private:
    /**
     * @brief
     *  The mask for the alpha Bits.
     */
    uint32_t alphaMask;

    /**
     * @brief
     *  The mask for the blue Bits.
     */
    uint32_t blueMask;

    /**
     * @brief
     *  The mask for the green Bits.
     */
    uint32_t greenMask;

    /**
     * @brief
     *  The mask for the red Bits.
     */
    uint32_t redMask;

private:
    /**
     * @brief
     *  The shift for the alpha bits.
     */
    uint32_t alphaShift;

    /**
     * @brief
     *  The shift for the blue Bits.
     */
    uint32_t blueShift;

    /**
     * @brief
     *  The shift for the green Bits.
     */
    uint32_t greenShift;

    /**
     * @brief
     *  The shift for the red Bits.
     */
    uint32_t redShift;

private:
    /**
     * @brief
     *  The colour depth of this pixel format.
     */
    ColourDepth colourDepth;

protected:

    /**
     * @brief
     *  Construct this pixel descriptor.
     * @param pixelFormat
     *  the pixel format
     * @param redShift, greenShift, blueShift, alphaShift
     *  the shifts for Bits of the the red, green, blue and alpha components
     * @param redMask, greenMask, blueMask, alphaMask
     *  the masks for Bits of the red, green, blue and alpha components
     * @param colourDepth
     *  the colour depth of this pixel format
     */
    PixelFormatDescriptor(PixelFormat pixelFormat,
                          uint32_t redShift, uint32_t greenShift,
                          uint32_t blueShift, uint32_t alphaShift,
                          uint32_t redMask, uint32_t greenMask,
                          uint32_t blueMask, uint32_t alphaMask,
                          const ColourDepth& colourDepth);

public:
    /**
     * @brief
     *  Get the shift of the alpha Bits.
     * @return
     *  the shift of the alpha Bits
     */
    uint32_t getAlphaShift() const;

    /**
     * @brief
     *  Get the shift of the blue Bits.
     * @return
     *  the shift of the blue Bits
     */
    uint32_t getBlueShift() const;

    /**
     * @brief
     *  Get the shift of the green Bits.
     * @return
     *  the shift of the green Bits
     */
    uint32_t getGreenShift() const;

    /**
     * @brief
     *  Get the shift of the red Bits.
     * @return
     *  the shift of the red Bits
     */
    uint32_t getRedShift() const;

public:
    /**
     * @brief
     *  The mask for the alpha Bits.
     * @return
     *  the mask of the alpha Bits
     */
    uint32_t getAlphaMask() const;

    /**
     * @brief
     *   Get the mask for the blue Bits.
     * @return
     *  the mask of the blue Bits
     */
    uint32_t getBlueMask() const;

    /**
     * @brief
     *  The mask for the green Bits.
     * @return
     *  the mask of the green Bits
     */
    uint32_t getGreenMask() const;

    /**
     * @brief
     *  Get the mask for the red Bits.
     * @return
     *  the mask of the red Bits
     */
    uint32_t getRedMask() const;

public:
    /**
     * @brief
     *  Get the colour depth of this pixel format.
     * @return
     *  the colour depth of this pixel format
     */
    const ColourDepth& getColourDepth() const;

public:
    /**
     * @brief
     *  Get the pixel format.
     * @return
     *  the pixel format
     */
    PixelFormat getPixelFormat() const;

    template<PixelFormat _PixelFormat>
    static const PixelFormatDescriptor& get();

    /**
     * @brief
     *  Get the pixel format descriptor for a pixel format.
     * @param pixelFormat
     *  the pixel format
     * @return
     *  the pixel format descriptor for the pixel format
     */
    static const PixelFormatDescriptor& get(PixelFormat pixelFormat);

};

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::B8G8R8>();

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::B8G8R8A8>();

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::R8G8B8>();

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::R8G8B8A8>();

} // namespace Ego

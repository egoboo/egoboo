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

/// @file egolib/Graphics/PixelFormat.hpp
/// @brief Pixel formats and pixel format descriptors.
/// @author Michael Heilmann


#pragma once

#include "egolib/Graphics/ColourDepth.hpp"

namespace Ego {

/// @brief Enumeration of canonical identifiers for a pixel formats.
enum class PixelFormat
{
    /// @brief
    /// A pixel consists of 3 Bytes \f$x_0\f$, \f$x_1\f$, and \f$x_2\f$
    /// at consecutive addresses in memory such that if \f$a\f$ is the
    /// address of the pixel then
    /// the address of \f$x_0\f$ is \f$a(x_0) = a + 0\f$,
    /// the address of \f$x_1\f$ is \f$a(x_1) = a + 1\f$, and
    /// the address of \f$x_2\f$ is \f$a(x_2) = a + 2\f$.
    /// The value of \f$x_0\f$ denotes the value of the blue component,
    /// the value of \f$x_1\f$ the value of the green component, and
    /// the value of \f$x_2\f$ the value of the red component.
    /// All component values are within the range of \f$[0,255]\f$
    /// where \f$0\f$ indicates the minimum intensity and \f$255\f$ the maximum intensity.
    B8G8R8,

    /// @brief
    /// A pixel consists of 4 Bytes \f$x_0\f$, \f$x_1\f$, \f$x_2\f$, and \f$x_3\f$
    /// at consecutive addresses in memory such that if \f$a\f$ is the
    /// address of the pixels then
    /// the address of \f$x_0\f$ is \f$a(x_0) = a + 0\f$,
    /// the address of \f$x_1\f$ is \f$a(x_1) = a + 1\f$,
    /// the address of \f$x_2\f$ is \f$a(x_2) = a + 2\f$, and
    /// the address of \f$x_3\f$ is \f$a(x_3) = a + 3\f$.
    /// The value of \f$x_0\f$ denotes the value of the blue component,
    /// the value of \f$x_1\f$ the value of the green component,
    /// the value of \f$x_2\f$ the value of the red component, and
    /// the value of \f$x_3\f$ the value of the alpha component.
    /// All component values are within the range of \f$[0,255]\f$
    /// where \f$0\f$ indicates the minimum intensity and \f$255\f$ the maximum intensity.
    B8G8R8A8,

    /// @brief
    /// A pixel consists of 3 Bytes \f$x_0\f$, \f$x_1\f$, and \f$x_2\f$
    /// at consecutive addresses in memory such that if \f$a\f$ is the
    /// address of the pixel then
    /// the address of \f$x_0\f$ is \f$a(x_0) = a + 0\f$,
    /// the address of \f$x_1\f$ is \f$a(x_1) = a + 1\f$, and
    /// the address of \f$x_2\f$ is \f$a(x_2) = a + 2\f$.
    /// The value of \f$x_0\f$ denotes the value of the red component,
    /// the value of \f$x_1\f$ the value of the green component, and
    /// the value of \f$x_2\f$ the value of the blue component.
    /// All component values are within the range of \f$[0,255]\f$
    /// where \f$0\f$ indicates the minimum intensity and \f$255\f$ the maximum intensity.
    R8G8B8,

    /// @brief
    /// A pixel consists of 4 Bytes \f$x_0\f$, \f$x_1\f$, \f$x_2\f$, and \f$x_3\f$
    /// at consecutive addresses in memory such that if \f$a\f$ is the
    /// address of the pixels then
    /// the address of \f$x_0\f$ is \f$a(x_0) = a + 0\f$,
    /// the address of \f$x_1\f$ is \f$a(x_1) = a + 1\f$,
    /// the address of \f$x_2\f$ is \f$a(x_2) = a + 2\f$, and
    /// the address of \f$x_3\f$ is \f$a(x_3) = a + 3\f$.
    /// The value of \f$x_0\f$ denotes the value of the red component,
    /// the value of \f$x_1\f$ the value of the green component,
    /// the value of \f$x_2\f$ the value of the blue component, and
    /// the value of \f$x_3\f$ the value of the alpha component.
    /// All component values are within the range of \f$[0,255]\f$
    /// where \f$0\f$ indicates the minimum intensity and \f$255\f$ the maximum intensity.
    R8G8B8A8,

    /// @brief
    /// A pixel consists of 4 Bytes \f$x_0\f$, \f$x_1\f$, \f$x_2\f$, and \f$x_3\f$
    /// at consecutive addresses in memory such that if \f$a\f$ is the
    /// address of the pixels then
    /// the address of \f$x_0\f$ is \f$a(x_0) = a + 0\f$,
    /// the address of \f$x_1\f$ is \f$a(x_1) = a + 1\f$,
    /// the address of \f$x_2\f$ is \f$a(x_2) = a + 2\f$, and
    /// the address of \f$x_3\f$ is \f$a(x_3) = a + 3\f$.
    /// The value of \f$x_0\f$ denotes the value of alpha red component,
    /// the value of \f$x_1\f$ the value of the blue component,
    /// the value of \f$x_2\f$ the value of the green component, and
    /// the value of \f$x_3\f$ the value of the red component.
    /// All component values are within the range of \f$[0,255]\f$
    /// where \f$0\f$ indicates the minimum intensity and \f$255\f$ the maximum intensity.
    A8B8G8R8,
};

/// @brief A pixel format descriptor suitable to describe the pixel
/// formats as specified by the Ego::PixelFormat enumeration.
/// @remark The bitmasks are given w.r.t. the host byte order.
class PixelFormatDescriptor
{
private:
    /// @brief The pixel format.
    PixelFormat pixelFormat;

private:
    /// @brief The mask for the alpha Bits.
    uint32_t alphaMask;

    /// @brief The mask for the blue Bits.
    uint32_t blueMask;

    /// @brief The mask for the green Bits.
    uint32_t greenMask;

    /// @brief The mask for the red Bits.
    uint32_t redMask;

private:
    /// @brief The shift for the alpha bits.
    uint32_t alphaShift;

    /// @brief The shift for the blue Bits.
    uint32_t blueShift;

    /// @brief The shift for the green Bits.
    uint32_t greenShift;

    /// @brief The shift for the red Bits.
    uint32_t redShift;

private:
    /// @brief The colour depth of this pixel format.
    ColourDepth colourDepth;

protected:
    /// @brief Construct this pixel descriptor.
    /// @param pixelFormat the pixel format
    /// @param redShift, greenShift, blueShift, alphaShift
    /// the shifts for Bits of the the red, green, blue and alpha components (w.r.t. the host Byte order)
    /// @param redMask, greenMask, blueMask, alphaMask
    /// the masks for Bits of the red, green, blue and alpha components (w.r.t. the host Byte order)
    /// @param colourDepth the colour depth of this pixel format
    PixelFormatDescriptor(PixelFormat pixelFormat,
                          uint32_t redShift, uint32_t greenShift,
                          uint32_t blueShift, uint32_t alphaShift,
                          uint32_t redMask, uint32_t greenMask,
                          uint32_t blueMask, uint32_t alphaMask,
                          const ColourDepth& colourDepth);

public:
    /// @brief Get the shift of the alpha Bits (w.r.t. host Byte order).
    /// @return the shift of the alpha Bits (w.r.t. the host Byte order)
    uint32_t getAlphaShift() const;

    /// @brief Get the shift of the blue Bits (w.r.t. host Byte order).
    /// @return the shift of the blue Bits (w.r.t. host Byte order)
    uint32_t getBlueShift() const;

    /// @brief Get the shift of the green Bits (w.r.t. host Byte order).
    /// @return the shift of the green Bits (w.r.t. host Byte order)
    uint32_t getGreenShift() const;

    /// @brief Get the shift of the red Bits (w.r.t. host Byte order).
    /// @return the shift of the red Bits (w.w.r.t. host Byte order)
    uint32_t getRedShift() const;

public:
    /// @brief The mask for the alpha Bits (w.r.t. the host Byte order).
    /// @return the mask of the alpha Bits (w.r.t. the host Byte order)
    uint32_t getAlphaMask() const;

    /// @brief Get the mask for the blue Bits (w.r.t. the host Byte order).
    /// @return the mask of the blue Bits (w.r.t. the host Byte order)
    uint32_t getBlueMask() const;

    /// @brief The mask for the green Bits (w.r.t. the host Byte order).
    /// @return the mask of the green Bits (w.r.t. the host Byte order)
    uint32_t getGreenMask() const;

    /// @brief Get the mask for the red Bits (w.r.t. the host Byte order).
    /// @return the mask of the red Bits (w.r.t. the host Byte order)
    uint32_t getRedMask() const;

public:
    /// @brief Get the colour depth of this pixel format.
    /// @return the colour depth of this pixel format
    const ColourDepth& getColourDepth() const;

public:
    /// @brief Get the pixel format.
    /// @return the pixel format
    PixelFormat getPixelFormat() const;

    template<PixelFormat _PixelFormat>
    static const PixelFormatDescriptor& get();

    /// @brief Get the pixel format descriptor for a pixel format.
    /// @param pixelFormat the pixel format
    /// @return the pixel format descriptor for the pixel format
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

template <>
const PixelFormatDescriptor& PixelFormatDescriptor::get<PixelFormat::A8B8G8R8>();

} // namespace Ego

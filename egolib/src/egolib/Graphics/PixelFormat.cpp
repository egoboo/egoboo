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

/// @file egolib/Graphics/PixelFormat.cpp
/// @brief Pixel formats and pixel format descriptors.
/// @author Michael Heilmann

#include "egolib/Graphics/PixelFormat.hpp"

namespace Ego {

pixel_descriptor::pixel_descriptor(idlib::pixel_format pixel_format,
                                   const idlib::pixel_component_descriptor& red,
                                   const idlib::pixel_component_descriptor& green,
                                   const idlib::pixel_component_descriptor& blue,
                                   const idlib::pixel_component_descriptor& alpha,
                                   const idlib::rgba_depth& color_depth)
    : m_pixel_format(pixel_format),
      m_red(red), m_green(green), m_blue(blue), m_alpha(alpha),
      m_color_depth(color_depth)
{}

const idlib::pixel_component_descriptor& pixel_descriptor::get_alpha() const
{
    return m_alpha;
}

const idlib::pixel_component_descriptor& pixel_descriptor::get_blue() const
{
    return m_blue;
}

const idlib::pixel_component_descriptor& pixel_descriptor::get_green() const
{
    return m_green;
}

const idlib::pixel_component_descriptor& pixel_descriptor::get_red() const
{
    return m_red;
}

const idlib::rgba_depth& pixel_descriptor::get_color_depth() const
{
    return m_color_depth;
}

idlib::pixel_format pixel_descriptor::get_pixel_format() const
{
    return m_pixel_format;
}

const pixel_descriptor& pixel_descriptor::get(idlib::pixel_format pixel_format)
{
    switch (pixel_format)
    {
        case idlib::pixel_format::B8G8R8:
        {
            return get<idlib::pixel_format::B8G8R8>();
        }
        break;
        case idlib::pixel_format::B8G8R8A8:
        {
            return get<idlib::pixel_format::B8G8R8A8>();
        }
        break;
        case idlib::pixel_format::R8G8B8:
        {
            return get<idlib::pixel_format::R8G8B8>();
        }
        break;
        case idlib::pixel_format::R8G8B8A8:
        {
            return get<idlib::pixel_format::R8G8B8A8>();
        }
        break;
        default:
        {
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
        }
    };
}

static constexpr uint32_t shift(uint32_t little_endian, uint32_t big_endian)
{
    if (idlib::get_byte_order() == idlib::byte_order::little_endian)
        return little_endian;
    else if (idlib::get_byte_order() == idlib::byte_order::big_endian)
        return big_endian;
}

static constexpr uint32_t mask(uint32_t shift)
{
    return ((uint32_t)0xff) << shift;
}

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::B8G8R8>()
{
    static const uint32_t blueShift =
        shift(0, 16); // 0x00.00.ff, 0xff.00.00
    static const uint32_t greenShift =
        shift(8, 8);  // 0x00.ff.00, 0xff.00.00
    static const uint32_t redShift =
        shift(16, 0); // 0xff.00.00, 0x00.00.ff
    static const uint32_t alphaShift =
        shift(0, 0);  // no alpha

    static const uint32_t redMask =
        mask(redShift);
    static const uint32_t greenMask =
        mask(greenShift);
    static const uint32_t blueMask =
        mask(blueShift);
    static const uint32_t alphaMask =
        mask(alphaShift);

    static const pixel_descriptor INSTANCE
    (
        idlib::pixel_format::R8G8B8A8,
        { idlib::pixel_component_semantics::RED, redMask, redShift },
        { idlib::pixel_component_semantics::GREEN, greenMask, greenShift },
        { idlib::pixel_component_semantics::BLUE, blueMask, blueShift },
        { idlib::pixel_component_semantics::ALPHA, alphaMask, alphaShift },
        idlib::rgba_depth({ 8, 8, 8 }, 0)
    );
    return INSTANCE;
}

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::B8G8R8A8>()
{
    static const uint32_t blueShift =
        shift(0, 24); // 0x00.00.00.ff, 0xff.00.00.00
    static const uint32_t greenShift =
        shift(8, 16); // 0x00.00.ff.00, 0x00.ff.00.00
    static const uint32_t redShift =
        shift(16, 8); // 0x00.ff.00.00, 0x00.00.ff.00
    static const uint32_t alphaShift =
        shift(24, 0); // 0xff.00.00.00, 0x00.00.00.ff

    static const uint32_t redMask =
        mask(redShift);
    static const uint32_t greenMask =
        mask(greenShift);
    static const uint32_t blueMask =
        mask(blueShift);
    static const uint32_t alphaMask =
        mask(alphaShift);

    static const pixel_descriptor INSTANCE
    (
        idlib::pixel_format::B8G8R8A8,
        { idlib::pixel_component_semantics::RED, redMask, redShift },
        { idlib::pixel_component_semantics::GREEN, greenMask, greenShift },
        { idlib::pixel_component_semantics::BLUE, blueMask, blueShift },
        { idlib::pixel_component_semantics::ALPHA, alphaMask, alphaShift },
        idlib::rgba_depth({ 8, 8, 8 }, 8)
    );
    return INSTANCE;
}

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::R8G8B8>()
{
    static const uint32_t redShift =
        shift(0, 16); // 0x00.00.ff, 0xff.00.00
    static const uint32_t greenShift =
        shift(8, 8);  // 0x00.ff.00, 0x00.ff.00
    static const uint32_t blueShift =
        shift(16, 0); // 0xff.00.00, 0x00.00.ff
    static const uint32_t alphaShift =
        shift(0, 0);  // no alpha

    static const uint32_t redMask =
        ((uint32_t)0xff) << redShift;
    static const uint32_t greenMask =
        ((uint32_t)0xff) << greenShift;
    static const uint32_t blueMask =
        ((uint32_t)0xff) << blueShift;
    static const uint32_t alphaMask =
        ((uint32_t)0x00) << alphaShift;
    static const pixel_descriptor INSTANCE
    (
        idlib::pixel_format::R8G8B8,
        { idlib::pixel_component_semantics::RED, redMask, redShift },
        { idlib::pixel_component_semantics::GREEN, greenMask, greenShift },
        { idlib::pixel_component_semantics::BLUE, blueMask, blueShift },
        { idlib::pixel_component_semantics::ALPHA, alphaMask, alphaShift },
        idlib::rgba_depth({ 8, 8, 8 }, 0)
    );
    return INSTANCE;
}

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::R8G8B8A8>()
{
    static const uint32_t redShift =
        shift(0, 24); // 0x00.00.00.ff, 0xff.00.00.00
    static const uint32_t greenShift =
        shift(8, 16); // 0x00.00.ff.00, 0x00.ff.00.00
    static const uint32_t blueShift =
        shift(16, 8); // 0x00.ff.00.00, 0x00.00.ff.00
    static const uint32_t alphaShift =
        shift(24, 0); // 0xff.00.00.00, 0x00.00.00.ff

    static const uint32_t redMask =
        mask(redShift);
    static const uint32_t greenMask =
        mask(greenShift);
    static const uint32_t blueMask =
        mask(blueShift);
    static const uint32_t alphaMask =
        mask(alphaShift);

    static const pixel_descriptor INSTANCE
    (
        idlib::pixel_format::R8G8B8A8,
        { idlib::pixel_component_semantics::RED, redMask, redShift },
        { idlib::pixel_component_semantics::GREEN, greenMask, greenShift },
        { idlib::pixel_component_semantics::BLUE, blueMask, blueShift },
        { idlib::pixel_component_semantics::ALPHA, alphaMask, alphaShift },
        idlib::rgba_depth({ 8, 8, 8 }, 8)
    );
    return INSTANCE;
}

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::A8B8G8R8>()
{
    static const uint32_t alphaShift =
        shift(0, 24); // 0x00.00.00.ff, 0xff.00.00.00
    static const uint32_t blueShift =
        shift(8, 16); // 0x00.00.ff.00, 0x00.ff.00.00
    static const uint32_t greenShift =
        shift(16, 8); // 0x00.ff.00.00, 0x00.00.ff.00
    static const uint32_t redShift =
        shift(24, 0); // 0xff.00.00.00, 0x00.00.00.ff

    static const uint32_t redMask =
        mask(redShift);
    static const uint32_t greenMask =
        mask(greenShift);
    static const uint32_t blueMask =
        mask(blueShift);
    static const uint32_t alphaMask =
        mask(alphaShift);

    static const pixel_descriptor INSTANCE
    (
        idlib::pixel_format::A8B8G8R8,
        { idlib::pixel_component_semantics::RED, redMask, redShift },
        { idlib::pixel_component_semantics::GREEN, greenMask, greenShift },
        { idlib::pixel_component_semantics::BLUE, blueMask, blueShift },
        { idlib::pixel_component_semantics::ALPHA, alphaMask, alphaShift },
        idlib::rgba_depth({ 8, 8, 8 }, 8)
    );
    return INSTANCE;
}

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::A8R8G8B8>()
{
    static const uint32_t alphaShift =
        shift(0, 24); // 0x00.00.00.ff, 0xff.00.00.00
    static const uint32_t redShift =
        shift(8, 16); // 0x00.00.ff.00, 0x00.ff.00.00
    static const uint32_t greenShift =
        shift(16, 8); // 0x00.ff.00.00, 0x00.00.ff.00
    static const uint32_t blueShift =
        shift(24, 0); // 0xff.00.00.00, 0x00.00.00.ff

    static const uint32_t redMask =
        mask(redShift);
    static const uint32_t greenMask =
        mask(greenShift);
    static const uint32_t blueMask =
        mask(blueShift);
    static const uint32_t alphaMask =
        mask(alphaShift);

    static const pixel_descriptor INSTANCE
    (
        idlib::pixel_format::A8R8G8B8,
        { idlib::pixel_component_semantics::RED, redMask, redShift },
        { idlib::pixel_component_semantics::GREEN, greenMask, greenShift },
        { idlib::pixel_component_semantics::BLUE, blueMask, blueShift },
        { idlib::pixel_component_semantics::ALPHA, alphaMask, alphaShift },
        idlib::rgba_depth({ 8, 8, 8 }, 8)
    );
    return INSTANCE;
}

} // namespace Ego

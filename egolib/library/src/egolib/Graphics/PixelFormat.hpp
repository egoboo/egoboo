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

#include "egolib/integrations/video.hpp"

namespace Ego {

/// @brief A pixel format descriptor suitable to describe the pixel
/// formats as specified by the Ego::PixelFormat enumeration.
/// @remark The bitmasks are given w.r.t. the host byte order.
class pixel_descriptor
{
private:
    /// @brief The pixel format.
    idlib::pixel_format m_pixel_format;

    /// @brief The pixel component descriptor for the alpha component.
    idlib::pixel_component_descriptor m_alpha;

    /// @brief The pixel component descriptor for the blue component.
    idlib::pixel_component_descriptor m_blue;

    /// @brief The pixel component descriptor for the green component.
    idlib::pixel_component_descriptor m_green;

    /// @brief The pixel component descriptor for the red component.
    idlib::pixel_component_descriptor m_red;

    /// @brief The colour depth of this pixel format.
    idlib::rgba_depth m_color_depth;

protected:
    /// @brief Construct this pixel descriptor.
    /// @param pixel_format the pixel format
    /// @param red, green, blue, alpha the pixel component descriptors of the red, green, blue, and alpha components
    /// @param color_depth the color depth
    pixel_descriptor(idlib::pixel_format pixel_format,
                     const idlib::pixel_component_descriptor& red,
                     const idlib::pixel_component_descriptor& green,
                     const idlib::pixel_component_descriptor& blue,
                     const idlib::pixel_component_descriptor& alpha,
                     const idlib::rgba_depth& color_depth);

public:
    /// @brief Get the pixel component descriptor of the alpha component.
    /// @return the pixel component descriptor of the alpha component
    const idlib::pixel_component_descriptor& get_alpha() const;

    /// @brief Get the pixel component descriptor of the blue component.
    /// @return the pixel component descriptor of the blue component
    const idlib::pixel_component_descriptor& get_blue() const;

    /// @brief Get the pixel component descriptor of the green component.
    /// @return the pixel component descriptor of the green component
    const idlib::pixel_component_descriptor& get_green() const;

    /// @brief Get the pixel component descriptor of the red component.
    /// @return the pixel component descriptor of the red component
    const idlib::pixel_component_descriptor& get_red() const;

    /// @brief Get the colour depth of this pixel format.
    /// @return the colour depth of this pixel format
    const idlib::rgba_depth& get_color_depth() const;

    /// @brief Get the pixel format.
    /// @return the pixel format
    idlib::pixel_format get_pixel_format() const;

    template<idlib::pixel_format pixel_format>
    static const pixel_descriptor& get();

    /// @brief Get the pixel format descriptor for a pixel format.
    /// @param pixelFormat the pixel format
    /// @return the pixel format descriptor for the pixel format
    static const pixel_descriptor& get(idlib::pixel_format pixel_format);

};

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::B8G8R8>();

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::B8G8R8A8>();

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::R8G8B8>();

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::R8G8B8A8>();

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::A8B8G8R8>();

template <>
const pixel_descriptor& pixel_descriptor::get<idlib::pixel_format::A8R8G8B8>();

} // namespace Ego

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

/// @file egolib/Renderer/TextureSampler.hpp
/// @brief A texture sampler.
/// @author Michael Heilmann

#pragma once

#include "egolib/Renderer/TextureFilter.hpp"
#include "egolib/Renderer/TextureAddressMode.hpp"

namespace Ego {

/// @brief A texture sampler is part of the state of a texture (Ego::Texture).
struct TextureSampler
{
private:
    /// @brief The minification filter.
    TextureFilter minFilter;

    /// @brief The magnification filter.
    TextureFilter magFilter;

    /// @brief The mip map filter.
    TextureFilter mipMapFilter;

    /// @brief The addressing mode along the s-axis.
    TextureAddressMode addressModeS;

    /// @brief The addressing mode along the t-axis.
    TextureAddressMode addressModeT;

    /// @brief The levels of anistropic filtering.
    float anisotropyLevel;

public:
    /// @brief Construct this texture sampler.
    /// @param minFilter the minification filter
    /// @param magFilter the magnification filter
    /// @param mipMapFilter the mipmap filter
    /// @param addressModeS the address mode for the s-axis
    /// @param addressModeT the address mode for the t-axis
    /// @param anisotropyLevels the levels of anisotropic filtering
    /// When the sampler is applied, this value is clamped to \f$[min,max]\f$ where
    /// \f$min\f$ is the minimum level and \f$max\f$ is the maximum level of anistropic filtering supported.
    /// The minimum level means anisotropic filtering is off i.e. isotropic filtering is performed.
    TextureSampler(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMapFilter,
                   TextureAddressMode addressModeS, TextureAddressMode addressModeT, float anisotropyLevel);

    /// @brief Copy-construct this texture sampler from another texture sampler.
    /// @param other the other texture sampler
    TextureSampler(const TextureSampler& other);

public:
    /// @brief Assign this texture sampler from another texture sampler.
    /// @param other the other texture sampler
    /// @return this texture sampler
    const TextureSampler& operator=(const TextureSampler& other);

public:
    /// @brief Get the minification filter.
    /// @return the minification filter
    TextureFilter getMinFilter() const;

    /// @brief Get the magnification filter.
    /// @return the magnification filter
    TextureFilter getMagFilter() const;

    /// @brief Get the mipmap filter.
    /// @return the mipmap filter
    TextureFilter getMipMapFilter() const;

    /// @brief Get the address mode for the s-axis.
    /// @return the address mode for the s-axis
    TextureAddressMode getAddressModeS() const;

    /// @brief Get the address mode for the t-axis.
    /// @return the address mode for the t-axis
    TextureAddressMode getAddressModeT() const;

    /// @brief Get the levels of anistropic filtering.
    /// @return the levels of anisotropic filtering
    float getAnisotropyLevels() const;

}; // struct TextureSampler

} // namespace Ego

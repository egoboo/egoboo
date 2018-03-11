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

/// @file egolib/Renderer/RendererInfo.hpp
/// @brief Base of information on a renderer.
/// @author Michael Heilmann

#pragma once

#include "egolib/Log/_Include.hpp"
#include "egolib/integrations/video.hpp"

namespace Ego {

/// @brief Base of information on a renderer.
class RendererInfo
{
protected:
    /// @brief If anisotropy is desired.
    bool m_isAnisotropyDesired;
    
    /// @brief The desired anisotropy.
    float m_desiredAnisotropy;

    /// @brief The desired minimization filter.
    idlib::texture_filter_method m_desiredMinimizationFilter;

    /// @brief The desired maximization filter.
    idlib::texture_filter_method m_desiredMaximizationFilter;

    /// @brief The desired MipMap filter.
    idlib::texture_filter_method m_desiredMipMapFilter;

    /// @brief List of connections.
    std::vector<idlib::connection> m_connections;

public:
    /// @brief Construct this renderer information.
    RendererInfo();

    /// @brief Destruct this renderer information.
    virtual ~RendererInfo();

    RendererInfo(const RendererInfo&) = delete;
    RendererInfo& operator=(const RendererInfo&) = delete;

public:
    /// @brief Get the name of the renderer.
    /// @return the name of the renderer
    virtual std::string getRenderer() const = 0;

    /// @brief Get the name of the vendor of the renderer.
    /// @return the name of the vendor of the renderer
    virtual std::string getVendor() const = 0;

    /// @brief Get the version of the renderer.
    /// @return the version of the renderer
    virtual std::string getVersion() const = 0;

    /// @brief Get if anisotropy is supported.
    /// @return @a true if anisotropy is supported, @a false otherwise
    virtual bool isAnisotropySupported() const noexcept = 0;

    /// @brief Get the minimum supported anisotropy.
    /// @return the minimum supported anisotropy if anisotropy is supported or not a number
    /// @remark The minimum supported anisotropy is 1.0f if anisotropy is supported.
    virtual float getMinimumSupportedAnisotropy() const noexcept = 0;

    /// @brief Get the maximum supported anisotropy.
    /// @return the maximum supported anisotropy if anisotropy is supported or not a number
    /// @remark The maximum supported anisotropy is greater than or equal to the minimum supported anisotropy.
    virtual float getMaximumSupportedAnisotropy() const noexcept = 0;


    /// @brief Get if an anisotropy is desired.
    /// @return @a true if anisotropy is desired
    bool isAnisotropyDesired() const noexcept;
    idlib::signal<void()> AnisotropyDesiredChanged;

    /// @brief Get the desired anisotropy.
    /// @return the desired anisotropy
    float getDesiredAnisotropy() const noexcept;
    idlib::signal<void()> DesiredAnisotropyChanged;

    /// @brief Get the desired minimization filter.
    /// @return the desired minimization filter
    idlib::texture_filter_method getDesiredMinimizationFilter() const noexcept;
    idlib::signal<void()> DesiredMinimizationFilterChanged;

    /// @brief Get the desired maximization filter.
    /// @return the desired maximization filter
    idlib::texture_filter_method getDesiredMaximizationFilter() const noexcept;
    idlib::signal<void()> DesiredMaximizationFilterChanged;

    /// @brief Get the desired mipmap filter.
    /// @return the desired mipmap filter
    idlib::texture_filter_method getDesiredMipMapFilter() const noexcept;
    idlib::signal<void()> DesiredMipMapFilterChanged;

    /// @brief Get the maximum texture size.
    /// @return the maximum texture size. Always positive.
    virtual int getMaximumTextureSize() const noexcept = 0;

    /// @brief Get the renderer information as multi-line human-readly string.
    /// @return the string
    virtual std::string toString() const = 0;

}; // class RendererInfo

} // namespace Ego

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

/// @file egolib/Renderer/Texture.hpp
/// @brief Common interface of all texture object encapsulations.
/// @author Michael Heilmann

#pragma once

#include "egolib/Image/Image.hpp"
#include <string>
#include <memory>

namespace Ego {

class Texture
{
protected:
    /// @brief The type of this texture.
    idlib::texture_type m_type;

    /// @brief The sampler of this texture.
    idlib::texture_sampler m_sampler;

    /// @brief The name of the texture.
    /// @remark
    /// Usually the pathname of the texture descriptor file,
    /// for dynamic textures and special textures a name that can not be used as a pathname.
    std::string m_name;

    /// @brief The width, in pixels, of the source of this texture.
    /// @remark This value might differ for technical reasons from the width of the texture.
    int m_sourceWidth;

    /// @brief The height, in pixels, of the source of this texture.
    /// @remark This value might differ for technical reasons from the height of the texture.
    int m_sourceHeight;

    /// @brief The width, in pixels, of the the texture.
    int m_width;

    /// @brief The height, in pixels, of the texture.
    int m_height;

    /// @brief @a true if this texture has an alpha component, @a false otherwise.
    bool m_hasAlpha;

public:
    /// @brief A pointer to the source of the texture if available, a null pointer otherwise.
    std::shared_ptr<SDL_Surface> m_source;

    /// @brief If the pixels are dirty.
    bool m_arePixelsDirty;

    /// @brief If the sampler is dirty.
    bool m_isSamplerDirty;

protected:
    /// @brief Construct this texture.
    /// @remark Intentionally protected.
    Texture(const std::string& name, idlib::texture_type type, const idlib::texture_sampler& sampler,
            int width, int height, int sourceWidth, int sourceHeight,
			std::shared_ptr<SDL_Surface> source,
            bool hasAlpha);

public:
    /// @brief Destruct this texture.
    virtual ~Texture();

    /// @brief Get the type of this texture.
    /// @return the type of this texture
    idlib::texture_type getType() const;

    /// @brief Get the mipmap filter of this texture.
    /// @return the mipmap filter
    idlib::texture_filter_method getMipMapFilter() const;

    /// @brief Set the mipmap filter of this texture.
    /// @param mipMapFilter the mipmap filter
    void setMipMapFilter(idlib::texture_filter_method minFilter);

    /// @brief Get the minification filter of this texture.
    /// @return the minification filter
    idlib::texture_filter_method getMinFilter() const;

    /// @brief Set the minification filter of this texture.
    /// @param minFilter the minification filter
    void setMinFilter(idlib::texture_filter_method minFilter);

    /// @brief Get the magnification filter of this texture.
    /// @return the magnification filter
    idlib::texture_filter_method getMagFilter() const;

    /// @brief Set the magnification filter of this texture.
    /// @param magFilter the magnification filter
    void setMagFilter(idlib::texture_filter_method magFilter);

    /// @brief Get the address mode of this texture along the s-axis.
    /// @return the address mode of this texture along the s-axis.
    idlib::texture_address_mode getAddressModeS() const;

    /// @brief Set the address mode of this texture along the s-axis.
    /// @param addressMode the address mode of this texture along the s-axis
    void setAddressModeS(idlib::texture_address_mode addressModeS);

    /// @brief Get the address mode of this texture along the t-axis.
    /// @return the address mode of this texture along the t-axis.
    idlib::texture_address_mode getAddressModeT() const;

    /// @brief Set the address mode of this texture along the t-axis.
    /// @param addressMode the address mode of this texture along the t-axis
    void setAddressModeT(idlib::texture_address_mode addressModeT);

    /// @brief Get the width, in pixels, of this texture.
    /// @return the width, in pixels of this texture
    int getWidth() const;

    /// @brief Get the height, in pixels, of this texture.
    /// @return the height, in pixels of this texture
    int getHeight() const;

    /// @brief Get the width, in pixels, of the source of this texture.
    /// @return the width, in pixels, of the source of this texture
    /// @remark This value might differ for technical reasons from the width of the texture.
    int getSourceWidth() const;

    /// @brief Get the height, in pixels, of the source of this texture.
    /// @return the height, in pixels, of the source of this texture
    /// @remark This value might differ for technical reasons from the height of the texture.
    int getSourceHeight() const;

    /// @brief Get if this texture has an alpha component.
    /// @return @a true if this texture has an alpha component, @a false otherwise
    bool hasAlpha() const;

    /**
     * @brief
     *  Set the name of this texture.
     * @param name
     *  the name
     */
    void setName(const std::string& name);

    /**
     * @brief
     *  Get the name of this texture.
     * @return
     *  the name of this texture
     */
    const std::string& getName() const;

    /// @brief Get the texture sampler.
    /// @return the texture sampler
    const idlib::texture_sampler& getSampler() const;

    /// @brief Get the source.
    /// @return the source
    std::shared_ptr<SDL_Surface> getSource() const;

public:
	virtual bool load(const std::string& name, const std::shared_ptr<SDL_Surface>& surface) = 0;
	virtual bool load(const std::shared_ptr<SDL_Surface>& image) = 0;

	/**
	 * @brief
	 *  Delete backing image, delete OpenGL ID, assign OpenGL ID of the error texture, assign no backing image.
	 * @param self
	 *  this texture
	 */
	virtual void release() = 0;

    /**
     * @brief
     *  Get if the default texture data is uploaded to this texture.
     * @return
     *  @a true if the default texture data is uploaded to this texture, @a false otherwise
     */
    virtual bool isDefault() const = 0;

}; // struct Texture

} // namespace Ego

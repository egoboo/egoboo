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

/// @file   egolib/Renderer/Texture.hpp
/// @brief  Common interface of all texture object encapsulations.
/// @author Michael Heilmann

#pragma once

#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Renderer/TextureFilter.hpp"
#include "egolib/Renderer/TextureAddressMode.hpp"
#include "egolib/Renderer/TextureType.hpp"
#include "egolib/Renderer/TextureSampler.hpp"

#include "egolib/typedef.h"

namespace Ego {

class Texture {
protected:
    using String = std::string;
    template <typename T> using SharedPtr = std::shared_ptr<T>;

protected:

    /**
     * @brief
     *  The type of this texture.
     */
    TextureType _type;

    /**
     * @brief
     *  The minification filter of this texture.
     */
    TextureFilter _minFilter;
    
    /**
     * @brief
     *  The magnification filter of this texture.
     */
    TextureFilter _magFilter;
    
    /**
     * @brief
     *  The mipmap filter of this texture.
     */
    TextureFilter _mipMapFilter;

    /**
     * @brief
     *  The address mode along the s-axis.
     */
    TextureAddressMode _addressModeS;

    /**
     * @brief
     *  The texture address mode along the t-axis.
     */
    TextureAddressMode _addressModeT;

    /**
     * @brief
     *  The name of the texture.
     * @remark
     *  Usually the pathname of the texture descriptor file,
     *  for dynamic textures and special textures a name
     *  that can not be used as a pathname.
     */
    String _name;

    /**
     * @brief
     *  The width, in pixels, of the source of this texture.
     * @remark
     *  This value might differ for technical reasons from the width of the texture.
     */
    int _sourceWidth;

    /**
     * @brief
     *  The height, in pixels, of the source of this texture.
     * @remark
     *  This value might differ for technical reasons from the height of the texture.
     */
    int _sourceHeight;

    /**
     * @brief
     *  The width, in pixels, of the the texture.
     */
    int _width;

    /**
     * @brief
     *  The height, in pixels, of the texture.
     */
    int _height;

    /**
     * @brief
     *  @a true if this texture has an alpha component, @a false otherwise.
     */
    bool _hasAlpha;

public:

    /**
     * @brief
     *  A pointer to the source of the texture if available, a null pointer otherwise.
     */
    SharedPtr<SDL_Surface> _source;

protected:

    /**
     * @brief
     *  Construct this texture.
     * @remark
     *  Intentionally protected.
     */
    Texture(const String& name,
            TextureType type, TextureAddressMode addressModeS, TextureAddressMode addressModeT,
            int width, int height, int sourceWidth, int sourceHeight, SharedPtr<SDL_Surface> source,
            bool hasAlpha);

public:

    /**
     * @brief
     *  Destruct this texture.
     */
    virtual ~Texture();

    /**
     * @brief
     *  Get the type of this texture.
     * @return
     *  the type of this texture
     */
    TextureType getType() const;

    /**
     * @brief
     *  Get the mipmap filter of this texture.
     * @return
     *  the mipmap filter
     */
    TextureFilter getMipMapFilter() const;

    /**
     * @brief
     *  Set the mipmap filter of this texture.
     * @param mipMapFilter
     *  the mipmap filter
     */
    void setMipMapFilter(TextureFilter minFilter);

    /**
     * @brief
     *  Get the minification filter of this texture.
     * @return
     *  the minification filter
     */
    TextureFilter getMinFilter() const;

    /**
     * @brief
     *  Set the minification filter of this texture.
     * @param minFilter
     *  the minification filter
     */
    void setMinFilter(TextureFilter minFilter);

    /**
     * @brief
     *  Get the magnification filter of this texture.
     * @return
     *  the magnification filter
     */
    TextureFilter getMagFilter() const;

    /**
     * @brief
     *  Set the magnification filter of this texture.
     * @param magFilter
     *  the magnification filter
     */
    void setMagFilter(TextureFilter magFilter);

    /**
     * @brief
     *  Get the address mode of this texture along the s-axis.
     * @return
     *  the address mode of this texture along the s-axis.
     */
    TextureAddressMode getAddressModeS() const;

    /**
     * @brief
     *  Set the address mode of this texture along the s-axis.
     * @param addressMode
     *  the address mode of this texture along the s-axis
     */
    void setAddressModeS(TextureAddressMode addressModeS);

    /**
     * @brief
     *  Get the address mode of this texture along the t-axis.
     * @return
     *  the address mode of this texture along the t-axis.
     */
    TextureAddressMode getAddressModeT() const;

    /**
     * @brief
     *  Set the address mode of this texture along the t-axis.
     * @param addressMode
     *  the address mode of this texture along the t-axis
     */
    void setAddressModeT(TextureAddressMode addressModeT);

    /**
     * @brief
     *  Get the width, in pixels, of this texture.
     * @param self
     *  this texture
     * @return
     *  the width, in pixels of this texture
     */
    int getWidth() const;

    /**
     * @brief
     *  Get the height, in pixels, of this texture.
     * @return
     *  the height, in pixels of this texture
     */
    int getHeight() const;

    /**
     * @brief
     *  Get the width, in pixels, of the source of this texture.
     * @return
     *  the width, in pixels, of the source of this texture
     * @remark
     *  This value might differ for technical reasons from the width of the texture.
     */
    int getSourceWidth() const;

    /**
     * @brief
     *  Get the height, in pixels, of the source of this texture.
     * @return
     *  the height, in pixels, of the source of this texture
     * @remark
     *  This value might differ for technical reasons from the height of the texture.
     */
    int getSourceHeight() const;

    /**
     * @brief
     *  Get if this texture has an alpha component.
     * @return
     *  @a true if this texture has an alpha component,
     *  @a false otherwise
     */
    bool hasAlpha() const;

    /**
     * @brief
     *  Set the name of this texture.
     * @param name
     *  the name
     */
    void setName(const String& name);

    /**
     * @brief
     *  Get the name of this texture.
     * @return
     *  the name of this texture
     */
    const String& getName() const;

public:
	virtual bool load(const String& name, const SharedPtr<SDL_Surface>& surface) = 0;
	virtual bool load(const SharedPtr<SDL_Surface>& image) = 0;

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

namespace Ego {
namespace OpenGL {
/// An encapsulation of the OpenGL texture state.
struct Texture : public Ego::Texture
{

protected:

    /**
     * @brief
     *  The OpenGL texture ID.
     * @remark
     *  At any point, a texture has a valid OpenGL texture ID assigned, <em>unless</em> resources were lost.
     */
    GLuint  _id;

public:
    void load(const String& name, const SharedPtr<SDL_Surface>& surface, TextureType type, const TextureSampler& sampler);
	/** @override Ego::Texture::upload(const String& name, const SharedPtr<SDL_Surface>&) */
    bool load(const String& name, const SharedPtr<SDL_Surface>& surface) override;

    /** @override Ego::Texture::upload(const std::shared_ptr<SDL_Surface>&) */
    bool load(const SharedPtr<SDL_Surface>& surface) override;
    
    /** @override Ego::Texture::release */
    void release() override;
    
    /** @override Ego::Texture::isDefault */
    bool isDefault() const override;

public:

    /**
     * @brief
     *  Construct this texture.
     * @post
     *  This texture is bound to the backing error texture.
     */
    Texture();
    /**
     * @brief
     *  Construct this texture.
     */
    Texture(GLuint id, const String& name,
            TextureType type, TextureAddressMode addressModeS, TextureAddressMode addressModeT,
            int width, int height, int sourceWidth, int sourceHeight, SharedPtr<SDL_Surface> source,
            bool hasAlpha);

    /**
     * @brief
     *  Destruct this texture.
     */
    virtual ~Texture();

public:

    GLuint getTextureID() const;

};

/**
 * @brief
 *  Initialize the error textures.
 * @todo
 *  Move into texture manager.
 */
void initializeErrorTextures();
/**
 * @brief
 *  Uninitialize the error textures.
 * @todo
 *  Move into texture manager.
 */
void uninitializeErrorTextures();

} // namespace OpenGL
} // namespace Ego

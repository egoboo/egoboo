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

/// @file egolib/Renderer/OpenGL/Texture.hpp
/// @brief Implementation of textures for OpenGL 2.1.
/// @author Michael Heilmann

#pragma once

#include "egolib/Renderer/Texture.hpp"
#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_extensions.h"

namespace Ego {
namespace OpenGL {

/// An encapsulation of the OpenGL texture state.
struct Texture : public Ego::Texture
{
protected:
    /// @brief The OpenGL texture ID.
    /// @remark At any point, a texture has a valid OpenGL texture ID assigned, <em>unless</em> resources were lost.
    GLuint  _id;

public:
    void load(const std::string& name, const std::shared_ptr<SDL_Surface>& surface, TextureType type, const TextureSampler& sampler);
    
    /** @override Ego::Texture::load(const String& name, const SharedPtr<SDL_Surface>&) */
    bool load(const std::string& name, const std::shared_ptr<SDL_Surface>& surface) override;

    /** @override Ego::Texture::load(const std::shared_ptr<SDL_Surface>&) */
    bool load(const std::shared_ptr<SDL_Surface>& surface) override;

    /** @override Ego::Texture::release */
    void release() override;

    /** @override Ego::Texture::isDefault */
    bool isDefault() const override;

public:

    /// @brief Construct this texture.
    /// @post This texture is bound to the backing error texture.
    Texture();

    ///@brief Construct this texture.
    Texture(GLuint id, const std::string& name,
            TextureType type, TextureAddressMode addressModeS, TextureAddressMode addressModeT,
            int width, int height, int sourceWidth, int sourceHeight, std::shared_ptr<SDL_Surface> source,
            bool hasAlpha);

    /// @brief Destruct this texture.
    virtual ~Texture();

public:
    GLuint getTextureID() const;

};

/// @brief Initialize the error textures.
/// @todo Move into texture manager.
void initializeErrorTextures();

/// @brief Uninitialize the error textures.
/// @todo Move into texture manager.
void uninitializeErrorTextures();

} // namespace OpenGL
} // namespace Ego

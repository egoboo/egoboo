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
#include "egolib/Extensions/ogl_extensions.h"

namespace Ego {
namespace OpenGL {

class Renderer;
class RendererInfo;

/// An encapsulation of the OpenGL texture state.
class Texture : public Ego::Texture
{
protected:
    Renderer *m_renderer;
    /// @brief The OpenGL texture ID.
    /// @remark At any point, a texture has a valid OpenGL texture ID assigned, <em>unless</em> resources were lost.
    GLuint m_id;

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
    Texture(Renderer *renderer);

    /// @brief Construct this texture.
    Texture(Renderer *renderer, GLuint id, const std::string& name,
            TextureType type, const TextureSampler& sampler,
            int width, int height, int sourceWidth, int sourceHeight, std::shared_ptr<SDL_Surface> source,
            bool hasAlpha);

    /// @brief Destruct this texture.
    virtual ~Texture();

public:
    GLuint getId() const;
    void setId(GLuint id);
};

} // namespace OpenGL
} // namespace Ego

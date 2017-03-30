#pragma once

#include "egolib/Renderer/TextureType.hpp"
#include "egolib/Renderer/TextureSampler.hpp"
#include "egolib/Image/SDL_Image_Extensions.h"

namespace Ego {
namespace OpenGL {

class RendererInfo;

class DefaultTexture
{
private:
    /// @brief The renderer info.
    std::shared_ptr<RendererInfo> m_info;

    /// @brief The image of the default texture.
    std::shared_ptr<SDL_Surface> m_image;

    /// The texture sampler.
    TextureSampler m_sampler;
    
    /// The type.
    TextureType m_type;
    
    /// The OpenGL texture ID.
    GLuint m_id;

    /// @brief The name.
    std::string m_name;

public:
    // Construct this default texture.
    DefaultTexture(std::shared_ptr<RendererInfo> info, const std::string& name, TextureType type);
    
    // Destruct this default texture.
    ~DefaultTexture();

    const TextureSampler& getSampler() const;

    GLuint getId() const;

    TextureType getType() const;

    bool hasAlpha() const;

    const std::string& getName() const;

    int getSourceWidth() const;

    int getSourceHeight() const;

    int getWidth() const;

    int getHeight() const;
};

} // namespace OpenGL
} // namespace Ego

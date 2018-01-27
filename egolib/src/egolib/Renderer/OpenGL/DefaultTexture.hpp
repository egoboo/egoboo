#pragma once

#include "egolib/Image/SDL_Image_Extensions.h"
#include <GL/glew.h>

namespace Ego { namespace OpenGL {

class RendererInfo;

class DefaultTexture
{
private:
    /// @brief The renderer info.
    std::shared_ptr<RendererInfo> m_info;

    /// @brief The image of the default texture.
    std::shared_ptr<SDL_Surface> m_image;

    /// The texture sampler.
    id::texture_sampler m_sampler;
    
    /// The type.
    id::texture_type m_type;
    
    /// The OpenGL texture ID.
    GLuint m_id;

    /// @brief The name.
    std::string m_name;

public:
    // Construct this default texture.
    DefaultTexture(std::shared_ptr<RendererInfo> info, const std::string& name, id::texture_type type);
    
    // Destruct this default texture.
    ~DefaultTexture();

    const id::texture_sampler& getSampler() const;

    GLuint getId() const;

    id::texture_type getType() const;

    bool hasAlpha() const;

    const std::string& getName() const;

    int getSourceWidth() const;

    int getSourceHeight() const;

    int getWidth() const;

    int getHeight() const;
};

} } // namespace Ego::OpenGL

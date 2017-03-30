#include "egolib/Renderer/OpenGL/DefaultTexture.hpp"

#include "egolib/Renderer/OpenGL/Utilities.hpp"
#include "egolib/Image/ImageManager.hpp"

namespace Ego {
namespace OpenGL {

DefaultTexture::DefaultTexture(std::shared_ptr<RendererInfo> info, const std::string& name, TextureType type) :
    m_name(name), m_type(type), m_image(ImageManager::get().getDefaultImage()),
    m_sampler(TextureFilter::Nearest, TextureFilter::Nearest,
              TextureFilter::None, TextureAddressMode::Repeat,
              TextureAddressMode::Repeat, 1.0f),
    m_info(info)
{
    GLenum target_gl;
    switch (type)
    {
        case TextureType::_1D:
            target_gl = GL_TEXTURE_1D;
            break;
        case TextureType::_2D:
            target_gl = GL_TEXTURE_2D;
            break;
        default:
            throw id::invalid_argument_error(__FILE__, __LINE__, "invalid texture target");
    };
    Utilities2::clearError();
    // (1) Create the OpenGL texture.
    glGenTextures(1, &m_id);
    if (Utilities2::isError())
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to create error texture");
    }
    // (2) Bind the OpenGL texture.
    glBindTexture(target_gl, m_id);
    if (Utilities2::isError())
    {
        glDeleteTextures(1, &m_id);
        throw id::runtime_error(__FILE__, __LINE__, "unable to bind error texture");
    }
    // (3) Set the texture parameters.
    try
    {
        Utilities2::setSampler(std::static_pointer_cast<RendererInfo>(m_info), type, m_sampler);
    }
    catch (...)
    {
        glDeleteTextures(1, &m_id);
        std::rethrow_exception(std::current_exception());
    }

    // (4) Upload the image data.
    if (type == TextureType::_1D)
    {
        static const auto pfd = PixelFormatDescriptor::get<PixelFormat::R8G8B8A8>();
        Utilities2::upload_1d(pfd, m_image->w, m_image->pixels);
    }
    else
    {
        static const auto pfd = PixelFormatDescriptor::get<PixelFormat::R8G8B8A8>();
        Utilities2::upload_2d(pfd, m_image->w, m_image->h, m_image->pixels);
    }
    if (Utilities2::isError())
    {
        glDeleteTextures(1, &m_id);
        throw id::runtime_error(__FILE__, __LINE__, "unable to upload error image into error texture");
    }
}

DefaultTexture::~DefaultTexture()
{
    glDeleteTextures(1, &m_id);
}


const TextureSampler& DefaultTexture::getSampler() const
{
    return m_sampler;
}

GLuint DefaultTexture::getId() const
{
    return m_id;
}

TextureType DefaultTexture::getType() const
{
    return m_type;
}

bool DefaultTexture::hasAlpha() const
{
    return false;
}

const std::string& DefaultTexture::getName() const
{
    return m_name;
}

int DefaultTexture::getSourceWidth() const
{
    return m_image->w;
}

int DefaultTexture::getSourceHeight() const
{
    return m_image->h;
}

int DefaultTexture::getWidth() const
{
    return getSourceWidth();
}

int DefaultTexture::getHeight() const
{
    return getSourceHeight();
}

} // namespace OpenGL
} // namespace Ego

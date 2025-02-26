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

/// @file egolib/Renderer/OpenGL/Texture.cpp
/// @brief Implementation of textures for OpenGL 2.1.
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/Texture.hpp"

#include "egolib/Renderer/OpenGL/Renderer.hpp"
#include "egolib/Renderer/OpenGL/Utilities.hpp"
#include "egolib/Renderer/OpenGL/RendererInfo.hpp"
#include "egolib/Renderer/OpenGL/DefaultTexture.hpp"
#include "egolib/Image/ImageManager.hpp"

namespace Ego {
namespace OpenGL {

Texture::Texture(Renderer *renderer) :
    Texture
    (
        renderer,
        renderer->m_defaultTexture2d->getId(),
        renderer->m_defaultTexture2d->getName(),
        renderer->m_defaultTexture2d->getType(),
        renderer->m_defaultTexture2d->getSampler(),
        renderer->m_defaultTexture2d->getWidth(), renderer->m_defaultTexture2d->getHeight(),
        renderer->m_defaultTexture2d->getSourceWidth(), renderer->m_defaultTexture2d->getSourceHeight(),
        nullptr,
        renderer->m_defaultTexture2d->hasAlpha()
    )
{}

Texture::Texture(Renderer *renderer, GLuint id, const std::string& name,
                 idlib::texture_type type, const idlib::texture_sampler& sampler,
                 int width, int height, int sourceWidth, int sourceHeight,
				 std::shared_ptr<SDL_Surface> source,
                 bool hasAlpha) :
    Ego::Texture
    (
        name,
        type,
        sampler,
        width, height,
        sourceWidth, sourceHeight,
        source,
        hasAlpha
    ),
    m_id(id),
    m_renderer(renderer)
{}

Texture::~Texture()
{
    release();
}

GLuint Texture::getId() const
{
    return m_id;
}

void Texture::setId(GLuint id)
{
    m_id = id;
}

void Texture::load(const std::string& name, const std::shared_ptr<SDL_Surface>& surface, idlib::texture_type type, const idlib::texture_sampler& sampler)
{
    // Bind this texture to the backing error texture.
    release();

    // If no surface is provided, keep this texture bound to the backing error texture.
    if (!surface)
    {
        throw idlib::argument_null_error(__FILE__, __LINE__, "surface");
    }

    std::shared_ptr<SDL_Surface> newSurface = surface;

    // Convert to RGBA if the image has non-opaque alpha values or alpha modulation and convert to RGB otherwise.
    bool hasAlpha = SDL::testAlpha(newSurface.get());
    const auto& pixel_format = hasAlpha ? pixel_descriptor::get<idlib::pixel_format::R8G8B8A8>()
                                        : pixel_descriptor::get<idlib::pixel_format::R8G8B8>();
    newSurface = convert(newSurface, pixel_format);

    // Convert to power of two.
    newSurface = idlib::power_of_two(newSurface);

    // (1)Generate a new OpenGL texture ID.
    Utilities::clearError();
    GLuint id;
    glGenTextures(1, &id);
    if (Utilities::isError())
    {
        throw idlib::runtime_error(__FILE__, __LINE__, "glGenTextures failed");
    }
    // (2) Bind the new OpenGL texture ID.
    GLenum target_gl;
    switch (type)
    {
        case idlib::texture_type::_1D:
            target_gl = GL_TEXTURE_1D;
            break;
        case idlib::texture_type::_2D:
            target_gl = GL_TEXTURE_2D;
            break;
        default:
        {
            glDeleteTextures(1, &id);
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
        }
    };
    glBindTexture(target_gl, id);
    if (Utilities::isError())
    {
        glDeleteTextures(1, &id);
        throw idlib::runtime_error(__FILE__, __LINE__, "glBindTexture failed");
    }
    // (3) Set the texture sampler.
    try
    {
        Utilities2::setSampler(std::static_pointer_cast<RendererInfo>(m_renderer->getInfo()), type, sampler);
    }
    catch (...)
    {
        glDeleteTextures(1, &id);
        std::rethrow_exception(std::current_exception());
    }
    // (4) Upload the texture data.
    switch (type)
    {
        case idlib::texture_type::_2D:
        {
            if (idlib::texture_filter_method::none != sampler.mip_filter_method())
            {
                Utilities2::upload_2d_mipmap(pixel_format, newSurface->w, newSurface->h, newSurface->pixels);
            }
            else
            {
                Utilities2::upload_2d(pixel_format, newSurface->w, newSurface->h, newSurface->pixels);
            }
        }
        break;
        case idlib::texture_type::_1D:
        {
            Utilities2::upload_1d(pixel_format, newSurface->w, newSurface->pixels);
        }
        break;
        default:
        {
            glDeleteTextures(1, &id);
            throw idlib::unhandled_switch_case_error(__FILE__, __LINE__);
        }
        break;
    };

    // Store the appropriate data.
    m_sampler = sampler;
    m_type = type;
    m_id = id;
    m_width = newSurface->w;
    m_height = newSurface->h;
    m_source = surface;
    m_sourceWidth = surface->w;
    m_sourceHeight = surface->h;
    m_hasAlpha = hasAlpha;
    m_name = name;
}

bool Texture::load(const std::string& name, const std::shared_ptr<SDL_Surface>& source)
{
    auto info = Ego::Renderer::get().getInfo();
    // Determine the texture sampler.
    idlib::texture_sampler sampler(info->getDesiredMinimizationFilter(),
                                   info->getDesiredMaximizationFilter(),
                                   info->getDesiredMipMapFilter(),
                                   idlib::texture_address_mode::repeat, idlib::texture_address_mode::repeat,
                                   info->getDesiredAnisotropy());
    // Determine the texture type.
    auto type = ((1 == source->h) && (source->w > 1)) ? idlib::texture_type::_1D : idlib::texture_type::_2D;
    load(name, source, type, sampler);
    return true;
}

bool Texture::load(const std::shared_ptr<SDL_Surface>& source)
{
    std::ostringstream stream;
    stream << "<source " << static_cast<void *>(source.get()) << ">";
    return load(stream.str(), source);
}

void  Texture::release()
{
    if (isDefault())
    {
        return;
    }

    // Delete the OpenGL texture and assign the error texture.
    glDeleteTextures(1, &(m_id));
    Utilities::isError();

    // Delete the source if it exists
    if (m_source)
    {
       m_source = nullptr;
    }

    // The texture is the 2D error texture.
    m_type = m_renderer->m_defaultTexture2d->getType();
    m_id = m_renderer->m_defaultTexture2d->getId();

    // The texture coordinates of this texture are repeated along the s and t axes.
    m_sampler = m_renderer->m_defaultTexture2d->getSampler();

    // The size (width and height) of this texture is the size of the error image.
    m_width = m_renderer->m_defaultTexture2d->getWidth();
    m_height = m_renderer->m_defaultTexture2d->getHeight();

    // The size (width and height) the source of this texture is the size of the error image as well.
    m_sourceWidth = m_renderer->m_defaultTexture2d->getSourceWidth();
    m_sourceHeight = m_renderer->m_defaultTexture2d->getSourceHeight();

    // The texture has the empty string as its name and the source is not available.
    m_name = m_renderer->m_defaultTexture2d->getName();

    // (The error texture has no alpha component).
    m_hasAlpha = m_renderer->m_defaultTexture2d->hasAlpha();
}

bool Texture::isDefault() const
{
    return getId() == m_renderer->m_defaultTexture2d->getId()
        || getId() == m_renderer->m_defaultTexture2d->getId();
}

} // namespace OpenGL
} // namespace Ego

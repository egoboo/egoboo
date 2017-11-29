#include "egolib/Renderer/OpenGL/Utilities.hpp"
#include "egolib/Renderer/OpenGL/RendererInfo.hpp"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace OpenGL {

void Utilities2::clearError()
{
    while (GL_NO_ERROR != glGetError())
    {
        /* Nothing to do. */
    }
}

bool Utilities2::isError()
{
    GLenum error = glGetError();
    if (GL_NO_ERROR != error)
    {
        switch (error)
        {
            case GL_INVALID_ENUM:
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "GL_INVALID_ENUM", Log::EndOfEntry);
                break;
            case GL_INVALID_VALUE:
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "GL_INVALID_VALUE", Log::EndOfEntry);
                break;
            case GL_INVALID_OPERATION:
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "GL_INVALID_OPERATION", Log::EndOfEntry);
                break;
            #if defined(GL_INVALID_FRAMEBUFFER_OPERATION)
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "GL_INVALID_FRAMEBUFFER_OPERATION", Log::EndOfEntry);
                break;
            #endif
            case GL_OUT_OF_MEMORY:
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "GL_OUT_OF_MEMORY", Log::EndOfEntry);
                break;
            case GL_STACK_UNDERFLOW:
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "GL_STACK_UNDERFLOW", Log::EndOfEntry);
                break;
            case GL_STACK_OVERFLOW:
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "GL_STACK_OVERFLOW", Log::EndOfEntry);
                break;
        };
        clearError();
        return true;
    }
    return false;
}

std::unordered_set<std::string> Utilities2::getExtensions()
{
    Utilities::clearError();
    const GLubyte *bytes = glGetString(GL_EXTENSIONS);
    if (Utilities::isError())
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to acquire renderer back-end information");
    }
    auto tokens = Ego::split(std::string((const char *)bytes), std::string(" "));
    return std::unordered_set<std::string>(tokens.cbegin(), tokens.cend());
}

std::string Utilities2::getRenderer()
{
    Utilities::clearError();
    const GLubyte *bytes = glGetString(GL_RENDERER);
    if (Utilities::isError())
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to acquire renderer back-end information");
    }
    return (const char *)bytes;
}

std::string Utilities2::getVendor()
{
    Utilities::clearError();
    const GLubyte *bytes = glGetString(GL_VENDOR);
    if (Utilities::isError())
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to acquire renderer back-end information");
    }
    return (const char *)bytes;
}

std::string Utilities2::getVersion()
{
    Utilities::clearError();
    const GLubyte *bytes = glGetString(GL_VERSION);
    if (Utilities::isError())
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to acquire renderer back-end information");
    }
    return (const char *)bytes;
}

uint8_t Utilities2::getDepthBufferDepth()
{
    // Get the depth buffer depth.
    GLint depth;
    glGetIntegerv(GL_DEPTH_BITS, &depth);
    if (Utilities::isError())
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to acquire renderer back-end information");
    }
    return depth;
}

uint8_t Utilities2::getStencilBufferDepth()
{
    // Get the stencil buffer depth.
    GLint depth;
    glGetIntegerv(GL_STENCIL_BITS, &depth);
    if (Utilities::isError())
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to acquire renderer back-end information");
    }
    return depth;
}

GLint Utilities2::toOpenGL(id::texture_address_mode textureAddressMode)
{
    switch (textureAddressMode)
    {
        case id::texture_address_mode::clamp:
            return GL_CLAMP;
        case id::texture_address_mode::clamp_to_border:
            return GL_CLAMP_TO_BORDER;
        case id::texture_address_mode::clamp_to_edge:
            return GL_CLAMP_TO_EDGE;
        case id::texture_address_mode::repeat:
            return GL_REPEAT;
        case id::texture_address_mode::repeat_mirrored:
            return GL_MIRRORED_REPEAT;
        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    }
}

GLenum Utilities2::toOpenGL(id::primitive_type primitiveType)
{
    switch (primitiveType)
    {
        case id::primitive_type::points:
            return GL_POINTS;
        case id::primitive_type::lines:
            return GL_LINES;
        case id::primitive_type::line_loop:
            return GL_LINE_LOOP;
        case id::primitive_type::line_strip:
            return GL_LINE_STRIP;
        case id::primitive_type::triangles:
            return GL_TRIANGLES;
        case id::primitive_type::triangle_fan:
            return GL_TRIANGLE_FAN;
        case id::primitive_type::triangle_strip:
            return GL_TRIANGLE_STRIP;
        case id::primitive_type::quadriliterals:
            return GL_QUADS;
        case id::primitive_type::quadriliteral_strip:
            return GL_QUAD_STRIP;
        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    }
}

bool Utilities2::isAnisotropySupported()
{
    static const std::string extensionString("GL_EXT_texture_filter_anisotropic");
    auto extensions = getExtensions();
    return extensions.cend() != extensions.find(extensionString);
}

float Utilities2::getMaximumSupportedAnisotropy()
{
    if (!isAnisotropySupported())
    {
        throw id::runtime_error(__FILE__, __LINE__, "anisotropy is not supported");
    }
    float maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    if (Utilities::isError())
    {
        throw id::runtime_error(__FILE__, __LINE__, "unable to determine maximum supported anisotropy levels");
    }
    return maxAnisotropy;
}

float Utilities2::getMinimumSupportedAnisotropy()
{
    if (!isAnisotropySupported())
    {
        throw id::runtime_error(__FILE__, __LINE__, "anisotropy is not supported");

    }
    return 1.0f;
}

void Utilities2::upload_1d(const pixel_descriptor& pfd, GLsizei w, const void *data)
{
    GLenum internalFormat_gl, format_gl, type_gl;
    Utilities2::toOpenGL(pfd, internalFormat_gl, format_gl, type_gl);
    PushClientAttrib pca(GL_CLIENT_PIXEL_STORE_BIT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage1D(GL_TEXTURE_1D, 0, internalFormat_gl, w, 0, format_gl, type_gl, data);
}

void Utilities2::upload_2d(const pixel_descriptor& pfd, GLsizei w, GLsizei h, const void *data)
{
    GLenum internalFormat_gl, format_gl, type_gl;
    Utilities2::toOpenGL(pfd, internalFormat_gl, format_gl, type_gl);
    PushClientAttrib pca(GL_CLIENT_PIXEL_STORE_BIT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat_gl, w, h, 0, format_gl, type_gl, data);
}

void Utilities2::upload_2d_mipmap(const pixel_descriptor& pfd, GLsizei w, GLsizei h, const void *data)
{
    GLenum internalFormat_gl, format_gl, type_gl;
    Utilities2::toOpenGL(pfd, internalFormat_gl, format_gl, type_gl);
    PushClientAttrib pca(GL_CLIENT_PIXEL_STORE_BIT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat_gl, w, h, 0, format_gl, type_gl, data);

    if (w == 1 && h == 1) return;

    uint32_t alphaMask = pfd.get_alpha().get_mask(),
             redMask = pfd.get_red().get_mask(),
             greenMask = pfd.get_green().get_mask(),
             blueMask = pfd.get_blue().get_mask();
    int bpp = pfd.get_color_depth().depth();

    SDL_Surface *surf = SDL_CreateRGBSurfaceFrom((void *)data, w, h, bpp, w * bpp / 8, redMask, greenMask, blueMask, alphaMask);
    SDL_assert(surf != nullptr);

    GLsizei newW = w;
    GLsizei newH = h;
    GLint level = 0;

    do
    {
        if (newW > 1) newW /= 2;
        if (newH > 1) newH /= 2;
        level++;

        SDL_Surface *newSurf = SDL_CreateRGBSurface(0, newW, newH, bpp, redMask, greenMask, blueMask, alphaMask);
        SDL_assert(newSurf != nullptr);

        /// @todo this is 'low-quality' and not thread-safe
        SDL_SoftStretch(surf, nullptr, newSurf, nullptr);

        glTexImage2D(GL_TEXTURE_2D, level, internalFormat_gl, newW, newH, 0, format_gl, type_gl, newSurf->pixels);
        SDL_FreeSurface(newSurf);
    } while (!(newW == 1 && newH == 1));

    SDL_FreeSurface(surf);
}

void Utilities2::toOpenGL(id::texture_filter_method minFilter, id::texture_filter_method magFilter, id::texture_filter_method mipMapFilter, GLint& minFilter_gl, GLint& magFilter_gl)
{
    switch (minFilter)
    {
        // In OpenGL for the minification filter, "none" and "nearest" coincide.
        case id::texture_filter_method::none:
        case id::texture_filter_method::nearest:
            switch (mipMapFilter)
            {
                case id::texture_filter_method::none:
                    minFilter_gl = GL_NEAREST;
                    break;
                case id::texture_filter_method::nearest:
                    minFilter_gl = GL_NEAREST_MIPMAP_NEAREST;
                    break;
                case id::texture_filter_method::linear:
                    minFilter_gl = GL_NEAREST_MIPMAP_LINEAR;
                    break;
                default:
                    throw id::unhandled_switch_case_error(__FILE__, __LINE__);
            }
            break;
        case id::texture_filter_method::linear:
            switch (mipMapFilter)
            {
                case id::texture_filter_method::none:
                    minFilter_gl = GL_LINEAR;
                    break;
                case id::texture_filter_method::nearest:
                    minFilter_gl = GL_LINEAR_MIPMAP_NEAREST;
                    break;
                case id::texture_filter_method::linear:
                    minFilter_gl = GL_LINEAR_MIPMAP_LINEAR;
                    break;
                default:
                    throw id::unhandled_switch_case_error(__FILE__, __LINE__);
            }
            break;
        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    };
    switch (magFilter)
    {
        // In OpenGL for the magnification filter, "none" and "nearest" coincide.
        case id::texture_filter_method::none:
        case id::texture_filter_method::nearest:
            magFilter_gl = GL_NEAREST;
            break;
        case id::texture_filter_method::linear:
            magFilter_gl = GL_LINEAR;
            break;
        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    };
}


void Utilities2::toOpenGL(const pixel_descriptor& pfd, GLenum& internalFormat_gl, GLenum& format_gl, GLenum& type_gl)
{
    switch (pfd.get_pixel_format())
    {
        case id::pixel_format::R8G8B8:
            internalFormat_gl = GL_RGB;
            format_gl = GL_RGB;
            type_gl = GL_UNSIGNED_BYTE;
            break;
        case id::pixel_format::R8G8B8A8:
            internalFormat_gl = GL_RGBA;
            format_gl = GL_RGBA;
            type_gl = GL_UNSIGNED_BYTE;
            break;
        case id::pixel_format::B8G8R8:
            internalFormat_gl = GL_BGR;
            format_gl = GL_BGR;
            type_gl = GL_UNSIGNED_BYTE;
            break;
        case id::pixel_format::B8G8R8A8:
            internalFormat_gl = GL_BGRA;
            format_gl = GL_BGRA;
            type_gl = GL_UNSIGNED_BYTE;
            break;
        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__, "pixel format not supported");
    };
}

void Utilities2::setSampler(const std::shared_ptr<RendererInfo>& info, id::texture_type target, const id::texture_sampler& sampler)
{
    Utilities::clearError();
    GLenum target_gl;
    switch (target)
    {
        case id::texture_type::_2D:
            target_gl = GL_TEXTURE_2D;
            break;
        case id::texture_type::_1D:
            target_gl = GL_TEXTURE_1D;
            break;
        default:
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    }
    if (Utilities::isError())
    {
        return;
    }

    GLint addressModeS_gl = Utilities2::toOpenGL(sampler.address_mode_s()),
          addressModeT_gl = Utilities2::toOpenGL(sampler.address_mode_t());
    glTexParameteri(target_gl, GL_TEXTURE_WRAP_S, addressModeS_gl);
    glTexParameteri(target_gl, GL_TEXTURE_WRAP_T, addressModeT_gl);
    if (Utilities::isError())
    {
        return;
    }

    GLint minFilter_gl, magFilter_gl;
    Utilities2::toOpenGL(sampler.min_filter_method(), sampler.mag_filter_method(), sampler.mip_filter_method(),
                         minFilter_gl, magFilter_gl);
    glTexParameteri(target_gl, GL_TEXTURE_MIN_FILTER, minFilter_gl);
    glTexParameteri(target_gl, GL_TEXTURE_MAG_FILTER, magFilter_gl);
    if (Utilities::isError())
    {
        return;
    }

    if (id::texture_type::_2D == target && info->isAnisotropySupported() && info->isAnisotropyDesired())
    {
        float anisotropyLevel = Math::constrain(sampler.anisotropy_levels(),
                                                info->getMinimumSupportedAnisotropy(),
                                                info->getMaximumSupportedAnisotropy());
        glTexParameterf(target_gl, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropyLevel);
        if (Utilities::isError())
        {
            return;
        }
    }
}

} // namespace OpenGL
} // namespace Ego

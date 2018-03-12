#include "egolib/Renderer/OpenGL/RendererInfo.hpp"

#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/egoboo_setup.h"
#include "egolib/Renderer/OpenGL/Utilities.hpp"

namespace Ego::OpenGL {

RendererInfo::RendererInfo() :
    m_renderer(Utilities2::getRenderer()),
    m_vendor(Utilities2::getVendor()),
    m_version(Utilities2::getVersion()),
    m_isAnisotropySupported(Utilities2::isAnisotropySupported()),
    m_minimumSupportedAnisotropy(std::numeric_limits<single>::quiet_NaN()),
    m_maximumSupportedAnisotropy(std::numeric_limits<single>::quiet_NaN()),
    m_extensions(Utilities2::getExtensions())
{
    if (m_isAnisotropySupported)
    {
        single temporary;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &temporary);
        if (Utilities::isError())
        {
            throw idlib::runtime_error(__FILE__, __LINE__, "unable to acquire renderer back-end information");
        }
        m_maximumSupportedAnisotropy = temporary;
        m_minimumSupportedAnisotropy = 1.0f;
    }
    {
        GLint temporary;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &temporary);
        if (Utilities::isError())
        {
            throw idlib::runtime_error(__FILE__, __LINE__, "unable to acquire renderer back-end information");
        }
        m_maximumTextureSize = temporary;
    }
}

RendererInfo::~RendererInfo()
{}

std::string RendererInfo::getRenderer() const
{
    return m_renderer;
}

std::string RendererInfo::getVendor() const
{
    return m_vendor;
}

std::string RendererInfo::getVersion() const
{
    return m_version;
}

bool RendererInfo::isAnisotropySupported() const noexcept
{
    return m_isAnisotropySupported;
}

single RendererInfo::getMinimumSupportedAnisotropy() const noexcept
{
    return m_minimumSupportedAnisotropy;
}

single RendererInfo::getMaximumSupportedAnisotropy() const noexcept
{
    return m_maximumSupportedAnisotropy;
}

int RendererInfo::getMaximumTextureSize() const noexcept
{
    return m_maximumTextureSize;
}

const std::unordered_set<std::string>& RendererInfo::getExtensions() const
{
    return m_extensions;
}

std::string RendererInfo::toString() const
{
    std::ostringstream os;
    os << "OpenGL" << std::endl
       << "  version    = " << getVersion() << std::endl
       << "  vendor     = " << getVendor() << std::endl
       << "  renderer   = " << getRenderer() << std::endl
       << "  extensions = ";
    for (const auto& extension : getExtensions())
    {
        os << " " << extension;
    }
    os << "maximum texture size = " << getMaximumTextureSize() << std::endl;
    os << "anisotropy supported = " << isAnisotropySupported() << std::endl;
    if (isAnisotropySupported())
    {
        os << "minimum supported anisotropy = " << getMinimumSupportedAnisotropy() << std::endl;
        os << "maximum supported anisotropy = " << getMaximumSupportedAnisotropy() << std::endl;
    }
    os << std::endl;
    return os.str();
}

} // namespace Ego::OpenGL

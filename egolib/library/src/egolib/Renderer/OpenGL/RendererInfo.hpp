#pragma once

#include "egolib/Renderer/RendererInfo.hpp"

namespace Ego {
namespace OpenGL {

class RendererInfo : public Ego::RendererInfo
{
private:
    /// @brief The name of this renderer.
    std::string m_renderer;

    /// @brief The name of the vendor of this renderer.
    std::string m_vendor;

    /// @brief The version of this renderer.
    std::string m_version;

    /// @see Ego::OpenGL::RendererInfo::getMaximumTextureSize()
    int m_maximumTextureSize;

    /// @see Ego::OpenGL::RendererInfo::isAnisotropySupported()
    bool m_isAnisotropySupported;

    /// @see Ego::OpenGL::RendererInfo::getMinimumSupportedAnisotropy()
    float m_minimumSupportedAnisotropy;

    /// @see Ego::OpenGL::RendererInfo::getMaximumSupportedAnisotropy()
    float m_maximumSupportedAnisotropy;

public:
    /// @brief Construct this renderer info.
    RendererInfo();

    /// @brief Destruct this renderer info.
    virtual ~RendererInfo();

    /// @copydoc Ego::RendererInfo::getRenderer
    virtual std::string getRenderer() const override;

    /// @copydoc Ego::RendererInfo::getVendor
    virtual std::string getVendor() const override;

    /// @copydoc Ego::RendererInfo::getVersion
    std::string getVersion() const override;

    /// @copydoc Ego::RendererInfo::isAnisotropySupported
    bool isAnisotropySupported() const noexcept override;

    /// @copydoc Ego::RendererInfo::getMinimumSupportedAnisotropy
    float getMinimumSupportedAnisotropy() const noexcept override;

    /// @copydoc Ego::RendererInfo::getMaximumSupportedAnisostropy
    float getMaximumSupportedAnisotropy() const noexcept override;

    /// @copydoc Ego::RendererInfo::getMaximumTextureSize
    int getMaximumTextureSize() const noexcept override;

    /// @copydoc Ego::RendererInfo::toString
    std::string toString() const override;

private:
    /// @brief The set of OpenGL extensions supported by this OpenGL renderer.
    std::unordered_set<std::string> m_extensions;

public:
    /// @brief Get the set of OpenGL extensions supported by this OpenGL renderer.
    /// @return the set of OpenGL extensions supported by this OpenGL renderer.
    const std::unordered_set<std::string>& getExtensions() const;
};

} // namespace OpenGL
} // namespace Ego

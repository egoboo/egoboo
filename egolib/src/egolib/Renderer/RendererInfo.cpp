#include "egolib/Renderer/RendererInfo.hpp"

#include "egolib/egoboo_setup.h"

namespace Ego {

RendererInfo::RendererInfo()
{
    auto& configuration = egoboo_config_t::get();

    try
    {
        m_connections.push_back(configuration.graphic_anisotropy_enable.ValueChanged.subscribe([this]()
            { 
                auto& configuration = egoboo_config_t::get();
                m_isAnisotropyDesired = configuration.graphic_anisotropy_enable.getValue();
                AnisotropyDesiredChanged(); 
            }));
        m_connections.push_back(configuration.graphic_anisotropy_levels.ValueChanged.subscribe([this]()
            {
                auto& configuration = egoboo_config_t::get();
                m_desiredAnisotropy = configuration.graphic_anisotropy_levels.getValue();
                DesiredAnisotropyChanged();
            }));
        m_connections.push_back(configuration.graphic_textureFilter_minFilter.ValueChanged.subscribe([this]()
        { 
            auto& configuration = egoboo_config_t::get();
            m_desiredMinimizationFilter = configuration.graphic_textureFilter_minFilter.getValue();
            DesiredMinimizationFilterChanged(); 
        }));
        m_connections.push_back(configuration.graphic_textureFilter_magFilter.ValueChanged.subscribe([this]()
        { 
            auto& configuration = egoboo_config_t::get();
            m_desiredMaximizationFilter = configuration.graphic_textureFilter_magFilter.getValue();
            DesiredMaximizationFilterChanged(); 
        }));
        m_connections.push_back(configuration.graphic_textureFilter_mipMapFilter.ValueChanged.subscribe([this]()
        { 
            auto& configuration = egoboo_config_t::get();
            m_desiredMipMapFilter = configuration.graphic_textureFilter_mipMapFilter.getValue();
            DesiredMipMapFilterChanged(); 
        }));
    }
    catch (...)
    {
        for (auto& connection : m_connections)
        {
            connection.disconnect();
        }
        throw std::current_exception();
    }
    try
    {
        m_isAnisotropyDesired = configuration.graphic_anisotropy_enable.getValue();
        m_desiredAnisotropy = configuration.graphic_anisotropy_levels.getValue();
        m_desiredMinimizationFilter = configuration.graphic_textureFilter_minFilter.getValue();
        m_desiredMaximizationFilter = configuration.graphic_textureFilter_magFilter.getValue();
        m_desiredMipMapFilter = configuration.graphic_textureFilter_mipMapFilter.getValue();
    }
    catch (...)
    {
        for (auto& connection : m_connections)
        {
            connection.disconnect();
        }
        throw std::current_exception();
    }
}

RendererInfo::~RendererInfo()
{
    for (auto& connection : m_connections)
    {
        connection.disconnect();
    }
}

bool RendererInfo::isAnisotropyDesired() const noexcept
{
    return m_isAnisotropyDesired;
}

float RendererInfo::getDesiredAnisotropy() const noexcept
{
    return m_desiredAnisotropy;
}

TextureFilter RendererInfo::getDesiredMinimizationFilter() const noexcept
{
    return m_desiredMinimizationFilter;
}

TextureFilter RendererInfo::getDesiredMaximizationFilter() const noexcept
{
    return m_desiredMaximizationFilter;
}

TextureFilter RendererInfo::getDesiredMipMapFilter() const noexcept
{
    return m_desiredMipMapFilter;
}

} // namespace Ego

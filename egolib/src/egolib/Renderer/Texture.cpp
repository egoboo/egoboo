//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
//*    distributed with Egoboo.
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

/// @file egolib/Renderer/Texture.hpp
/// @brief Common interface of all texture object encapsulations.
/// @author Michael Heilmann

#include "egolib/Renderer/Texture.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Math/_Include.hpp"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/Image/Image.hpp"

namespace Ego {

Texture::Texture(const std::string& name,
                 id::texture_type type,
                 const id::texture_sampler& sampler,
	             int width, int height, int sourceWidth, int sourceHeight,
				 std::shared_ptr<SDL_Surface> source,
                 bool hasAlpha) :
    m_name(name),
    m_type(type),
    m_width(width), m_height(height), m_sourceWidth(sourceWidth), m_sourceHeight(sourceHeight), m_source(source),
    m_sampler(sampler),
    m_hasAlpha(hasAlpha),
    m_arePixelsDirty(true),
    m_isSamplerDirty(true)
{}

Texture::~Texture()
{}

id::texture_type Texture::getType() const
{
    return m_type;
}

id::texture_filter_method Texture::getMipMapFilter() const
{
    return m_sampler.mip_filter_method();
}

void Texture::setMipMapFilter(id::texture_filter_method mipMapFilter)
{
    if (mipMapFilter != m_sampler.mip_filter_method())
    {
        m_isSamplerDirty = true;
    }
    m_sampler.mip_filter_method(mipMapFilter);
}

id::texture_filter_method Texture::getMinFilter() const
{
    return m_sampler.min_filter_method();
}

void Texture::setMinFilter(id::texture_filter_method minFilter)
{
    if (minFilter != m_sampler.min_filter_method())
    {
        m_isSamplerDirty = true;
    }
    m_sampler.min_filter_method(minFilter);
}

id::texture_filter_method Texture::getMagFilter() const
{
    return m_sampler.mag_filter_method();
}

void Texture::setMagFilter(id::texture_filter_method magFilter)
{
    if (magFilter != m_sampler.mag_filter_method())
    {
        m_isSamplerDirty = true;
    }
    m_sampler.mag_filter_method(magFilter);
}

id::texture_address_mode Texture::getAddressModeS() const
{
    return m_sampler.address_mode_s();
}

void Texture::setAddressModeS(id::texture_address_mode addressModeS)
{
    if (addressModeS != m_sampler.address_mode_s())
    {
        m_isSamplerDirty = true;
    }
    m_sampler.address_mode_s(addressModeS);
}

id::texture_address_mode Texture::getAddressModeT() const
{
    return m_sampler.address_mode_t();
}

void Texture::setAddressModeT(id::texture_address_mode addressModeT)
{
    if (addressModeT != m_sampler.address_mode_t())
    {
        m_isSamplerDirty = true;
    }
    m_sampler.address_mode_t(addressModeT);
}

int Texture::getSourceHeight() const
{
    return m_sourceHeight;
}

int Texture::getSourceWidth() const
{
    return m_sourceWidth;
}

int Texture::getWidth() const
{
    return m_width;
}

int Texture::getHeight() const
{
    return m_height;
}

bool Texture::hasAlpha() const
{
    return m_hasAlpha;
}

void Texture::setName(const std::string& name)
{
    m_name = name;
}

const std::string& Texture::getName() const
{
    return m_name;
}

const id::texture_sampler& Texture::getSampler() const
{
    return m_sampler;
}

std::shared_ptr<SDL_Surface> Texture::getSource() const
{ return m_source; }

} // namespace Ego

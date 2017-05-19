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

/// @file egolib/Renderer/TextureSampler.cpp
/// @brief A texture sampler.
/// @author Michael Heilmann

#include "egolib/Renderer/TextureSampler.hpp"

namespace Ego {

TextureSampler::TextureSampler(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMapFilter,
		                       TextureAddressMode addressModeS, TextureAddressMode addressModeT,
                               float anisotropyLevels) : 
    m_minFilter(minFilter), m_magFilter(magFilter), m_mipMapFilter(mipMapFilter),
	m_addressModeS(addressModeS), m_addressModeT(addressModeT),
    m_anisotropyLevels(anisotropyLevels)
{}

TextureSampler::TextureSampler(const TextureSampler& other) :
    m_minFilter(other.m_minFilter), m_magFilter(other.m_magFilter), m_mipMapFilter(other.m_mipMapFilter),
	m_addressModeS(other.m_addressModeS), m_addressModeT(other.m_addressModeT),
    m_anisotropyLevels(other.m_anisotropyLevels)
{}

const TextureSampler& TextureSampler::operator=(const TextureSampler& other)
{
	m_minFilter = other.m_minFilter;
	m_magFilter = other.m_magFilter;
	m_mipMapFilter = other.m_mipMapFilter;
	m_addressModeS = other.m_addressModeS;
	m_addressModeT = other.m_addressModeT;
	m_anisotropyLevels = other.m_anisotropyLevels;
	return *this;
}

TextureFilter TextureSampler::getMinFilter() const
{
	return m_minFilter;
}

void TextureSampler::setMinFilter(TextureFilter minFilter)
{
    m_minFilter = minFilter;
}

TextureFilter TextureSampler::getMagFilter() const
{
	return m_magFilter;
}

void TextureSampler::setMagFilter(TextureFilter magFilter)
{
    m_magFilter = magFilter;
}

TextureFilter TextureSampler::getMipMapFilter() const
{
	return m_mipMapFilter;
}

void TextureSampler::setMipMapFilter(TextureFilter mipMapFilter)
{
    m_mipMapFilter = mipMapFilter;
}

TextureAddressMode TextureSampler::getAddressModeS() const
{
	return m_addressModeS;
}

void TextureSampler::setAddressModeS(TextureAddressMode addressModeS)
{
    m_addressModeS = addressModeS;
}

TextureAddressMode TextureSampler::getAddressModeT() const
{
	return m_addressModeT;
}

void TextureSampler::setAddressModeT(TextureAddressMode addressModeT)
{
    m_addressModeT = addressModeT;
}

float TextureSampler::getAnisotropyLevels() const
{
	return m_anisotropyLevels;
}

void TextureSampler::setAnisotropyLevels(float anisotropyLevels)
{
    m_anisotropyLevels = anisotropyLevels;
}

} // namespace Ego

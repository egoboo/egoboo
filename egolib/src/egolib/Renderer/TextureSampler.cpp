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

/// @file   egolib/Renderer/TextureSampler.cpp
/// @brief  A texture sampler.
/// @author Michael Heilmann

#include "egolib/Renderer/TextureSampler.hpp"

namespace Ego {

TextureSampler::TextureSampler(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMapFilter,
		                       TextureAddressMode addressModeS, TextureAddressMode addressModeT, float anisotropyLevel)
	: minFilter(minFilter), magFilter(magFilter), mipMapFilter(mipMapFilter),
	addressModeS(addressModeS), addressModeT(addressModeT), anisotropyLevel(anisotropyLevel) {
}

TextureSampler::TextureSampler(const TextureSampler& other)
	: minFilter(other.minFilter), magFilter(other.magFilter), mipMapFilter(other.mipMapFilter),
	addressModeS(other.addressModeS), addressModeT(other.addressModeT), anisotropyLevel(other.anisotropyLevel) {}

const TextureSampler& TextureSampler::operator=(const TextureSampler& other) {
	minFilter = other.minFilter;
	magFilter = other.magFilter;
	mipMapFilter = other.mipMapFilter;
	addressModeS = other.addressModeS;
	addressModeT = other.addressModeT;
	anisotropyLevel = other.anisotropyLevel;
	return *this;
}

TextureFilter TextureSampler::getMinFilter() const {
	return minFilter;
}

TextureFilter TextureSampler::getMagFilter() const {
	return magFilter;
}

TextureFilter TextureSampler::getMipMapFilter() const {
	return mipMapFilter;
}

TextureAddressMode TextureSampler::getAddressModeS() const {
	return addressModeS;
}

TextureAddressMode TextureSampler::getAddressModeT() const {
	return addressModeT;
}

float TextureSampler::getAnisotropyLevel() const {
	return anisotropyLevel;
}

} // namespace Ego

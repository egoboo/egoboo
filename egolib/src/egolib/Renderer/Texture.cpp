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
#include "egolib/Renderer/TextureSampler.hpp"

namespace Ego {

Texture::Texture(const std::string& name,
                 TextureType type, TextureAddressMode addressModeS, TextureAddressMode addressModeT,
                 int width, int height, int sourceWidth, int sourceHeight, std::shared_ptr<SDL_Surface> source,
                 bool hasAlpha) :
    _name(name),
    _type(type), _addressModeS(addressModeS), _addressModeT(addressModeT),
    _width(width), _height(height), _sourceWidth(sourceWidth), _sourceHeight(sourceHeight), _source(source),
    _minFilter(g_ogl_textureParameters.textureFilter.minFilter), _magFilter(g_ogl_textureParameters.textureFilter.magFilter), _mipMapFilter(g_ogl_textureParameters.textureFilter.mipMapFilter),
    _hasAlpha(hasAlpha)
{}

Texture::~Texture()
{}

TextureType Texture::getType() const {
    return _type;
}

TextureFilter Texture::getMipMapFilter() const {
    return _mipMapFilter;
}

void Texture::setMipMapFilter(TextureFilter mipMapFilter) {
    _mipMapFilter = mipMapFilter;
}

TextureFilter Texture::getMinFilter() const {
    return _minFilter;
}

void Texture::setMinFilter(TextureFilter minFilter) {
    _minFilter = minFilter;
}

TextureFilter Texture::getMagFilter() const {
    return _magFilter;
}

void Texture::setMagFilter(TextureFilter magFilter) {
    _magFilter = magFilter;
}

TextureAddressMode Texture::getAddressModeS() const {
    return _addressModeS;
}

void Texture::setAddressModeS(TextureAddressMode addressModeS) {
    _addressModeS = addressModeS;
}

TextureAddressMode Texture::getAddressModeT() const {
    return _addressModeT;
}

void Texture::setAddressModeT(TextureAddressMode addressModeT) {
    _addressModeT = addressModeT;
}

int Texture::getSourceHeight() const
{
    return _sourceHeight;
}

int Texture::getSourceWidth() const
{
    return _sourceWidth;
}

int Texture::getWidth() const
{
    return _width;
}

int Texture::getHeight() const
{
    return _height;
}

bool Texture::hasAlpha() const
{
    return _hasAlpha;
}

void Texture::setName(const std::string& name)
{
    _name = name;
}

const std::string& Texture::getName() const
{
    return _name;
}

} // namespace Ego

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

/// @file   egolib/Renderer/OpenGL/TextureUnit.cpp
/// @brief  Texture unit facade for OpenGL 2.1
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/TextureUnit.hpp"

namespace Ego
{
namespace OpenGL
{

TextureUnit::TextureUnit()
{}

TextureUnit::~TextureUnit()
{}

void TextureUnit::setActivated(oglx_texture_t *texture)
{
#if 0
    if (!texture)
    {
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        Ego::OpenGL::Utilities::bind(texture->_id, texture->_type, texture->_textureAddressModeS, texture->_textureAddressModeT);
    }
    Ego::OpenGL::Utilities::isError();
#endif
}

} // namespace OpenGL
} // namespace Ego

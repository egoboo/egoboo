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

/// @file egolib/Renderer/OpenGL/TextureUnit.cpp
/// @brief Implementation of a texture unit facade for OpenGL 2.1.
/// @author Michael Heilmann

#include "egolib/Renderer/OpenGL/TextureUnit.hpp"
#include "egolib/Renderer/OpenGL/Texture.hpp"
#include "egolib/Renderer/OpenGL/Utilities.hpp"
#include "egolib/Renderer/OpenGL/RendererInfo.hpp"

namespace Ego {
namespace OpenGL {

TextureUnit::TextureUnit(const std::shared_ptr<RendererInfo>& info) :
    m_info(info)
{}

TextureUnit::~TextureUnit()
{}

void TextureUnit::setActivated(Texture *texture)
{
    if (!texture)
    {
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        Utilities::clearError();
        GLenum target_gl;
        switch (texture->getType())
        {
            case idlib::texture_type::_2D:
                glEnable(GL_TEXTURE_2D);
                glDisable(GL_TEXTURE_1D);
                target_gl = GL_TEXTURE_2D;
                break;
            case idlib::texture_type::_1D:
                glEnable(GL_TEXTURE_1D);
                glDisable(GL_TEXTURE_2D);
                target_gl = GL_TEXTURE_1D;
                break;
            default:
                throw std::runtime_error("unreachable code reached");
        }
        if (Utilities::isError())
        {
            return;
        }
        glBindTexture(target_gl, texture->getId());
        if (Utilities::isError())
        {
            return;
        }
        Utilities2::setSampler(m_info, texture->getType(), texture->getSampler());
        if (Utilities::isError())
        {
            return;
        }
    }
    Utilities::isError();
}

void TextureUnit::setActivated(const Ego::Texture *texture)
{
    setActivated(const_cast<Texture *>(static_cast<const Texture *>(texture)));
}

} // namespace OpenGL
} // namespace Ego

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

void TextureUnit::setActivated(const oglx_texture_t *texture)
{
	if (!texture)
	{
		glDisable(GL_TEXTURE_1D);
		glDisable(GL_TEXTURE_2D);
	}
	else
	{
		auto anisotropy_enable = g_ogl_textureParameters.anisotropy_enable;
		auto anisotropy_level = g_ogl_textureParameters.anisotropy_level;
		Utilities::clearError();
		GLenum target_gl;
		switch (texture->getType())
		{
		case TextureType::_2D:
			glEnable(GL_TEXTURE_2D);
			glDisable(GL_TEXTURE_1D);
			target_gl = GL_TEXTURE_2D;
			break;
		case TextureType::_1D:
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
		glBindTexture(target_gl, texture->getTextureID());
		if (Utilities::isError())
		{
			return;
		}

		glTexParameteri(target_gl, GL_TEXTURE_WRAP_S, Utilities::toOpenGL(texture->getAddressModeS()));
		glTexParameteri(target_gl, GL_TEXTURE_WRAP_T, Utilities::toOpenGL(texture->getAddressModeT()));


		if (Utilities::isError())
		{
			return;
		}

		GLint minFilter_gl, magFilter_gl;
		Utilities::toOpenGL(texture->getMinFilter(), texture->getMagFilter(), texture->getMipMapFilter(), minFilter_gl, magFilter_gl);
		glTexParameteri(target_gl, GL_TEXTURE_MIN_FILTER, minFilter_gl);
		glTexParameteri(target_gl, GL_TEXTURE_MAG_FILTER, magFilter_gl);
		if (Ego::OpenGL::Utilities::isError())
		{
			return;
		}


		if (GL_TEXTURE_2D == target_gl && g_ogl_caps.anisotropic_supported && anisotropy_enable && anisotropy_level >= 1.0f)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy_level);
		}

		if (Ego::OpenGL::Utilities::isError())
		{
			return;
		}
	}
	Ego::OpenGL::Utilities::isError();
}

} // namespace OpenGL
} // namespace Ego

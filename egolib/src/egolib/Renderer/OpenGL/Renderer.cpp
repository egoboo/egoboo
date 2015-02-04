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

/// @file   egolib/Renderer/OpenGL/Renderer.cpp
/// @brief  OpenGL 2.1 based renderer
/// @author Michael Heilmann
#include "egolib/Renderer/OpenGL/Renderer.hpp"

namespace Ego
{
	namespace OpenGL
	{
		Renderer::~Renderer()
		{
			//dtor
		}

		void Renderer::multiplyMatrix(const fmat_4x4_t& matrix)
		{
			// fmat_4x4_t will not remain a simple array, hence the data must be packed explicitly to be passed
			// to the OpenGL API. However, currently this code is redundant.
			GLXmatrix t;
			for (size_t i = 0; i < 4; ++i)
			{
				for (size_t j = 0; j < 4; ++j)
				{
					t[i * 4 + j] = matrix.v[i * 4 + j];
				}
			}
			GL_DEBUG(glMultMatrixf)(t);
		}

		void Renderer::loadMatrix(const fmat_4x4_t& matrix)
		{
			// fmat_4x4_t will not remain a simple array, hence the data must be packed explicitly to be passed
			// to the OpenGL API. However, currently this code is redundant.
			GLXmatrix t;
			for (size_t i = 0; i < 4; ++i)
			{
				for (size_t j = 0; j < 4; ++j)
				{
					t[i * 4 + j] = matrix.v[i * 4 + j];
				}
			}
			GL_DEBUG(glLoadMatrixf)(t);
		}

		void Renderer::setColour(const Colour4f& colour)
		{
			GL_DEBUG(glColor4f(colour.getRed(), colour.getGreen(),
				               colour.getBlue(), colour.getAlpha()));
		}

		void Renderer::setDepthTestEnabled(bool enabled)
		{
			if (enabled)
			{
				GL_DEBUG(glEnable)(GL_DEPTH_TEST);
			}
			else
			{
				GL_DEBUG(glDisable)(GL_DEPTH_TEST);
			}
		}

	};

	const GLXvector4f white_vec = { 1.0f, 1.0f, 1.0f, 1.0f };
	const GLXvector4f black_vec = { 0.0f, 0.0f, 0.0f, 1.0f };
	const GLXvector4f red_vec   = { 1.0f, 0.0f, 0.0f, 1.0f };
	const GLXvector4f green_vec = { 0.0f, 1.0f, 0.0f, 1.0f };
	const GLXvector4f blue_vec  = { 0.0f, 0.0f, 1.0f, 1.0f };
};
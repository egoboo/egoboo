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

/// @file egolib/opengl/renderer.c
#include "egolib/opengl/renderer.h"

const GLXvector4f white_vec = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLXvector4f black_vec = { 0.0f, 0.0f, 0.0f, 1.0f };

/**
 * @brief
 *	Was the OpenGL renderer initialized?
 */
static bool initialized = false;

int Egoboo_Renderer_OpenGL_initialize()
{
	assert(!initialized);
	initialized = true;
	return 0;
}

void Egoboo_Renderer_OpenGL_uninitialize()
{
	assert(initialized);
}

void Egoboo_Renderer_OpenGL_multMatrix(const fmat_4x4_t *m)
{
	EGOBOO_ASSERT(NULL != m); /// @todo This currently still causes a warning because apparently @a NULL is defined as <tt>(0)</tt> instead of <tt>((void *)0)</tt> as it should be.
	// fmat_4x4_t will not remain a simple array, hence the data must be packed explicitly to be passed
	// to the OpenGL API. However, currently this code is redundant.
	GLXmatrix t;
	for (size_t i = 0; i < 4; ++i)
	{
		for (size_t j = 0; j < 4; ++j)
		{
			t[i * 4 + j] = m->v[i * 4 + j];
		}
	}
	GL_DEBUG(glMultMatrixf)(t);
}

void Egoboo_Renderer_OpenGL_loadMatrix(const fmat_4x4_t *m)
{
	EGOBOO_ASSERT(NULL != m); /// @todo This currently still causes a warning because apparently @a NULL is defined as <tt>(0)</tt> instead of <tt>((void *)0)</tt> as it should be.
	// fmat_4x4_t will not remain a simple array, hence the data must be packed explicitly to be passed
	// to the OpenGL API. However, currently this code is redundant.
	GLXmatrix t;
	for (size_t i = 0; i < 4; ++i)
	{
		for (size_t j = 0; j < 4; ++j)
		{
			t[i * 4 + j] = m->v[i * 4 + j];
		}
	}
	GL_DEBUG(glLoadMatrixf)(t);
}
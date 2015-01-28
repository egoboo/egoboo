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

/// @file egolib/opengl/renderer.h
#pragma once

#include "egolib/typedef.h"
#include "egolib/extensions/ogl_debug.h"
#include "egolib/extensions/ogl_extensions.h"
#include "egolib/extensions/ogl_include.h"
#include "egolib/extensions/ogl_texture.h"
#include "egolib/extensions/SDL_extensions.h"
#include "egolib/extensions/SDL_GL_extensions.h"
#include "egolib/_math.h"
#include "egolib/matrix.h"



/**
 * @ingroup egoboo-opengl
 * @brief
 *	The Egoboo OpenGL back-end.
 * @author
 *	Michael Heilmann
 */
namespace Ego
{
	/// The color "white" to be used with glColor4f.
	extern const GLXvector4f white_vec;
	/// The color "black" to be used with glColor4f.
	extern const GLXvector4f black_vec;
	/// The color "red"   to be used with glColor4f.
	extern const GLXvector4f red_vec;
	/// The color "green" to be used with glColor4f.
	extern const GLXvector4f green_vec;
	/// The color "blue"  to be used with glColor4f.
	extern const GLXvector4f blue_vec;
	/**
	 * @ingroup egoboo-opengl
	 * @brief
	 *	Initialize the OpenGL back-end.
	 * @return
	 *	@a 0 on success, a non-zero value on failure
	 */
	int Renderer_OpenGL_initialize();

	/**
	 * @ingroup egoboo-opengl
	 * @brief
	 *	Uninitialize the OpenGL back-end.
	 */
	void Renderer_OpenGL_uninitialize();

	/**
	 * @ingroup egoboo-opengl
	 * @brief
	 *	Multiply the OpenGL matrix with the given matrix (using glMultMatrix).
	 * @param m
	 *	the matrix
	 */
	void Renderer_OpenGL_multMatrix(const fmat_4x4_t& m);

	/**
	 * @ingroup egoboo-opengl
	 * @brief
	 *	Set the OpenGL matrix to the given matrix (using glLoadMatrix).
	 * @param m
	 *	the matrix
	 */
	void Renderer_OpenGL_loadMatrix(const fmat_4x4_t& m);
#if 0
	/**
	 * @ingroup egolib-opengl
	 * @brief
	 *	Set the colour.
	 * @param c
	 *	the colour
	 */
	void Renderer_OpenGL_setColour(const Colour4f& c);
#endif
};
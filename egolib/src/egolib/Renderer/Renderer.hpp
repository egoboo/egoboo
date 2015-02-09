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

/// @file   egolib/Renderer/Renderer.hpp
/// @brief  Common interface of all renderers
/// @author Michael Heilmann
#pragma once

#include "egolib/_math.h"
#include "egolib/math/Colour4f.hpp"
#include "egolib/matrix.h"
#include "egolib/platform.h"
#include "egolib/typedef.h"
#include "egolib/extensions/ogl_debug.h"
#include "egolib/extensions/ogl_extensions.h"
#include "egolib/extensions/ogl_include.h"
#include "egolib/extensions/ogl_texture.h"
#include "egolib/extensions/SDL_extensions.h"
#include "egolib/extensions/SDL_GL_extensions.h"
#include "egolib/_math.h"
#include "egolib/math/Colour4f.hpp"
#include "egolib/matrix.h"

namespace Ego
{
	using namespace Math;
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
	
	class Renderer
	{

	private:

		/**
		 * @brief
		 *	The singleton instance of this renderer.
		 * @remark
		 *	Intentionally private.
		 */
		static Renderer *singleton;

	protected:

		/**
		 * @brief
		 *	Construct this renderer.
		 * @remark
		 *	Intentionally protected.
		 */
		Renderer();

		/**
		 * @brief
		 *	Destruct this renderer.
		 * @remark
		 *	Intentionally protected
		 */
		virtual ~Renderer();

	public:

		/**
		 * @brief
		 *	Set the current colour.
		 * @param colour
		 *	the current colour
		 */
		virtual void setColour(const Colour4f& colour) = 0;

		/**
		 * @brief
		 *	Enable/disable depth tests and depth buffer updates.
		 * @param enabled
		 *	@a true enables depth tests and depth buffer updates,
		 *	@a false disables them
		 */
		virtual void setDepthTestEnabled(bool enabled) = 0;

		/**
		 * @brief
		 *	Enable/disable stencil test and stencil buffer updates.
		 * @param enable
		 *	@a true enables stencil tests and stencil buffer updates,
		 *	@a false disables them
		 */
		virtual void setStencilTestEnabled(bool enabled) = 0;

		/**
		 * @brief
		 *	Enable/disable scissor tests.
		 * @param enable
		 *	@a true enables scissor tests,
		 *	@a false disables then
		 */
		virtual void setScissorTestEnabled(bool enabled) = 0;

		/**
		 * @brief
		 *	Replace the current matrix with the given matrix.
		 * @param matrix
		 *	the matrix
		 */
		virtual void loadMatrix(const fmat_4x4_t& matrix) = 0;

		/**
		 * @brief
		 *	Multiply the current matrix with the given matrix.
		 * @param matrix
		 *	the matrix
		 */
		virtual void multiplyMatrix(const fmat_4x4_t& matrix) = 0;

		/**
		 * @brief
		 *	Start-up the renderer.
		 * @remark
		 *	If the renderer is started-up, a call to this method is a no-op.
		 */
		static void startUp();

		/**
		 * @brief
		 *	Get the renderer.
		 * @return
		 *	the renderer
		 * @pre
		 *	The renderer must be started-up.
		 * @warning
		 *	Shutting-down the renderer will invalidate any pointers returned by calls to this method prior to shut-down.
		 */
		static Renderer *getSingleton();

		/**
		 * @brief
		 *	Shut-down the renderer.
		 * @remark
		 *	If the renderer is not started-up, a calll to this method is a no-op.
		 */
		static void shutDown();
	};
};

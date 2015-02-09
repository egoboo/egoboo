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

/// @file   egolib/Renderer/OpenGL/Renderer.hpp
/// @brief  OpenGL 2.1 based renderer
/// @author Michael Heilmann
#pragma once

#include "egolib/Renderer/Renderer.hpp"
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



/**
 * @ingroup egoboo-opengl
 * @brief
 *	The Egoboo OpenGL back-end.
 * @author
 *	Michael Heilmann
 */
namespace Ego
{
	using namespace Math;

	namespace OpenGL
	{
		class Renderer : public Ego::Renderer
		{
		public:
			virtual ~Renderer();

			/** @copydoc Ego::Renderer::multMatrix */
			void multiplyMatrix(const fmat_4x4_t& matrix);

			/** @copydoc Ego::Renderer::loadMatrix */
			void loadMatrix(const fmat_4x4_t& matrix);

			/** @copydoc Ego::Renderer::setColour */
			virtual void setColour(const Colour4f& colour);

			/** @copydoc Ego::Renderer::setDepthTestEnabled */
			virtual void setDepthTestEnabled(bool enabled);

			/** @copydoc Ego::Renderer::setStencilTestEnabled */
			virtual void setStencilTestEnabled(bool enabled);

			/** @copydoc Ego::Renderer::setScissorTestEnabled */
			virtual void setScissorTestEnabled(bool enabled);
		};
	};

};
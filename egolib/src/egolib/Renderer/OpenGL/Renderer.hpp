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
        protected:
            /**
             * @brief
             *  The list of OpenGL extensions supported by this OpenGL implementation.
             */
            std::unordered_set<std::string> _extensions;
            /**
             * @brief
             *  The name of the vendor of this OpenGL implementation.
             */
            std::string _vendor;
            /**
             * @brief
             *  The name of this OpenGL implementation.
             */
            std::string _name;
		public:
            /**
             * @brief
             *  Construct this OpenGL renderer.
             */
            Renderer();
            /**
             * @brief
             *  Destruct this OpenGL renderer.
             */
			virtual ~Renderer();

            // Disable copying class.
            Renderer(const Renderer&) = delete;
            Renderer& operator=(const Renderer&) = delete;

            /** @copydoc Ego::Renderer::setClearColour */
            virtual void setClearColour(const Colour4f& colour) override;

            /** @copydoc Ego::Renderer::setAlphaTestEnabled */
            virtual void setAlphaTestEnabled(bool enabled) override;

            /** @copydoc Ego::Renderer::setBlendingEnabled */
            virtual void setBlendingEnabled(bool enabled) override;

			/** @copydoc Ego::Renderer::multMatrix */
			virtual void multiplyMatrix(const fmat_4x4_t& matrix) override;

			/** @copydoc Ego::Renderer::loadMatrix */
            virtual void loadMatrix(const fmat_4x4_t& matrix) override;

			/** @copydoc Ego::Renderer::setColour */
            virtual void setColour(const Colour4f& colour) override;

			/** @copydoc Ego::Renderer::setDepthTestEnabled */
            virtual void setDepthTestEnabled(bool enabled) override;

			/** @copydoc Ego::Renderer::setStencilTestEnabled */
            virtual void setStencilTestEnabled(bool enabled) override;

			/** @copydoc Ego::Renderer::setScissorTestEnabled */
            virtual void setScissorTestEnabled(bool enabled) override;
		};
	};

};
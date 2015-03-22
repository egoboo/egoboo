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
#include "egolib/Core/StringUtilities.hpp"
#include "egolib/Core/CollectionUtilities.hpp"

// The following code ensures that for each OpenGL function variable static PF...PROC gl... = NULL; is declared/defined.
#define GLPROC(variable,type,name) \
    static type variable = NULL;
#include "egolib/Renderer/OpenGL/OpenGL.inl"
#undef GLPROC

namespace Ego
{
    namespace OpenGL
    {
        // The following function dynamically links the OpenGL function.
        static bool link()
        {
            static bool linked = false;
            if (!linked)
            {
#define GLPROC(variable,type,name) \
                variable = (type)SDL_GL_GetProcAddress(name); \
                if (!variable) \
                { \
                    return false; \
                }
#include "egolib/Renderer/OpenGL/OpenGL.inl"
#undef GLPROC
            }
            linked = true;
            return true;
        }

    }
}

namespace Ego
{
	namespace OpenGL
	{

        /**
         * @brief
         *  Get the name of this OpenGL implementation.
         * @return
         *  the name of this OpenGL implementation
         */
        std::string getName()
        {
            while (GL_NO_ERROR != glGetError()) {}
            const GLubyte *bytes = glGetString(GL_RENDERER);
            GLenum error = glGetError();
            if (GL_NO_ERROR != error)
            {
                throw std::runtime_error("unable to acquire renderer back-end information");
            }
            return (const char *)bytes;
        }
        /**
         * @brief
         *  Get the name of the vendor of this OpenGL implementation.
         * @return
         *  the name of the vendor of this OpenGL implementation
         */
        std::string getVendor()
        {
            while (GL_NO_ERROR != glGetError()) {}
            const GLubyte *bytes = glGetString(GL_RENDERER);
            GLenum error = glGetError();
            if (GL_NO_ERROR != error)
            {
                throw std::runtime_error("unable to acquire renderer back-end information");
            }
            return (const char *)bytes;
        }
        /**
         * @brief
         *  Get the list of extensions supported by this OpenGL implementation.
         * @param [out] extensions
         *  a set of strings
         * @post
         *  the set of extensions supported by this OpenGL implementation was added to the set
         */
        std::vector<std::string> getExtensions()
        {
            while (GL_NO_ERROR != glGetError()) {}
            const GLubyte *bytes = glGetString(GL_EXTENSIONS);
            GLenum error = glGetError();
            if (GL_NO_ERROR != error)
            {
                throw std::runtime_error("unable to acquire renderer back-end information");
            }
            return Ego::split(std::string((const char *)bytes),std::string(" "));
        }

        Renderer::Renderer() :
            _extensions(make_unordered_set(getExtensions())),
            _vendor(getVendor()),
            _name(getName())
        {
            Ego::OpenGL::link();
        }

		Renderer::~Renderer()
		{
			//dtor
		}

        void Renderer::setAlphaTestEnabled(bool enabled)
        {
            if (enabled)
            {
                GL_DEBUG(glEnable)(GL_ALPHA_TEST);
            }
            else
            {
                GL_DEBUG(glDisable)(GL_ALPHA_TEST);
            }
        }

        void Renderer::setBlendingEnabled(bool enabled)
        {
            if (enabled)
            {
                GL_DEBUG(glEnable)(GL_BLEND);
            }
            else
            {
                GL_DEBUG(glDisable)(GL_BLEND);
            }
        }

        void Renderer::setClearColour(const Colour4f& colour)
        {
            GL_DEBUG(glClearColor)(colour.getRed(), colour.getGreen(),
                                   colour.getBlue(), colour.getAlpha());
        }

        void Renderer::setClearDepth(float depth)
        {
            GL_DEBUG(glClearDepth)(depth);
        }

		void Renderer::setColour(const Colour4f& colour)
		{
			GL_DEBUG(glColor4f)(colour.getRed(), colour.getGreen(),
				                colour.getBlue(), colour.getAlpha());
		}

        void Renderer::setCullingMode(CullingMode mode)
        {
            switch (mode)
            {
            case CullingMode::None:
                GL_DEBUG(glDisable)(GL_CULL_FACE);
                break;
            case CullingMode::Front:
                GL_DEBUG(glEnable)(GL_CULL_FACE);
                GL_DEBUG(glCullFace)(GL_FRONT);
                break;
            case CullingMode::Back:
                GL_DEBUG(glEnable)(GL_CULL_FACE);
                GL_DEBUG(glCullFace)(GL_BACK);
                break;
            case CullingMode::BackAndFront:
                GL_DEBUG(glEnable)(GL_CULL_FACE);
                GL_DEBUG(glCullFace)(GL_FRONT_AND_BACK);
                break;
            };
            
        }

        void Renderer::setDepthFunction(CompareFunction function)
        {
            switch (function)
            {
            case CompareFunction::AlwaysFail:
                GL_DEBUG(glDepthFunc)(GL_NEVER);
                break;
            case CompareFunction::AlwaysPass:
                GL_DEBUG(glDepthFunc)(GL_ALWAYS);
                break;
            case CompareFunction::Less:
                GL_DEBUG(glDepthFunc)(GL_LESS);
                break;
            case CompareFunction::LessOrEqual:
                GL_DEBUG(glDepthFunc)(GL_LEQUAL);
                break;
            case CompareFunction::Equal:
                GL_DEBUG(glDepthFunc)(GL_EQUAL);
                break;
            case CompareFunction::NotEqual:
                GL_DEBUG(glDepthFunc)(GL_NOTEQUAL);
                break;
            case CompareFunction::GreaterOrEqual:
                GL_DEBUG(glDepthFunc)(GL_GEQUAL);
                break;
            case CompareFunction::Greater:
                GL_DEBUG(glDepthFunc)(GL_GREATER);
                break;
            };
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

        void Renderer::setDepthWriteEnabled(bool enabled)
        {
            GL_DEBUG(glDepthMask)(enabled ? GL_TRUE : GL_FALSE);
        }

        void Renderer::setScissorRectangle(float left, float bottom, float width, float height)
        {
            if (width < 0)
            {
                throw std::invalid_argument("width < 0");
            }
            if (height < 0)
            {
                throw std::invalid_argument("height < 0");
            }
            GL_DEBUG(glScissor)(left, bottom, width, height);
        }

        void Renderer::setScissorTestEnabled(bool enabled)
        {
            if (enabled)
            {
                GL_DEBUG(glEnable)(GL_SCISSOR_TEST);
            }
            else
            {
                GL_DEBUG(glDisable)(GL_SCISSOR_TEST);
            }
        }

        void Renderer::setStencilMaskBack(uint32_t mask)
        {
            static_assert(sizeof(GLint) >= sizeof(uint32_t), "GLint is smaller than uint32_t");
            GL_DEBUG(glStencilMaskSeparate)(GL_BACK, mask);
        }

        void Renderer::setStencilMaskFront(uint32_t mask)
        {
            static_assert(sizeof(GLint) >= sizeof(uint32_t), "GLint is smaller than uint32_t");
            GL_DEBUG(glStencilMaskSeparate)(GL_FRONT, mask);
        }

		void Renderer::setStencilTestEnabled(bool enabled)
		{
			if (enabled)
			{
				GL_DEBUG(glEnable)(GL_STENCIL_TEST);
			}
			else
			{
				GL_DEBUG(glDisable)(GL_STENCIL_TEST);
			}
		}

        void Renderer::setViewportRectangle(float left, float bottom, float width, float height)
        {
            if (width < 0)
            {
                throw std::invalid_argument("width < 0");
            }
            if (height < 0)
            {
                throw std::invalid_argument("height < 0");
            }
            GL_DEBUG(glViewport)(left, bottom, width, height);
        }

        void Renderer::setWindingMode(WindingMode mode)
        {
            switch (mode)
            {
            case WindingMode::Clockwise:
                GL_DEBUG(glFrontFace)(GL_CW);
                break;
            case WindingMode::AntiClockwise:
                GL_DEBUG(glFrontFace)(GL_CCW);
                break;
            }
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

	};

};

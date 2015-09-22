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
#include "egolib/Renderer/OpenGL/AccumulationBuffer.hpp"
#include "egolib/Renderer/OpenGL/ColourBuffer.hpp"
#include "egolib/Renderer/OpenGL/DepthBuffer.hpp"
#include "egolib/Renderer/OpenGL/TextureUnit.hpp"
#include "egolib/Renderer/OpenGL/Texture.hpp"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/Extensions/SDL_GL_extensions.h"
#include "egolib/_math.h"

/**
 * @ingroup egoboo-opengl
 * @brief
 *    The Egoboo OpenGL back-end.
 * @author
 *    Michael Heilmann
 */
namespace Ego
{
namespace OpenGL
{

using namespace Math;

struct Capabilities {
	/**
	 * @brief
	 *  Get the set of extension strings of extensions supported by this OpenGL implementation.
	 * @return
	 *	the set of extension strings
	 */
	static std::unordered_set<std::string> getExtensions();
	/**
	 * @brief
	 *  Get the name of the vendor of this OpenGL implementation.
	 * @return
	 *  the name of the vendor of this OpenGL implementation
	 */
	static std::string getVendor();
	/**
	 * @brief
	 *  Get the name of this OpenGL implementation.
	 * @return
	 *  the name of this OpenGL implementation
	 */
	static std::string getName();
	/**
	 * @brief
	 *  Get the version of this OpenGL implementation.
	 * @return
	 *  the version of this OpenGL implementation
	 */
	static std::string getVersion();
};

class Renderer : public Ego::Renderer
{
protected:
    /**
     * @brief
     *  The accumulation buffer facade.
     */
    AccumulationBuffer _accumulationBuffer;
    /**
     * @brief
     *  The colour buffer facade.
     */
    ColourBuffer _colourBuffer;
    /**
     * @brief
     *  The depth buffer facade.
     */
    DepthBuffer _depthBuffer;
    /**
     * @brief
     *  The texture unit facade
     */
    TextureUnit _textureUnit;
    /**
     * @brief
     *  The set of OpenGL extensions supported by this OpenGL implementation.
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
	/**
	 * @brief
	 *	The version of this OpenGL implementation.
	 */
	std::string _version;
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

public:

    /** @copydoc Ego::Renderer::getAccumulationBuffer() */
    virtual Ego::AccumulationBuffer& getAccumulationBuffer() override;

    /** @copydoc Ego::Renderer::getColourBuffer */
    virtual Ego::ColourBuffer& getColourBuffer() override;

    /** @copydoc Ego::Renderer::getDepthBuffer() */
    virtual Ego::DepthBuffer& getDepthBuffer() override;

	/** @copydoc Ego::Renderer::getTextureUnit() */
	virtual Ego::TextureUnit& getTextureUnit() override;

    /** @copydoc Ego::Renderer::setAlphaTestEnabled */
    virtual void setAlphaTestEnabled(bool enabled) override;

	/** @copydoc Ego::Renderer::setAlphaFunction */
	virtual void setAlphaFunction(CompareFunction function, float value) override;

    /** @copydoc Ego::Renderer::setBlendingEnabled */
    virtual void setBlendingEnabled(bool enabled) override;

	/** @copydoc Ego::Renderer::setSourceBlendFunction */
	virtual void setBlendFunction(BlendFunction sourceColour, BlendFunction sourceAlpha,
		                          BlendFunction destinationColour, BlendFunction destinationAlpha) override;

    /** @copydoc Ego::Renderer::setColour */
    virtual void setColour(const Colour4f& colour) override;

    /** @copydoc Ego::Renderer::setCullingMode */
    virtual void setCullingMode(CullingMode mode) override;

    /** @copydoc Ego::Renderer::setDepthFunction */
    virtual void setDepthFunction(CompareFunction function) override;

    /** @copydoc Ego::Renderer::setDepthTestEnabled */
    virtual void setDepthTestEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setDepthWriteEnabled */
    virtual void setDepthWriteEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setScissorTestEnabled */
    virtual void setScissorTestEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setScissorRectangle */
    virtual void setScissorRectangle(float left, float bottom, float width, float height) override;

    /** @copydoc Ego::Renderer::setStencilMaskBack */
    virtual void setStencilMaskBack(uint32_t mask) override;

    /** @copydoc Ego::Renderer::setStencilMaskFront */
    virtual void setStencilMaskFront(uint32_t mask) override;

    /** @copydoc Ego::Renderer::setStencilTestEnabled */
    virtual void setStencilTestEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setViewportRectangle */
    virtual void setViewportRectangle(float left, float bottom, float width, float height) override;

    /** @copydoc Ego::Renderer::setWindingMode */
    virtual void setWindingMode(WindingMode mode) override;

    /** @copydoc Ego::Renderer::loadMatrix */
    virtual void loadMatrix(const Matrix4f4f& matrix) override;

    /** @copydoc Ego::Renderer::multMatrix */
    virtual void multiplyMatrix(const Matrix4f4f& matrix) override;

    /** @copydoc Ego::Renderer::setPerspectiveCorrectionEnabled */
    virtual void setPerspectiveCorrectionEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setDitheringEnabled  */
    virtual void setDitheringEnabled(bool enabled) override;

	/** @copydoc Ego::Renderer::setPointSmoothEnabled */
	virtual void setPointSmoothEnabled(bool enabled) override;

	/** @copydoc Ego::Renderer::setLineSmoothEnabled */
	virtual void setLineSmoothEnabled(bool enabled) override;

	/** @copydoc Ego::Renderer::setLineWidth */
	virtual void setLineWidth(float width) override;

	/** @copydoc Ego::Renderer::setPointSize */
	virtual void setPointSize(float size) override;

	/** @copydoc Ego::Renderer::setPolygonSmoothEnabled */
	virtual void setPolygonSmoothEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setMultisamplesEnabled */
    virtual void setMultisamplesEnabled(bool enabled) override;

	/** @copydoc Ego::Renderer::setLightingEnabled */
	virtual void setLightingEnabled(bool enabled) override;

	/** @copydoc Ego::Renderer::setRasterizationMode */
	virtual void setRasterizationMode(RasterizationMode mode) override;

    /** @copydoc Ego::Renderer::setGouraudShadingEnabled */
    virtual void setGouraudShadingEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::render */
    virtual void render(VertexBuffer& vertexBuffer, PrimitiveType primitiveType, size_t index, size_t length) override;

private:
	GLenum toOpenGL(BlendFunction source);

}; // class Renderer

} // namespace OpenGL
} // namespace Ego

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
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_texture.h"
#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/Extensions/SDL_GL_extensions.h"
#include "egolib/_math.h"
#include "egolib/Math/Colour4f.hpp"
#include "egolib/Math/Matrix44.hpp"



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

class AccumulationBuffer : public Ego::AccumulationBuffer
{
public:
    AccumulationBuffer();
    virtual ~AccumulationBuffer();
public:
    /** @copydoc Ego::Buffer<Colour4f>::clear */
    virtual void clear() override;
    /** @copydoc Ego::Buffer<Colour4f>::setClearValue */
    virtual void setClearValue(const Colour4f& value) override;
};

class ColourBuffer : public Ego::ColourBuffer
{
public:
    ColourBuffer();
    virtual ~ColourBuffer();
public:
    /** @copydoc Ego::Buffer<Colour4f>::clear */
    virtual void clear() override;
    /** @copydoc Ego::Buffer<Colour4f>::setClearValue */
    virtual void setClearValue(const Colour4f& value) override;
};

class DepthBuffer : public Ego::DepthBuffer
{
public:
    DepthBuffer();
    virtual ~DepthBuffer();
public:
    /** @copydoc Ego::Buffer<float>::clear */
    virtual void clear() override;
    /** @copydoc Ego::Buffer<float>::setClearValue */
    virtual void setClearValue(const float& value) override;
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
     *  The colour buffe facade.
     */
    ColourBuffer _colourBuffer;
    /**
     * @brief
     *  The depth buffer facade.
     */
    DepthBuffer _depthBuffer;
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

public:

    /** @copydoc Ego::Renderer::getAccumulationBuffer() */
    virtual Ego::AccumulationBuffer& getAccumulationBuffer() override;

    /** @copydoc Ego::Renderer::getColourBuffer */
    virtual Ego::ColourBuffer& getColourBuffer() override;

    /** @copydoc Ego::Renderer::getDepthBuffer() */
    virtual Ego::DepthBuffer& getDepthBuffer() override;

    /** @copydoc Ego::Renderer::setAlphaTestEnabled */
    virtual void setAlphaTestEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setBlendingEnabled */
    virtual void setBlendingEnabled(bool enabled) override;

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
    virtual void loadMatrix(const fmat_4x4_t& matrix) override;

    /** @copydoc Ego::Renderer::multMatrix */
    virtual void multiplyMatrix(const fmat_4x4_t& matrix) override;

}; // class Renderer

} // namespace OpenGL
} // namespace Ego

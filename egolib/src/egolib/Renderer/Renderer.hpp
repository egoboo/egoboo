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
/// @brief  Common interface of all renderers.
/// @author Michael Heilmann
#pragma once

#include "egolib/_math.h"
#include "egolib/Math/Colour4f.hpp"
#include "egolib/Math/Matrix44.hpp"
#include "egolib/Renderer/CompareFunction.hpp"
#include "egolib/Renderer/CullingMode.hpp"
#include "egolib/Renderer/WindingMode.hpp"
#include "egolib/platform.h"
#include "egolib/typedef.h"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_texture.h"
#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/Extensions/SDL_GL_extensions.h"

namespace Ego
{

using namespace Math;

/**
 * @brief
 *  A facade for internal buffers like the accumulation buffer, the colour buffer or the depth buffer.
 */
template <typename DataType>
class Buffer
{
protected:
    /**
     * @brief
     *  Construct this buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    Buffer()
    {}
    /**
     * @brief
     *  Destruct this buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    virtual ~Buffer()
    {}
public:
    /**
     * @brief
     *  Clear this buffer using its clear value.
     */
    virtual void clear() = 0;
    /**
     * @brief
     *  Set the clear value of this buffer.
     * @param value
     *  the clear value of this buffer
     */
    virtual void setClearValue(const DataType& value) = 0;
#if 0
    /**
     * @brief
     *  Get the clear value of this buffer.
     * @return
     *  the clear value of this buffer
     */
    virtual const DataType& getClearValue() const = 0;
#endif
};

class AccumulationBuffer : public Ego::Buffer<Colour4f>
{
protected:
    /**
     * @brief
     *  Construct this accumulation buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    AccumulationBuffer();
    /**
     * @brief
     *  Destruct this accumulation buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    virtual ~AccumulationBuffer();
};

class ColourBuffer : public Ego::Buffer<Colour4f>
{
protected:
    /**
     * @brief
     *  Construct this colour buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    ColourBuffer();
    /**
     * @brief
     *  Destruct this colour buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    virtual ~ColourBuffer();
};

class DepthBuffer : public Ego::Buffer<float>
{
protected:
    /**
     * @brief
     *  Construct this depth buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    DepthBuffer();
    /**
     * @brief
     *  Destruct this depth buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    virtual ~DepthBuffer();
};

class Renderer
{

private:

    /**
     * @brief
     *  The singleton instance of this renderer.
     * @remark
     *  Intentionally private.
     */
    static Renderer *singleton;

protected:

    /**
     * @brief
     *  Construct this renderer.
     * @remark
     *  Intentionally protected.
     */
    Renderer();

    /**
     * @brief
     *  Destruct this renderer.
     * @remark
     *  Intentionally protected
     */
    virtual ~Renderer();

public:

    // Disable copying class.
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

public:

    /**
     * @brief
     *  Get the accumulation buffer (facade).
     * @return
     *  the accumulation buffer (facade)
     */
    virtual AccumulationBuffer& getAccumulationBuffer() = 0;

    /**
     * @brief
     *  Get the colour buffer (facade).
     * @return
     *  the colour buffer (facade)
     */
    virtual ColourBuffer& getColourBuffer() = 0;

    /**
     * @brief
     *  Get the depth buffer (facade).
     * @return
     *  the depth buffer (facade)
     */
    virtual DepthBuffer& getDepthBuffer() = 0;

    /**
     * @brief
     *  Enable/disable alpha tests.
     * @param enabled
     *  @a true enables alpha tests,
     *  @a false disables them
     */
    virtual void setAlphaTestEnabled(bool enabled) = 0;

    /**
     * @brief
     *  Enable/disable blending.
     * @param enabled
     *  @a true enables blending,
     *  @a false disables it
     */
    virtual void setBlendingEnabled(bool enabled) = 0;

    /**
     * @brief
     *  Set the current colour.
     * @param colour
     *  the current colour
     */
    virtual void setColour(const Colour4f& colour) = 0;

    /**
     * @brief
     *  Set the culling mode.
     * @param mode
     *  the culling mode
     */
    virtual void setCullingMode(CullingMode mode) = 0;

    /**
     * @brief
     *  Set the depth test compare function.
     * @param function
     *  the depth test compare function
     */
    virtual void setDepthFunction(CompareFunction function) = 0;

    /**
     * @brief
     *    Enable/disable depth tests and depth buffer updates.
     * @param enabled
     *    @a true enables depth tests and depth buffer updates,
     *    @a false disables them
     */
    virtual void setDepthTestEnabled(bool enabled) = 0;

    /**
     * @brief
     *  Enable/disable the depth buffer writes.
     * @param enable
     *    @a true enables scissor tests,
     *    @a false disables then
     */
    virtual void setDepthWriteEnabled(bool enabled) = 0;

    /**
     * @brief
     *  Set the scissor rectangle.
     * @param left, bottom
     *  the left/bottom corner of the scissor rectangle
     * @param width, height
     *  the width and height of the scissor rectangle
     * @throw std::invalid_argument
     *  if @a width or @a height are smaller than @a 0
     */
    virtual void setScissorRectangle(float left, float bottom, float width, float height) = 0;

    /**
     * @brief
     *    Enable/disable scissor tests.
     * @param enable
     *    @a true enables scissor tests,
     *    @a false disables then
     */
    virtual void setScissorTestEnabled(bool enabled) = 0;

    /**
     * @brief
     *  Set the stencil bit mask affecting back-facing polygons.
     * @param mask
     *  the mask
     * @see
     *  Ego::Renderer::setStencilMaskFront
     */
    virtual void setStencilMaskBack(uint32_t mask) = 0;

    /**
     * @brief
     *  Set the stencil bit mask affecting all primitives except of back-facing polygons.
     * @param mask
     *  the mask
     * @remark
     *  For a stencil buffer of @a n bits, the least @a n bits in the mask
     *  are the mask.  Where a @a 1 appears in the mask, it's possible to
     *  write to the corresponding bit in the stencil buffer. Where a @a 0
     *  appears, the corresponding bit is write-protected.
     */
    virtual void setStencilMaskFront(uint32_t mask) = 0;

    /**
     * @brief
     *  Enable/disable stencil test and stencil buffer updates.
     * @param enable
     *  @a true enables stencil tests and stencil buffer updates,
     *  @a false disables them
     */
    virtual void setStencilTestEnabled(bool enabled) = 0;

    /**
     * @brief
     *  Set the viewport rectangle.
     * @param left, bottom
     *  the left/bottom corner of the viewport rectangle
     * @param width, height
     *  the width and height of the viewport rectangle
     * @throw std::invalid_argument
     *  if @a width or @a height are smaller than @a 0
     */
    virtual void setViewportRectangle(float left, float bottom, float width, float height) = 0;

    /**
     * @brief
     *  Set the winding mode.
     * @param mode
     *  the winding mode
     */
    virtual void setWindingMode(WindingMode mode) = 0;

    /**
     * @brief
     *  Replace the current matrix with the given matrix.
     * @param matrix
     *  the matrix
     */
    virtual void loadMatrix(const fmat_4x4_t& matrix) = 0;

    /**
     * @brief
     *  Multiply the current matrix with the given matrix.
     * @param matrix
     *  the matrix
     */
    virtual void multiplyMatrix(const fmat_4x4_t& matrix) = 0;

    /**
     * @brief
     *  Start-up the renderer.
     * @remark
     *  If the renderer is initialized, a call to this method is a no-op.
     */
    static void initialize();

    /**
     * @brief
     *  Get the renderer.
     * @return
     *  the renderer
     * @pre
     *  The renderer must be initialized.
     * @warning
     *  Uninitializing the renderer will invalidate any references returned by calls to this method prior to uninitialization.
     */
    static Renderer& get();

    /**
     * @brief
     *    Shut-down the renderer.
     * @remark
     *    If the renderer is not initialized, a calll to this method is a no-op.
     */
    static void uninitialize();
};

} // namespace Ego

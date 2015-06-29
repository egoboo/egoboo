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
#include "egolib/Renderer/RasterizationMode.hpp"
#include "egolib/Renderer/CullingMode.hpp"
#include "egolib/Renderer/WindingMode.hpp"
#include "egolib/Renderer/PrimitiveType.hpp"
#include "egolib/Graphics/VertexBuffer.hpp"
#include "egolib/Renderer/Texture.hpp"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/ogl_include.h"
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
class Buffer : public Id::NonCopyable
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

/**
 * @brief
 *  A facade for an accumulation buffer.
 */
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

/**
 * @brief
 *  A facade for a colour buffer.
 */
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

/**
 * @brief
 *  A facade for an depth buffer.
 */
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

/**
 * @brief
 *  A facade for a texture unit.
 */
class TextureUnit
{

protected:

    /**
     * @brief
     *  Construct this texture unit (facade).
     * @remark
     *  Intentionally protected.
     */
    TextureUnit();
    
    /**
     * @brief
     *  Destruct this texture unit (facade).
     * @remark
     *  Intentionally protected.
     */
    virtual ~TextureUnit();

public:

    /**
     * @brief
     *  Activate/deactivate  a texture unit.
     * @param texture
     *  a pointer to a texture or a null pointer
     * @post
     *  If a pointer to a texture was passed,
     *  the texture unit was activated using that texture.
     *  If a null pointer was passed,
     *  the texture unit was deactivated.
     * @todo
     *  As there is still no proper reference counting,
     *  the caller has to make sure that a texture object is valid until
     *  the texture unit is deactivated.
     */
    virtual void setActivated(oglx_texture_t *texture) = 0;

};

class Renderer;

class RendererFactory
{
public:
    Renderer *operator()();
};

class Renderer : public Ego::Core::Singleton<Renderer,RendererFactory>
{

protected:
    // Befriend with the singleton to grant access to Renderer::~Renderer.
    using TheSingleton = Ego::Core::Singleton<Renderer, RendererFactory>;
    friend TheSingleton;
    // Befriend with the factory to grant access to Renderer::Renderer.
    using TheFactory = RendererFactory;
    friend RendererFactory;

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
	 *	Set the alpha compare function.
	 * @param function
	 *	the alpha compare function
	 * @param value
	 *	the reference alpha value that incoming alpha values are compared to.
	 *	Must be within the bounds of @a 0.0f (inclusive) and @a 1.0f (inclusive).
	 * @remark
	 *	The alpha compare function is used to compare alpha value of an incoming fragment to a reference alpha value.
	 *	The outcome of this test is binary, either the incoming fragment passes or fails the comparison.
	 *
	 *	The alpha compare functions are as follows:
	 *	<ul>
	 *		<li>Ego::CompareFunction::AlwaysFail:
	 *		The incoming alpha value never passes (regardless of the reference alpha value)</li>
	 *		<li>Ego::CompareFunction::AlwaysPass:
	 *		The incoming alpha value always passes (regardless of the reference alpha value)</li>
	 *		<li>Ego::CompareFunction::Less:
	 *		The incoming alpha value passes if it is
	 *		less than the reference alpha value</li>
	 *		<li>Ego::CompareFunction::LessOrEqual:
	 *		The incoming alpha value passes if it is
	 *		less than or equal to the reference alpha value</li>
	 *		<li>Ego::CompareFunction::Equal:
	 *		The incoming alpha value passes if it is
	 *		equal to the reference alpha value</li>
	 *		<li>Ego::CompareFunction::NotEqual:
	 *		The incoming alpha value passes if it is
	 *		not equal to the reference alpha value</li>
	 *		<li>Ego::CompareFunction::Greater:
	 *		The incoming alpha value passes if it is
	 *		greater than the reference alpha value</li>
	 *		<li>Ego::CompareFunction::GreaterOrEqual:
	 *		The incoming alpha value passes if it is
	 *		greater than or equal to the reference alpha value</li>
	 *	</ul>
	 * @remark
	 *	The initial alpha comparison function is Ego::CompareFunction::Always,
	 *	the initial reference alpha value is @a 0.
	 * @throw std::invalid_argument
	 *	if @a value is not within the bounds of @a 0.0f (inclusive) and @a 1.0f (inclusive).
	 */
	virtual void setAlphaFunction(CompareFunction function, float value) = 0;

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
     *  Set the depth compare function.
     * @param function
     *  the depth compare function
	 * @remark
	 *	The depth compare function is used to compare the depth value of an incoming pixel to the depth value present in the depth buffer.
	 *	The outcome of this test is binary, either the incoming pixel passes or fails the comparison.
	 *
	 *	The depth compare functions are as follows:
	 *	<ul>
	 *		<li>Ego::CompareFunction::AlwaysFail:
	 *		The incoming pixel never passes</li>
	 *		<li>Ego::CompareFunction::AlwaysPass:
	 *		The incoming pixel always passes</li>
	 *		<li>Ego::CompareFunction::Less:
	 *		The incoming pixel passes if it is depth value is
	 *		less than the depth value present in the depth buffer</li>
	 *		<li>Ego::CompareFunction::LessOrEqual:
	 *		The incoming pixel passes if its depth value is
	 *		less than or equal to the depth value present in the depth buffer</li>
	 *		<li>Ego::CompareFunction::Equal:
	 *		The incoming pixel passes if its depth value is
	 *		equal to the depth value present in the depth buffer</li>
	 *		<li>Ego::CompareFunction::NotEqual:
	 *		The incoming pixel passes if its depth value is
	 *		not equal to the depth value present in the depth buffer</li>
	 *		<li>Ego::CompareFunction::Greater:
	 *		The incoming pixel passes if its depth value is
	 *		greater than the depth value present in the depth buffer</li>
	 *		<li>Ego::CompareFunction::GreaterOrEqual:
	 *		The incoming pixel passes if its depth value is
	 *		greater than or equal to the depth value present in the depth buffer</li>
	 *	</ul>
	 * @warning
	 *	If depth testing is disabled, then the pixel always passes. However, the depth buffer is not modified.
	 *	To unconditionally write to the depth buffer, depth testing should be set to enabled and the depth test
	 *	function to always pass. 
     */
    virtual void setDepthFunction(CompareFunction function) = 0;

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
     *  Enable/disable the depth buffer writes.
     * @param enable
     *	@a true enables scissor tests,
     *	@a false disables then
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
    *  Enable/disable perspective correction.
    * @param enable
    *  @a true enables perspective correction,
    *  @a false disables it
    */
    virtual void setPerspectiveCorrectionEnabled(bool enabled)  = 0;

    /**
     * @brief
     *  Enable/disable dithering.
     * @param enable
     *  @a true enables dithering,
     *  @a false disables it
     */
    virtual void setDitheringEnabled(bool enabled)  = 0;

	/**
	 * @brief
	 *	Enable/disable antialiasing of points.
	 * @param enable
	 *	@a true enables antialiasing of points,
	 *	@a false disables it
	 * @remark
	 *	Enabling/disabling antialiasing of points has no impact if
	 *	multisampling is enabled (cfg. "OpenGL 1.3 Specification", sec. 3.3.3).
	 */
	virtual void setPointSmoothEnabled(bool enabled) = 0;

	/**
	 * @brief
	 *	Enable/disable antialiasing of lines.
	 * @param enabled
	 *	@a true enables antialiasing of lines,
	 *	@a false disables it
	 * @remark
	 *	Enabling/disabling antialiasing of lines has no impact if
	 *	multisampling is enabled (cfg. "OpenGL 1.3 Specification", sec. 3.4.4).
	 */
	virtual void setLineSmoothEnabled(bool enabled) = 0;

	/**
	 * @brief
	 *	Enable/disable antialiasing of lines.
	 * @param enabled
	 *	@a true enables antialiasing of lines,
	 *	@a false disables it
	 * @remark
	 *	Enabling/disabling antialiasing of polygons has no impact if
	 *	multisampling is enabled (cfg. "OpenGL 1.3 Specification", sec. 3.5.6).
	 */
	virtual void setPolygonSmoothEnabled(bool enabled) = 0;

    /**
     * @brief
     *  Enable/disable multisamples.
     * @param enabled
     *  @a true enables multisamples,
     *  @a false disables it
     */
    virtual void setMultisamplesEnabled(bool enabled) = 0;

	/**
	 * @brief
	 *	Enable/disable lighting.
	 * @param enabled
	 *	@a true enables lighting,
	 *	@a false disables it
	 */
	virtual void setLightingEnabled(bool enabled) = 0;

	/**
	 * @brief
	 *	Set the rasterization mode (for front- and back-facing polygons).
	 * @param mode
	 *	the rasterization mode
	 */
	virtual void setRasterizationMode(RasterizationMode mode) = 0;

    /**
     * @brief
     *  Enable/disable Gouraud shading.
     * @param enable
     *  @a true enables Gouraud shading,
     *  @a false disables it
     */
    virtual void setGouraudShadingEnabled(bool enabled) = 0;

    /**
     * @brief
     *  Render a vertex buffer.
     * @param primitiveType
     *  the primitive type
     * @param vertexBuffer
     *  a pointer to a vertex buffer
     * @param index
     *  the index of the first vertex to render
     * @param length
     *  the number of vertices to render
     * @throw std::invalid_argument
     *  if <tt>index + length</tt> is greater than the number of vertices in the vertex buffer
     * @throw std::invalid_argument
     *  if @a length is non-zero and
     *  - is not disible by 3 for the triangles primitive type or
     *  - is not divisible by 4 for the quadriliterals primitive type.
     */
    virtual void render(VertexBuffer& vertexBuffer, PrimitiveType primitiveType, size_t index, size_t length) = 0;

};

} // namespace Ego

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

#include "egolib/Math/_Include.hpp"
#include "egolib/Core/Singleton.hpp"
#include "egolib/Renderer/BlendFunction.hpp"
#include "egolib/Renderer/CompareFunction.hpp"
#include "egolib/Renderer/RasterizationMode.hpp"
#include "egolib/Renderer/CullingMode.hpp"
#include "egolib/Renderer/WindingMode.hpp"
#include "egolib/Renderer/PrimitiveType.hpp"
#include "egolib/Renderer/TextureSampler.hpp"
#include "egolib/Renderer/RendererInfo.hpp"
#include "egolib/Graphics/VertexBuffer.hpp"
#include "egolib/Renderer/Texture.hpp"

namespace Ego {

/**
 * @brief
 *  A facade for internal buffers like the accumulation buffer, the colour buffer or the depth buffer.
 */
template <typename DataType>
class BufferFacade : public Id::NonCopyable {
protected:
    /**
     * @brief
     *  Construct this buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    BufferFacade() {}

    /**
     * @brief
     *  Destruct this buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    virtual ~BufferFacade() {}

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
class AccumulationBuffer : public BufferFacade<Math::Colour4f> {
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

public:
    /**
     * @brief
     *  Get the colour depth of this accumulation buffer.
     * @return
     *  the colour depth of this accumulation buffer
     */
    virtual const ColourDepth& getColourDepth() = 0;

};

/**
 * @brief
 *  A facade for a colour buffer.
 */
class ColourBuffer : public BufferFacade<Math::Colour4f> {
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

public:
    /**
     * @brief
     *  Get the colour depth of this colour buffer.
     * @return
     *  the colour depth of this colour buffer
     */
    virtual const ColourDepth& getColourDepth() = 0;

};

/**
 * @brief
 *  A facade for an depth buffer.
 */
class DepthBuffer : public BufferFacade<float> {
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

public:
    /**
     * @brief
     *  Get the depth of this depth buffer.
     * @return
     *  the depth of this depth buffer
     */
    virtual uint8_t getDepth() = 0;

};

/**
 * @brief
 *  A facade for a stencil buffer.
 */
class StencilBuffer : public BufferFacade<float> {
protected:
    /**
     * @brief
     *  Construct this stencil buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    StencilBuffer();

    /**
     * @brief
     *  Destruct this stencil buffer (facade).
     * @remark
     *  Intentionally protected.
     */
    virtual ~StencilBuffer();

public:
    /**
     * @brief
     *  Get the depth of this stencil buffer.
     * @return
     *  the depth of this stencil buffer
     */
    virtual uint8_t getDepth() = 0;

};

/**
 * @brief
 *  A facade for a texture unit.
 */
class TextureUnit {

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
    virtual void setActivated(const Texture *texture) = 0;

};

class Renderer;

namespace Core {

/**
 * @brief Creator functor creating the back-end.
 */
template <>
struct CreateFunctor<Renderer> {
    Renderer *operator()() const;
};

} // namespace Core

class Renderer : public Core::Singleton<Renderer> {
protected:
    friend Core::Singleton<Renderer>::CreateFunctorType;
    friend Core::Singleton<Renderer>::DestroyFunctorType;

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
    /**
     * @brief
     *  Get information about the renderer.
     * @return
     *  information about the renderer
     */
    virtual const RendererInfo& getInfo() = 0;

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
     *  Get the stencil buffer (facade).
     * @return
     *  the stencil buffer (facade)
     */
    virtual StencilBuffer& getStencilBuffer() = 0;

    /**
     * @brief
     *  Get the texture unit (facade).
     * @return
     *  the texture unit (facade)
     */
    virtual TextureUnit& getTextureUnit() = 0;

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
     *  Set the alpha compare function.
     * @param function
     *  the alpha compare function
     * @param value
     *  the reference alpha value that incoming alpha values are compared to.
     *  Must be within the bounds of @a 0.0f (inclusive) and @a 1.0f (inclusive).
     * @remark
     *  The alpha compare function is used to compare the alpha value of an incoming fragment to the reference alpha value.
     *  The outcome of this test is binary, either the incoming fragment passes or fails the comparison.
     *
     *  The alpha compare functions are as follows:
     *  <ul>
     *  	<li>Ego::CompareFunction::AlwaysFail:
     *  	The incoming alpha value never passes (regardless of the reference alpha value)</li>
     *  	<li>Ego::CompareFunction::AlwaysPass:
     *  	The incoming alpha value always passes (regardless of the reference alpha value)</li>
     *  	<li>Ego::CompareFunction::Less:
     *  	The incoming alpha value passes if it is
     *  	less than the reference alpha value</li>
     *  	<li>Ego::CompareFunction::LessOrEqual:
     *  	The incoming alpha value passes if it is
     *  	less than or equal to the reference alpha value</li>
     *  	<li>Ego::CompareFunction::Equal:
     *  	The incoming alpha value passes if it is
     *  	equal to the reference alpha value</li>
     *  	<li>Ego::CompareFunction::NotEqual:
     *  	The incoming alpha value passes if it is
     *  	not equal to the reference alpha value</li>
     *  	<li>Ego::CompareFunction::Greater:
     *  	The incoming alpha value passes if it is
     *  	greater than the reference alpha value</li>
     *  	<li>Ego::CompareFunction::GreaterOrEqual:
     *  	The incoming alpha value passes if it is
     *  	greater than or equal to the reference alpha value</li>
     *  </ul>
     * @remark
     *  The initial alpha compare function is Ego::CompareFunction::Always,
     *  the initial reference alpha value is @a 0.
     * @throw std::invalid_argument
     *  if @a value is not within the bounds of @a 0.0f (inclusive) and @a 1.0f (inclusive).
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
     *  Set the blend function.
     * @param sourceColour
     *  the source color blend function
     * @param sourceAlpha
     *  the source alpha blend function
     * @param destinationColour
     *  the destination color blend function
     * @param destinationAlpha
     *  the destination alpha blend function
     *  the source blend function
     * @remark
     *  In the following text,                        first source, second source and destination color components are referred to as
     *  \f$\left(R_{s_0}, G_{s_0}, B_{s_0}, A_{s_0}\right)\f$,     \f$\left(R_{s_1}, G_{s_1}, B_{s_1}, A_{s_1}\right)\f$ and \f$\left
     *  (R_d, G_d, B_d, A_d\right)\f$, respectively. The color specified by Ego::Renderer::setBlendColour(const Ego::Math::Colour4f&)
     *  is referred to as \f$\left(R_c, G_c, B_c, A_c\right)\f$. The first column in the following table denotes the parameter value,
     *  the second and third column the semantics of that value if assigned to the colour and alpha parameter       (either source or
     *  destination), respectively.
     *  <table>
     *  <tr><td>Parameter                                </td><td>RGB                                      </td> <td>A                </td></tr>
     *
     *  <tr><td>Ego::BlendFunc::Zero                     </td><td>\f$(0,0,0)\f$                            </td> <td>\f$0\f$          </td></tr>
     *  <tr><td>Ego::BlendFunc::One                      </td><td>\f$(1,1,1)\f$                            </td> <td>\f$1\f$          </td></tr>
     *
     *  <tr><td>Ego::BlendFunc::SourceColour             </td><td>\f$(R_{s_0},G_{s_0},B_{s_0})\f$          </td> <td>\f$A_{s_0}\f$    </td></tr>
     *  <tr><td>Ego::BlendFunc::OneMinusSourceColour     </td><td>\f$(1, 1, 1) - (R_{s_0},G_{s_0}, B_{s_0})</td> <td>\f$1 - A_{s_0}\f$</td></tr>
     *
     *  <tr><td>Ego::BlendFunc::DestinationColour        </td><td>\f$(R_d, G_d, B_d)\f$                    </td> <td>\f$A_d\f$        </td></tr>
     *  <tr><td>Ego::BlendFunc::OneMinusDestinationColour</td><td>\f$(1, 1, 1) - (R_d, G_d, B_d)\f$        </td> <td>\f$1 - A_d\f$    </td></tr>
     *
     *  <tr><td>Ego::BlendFunc::SourceAlpha              </td><td>\f$(A_{s_0}, A_{s_0}, A_{s_0})\f$         </td><td>\f$A_{s_0}\f$    </td></tr>
     *  <tr><td>Ego::BlendFunc::OneMinusSourceAlpha      </td><td>\f$(1, 1, 1) - (A_{s_0}, A_{s_0}, A_{s_0})</td><td>\f$1 - A_{s_0}\f$</td></tr>
     *
     *  <tr><td>Ego::BlendFunc::DestinationAlpha         </td><td>\f$(A_d, A_d, A_d)\f$                     </td><td>\f$A_d\f$        </td></tr>
     *  <tr><td>Ego::BlendFunc::OneMinusDestinationAlpha </td><td>\f$(1, 1, 1) - (A_d, A_d, A_d)\f$         </td><td>\f$1 - A_d\f$    </td></tr>
     *
     *  <tr><td>Ego::BlendFunc::ConstantColour           </td><td>\f$(R_c, G_c, B_c\f$)                     </td><td>\f$A_c\f$        </td></tr>
     *  <tr><td>Ego::BlendFunc::OneMinusConstantColour   </td><td>\f$(1, 1, 1) - (R_c, G_c, B_c)\f$         </td><td>\f$1 - A_c\f$    </td></tr>
     *
     *  <tr><td>Ego::BlendFunc::ConstantAlpha            </td><td>\f$(A_c, A_c, A_c)\f$                     </td><td>\f$A_c\f$        </td></tr>
     *  <tr><td>Ego::Blendfunc::OneMinusConstantAlpha    </td><td>\f$(1, 1, 1) - (A_c, A_c, A_c)\f$         </td><td>\f$1 - A_c\f$    </td></tr>
     *
     *  <tr><td>Ego::BlendFunc::SourceAlphaSaturate      </td><td>\f$(i, i, i)\f$                           </td><td>\f$1\f$          </td></tr>
     *  </table>
     *  where
     *  \f{align*}{
     *  i = \min\left(A_{s_0}, 1 - A_d\right)
     *  \f}
     */
    virtual void setBlendFunction(BlendFunction sourceColour, BlendFunction sourceAlpha,
                                  BlendFunction destinationColour, BlendFunction destinationAlpha) = 0;
    /**
     * @brief
     *  Set the blend function.
     * @param source
     *  the source colour and source alpha blend function
     * @param target
     *  the destination colour and destination alpha blend function
     * @remark
     *  This method is a convenience method and a call
     *  @code
     *  o.setBlendFunction(x,y);
     *  @endcode
     *  is effectively equivalent to a call
     *  @code
     *  o.setBlendFunction(x, x, y, y)
     *  @endcode
     */
    virtual void setBlendFunction(BlendFunction source, BlendFunction destination) {
        setBlendFunction(source, source, destination, destination);
    }

    /**
     * @brief
     *  Set the current colour.
     * @param colour
     *  the current colour
     */
    virtual void setColour(const Math::Colour4f& colour) = 0;

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
     *  The depth compare function is used to compare the depth value of an incoming pixel to the depth value present in the depth buffer.
     *  The outcome of this test is binary, either the incoming pixel passes or fails the comparison.
     *
     *  The depth compare functions are as follows:
     *  <ul>
     *  	<li>Ego::CompareFunction::AlwaysFail:
     *  	The incoming pixel never passes</li>
     *  	<li>Ego::CompareFunction::AlwaysPass:
     *  	The incoming pixel always passes</li>
     *  	<li>Ego::CompareFunction::Less:
     *  	The incoming pixel passes if it is depth value is
     *  	less than the depth value present in the depth buffer</li>
     *  	<li>Ego::CompareFunction::LessOrEqual:
     *  	The incoming pixel passes if its depth value is
     *  	less than or equal to the depth value present in the depth buffer</li>
     *  	<li>Ego::CompareFunction::Equal:
     *  	The incoming pixel passes if its depth value is
     *  	equal to the depth value present in the depth buffer</li>
     *  	<li>Ego::CompareFunction::NotEqual:
     *  	The incoming pixel passes if its depth value is
     *  	not equal to the depth value present in the depth buffer</li>
     *  	<li>Ego::CompareFunction::Greater:
     *  	The incoming pixel passes if its depth value is
     *  	greater than the depth value present in the depth buffer</li>
     *  	<li>Ego::CompareFunction::GreaterOrEqual:
     *  	The incoming pixel passes if its depth value is
     *  	greater than or equal to the depth value present in the depth buffer</li>
     *  </ul>
     * @warning
     *  If depth testing is disabled, then the pixel always passes. However, the depth buffer is not modified.
     *  To unconditionally write to the depth buffer, depth testing should be set to enabled and the depth test
     *  function to always pass.
     */
    virtual void setDepthFunction(CompareFunction function) = 0;

    /// @brief Enable/disable depth testss.
    /// @param enabled @a true enables depth tests, @a false disables them
    virtual void setDepthTestEnabled(bool enabled) = 0;

    /// @brief Enable/disable the depth buffer writes.
    /// @param enable @a true enables depth buffer writes, @a false disables then
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

    /// @brief Enable/disable scissor tests.
    /// @param enable @a true enables scissor tests, @a false disables then
    virtual void setScissorTestEnabled(bool enabled) = 0;

    /// @brief Set the stencil bit mask affecting back-facing polygons.
    /// @param mask the mask
    /// @see Ego::Renderer::setStencilMaskFront
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

    /// @brief Enable/disable stencil tests and stencil buffer updates.
    /// @param enable @a true enables stencil tests and stencil buffer updates, @a false disables them
    virtual void setStencilTestEnabled(bool enabled) = 0;

    /// @brief Set the viewport rectangle.
    /// @param left, bottom the left/bottom corner of the viewport rectangle
    /// @param width, height the width and height of the viewport rectangle
    /// @throw std::invalid_argument if @a width or @a height are smaller than @a 0
    virtual void setViewportRectangle(float left, float bottom, float width, float height) = 0;

    /// @brief Set the winding mode.
    /// @param mode the winding mode
    virtual void setWindingMode(WindingMode mode) = 0;


    /**
     * @brief
     *  Multiply the current matrix with the given matrix.
     * @param matrix
     *  the matrix
     */
    virtual void multiplyMatrix(const Matrix4f4f& matrix) = 0;

    /// @brief Enable/disable perspective correction.
    /// @param enable @a true enables perspective correction, @a false disables it
    virtual void setPerspectiveCorrectionEnabled(bool enabled) = 0;

    /// @brief Enable/disable dithering.
    /// @param enable @a true enables dithering, @a false disables it
    virtual void setDitheringEnabled(bool enabled) = 0;

    /// @brief Enable/disable antialiasing of points.
    /// @param enable @a true enables antialiasing of points, @a false disables it
    /// @remark
    /// Enabling/disabling antialiasing of points has no impact if
    /// multisampling is enabled (cfg. "OpenGL 1.3 Specification", sec. 3.3.3).
    virtual void setPointSmoothEnabled(bool enabled) = 0;

    /// @brief Enable/disable antialiasing of lines.
    /// @param enabled @a true enables antialiasing of lines, @a false disables it
    /// @remark
    /// Enabling/disabling antialiasing of lines has no impact if
    /// multisampling is enabled (cfg. "OpenGL 1.3 Specification", sec. 3.4.4).
    virtual void setLineSmoothEnabled(bool enabled) = 0;

    /// @brief Set line width.
    /// @param width the line width
    virtual void setLineWidth(float width) = 0;

    /// @brief Set point size.
    /// @param size the point size
    virtual void setPointSize(float size) = 0;

    /// @brief Enable/disable antialiasing of lines.
    /// @param enabled @a true enables antialiasing of lines, @a false disables it
    /// @remark
    /// Enabling/disabling antialiasing of polygons has no impact if
    /// multisampling is enabled (cfg. "OpenGL 1.3 Specification", sec. 3.5.6).
    virtual void setPolygonSmoothEnabled(bool enabled) = 0;

    /// @brief Enable/disable multisamples.
    /// @param enabled @a true enables multisamples, @a false disables then
    virtual void setMultisamplesEnabled(bool enabled) = 0;

    /// @brief Enable/disable lighting.
    /// @param enabled @a true enables lighting, @a false disables it
    virtual void setLightingEnabled(bool enabled) = 0;

    /// @brief Set the rasterization mode (for front- and back-facing polygons).
    /// @param mode the rasterization mode
    virtual void setRasterizationMode(RasterizationMode mode) = 0;

    /// @brief Enable/disable Gouraud shading.
    /// @param enable @a true enables Gouraud shading, @a false disables it
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

    /**
     * @brief
     *  Create a texture.
     * @return
     *  the texture
     * @post
     *  The texture is the default texture.
     */
    virtual std::shared_ptr<Texture> createTexture() = 0;

private:
    Matrix4f4f m_projectionMatrix;
    Matrix4f4f m_viewMatrix;
    Matrix4f4f m_worldMatrix;

public:
    /// @brief Set the projection matrix.
    /// @param projectionMatrix the projection matrix
    virtual void setProjectionMatrix(const Matrix4f4f& projectionMatrix);
    /// @brief Get the projection matrix.
    /// @return the projection matrix
    /// @remark
    /// The default projection matrix is a perspective projection matrix
    /// with a field of view angle in the y direction of 45 degrees, an
    /// aspect ratio in the x-direction of 4/3 with the near clipping
    /// plane in distance 0.1 and the far clipping plane in distance 1000.
    virtual Matrix4f4f getProjectionMatrix() const;

    /// @brief Set the view matrix.
    /// @param viewMatrix the view matrix
    virtual void setViewMatrix(const Matrix4f4f& viewMatrix);
    /// @brief Get the view matrix.
    /// @return the view matrix
    /// @remark
    /// The default view matrix is the identity matrix.
    virtual Matrix4f4f getViewMatrix() const;

    /// @brief Set the world matrix.
    /// @param worldMatrix the world matrix
    virtual void setWorldMatrix(const Matrix4f4f& worldMatrix);
    /// @brief Get the world matrix.
    /// @return the world matrix
    /// @remark The default world matrix is the identity matrix.
    virtual Matrix4f4f getWorldMatrix() const;

};

} // namespace Ego

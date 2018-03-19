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

/// @file egolib/Renderer/OpenGL/Renderer.hpp
/// @brief Implementation of a renderer for OpenGL 2.1
/// @author Michael Heilmann

#pragma once

#include "egolib/Renderer/Renderer.hpp"
//#include "egolib/typedef.h"
#include "egolib/Renderer/OpenGL/AccumulationBuffer.hpp"
#include "egolib/Renderer/OpenGL/ColourBuffer.hpp"
#include "egolib/Renderer/OpenGL/DepthBuffer.hpp"
#include "egolib/Renderer/OpenGL/StencilBuffer.hpp"
#include "egolib/Renderer/OpenGL/TextureUnit.hpp"
#include "egolib/platform.h"
#include "egolib/Math/_Include.hpp"
#define GLEW_STATIC
#include <GL/glew.h>

namespace Ego::OpenGL {

// Forward declaration.
class DefaultTexture;
class RendererInfo;

class Renderer : public Ego::Renderer
{
public:
	// Befriend with the create functor.
	friend RendererCreateFunctor;

    /// @brief Unique pointer to the 1D default texture.
    std::unique_ptr<DefaultTexture> m_defaultTexture1d;

    /// @brief Unique pointer to the 2D default texture.
    std::unique_ptr<DefaultTexture> m_defaultTexture2d;

protected:  
    /// @brief The accumulation buffer facade.
    AccumulationBuffer m_accumulationBuffer;
    
    /// @brief The colour buffer facade.
    ColourBuffer m_colourBuffer;

    /// @brief The depth buffer facade.
    DepthBuffer m_depthBuffer;
    
    /// @brief The stencil buffer facade.
    StencilBuffer m_stencilBuffer;
    
    /// @brief The texture unit facade
    TextureUnit m_textureUnit;

    /// @brief Information on the renderer.
    std::shared_ptr<RendererInfo> m_info;

    /// @brief The set of OpenGL extensions supported by this OpenGL implementation.
    std::unordered_set<std::string> m_extensions;

    Renderer(const std::shared_ptr<RendererInfo>& info);

public:
    /// @brief Construct this OpenGL renderer.
    Renderer();

    /// @brief Destruct this OpenGL renderer.
    virtual ~Renderer();

public:
    /** @copydoc Ego::Renderer::getInfo() */
    virtual std::shared_ptr<Ego::RendererInfo> getInfo() override;

public:

    /** @copydoc Ego::Renderer::getAccumulationBuffer() */
    virtual Ego::AccumulationBuffer& getAccumulationBuffer() override;

    /** @copydoc Ego::Renderer::getColourBuffer */
    virtual Ego::ColourBuffer& getColourBuffer() override;

    /** @copydoc Ego::Renderer::getDepthBuffer() */
    virtual Ego::DepthBuffer& getDepthBuffer() override;

    /** @copydoc Ego::Renderer::getStencilBuffer() */
    virtual Ego::StencilBuffer& getStencilBuffer() override;

    /** @copydoc Ego::Renderer::getTextureUnit() */
    virtual Ego::TextureUnit& getTextureUnit() override;

    /** @copydoc Ego::Renderer::setAlphaTestEnabled */
    virtual void setAlphaTestEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setAlphaFunction */
    virtual void setAlphaFunction(idlib::compare_function function, single value) override;

    /** @copydoc Ego::Renderer::setBlendingEnabled */
    virtual void setBlendingEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setSourceBlendFunction */
    virtual void setBlendFunction(idlib::color_blend_parameter sourceColour, idlib::color_blend_parameter sourceAlpha,
		                          idlib::color_blend_parameter destinationColour, idlib::color_blend_parameter destinationAlpha) override;

    /** @copydoc Ego::Renderer::setColour */
    virtual void setColour(const Colour4f& colour) override;

    /** @copydoc Ego::Renderer::setCullingMode */
    virtual void setCullingMode(idlib::culling_mode mode) override;

    /** @copydoc Ego::Renderer::setDepthFunction */
    virtual void setDepthFunction(idlib::compare_function function) override;

    /** @copydoc Ego::Renderer::setDepthTestEnabled */
    virtual void setDepthTestEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setDepthWriteEnabled */
    virtual void setDepthWriteEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setScissorTestEnabled */
    virtual void setScissorTestEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setScissorRectangle */
    virtual void setScissorRectangle(single left, single bottom, single width, single height) override;

    /** @copydoc Ego::Renderer::setStencilMaskBack */
    virtual void setStencilMaskBack(uint32_t mask) override;

    /** @copydoc Ego::Renderer::setStencilMaskFront */
    virtual void setStencilMaskFront(uint32_t mask) override;

    /** @copydoc Ego::Renderer::setStencilTestEnabled */
    virtual void setStencilTestEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setViewportRectangle */
    virtual void setViewportRectangle(single left, single bottom, single width, single height) override;

    /** @copydoc Ego::Renderer::setWindingMode */
    virtual void setWindingMode(idlib::winding_mode mode) override;

    /** @copydoc Ego::Renderer::multMatrix */
    virtual void multiplyMatrix(const idlib::matrix_4s4s& matrix) override;

    /** @copydoc Ego::Renderer::setPerspectiveCorrectionEnabled */
    virtual void setPerspectiveCorrectionEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setDitheringEnabled  */
    virtual void setDitheringEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setPointSmoothEnabled */
    virtual void setPointSmoothEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setLineSmoothEnabled */
    virtual void setLineSmoothEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setLineWidth */
    virtual void setLineWidth(single width) override;

    /** @copydoc Ego::Renderer::setPointSize */
    virtual void setPointSize(single size) override;

    /** @copydoc Ego::Renderer::setPolygonSmoothEnabled */
    virtual void setPolygonSmoothEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setMultisamplesEnabled */
    virtual void setMultisamplesEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setLightingEnabled */
    virtual void setLightingEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::setRasterizationMode */
    virtual void setRasterizationMode(idlib::rasterization_mode mode) override;

    /** @copydoc Ego::Renderer::setGouraudShadingEnabled */
    virtual void setGouraudShadingEnabled(bool enabled) override;

    /** @copydoc Ego::Renderer::render */
    virtual void render(idlib::vertex_buffer& vertexBuffer, const idlib::vertex_descriptor& vertexDescriptor, idlib::primitive_type primitiveType, size_t index, size_t length) override;

    /** @copydoc Ego::Renderer::createTexture */
    virtual std::shared_ptr<Ego::Texture> createTexture() override;

public:
    /** @copydoc Ego::Renderer::setProjectionMatrix */
    void setProjectionMatrix(const idlib::matrix_4s4s& projectionMatrix) override;

    /** @copydoc Ego::Renderer::setViewMatrix */
    void setViewMatrix(const idlib::matrix_4s4s& viewMatrix) override;

    /** @copydoc Ego::Renderer::setWorldMatrix */
    void setWorldMatrix(const idlib::matrix_4s4s& worldMatrix) override;

private:
    std::array<single, 16> toOpenGL(const idlib::matrix_4s4s& source);
    GLenum toOpenGL(idlib::color_blend_parameter source);

}; // class Renderer

} // namespace Ego::OpenGL

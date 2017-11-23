#pragma once

#include "egolib/Graphics/ColourDepth.hpp"
#include "egolib/Renderer/TextureAddressMode.hpp"
#include "egolib/Renderer/TextureFilter.hpp"
#include "egolib/Renderer/TextureType.hpp"
#include "egolib/Renderer/TextureSampler.hpp"
#include "egolib/Graphics/PixelFormat.hpp"

namespace Ego {
namespace OpenGL {

// Forward declaration.
class RendererInfo;

class Utilities2
{
public:
    /// @brief Clear the OpenGL error flag.
    static void clearError();

    /// @brief If the OpenGL error flag is set, log a description of the error as a warning, and clear the error flag.
    /// @param raise if @a true, an std::runtime_error is raised if the OpenGL error flag wa set
    /// @return @a true if the OpenGL error flag was set, @a false otherwise
    static bool isError();

    /// @brief Get the set of extension strings of extensions supported by this OpenGL implementation.
    /// @return the set of extension strings
    static std::unordered_set<std::string> getExtensions();

    /// @brief Get the name of the renderer of this OpenGL implementation.
    /// @return the name of the renderer of this OpenGL implementation
    static std::string getRenderer();

    /// @brief Get the name of the vendor of this OpenGL implementation.
    /// @return the name of the vendor of this OpenGL implementation
    static std::string getVendor();

    /// @brief Get the version of this OpenGL implementation.
    /// @return the version of this OpenGL implementation
    static std::string getVersion();

    /// @brief Get the depth buffer depth.
    /// @return the depth buffer depth
    static uint8_t getDepthBufferDepth();

    /// @brief Get the stencil buffer depth.
    /// @return the stencil buffer depth
    static uint8_t getStencilBufferDepth();

    /// @brief Get the colour buffer colour depth.
    /// @return the colour buffer colour depth
    static ColourDepth getColourBufferColourDepth();

    /// @brief Get the accumulation colour depth.
    /// @return the accumulation colour depth
    static ColourDepth getAccumulationBufferColourDepth();

    /// @brief Map an internal texture address mode to an OpenGL texture address mode.
    /// @param textureAddressMode the internal texture address mode
    /// @return the OpenGL texture address mode
    /// @remark The mapping from internal texture address modes to OpenGL texture address modes is depicted in the table below: 
    /// internal                                | OpenGL
    /// --------------------------------------- | ------------------
    /// Ego::TextureAddressMode::Clamp          | GL_CLAMP
    /// Ego::TextureAddressMode::Cleamp         | GL_CLAMP
    /// Ego::TextureAddressMode::ClampToBorder  | GL_CLAMP_TO_BORDER
    /// Ego::TextureAddressMode::ClampToEdge    | GL_CLAMP_TO_EDGE
    /// Ego::TextureAddressMode::Repeat         | GL_REPEAT
    /// Ego::TextureAddressMode::RepeatMirrored | GL_MIRRORED_REPEAT
    static GLint toOpenGL(TextureAddressMode textureAddressMode);

    /// @brief Map an internal primitive type to an OpenGL primitive type.
    /// @param primitive type the internal primitive type
    /// @return the OpenGL primitive type
    /// @remark The mapping from internal primitive types to OpenGL primiive types is depicted in the table below:
    /// internal                                | OpenGL
    /// --------------------------------------- | -----------------
    /// id::primitive_typee::triangles          | GL_TRIANGLES
    /// id::primitive_type::quadriliterals      | GL_QUADS
    /// id::primitive_type::triangle_Fan        | GL_TRIANGLE_FAN
    /// id::primitive_type::triangle_strip      | GL_TRIANGLE_STRIP
    /// id::primitive_type::quadriliteral_strip | GL_QUAD_STRIP
    static GLenum toOpenGL(id::primitive_type primitiveType);

    /// @brief Get if anisotropy is supported.
    /// @return @a true if anisotropy is supported, @a false otherwise
    static bool isAnisotropySupported();

    /// @brief Get the maximum supported anisotropy.
    /// @return the maximum supported anisotropy
    /// @remark The maximum supported anisotropy is greater than or equal to the minimum supported anisotropy.
    /// @throw id::runtime_error anisotropy is not supported.
    static float getMaximumSupportedAnisotropy();

    /// @brief Get the minimum supported level of anisotropy.
    /// @return the minimum supported level of anisotropy
    /// @remark The minimum supported anisotropy is 1.0.
    /// @throw id::runtime_error anisotropy is not supported
    static float getMinimumSupportedAnisotropy();

    /// @brief Upload a 1D texture.
    /// @param pdf the pixel descriptor describing the format of a pixels
    /// @param w the width of the pixel rectangle
    /// @param data a pointer to the pixels
    static void upload_1d(const PixelFormatDescriptor& pfd, GLsizei w, const void *data);
    
    /// @brief Upload a 2D texture.
    /// @param pdf the pixel descriptor describing the format of a pixels
    /// @param w, h the width and height of the pixel rectangle
    /// @param data a pointer to the pixels
    static void upload_2d(const PixelFormatDescriptor& pfd, GLsizei w, GLsizei h, const void *data);
    
    /// @brief Upload a 2D texture generating the mipmaps.
    /// @param pdf the pixel descriptor describing the format of a pixels
    /// @param w, h the width and height of the pixel rectangle
    /// @param data a pointer to the pixels
    static void upload_2d_mipmap(const PixelFormatDescriptor& pfd, GLsizei w, GLsizei h, const void *data);

    static void toOpenGL(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMapFilter, GLint& minFilter_gl, GLint& magFilter_gl);

    static void toOpenGL(const PixelFormatDescriptor& pfd, GLenum& internalFormat_gl, GLenum& format_gl, GLenum& type_gl);

    static void setSampler(const std::shared_ptr<RendererInfo>& info, TextureType type, const TextureSampler& sampler);
};

} // namespace OpenGL
} // namespace Ego

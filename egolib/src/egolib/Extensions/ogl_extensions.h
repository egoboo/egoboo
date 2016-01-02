//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
//*    distributed with Egoboo.
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

/// @defgroup _ogl_extensions_ Extensions to OpenGL

/// @file egolib/Extensions/ogl_extensions.h
/// @ingroup _ogl_extensions_
/// @brief Definitions for extended functions and variables for OpenGL
/// @details

#pragma once

#include "egolib/file_common.h"
#include "egolib/egoboo_setup.h"
#include "egolib/Renderer/TextureAddressMode.hpp"
#include "egolib/Renderer/TextureType.hpp"
#include "egolib/Graphics/PixelFormat.hpp"
#include "egolib/Renderer/PrimitiveType.hpp"
#include "egolib/Renderer/CullingMode.hpp"
#include "egolib/Renderer/WindingMode.hpp"
#include "egolib/Renderer/TextureSampler.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// wrapper for uploading texture information

namespace Ego
{
namespace OpenGL
{

template <typename T> using UnorderedSet = std::unordered_set<T>;

struct Utilities {
protected:
    using RuntimeErrorException = Id::RuntimeErrorException;
    using UnhandledSwitchCaseException = Id::UnhandledSwitchCaseException;
    using String = std::string;

protected:
    static const String anisotropyExtension;

public:

    /**
     * @brief
     *  Get the depth buffer depth.
     * @return
     *  the depth buffer depth
     */
    static uint8_t getDepthBufferDepth();

    /**
     * @brief
     *  Get the stencil buffer depth.
     * @return
     *  the stencil buffer depth
     */
    static uint8_t getStencilBufferDepth();

    /**
     * @brief
     *  Get the colour buffer colour depth.
     * @return
     *  the colour buffer colour depth
     */
    static ColorDepth getColourBufferColourDepth();

    /**
     * @brief
     *  Get the accumulation colour depth.
     * @return
     *  the accumulation colour depth
     */
    static ColorDepth getAccumulationBufferColourDepth();

public:
    /**
     * @brief
     *  Get the set of extension strings of extensions supported by this OpenGL implementation.
     * @return
     *  the set of extension strings
     */
    static UnorderedSet<String> getExtensions();

public:
    /**
     * @brief
     *  Get the name of the renderer of this OpenGL implementation.
     * @return
     *  the name of the renderer of this OpenGL implementation
     */
    static String getRenderer();

    /**
     * @brief
     *  Get the name of the vendor of this OpenGL implementation.
     * @return
     *  the name of the vendor of this OpenGL implementation
     */
    static String getVendor();

    /**
     * @brief
     *  Get the version of this OpenGL implementation.
     * @return
     *  the version of this OpenGL implementation
     */
    static String getVersion();

public:
    /**
     * @brief Get the maximal level of anisotropy.
     * @return the maximal level of anisotropy
     * @remark The smallest maximal level of anisotropy is 1.0f.
     */
    static float getMaxAnisotropy();

    /**
     * @brief Get the minimal level of anisotropy.
     * @return the minimal level of anisotropy
     * @remark The smallest minimal level of anisotropy is 1.0f.
     */
    static float getMinAnisotropy();

public:
    /**
     * @brief
     *  Clear the OpenGL error flag.
     */
    static void clearError();

    /**
     * @brief
     *  If the OpenGL error flag is set, log a description of the error as a warning, and clear the error flag.
     * @param raise
     *  if @a true, an std::runtime_error is raised if the OpenGL error flag wa set
     * @return
     *  @a true if the OpenGL error flag was set, @a false otherwise
     */
    static bool isError();

    /**
     * @brief Upload a 1D texture.
     * @param pdf the pixel descriptor describing the format of a pixels
     * @param w the width of the pixel rectangle
     * @param data a pointer to the pixels
     */
    static void upload_1d(const PixelFormatDescriptor& pfd, GLsizei w, const void *data);
    /**
     * @brief Upload a 2D texture.
     * @param pdf the pixel descriptor describing the format of a pixels
     * @param w, h the width and height of the pixel rectangle
     * @param data a pointer to the pixels
     */
    static void upload_2d(const PixelFormatDescriptor& pfd, GLsizei w, GLsizei h, const void *data);
    /**
     * @brief Upload a 2D texture generating the mipmaps.
     * @param pdf the pixel descriptor describing the format of a pixels
     * @param w, h the width and height of the pixel rectangle
     * @param data a pointer to the pixels
     */
    static void upload_2d_mipmap(const PixelFormatDescriptor& pfd, GLsizei w, GLsizei h, const void *data);

    /**
     * @brief
     *  Translate an internal texture adress mode into an OpenGL texture address mode.
     * @param textureAddressMode
     *  the internal texture address mode
     * @return
     *  the OpenGL texture address mode.
     *  internal                                | OpenGL
     *  --------------------------------------- | -------------
     *  Ego::TextureAddressMode::Clamp          | @a GL_CLAMP
     *  Ego::TextureAddressMode::Cleamp         | @a GL_CLAMP
     *  Ego::TextureAddressMode::ClampToBorder  | @a GL_CLAMP_TO_BORDER
     *  Ego::TextureAddressMode::ClampToEdge    | @a GL_CLAMP_TO_EDGE
     *  Ego::TextureAddressMode::Repeat         | @a GL_REPEAT
     *  Ego::TextureAddressMode::RepeatMirrored | @a GL_MIRRORED_REPEAT
     */
    static GLint toOpenGL(TextureAddressMode textureAddressMode);
    /**
     * @brief
     *  Translate an internal primitive type into an OpenGL primitive type.
     * @param primitive type
     *  the internal primitive type
     * @return
     *  the OpenGL texture address mode.
     *  internal                                | OpenGL
     *  --------------------------------------- | -------------
     *  Ego::PrimitiveType::Triangles           | @a GL_TRIANGLES
     *  Ego::PrimitiveType::Quadriliterals      | @a GL_QUADS
     *  Ego::PrimitiveType::TriangleFan         | @a GL_TRIANGLE_FAN
     *  Ego::PrimitiveType::TriangleStrip       | @a GL_TRIANGLE_STRIP
     *  Ego::PrimitiveType::QuadriliteralStrip  | @a GL_QUAD_STRIP
     */
    static GLenum toOpenGL(PrimitiveType primitiveType);

    static void toOpenGL(TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMapFilter, GLint& minFilter_gl, GLint& magFilter_gl);

    static void toOpenGL(const PixelFormatDescriptor& pfd, GLenum& internalFormat_gl, GLenum& format_gl, GLenum& type_gl);


    static void setSampler(TextureType type, const TextureSampler& sampler);
    static void bind(GLuint id, TextureType type, TextureAddressMode textureAddressModeS, TextureAddressMode textureAddressModeT);

    static const std::string& toString(GLenum error);

};

struct PushAttrib {
private:
    GLbitfield bitfield;
public:
    PushAttrib(GLbitfield bitfield) : bitfield(bitfield) {
        glPushAttrib(bitfield);
    }
    ~PushAttrib() {
        glPopAttrib();
        Utilities::isError();
    }
}; // struct PushAttrib

struct PushClientAttrib {
private:
    GLbitfield bitfield;
public:
    PushClientAttrib(GLbitfield bitfield) : bitfield(bitfield) {
        glPushClientAttrib(bitfield);
    }
    ~PushClientAttrib() {
        glPopClientAttrib();
        Utilities::isError();
    }
}; // struct PushClientAttrib

} // namespace OpenGL
} // namespace Ego




//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// OpenGL graphics info
struct oglx_caps_t
{
    std::unordered_set<std::string> gl_extensions;

    // stack depths
    GLint max_modelview_stack_depth;     ///< Maximum modelview-matrix stack depth
    GLint max_projection_stack_depth;    ///< Maximum projection-matrix stack depth
    GLint max_texture_stack_depth;       ///< Maximum depth of texture matrix stack
    GLint max_name_stack_depth;          ///< Maximum selection-name stack depth
    GLint max_attrib_stack_depth;        ///< Maximum depth of the attribute stack
    GLint max_client_attrib_stack_depth; ///< Maximum depth of the client attribute stack

    // Antialiasing settings
    GLint   subpixel_bits;           ///< Number of bits of subpixel precision in x and y
    GLfloat point_size_range[2];     ///< Range (low to high) of antialiased point sizes
    GLfloat point_size_granularity;  ///< Antialiased point-size granularity
    GLfloat line_width_range[2];     ///< Range (low to high) of antialiased line widths
    GLfloat line_width_granularity;  ///< Antialiased line-width granularity

    // display settings
    GLint     max_viewport_dims[2];  ///< Maximum viewport dimensions
    GLboolean aux_buffers;           ///< Number of auxiliary buffers
    GLboolean rgba_mode;             ///< True if color buffers store RGBA
    GLboolean index_mode;            ///< True if color buffers store indices
    GLboolean doublebuffer;          ///< True if front and back buffers exist
    GLboolean stereo;                ///< True if left and right buffers exist
    GLint     index_bits;            ///< Number of bits per index in color buffers

    // Misc
    GLint max_lights;                    ///< Maximum number of lights
    GLint max_clip_planes;               ///< Maximum number of user clipping planes
    GLint max_texture_size;              ///< See discussion in "Texture Proxy" in Chapter 9

    GLint max_pixel_map_table;           ///< Maximum size of a glPixelMap() translation table
    GLint max_list_nesting;              ///< Maximum display-list call nesting
    GLint max_eval_order;                ///< Maximum evaluator polynomial order

    /// Is anisotropy supported?
    GLboolean anisotropic_supported;
    /// The maximum anisotropic filterings between @a 1 and @a 16.
    GLfloat   maxAnisotropy;
    GLfloat   log2Anisotropy;                    ///< Max levels of anisotropy

    oglx_caps_t();
    static void report(oglx_caps_t& self);
};

void oglx_Get_Screen_Info(oglx_caps_t *self);
void oglx_report_caps();

extern oglx_caps_t g_ogl_caps;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct oglx_video_parameters_t
{
    GLboolean antialiasing;         ///< current antialiasing value
    GLboolean multisample;          ///< whether multisampling is being supported through GL_MULTISAMPLE
    GLboolean multisample_arb;      ///< whether multisampling is being supported through GL_MULTISAMPLE_ARB
    GLenum perspective;             ///< current correction hint
    GLboolean dither;               ///< current dithering flag
    GLenum shading;                 ///< current shading type
    GLboolean anisotropy_enable;
    GLfloat anisotropy_levels;         ///< current value of the anisotropic filtering

    static void defaults(oglx_video_parameters_t& self);
    static void download(oglx_video_parameters_t& self, egoboo_config_t& cfg);
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct oglx_texture_parameters_t
{
    struct
    {
        Ego::TextureFilter minFilter;
        Ego::TextureFilter magFilter;
        Ego::TextureFilter mipMapFilter;
    } textureFilter;
    bool anisotropy_enable;
    float anisotropy_level;
    static void defaults(oglx_texture_parameters_t& self);
    static void download(oglx_texture_parameters_t& self, egoboo_config_t& cfg);
};

extern oglx_texture_parameters_t g_ogl_textureParameters;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void oglx_begin_culling(Ego::CullingMode cullingMode, Ego::WindingMode windingMode);

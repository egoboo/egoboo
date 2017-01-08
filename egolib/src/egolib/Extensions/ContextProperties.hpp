#pragma once

#include "egolib/Log/_Include.hpp"
#include "egolib/Graphics/ColourDepth.hpp"

namespace Ego {

/// @brief Used to pass information on multisampling of a graphics context from and to a backend.
/// @remark @todo Originally there was only multisample antialiasing (MSAA). Samples was designated
/// for the number of MSAA samples per pixel to allocate for that system framebuffer. Clean, simple.
/// Then the GPU vendors added support for supersample antialiasing (SSAA), coverage sample
/// antialiasing (CSAA), and combinations of the three.
struct MultisamplingProperties : public Id::EqualToExpr<MultisamplingProperties>
{
    /// @brief See MultisamplingProperties::setBuffers for more information.
    int buffers;

    /// @brief See MultisamplingProperties::setSamples for more information.
    int samples;

    /// @brief Construct these multisampling properties with default values.
    MultisamplingProperties();

    /// @brief Construct these multisampling properties with values of other multisampling properties.
    /// @param other the other multisampling properties
    MultisamplingProperties(const MultisamplingProperties& other);

    /// @brief Assign these multisampling properties with values of other multisampling properties.
    /// @param other the other multisampling properties
    /// @return these multisampling properties
    MultisamplingProperties& operator=(const MultisamplingProperties& other);

    /// @brief Set the number of multisampling samples.
    /// @param samples the number of multisampling samples
    /// @remark Maps to SDL_GL_MULTISAMPLESAMPLES in SDL.
    /// @default The default number of multisampling samples is 4.
    void setSamples(int samples);

    /// @brief Get the number of multisampling samples.
    /// @return the number of multisampling samples
    int getSamples() const;

    /// @brief Set the number of multisampling buffers.
    /// @param buffers the number of multisampling buffers
    /// @remark Maps to SDL_GL_MULTISAMPLEBUFFERS in SDL.
    /// @default The default number of multisampling buffers is 1.
    void setBuffers(int buffers);

    /// @brief Get the number of multisampling buffers.
    /// @return the number of multisampling buffers
    int getBuffers() const;

    /// @brief Upload the values of these multisampling properties to the backend.
    void upload() const;

    /// @brief Download the values of these multisampling properties from the backend.
    void download();

    // CRTP
    bool equalTo(const MultisamplingProperties& other) const;
};

Log::Entry& operator<<(Log::Entry& e, const MultisamplingProperties& s);

/// @brief Used to pass information on a graphics context from and to a backend.
struct ContextProperties : public Id::EqualToExpr<ContextProperties>
{
    /// The depth of the colour buffer.
    /// SDL_GL_RED_SIZE, SDL_GL_GREEN_SIZE, SDL_GL_BLUE_SIZE, SDL_GL_ALPHA_SIZE
    ColourDepth colourBufferDepth;
    /// The depth of the accumulation buffer.
    /// SDL_GL_ACCUM_RED_SIZE, SDL_GL_ACCUM_GREEN_SIZE, SDL_GL_ACCUM_BLUE_SIZE, SDL_GL_ACCUM_ALPHA_SIZE
    ColourDepth accumulationBufferDepth;
    /// The multisampling properties.
    MultisamplingProperties multisampling;
    int buffer_size;        ///< SDL_GL_BUFFER_SIZE
    int doublebuffer;       ///< SDL_GL_DOUBLEBUFFER
    int depthBufferDepth;         ///< SDL_GL_DEPTH_SIZE
    int stencilBufferDepth;       ///< SDL_GL_STENCIL_SIZE
    int stereo;             ///< SDL_GL_STEREO

    int accelerated_visual; ///< SDL_GL_ACCELERATED_VISUAL
    int swap_control;       ///< SDL_GL_SWAP_CONTROL

    /// @brief Construct these context properties with default values.
    ContextProperties();

    /// @brief Copy-construct these context properties with values of other context properties.
    /// @param other the other context properties
    ContextProperties(const ContextProperties& other);

    /// @brief Assign these context properties with values of other context properties.
    /// @param other the other context properties
    /// @return these context properties
    ContextProperties& operator=(const ContextProperties& other);

    static void validate(ContextProperties& self);
    
    /// Upload the values of these context properties to the backend.
    void upload() const;
    
    /// Download the values of these context properties from the backend.
    void download();

    // CRTP
    bool equalTo(const ContextProperties& other) const;
};

Log::Entry& operator<<(Log::Entry& e, const ContextProperties& s);

} // namespace Ego
